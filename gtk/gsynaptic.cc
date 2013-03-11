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
#include <fcntl.h>
#include <signal.h>
#include <cstdlib>
#include <cassert>
#include <errno.h>
#include <fstream>
#include <cstring>

#include "rgmainwindow.h"
#include "rguserdialog.h"
#include "locale.h"
#include "stdio.h"
#include "rgutils.h"
#include "rgpackagestatus.h"



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
      _("-f=? Give an alternative filter file\n") <<
      _("-t   Give an alternative main window title (e.g. hostname with `uname -n`)\n") <<
      _("-i=? Start with the initial Filter with given name\n") <<
      _("-o=? Set an arbitrary configuration option, eg -o dir::cache=/tmp\n")<<
      _("--upgrade-mode  Call Upgrade and display changes\n") <<
      _("--dist-upgrade-mode  Call DistUpgrade and display changes\n") <<
      _("--update-at-startup  Call \"Reload\" on startup\n")<<
      _("--non-interactive Never prompt for user input\n") << 
      _("--task-window Open with task window\n") <<
      _("--add-cdrom Add a cdrom at startup (needs path for cdrom)\n") <<
      _("--ask-cdrom Ask for adding a cdrom and exit\n") <<
      _("--test-me-harder  Run test in a loop\n");
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
   0, "set-selections-file", "Volatile::Set-Selections-File", CommandLine::HasArg}
   , {
   0, "non-interactive", "Volatile::Non-Interactive", 0}
   , {
   0, "upgrade-mode", "Volatile::Upgrade-Mode", 0}
   , {
   0, "dist-upgrade-mode", "Volatile::DistUpgrade-Mode", 0}
   , {
   0, "add-cdrom", "Volatile::AddCdrom-Mode", CommandLine::HasArg}
   , {
   0, "ask-cdrom", "Volatile::AskCdrom-Mode", 0}
   , {
   0, "parent-window-id", "Volatile::ParentWindowId", CommandLine::HasArg}
   , {
   0, "plug-progress-into", "Volatile::PlugProgressInto", CommandLine::HasArg}
   , {
   0, "progress-str", "Volatile::InstallProgressStr", CommandLine::HasArg} 
   , {
   0, "finish-str", "Volatile::InstallFinishedStr", CommandLine::HasArg} 
   , {
   't', "title", "Volatile::MyName", CommandLine::HasArg}
   , {
   0, "update-at-startup", "Volatile::Update-Mode", 0}
   , {
   0, "hide-main-window", "Volatile::HideMainwindow", 0}
   , {
   0, "task-window", "Volatile::TaskWindow", 0}
   , {
   0, "test-me-harder", "Volatile::TestMeHarder", 0}
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
         RGGtkBuilderUserDialog dia(mainWindow);
         dia.run("welcome");
         GtkWidget *cb = GTK_WIDGET(gtk_builder_get_object(dia.getGtkBuilder(),
                                              "checkbutton_show_again"));
         assert(cb);
         _config->Set("Synaptic::showWelcomeDialog",
                      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb)));
      }
}

void update_check(RGMainWindow *mainWindow, RPackageLister *lister)
{
   struct stat st;

   // check when the last update happend updates
   UpdateType update =
      (UpdateType) _config->FindI("Synaptic::update::type", UPDATE_ASK);
   if(update != UPDATE_CLOSE) {
      // check when last update happend
      int lastUpdate = _config->FindI("Synaptic::update::last",0);
      int minimal= _config->FindI("Synaptic::update::minimalIntervall", 48);

      // check for the mtime of the various package lists
      vector<string> filenames = lister->getPolicyArchives(true);
      for (int i=0;i<filenames.size();i++) {
	 stat(filenames[i].c_str(), &st);
	 if(filenames[i] != "/var/lib/dpkg/status")
	    lastUpdate = max(lastUpdate, (int)st.st_mtime);
      }
      
      // new apt uses this
      string update_stamp = _config->FindDir("Dir::State","var/lib/apt");
      update_stamp += "update-stamp";
      if(FileExists(update_stamp)) {
	 stat(update_stamp.c_str(), &st);
	 lastUpdate = max(lastUpdate, (int)st.st_mtime);
      }

      // 3600s=1h 
      if((lastUpdate + minimal*3600) < time(NULL)) {
	 if(update == UPDATE_AUTO) 
	    mainWindow->cbUpdateClicked(NULL, mainWindow);
	 else {
	    RGGtkBuilderUserDialog dia(mainWindow);
	    int res = dia.run("update_outdated",true);
	    GtkWidget *cb = GTK_WIDGET(gtk_builder_get_object(dia.getGtkBuilder(),
						 "checkbutton_remember"));
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


// lock stuff
static int sigterm_unix_signal_pipe_fds[2];
static GIOChannel *sigterm_iochn;

static void 
handle_sigusr1 (int value)
{
   static char marker[1] = {'S'};

   /* write a 'S' character to the other end to tell about
    * the signal. Note that 'the other end' is a GIOChannel thingy
    * that is only called from the mainloop - thus this is how we
    * defer this since UNIX signal handlers are evil
    *
    * Oh, and write(2) is indeed reentrant */
   write (sigterm_unix_signal_pipe_fds[1], marker, 1);
}

static gboolean
sigterm_iochn_data (GIOChannel *source, 
		    GIOCondition condition, 
		    gpointer user_data)
{
   GError *err = NULL;
   gchar data[1];
   gsize bytes_read;
   
   RGMainWindow *me = (RGMainWindow *)user_data;

   /* Empty the pipe */
   if (G_IO_STATUS_NORMAL != 
       g_io_channel_read_chars (source, data, 1, &bytes_read, &err)) {
      g_warning("Error emptying callout notify pipe: %s", err->message);
      g_error_free (err);
      return TRUE;
   }
   if(data[0] == 'S')
      me->activeWindowToForeground();

   return TRUE;
}

// test if a lock is aquired already, return 0 if no lock is found, 
// the pid of the locking application or -1 on error
pid_t TestLock(string File)
{
   int FD = open(File.c_str(),0);
   if(FD < 0) {
      if(errno == ENOENT) {
	 // File does not exist, no there can't be a lock
	 return 0; 
      } else {
	 //cout << "open, errno: " << errno << endl;
	 //perror("open");
	 return(-1);
      }
   }
   struct flock fl;
   fl.l_type = F_WRLCK;
   fl.l_whence = SEEK_SET;
   fl.l_start = 0;
   fl.l_len = 0;
   if (fcntl(FD, F_GETLK, &fl) < 0) {
      int Tmp = errno;
      close(FD);
      cerr << "fcntl error" << endl;
      errno = Tmp;
      return(-1);
   }
   close(FD);
   // lock is available
   if(fl.l_type == F_UNLCK)
      return(0);
   // file is locked by another process
   return (fl.l_pid);
}

// check if we can get a lock, must be done after we read the configuration
// if a lock is found and the app is synaptic send it a "come to foreground"
// signal (USR1) or if not synaptic display a error and exit
// 1. check if there is another synaptic running
//    a) if so, check if it runs interactive and we are not running interactive
//       *) if so, send signal and show message
//       *) if not, send signal
// 2. check if we can get a /var/lib/dpkg/lock 
//    *) if not, show message and fail
void check_and_aquire_lock()
{
   if (getuid() != 0) 
      return;

   GtkWidget *dia;
   gchar *msg = NULL;
   pid_t LockedApp, runsNonInteractive;
   bool weNonInteractive;

   string SynapticLock = RConfDir()+"/lock";
   string SynapticNonInteractiveLock = RConfDir()+"/lock.non-interactive";
   weNonInteractive = _config->FindB("Volatile::Non-Interactive", false);

   // 1. test for another synaptic
   LockedApp = TestLock(SynapticLock);
   if(LockedApp > 0) {
      runsNonInteractive = TestLock(SynapticNonInteractiveLock);
      //cout << "runsNonIteractive: " << runsNonInteractive << endl;
      //cout << "weNonIteractive: " << weNonInteractive << endl;
      if(weNonInteractive && runsNonInteractive <= 0) {
	 // message that we can't turn a non-interactive into a interactive
	 // one
	 msg = g_strdup_printf("<big><b>%s</b></big>\n\n%s",
			       _("Another synaptic is running"),
			       _("There is another synaptic running in "
				 "interactive mode. Please close it first. "
				 ));
      } else if(runsNonInteractive > 0) {
	 msg = g_strdup_printf("<big><b>%s</b></big>\n\n%s",
			       _("Another synaptic is running"),
			       _("There is another synaptic running in "
				 "non-interactive mode. Please wait for it "
				 "to finish first."
				 ));
      }

      if(msg != NULL) {
	 dia = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR, 
					GTK_BUTTONS_CLOSE, 
					NULL);

         gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dia), msg);
	 gtk_dialog_run(GTK_DIALOG(dia));
	 gtk_widget_destroy(dia);
      }
      g_free(msg);

      cout << "Another synaptic is running. Trying to bring it to the foreground" << endl;
      kill(LockedApp, SIGUSR1);
      exit(0);
   }

   // 2. test if we can get a lock
   string AdminDir = flNotFile(_config->Find("Dir::State::status"));
   LockedApp = TestLock(AdminDir + "lock");
   if (LockedApp > 0) {
      msg = g_strdup_printf("<big><b>%s</b></big>\n\n%s",
			    _("Unable to get exclusive lock"),
			    _("This usually means that another "
			      "package management application "
			      "(like apt-get or aptitude) is "
			      "already running. Please close that "
			      "application first."));
      dia = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR, 
					GTK_BUTTONS_CLOSE,
					NULL);

      gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dia), msg);
      gtk_dialog_run(GTK_DIALOG(dia));
      g_free(msg);
      exit(1);
   }
   
   // we can't get a lock?!?
   if(GetLock(SynapticLock, true) < 0) {
      _error->DumpErrors();
      exit(1);
   }
   // if we run nonInteracitvely, get a seond lock
   if(weNonInteractive && GetLock(SynapticNonInteractiveLock, true) < 0) {
      _error->DumpErrors();
      exit(1);
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
   
   // read the cmdline
   CommandLine CmdL(Args, _config);

   if (CmdL.Parse(argc, (const char **)argv) == false) {
      RGUserDialog userDialog;
      userDialog.showErrors();
      exit(1);
   }
   
   if (_config->FindB("help") == true)
      ShowHelp(CmdL);

   if (getuid() != 0) {
      RGUserDialog userDialog;
      userDialog.warning(g_strdup_printf("<b><big>%s</big></b>\n\n%s",
                                         _("Starting \"Synaptic Package Manager\" without "
                                           "administrative privileges"),
				         _("You will not be able to apply "
				           "any changes, but you can still "
					   "export the marked changes or "
					   "create a download script "
					   "for them.")));
   }

   if (!RInitConfiguration("synaptic.conf")) {
      RGUserDialog userDialog;
      userDialog.showErrors();
      exit(1);
   }

   bool UpdateMode = _config->FindB("Volatile::Update-Mode",false);
   bool NonInteractive = _config->FindB("Volatile::Non-Interactive", false);

   // check if there is another application runing and
   // act accordingly
   check_and_aquire_lock();

   // read configuration early
   _roptions->restore();

   SetLanguages();

   // init the static pkgStatus class. this loads the status pixmaps 
   // and colors
   RGPackageStatus::pkgStatus.init();

   RPackageLister *packageLister = new RPackageLister();
   RGMainWindow *mainWindow = new RGMainWindow(packageLister, "main");

   // install a sigusr1 signal handler and put window into 
   // foreground when called. use the io_watch trick because gtk is not
   // reentrant
   // SIGUSR1 handling via pipes  
   if (pipe (sigterm_unix_signal_pipe_fds) != 0) {
      g_warning ("Could not setup pipe, errno=%d", errno);
      return 1;
   }
   sigterm_iochn = g_io_channel_unix_new (sigterm_unix_signal_pipe_fds[0]);
   if (sigterm_iochn == NULL) {
      g_warning("Could not create GIOChannel");
      return 1;
   }
   g_io_add_watch (sigterm_iochn, G_IO_IN, sigterm_iochn_data, mainWindow);
   signal (SIGUSR1, handle_sigusr1);
   // -------------------------------------------------------------

   // read which default distro to use
   string s = _config->Find("Synaptic::DefaultDistro", "");
   if (s != "")
      _config->Set("APT::Default-Release", s);

#ifndef HAVE_RPM
   mainWindow->setTitle(_("Synaptic Package Manager "));
#else
   mainWindow->setTitle(_config->Find("Synaptic::MyName", "Synaptic"));
#endif
   // this is for stuff like "synaptic -t `uname -n`"
   s = _config->Find("Volatile::MyName","");
   if(s.size() > 0)
      mainWindow->setTitle(s);
   
   if(_config->FindB("Volatile::HideMainwindow", false))
      mainWindow->hide();
   else
      mainWindow->show();

   RGFlushInterface();

   mainWindow->setInterfaceLocked(true);

   string cd_mount_point = _config->Find("Volatile::AddCdrom-Mode", "");
   if(!cd_mount_point.empty()) {
      _config->Set("Acquire::cdrom::mount",cd_mount_point);
      _config->Set("APT::CDROM::NoMount", true);
      mainWindow->cbAddCDROM(NULL, mainWindow);
   } else if(_config->FindB("Volatile::AskCdrom-Mode",false)) {
      mainWindow->cbAddCDROM(NULL, mainWindow);
      return 0;
   }

   //no need to open a cache that will invalid after the update
   if(!UpdateMode) {
      mainWindow->setTreeLocked(true);
      if(!packageLister->openCache()) {
	 mainWindow->showErrors();
	 exit(1);
      }
      mainWindow->restoreState();
      mainWindow->showErrors();
      mainWindow->setTreeLocked(false);
   }
   
   if (_config->FindB("Volatile::startInRepositories", false)) {
      mainWindow->cbShowSourcesWindow(NULL, mainWindow);
   }

   // selections from stdin
   if (_config->FindB("Volatile::Set-Selections", false) == true) {
      packageLister->unregisterObserver(mainWindow);
      packageLister->readSelections(cin);
      packageLister->registerObserver(mainWindow);
   }

   // selections from a file
   string selections_filename;
   selections_filename = _config->Find("Volatile::Set-Selections-File", "");
   if (selections_filename != "") {
      packageLister->unregisterObserver(mainWindow);
      ifstream selfile(selections_filename.c_str());
      packageLister->readSelections(selfile);
      selfile.close();
      packageLister->registerObserver(mainWindow);
   }

   mainWindow->setInterfaceLocked(false);

   if(UpdateMode) {
      mainWindow->cbUpdateClicked(NULL, mainWindow);
      mainWindow->setTreeLocked(true);
      if(!packageLister->openCache()) {
	 mainWindow->showErrors();
	 exit(1);
      }
      mainWindow->restoreState();
      mainWindow->setTreeLocked(false);
      mainWindow->showErrors();
      mainWindow->changeView(PACKAGE_VIEW_STATUS, _("Installed (upgradable)"));
   }

   if(_config->FindB("Volatile::TestMeHarder", false))
   {
      _config->Set("Volatile::Non-Interactive","true");
      _config->Set("Synaptic::closeZvt","true");

      while(true)
      {
	 mainWindow->cbUpdateClicked(NULL, mainWindow);
	 mainWindow->changeView(PACKAGE_VIEW_STATUS, _("Installed"));
	 GtkTreePath *p = gtk_tree_path_new_from_string("0");
	 mainWindow->cbPackageListRowActivated(NULL, p, NULL, mainWindow);

	 GObject *o = (GObject*)g_object_new( G_TYPE_OBJECT,NULL);
	 g_object_set_data(o, "me", mainWindow);
	 mainWindow->cbPkgAction(GTK_WIDGET(o), (void*)PKG_REINSTALL);
	 mainWindow->cbProceedClicked(NULL, mainWindow);
      }
   }

   if(_config->FindB("Volatile::Upgrade-Mode",false) 
      || _config->FindB("Volatile::DistUpgrade-Mode",false) ) {
      mainWindow->cbUpgradeClicked(NULL, mainWindow);
      mainWindow->changeView(PACKAGE_VIEW_CUSTOM, _("Marked Changes"));
   }

   if(_config->FindB("Volatile::TaskWindow",false)) {
      mainWindow->cbTasksClicked(NULL, mainWindow);
   }

   string filter = _config->Find("Volatile::initialFilter","");
   if(filter != "")
      mainWindow->changeView(PACKAGE_VIEW_CUSTOM, filter);

   if (NonInteractive) {
      mainWindow->cbProceedClicked(NULL, mainWindow);
   } else {
      welcome_dialog(mainWindow);
      gtk_widget_grab_focus( GTK_WIDGET(gtk_builder_get_object(
                                          mainWindow->getGtkBuilder(),
                                          "entry_fast_search")));
#if 0
      update_check(mainWindow, packageLister);
#endif 
      gtk_main();
   }

   return 0;
}


// vim:sts=4:sw=4
