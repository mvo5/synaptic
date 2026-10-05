/* synapticd.cc - privileged backend of synaptic
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

/* Speaks varlink on stdin/stdout, so the GUI can start it through
 * pkexec with a socketpair and no further plumbing. The locks are
 * taken for the whole lifetime of the process, which ends when the
 * connection closes. */

#include "config.h" // IWYU pragma: associated

#include "interface.h"
#include "rpackagecache.h"

#include <apt-pkg/acquire-item.h>
#include <apt-pkg/acquire-worker.h>
#include <apt-pkg/acquire.h>
#include <apt-pkg/algorithms.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/depcache.h>
#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/init.h>
#include <apt-pkg/install-progress.h>
#include <apt-pkg/packagemanager.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/progress.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/update.h>
#include <clocale>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <poll.h>
#include <pty.h>
#include <string>
#include <sys/wait.h>
#include <systemd/sd-event.h>
#include <systemd/sd-json.h>
#include <systemd/sd-varlink.h>
#include <unistd.h>

using std::string;

namespace {

struct Daemon
{
   RPackageCache cache;
   bool locked = false;
   string lockError;
};

struct JsonRef
{
   sd_json_variant *v = nullptr;
   ~JsonRef()
   {
      sd_json_variant_unref(v);
   }
};

string drainErrors()
{
   string all, one;
   while (!_error->empty()) {
      _error->PopMessage(one);
      if (!all.empty())
         all += "\n";
      all += one;
   }
   return all;
}

void logError(const string &msg)
{
   fprintf(stderr, "synapticd: %s\n", msg.c_str());
}

int replyError(sd_varlink *link, const char *error, const string &message)
{
   return sd_varlink_errorbo(
      link, error, SD_JSON_BUILD_PAIR_STRING("message", message.c_str()));
}

// Forwards libapt's acquire callbacks to the client as FetchEvent
// notifies. Runs inside the blocking ListUpdate(), so every notify is
// flushed by hand; the event loop is not spinning meanwhile.
class FetchStatus : public pkgAcquireStatus
{
   sd_varlink *_link;
   const char *_key;
   bool _clientGone = false;

   static int itemVariant(sd_json_variant **ret, pkgAcquire::ItemDesc &item)
   {
      return sd_json_buildo(
         ret,
         SD_JSON_BUILD_PAIR_STRING("uri", item.URI.c_str()),
         SD_JSON_BUILD_PAIR_STRING("description", item.Description.c_str()),
         SD_JSON_BUILD_PAIR_STRING("short_description", item.ShortDesc.c_str()),
         SD_JSON_BUILD_PAIR_UNSIGNED("size", item.Owner->FileSize));
   }

   void send(sd_json_variant *event)
   {
      if (_clientGone)
         return;
      int r =
         sd_varlink_notifybo(_link, SD_JSON_BUILD_PAIR_VARIANT(_key, event));
      if (r >= 0)
         r = sd_varlink_flush(_link);
      if (r < 0) {
         logError(string("sending event failed: ") + strerror(-r));
         _clientGone = true;
      }
   }

   void sendItem(const char *kind,
                 pkgAcquire::ItemDesc &item,
                 const char *error = nullptr)
   {
      JsonRef it, ev;
      if (itemVariant(&it.v, item) < 0)
         return;
      if (sd_json_buildo(
             &ev.v,
             SD_JSON_BUILD_PAIR_STRING("kind", kind),
             SD_JSON_BUILD_PAIR_VARIANT("item", it.v),
             SD_JSON_BUILD_PAIR_CONDITION(
                error != nullptr, "error", SD_JSON_BUILD_STRING(error))) < 0)
         return;
      send(ev.v);
   }

   void sendKind(const char *kind)
   {
      JsonRef ev;
      if (sd_json_buildo(&ev.v, SD_JSON_BUILD_PAIR_STRING("kind", kind)) < 0)
         return;
      send(ev.v);
   }

 public:
   // key: the name of the reply field the events go into
   FetchStatus(sd_varlink *link, const char *key) : _link(link), _key(key)
   {}

   void Start() override
   {
      pkgAcquireStatus::Start();
      sendKind("start");
   }

   void Stop() override
   {
      pkgAcquireStatus::Stop();
      sendKind("stop");
   }

   void Fetch(pkgAcquire::ItemDesc &item) override
   {
      sendItem("fetch", item);
   }

   void IMSHit(pkgAcquire::ItemDesc &item) override
   {
      sendItem("hit", item);
   }

   void Done(pkgAcquire::ItemDesc &item) override
   {
      sendItem("done", item);
   }

   void Fail(pkgAcquire::ItemDesc &item) override
   {
      sendItem("fail", item, item.Owner->ErrorText.c_str());
   }

   bool MediaChange(string media, string drive) override
   {
      _error->Error(
         "Media change to '%s' in '%s' is not supported by synapticd yet",
         media.c_str(),
         drive.c_str());
      return false;
   }

   bool Pulse(pkgAcquire *owner) override
   {
      pkgAcquireStatus::Pulse(owner);

      JsonRef workers;
      for (pkgAcquire::Worker *w = owner->WorkersBegin(); w != nullptr;
           w = owner->WorkerStep(w)) {
         if (w->CurrentItem == nullptr)
            continue;
         JsonRef it, entry;
         if (itemVariant(&it.v, *w->CurrentItem) < 0)
            return !_clientGone;
         if (sd_json_buildo(&entry.v,
                            SD_JSON_BUILD_PAIR_VARIANT("item", it.v),
                            SD_JSON_BUILD_PAIR_UNSIGNED(
                               "current", w->CurrentItem->CurrentSize),
                            SD_JSON_BUILD_PAIR_UNSIGNED(
                               "total", w->CurrentItem->TotalSize)) < 0)
            return !_clientGone;
         if (sd_json_variant_append_array(&workers.v, entry.v) < 0)
            return !_clientGone;
      }

      JsonRef ev;
      if (sd_json_buildo(
             &ev.v,
             SD_JSON_BUILD_PAIR_STRING("kind", "pulse"),
             SD_JSON_BUILD_PAIR_UNSIGNED("current_bytes", CurrentBytes),
             SD_JSON_BUILD_PAIR_UNSIGNED("total_bytes", TotalBytes),
             SD_JSON_BUILD_PAIR_UNSIGNED("current_items", CurrentItems),
             SD_JSON_BUILD_PAIR_UNSIGNED("total_items", TotalItems),
             SD_JSON_BUILD_PAIR_UNSIGNED("current_cps", CurrentCPS),
             SD_JSON_BUILD_PAIR_CONDITION(workers.v != nullptr,
                                          "workers",
                                          SD_JSON_BUILD_VARIANT(workers.v))) <
          0)
         return !_clientGone;
      send(ev.v);

      // a vanished client is the only way to cancel for now
      return !_clientGone;
   }
};

const sd_json_dispatch_field noParameters[] = {{}};

int methodStatus(sd_varlink *link,
                 sd_json_variant *parameters,
                 sd_varlink_method_flags_t /* flags */,
                 void *userdata)
{
   auto *d = static_cast<Daemon *>(userdata);

   int r = sd_varlink_dispatch(link, parameters, noParameters, nullptr);
   if (r != 0)
      return r;

   return sd_varlink_replybo(
      link,
      SD_JSON_BUILD_PAIR_STRING("version", VERSION),
      SD_JSON_BUILD_PAIR_BOOLEAN("locked", d->locked),
      SD_JSON_BUILD_PAIR_CONDITION(
         !d->locked, "lock_error", SD_JSON_BUILD_STRING(d->lockError.c_str())));
}

int methodUpdateCache(sd_varlink *link,
                      sd_json_variant *parameters,
                      sd_varlink_method_flags_t /* flags */,
                      void *userdata)
{
   auto *d = static_cast<Daemon *>(userdata);

   int r = sd_varlink_dispatch(link, parameters, noParameters, nullptr);
   if (r != 0)
      return r;

   if (!d->locked)
      return sd_varlink_error(
         link, SYNAPTIC_VARLINK_INTERFACE ".NotLocked", nullptr);

   _error->Discard();

   pkgSourceList list;
   if (!list.ReadMainList())
      return replyError(
         link, SYNAPTIC_VARLINK_INTERFACE ".UpdateFailed", drainErrors());

   FetchStatus status(link, "event");
   // apt's default pulse interval; the GUI's 5 ms only exists to pump
   // its main loop from inside the callback
   bool ok = ListUpdate(status, list, 500000);
   if (!d->cache.relockLists())
      ok = false;

   if (!ok)
      return replyError(
         link, SYNAPTIC_VARLINK_INTERFACE ".UpdateFailed", drainErrors());

   string warnings = drainErrors();
   return sd_varlink_replybo(
      link,
      SD_JSON_BUILD_PAIR_CONDITION(!warnings.empty(),
                                   "warnings",
                                   SD_JSON_BUILD_STRING(warnings.c_str())));
}

// Terminal output is arbitrary bytes but JSON strings must be UTF-8
string sanitizeUtf8(const char *buf, size_t n)
{
   string out;
   size_t i = 0;
   while (i < n) {
      unsigned char c = buf[i];
      size_t len = c < 0x80         ? 1
                   : (c >> 5) == 6  ? 2
                   : (c >> 4) == 14 ? 3
                   : (c >> 3) == 30 ? 4
                                    : 0;
      bool ok = len > 0 && i + len <= n;
      for (size_t k = 1; ok && k < len; k++)
         ok = (buf[i + k] & 0xC0) == 0x80;
      if (ok) {
         out.append(buf + i, len);
         i += len;
      } else {
         out += '?';
         i++;
      }
   }
   return out;
}

struct CommitOptions
{
   bool downloadOnly = false;
   bool fixMissing = false;
   const char *conffile = nullptr;
   bool terminal = false;
};

struct CommitParams
{
   sd_json_variant *selections = nullptr;
   sd_json_variant *options = nullptr;
   ~CommitParams()
   {
      sd_json_variant_unref(selections);
      sd_json_variant_unref(options);
   }
};

// an absent or null option means false
int dispatchOptionalBool(const char * /* name */,
                         sd_json_variant *variant,
                         sd_json_dispatch_flags_t /* flags */,
                         void *userdata)
{
   *static_cast<bool *>(userdata) =
      !sd_json_variant_is_null(variant) && sd_json_variant_boolean(variant);
   return 0;
}

// percent < 0 and null pointers leave the field out
int sendInstallEvent(sd_varlink *link,
                     const char *kind,
                     const char *package = nullptr,
                     int percent = -1,
                     const char *message = nullptr,
                     const char *output = nullptr)
{
   JsonRef ev;
   int r = sd_json_buildo(
      &ev.v,
      SD_JSON_BUILD_PAIR_STRING("kind", kind),
      SD_JSON_BUILD_PAIR_CONDITION(
         package != nullptr, "package", SD_JSON_BUILD_STRING(package)),
      SD_JSON_BUILD_PAIR_CONDITION(
         percent >= 0, "percent", SD_JSON_BUILD_INTEGER(percent)),
      SD_JSON_BUILD_PAIR_CONDITION(
         message != nullptr, "message", SD_JSON_BUILD_STRING(message)),
      SD_JSON_BUILD_PAIR_CONDITION(
         output != nullptr, "output", SD_JSON_BUILD_STRING(output)));
   if (r < 0)
      return r;
   r = sd_varlink_notifybo(link, SD_JSON_BUILD_PAIR_VARIANT("install", ev.v));
   if (r < 0)
      return r;
   return sd_varlink_flush(link);
}

// A line from libapt's status fd: "pmstatus:pkg:percent:message"
// (also pmerror, pmconffile, pmrecover). The message may contain ':'.
void handleStatusLine(sd_varlink *link, const string &line, string &errors)
{
   string field[4];
   size_t start = 0;
   for (int i = 0; i < 4; i++) {
      size_t colon = i < 3 ? line.find(':', start) : string::npos;
      field[i] = line.substr(
         start, colon == string::npos ? string::npos : colon - start);
      if (colon == string::npos)
         break;
      start = colon + 1;
   }

   const char *kind;
   if (field[0] == "pmstatus")
      kind = "status";
   else if (field[0] == "pmerror")
      kind = "error";
   else if (field[0] == "pmconffile")
      kind = "conffile";
   else if (field[0] == "pmrecover")
      kind = "recover";
   else
      return;

   if (field[0] == "pmerror")
      errors += field[1] + ": " + field[3] + "\n";

   sendInstallEvent(
      link, kind, field[1].c_str(), atoi(field[2].c_str()), field[3].c_str());
}

// Runs dpkg in a child on a pty, like the GUI did in-process. The
// parent forwards the status fd as events and either hands the pty
// master to the client or streams its output.
pkgPackageManager::OrderResult runDpkg(sd_varlink *link,
                                       pkgPackageManager *pm,
                                       const CommitOptions &opts,
                                       string &errors)
{
   pkgPackageManager::OrderResult res = pm->DoInstallPreFork();
   if (res == pkgPackageManager::Failed)
      return res;

   int statusPipe[2];
   if (pipe2(statusPipe, O_CLOEXEC) < 0) {
      _error->Errno("pipe2", "Could not create the dpkg status pipe");
      return pkgPackageManager::Failed;
   }

   int master = -1;
   pid_t child = forkpty(&master, nullptr, nullptr, nullptr);
   if (child < 0) {
      _error->Errno("forkpty", "Could not create the dpkg terminal");
      close(statusPipe[0]);
      close(statusPipe[1]);
      return pkgPackageManager::Failed;
   }

   if (child == 0) {
      close(statusPipe[0]);
      if (strcmp(opts.conffile, "keep") == 0)
         _config->Set("Dpkg::Options::", "--force-confold");
      else if (strcmp(opts.conffile, "replace") == 0)
         _config->Set("Dpkg::Options::", "--force-confnew");

      APT::Progress::PackageManagerProgressFd progress(statusPipe[1]);
      res = pm->DoInstallPostFork(&progress);
      // onto the pty, i.e. visible in the terminal
      _error->DumpErrors();
      if (res == pkgPackageManager::Failed) {
         const char *recover = "pmrecover:dpkg:0:Trying to recover\n";
         // the parent learns about failure from the exit status anyway
         (void)!write(statusPipe[1], recover, strlen(recover));
         (void)!system("dpkg --configure -a");
      }
      _exit(res);
   }
   close(statusPipe[1]);

   if (opts.terminal) {
      int r = sd_varlink_push_dup_fd(link, master);
      if (r >= 0)
         r = sendInstallEvent(link, "terminal");
      if (r < 0)
         logError(string("passing the terminal failed: ") + strerror(-r));
      // the client owns it now; reading it here would steal dpkg's output
      close(master);
      master = -1;
   }

   string pending;
   int statusFd = statusPipe[0];
   while (statusFd >= 0 || master >= 0) {
      struct pollfd fds[2];
      int n = 0;
      if (statusFd >= 0)
         fds[n++] = {statusFd, POLLIN, 0};
      if (master >= 0)
         fds[n++] = {master, POLLIN, 0};
      if (poll(fds, n, -1) < 0) {
         if (errno == EINTR)
            continue;
         break;
      }
      char buf[4096];
      for (int i = 0; i < n; i++) {
         if (fds[i].revents == 0)
            continue;
         ssize_t len = read(fds[i].fd, buf, sizeof(buf));
         if (len <= 0) {
            // a pty reports EIO once the child side is gone
            close(fds[i].fd);
            if (fds[i].fd == statusFd)
               statusFd = -1;
            else
               master = -1;
            continue;
         }
         if (fds[i].fd == statusFd) {
            pending.append(buf, len);
            size_t nl;
            while ((nl = pending.find('\n')) != string::npos) {
               handleStatusLine(link, pending.substr(0, nl), errors);
               pending.erase(0, nl + 1);
            }
         } else {
            sendInstallEvent(link,
                             "output",
                             nullptr,
                             -1,
                             nullptr,
                             sanitizeUtf8(buf, len).c_str());
         }
      }
   }

   int status = 0;
   if (waitpid(child, &status, 0) < 0 || !WIFEXITED(status))
      return pkgPackageManager::Failed;
   return (pkgPackageManager::OrderResult)WEXITSTATUS(status);
}

// Marks the cache as the client asks, without running the resolver:
// the client already did that, so the result must be exactly what it
// sent, or it is rejected. Returns 1 to go on, otherwise the result of
// the error reply.
int applySelections(sd_varlink *link,
                    pkgDepCache &deps,
                    sd_json_variant *selections)
{
   auto reject = [&](const char *name, const char *reason) {
      return sd_varlink_errorbo(link,
                                SYNAPTIC_VARLINK_INTERFACE ".InvalidSelection",
                                SD_JSON_BUILD_PAIR_STRING("name", name),
                                SD_JSON_BUILD_PAIR_STRING("reason", reason));
   };

   {
      pkgDepCache::ActionGroup group(deps);
      for (size_t i = 0; i < sd_json_variant_elements(selections); i++) {
         sd_json_variant *sel = sd_json_variant_by_index(selections, i);
         // the types were validated against the interface definition
         const char *name =
            sd_json_variant_string(sd_json_variant_by_key(sel, "name"));
         const char *arch =
            sd_json_variant_string(sd_json_variant_by_key(sel, "arch"));
         const char *action =
            sd_json_variant_string(sd_json_variant_by_key(sel, "action"));
         bool automatic =
            sd_json_variant_boolean(sd_json_variant_by_key(sel, "auto"));
         sd_json_variant *v = sd_json_variant_by_key(sel, "version");
         const char *version = v != nullptr && !sd_json_variant_is_null(v)
                                  ? sd_json_variant_string(v)
                                  : nullptr;

         pkgCache::PkgIterator pkg = deps.GetCache().FindPkg(name, arch);
         if (pkg.end())
            return reject(name, "unknown package");

         if (strcmp(action, "install") == 0) {
            if (version == nullptr)
               return reject(name, "install needs a version");
            pkgCache::VerIterator ver = pkg.VersionList();
            for (; !ver.end(); ++ver)
               if (strcmp(ver.VerStr(), version) == 0)
                  break;
            if (ver.end())
               return reject(name, "version not available");
            deps.SetCandidateVersion(ver);
            if (!deps.MarkInstall(
                   pkg, /* AutoInst */ false, 0, /* FromUser */ !automatic))
               return reject(name, "cannot be installed");
            deps.MarkAuto(pkg, automatic);
         } else if (strcmp(action, "remove") == 0 ||
                    strcmp(action, "purge") == 0) {
            if (!deps.MarkDelete(pkg, /* MarkPurge */ action[0] == 'p'))
               return reject(name, "cannot be removed");
         } else {
            deps.MarkKeep(pkg, /* Soft */ false, /* FromUser */ true);
            deps.MarkAuto(pkg, automatic);
         }
      }
   }

   if (deps.BrokenCount() == 0)
      return 1;

   JsonRef broken;
   for (pkgCache::PkgIterator pkg = deps.GetCache().PkgBegin(); !pkg.end();
        ++pkg)
      if (deps[pkg].InstBroken() || deps[pkg].NowBroken())
         sd_json_variant_append_arrayb(
            &broken.v, SD_JSON_BUILD_STRING(pkg.FullName().c_str()));
   return sd_varlink_errorbo(link,
                             SYNAPTIC_VARLINK_INTERFACE ".Broken",
                             SD_JSON_BUILD_PAIR_VARIANT("packages", broken.v));
}

int methodCommit(sd_varlink *link,
                 sd_json_variant *parameters,
                 sd_varlink_method_flags_t /* flags */,
                 void *userdata)
{
   auto *d = static_cast<Daemon *>(userdata);

   CommitParams p;
   static const sd_json_dispatch_field table[] = {
      {"selections",
       SD_JSON_VARIANT_ARRAY,
       sd_json_dispatch_variant,
       offsetof(CommitParams, selections),
       SD_JSON_MANDATORY},
      {"options",
       SD_JSON_VARIANT_OBJECT,
       sd_json_dispatch_variant,
       offsetof(CommitParams, options),
       SD_JSON_MANDATORY},
      {}};
   int r = sd_varlink_dispatch(link, parameters, table, &p);
   if (r != 0)
      return r;

   CommitOptions opts;
   static const sd_json_dispatch_field optionTable[] = {
      {"download_only",
       SD_JSON_VARIANT_BOOLEAN,
       dispatchOptionalBool,
       offsetof(CommitOptions, downloadOnly),
       SD_JSON_NULLABLE},
      {"fix_missing",
       SD_JSON_VARIANT_BOOLEAN,
       dispatchOptionalBool,
       offsetof(CommitOptions, fixMissing),
       SD_JSON_NULLABLE},
      {"conffile",
       SD_JSON_VARIANT_STRING,
       sd_json_dispatch_const_string,
       offsetof(CommitOptions, conffile),
       SD_JSON_MANDATORY},
      {"terminal",
       SD_JSON_VARIANT_BOOLEAN,
       dispatchOptionalBool,
       offsetof(CommitOptions, terminal),
       SD_JSON_NULLABLE},
      {}};
   if (sd_json_dispatch(
          p.options, optionTable, (sd_json_dispatch_flags_t)0, &opts) < 0)
      return sd_varlink_error_invalid_parameter_name(link, "options");
   if (strcmp(opts.conffile, "ask") == 0 && !opts.terminal)
      return sd_varlink_error_invalid_parameter_name(link, "conffile");

   if (!d->locked)
      return sd_varlink_error(
         link, SYNAPTIC_VARLINK_INTERFACE ".NotLocked", nullptr);

   _error->Discard();

   OpProgress quiet;
   if (!d->cache.open(&quiet, /* lock */ true) ||
       !pkgApplyStatus(*d->cache.deps()))
      return replyError(
         link, SYNAPTIC_VARLINK_INTERFACE ".InstallFailed", drainErrors());
   pkgDepCache &deps = *d->cache.deps();

   r = applySelections(link, deps, p.selections);
   if (r <= 0)
      return r;

   FileFd archiveLock;
   if (!_config->FindB("Debug::NoLocking", false)) {
      archiveLock.Fd(
         GetLock(_config->FindDir("Dir::Cache::Archives") + "lock"));
      if (archiveLock.Fd() < 0)
         return replyError(
            link, SYNAPTIC_VARLINK_INTERFACE ".FetchFailed", drainErrors());
   }

   pkgSourceList list;
   if (!list.ReadMainList())
      return replyError(
         link, SYNAPTIC_VARLINK_INTERFACE ".FetchFailed", drainErrors());

   pkgRecords records(deps.GetCache());
   std::unique_ptr<pkgPackageManager> pm(_system->CreatePM(&deps));
   FetchStatus status(link, "fetch");
   pkgAcquire fetcher(&status);
   if (!pm->GetArchives(&fetcher, &list, &records) || _error->PendingError())
      return replyError(
         link, SYNAPTIC_VARLINK_INTERFACE ".FetchFailed", drainErrors());

   // the structure follows apt-get: fetch, install, and go round again
   // if the package manager asks for more archives
   while (true) {
      if (fetcher.Run(500000) == pkgAcquire::Failed)
         return replyError(
            link, SYNAPTIC_VARLINK_INTERFACE ".FetchFailed", drainErrors());

      bool failed = false, transient = false;
      string problems;
      for (auto I = fetcher.ItemsBegin(); I != fetcher.ItemsEnd(); ++I) {
         if ((*I)->Status == pkgAcquire::Item::StatDone && (*I)->Complete)
            continue;
         if ((*I)->Status == pkgAcquire::Item::StatIdle) {
            transient = true;
            continue;
         }
         (*I)->Finished();
         problems += "Failed to fetch " + (*I)->DescURI() + "  " +
                     (*I)->ErrorText + "\n";
         failed = true;
      }

      if (opts.downloadOnly) {
         if (failed)
            return replyError(
               link, SYNAPTIC_VARLINK_INTERFACE ".FetchFailed", problems);
         break;
      }
      if (failed) {
         if (transient || !opts.fixMissing)
            return replyError(
               link, SYNAPTIC_VARLINK_INTERFACE ".FetchFailed", problems);
         if (!pm->FixMissing())
            return replyError(link,
                              SYNAPTIC_VARLINK_INTERFACE ".FetchFailed",
                              problems + "Unable to correct missing packages");
      }
      if (transient)
         return replyError(link,
                           SYNAPTIC_VARLINK_INTERFACE ".FetchFailed",
                           "Media changes are not supported by synapticd yet");

      string dpkgErrors;
      _system->UnLockInner();
      pkgPackageManager::OrderResult res =
         runDpkg(link, pm.get(), opts, dpkgErrors);
      if (!_system->LockInner())
         logError("could not re-take the dpkg lock: " + drainErrors());

      if (res == pkgPackageManager::Failed || _error->PendingError())
         return replyError(link,
                           SYNAPTIC_VARLINK_INTERFACE ".InstallFailed",
                           dpkgErrors + drainErrors());
      if (res == pkgPackageManager::Completed)
         break;

      fetcher.Shutdown();
      if (!pm->GetArchives(&fetcher, &list, &records))
         return replyError(
            link, SYNAPTIC_VARLINK_INTERFACE ".FetchFailed", drainErrors());
   }

   return sd_varlink_reply(link, nullptr);
}

int onConnect(sd_varlink_server * /* server */,
              sd_varlink *link,
              void * /* userdata */)
{
   // for the dpkg terminal
   return sd_varlink_set_allow_fd_passing_output(link, true);
}

int run(Daemon &d)
{
   sd_varlink_server *server = nullptr;
   sd_event *event = nullptr;
   sd_varlink *link = nullptr;
   int r;

   r = sd_varlink_server_new(&server, SD_VARLINK_SERVER_INHERIT_USERDATA);
   if (r >= 0)
      r = sd_varlink_server_add_interface(
         server, &vl_interface_io_github_mvo5_synaptic);
   if (r >= 0)
      r = sd_varlink_server_bind_method(
         server, SYNAPTIC_VARLINK_INTERFACE ".Status", methodStatus);
   if (r >= 0)
      r = sd_varlink_server_bind_method(
         server, SYNAPTIC_VARLINK_INTERFACE ".UpdateCache", methodUpdateCache);
   if (r >= 0)
      r = sd_varlink_server_bind_method(
         server, SYNAPTIC_VARLINK_INTERFACE ".Commit", methodCommit);
   if (r >= 0)
      r = sd_varlink_server_bind_connect(server, onConnect);
   if (r >= 0)
      r = sd_varlink_server_set_info(server,
                                     "Synaptic",
                                     "synapticd",
                                     VERSION,
                                     "https://github.com/mvo5/synaptic");
   if (r >= 0)
      r = sd_event_new(&event);
   // exit-on-idle must be set before the event loop is attached:
   // with the loop attached and no connection yet it exits at once
   if (r >= 0)
      r = sd_varlink_server_set_exit_on_idle(server, true);
   if (r >= 0)
      r = sd_varlink_server_attach_event(server, event, 0);
   if (r >= 0) {
      sd_varlink_server_set_userdata(server, &d);
      // a socket passed via LISTEN_FDS (varlinkctl, socket activation)
      // wins over stdio (the GUI through pkexec)
      r = sd_varlink_server_listen_auto(server);
   }
   if (r == 0)
      r = sd_varlink_server_add_connection_stdio(server, &link);
   if (r >= 0)
      r = sd_event_loop(event);

   if (r < 0)
      logError(string("varlink setup failed: ") + strerror(-r));

   sd_varlink_unref(link);
   sd_varlink_server_unref(server);
   sd_event_unref(event);
   return r < 0 ? 1 : 0;
}

} // namespace

int main(int argc, char **argv)
{
   if (argc != 1) {
      fprintf(stderr,
              "Usage: %s\n"
              "Serves the %s varlink interface on stdin/stdout.\n",
              argv[0],
              SYNAPTIC_VARLINK_INTERFACE);
      return 2;
   }

   // apt localizes its error messages; the client shows them verbatim
   setlocale(LC_ALL, "");

   if (!pkgInitConfig(*_config) || !pkgInitSystem(*_config, _system)) {
      logError(drainErrors());
      return 1;
   }

   Daemon d;
   d.locked = d.cache.lock();
   if (!d.locked)
      d.lockError = drainErrors();

   int rc = run(d);
   d.cache.releaseLock();
   return rc;
}
