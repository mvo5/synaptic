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

// we compile only if ZVT is selected
#ifdef HAVE_TERMINAL

#include "i18n.h"

#include "rgzvtinstallprogress.h"
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

#ifdef HAVE_ZVT
#include <libzvt/libzvt.h>
#endif
#ifdef HAVE_VTE
#include <vte/vte.h>
#endif


using namespace std;

void RGZvtInstallProgress::startUpdate()
{
   show();
   gtk_label_set_markup(GTK_LABEL(_statusL), _("<i>Running...</i>"));
   gtk_widget_set_sensitive(_closeB, false);
   RGFlushInterface();
}

void RGZvtInstallProgress::finishUpdate()
{
   string finishMsg = _("\nSuccessfully applied all changes. You can close the window now");
   string errorMsg =
      _("\nFailed to apply all changes! Scroll in this buffer to see what went wrong");

   gtk_widget_set_sensitive(_closeB, true);

   RGFlushInterface();
   updateFinished = true;

   _config->Set("Synaptic::closeZvt",
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_closeOnF))
                ? "true" : "false");

   if (!RWriteConfigFile(*_config)) {
      _error->Error(_("An error occurred while saving configurations."));
      RGUserDialog userDialog(this);
      userDialog.showErrors();
   }

   if (res == 0 && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_closeOnF))) {
      hide();
      return;
   }
#ifdef HAVE_ZVT
   if (res == 0)
      zvt_term_feed(ZVT_TERM(_term), (char *)finishMsg.c_str(),
                    (long)finishMsg.size());
   else
      zvt_term_feed(ZVT_TERM(_term), (char *)errorMsg.c_str(),
                    (long)errorMsg.size());
#endif
#ifdef HAVE_VTE
   if (res == 0)
      vte_terminal_feed(VTE_TERMINAL(_term), utf8(finishMsg.c_str()),
                        (long)finishMsg.size());
   else
      vte_terminal_feed(VTE_TERMINAL(_term), utf8(errorMsg.c_str()),
                        (long)errorMsg.size());
#endif
   gtk_label_set_markup(GTK_LABEL(_statusL), _("<i>Finished</i>"));
}

void RGZvtInstallProgress::stopShell(GtkWidget *self, void *data)
{
   RGZvtInstallProgress *me = (RGZvtInstallProgress *) data;

   if (!me->updateFinished) {
      gtk_label_set_markup(GTK_LABEL(me->_statusL),
                           _("<i>Can't close while running</i>"));
      return;
   }

   RGFlushInterface();
   me->hide();
}

bool RGZvtInstallProgress::close()
{
   stopShell(NULL, this);
   return true;
}

gboolean RGZvtInstallProgress::zvtFocus(GtkWidget *widget,
                                        GdkEventButton * event,
                                        gpointer user_data)
{
   //cout << "zvtFocus" << endl;

   gtk_widget_grab_focus(widget);

   return FALSE;
}

RGZvtInstallProgress::RGZvtInstallProgress(RGMainWindow *main)
: RInstallProgress(), RGGladeWindow(main, "zvtinstallprogress"),
updateFinished(false)
{
   setTitle(_("Applying Changes..."));

   GtkWidget *hbox = glade_xml_get_widget(_gladeXML, "hbox_terminal");
   assert(hbox);

#ifdef HAVE_ZVT
   // libzvt will segfault if the locales are not supported, 
   // we workaround this here
   char *s = NULL;
   if (!XSupportsLocale()) {
      s = getenv("LC_ALL");
      unsetenv("LC_ALL");
      gtk_set_locale();
   }
   _term = zvt_term_new_with_size(80, 24);
   if (_config->FindB("Synaptic::useUserTerminalFont")) {
      char *s = (char *)_config->Find("Synaptic::TerminalFontName").c_str();
      zvt_term_set_font_name(ZVT_TERM(_term), s);
   }
   if (s != NULL) {
      setenv("LC_ALL", s, 1);
      gtk_set_locale();
   }

   zvt_term_set_size(ZVT_TERM(_term), 80, 24);
   gtk_widget_set_size_request(GTK_WIDGET(_term),
                               (ZVT_TERM(_term)->grid_width *
                                ZVT_TERM(_term)->charwidth) + 2 /*padding */  +
                               (GTK_WIDGET(_term)->style->xthickness * 2),
                               (ZVT_TERM(_term)->grid_height *
                                ZVT_TERM(_term)->charheight) +
                               2 /*padding */  +
                               (GTK_WIDGET(_term)->style->ythickness * 2));
   zvt_term_set_size(ZVT_TERM(_term), 80, 24);

   zvt_term_set_scroll_on_output(ZVT_TERM(_term), TRUE);
   zvt_term_set_scroll_on_keystroke(ZVT_TERM(_term), TRUE);
   zvt_term_set_auto_window_hint(ZVT_TERM(_term), FALSE);
   zvt_term_set_scrollback(ZVT_TERM(_term), 10000);
   GtkWidget *scrollbar =
      gtk_vscrollbar_new(GTK_ADJUSTMENT(ZVT_TERM(_term)->adjustment));
   GTK_WIDGET_UNSET_FLAGS(scrollbar, GTK_CAN_FOCUS);
   g_signal_connect(G_OBJECT(_term), "button-press-event",
                    (void (*)())zvtFocus, this);
#endif
#ifdef HAVE_VTE
   _term = vte_terminal_new();
   if (_config->FindB("Synaptic::useUserTerminalFont")) {
      char *s = (char *)_config->Find("Synaptic::TerminalFontName").c_str();
      vte_terminal_set_font_from_string(VTE_TERMINAL(_term), s);
   }
#endif
   //GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(hbox), _term, TRUE, TRUE, 0);
   gtk_widget_show(_term);
#ifdef HAVE_ZVT
   gtk_box_pack_end(GTK_BOX(hbox), scrollbar, FALSE, FALSE, 0);
   gtk_widget_show(scrollbar);
#endif

   _closeOnF = glade_xml_get_widget(_gladeXML, "checkbutton_close_after_pm");
   assert(_closeOnF);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_closeOnF),
                                _config->FindB("Synaptic::closeZvt", false));

   _statusL = glade_xml_get_widget(_gladeXML, "label_status");
   _closeB = glade_xml_get_widget(_gladeXML, "button_close");
   gtk_signal_connect(GTK_OBJECT(_closeB), "clicked",
                      (GtkSignalFunc) stopShell, this);
}



pkgPackageManager::OrderResult
RGZvtInstallProgress::start(RPackageManager *pm,
                            int numPackages, int numPackagesTotal)
{
   void *dummy;
   int open_max, ret = 250;
   pid_t _child_id;

   res = pm->DoInstallPreFork();
   if (res == pkgPackageManager::Failed)
       return res;

#ifdef HAVE_ZVT
   _child_id = zvt_term_forkpty(ZVT_TERM(_term), FALSE);
#endif
#ifdef HAVE_VTE
   _child_id =
      vte_terminal_forkpty(VTE_TERMINAL(_term), NULL, NULL, false, false,
                           false);
#endif
   if (_child_id == -1) {
      cerr <<
         "Internal Error: impossible to fork children. Synaptics is going to stop. Please report."
         << endl;
      cerr << "errorcode: " << errno << endl;
      exit(1);
   }

   if (_child_id == 0) {
      // Close all file descriptors but first 3
      open_max = sysconf(_SC_OPEN_MAX);
      for (int i = 3; i < open_max; i++)
         ::close(i);
      // make sure, that term is set correctly
      setenv("TERM", "xterm", 1);
      res = pm->DoInstallPostFork();

      _exit(res);
   }

   startUpdate();

   while(waitpid(_child_id, &ret,WNOHANG) == 0)
       updateInterface();

#ifdef HAVE_ZVT
   res = (pkgPackageManager::OrderResult)zvt_term_closepty(ZVT_TERM(_term));
#endif
   //FIXME: how to get the exit-status from vte?

   finishUpdate();

   return res;
}

void RGZvtInstallProgress::updateInterface()
{
   if (gtk_events_pending()) {
      while (gtk_events_pending())
         gtk_main_iteration();
   } else {
      usleep(1000);
   }
}


#endif

// vim:ts=3:sw=3:et
