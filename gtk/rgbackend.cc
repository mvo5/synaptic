/* rgbackend.cc - client of synapticd, the privileged backend
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

#include "rgbackend.h"

#include "i18n.h"
#include "rfetchevent.h"
#include "rfetchstatus.h"

#include <apt-pkg/error.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <systemd/sd-json.h>
#include <systemd/sd-varlink.h>
#include <unistd.h>

using std::string;

RGBackend *_backend = nullptr;

namespace {

#define INTERFACE "io.github.mvo5.synaptic"

struct VarlinkSource
{
   GSource source;
   GPollFD pfd;
   sd_varlink *link;
};

// the one call in flight
struct Call
{
   const RGBackend::ReplyHandler &onReply;
   GMainLoop *loop;
   bool done = false;
   bool ok = true;
   string error;
};

string describeError(const char *error_id, sd_json_variant *parameters)
{
   string msg = error_id;
   sd_json_variant *m = sd_json_variant_by_key(parameters, "message");
   if (m != nullptr && sd_json_variant_is_string(m))
      msg = sd_json_variant_string(m);
   return msg;
}

} // namespace

RGBackend *RGBackend::fromEnvironment()
{
   const char *path = getenv("SYNAPTIC_DAEMON");
   if (path == nullptr || *path == '\0')
      return nullptr;
   // the GUI of the future runs as the user; during the migration it
   // may still be root itself, then pkexec would only get in the way
   return new RGBackend(path, /* viaPkexec */ getuid() != 0);
}

RGBackend::~RGBackend()
{
   if (_source != nullptr) {
      g_source_destroy(_source);
      g_source_unref(_source);
   }
   // closing the connection ends the daemon and releases the locks
   sd_varlink_flush_close_unref(_link);
   if (_process != nullptr)
      g_object_unref(_process);
}

bool RGBackend::spawn(string &error)
{
   int sv[2];
   if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) {
      error = string("socketpair: ") + strerror(errno);
      return false;
   }

   // pkexec keeps stdio, which is why the daemon speaks varlink there
   GSubprocessLauncher *launcher =
      g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_NONE);
   g_subprocess_launcher_take_stdin_fd(launcher, sv[1]);
   g_subprocess_launcher_take_stdout_fd(launcher, dup(sv[1]));

   const char *argv[3];
   int n = 0;
   if (_viaPkexec)
      argv[n++] = "pkexec";
   argv[n++] = _path.c_str();
   argv[n] = nullptr;

   GError *gerror = nullptr;
   _process = g_subprocess_launcher_spawnv(launcher, argv, &gerror);
   g_object_unref(launcher);
   if (_process == nullptr) {
      error = gerror->message;
      g_error_free(gerror);
      close(sv[0]);
      return false;
   }

   int r = sd_varlink_connect_fd(&_link, sv[0]);
   if (r < 0) {
      error = string("sd_varlink_connect_fd: ") + strerror(-r);
      close(sv[0]);
      return false;
   }
   sd_varlink_set_allow_fd_passing_input(_link, true);
   return true;
}

gboolean RGBackend::sourcePrepare(GSource *source, gint *timeout)
{
   auto *vs = (VarlinkSource *)source;

   // always read: sd_varlink_get_events() only asks for input once a
   // process() call has set the connection up for the pending reply
   vs->pfd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
   int events = sd_varlink_get_events(vs->link);
   if (events > 0 && (events & EPOLLOUT))
      vs->pfd.events |= G_IO_OUT;

   *timeout = -1;
   uint64_t deadline;
   if (sd_varlink_get_timeout(vs->link, &deadline) > 0) {
      int64_t now = g_get_monotonic_time();
      *timeout = deadline > (uint64_t)now ? (deadline - now) / 1000 : 0;
      if (*timeout == 0)
         return TRUE;
   }
   return FALSE;
}

gboolean RGBackend::sourceCheck(GSource *source)
{
   auto *vs = (VarlinkSource *)source;
   return vs->pfd.revents != 0;
}

gboolean RGBackend::sourceDispatch(GSource *source,
                                   GSourceFunc,
                                   gpointer userdata)
{
   auto *vs = (VarlinkSource *)source;
   auto *self = static_cast<RGBackend *>(userdata);

   int r;
   while ((r = sd_varlink_process(vs->link)) > 0)
      ;
   if (r < 0 && !self->_disconnected) {
      self->_disconnected = true;
      _error->Error(_("Lost the connection to synapticd: %s"), strerror(-r));
   }
   return G_SOURCE_CONTINUE;
}

bool RGBackend::attach()
{
   static GSourceFuncs funcs = {
      sourcePrepare, sourceCheck, sourceDispatch, nullptr, nullptr, nullptr};

   _source = g_source_new(&funcs, sizeof(VarlinkSource));
   auto *vs = (VarlinkSource *)_source;
   vs->link = _link;
   vs->pfd.fd = sd_varlink_get_fd(_link);
   vs->pfd.events = G_IO_IN;
   g_source_add_poll(_source, &vs->pfd);
   g_source_set_callback(_source, nullptr, this, nullptr);
   g_source_attach(_source, nullptr);
   return true;
}

bool RGBackend::start(string &error)
{
   if (!spawn(error) || !attach())
      return false;

   auto onStatus = [this](sd_json_variant *reply) {
      sd_json_variant *locked = sd_json_variant_by_key(reply, "locked");
      _locked = locked != nullptr && sd_json_variant_boolean(locked);
      if (_locked)
         return;
      sd_json_variant *why = sd_json_variant_by_key(reply, "lock_error");
      _lockError = why != nullptr && sd_json_variant_is_string(why)
                      ? sd_json_variant_string(why)
                      : _("synapticd could not take the package system locks");
   };
   return call(INTERFACE ".Status", nullptr, /* more */ false, onStatus, error);
}

int RGBackend::onReply(sd_varlink *,
                       sd_json_variant *parameters,
                       const char *error_id,
                       sd_varlink_reply_flags_t flags,
                       void *userdata)
{
   auto *call = static_cast<Call *>(userdata);

   if (error_id != nullptr) {
      call->ok = false;
      call->error = describeError(error_id, parameters);
   } else {
      call->onReply(parameters);
   }

   if (error_id != nullptr || !(flags & SD_VARLINK_REPLY_CONTINUES)) {
      call->done = true;
      g_main_loop_quit(call->loop);
   }
   return 0;
}

// Synchronous calls would be simpler, but sd-varlink hands the reply
// of a synchronous call to the callback of the next asynchronous one,
// so everything goes through here.
bool RGBackend::call(const char *method,
                     sd_json_variant *parameters,
                     bool more,
                     const ReplyHandler &onReply,
                     string &error)
{
   if (_disconnected) {
      error = _("Lost the connection to synapticd");
      return false;
   }

   Call call{onReply, g_main_loop_new(nullptr, FALSE)};

   sd_varlink_set_userdata(_link, &call);
   sd_varlink_bind_reply(_link, RGBackend::onReply);
   int r = more ? sd_varlink_observe(_link, method, parameters)
                : sd_varlink_invoke(_link, method, parameters);
   // sends the call and arms the connection for the replies
   while (r >= 0 && (r = sd_varlink_process(_link)) > 0)
      ;
   if (r < 0) {
      error = string(method) + ": " + strerror(-r);
      g_main_loop_unref(call.loop);
      return false;
   }

   // the GUI stays responsive while the daemon works
   while (!call.done && !_disconnected)
      g_main_loop_run(call.loop);
   g_main_loop_unref(call.loop);
   sd_varlink_bind_reply(_link, nullptr);

   if (!call.done) {
      error = _("Lost the connection to synapticd");
      return false;
   }
   error = call.error;
   return call.ok;
}

bool RGBackend::updateCache(RFetchStatus *status, string &error)
{
   auto onReply = [status](sd_json_variant *parameters) {
      FetchEvent ev;
      if (fetchEventFromJson(sd_json_variant_by_key(parameters, "event"), ev))
         status->handleFetchEvent(ev);
      sd_json_variant *warnings =
         sd_json_variant_by_key(parameters, "warnings");
      if (warnings != nullptr && sd_json_variant_is_string(warnings))
         _error->Warning("%s", sd_json_variant_string(warnings));
   };
   return call(
      INTERFACE ".UpdateCache", nullptr, /* more */ true, onReply, error);
}
