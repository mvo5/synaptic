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
#include <apt-pkg/error.h>
#ifdef HAVE_RPM
#include <apt-pkg/configuration.h>
#endif

#include "rinstallprogress.h"

#include "rpackagemanager.h"
#include "i18n.h"



pkgPackageManager::OrderResult RInstallProgress::start(RPackageManager *pm,
                                                       int numPackages,
                                                       int numPackagesTotal)
{
   void *dummy;
   pkgPackageManager::OrderResult res;
   int ret;
   pid_t _child_id;

   //cout << "RInstallProgress::start()" << endl;
#ifdef HAVE_RPM
   _config->Set("RPM::Interactive", "false");
#endif

   res = pm->DoInstallPreFork();
   if (res == pkgPackageManager::Failed)
       return res;

   /*
    * This will make a pipe from where we can read child's output
    */
   int fd[2];
   pipe(fd);
   int fd_control[2];
   pipe(fd_control);

   _child_id = fork();

   if (_child_id == 0) {
      // make the write end of the pipe to the child become the new stdout 
      // and stderr (for the child)
#ifdef HAVE_RPM
      dup2(fd[1], 1);
      dup2(1, 2);
      close(fd[0]);
      close(fd[1]);
#endif
      // make the read end of the control pipe to the child become the new
      // stdin
      dup2(fd_control[0],0);
      close(fd_control[0]);
      close(fd_control[1]);
#ifdef HAVE_RPM
      res = pm->DoInstallPostFork();
#else
      //res = pm->DoInstallPostFork(fd[1]);
      res = pm->DoInstallPostFork();
#endif
      // dump errors into cerr (pass it to the parent process)	
      _error->DumpErrors();
      _exit(res);
   }
   // this is where we read stuff from the child
   _childin = fd[0];
   close(fd[1]);
   _child_control = fd_control[1];
   close(fd_control[0]);


   // make it nonblocking
   fcntl(_childin, F_SETFL, O_NONBLOCK);

   _donePackages = 0;
   _numPackages = numPackages;
   _numPackagesTotal = numPackagesTotal;


   startUpdate();
   while (waitpid(_child_id, &ret, WNOHANG) == 0)
      updateInterface();

   res = (pkgPackageManager::OrderResult) WEXITSTATUS(ret);

   finishUpdate();


   close(_childin);
   close(_child_control);

   return res;
}

// vim:sts=4:sw=4
