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

#include "config.h"

#include <cassert>
#include "rgwindow.h"
#include "rgmainwindow.h"
#include "rgpkgdetails.h"
#include "rggtkbuilderwindow.h"
#include "rpackage.h"
#include "rgpackagestatus.h"
#include "rgchangelogdialog.h"
#include "sections_trans.h"

RGPkgDetailsWindow::RGPkgDetailsWindow(RGWindow *parent)
   : RGGtkBuilderWindow(parent, "details")
{
   g_signal_connect(gtk_builder_get_object(_builder, "button_close"),
                    "clicked",
                    G_CALLBACK(cbCloseClicked), this); 
   
   GtkWidget *comboDepends = GTK_WIDGET(gtk_builder_get_object
                                        (_builder, "combobox_depends"));
   g_signal_connect(G_OBJECT(comboDepends),
                    "changed",
                    G_CALLBACK(cbDependsMenuChanged), this);
   GtkListStore *relTypes = gtk_list_store_new(1, G_TYPE_STRING);
   GtkTreeIter relIter;

   // HACK: the labels for the combo box items are defined as
   //       the relOptions array in gtk/rgmainwindow.h.
   //       We already include it, so we get those for free.
   for (int i = 0; relOptions[i] != NULL; i++) {
      gtk_list_store_append(relTypes, &relIter);
      gtk_list_store_set(relTypes, &relIter, 0, _(relOptions[i]), -1);
   }
   gtk_combo_box_set_model(GTK_COMBO_BOX(comboDepends),
                           GTK_TREE_MODEL(relTypes));
   GtkCellRenderer *relRenderText = gtk_cell_renderer_text_new();
   gtk_cell_layout_clear(GTK_CELL_LAYOUT(comboDepends));
   gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(comboDepends),
                              relRenderText, FALSE);
   gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(comboDepends),
                                 relRenderText, "text", 0);
   gtk_combo_box_set_active(GTK_COMBO_BOX(comboDepends), 0);

   GtkWidget *label;
   label = GTK_WIDGET(gtk_builder_get_object(_builder, "label_maintainer"));
   g_signal_connect(G_OBJECT(label), "activate-link", 
                    G_CALLBACK(cbOpenLink), NULL);
}

void RGPkgDetailsWindow::cbCloseClicked(GtkWidget *self, void *data)
{
   RGPkgDetailsWindow *me = static_cast<RGPkgDetailsWindow*>(data);

   me->hide();
}

vector<string> 
RGPkgDetailsWindow::formatDepInformation(vector<DepInformation> deps)
{
   vector<string> depStrings;
   string depStr;
   
   for(unsigned int i=0;i<deps.size();i++) {
      depStr="";

      // type in bold (Depends, PreDepends)
      depStr += string("<b>") + _(DepTypeStr[deps[i].type]) + string(":</b> ");

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

void RGPkgDetailsWindow::cbShowBigScreenshot(GtkWidget *box, 
                                             GdkEventButton *event, 
                                             void *data)
{
   //cerr << "cbShowBigScreenshot" << endl;
   RPackage *pkg = (RPackage *)data;
   
   doShowBigScreenshot(pkg);
}

void RGPkgDetailsWindow::doShowBigScreenshot(RPackage *pkg)
{
   RGFetchProgress *status = new RGFetchProgress(NULL);;
   pkgAcquire fetcher(status);
   string filename = pkg->getScreenshotFile(&fetcher, false);
   GtkWidget *img = gtk_image_new_from_file(filename.c_str());
   GtkWidget *win = gtk_dialog_new();
   gtk_window_set_default_size(GTK_WINDOW(win), 500, 400);
   gtk_dialog_add_button(GTK_DIALOG(win), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
   gtk_widget_show(img);
   GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (win));
   gtk_container_add(GTK_CONTAINER(content_area), img);
   gtk_dialog_run(GTK_DIALOG(win));
   gtk_widget_destroy(win);
}

void RGPkgDetailsWindow::cbShowScreenshot(GtkWidget *button, void *data)
{
   struct screenshot_info *si = (struct screenshot_info*)data;

   if(_config->FindB("Synaptic::InlineScreenshots") == false)
   {
      doShowBigScreenshot(si->pkg);
      return;
   } else {

      // hide button
      gtk_widget_hide(button);
      
      // get screenshot
      RGFetchProgress *status = new RGFetchProgress(NULL);;
      pkgAcquire fetcher(status);
      string filename = si->pkg->getScreenshotFile(&fetcher);
      GtkWidget *event = gtk_event_box_new();
      GtkWidget *img = gtk_image_new_from_file(filename.c_str());
      gtk_container_add(GTK_CONTAINER(event), img);
      g_signal_connect(G_OBJECT(event), "button_press_event", 
                       G_CALLBACK(cbShowBigScreenshot), 
                       (void*)si->pkg);
      gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(si->textview), 
                                        GTK_WIDGET(event), si->anchor);
      gtk_widget_show_all(event);
   }
}

void RGPkgDetailsWindow::cbShowChangelog(GtkWidget *button, void *data)
{
   RPackage *pkg = (RPackage*)data;
   RGWindow *parent = (RGWindow*)g_object_get_data(G_OBJECT(button), "me");
   ShowChangelogDialog(parent, pkg);
}

gboolean RGPkgDetailsWindow::cbOpenLink(GtkWidget *label, 
                                        gchar *uri, 
                                        void *data)
{
   //std::cerr << "cbOpenLink: " << uri << std::endl;
   std::vector<const gchar *> cmd;
   cmd.push_back("xdg-open");
   cmd.push_back(uri);
   RunAsSudoUserCommand(cmd);

   return TRUE;
}

gboolean RGPkgDetailsWindow::cbOpenHomepage(GtkWidget *button, void* data)
{
   RPackage *pkg = (RPackage*)data;
   std::vector<const gchar*> cmd = GetBrowserCommand(pkg->homepage());
   //std::cerr << "cbOpenHomepage: " << cmd[0] << std::endl;
   RunAsSudoUserCommand(cmd);

   return TRUE;
}

void RGPkgDetailsWindow::fillInValues(RGGtkBuilderWindow *me, 
                                      RPackage *pkg,
				      bool setTitle)
{
   assert(me!=NULL);

   // TRANSLATORS: Title of the package properties dialog 
   //              %s is the name of the package
   if(setTitle) {
      gchar *str = g_strdup_printf(_("%s Properties"),pkg->name());
      me->setTitle(str);
      g_free(str);
   }

   char *pkg_summary = g_strdup_printf("%s\n%s",
					     pkg->name(), pkg->summary());
   me->setTextView("textview_pkgcommon", pkg_summary, true);
   g_free(pkg_summary);

   string maintainer = pkg->maintainer();
   int start_index = maintainer.find("<");
   int end_index = maintainer.rfind(">");
   if (start_index != -1 && end_index != -1) {
       gchar*  maintainer_label = g_markup_printf_escaped(
          "<a href=\"mailto:%s\">%s</a>", 
          maintainer.substr(start_index+1, end_index - start_index - 1).c_str(),
          maintainer.c_str() 
          );
       me->setMarkup("label_maintainer", maintainer_label);
       g_free(maintainer_label);
   } else {
       me->setLabel("label_maintainer", pkg->maintainer());
   }

   me->setPixmap("image_state", RGPackageStatus::pkgStatus.getPixbuf(pkg));
   me->setLabel("label_state", RGPackageStatus::pkgStatus.getLongStatusString(pkg));
   me->setLabel("label_priority", pkg->priority());
   me->setLabel("label_section", trans_section(pkg->section()).c_str());
   me->setLabel("label_installed_version", pkg->installedVersion());
   me->setLabel("label_installed_size", pkg->installedSize());

   me->setLabel("label_latest_version", pkg->availableVersion());
   me->setLabel("label_latest_size", pkg->availableInstalledSize());
   me->setLabel("label_latest_download_size", pkg->availablePackageSize());
   me->setLabel("label_source", pkg->srcPackage());

   // format description nicely and use emblems
   GtkWidget *textview;
   GtkTextBuffer *buf;
   GtkTextIter it, start, end;
   GtkWidget *emblem;
   const gchar *s;

   textview = GTK_WIDGET(gtk_builder_get_object
                         (me->getGtkBuilder(), "text_descr"));
   assert(textview);
   buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
   // clear old buffer
   gtk_text_buffer_get_start_iter(buf, &start);
   gtk_text_buffer_get_end_iter(buf, &end);
   gtk_text_buffer_delete(buf, &start, &end);
   // create bold tag
   GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buf);
   if(gtk_text_tag_table_lookup(tag_table, "bold") == NULL) {
      gtk_text_buffer_create_tag(buf, "bold", 
				 "weight", PANGO_WEIGHT_BOLD,
				 "scale", 1.1, 
				 NULL);
   }
   // set summary
   s = utf8(pkg->summary());
   gtk_text_buffer_get_start_iter(buf, &it);
   gtk_text_buffer_insert(buf, &it, s, -1);
   gtk_text_buffer_get_start_iter(buf, &start);
   gtk_text_buffer_apply_tag_by_name(buf, "bold", &start, &it);
   // set emblems 
   GdkPixbuf *pixbuf = RGPackageStatus::pkgStatus.getSupportedPix(pkg);
   if(pixbuf != NULL) {
      // insert space
      gtk_text_buffer_insert(buf, &it, " ", 1);
      // make image
      emblem = gtk_image_new_from_pixbuf(pixbuf);
      gtk_image_set_pixel_size(GTK_IMAGE(emblem), 16);
      // set eventbox and tooltip
      GtkWidget *event = gtk_event_box_new();
      GtkStyle *style = gtk_widget_get_style(textview);
      gtk_widget_modify_bg(event, GTK_STATE_NORMAL, 
			   &style->base[GTK_STATE_NORMAL]);
      gtk_container_add(GTK_CONTAINER(event), emblem);
      gtk_widget_set_tooltip_text(event, _("This application is supported by the distribution"));
      // create anchor
      GtkTextChildAnchor *anchor = gtk_text_buffer_create_child_anchor(buf, &it);
      gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(textview), event, anchor);
      gtk_widget_show_all(event);
   }

   // add button to get screenshot
   gtk_text_buffer_insert(buf, &it, "\n", 1);
   GtkTextChildAnchor *anchor = gtk_text_buffer_create_child_anchor(buf, &it);
   GtkWidget *button = gtk_button_new_with_label(_("Get Screenshot"));
   static struct screenshot_info si;
   si.anchor = anchor;
   si.textview = textview;
   si.pkg = pkg;
   g_signal_connect(G_OBJECT(button),"clicked", 
                    G_CALLBACK(cbShowScreenshot), 
                    &si);
   gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(textview), button, anchor);
   gtk_widget_show(button);

   // add button to get changelog
   gtk_text_buffer_insert(buf, &it, "    ", 1);
   anchor = gtk_text_buffer_create_child_anchor(buf, &it);
   button = gtk_button_new_with_label(_("Get Changelog"));
   g_object_set_data(G_OBJECT(button), "me", me);
   g_signal_connect(G_OBJECT(button),"clicked", 
                    G_CALLBACK(cbShowChangelog),
                    pkg);
   gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(textview), button, anchor);
   gtk_widget_show(button);

   // add button to open the homepage
   if (strlen(pkg->homepage())) {
       gtk_text_buffer_insert(buf, &it, "    ", 1);
       anchor = gtk_text_buffer_create_child_anchor(buf, &it);
       button = gtk_link_button_new_with_label(pkg->homepage(), _("Visit Homepage"));
       char *homepage_tooltip = g_strdup_printf("Visit %s",
					     pkg->homepage());
       g_signal_connect(G_OBJECT(button),"activate-link", 
                    G_CALLBACK(cbOpenHomepage),
                    pkg);
       gtk_widget_set_tooltip_text(button, homepage_tooltip);
       g_free(homepage_tooltip);
       g_object_set_data(G_OBJECT(button), "me", me);
       gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(textview), button, anchor);
       gtk_widget_show(button);
   }

   // show the rest of the description
   gtk_text_buffer_insert(buf, &it, "\n", 1);
   s = utf8(pkg->description());
   gtk_text_buffer_insert(buf, &it, s, -1);
   

   // build dependency lists
   vector<DepInformation> deps;
   deps = pkg->enumDeps();
   me->setTreeList("treeview_deplist", formatDepInformation(deps), true);
   
   // canidateVersion
   deps = pkg->enumDeps(true);
   me->setTreeList("treeview_availdep_list", formatDepInformation(deps), true);

   // rdepends Version
   deps = pkg->enumRDeps();
   me->setTreeList("treeview_rdeps", formatDepInformation(deps), true);
   
   // provides
   me->setTreeList("treeview_provides", pkg->provides());


   // file list
#ifndef HAVE_RPM
   gtk_widget_show(GTK_WIDGET(gtk_builder_get_object
                              (me->getGtkBuilder(),
                               "scrolledwindow_filelist")));
   me->setTextView("textview_files", pkg->installedFiles());
#endif

   // versions
   gchar *str;
   vector<string> list;
   vector<pair<string,string> > versions = pkg->getAvailableVersions();
   for(int i=0;i<versions.size();i++) {
      // TRANSLATORS: this the format of the available versions in 
      // the "Properties/Available versions" window
      // e.g. "0.56 (unstable)"
      //      "0.53.4 (testing)"
      str = g_strdup_printf(_("%s (%s)"), 
			    versions[i].first.c_str(), 
			    versions[i].second.c_str());
      list.push_back(str);
      g_free(str);
   }
   me->setTreeList("treeview_versions", list);

   /*
   g_signal_connect(gtk_builder_get_object
                    (me->getGtkBuilder(), "combobox_depends"),
                    "changed",
                    G_CALLBACK(cbDependsMenuChanged), me);
   */
}

void RGPkgDetailsWindow::cbDependsMenuChanged(GtkWidget *self, void *data)
{
   RGPkgDetailsWindow *me = (RGPkgDetailsWindow*)data;

   int nr =  gtk_combo_box_get_active(GTK_COMBO_BOX(self));
   GtkWidget *notebook = GTK_WIDGET(gtk_builder_get_object
                                    (me->_builder, "notebook_dep_tab"));
   assert(notebook);
   gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), nr);
}

RGPkgDetailsWindow::~RGPkgDetailsWindow()
{
   
}
