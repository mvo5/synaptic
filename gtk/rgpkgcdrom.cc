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

#include "config.h" // IWYU pragma: associated

#ifdef HAVE_APTPKG_CDROM

#include "rgpkgcdrom.h"

#include "i18n.h"
#include "rggtkbuilderwindow.h"
#include "rgmainwindow.h"
#include "rgutils.h"
#include "rgwindow.h"
#include "ruserdialog.h"

#include <cassert>
#include <apt-pkg/cdrom.h>
#include <gobject/gclosure.h>
#include <gtk/gtk.h>
#include <string>

using namespace std;

class RGDiscName final : public RGGtkBuilderWindow
{
 protected:

   GtkWidget *const _textEntry;
   bool _userConfirmed;

   static void onOkClicked(const GtkWidget *self, RGDiscName *diskName)
   {
      diskName->_userConfirmed = true;
      gtk_main_quit();
   }

   static void onCancelClicked(GtkWidget *self, RGDiscName *diskName)
   {
      gtk_main_quit();
   }

 public:

   RGDiscName(RGWindow *wwin, const string defaultName)
      : RGGtkBuilderWindow(wwin, "disc_name"),
        _textEntry(GTK_WIDGET(gtk_builder_get_object(_builder, "text_entry")))
   {
      assert(_textEntry != nullptr);

      setTitle(_("Disc Label"));

      gtk_entry_set_text(GTK_ENTRY(_textEntry), defaultName.c_str());

      g_signal_connect(gtk_builder_get_object(_builder, "ok"), "clicked", G_CALLBACK(onOkClicked), this);
      g_signal_connect(gtk_builder_get_object(_builder, "cancel"), "clicked", G_CALLBACK(onCancelClicked), this);
      gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window()), TRUE);
      gtk_window_set_transient_for(GTK_WINDOW(window()), GTK_WINDOW(wwin->window()));
      gtk_window_set_position(GTK_WINDOW(window()), GTK_WIN_POS_CENTER_ON_PARENT);
   }

   bool run(string &discName)
   {
      _userConfirmed = false;
      show();
      gtk_main();
      discName = gtk_entry_get_text(GTK_ENTRY(_textEntry));
      return _userConfirmed;
   }
};

void RGCDScanner::Update(string text, int current)
{
   if (!text.empty()) {
      gtk_label_set_text(GTK_LABEL(_label), text.c_str());
   }

   if (current > 0) {
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbar), ((double) current) / totalSteps);
   }

   show();
   RGFlushInterface();
}

bool RGCDScanner::ChangeCdrom()
{
   return _userDialog->proceed(_("Please insert a disc in the drive."));
}

bool RGCDScanner::AskCdromName(string &name)
{
   RGDiscName discName(this, name);
   return discName.run(name);
}

RGCDScanner::RGCDScanner(RGMainWindow *main, RUserDialog *userDialog)
   : pkgCdromStatus(), RGWindow(main, "cdscanner"),
     _label(gtk_label_new("\n\n")),
     _pbar(gtk_progress_bar_new()),
     _userDialog(userDialog)
{
   assert(_label != nullptr);
   assert(_pbar != nullptr);
   assert(_userDialog != nullptr);

   setTitle(_("Scanning CD-ROM"));

   gtk_window_set_default_size(GTK_WINDOW(_win), 300, 120);

   GtkWidget *topBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
   gtk_container_add(GTK_CONTAINER(_win), topBox);
   gtk_widget_set_margin_top(topBox, 12);
   gtk_widget_set_margin_bottom(topBox, 12);
   gtk_widget_set_margin_start(topBox, 12);
   gtk_widget_set_margin_end(topBox, 12);

   gtk_box_pack_start(GTK_BOX(topBox), _label, TRUE, TRUE, 10);

   gtk_widget_set_size_request(_pbar, -1, 25);
   gtk_box_pack_start(GTK_BOX(topBox), _pbar, FALSE, TRUE, 0);

   gtk_widget_show_all(topBox);

   gtk_window_set_transient_for(GTK_WINDOW(_win), GTK_WINDOW(main->window()));
   gtk_window_set_position(GTK_WINDOW(_win), GTK_WIN_POS_CENTER_ON_PARENT);
}

#endif // HAVE_APTPKG_CDROM
