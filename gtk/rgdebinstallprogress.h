/* rgdebinstallprogress.h
 *
 * Copyright (c) 2004 Canonical 
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


#ifndef _RGDEBINSTALLPROGRESS_H_
#define _RGDEBINSTALLPROGRESS_H_

#include "config.h"

#ifdef WITH_DPKG_STATUSFD

#include "rinstallprogress.h"
#include "rggtkbuilderwindow.h"
#include "rguserdialog.h"
#include<map>
#include <vte/reaper.h>


class RGMainWindow;


class RGDebInstallProgress:public RInstallProgress, public RGGtkBuilderWindow 
{
   typedef enum {
      EDIT_COPY,
      EDIT_SELECT_ALL,
      EDIT_SELECT_NONE,
   } TermAction;

   // the various stages of dpkg
   static const int NR_REMOVE_STAGES=3;
   static char* remove_stages[NR_REMOVE_STAGES];
   static char* remove_stages_translations[NR_REMOVE_STAGES];

   static const int NR_PURGE_STAGES=4;
   static char *purge_stages[NR_PURGE_STAGES];
   static char *purge_stages_translations[NR_PURGE_STAGES];

   // purge a already removed pkg
   static const int NR_PURGE_ONLY_STAGES=2;
   static char *purge_only_stages[NR_PURGE_ONLY_STAGES]; 
   static char *purge_only_stages_translations[NR_PURGE_ONLY_STAGES]; 

   static const int NR_INSTALL_STAGES=4;
   static char *install_stages[NR_INSTALL_STAGES];
   static char *install_stages_translations[NR_INSTALL_STAGES];

   static const int NR_UPDATE_STAGES=5;
   static char *update_stages[NR_UPDATE_STAGES];
   static char *update_stages_translations[NR_UPDATE_STAGES];

   static const int NR_REINSTALL_STAGES=6;
   static char *reinstall_stages[NR_REINSTALL_STAGES];
   static char *reinstall_stages_translations[NR_REINSTALL_STAGES];


   // widgets
   GtkWidget *_label_status;
   GtkWidget *_labelSummary;

   GtkWidget *_pbarTotal;
   GtkWidget *_term;
   GtkWidget *_autoClose; // checkbutton

   GtkWidget *_popupMenu; // Popup menu of the terminal

   // if we run embedded
   GtkWidget *_sock;

   bool _startCounting;

   RGUserDialog *_userDialog;

   int _progress;
   int _totalActions;

   // when the internal terminal timesout after no activity
   int _terminalTimeout;

   // this map contains the name and a pointer to the stages arrays
   map<string, char**> _actionsMap;

   // this map contains the name and a pointer to the translation arrays
   map<string, char**> _translationsMap;

   // this map contains what stage is already completted
   map<string, int> _stagesMap;

   // last time something changed
   time_t last_term_action;

   pid_t _child_id;
   pkgPackageManager::OrderResult res;
   bool child_has_exited;
   static void child_exited(VteReaper *vtereaper,gint child_pid, gint ret,
			    gpointer data);
   static void terminalAction(GtkWidget *terminal, TermAction action);


 protected:
   virtual void startUpdate();
   virtual void updateInterface();
   virtual void finishUpdate();
   virtual bool close();

   virtual pkgPackageManager::OrderResult start(pkgPackageManager *pm,
		   				int numPackages = 0,
						int totalPackages = 0);

   virtual void prepare(RPackageLister *lister);
   
   void conffile(gchar *conffile, gchar *status);

   // gtk stuff
   static void cbCancel(GtkWidget *self, void *data);
   static void cbClose(GtkWidget *self, void *data);
   static void content_changed(GObject *object, gpointer    user_data);
   static void expander_callback(GObject *object,GParamSpec *param_spec,
				  gpointer    user_data);
   static gboolean key_press_event(GtkWidget   *widget,
				   GdkEventKey *event,
				   gpointer     user_data);
   static gboolean cbTerminalClicked(GtkWidget *widget, GdkEventButton *event,
           gpointer user_data);
   static void cbMenuitemClicked(GtkMenuItem *menuitem, gpointer user_data);

 public:
   RGDebInstallProgress(RGMainWindow *main, RPackageLister *lister);
   virtual ~RGDebInstallProgress();

};

#endif

#endif
