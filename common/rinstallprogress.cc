/* rinstallprogress.cc
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *                     2002 Michael Vogt
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <mvo@debian.org>
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

#include "rinstallprogress.h"

#include "i18n.h"

#include <apt-pkg/install-progress.h>
#include <apt-pkg/packagemanager.h>
#include <optional>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

string RInstallProgress::finishMsg =
   _("\nSuccessfully applied all changes. You can close the window now.");
string RInstallProgress::errorMsg =
   _("\nNot all changes and updates succeeded. For further details of the "
     "failure, please expand the 'Details' panel below.");
string RInstallProgress::incompleteMsg =
   _("\nSuccessfully installed all packages of the current medium. "
     "To continue the installation with the next medium close "
     "this window.");

const char *RInstallProgress::getResultStr(pkgPackageManager::OrderResult res)
{
   switch (res) {
      case 0: // completed
         return finishMsg.c_str();
         break;
      case 1: // failed
         return errorMsg.c_str();
         break;
      case 2: // incomplete
         return incompleteMsg.c_str();
         break;
   }

   return "Unknown install result.";
}

std::optional<pkgPackageManager::OrderResult> RInstallProgress::start(
   pkgPackageManager *pm,
   int numPackages,
   int numPackagesTotal)
{
   pkgPackageManager::OrderResult res;

   res = pm->DoInstallPreFork();
   if (res == pkgPackageManager::Failed)
      return res;

   _child_id = fork();

   if (_child_id == 0) {
      APT::Progress::PackageManagerProgressFd progress(-1);
      res = pm->DoInstallPostFork(&progress);
      _exit(res);
   }

   return std::nullopt;
}

std::optional<pkgPackageManager::OrderResult> RInstallProgress::poll()
{
   int ret = 0;
   pid_t pid = waitpid(_child_id, &ret, WNOHANG);
   if (pid == 0) {
      return std::nullopt;
   }

   // WEXITSTATUS of a signal-killed child is 0, i.e. Completed
   pkgPackageManager::OrderResult res =
      (pid > 0 && WIFEXITED(ret))
         ? (pkgPackageManager::OrderResult)WEXITSTATUS(ret)
         : pkgPackageManager::Failed;

   return res;
}
