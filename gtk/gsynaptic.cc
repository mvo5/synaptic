/* gsynaptic.cc - main() 
 * 
 * Copyright (c) 2001-2003 Alfredo K. Kojima
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
#include <cmath>
#include <apt-pkg/configuration.h>
#include <apt-pkg/cmndline.h>
#include <apt-pkg/error.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <signal.h>

#include "rgmainwindow.h"
#include "rguserdialog.h"
#include "locale.h"
#include "stdio.h"
#include "rgmisc.h"

bool ShowHelp(CommandLine &CmdL)
{
  std::cout << 
#ifndef HAVE_RPM
    PACKAGE" for Debian "VERSION
#else
    _config->Find("Synaptic::MyName", PACKAGE)+" "VERSION
#endif
    "\n\n" <<
    _("Usage: synaptic [options]\n") <<
    _("-h   This help text\n") <<
    _("-r   Open in the repository screen\n") <<
    _("-f=? Give a alternative filter file\n") <<
    _("-i=? Start with the initialFilter with the number given\n") <<
    _("-o=? Set an arbitary configuration option, eg -o dir::cache=/tmp\n");
    
    exit(0);
}

CommandLine::Args Args[] = {
  {'h',"help","help",0},
  {'f',"filter-file","Volatile::filterFile", CommandLine::HasArg}, 
  {'r',"repositories", "Volatile::startInRepositories", 0},
  {'i',"initial-filter","Volatile::initialFilter", CommandLine::HasArg},
  {0,"set-selections", "Volatile::Set-Selections", 0},
  {0,"non-interactive", "Volatile::Non-Interactive", 0},
  {'o',"option",0,CommandLine::ArbItem},
  {0,0,0,0}
};


static void SetLanguages()
{
    string LangList;
    if (_config->FindB("Synaptic::DynamicLanguages", true) == false) {
        LangList = _config->Find("Synaptic::Languages", "");
    } else {
	char *lang = getenv("LANG");
	if (lang == NULL) {
	    lang = getenv("LC_MESSAGES");
	    if (lang == NULL) {
		lang = getenv("LC_ALL");
	    }
	}
	if (lang != NULL && strcmp(lang, "C") != 0)
	    LangList = lang;
    }

    _config->Set("Volatile::Languages", LangList);
}


int main(int argc, char **argv)
{    
#ifdef ENABLE_NLS
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);
#endif

    gtk_init(&argc, &argv);    
    //XSynchronize(dpy, 1);

    // check if the locales are actually supported, I got a number
    // of bugreports that where caused by incorrect locales, try to
    // work around this problem here
    if(!XSupportsLocale()) {
	RGGladeUserDialog locales(NULL);
	// run a dialog that warns about the incorrect locale settings
	locales.run("locales_warning");
    } 

    if (getuid() != 0) {
	RGUserDialog userDialog;
	userDialog.error(_("You must run this program as the root user."));
	exit(1);
    }  

    if (!RInitConfiguration("synaptic.conf")) {
	RGUserDialog userDialog;
	userDialog.showErrors();
	exit(1);
    }

    // read configuration early
    _roptions->restore();
 
    // read the cmdline
    CommandLine CmdL(Args, _config);
    if(CmdL.Parse(argc,(const char**)argv) == false) {
	RGUserDialog userDialog;
	userDialog.showErrors();
	exit(1);
    }  
    
    if(_config->FindB("help") == true)
      ShowHelp(CmdL);

    SetLanguages();

    // we ignore sigpipe as it is thrown sporadic on 
    // my debian, kernel 2.6 systems
    struct sigaction new_act;
    memset( &new_act, 0, sizeof( new_act ) );
    new_act.sa_handler = SIG_IGN;
    sigaction( SIGPIPE, &new_act, NULL);

    // init the static pkgStatus class. this loads the status pixmaps 
    // and colors
    RPackageStatus::pkgStatus.init();
    
    RPackageLister *packageLister = new RPackageLister();
    string main_name = _config->Find("Synaptic::MainName", "main_hpaned");
    RGMainWindow *mainWindow = new RGMainWindow(packageLister, main_name);

    // read which default distro to use
    string s = _config->Find("Synaptic::DefaultDistro","");
    if(s != "")
	_config->Set("APT::Default-Release",s);
    
#ifndef HAVE_RPM
      mainWindow->setTitle(_("Synaptic Package Manager "));
#else
    mainWindow->setTitle(_config->Find("Synaptic::MyName", "Synaptic"));
#endif
    mainWindow->show();

    RGFlushInterface();
    
    mainWindow->setInterfaceLocked(true);
    
    bool openCacheError = false;
    if (!packageLister->openCache(false)) {
	mainWindow->showErrors();
	openCacheError = true;
    }

    if (_config->FindB("Volatile::startInRepositories", false)) {
	mainWindow->showRepositoriesWindow();
    }

    if (_config->FindB("Volatile::Set-Selections", false) == true) {
	packageLister->unregisterObserver(mainWindow);
        packageLister->readSelections(cin);
	packageLister->registerObserver(mainWindow);
    }

    mainWindow->restoreState();
#ifdef HAVE_DEBTAGS
    if(!openCacheError)
	mainWindow->initDebtags();
#endif
    mainWindow->showErrors();

    mainWindow->setInterfaceLocked(false);

    // show welcome dialog
    if(_config->FindB("Synaptic::showWelcomeDialog",true)) {
	RGGladeUserDialog dia(mainWindow);
	dia.run("welcome");
	GtkWidget *cb = glade_xml_get_widget(dia.getGladeXML(),
					     "checkbutton_show_again");
	assert(cb);
	_config->Set("Synaptic::showWelcomeDialog",
		     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb)));
    }
    

    if (_config->FindB("Volatile::Non-Interactive", false)) {
	mainWindow->proceed();
    } else {
	gtk_main();
    }
    
    return 0;
}


// vim:sts=4:sw=4
