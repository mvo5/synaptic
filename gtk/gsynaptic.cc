/* gsynaptic.cc - main() 
 * 
 * Copyright (c) 2001 Alfredo K. Kojima
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
#include "raptoptions.h"
#include "rpackagelister.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/cmndline.h>
#include <apt-pkg/error.h>

#include <X11/Xlib.h>
#include <gdk/gdkx.h>

#include <unistd.h>

#include "rgmainwindow.h"
#include "locale.h"
#include "galertpanel.h"

bool ShowHelp(CommandLine &CmdL)
{
  std::cout << 
#ifndef HAVE_RPM
    PACKAGE" for Debian "VERSION
#else
    _config->Find("Synaptic::MyName", PACKAGE)+" "VERSION
#endif
    "\n\n"
    "Usage: synaptic [options] \n"
    "-h   This help text \n"
    "-f=? Give a alternative filter file\n" 
    "-i=? Start with the initialFilter with the number given\n" 
    "-o=? Set an arbitary configuration option, eg -o dir::cache=/tmp\n";
    
    exit(0);
}

CommandLine::Args Args[] = {
  {'h',"help","help",0},
  {'f',"filter-file","Volatile::filterFile", CommandLine::HasArg}, 
  {'i',"initial-filter","Volatile::initialFilter", CommandLine::HasArg},
  {0,"set-selections", "Volatile::Set-Selections", 0},
  {0,"non-interactive", "Volatile::Non-Interactive", 0},
  {'o',"option",0,CommandLine::ArbItem},
  {0,0,0,0}
};


void RGFlushInterface()
{
    XSync(gdk_display, False);

    while (gtk_events_pending()) {
	gtk_main_iteration();
    }
}


int main(int argc, char **argv)
{    
#ifdef ENABLE_NLS
    //setlocale(LC_ALL, "");
    gtk_set_locale();
    
//    bindtextdomain(PACKAGE, "/usr/local/share/locale");
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    gtk_init(&argc, &argv);    
    //XSynchronize(dpy, 1);

    if (getuid() != 0) {
	gtk_run_alert_panel(NULL, _("Error"),
			    _("You must run this program as the root user."),
			    _("OK"), NULL, NULL);
	exit(1);
    }   
  
    if (!RInitConfiguration("synaptic.conf")) {
	_error->DumpErrors();
	exit(1);
    }

    // read configuration early
    _roptions->restore();
 
    // read the cmdline
    CommandLine CmdL(Args, _config);
    if(CmdL.Parse(argc,(const char**)argv) == false) {
      _error->DumpErrors();
      exit(1);
    }  
    if(_config->FindB("help") == true)
      ShowHelp(CmdL);

    RPackageLister *packageLister = new RPackageLister();
    RGMainWindow *mainWindow = new RGMainWindow(packageLister);

#ifndef HAVE_RPM
    mainWindow->setTitle(PACKAGE" for Debian "VERSION);
#else
    mainWindow->setTitle(_config->Find("Synaptic::MyName", PACKAGE)+" "VERSION);
#endif
    mainWindow->show();

    RGFlushInterface();
    
    mainWindow->setInterfaceLocked(true);
    
    if (!packageLister->openCache(false)) {
	mainWindow->showErrors();
	//exit(1);
    }

    if (_config->FindB("Volatile::Set-Selections", False)) {
        packageLister->readSelections(cin);
    }

    mainWindow->setInterfaceLocked(false);
    mainWindow->restoreState();
    mainWindow->showErrors();

    if (_config->FindB("Volatile::Non-Interactive", false)) {
	mainWindow->proceed();
    } else {
	gtk_main();
    }
    
    return 0;
}

// vim:sts=4:sw=4
