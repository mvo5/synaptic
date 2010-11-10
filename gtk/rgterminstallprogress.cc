/* rgzvtinstallprogress.cc
 *
 * Copyright (c) 2002 Michael Vogt
 *
 * Author: Michael Vogt <mvo@debian.org>
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

#ifdef HAVE_TERMINAL

#include "i18n.h"

#include "rgterminstallprogress.h"
#include "rgmainwindow.h"
#include "rguserdialog.h"
#include "rconfiguration.h"
#include "gsynaptic.h"

#include <X11/Xlib.h>
#include <iostream>
#include <cerrno>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/pkgsystem.h>
#include <cassert>
#include <cstring>
#include <cstdlib>

#include <vte/vte.h>
#include <vte/reaper.h>

#include <cstdlib>
#include <cstring>

using namespace std;


RGTermInstallProgress::RGTermInstallProgress(RGMainWindow *main) 
   : RInstallProgress(), RGGladeWindow(main, "zvtinstallprogress"), _sock(NULL)
{
   setTitle(_("Applying Changes"));

   _term = vte_terminal_new();
   _scrollbar = gtk_vscrollbar_new (vte_terminal_get_adjustment(VTE_TERMINAL(_term)));
   GTK_WIDGET_UNSET_FLAGS (_scrollbar, GTK_CAN_FOCUS);
   vte_terminal_set_scrollback_lines(VTE_TERMINAL(_term), 10000);
   if(_config->FindB("Synaptic::useUserTerminalFont")) {
      char *s =(char*)_config->Find("Synaptic::TerminalFontName").c_str();
      vte_terminal_set_font_from_string(VTE_TERMINAL(_term), s);
   } else {
      vte_terminal_set_font_from_string(VTE_TERMINAL(_term), "monospace 10");
   }


   _closeOnF = glade_xml_get_widget(_gladeXML, "checkbutton_close_after_pm");
   assert(_closeOnF);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_closeOnF), 
				_config->FindB("Synaptic::closeZvt", false));

   _statusL = glade_xml_get_widget(_gladeXML, "label_status");
   _closeB = glade_xml_get_widget(_gladeXML, "button_close");
   gtk_signal_connect(GTK_OBJECT(_closeB), "clicked",
		      (GtkSignalFunc)stopShell, this);

   if(_config->FindB("Volatile::Non-Interactive", false)) 
      gtk_widget_hide(_closeOnF);
}


void RGTermInstallProgress::child_exited(VteReaper *vtereaper,
					gint child_pid, gint ret, 
					gpointer data)
{
   RGTermInstallProgress *me = (RGTermInstallProgress*)data;
   if(child_pid == me->_child_id) {
//        cout << "child exited" << endl;
//        cout << "waitpid returned: " << WEXITSTATUS(ret) << endl;
      me->res = (pkgPackageManager::OrderResult)WEXITSTATUS(ret);
      me->child_has_exited=true;
   }
}

void RGTermInstallProgress::startUpdate()
{
   GtkWidget *win =  glade_xml_get_widget(_gladeXML, "window_zvtinstallprogress");
   GtkWidget *hbox = glade_xml_get_widget(_gladeXML, "hbox_terminal");
   assert(hbox);
   gtk_widget_show_all(win);
   gtk_box_pack_start(GTK_BOX(hbox), _term, TRUE, TRUE, 0);
   gtk_box_pack_end(GTK_BOX(hbox), _scrollbar, FALSE, FALSE, 0);
   gtk_widget_show(_term);
   gtk_widget_show(_scrollbar);

   child_has_exited=false;
   VteReaper* reaper = vte_reaper_get();
   g_signal_connect(G_OBJECT(reaper), "child-exited",
		    G_CALLBACK(child_exited),
		    this);
 
   // check if we run embedded
   int id = _config->FindI("Volatile::PlugProgressInto", -1);
   //cout << "Plug ID: " << id << endl;
   if (id > 0) {
      gtk_widget_hide(glade_xml_get_widget(_gladeXML, "window_zvtinstallprogress"));
      GtkWidget *vbox = glade_xml_get_widget(_gladeXML, "vbox_terminstall_progress");
      _sock =  gtk_plug_new(id);
      gtk_widget_reparent(vbox, _sock);
      gtk_widget_show(_sock);
   } else {
      show();
   }

   gtk_label_set_markup(GTK_LABEL(_statusL), _("<i>Running...</i>"));
   gtk_widget_set_sensitive(_closeB, false);
   RGFlushInterface();
}

void RGTermInstallProgress::finishUpdate()
{
   gtk_widget_set_sensitive(_closeB, true);
   
   RGFlushInterface();
   _updateFinished = true;

   _config->Set("Synaptic::closeZvt", 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_closeOnF))
		? "true" : "false");
   
   if (!RWriteConfigFile(*_config)) {
      _error->Error(_("An error occurred while saving configurations."));
      RGUserDialog userDialog(this);
      userDialog.showErrors();
   }
  
   if(res == 0 &&(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_closeOnF)) ||
		  _config->FindB("Volatile::Non-Interactive", false))) {
      hide();
      return;
   }

   const char *msg = _(getResultStr(res));
   vte_terminal_feed(VTE_TERMINAL(_term), utf8_to_locale(msg), strlen(msg));
   gtk_label_set_markup(GTK_LABEL(_statusL), _("<i>Finished</i>"));
   if(res == 0)
      gtk_widget_grab_focus(GTK_WIDGET(_closeB));
}

void RGTermInstallProgress::stopShell(GtkWidget *self, void* data)
{
   RGTermInstallProgress *me = (RGTermInstallProgress*)data;  

   if(!me->_updateFinished) {
      gtk_label_set_markup(GTK_LABEL(me->_statusL), 
			   _("<i>Can't close while running</i>"));
      return;
   } 

   RGFlushInterface();
   if(me->_sock != NULL) {
      gtk_widget_destroy(me->_sock);
   } else {
      me->hide();
   }
}

bool RGTermInstallProgress::close()
{
   stopShell(NULL, this);
   return true;
}

gboolean RGTermInstallProgress::zvtFocus (GtkWidget *widget,
					 GdkEventButton *event,
					 gpointer user_data)
{
   //cout << "zvtFocus" << endl;
    
   gtk_widget_grab_focus(widget);
    
   return FALSE;
}




pkgPackageManager::OrderResult 
RGTermInstallProgress::start(pkgPackageManager *pm,
			    int numPackages,
			    int numPackagesTotal)
{
   //cout << "RGTermInstallProgress::start()" << endl;

   void *dummy;
   int open_max, ret = 250;

   res = pm->DoInstallPreFork();
   if (res == pkgPackageManager::Failed)
      return res;

   _child_id = vte_terminal_forkpty(VTE_TERMINAL(_term),NULL,NULL,false,false,false);
   if (_child_id == -1) {
      cerr << "Internal Error: impossible to fork children. Synaptics is going to stop. Please report." << endl;
      cerr << "errorcode: " << errno << endl;
      exit(1);
   }

   if (_child_id == 0) {

      // we ignore sigpipe as it is thrown sporadic on
      // debian, kernel 2.6 systems
      struct sigaction new_act;
      memset( &new_act, 0, sizeof( new_act ) );
      new_act.sa_handler = SIG_IGN;
      sigaction( SIGPIPE, &new_act, NULL);
      res = pm->DoInstallPostFork();
      _exit(res);
   }

   startUpdate();
   // make sure that the child has really exited and we catched the
   // return code
   while(!child_has_exited)
      updateInterface();

   finishUpdate();

   return res;
}

void RGTermInstallProgress::updateInterface()
{    
   if (gtk_events_pending()) {
      while (gtk_events_pending()) gtk_main_iteration();
   } else {
      // 0.1 secs 
      usleep(10000);
   }
}


#endif

// vim:sts=3:sw=3
