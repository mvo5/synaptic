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
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <cassert>

#include "rgmainwindow.h"
#include "rguserdialog.h"
#include "locale.h"
#include "stdio.h"
#include "rgmisc.h"

typedef enum {
   UPDATE_ASK,
   UPDATE_CLOSE,
   UPDATE_AUTO
} UpdateType;


bool ShowHelp(CommandLine & CmdL)
{
   std::cout <<
#ifndef HAVE_RPM
      PACKAGE " for Debian " VERSION
#else
      _config->Find("Synaptic::MyName", PACKAGE) + " " VERSION
#endif
      "\n\n" <<
      _("Usage: synaptic [options]\n") <<
      _("-h   This help text\n") <<
      _("-r   Open in the repository screen\n") <<
      _("-f=? Give a alternative filter file\n") <<
      _("-i=? Start with the initialFilter with the number given\n") <<
      _("-o=? Set an arbitary configuration option, eg -o dir::cache=/tmp\n")<<
      _("--upgrade-mode  Call Refresh, Upgrade and display changes\n") <<
      _("--dist-upgrade-mode  Call Refresh, DistUpgrade and display changes\n") <<
      _("--update-at-startup  Call \"reload\" on startup\n")<<
      _("--non-interactive Never prompt for user input\n") << 
      _("--task-window Open with task window\n") <<
      _("--add-cdrom Add a cdrom at startup\n");
   exit(0);
}

CommandLine::Args Args[] = {
   {
   'h', "help", "help", 0}
   , {
   'f', "filter-file", "Volatile::filterFile", CommandLine::HasArg}
   , {
   'r', "repositories", "Volatile::startInRepositories", 0}
   , {
   'i', "initial-filter", "Volatile::initialFilter", CommandLine::HasArg}
   , {
   0, "set-selections", "Volatile::Set-Selections", 0}
   , {
   0, "non-interactive", "Volatile::Non-Interactive", 0}
   , {
   0, "upgrade-mode", "Volatile::Upgrade-Mode", 0}
   , {
   0, "dist-upgrade-mode", "Volatile::DistUpgrade-Mode", 0}
   , {
   0, "add-cdrom", "Volatile::AddCdrom-Mode", CommandLine::HasArg}
   , {
   0, "plug-progress-into", "Volatile::PlugProgressInto", CommandLine::HasArg}
   , {
   0, "update-at-startup", "Volatile::Update-Mode", 0}
   , {
   0, "hide-main-window", "Volatile::HideMainwindow", 0}
   , {
   0, "task-window", "Volatile::TaskWindow", 0}
   , {
   'o', "option", 0, CommandLine::ArbItem}
   , {
   0, 0, 0, 0}
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

void welcome_dialog(RGMainWindow *mainWindow)
{
      // show welcome dialog
      if (_config->FindB("Synaptic::showWelcomeDialog", true) &&
	  !_config->FindB("Volatile::Upgrade-Mode",false)) {
         RGGladeUserDialog dia(mainWindow);
         dia.run("welcome");
         GtkWidget *cb = glade_xml_get_widget(dia.getGladeXML(),
                                              "checkbutton_show_again");
         assert(cb);
         _config->Set("Synaptic::showWelcomeDialog",
                      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb)));
      }
}

void update_check(RGMainWindow *mainWindow, RPackageLister *lister)
{
   //cout << "update_check" << endl;

   // check updates
   UpdateType update =
      (UpdateType) _config->FindI("Synaptic::update::type", UPDATE_ASK);
   if(update != UPDATE_CLOSE) {
      // check when last update happend
      int lastUpdate = _config->FindI("Synaptic::update::last",0);
      int minimal= _config->FindI("Synaptic::update::minimalIntervall", 48);

      // check for the mtime of the various package lists
      vector<string> filenames = lister->getPolicyArchives(true);
      for (int i=0;i<filenames.size();i++) {
	 struct stat st;
	 stat(filenames[i].c_str(), &st);
// 	 cout << "file: " << filenames[i] << " " 
// 	      << ctime(&st.st_mtime) << endl;
	 if(filenames[i] != "/var/lib/dpkg/status")
	    lastUpdate = max(lastUpdate, (int)st.st_mtime);
      }

      // 3600s=1h 
      if((lastUpdate + minimal*3600) < time(NULL)) {
	 if(update == UPDATE_AUTO) 
	    mainWindow->cbUpdateClicked(NULL, mainWindow);
	 else {
	    RGGladeUserDialog dia(mainWindow);
	    int res = dia.run("update_outdated",true);
	    GtkWidget *cb = glade_xml_get_widget(dia.getGladeXML(),
						 "checkbutton_remember");
	    assert(cb);
	    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb))) {
	       if(res == GTK_RESPONSE_CANCEL)
		  _config->Set("Synaptic::update::type", UPDATE_CLOSE);
	       if(res == GTK_RESPONSE_OK)
		  _config->Set("Synaptic::update::type", UPDATE_AUTO);
	    }
	    if(res == GTK_RESPONSE_OK)
	       mainWindow->cbUpdateClicked(NULL, mainWindow);
	 }
      }
   }
}

int main(int argc, char **argv)
{
#ifdef ENABLE_NLS
   bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
   bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
   textdomain(GETTEXT_PACKAGE);

#ifdef HAVE_RPM
   bind_textdomain_codeset("rpm", "UTF-8");
#endif
#endif

   gtk_init(&argc, &argv);
   //XSynchronize(dpy, 1);

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
   if (CmdL.Parse(argc, (const char **)argv) == false) {
      RGUserDialog userDialog;
      userDialog.showErrors();
      exit(1);
   }

   if (_config->FindB("help") == true)
      ShowHelp(CmdL);

   SetLanguages();

   // init the static pkgStatus class. this loads the status pixmaps 
   // and colors
   RGPackageStatus::pkgStatus.init();

   RPackageLister *packageLister = new RPackageLister();
   RGMainWindow *mainWindow = new RGMainWindow(packageLister, "main");

   // read which default distro to use
   string s = _config->Find("Synaptic::DefaultDistro", "");
   if (s != "")
      _config->Set("APT::Default-Release", s);

#ifndef HAVE_RPM
   mainWindow->setTitle(_("Synaptic Package Manager "));
#else
   mainWindow->setTitle(_config->Find("Synaptic::MyName", "Synaptic"));
#endif
   
   if(_config->FindB("Volatile::HideMainwindow", false))
      mainWindow->hide();
   else
      mainWindow->show();

   RGFlushInterface();

   mainWindow->setInterfaceLocked(true);

   if (!packageLister->openCache(false))
      mainWindow->showErrors();

   if (_config->FindB("Volatile::startInRepositories", false)) {
      mainWindow->cbShowSourcesWindow(NULL, mainWindow);
   }

   if (_config->FindB("Volatile::Set-Selections", false) == true) {
      packageLister->unregisterObserver(mainWindow);
      packageLister->readSelections(cin);
      packageLister->registerObserver(mainWindow);
   }

   mainWindow->setInterfaceLocked(false);
   mainWindow->restoreState();
   mainWindow->showErrors();

   
   string cd_mount_point = _config->Find("Volatile::AddCdrom-Mode", "");
   if(!cd_mount_point.empty()) {
      _config->Set("Acquire::cdrom::mount",cd_mount_point);
      _config->Set("APT::CDROM::NoMount", true);
      mainWindow->cbAddCDROM(NULL, mainWindow);
   }

   if(_config->FindB("Volatile::Upgrade-Mode",false) 
      || _config->FindB("Volatile::DistUpgrade-Mode",false) ) {
      mainWindow->cbUpgradeClicked(NULL, mainWindow);
      mainWindow->changeView(PACKAGE_VIEW_CUSTOM, _("Marked Changes"));
   }

   if(_config->FindB("Volatile::Update-Mode",false)) {
      mainWindow->cbUpdateClicked(NULL, mainWindow);
      mainWindow->changeView(PACKAGE_VIEW_STATUS, _("Installed (upgradable)"));
   }

   if(_config->FindB("Volatile::TaskWindow",false)) {
      mainWindow->cbTasksClicked(NULL, mainWindow);
   }

   if (_config->FindB("Volatile::Non-Interactive", false)) {
      mainWindow->cbProceedClicked(NULL, mainWindow);
   } else {
      welcome_dialog(mainWindow);
      update_check(mainWindow, packageLister);
      
      gtk_main();
   }

   return 0;
}


// vim:sts=4:sw=4
