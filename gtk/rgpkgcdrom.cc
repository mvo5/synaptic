/* rgpkgcdrom.cc - make use of the apt-pkg/cdrom.h code
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2004 Canonical
 * 
 * Authors: Alfredo K. Kojima <kojima@conectiva.com.br>
 *          Michael Vogt <michael.vogt@ubuntu.com>
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
#ifdef HAVE_APTPKG_CDROM

#include "rgmainwindow.h"
#include "rgpkgcdrom.h"
#include "gsynaptic.h"

#include <unistd.h>
#include <stdio.h>

#include "i18n.h"

class RGDiscName : public RGGtkBuilderWindow 
{
 protected:

   GtkWidget *_textEntry;
   bool _userConfirmed;

   static void onOkClicked(GtkWidget *self, void *data);
   static void onCancelClicked(GtkWidget *self, void *data);

 public:
   RGDiscName(RGWindow *wwin, const string defaultName);

   bool run(string &name);
};


void RGCDScanner::Update(string text, int current)
{
   if(text.size() > 0)
      gtk_label_set_text(GTK_LABEL(_label), text.c_str());

   if(current > 0)
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbar),
				    ((float)current) / totalSteps);
   show();
   RGFlushInterface();
}

bool RGCDScanner::ChangeCdrom()
{
   return _userDialog->proceed(_("Please insert a disc in the drive."));
}

bool RGCDScanner::AskCdromName(string &name)
{
   //cout << "askCdromName()" << endl;
   RGDiscName discName(this, name);
   
   if (!discName.run(name)) {
      return false;
   }

   return true;
}

RGCDScanner::RGCDScanner(RGMainWindow *main, RUserDialog *userDialog)
: pkgCdromStatus(), RGWindow(main, "cdscanner", true, false)
{
   setTitle(_("Scanning CD-ROM"));

   _userDialog = userDialog;

   gtk_window_set_default_size(GTK_WINDOW(_win), 300, 120);

   gtk_container_set_border_width(GTK_CONTAINER(_topBox), 10);

   _label = gtk_label_new("\n\n");
   gtk_widget_show(_label);
   gtk_box_pack_start(GTK_BOX(_topBox), _label, TRUE, TRUE, 10);

   _pbar = gtk_progress_bar_new();
   gtk_widget_show(_pbar);
   gtk_widget_set_size_request(_pbar, -1, 25);
   gtk_box_pack_start(GTK_BOX(_topBox), _pbar, FALSE, TRUE, 0);

   //gtk_window_set_skip_taskbar_hint(GTK_WINDOW(_win), TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(_win), 
                                GTK_WINDOW(main->window()));
   gtk_window_set_position(GTK_WINDOW(_win),
			   GTK_WIN_POS_CENTER_ON_PARENT);
}

bool RGCDScanner::run()
{
   pkgCdrom scanner;

   return scanner.Add(this);
}



RGDiscName::RGDiscName(RGWindow *wwin, const string defaultName)
: RGGtkBuilderWindow(wwin, "disc_name")
{
   setTitle(_("Disc Label"));
   _textEntry = GTK_WIDGET(gtk_builder_get_object(_builder, "text_entry"));
   gtk_entry_set_text(GTK_ENTRY(_textEntry), defaultName.c_str());

   g_signal_connect(gtk_builder_get_object(_builder, "ok"),
                    "clicked",
                    G_CALLBACK(onOkClicked), this);
   g_signal_connect(gtk_builder_get_object(_builder, "cancel"),
                    "clicked",
                    G_CALLBACK(onCancelClicked), this);
   gtk_window_set_skip_taskbar_hint(GTK_WINDOW(_win), TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(_win), 
                                GTK_WINDOW(wwin->window()));
   gtk_window_set_position(GTK_WINDOW(_win),
			   GTK_WIN_POS_CENTER_ON_PARENT);
}

void RGDiscName::onOkClicked(GtkWidget *self, void *data)
{
   RGDiscName *me = (RGDiscName *) data;
   me->_userConfirmed = true;
   gtk_main_quit();
}

void RGDiscName::onCancelClicked(GtkWidget *self, void *data)
{
   gtk_main_quit();
}

bool RGDiscName::run(string &discName)
{
   _userConfirmed = false;
   show();
   gtk_main();
   discName = gtk_entry_get_text(GTK_ENTRY(_textEntry));
   return _userConfirmed;
}

#endif

// vim:sts=4:sw=4
