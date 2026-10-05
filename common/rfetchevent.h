/* rfetchevent.h - download progress events, shared by daemon and GUI
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

// What the GUI shows about a download. The same struct travels as
// FetchEvent in the io.github.mvo5.synaptic varlink interface.
struct FetchItem
{
   // 1-based, assigned when an item is first seen; 0 means none
   unsigned long id = 0;
   std::string uri;
   std::string description;
   std::string shortDescription;
   unsigned long long size = 0;
};

struct FetchWorker
{
   FetchItem item;
   unsigned long long current = 0;
   unsigned long long total = 0;
};

struct FetchEvent
{
   enum Kind { Start, Fetch, Hit, Done, Fail, Pulse, Stop };

   Kind kind = Start;
   // fetch, hit, done, fail
   bool hasItem = false;
   FetchItem item;
   // fail
   std::string error;
   // pulse
   unsigned long long currentBytes = 0;
   unsigned long long totalBytes = 0;
   unsigned long long currentCPS = 0;
   unsigned long currentItems = 0;
   unsigned long totalItems = 0;
   std::vector<FetchWorker> workers;

   const char *kindName() const;
   static bool kindFromName(const char *name, Kind &kind);
};

// JSON as in the varlink interface. The caller unrefs *ret.
int fetchEventToJson(const FetchEvent &ev, sd_json_variant **ret);
bool fetchEventFromJson(sd_json_variant *v, FetchEvent &ev);
