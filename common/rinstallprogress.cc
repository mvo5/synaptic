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

#include "config.h"

#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>
#include <cstdio>
#ifdef HAVE_RPM
#include <apt-pkg/configuration.h>
#endif

#include "rinstallprogress.h"

#include "i18n.h"

void *RInstallProgress::loop(void *data)
{
   RInstallProgress *me = (RInstallProgress *) data;

   me->startUpdate();
   while (me->_child_id >= 0)
      me->updateInterface();
   me->finishUpdate();

   pthread_exit(NULL);

   return NULL;
}



pkgPackageManager::OrderResult RInstallProgress::start(pkgPackageManager *pm,
                                                       int numPackages,
                                                       int numPackagesTotal)
{
   void *dummy;
   pkgPackageManager::OrderResult res;
   int ret;

   //cout << "RInstallProgress::start()" << endl;

#ifdef HAVE_RPM

   _config->Set("RPM::Interactive", "false");

   /*
    * This will make a pipe from where we can read child's output
    */
   int fd[2];
   pipe(fd);

   _child_id = fork();

   if (_child_id == 0) {
      // make the write end of the pipe to the child become the new stdout 
      // and stderr (for the child)
      dup2(fd[1], 1);
      dup2(1, 2);
      close(fd[0]);
      close(fd[1]);

      res = pm->DoInstall();

      exit(0);
   }
   // this is where we read stuff from the child
   _childin = fd[0];
   close(fd[1]);

   // make it nonblocking
   fcntl(_childin, F_SETFL, O_NONBLOCK);

   _donePackages = 0;
   _numPackages = numPackages;
   _numPackagesTotal = numPackagesTotal;

#else

   _child_id = fork();

   if (_child_id == 0) {
      res = pm->DoInstall();
      _exit(res);
   }
#endif

   startUpdate();
   while (waitpid(_child_id, &ret, WNOHANG) == 0)
      updateInterface();

   res = (pkgPackageManager::OrderResult) WEXITSTATUS(ret);

   finishUpdate();

#ifdef HAVE_RPM
   close(_childin);
#endif

   return res;
}

// vim:sts=4:sw=4
