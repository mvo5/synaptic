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
#include "rgmisc.h"

#include "i18n.h"

static void closeWindow(GtkWidget *self, void *data)
{
   RGIconLegendPanel *me = (RGIconLegendPanel *) data;

   me->hide();
}


RGIconLegendPanel::RGIconLegendPanel(RGWindow *parent)
: RGGladeWindow(parent, "iconlegend")
{
   setTitle(_("Icon Legend"));
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_button_close_clicked",
                                 G_CALLBACK(closeWindow), this);
   GtkWidget *vbox = glade_xml_get_widget(_gladeXML, "vbox_main");
   assert(vbox);

   GtkWidget *hbox, *label, *pix;

   for (int i = 0; i < RGPackageStatus::N_STATUS_COUNT; i++) {
      hbox = gtk_hbox_new(FALSE, 12);

      pix = gtk_image_new_from_pixbuf(RGPackageStatus::pkgStatus.getPixbuf(i));
      gtk_box_pack_start(GTK_BOX(hbox), pix, FALSE, FALSE, 0);

      label = gtk_label_new(RGPackageStatus::pkgStatus.getLongStatusString(i));
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

      gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
   }

   gtk_widget_show_all(vbox);

   show();
}
