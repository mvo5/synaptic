/* rcommit.h - what the GUI sends to and gets from synapticd's Commit
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

#pragma once

#include "config.h" // IWYU pragma: associated

#include <string>
#include <vector>

typedef struct sd_json_variant sd_json_variant;

// The requested state of one package, as the Selection type of the
// io.github.mvo5.synaptic interface. The GUI resolves dependencies
// before sending, the daemon applies the list verbatim.
struct Selection
{
   enum Action { Install, Remove, Purge, Keep, Reinstall };

   std::string name;
   std::string arch;
   Action action = Keep;
   // install and reinstall: the exact version
   std::string version;
   bool automatic = false;
};

struct CommitOptions
{
   enum Conffile { Ask, KeepConffile, Replace };

   bool downloadOnly = false;
   bool fixMissing = false;
   Conffile conffile = Ask;
   // ask for the dpkg pty instead of streamed output
   bool terminal = false;
};

// What the daemon reports while dpkg runs: the status-fd lines it
// used to be the GUI's job to parse.
struct InstallEvent
{
   enum Kind { Terminal, Status, Error, Conffile, Recover, Output };

   Kind kind = Status;
   // status, error: the package; conffile: the file
   std::string package;
   int percent = -1;
   std::string message;
   std::string output;
};

// Where the GUI hands the install events
class RInstallEventHandler
{
 public:
   virtual ~RInstallEventHandler() = default;
   virtual void handleInstallEvent(const InstallEvent &ev) = 0;
   // the dpkg pty master; the receiver owns the fd
   virtual void attachTerminal(int fd) = 0;
};

// JSON as in the varlink interface. The caller unrefs *ret.
int selectionsToJson(const std::vector<Selection> &selections,
                     sd_json_variant **ret);
int commitOptionsToJson(const CommitOptions &options, sd_json_variant **ret);
bool installEventFromJson(sd_json_variant *v, InstallEvent &ev);
