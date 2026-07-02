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

   task<bool> run(string &name);
};


void RGCDScanner::Update(string text, int current)
{
   if(text.size() > 0)
      gtk_label_set_text(GTK_LABEL(_label), text.c_str());

   if(current > 0)
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbar),
				    ((float)current) / totalSteps);
   show();
}

bool RGCDScanner::ChangeCdrom()
{
   // TODO
   // co_return co_await _userDialog->proceed(_("Please insert a disc in the drive."));
   return false;
}

bool RGCDScanner::AskCdromName(string &name)
{
   //cout << "askCdromName()" << endl;
   RGDiscName discName(this, name);
   // TODO
//   co_return co_await discName.run(name);
   return false;
}

RGCDScanner::RGCDScanner(RGMainWindow *main, RUserDialog *userDialog)
: pkgCdromStatus(), RGWindow(main, "cdscanner", true, false)
{
   setTitle(_("Scanning CD-ROM"));

   _userDialog = userDialog;

   gtk_window_set_default_size(GTK_WINDOW(_win), 300, 120);

   gtk_box_set_spacing(GTK_BOX(_topBox), 10);
   gtk_widget_set_margin_top(_topBox, 10);
   gtk_widget_set_margin_bottom(_topBox, 10);
   gtk_widget_set_margin_start(_topBox, 10);
   gtk_widget_set_margin_end(_topBox, 10);

   _label = gtk_label_new("\n\n");
   gtk_widget_set_hexpand(_label, true);
   gtk_widget_set_vexpand(_label, true);
   gtk_box_append(GTK_BOX(_topBox),_label);

   _pbar = gtk_progress_bar_new();
   gtk_widget_show(_pbar);
   gtk_widget_set_size_request(_pbar, -1, 25);
   gtk_box_append(GTK_BOX(_topBox), _pbar);

   //gtk_window_set_skip_taskbar_hint(GTK_WINDOW(_win), TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(_win), 
                                GTK_WINDOW(main->window()));
}

task<bool> RGCDScanner::run()
{
   pkgCdrom scanner;

   bool result = scanner.Add(this);
   co_return result;
}



RGDiscName::RGDiscName(RGWindow *wwin, const string defaultName)
: RGGtkBuilderWindow(wwin, "disc_name")
{
   setTitle(_("Disc Label"));
   _textEntry = GTK_WIDGET(gtk_builder_get_object(_builder, "text_entry"));
   gtk_editable_set_text(GTK_EDITABLE(_textEntry), defaultName.c_str());

   g_signal_connect(gtk_builder_get_object(_builder, "ok"),
                    "clicked",
                    G_CALLBACK(onOkClicked), this);
   g_signal_connect(gtk_builder_get_object(_builder, "cancel"),
                    "clicked",
                    G_CALLBACK(onCancelClicked), this);
   gtk_window_set_transient_for(GTK_WINDOW(_win), 
                                GTK_WINDOW(wwin->window()));
}

void RGDiscName::onOkClicked(GtkWidget *self, void *data)
{
   RGDiscName *me = (RGDiscName *) data;
   me->_userConfirmed = true;
   gtk_window_close (GTK_WINDOW (me->_win));
}

void RGDiscName::onCancelClicked(GtkWidget *self, void *data)
{
   RGDiscName *me = (RGDiscName *) data;
   gtk_window_close (GTK_WINDOW (me->_win));
}

task<bool> RGDiscName::run(string &discName)
{
   _userConfirmed = false;
   co_await co_run_window (GTK_WINDOW (_win));
   discName = gtk_editable_get_text(GTK_EDITABLE(_textEntry));
   co_return _userConfirmed;
}

#endif

// vim:sts=4:sw=4
