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

#include "i18n.h"

#include <unistd.h>
#include <sys/fcntl.h>
#ifdef HAVE_RPM
#include <apt-pkg/configuration.h>
#endif

#include "rinstallprogress.h"

void *RInstallProgress::loop(void *data)
{
    RInstallProgress *me = (RInstallProgress*)data;

    me->startUpdate();
    while (me->_thread_id >= 0) {
	me->updateInterface();
    }

    me->finishUpdate();
    pthread_exit(NULL);
    
    return NULL;
}



pkgPackageManager::OrderResult RInstallProgress::start(pkgPackageManager *pm)
{
    void *dummy;
    pkgPackageManager::OrderResult res;

    //cout << "RInstallProgress::start()" << endl;
 
#ifdef HAVE_RPM
    _config->Set("RPM::Interactive", "false");
    
    int fd[2];
    /*
     * This will make a pipe from where we can read child's output
     */
    // our stdout will be _stdout from now on
    _stdout = dup(1); 

    // create our comm. channel with the child
    pipe(fd);

    // close our official stdout
    close(1);
    
    // make the write end of the pipe to the child become the new stdout 
    // (for the child)
    dup2(fd[1],1);

    // this is where we read stuff from the child
    _childin = fd[0];

    // make it nonblocking
    fcntl(_childin, F_SETFL, O_NONBLOCK);
#endif /* HAVE_RPM */

    _thread_id = pthread_create(&_thread, NULL, loop, this);

    res = pm->DoInstall();

    _thread_id = -1;
    pthread_join(_thread, &dummy);

#ifdef HAVE_RPM
    /*
     * Clean-up stuff so that everything is like before
     */
    close(1);
    close(_childin);
    dup2(_stdout, 1);
    close(_stdout);
#endif /* HAVE_RPM */
    return res;
}

