/* rgiconlegend.cc
 *
 * Copyright (c) 2004 Michael Vogt <mvo@debian.org>
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

#include <cassert>
#include "config.h"
#include "rgiconlegend.h"
#include "rgutils.h"
#include "rgpackagestatus.h"

#include "i18n.h"

static void closeWindow(GtkWidget *self, void *data)
{
   RGIconLegendPanel *me = (RGIconLegendPanel *) data;

   me->hide();
}


RGIconLegendPanel::RGIconLegendPanel(RGWindow *parent)
: RGGtkBuilderWindow(parent, "iconlegend")
{
   setTitle(_("Icon Legend"));
   g_signal_connect(gtk_builder_get_object(_builder, "button_close"),
                      "clicked",
                      G_CALLBACK(closeWindow), this);
   GtkWidget *vbox = GTK_WIDGET(gtk_builder_get_object(_builder, "vbox_main"));
   assert(vbox);

   GtkWidget *hbox, *label, *pix;

   for (int i = 0; i < RGPackageStatus::N_STATUS_COUNT; i++) {
      hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

      pix = gtk_image_new_from_pixbuf(RGPackageStatus::pkgStatus.getPixbuf(i));
      gtk_box_pack_start(GTK_BOX(hbox), pix, FALSE, FALSE, 0);

      label = gtk_label_new(RGPackageStatus::pkgStatus.getLongStatusString(i));
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

      gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
   }


   // package support status 
   hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
   GtkIconTheme *theme;
   GdkPixbuf *pixbuf;
   GError *error = NULL;
   const gchar *name = "package-supported";
   theme = gtk_icon_theme_get_default();
   pixbuf = gtk_icon_theme_load_icon(theme, name, 16, 
				     (GtkIconLookupFlags)0, &error);

   pix = gtk_image_new_from_pixbuf(pixbuf);
   gtk_box_pack_start(GTK_BOX(hbox), pix, FALSE, FALSE, 0);
   label = gtk_label_new(_config->Find("Synaptic::supported-text",
				       _("Package is supported")).c_str());
   gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

   gtk_widget_show_all(vbox);
   //skipTaskbar(true);
   show();
}
