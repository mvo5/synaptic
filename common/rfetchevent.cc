/* rfetchevent.cc - download progress events, shared by daemon and GUI
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

#include "rfetchevent.h"

#include <cstring>
#include <systemd/sd-json.h>

namespace {

const char *const kindNames[] =
   {"start", "fetch", "hit", "done", "fail", "pulse", "stop"};

struct JsonRef
{
   sd_json_variant *v = nullptr;
   ~JsonRef()
   {
      sd_json_variant_unref(v);
   }
};

int itemToJson(const FetchItem &item, sd_json_variant **ret)
{
   return sd_json_buildo(
      ret,
      SD_JSON_BUILD_PAIR_UNSIGNED("id", item.id),
      SD_JSON_BUILD_PAIR_STRING("uri", item.uri.c_str()),
      SD_JSON_BUILD_PAIR_STRING("description", item.description.c_str()),
      SD_JSON_BUILD_PAIR_STRING("short_description",
                                item.shortDescription.c_str()),
      SD_JSON_BUILD_PAIR_UNSIGNED("size", item.size));
}

bool getString(sd_json_variant *o, const char *key, std::string &out)
{
   sd_json_variant *f = sd_json_variant_by_key(o, key);
   if (f == nullptr || !sd_json_variant_is_string(f))
      return false;
   out = sd_json_variant_string(f);
   return true;
}

bool getUnsigned(sd_json_variant *o, const char *key, unsigned long long &out)
{
   sd_json_variant *f = sd_json_variant_by_key(o, key);
   if (f == nullptr || !sd_json_variant_is_unsigned(f))
      return false;
   out = sd_json_variant_unsigned(f);
   return true;
}

bool itemFromJson(sd_json_variant *o, FetchItem &item)
{
   if (o == nullptr || !sd_json_variant_is_object(o))
      return false;
   unsigned long long id = 0;
   getUnsigned(o, "id", id);
   item.id = id;
   getUnsigned(o, "size", item.size);
   return getString(o, "uri", item.uri) &&
          getString(o, "description", item.description) &&
          getString(o, "short_description", item.shortDescription);
}

} // namespace

const char *FetchEvent::kindName() const
{
   return kindNames[kind];
}

bool FetchEvent::kindFromName(const char *name, Kind &kind)
{
   for (size_t i = 0; i < sizeof(kindNames) / sizeof(kindNames[0]); i++) {
      if (strcmp(kindNames[i], name) == 0) {
         kind = (Kind)i;
         return true;
      }
   }
   return false;
}

int fetchEventToJson(const FetchEvent &ev, sd_json_variant **ret)
{
   JsonRef item, workers;
   int r;

   if (ev.hasItem) {
      r = itemToJson(ev.item, &item.v);
      if (r < 0)
         return r;
   }

   for (const FetchWorker &w : ev.workers) {
      JsonRef it, entry;
      r = itemToJson(w.item, &it.v);
      if (r < 0)
         return r;
      r = sd_json_buildo(&entry.v,
                         SD_JSON_BUILD_PAIR_VARIANT("item", it.v),
                         SD_JSON_BUILD_PAIR_UNSIGNED("current", w.current),
                         SD_JSON_BUILD_PAIR_UNSIGNED("total", w.total));
      if (r < 0)
         return r;
      r = sd_json_variant_append_array(&workers.v, entry.v);
      if (r < 0)
         return r;
   }

   bool pulse = ev.kind == FetchEvent::Pulse;
   return sd_json_buildo(
      ret,
      SD_JSON_BUILD_PAIR_STRING("kind", ev.kindName()),
      SD_JSON_BUILD_PAIR_CONDITION(
         ev.hasItem, "item", SD_JSON_BUILD_VARIANT(item.v)),
      SD_JSON_BUILD_PAIR_CONDITION(
         !ev.error.empty(), "error", SD_JSON_BUILD_STRING(ev.error.c_str())),
      SD_JSON_BUILD_PAIR_CONDITION(
         pulse, "current_bytes", SD_JSON_BUILD_UNSIGNED(ev.currentBytes)),
      SD_JSON_BUILD_PAIR_CONDITION(
         pulse, "total_bytes", SD_JSON_BUILD_UNSIGNED(ev.totalBytes)),
      SD_JSON_BUILD_PAIR_CONDITION(
         pulse, "current_items", SD_JSON_BUILD_UNSIGNED(ev.currentItems)),
      SD_JSON_BUILD_PAIR_CONDITION(
         pulse, "total_items", SD_JSON_BUILD_UNSIGNED(ev.totalItems)),
      SD_JSON_BUILD_PAIR_CONDITION(
         pulse, "current_cps", SD_JSON_BUILD_UNSIGNED(ev.currentCPS)),
      SD_JSON_BUILD_PAIR_CONDITION(
         workers.v != nullptr, "workers", SD_JSON_BUILD_VARIANT(workers.v)));
}

bool fetchEventFromJson(sd_json_variant *v, FetchEvent &ev)
{
   if (v == nullptr || !sd_json_variant_is_object(v))
      return false;

   std::string kind;
   if (!getString(v, "kind", kind) ||
       !FetchEvent::kindFromName(kind.c_str(), ev.kind))
      return false;

   sd_json_variant *item = sd_json_variant_by_key(v, "item");
   ev.hasItem = item != nullptr && !sd_json_variant_is_null(item);
   if (ev.hasItem && !itemFromJson(item, ev.item))
      return false;

   getString(v, "error", ev.error);

   unsigned long long n;
   getUnsigned(v, "current_bytes", ev.currentBytes);
   getUnsigned(v, "total_bytes", ev.totalBytes);
   getUnsigned(v, "current_cps", ev.currentCPS);
   if (getUnsigned(v, "current_items", n))
      ev.currentItems = n;
   if (getUnsigned(v, "total_items", n))
      ev.totalItems = n;

   ev.workers.clear();
   sd_json_variant *workers = sd_json_variant_by_key(v, "workers");
   if (workers != nullptr && sd_json_variant_is_array(workers)) {
      for (size_t i = 0; i < sd_json_variant_elements(workers); i++) {
         sd_json_variant *entry = sd_json_variant_by_index(workers, i);
         FetchWorker w;
         if (!sd_json_variant_is_object(entry) ||
             !itemFromJson(sd_json_variant_by_key(entry, "item"), w.item))
            return false;
         getUnsigned(entry, "current", w.current);
         getUnsigned(entry, "total", w.total);
         ev.workers.push_back(w);
      }
   }
   return true;
}
