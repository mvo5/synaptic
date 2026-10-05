/* rgbackend.h - client of synapticd, the privileged backend
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

#include <functional>
#include <gio/gio.h>
#include <string>
#include <systemd/sd-varlink.h>

class RFetchStatus;

// Starts synapticd through pkexec with a socketpair on its stdio and
// speaks varlink to it from the GTK main loop.
class RGBackend
{
 public:
   using ReplyHandler = std::function<void(sd_json_variant *parameters)>;

 private:
   std::string _path;
   bool _viaPkexec;
   sd_varlink *_link = nullptr;
   GSource *_source = nullptr;
   GSubprocess *_process = nullptr;
   bool _disconnected = false;
   bool _locked = false;
   std::string _lockError;

   bool spawn(std::string &error);
   bool attach();
   // runs the GTK main loop until the final reply; every reply goes to
   // onReply, an error reply ends up in error
   bool call(const char *method,
             sd_json_variant *parameters,
             bool more,
             const ReplyHandler &onReply,
             std::string &error);

   static gboolean sourcePrepare(GSource *source, gint *timeout);
   static gboolean sourceCheck(GSource *source);
   static gboolean sourceDispatch(GSource *source, GSourceFunc, gpointer);
   static int onReply(sd_varlink *link,
                      sd_json_variant *parameters,
                      const char *error_id,
                      sd_varlink_reply_flags_t flags,
                      void *userdata);

 public:
   // the daemon named by $SYNAPTIC_DAEMON, or nullptr to run in-process
   static RGBackend *fromEnvironment();

   // viaPkexec: false only for tests, which cannot authenticate
   RGBackend(const std::string &path, bool viaPkexec)
      : _path(path), _viaPkexec(viaPkexec)
   {}
   ~RGBackend();

   // spawns, connects and asks the daemon whether it holds the locks
   bool start(std::string &error);
   bool locked() const
   {
      return _locked;
   }
   const std::string &lockError() const
   {
      return _lockError;
   }

   // apt update; the events go to status, warnings into _error
   bool updateCache(RFetchStatus *status, std::string &error);
};

extern RGBackend *_backend;
