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


static gint applicationHandleLocalOptions(GApplication* app,
                                          GVariantDict* options,
                                          gpointer user_data);
static void applicationStartup(GApplication* app, gpointer user_data);
static void applicationActivate(GApplication* app, gpointer user_data);


static GOptionEntry option_entries[] = {
   { "repositories",          'r',  G_OPTION_FLAG_NONE,   G_OPTION_ARG_NONE,         nullptr, N_("Open in the repository screen"), nullptr },
   { "filter-file",           'f',  G_OPTION_FLAG_NONE,   G_OPTION_ARG_FILENAME,     nullptr, N_("Give an alternative filter file"), N_("<filter-file>") },
   { "title",                 't',  G_OPTION_FLAG_NONE,   G_OPTION_ARG_STRING,       nullptr, N_("Give an alternative main window title (e.g. hostname with `uname -n`)"), N_("title") },
   { "initial-filter",        'i',  G_OPTION_FLAG_NONE,   G_OPTION_ARG_STRING,       nullptr, N_("Start with the initial filter with given name"), N_("<filter-name>") },
   { "option",                'o',  G_OPTION_FLAG_NONE,   G_OPTION_ARG_STRING_ARRAY, nullptr, N_("Set an arbitrary configuration option, eg -o dir::cache=/tmp"), N_("<option>") },
   { "upgrade-mode",          '\0', G_OPTION_FLAG_NONE,   G_OPTION_ARG_NONE,         nullptr, N_("Call Upgrade and display changes"), nullptr },
   { "dist-upgrade-mode",     '\0', G_OPTION_FLAG_NONE,   G_OPTION_ARG_NONE,         nullptr, N_("Call DistUpgrade and display changes"), nullptr },
   { "update-at-startup",     '\0', G_OPTION_FLAG_NONE,   G_OPTION_ARG_NONE,         nullptr, N_("Call \"Reload\" on startup"), nullptr },
   { "non-interactive",       '\0', G_OPTION_FLAG_NONE,   G_OPTION_ARG_NONE,         nullptr, N_("Never prompt for user input"), nullptr },
   { "task-window",           '\0', G_OPTION_FLAG_NONE,   G_OPTION_ARG_NONE,         nullptr, N_("Open with task window"), nullptr },
   { "add-cdrom",             '\0', G_OPTION_FLAG_NONE,   G_OPTION_ARG_STRING,       nullptr, N_("Add a cdrom at startup"), N_("<path-for-cdrom>") },
   { "ask-cdrom",             '\0', G_OPTION_FLAG_NONE,   G_OPTION_ARG_NONE,         nullptr, N_("Ask for adding a cdrom and exit"), nullptr },
   { "test-me-harder",        '\0', G_OPTION_FLAG_NONE,   G_OPTION_ARG_NONE,         nullptr, N_("Run test in a loop"), nullptr },
   { "set-selections",        '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,         nullptr, nullptr, nullptr },
   { "set-selections-file",   '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING,       nullptr, nullptr, nullptr },
   { "parent-window-id",      '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING,       nullptr, nullptr, nullptr },
   { "progress-str",          '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING,       nullptr, nullptr, nullptr },
   { "finish-str",            '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING,       nullptr, nullptr, nullptr },
   { "hide-main-window",      '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,         nullptr, nullptr, nullptr },
   { 0 }
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
	    mainWindow->cbUpdateClicked(nullptr, nullptr, mainWindow);
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
	       mainWindow->cbUpdateClicked(nullptr, nullptr, mainWindow);
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

   GtkApplication *app = gtk_application_new(
      "io.github.mvo5.synaptic",
      G_APPLICATION_DEFAULT_FLAGS);

   g_application_add_main_option_entries(G_APPLICATION(app), option_entries);

   g_signal_connect(app, "startup",
      G_CALLBACK(applicationStartup), nullptr);
   g_signal_connect(app, "handle-local-options",
      G_CALLBACK(applicationHandleLocalOptions), nullptr);
   g_signal_connect(app, "activate",
      G_CALLBACK(applicationActivate), nullptr);

   int status = g_application_run(G_APPLICATION(app), argc, argv);
   g_object_unref(app);

   return status;
}

static gint applicationHandleLocalOptions(GApplication* app,
                                          GVariantDict* options,
                                          gpointer user_data)
{
   gboolean flag;
   gchar *arg;
   gchar **args;

   if (g_variant_dict_lookup(options, "repositories", "b", &flag))
      _config->Set("Volatile::startInRepositories", flag ? "true" : "false");
   if (g_variant_dict_lookup(options, "filter-file", "&s", &arg))
      _config->Set("Volatile::filterFile", arg);
   if (g_variant_dict_lookup(options, "title", "&s", &arg))
      _config->Set("Volatile::MyName", arg);
   if (g_variant_dict_lookup(options, "initial-filter", "&s", &arg))
      _config->Set("Volatile::initialFilter", arg);
   if (g_variant_dict_lookup(options, "option", "^a&s", &args)) {
      for (int i = 0; args[i] != nullptr; ++i) {
         arg = args[i];
         printf ("OPTION %s\n", arg); fflush(stdout);
         const char *p = strchr(arg, '=');
         if (p)
            _config->Set(string(arg, p - arg), p + 1);
         else
            g_warning (_("Option %s: Configuration item specification must have an =<val>."), arg);
      }
      g_free(args);
   }
   if (g_variant_dict_lookup(options, "upgrade-mode", "b", &flag))
      _config->Set("Volatile::Upgrade-Mode", flag ? "true" : "false");
   if (g_variant_dict_lookup(options, "dist-upgrade-mode", "b", &flag))
      _config->Set("Volatile::DistUpgrade-Mode", flag ? "true" : "false");
   if (g_variant_dict_lookup(options, "update-at-startup", "b", &flag))
      _config->Set("Volatile::Update-Mode", flag ? "true" : "false");
   if (g_variant_dict_lookup(options, "non-interactive", "b", &flag))
      _config->Set("Volatile::Non-Interactive", flag ? "true" : "false");
   if (g_variant_dict_lookup(options, "task-window", "b", &flag))
      _config->Set("Volatile::TaskWindow", flag ? "true" : "false");
   if (g_variant_dict_lookup(options, "add-cdrom", "&s", &arg))
      _config->Set("Volatile::AddCdrom-Mode", arg);
   if (g_variant_dict_lookup(options, "ask-cdrom", "b", &flag))
      _config->Set("Volatile::AskCdrom-Mode", flag ? "true" : "false");
   if (g_variant_dict_lookup(options, "test-me-harder", "b", &flag))
      _config->Set("Volatile::TestMeHarder", flag ? "true" : "false");
   if (g_variant_dict_lookup(options, "set-selections", "b", &flag))
      _config->Set("Volatile::Set-Selections", flag ? "true" : "false");
   if (g_variant_dict_lookup(options, "set-selections-file", "&s", &arg))
      _config->Set("Volatile::Set-Selections-File", arg);
   if (g_variant_dict_lookup(options, "parent-window-id", "&s", &arg))
      _config->Set("Volatile::ParentWindowId", arg);
   if (g_variant_dict_lookup(options, "progress-str", "&s", &arg))
      _config->Set("Volatile::InstallProgressStr", arg);
   if (g_variant_dict_lookup(options, "finish-str", "&s", &arg))
      _config->Set("Volatile::InstallFinishedStr", arg);
   if (g_variant_dict_lookup(options, "hide-main-window", "b", &flag))
      _config->Set("Volatile::HideMainwindow", flag ? "true" : "false");

   return -1;
}

static void applicationStartup(GApplication* app, gpointer user_data)
{
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

   // check if there is another application runing and
   // act accordingly
   check_and_aquire_lock();

   // read configuration early
   _roptions->restore();

   SetLanguages();

   // init the static pkgStatus class. this loads the status pixmaps 
   // and colors
   RGPackageStatus::pkgStatus.init();
}

static void applicationActivate(GApplication* app, gpointer user_data)
{
   if (auto window = gtk_application_get_active_window(GTK_APPLICATION(app))) {
      gtk_window_present(window);
      return;
   }

   bool UpdateMode = _config->FindB("Volatile::Update-Mode",false);
   bool NonInteractive = _config->FindB("Volatile::Non-Interactive", false);

   RPackageLister *packageLister = new RPackageLister();
   RGMainWindow *mainWindow = new RGMainWindow(GTK_APPLICATION(app), packageLister, "main");

   // install a sigusr1 signal handler and put window into 
   // foreground when called. use the io_watch trick because gtk is not
   // reentrant
   // SIGUSR1 handling via pipes  
   if (pipe (sigterm_unix_signal_pipe_fds) != 0) {
      g_warning ("Could not setup pipe, errno=%d", errno);
      exit(1);
   }
   sigterm_iochn = g_io_channel_unix_new (sigterm_unix_signal_pipe_fds[0]);
   if (sigterm_iochn == NULL) {
      g_warning("Could not create GIOChannel");
      exit(1);
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
      mainWindow->cbAddCDROM(nullptr, nullptr, mainWindow);
   } else if(_config->FindB("Volatile::AskCdrom-Mode",false)) {
      mainWindow->cbAddCDROM(nullptr, nullptr, mainWindow);
      exit(0);
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
      mainWindow->cbShowSourcesWindow(NULL, NULL, mainWindow);
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
      mainWindow->cbUpdateClicked(nullptr, nullptr, mainWindow);
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
	 mainWindow->cbUpdateClicked(nullptr, nullptr, mainWindow);
	 mainWindow->changeView(PACKAGE_VIEW_STATUS, _("Installed"));
	 GtkTreePath *p = gtk_tree_path_new_from_string("0");
	 mainWindow->cbPackageListRowActivated(NULL, p, NULL, mainWindow);

	 GObject *o = (GObject*)g_object_new( G_TYPE_OBJECT,NULL);
	 g_object_set_data(o, "me", mainWindow);
	 mainWindow->cbPkgAction(PKG_REINSTALL);
	 mainWindow->cbProceedClicked(nullptr, nullptr, mainWindow);
      }
   }

   if(_config->FindB("Volatile::Upgrade-Mode",false) 
      || _config->FindB("Volatile::DistUpgrade-Mode",false) ) {
      mainWindow->cbUpgradeClicked(nullptr, nullptr, mainWindow);
      mainWindow->changeView(PACKAGE_VIEW_CUSTOM, _("Marked Changes"));
   }

   if(_config->FindB("Volatile::TaskWindow",false)) {
      mainWindow->cbTasksClicked(nullptr, nullptr, mainWindow);
   }

   string filter = _config->Find("Volatile::initialFilter","");
   if(filter != "")
      mainWindow->changeView(PACKAGE_VIEW_CUSTOM, filter);

   if (NonInteractive) {
      mainWindow->cbProceedClicked(nullptr, nullptr, mainWindow);
      exit(0);
   } else {
      welcome_dialog(mainWindow);
      gtk_widget_grab_focus( GTK_WIDGET(gtk_builder_get_object(
                                          mainWindow->getGtkBuilder(),
                                          "entry_fast_search")));
   }
}

// vim:sts=4:sw=4
