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

#include <cassert>
#include "rgwindow.h"
#include "rgmainwindow.h"
#include "rgpkgdetails.h"
#include "rggladewindow.h"
#include "rpackage.h"
#include "sections_trans.h"

RGPkgDetailsWindow::RGPkgDetailsWindow(RGWindow *parent, RPackage *pkg)
   : RGGladeWindow(parent, "details")
{
   gchar *str = g_strdup_printf(_("%s Properties"),pkg->name());
   setTitle(str);
   g_free(str);

   glade_xml_signal_connect_data(_gladeXML,
				 "on_button_close_clicked",
				 G_CALLBACK(cbCloseClicked),
				 this); 
   // fill in all the pkg-values
   fillInValues(pkg);
   skipTaskbar(true);
   show();
}

void RGPkgDetailsWindow::cbCloseClicked(GtkWidget *self, void *data)
{
   RGPkgDetailsWindow *me = static_cast<RGPkgDetailsWindow*>(data);

   gtk_widget_destroy(me->_win); 

   // XXX destroy class itself
}

vector<string> 
RGPkgDetailsWindow::formatDepInformation(vector<RPackage::DepInformation> deps)
{
   vector<string> depStrings;
   string depStr;
   
   for(unsigned int i=0;i<deps.size();i++) {
      depStr="";

      // type in bold (Depends, PreDepends)
      depStr += string("<b>") + deps[i].typeStr + string(":</b> ");

      // virutal is italic
      if(deps[i].isVirtual) {
	 depStr += string("<i>") + deps[i].name + string("</i>");
      } else {
	 // is real pkg
	 depStr += deps[i].name;
	 // additional version information
	 if(deps[i].version != NULL) {
	    gchar *s = g_markup_escape_text(deps[i].versionComp,-1);
	    depStr += string(" (") + s + deps[i].version + string(")");
	    g_free(s);
	 }
      }

      // this is for or-ed dependencies (make them one-line)
      while(deps[i].isOr) {
	 depStr += " | ";
	 // next dep
	 i++;
	 depStr += deps[i].name;
	 // additional version information
	 if(deps[i].version != NULL) {
	    gchar *s = g_markup_escape_text(deps[i].versionComp,-1);
	    depStr += string(" (") + s + deps[i].version + string(")");
	    g_free(s);
	 }
      }
      
      depStrings.push_back(depStr);
   }
   return depStrings;
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
#ifdef HAVE_RPM
   setLabel("label_section", pkg->section());
#else
   setLabel("label_section", trans_section(pkg->section()).c_str());
#endif
   setLabel("label_installed_version", pkg->installedVersion());
   setLabel("label_installed_size", pkg->installedSize());

   setLabel("label_latest_version", pkg->availableVersion());
   setLabel("label_latest_size", pkg->availableInstalledSize());
   setLabel("label_latest_download_size", pkg->availablePackageSize());

   string descr = string(pkg->summary()) + "\n" + string(pkg->description());
   setTextView("text_descr", descr.c_str(), true);

   // build dependency lists
   vector<RPackage::DepInformation> deps;
   deps = pkg->enumDeps();
   setTreeList("treeview_deplist", formatDepInformation(deps), true);
   
   // canidateVersion
   deps = pkg->enumDeps(true);
   setTreeList("treeview_availdep_list", formatDepInformation(deps), true);

   // rdepends Version
   deps = pkg->enumRDeps();
   setTreeList("treeview_rdeps", formatDepInformation(deps), true);
   
   // provides
   setTreeList("treeview_provides", pkg->provides());


   // file list
#ifndef HAVE_RPM
   gtk_widget_show(glade_xml_get_widget(_gladeXML, "scrolledwindow_filelist"));
   setTextView("textview_files", pkg->installedFiles());
#endif

   glade_xml_signal_connect_data(_gladeXML, "on_optionmenu_depends_changed",
				 G_CALLBACK(cbDependsMenuChanged), this);
}

void RGPkgDetailsWindow::cbDependsMenuChanged(GtkWidget *self, void *data)
{
   RGPkgDetailsWindow *me = (RGPkgDetailsWindow*)data;

   int nr =  gtk_option_menu_get_history(GTK_OPTION_MENU(self));
   GtkWidget *notebook = glade_xml_get_widget(me->_gladeXML, 
					      "notebook_dep_tab");
   assert(notebook);
   gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), nr);
}

RGPkgDetailsWindow::~RGPkgDetailsWindow()
{

}
