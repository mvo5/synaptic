/* rgpkgdetails.cc - show details of a pkg
 * 
 * Copyright (c) 2004 Michael Vogt
 *
 * Author: Michael Vogt <mvo@debian.org>
 *         Gustavo Niemeyer <niemeyer@conectiva.com>
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

#include "rgwindow.h"
#include "rgmainwindow.h"
#include "rgpkgdetails.h"
#include "rggladewindow.h"
#include "rpackage.h"

RGPkgDetailsWindow::RGPkgDetailsWindow(RGWindow *parent, RPackage *pkg)
   : RGGladeWindow(parent, "details")
{

   glade_xml_signal_connect_data(_gladeXML,
				 "on_button_close_clicked",
				 G_CALLBACK(cbCloseClicked),
				 this); 

   fillInValues(pkg);

}

void RGPkgDetailsWindow::cbCloseClicked(GtkWidget *self, void *data)
{
   RGPkgDetailsWindow *me = static_cast<RGPkgDetailsWindow*>(data);

   gtk_widget_destroy(me->_win); 

   // XXX destroy class itself
}


void RGPkgDetailsWindow::fillInValues(RPackage *pkg)
{
   char *pkg_summary = g_strdup_printf("%s\n%s",
					     pkg->name(), pkg->summary());
   setTextView("textview_pkgcommon", pkg_summary, true);
   g_free(pkg_summary);

   setLabel("label_maintainer", pkg->maintainer());
   setPixmap("image_state", RGPackageStatus::pkgStatus.getPixbuf(pkg));
   setLabel("label_state", RGPackageStatus::pkgStatus.getLongStatusString(pkg));
   setLabel("label_priority", pkg->priority());
   setLabel("label_section", pkg->section());

   setLabel("label_installed_version", pkg->installedVersion());
   setLabel("label_installed_size", pkg->installedSize());

   setLabel("label_latest_version", pkg->availableVersion());
   setLabel("label_latest_size", pkg->availableInstalledSize());
   setLabel("label_latest_download_size", pkg->availablePackageSize());

   string descr = string(pkg->summary()) + "\n" + string(pkg->description());
   setTextView("text_descr", descr.c_str(), true);

   setTreeList("treeview_deplist", pkg->enumDeps());

}


RGPkgDetailsWindow::~RGPkgDetailsWindow()
{

}
