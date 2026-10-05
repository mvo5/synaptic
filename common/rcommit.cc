/* rcommit.cc - what the GUI sends to and gets from synapticd's Commit
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

#include "config.h" // IWYU pragma: associated

#include "rcommit.h"

#include <cstring>
#include <systemd/sd-json.h>

namespace {

const char *const actionNames[] = {"install",
                                   "remove",
                                   "purge",
                                   "keep",
                                   "reinstall"};
const char *const conffileNames[] = {"ask", "keep", "replace"};
const char *const installKindNames[] =
   {"terminal", "status", "error", "conffile", "recover", "output"};

struct JsonRef
{
   sd_json_variant *v = nullptr;
   ~JsonRef()
   {
      sd_json_variant_unref(v);
   }
};

} // namespace

int selectionsToJson(const std::vector<Selection> &selections,
                     sd_json_variant **ret)
{
   JsonRef array;
   for (const Selection &s : selections) {
      bool versioned =
         s.action == Selection::Install || s.action == Selection::Reinstall;
      JsonRef entry;
      int r = sd_json_buildo(
         &entry.v,
         SD_JSON_BUILD_PAIR_STRING("name", s.name.c_str()),
         SD_JSON_BUILD_PAIR_STRING("arch", s.arch.c_str()),
         SD_JSON_BUILD_PAIR_STRING("action", actionNames[s.action]),
         SD_JSON_BUILD_PAIR_CONDITION(
            versioned, "version", SD_JSON_BUILD_STRING(s.version.c_str())),
         SD_JSON_BUILD_PAIR_BOOLEAN("auto", s.automatic));
      if (r < 0)
         return r;
      r = sd_json_variant_append_array(&array.v, entry.v);
      if (r < 0)
         return r;
   }
   if (array.v == nullptr)
      return sd_json_variant_new_array(ret, nullptr, 0);
   *ret = sd_json_variant_ref(array.v);
   return 0;
}

int commitOptionsToJson(const CommitOptions &options, sd_json_variant **ret)
{
   return sd_json_buildo(
      ret,
      SD_JSON_BUILD_PAIR_BOOLEAN("download_only", options.downloadOnly),
      SD_JSON_BUILD_PAIR_BOOLEAN("fix_missing", options.fixMissing),
      SD_JSON_BUILD_PAIR_STRING("conffile", conffileNames[options.conffile]),
      SD_JSON_BUILD_PAIR_BOOLEAN("terminal", options.terminal));
}

bool installEventFromJson(sd_json_variant *v, InstallEvent &ev)
{
   if (v == nullptr || !sd_json_variant_is_object(v))
      return false;

   sd_json_variant *kind = sd_json_variant_by_key(v, "kind");
   if (kind == nullptr || !sd_json_variant_is_string(kind))
      return false;
   bool known = false;
   for (size_t i = 0;
        i < sizeof(installKindNames) / sizeof(installKindNames[0]);
        i++) {
      if (strcmp(installKindNames[i], sd_json_variant_string(kind)) == 0) {
         ev.kind = (InstallEvent::Kind)i;
         known = true;
      }
   }
   if (!known)
      return false;

   auto str = [v](const char *key, std::string &out) {
      sd_json_variant *f = sd_json_variant_by_key(v, key);
      if (f != nullptr && sd_json_variant_is_string(f))
         out = sd_json_variant_string(f);
   };
   str("package", ev.package);
   str("message", ev.message);
   str("output", ev.output);
   sd_json_variant *percent = sd_json_variant_by_key(v, "percent");
   ev.percent = percent != nullptr && sd_json_variant_is_integer(percent)
                   ? (int)sd_json_variant_integer(percent)
                   : -1;
   return true;
}
