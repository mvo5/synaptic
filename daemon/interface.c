/* interface.c - varlink interface definition of synapticd
 *
 * Copyright (c) 2026 Michael Vogt <mvo@debian.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/* This file is C, not C++: the SD_VARLINK_DEFINE_* macros use
 * out-of-order designated initializers, which C++ rejects.
 * data/io.github.mvo5.synaptic.varlink is the textual form of this
 * definition and is checked against it by the test suite. */

#include "interface.h"

static SD_VARLINK_DEFINE_STRUCT_TYPE(
   FetchItem,
   SD_VARLINK_FIELD_COMMENT(
      "One file being downloaded, e.g. a package index. "
      "The id is stable across events for the same file."),
   SD_VARLINK_DEFINE_FIELD(id, SD_VARLINK_INT, 0),
   SD_VARLINK_DEFINE_FIELD(uri, SD_VARLINK_STRING, 0),
   SD_VARLINK_DEFINE_FIELD(description, SD_VARLINK_STRING, 0),
   SD_VARLINK_DEFINE_FIELD(short_description, SD_VARLINK_STRING, 0),
   SD_VARLINK_DEFINE_FIELD(size, SD_VARLINK_INT, 0));

static SD_VARLINK_DEFINE_STRUCT_TYPE(
   FetchWorker,
   SD_VARLINK_FIELD_COMMENT("Progress of one download in flight."),
   SD_VARLINK_DEFINE_FIELD_BY_TYPE(item, FetchItem, 0),
   SD_VARLINK_DEFINE_FIELD(current, SD_VARLINK_INT, 0),
   SD_VARLINK_DEFINE_FIELD(total, SD_VARLINK_INT, 0));

static SD_VARLINK_DEFINE_ENUM_TYPE(FetchEventKind,
                                   SD_VARLINK_DEFINE_ENUM_VALUE(start),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(fetch),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(hit),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(done),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(fail),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(pulse),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(stop));

static SD_VARLINK_DEFINE_STRUCT_TYPE(
   FetchEvent,
   SD_VARLINK_FIELD_COMMENT("Mirrors libapt's pkgAcquireStatus callbacks."),
   SD_VARLINK_DEFINE_FIELD_BY_TYPE(kind, FetchEventKind, 0),
   SD_VARLINK_FIELD_COMMENT("Set for fetch, hit, done and fail."),
   SD_VARLINK_DEFINE_FIELD_BY_TYPE(item, FetchItem, SD_VARLINK_NULLABLE),
   SD_VARLINK_FIELD_COMMENT("Set for fail."),
   SD_VARLINK_DEFINE_FIELD(error, SD_VARLINK_STRING, SD_VARLINK_NULLABLE),
   SD_VARLINK_FIELD_COMMENT("The remaining fields are set for pulse."),
   SD_VARLINK_DEFINE_FIELD(current_bytes, SD_VARLINK_INT, SD_VARLINK_NULLABLE),
   SD_VARLINK_DEFINE_FIELD(total_bytes, SD_VARLINK_INT, SD_VARLINK_NULLABLE),
   SD_VARLINK_DEFINE_FIELD(current_items, SD_VARLINK_INT, SD_VARLINK_NULLABLE),
   SD_VARLINK_DEFINE_FIELD(total_items, SD_VARLINK_INT, SD_VARLINK_NULLABLE),
   SD_VARLINK_DEFINE_FIELD(current_cps, SD_VARLINK_INT, SD_VARLINK_NULLABLE),
   SD_VARLINK_DEFINE_FIELD_BY_TYPE(workers,
                                   FetchWorker,
                                   SD_VARLINK_ARRAY | SD_VARLINK_NULLABLE));

static SD_VARLINK_DEFINE_ENUM_TYPE(SelectionAction,
                                   SD_VARLINK_DEFINE_ENUM_VALUE(install),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(remove),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(purge),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(keep));

static SD_VARLINK_DEFINE_STRUCT_TYPE(
   Selection,
   SD_VARLINK_FIELD_COMMENT(
      "The requested state of one package. The client has "
      "already resolved dependencies, so the list must be "
      "complete and consistent."),
   SD_VARLINK_DEFINE_FIELD(name, SD_VARLINK_STRING, 0),
   SD_VARLINK_DEFINE_FIELD(arch, SD_VARLINK_STRING, 0),
   SD_VARLINK_DEFINE_FIELD_BY_TYPE(action, SelectionAction, 0),
   SD_VARLINK_FIELD_COMMENT(
      "Required for install: the exact version to install."),
   SD_VARLINK_DEFINE_FIELD(version, SD_VARLINK_STRING, SD_VARLINK_NULLABLE),
   SD_VARLINK_FIELD_COMMENT("Mark the package as automatically installed."),
   SD_VARLINK_DEFINE_FIELD(auto, SD_VARLINK_BOOL, 0));

static SD_VARLINK_DEFINE_ENUM_TYPE(ConffilePolicy,
                                   SD_VARLINK_DEFINE_ENUM_VALUE(ask),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(keep),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(replace));

static SD_VARLINK_DEFINE_STRUCT_TYPE(
   CommitOptions,
   SD_VARLINK_FIELD_COMMENT("Only download the archives, do not run dpkg."),
   SD_VARLINK_DEFINE_FIELD(download_only, SD_VARLINK_BOOL, SD_VARLINK_NULLABLE),
   SD_VARLINK_FIELD_COMMENT(
      "Go on when some archives could not be downloaded."),
   SD_VARLINK_DEFINE_FIELD(fix_missing, SD_VARLINK_BOOL, SD_VARLINK_NULLABLE),
   SD_VARLINK_FIELD_COMMENT(
      "How dpkg handles changed configuration files. "
      "'ask' needs 'terminal', the question is answered there."),
   SD_VARLINK_DEFINE_FIELD_BY_TYPE(conffile, ConffilePolicy, 0),
   SD_VARLINK_FIELD_COMMENT(
      "Pass the dpkg terminal (pty master) as a file "
      "descriptor with the 'terminal' event. Otherwise its "
      "output is streamed as 'output' events."),
   SD_VARLINK_DEFINE_FIELD(terminal, SD_VARLINK_BOOL, SD_VARLINK_NULLABLE));

static SD_VARLINK_DEFINE_ENUM_TYPE(InstallEventKind,
                                   SD_VARLINK_DEFINE_ENUM_VALUE(terminal),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(status),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(error),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(conffile),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(recover),
                                   SD_VARLINK_DEFINE_ENUM_VALUE(output));

static SD_VARLINK_DEFINE_STRUCT_TYPE(
   InstallEvent,
   SD_VARLINK_FIELD_COMMENT("Mirrors the dpkg status-fd lines libapt emits."),
   SD_VARLINK_DEFINE_FIELD_BY_TYPE(kind, InstallEventKind, 0),
   SD_VARLINK_DEFINE_FIELD(package, SD_VARLINK_STRING, SD_VARLINK_NULLABLE),
   SD_VARLINK_DEFINE_FIELD(percent, SD_VARLINK_INT, SD_VARLINK_NULLABLE),
   SD_VARLINK_DEFINE_FIELD(message, SD_VARLINK_STRING, SD_VARLINK_NULLABLE),
   SD_VARLINK_FIELD_COMMENT("Terminal output, only without 'terminal'."),
   SD_VARLINK_DEFINE_FIELD(output, SD_VARLINK_STRING, SD_VARLINK_NULLABLE));

static SD_VARLINK_DEFINE_METHOD(
   Status,
   SD_VARLINK_DEFINE_OUTPUT(version, SD_VARLINK_STRING, 0),
   SD_VARLINK_FIELD_COMMENT(
      "Whether the dpkg and package list locks are held."),
   SD_VARLINK_DEFINE_OUTPUT(locked, SD_VARLINK_BOOL, 0),
   SD_VARLINK_FIELD_COMMENT("libapt's explanation when they are not."),
   SD_VARLINK_DEFINE_OUTPUT(lock_error,
                            SD_VARLINK_STRING,
                            SD_VARLINK_NULLABLE));

static SD_VARLINK_DEFINE_METHOD_FULL(
   UpdateCache,
   SD_VARLINK_REQUIRES_MORE,
   SD_VARLINK_FIELD_COMMENT(
      "Streamed while downloading; absent in the final reply."),
   SD_VARLINK_DEFINE_OUTPUT_BY_TYPE(event, FetchEvent, SD_VARLINK_NULLABLE),
   SD_VARLINK_FIELD_COMMENT("Non-fatal problems, e.g. unsigned repositories."),
   SD_VARLINK_DEFINE_OUTPUT(warnings, SD_VARLINK_STRING, SD_VARLINK_NULLABLE));

static SD_VARLINK_DEFINE_METHOD_FULL(
   Commit,
   SD_VARLINK_REQUIRES_MORE,
   SD_VARLINK_DEFINE_INPUT_BY_TYPE(selections, Selection, SD_VARLINK_ARRAY),
   SD_VARLINK_DEFINE_INPUT_BY_TYPE(options, CommitOptions, 0),
   SD_VARLINK_FIELD_COMMENT("Streamed while downloading, then while dpkg runs; "
                            "both absent in the final reply."),
   SD_VARLINK_DEFINE_OUTPUT_BY_TYPE(fetch, FetchEvent, SD_VARLINK_NULLABLE),
   SD_VARLINK_DEFINE_OUTPUT_BY_TYPE(install,
                                    InstallEvent,
                                    SD_VARLINK_NULLABLE));

static SD_VARLINK_DEFINE_ERROR(NotLocked);

static SD_VARLINK_DEFINE_ERROR(UpdateFailed,
                               SD_VARLINK_DEFINE_FIELD(message,
                                                       SD_VARLINK_STRING,
                                                       0));

static SD_VARLINK_DEFINE_ERROR(
   InvalidSelection,
   SD_VARLINK_DEFINE_FIELD(name, SD_VARLINK_STRING, 0),
   SD_VARLINK_DEFINE_FIELD(reason, SD_VARLINK_STRING, 0));

static SD_VARLINK_DEFINE_ERROR(Broken,
                               SD_VARLINK_DEFINE_FIELD(packages,
                                                       SD_VARLINK_STRING,
                                                       SD_VARLINK_ARRAY));

static SD_VARLINK_DEFINE_ERROR(FetchFailed,
                               SD_VARLINK_DEFINE_FIELD(message,
                                                       SD_VARLINK_STRING,
                                                       0));

static SD_VARLINK_DEFINE_ERROR(InstallFailed,
                               SD_VARLINK_DEFINE_FIELD(message,
                                                       SD_VARLINK_STRING,
                                                       0));

SD_VARLINK_DEFINE_INTERFACE(
   io_github_mvo5_synaptic,
   SYNAPTIC_VARLINK_INTERFACE,
   SD_VARLINK_INTERFACE_COMMENT(
      "Privileged backend of the Synaptic package manager."),
   &vl_type_FetchItem,
   &vl_type_FetchWorker,
   &vl_type_FetchEventKind,
   &vl_type_FetchEvent,
   &vl_type_SelectionAction,
   &vl_type_Selection,
   &vl_type_ConffilePolicy,
   &vl_type_CommitOptions,
   &vl_type_InstallEventKind,
   &vl_type_InstallEvent,
   SD_VARLINK_SYMBOL_COMMENT("Report the daemon version and lock state."),
   &vl_method_Status,
   SD_VARLINK_SYMBOL_COMMENT(
      "Download the package lists (apt update). Requires 'more'."),
   &vl_method_UpdateCache,
   SD_VARLINK_SYMBOL_COMMENT(
      "Apply a set of package selections: download the archives "
      "and run dpkg. Requires 'more'."),
   &vl_method_Commit,
   SD_VARLINK_SYMBOL_COMMENT("The daemon could not take the locks at startup."),
   &vl_error_NotLocked,
   &vl_error_UpdateFailed,
   SD_VARLINK_SYMBOL_COMMENT(
      "A selection names an unknown package or version, or "
      "could not be applied as requested."),
   &vl_error_InvalidSelection,
   SD_VARLINK_SYMBOL_COMMENT(
      "Applying the selections leaves these packages with "
      "unsatisfied dependencies."),
   &vl_error_Broken,
   &vl_error_FetchFailed,
   &vl_error_InstallFailed);
