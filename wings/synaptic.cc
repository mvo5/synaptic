/* synaptic.cc - main()
 * 
 * Copyright (c) 2000-2003 Conectiva S/A 
 * 
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
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

#include <iostream>

#include "config.h"

#include "i18n.h"

#include "rconfiguration.h"
#include "rpackagelister.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>


#include <unistd.h>

#include "rwmainwindow.h"

#include <WINGs/WINGs.h>


static Display *dpy;
static WMScreen *scr;



void RWFlushInterface()
{
    XSync(dpy, False);
    while (XPending(dpy)) {
	XEvent ev;
	
	WMNextEvent(dpy, &ev);
	WMHandleEvent(&ev);
    }
}


int main(int argc, char **argv)
{    
    setlocale(LC_ALL, "");
    
//    bindtextdomain(PACKAGE, "/usr/local/share/locale");
    textdomain(PACKAGE);
    
    WMInitializeApplication("synaptic", &argc, argv);
    
    dpy = XOpenDisplay("");
    if (!dpy) {
	cout << argv[0] << _(":could not open display") <<endl;
	exit(1);
    }
    
    //XSynchronize(dpy, 1);
    scr = WMCreateScreen(dpy, DefaultScreen(dpy));

    if (getuid() != 0) {
	WMRunAlertPanel(scr, NULL, _("Error"),
			_("You must run this program as the root user."),
			_("OK"), NULL, NULL);
	exit(1);
    }   
 
 
    if (!RInitConfiguration("synaptic.conf")) {
	_error->DumpErrors();
	exit(1);
    }

   
    
    RPackageLister *packageLister = new RPackageLister();


    RWMainWindow *mainWindow = new RWMainWindow(scr, packageLister);

#ifndef HAVE_RPM
    mainWindow->setTitle(PACKAGE" for Debian "VERSION);
#else
    mainWindow->setTitle(_config->Find("Synaptic::MyName", PACKAGE)+" "VERSION);
#endif

    mainWindow->show();

    RWFlushInterface();
    
    mainWindow->setInterfaceLocked(true);
    
    if (!packageLister->openCache(false)) {
	mainWindow->showErrors();
	exit(1);
    }
    
    mainWindow->restoreState();

    mainWindow->setInterfaceLocked(false);

    mainWindow->showErrors();
    
    WMScreenMainLoop(scr);
    
    return 0;
}

