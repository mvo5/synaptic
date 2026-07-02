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

#include "config.h"

#include <cassert>
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
   GtkGrid *grid = GTK_GRID(gtk_builder_get_object(_builder, "grid_main"));
   assert(grid);

   GtkWidget *label, *pix;
   int i;

   for (i = 0; i < RGPackageStatus::N_STATUS_COUNT; i++) {
      pix = gtk_image_new_from_icon_name(RGPackageStatus::pkgStatus.getPixbuf(i).c_str());
      gtk_grid_attach(grid, pix, 0, i + 1, 1, 1);

      label = gtk_label_new(RGPackageStatus::pkgStatus.getLongStatusString(i));
      gtk_label_set_xalign(GTK_LABEL(label), 0.0);
      gtk_grid_attach(grid, label, 1, i + 1, 1, 1);
   }

   // package support status 
   pix = gtk_image_new_from_icon_name("package-supported");
   gtk_grid_attach(grid, pix, 0, i + 1, 1, 1);

   label = gtk_label_new(_config->Find("Synaptic::supported-text",
				       _("Package is supported")).c_str());
   gtk_label_set_xalign(GTK_LABEL(label), 0.0);
   gtk_grid_attach(grid, label, 1, i + 1, 1, 1);

   //skipTaskbar(true);
   show();
}
