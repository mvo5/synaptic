/* rgmainwindow.cc - main window of the app
 * 
 * Copyright (c) 2001-2003 Conectiva S/A
 *               2002,2003 Michael Vogt <mvo@debian.org>
 * 
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <mvo@debian.org>
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
#include <stdio.h>
#include <ctype.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cmath>
#include <algorithm>
#include <fstream>

#include <apt-pkg/strutl.h>
#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <pwd.h>

#include "raptoptions.h"
#include "rconfiguration.h"
#include "rgmainwindow.h"
#include "rgfindwindow.h"
#include "rgfiltermanager.h"
#include "rpackagefilter.h"
#include "raptoptions.h"

#include "rgrepositorywin.h"
#include "rgpreferenceswindow.h"
#include "rgaboutpanel.h"
#include "rgsummarywindow.h"
#include "rgchangeswindow.h"
#include "rgcdscanner.h"
#include "rgsetoptwindow.h"

#include "rgfetchprogress.h"
#include "rgpkgdetails.h"
#include "rgcacheprogress.h"
#include "rguserdialog.h"
#include "rginstallprogress.h"
#include "rgdummyinstallprogress.h"
#include "rgterminstallprogress.h"
#include "rgmisc.h"
#include "sections_trans.h"

// icons and pixmaps
#include "synaptic_mini.xpm"

#include "i18n.h"

static char *ImportanceNames[] = {
   _("Unknown"),
   _("Normal"),
   _("Critical"),
   _("Security")
};


enum { WHAT_IT_DEPENDS_ON,
   WHAT_DEPENDS_ON_IT,
   WHAT_IT_WOULD_DEPEND_ON,
   WHAT_IT_PROVIDES,
   WHAT_IT_SUGGESTS
};

enum { DEP_NAME_COLUMN,         /* text */
   DEP_IS_NOT_AVAILABLE,        /* foreground-set */
   DEP_IS_NOT_AVAILABLE_COLOR,  /* foreground */
   DEP_PKG_INFO
};                              /* additional info (install 
                                   not installed) as text */

#define SELECTED_MENU_INDEX(popup) \
    (int)gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(gtk_option_menu_get_menu(GTK_OPTION_MENU(popup))))), "index")



#if ! GTK_CHECK_VERSION(2,2,0)
// This function is needed to be compatible with gtk 2.0
// data takes a GList** and fills the list with GtkTreePathes 
// (just like the return of gtk_tree_selection_get_selected_rows()).
void cbGetSelectedRows(GtkTreeModel *model,
                       GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
   GList **list;
   list = (GList **) data;
   *list = g_list_append(*list, gtk_tree_path_copy(path));
}
#endif



void RGMainWindow::changeView(int view, bool sethistory, string subView)
{
   //cout << "RGMainWindow::changeView()" << endl;

   if (sethistory)
      gtk_option_menu_set_history(GTK_OPTION_MENU(_viewPopup), view);

   RPackage *pkg = selectedPackage();
   setInterfaceLocked(TRUE);
   _blockActions = TRUE;

   _lister->setView(view);

   refreshSubViewList();

   if(!subView.empty()) {
      GtkTreeSelection* selection;
      GtkTreeModel *model;
      GtkTreeIter iter;
      char *str;

      GtkWidget *view = glade_xml_get_widget(_gladeXML, "treeview_subviews");
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
      if(gtk_tree_model_get_iter_first(model, &iter)) {
	 do {
	    gtk_tree_model_get(model, &iter, 0, &str, -1);
	    if(strcmp(str,subView.c_str()) == 0) {
	       gtk_tree_selection_select_iter(selection, &iter);
	       break;
	    }
	 } while(gtk_tree_model_iter_next(model, &iter));
      }
      
   }
   _lister->reapplyFilter();

   refreshTable(pkg);
   _blockActions = FALSE;
   setInterfaceLocked(FALSE);
   setStatusText();
}

void RGMainWindow::refreshSubViewList(string selectedSubView)
{
   //cout << "RGMainWindow::refreshSubViewList(): "<< selectedSubView << endl;

   vector<string> subViews = _lister->getSubViews();

   gchar *str = g_strdup_printf("<b>%s</b>", _("All"));
   subViews.insert(subViews.begin(), str);
   g_free(str);
   setTreeList("treeview_subviews", subViews, true);

   if(!selectedSubView.empty()) {
      GtkTreeSelection *selection;
      GtkTreeModel *model;
      GtkTreeIter iter;
      const char *str;

      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(_subViewList));
      bool ok =  gtk_tree_model_get_iter_first(model, &iter); 
      while(ok) {
	 gtk_tree_model_get(model, &iter, 0, &str, -1);
	 if(strcoll(str, selectedSubView.c_str()) == 0) {
	    gtk_tree_selection_select_iter(selection, &iter);
	    return;
	 }
	 ok = gtk_tree_model_iter_next(model, &iter);
      }
   } else {
#if GTK_CHECK_VERSION(2,4,0)
      // we auto set to "All" when we have gtk2.4 (without the list is 
      // too slow)
      GtkTreeModel *model;
      GtkTreeSelection *selection;
      GtkTreeIter iter;

      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(_subViewList));
      gtk_tree_model_get_iter_first(model, &iter);
      gtk_tree_selection_select_iter(selection, &iter);
#endif
   }
}


RPackage *RGMainWindow::selectedPackage()
{
   if (_pkgList == NULL)
      return NULL;

   GtkTreeSelection *selection;
   GtkTreeIter iter;
   RPackage *pkg = NULL;
   GList *li = NULL;
   GList *list = NULL;

   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
#if GTK_CHECK_VERSION(2,2,0)
   list = gtk_tree_selection_get_selected_rows(selection, &_pkgList);
#else
   gtk_tree_selection_selected_foreach(selection, cbGetSelectedRows, &list);
#endif

   if (list == NULL) // Empty.
      return NULL;

   // We are only interessted in the last element
   li = g_list_last(list);
   gtk_tree_model_get_iter(_pkgList, &iter, (GtkTreePath *) (li->data));

   gtk_tree_model_get(_pkgList, &iter, PKG_COLUMN, &pkg, -1);


   // free the list
   g_list_foreach(list, (void (*)(void *, void *))gtk_tree_path_free, NULL);
   g_list_free(list);


   return pkg;
}

string RGMainWindow::selectedSubView()
{
   GtkTreeSelection *selection;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gchar *subView = NULL;
   string ret = "(no selection)";

   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
   if (selection != NULL) {
      if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
         gtk_tree_model_get(model, &iter, 0, &subView, -1);
	 
	 // check if first item is selected ("All")
	 gchar *str=gtk_tree_model_get_string_from_iter(model, &iter);
	 if(str[0] == '0')
	    ret = "";
	 else
	    ret = utf8_to_locale(subView);
	 g_free(str);
         g_free(subView);
      }
   }

   return ret;
}


bool RGMainWindow::showErrors()
{
   return _userDialog->showErrors();
}

void RGMainWindow::notifyChange(RPackage *pkg)
{
   if (pkg != NULL)
      refreshTable(pkg);

   setStatusText();
}

void RGMainWindow::forgetNewPackages()
{
   int row = 0;
   while (row < _lister->viewPackagesSize()) {
      RPackage *elem = _lister->getViewPackage(row);
      if (elem->getFlags() && RPackage::FNew)
         elem->setNew(false);
   }
   _roptions->forgetNewPackages();
}


void RGMainWindow::refreshTable(RPackage *selectedPkg)
{
   //cout << "RGMainWindow::refreshTable(): " << selectedPkg << endl;

   _lister->setSubView(selectedSubView());

   _pkgList = GTK_TREE_MODEL(gtk_pkg_list_new(_lister));
   gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView),
                           GTK_TREE_MODEL(_pkgList));

   gtk_adjustment_value_changed(
         gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(_treeView)));
   gtk_adjustment_value_changed(
         gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(_treeView)));

   // set selected pkg to be selected again
   if(selectedPkg != NULL) {
      GtkTreeIter iter;
      RPackage *pkg;
      
      // make sure we have the keyboard focus after the refresh
      gtk_widget_grab_focus(_treeView);

      // find and select the pkg we are looking for
      bool ok =  gtk_tree_model_get_iter_first(_pkgList, &iter); 
      while(ok) {
	 gtk_tree_model_get(_pkgList, &iter, PKG_COLUMN, &pkg, -1);
	 if(pkg == selectedPkg) {
	    GtkTreePath* path = gtk_tree_model_get_path(_pkgList, &iter);
	    gtk_tree_view_set_cursor(GTK_TREE_VIEW(_treeView), path, 
				     NULL, false);
	    gtk_tree_path_free(path);
	    break;
	 }

	 ok = gtk_tree_model_iter_next(_pkgList, &iter);
      }
   }

   setStatusText();
}

void RGMainWindow::updatePackageInfo(RPackage *pkg)
{
   if (_blockActions)
      return;

   //cout << "RGMainWindow::updatePackageInfo(): " << pkg << endl;

   if (!pkg) {
      gtk_widget_set_sensitive(_pkginfo, false);
      gtk_text_buffer_set_text(_pkgCommonTextBuffer,
                               _("No package is selected.\n"), -1);
      
      gtk_widget_set_sensitive(_pkgReconfigureM, FALSE);
      gtk_widget_set_sensitive(_pkgHelpM, FALSE);
   } else {
      gtk_widget_set_sensitive(_pkginfo, true);
      string descr = string(pkg->summary()) + "\n" + string(pkg->description());
      setTextView("textview_pkgcommon", descr.c_str(), true);

#ifndef HAVE_RPM
      int flags = pkg->getFlags();
      if( flags & RPackage::FInstalled) {
 	 gtk_widget_set_sensitive(_pkgHelpM, TRUE);
	 if(pkg->dependsOn("debconf") || pkg->dependsOn("debconf-i18n"))
	    gtk_widget_set_sensitive(_pkgReconfigureM, TRUE);
	 else
	    gtk_widget_set_sensitive(_pkgReconfigureM, FALSE);
      }
#endif      
   }
   setStatusText();
}


// install a specific version
void RGMainWindow::installFromVersion(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) g_object_get_data(G_OBJECT(self), "me");
   RPackage *pkg = (RPackage *) g_object_get_data(G_OBJECT(self), "pkg");
   gchar *verInfo = (gchar *) data;

   // check if it's a interessting event
   if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self))) {
      return;
   }
   // set pkg to "not installed" 
   if (verInfo == NULL) {
      pkg->unsetVersion();
      me->pkgAction(PKG_DELETE);
      return;
   }

   string instVer;
   if (pkg->installedVersion() == NULL)
      instVer = "";
   else
      instVer = pkg->installedVersion();

   pkg->setNotify(false);
   if (instVer == string(verInfo)) {
      pkg->unsetVersion();
      me->pkgAction(PKG_KEEP);
   } else {
      pkg->setVersion(verInfo);
      me->pkgAction(PKG_INSTALL);

      if (!(pkg->getFlags() & RPackage::FInstall))
         pkg->unsetVersion();
   }
   pkg->setNotify(true);
}

bool RGMainWindow::askStateChange(RPackageLister::pkgState state, 
				  vector<RPackage *> exclude)
{
   vector<RPackage *> toKeep;
   vector<RPackage *> toInstall;
   vector<RPackage *> toReInstall;
   vector<RPackage *> toUpgrade;
   vector<RPackage *> toDowngrade;
   vector<RPackage *> toRemove;

   bool ask = _config->FindB("Synaptic::AskRelated", true);

   // ask if the user really want this changes
   bool changed = true;
   if (ask && _lister->getStateChanges(state, toKeep, toInstall, toReInstall,
                                           toUpgrade, toRemove, toDowngrade,
                                           exclude)) {
      RGChangesWindow *chng;
      // show a summary of what's gonna happen
      chng = new RGChangesWindow(this);
      if (!chng->showAndConfirm(_lister, toKeep, toInstall, toReInstall,
                                toUpgrade, toRemove, toDowngrade)) {
         // canceled operation
         _lister->restoreState(state);
	 // if a operation was canceled, we discard all errors from this
	 // operation too
	 _error->Discard();
         changed = false;
      }
      delete chng;
   }

   return changed;
}

void RGMainWindow::pkgAction(RGPkgAction action)
{
   GtkTreeSelection *selection;
   GtkTreeIter iter;
   GList *li, *list;

   setInterfaceLocked(TRUE);
   _blockActions = TRUE;

   // get list of selected pkgs
   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
#if GTK_CHECK_VERSION(2,2,0)
   list = li = gtk_tree_selection_get_selected_rows(selection, &_pkgList);
#else
   li = list = NULL;
   gtk_tree_selection_selected_foreach(selection, cbGetSelectedRows, &list);
   li = list;
#endif

   // save pkg state
   RPackageLister::pkgState state;
   bool ask = _config->FindB("Synaptic::AskRelated", true);

   // we always save the state (for undo)
   _lister->saveState(state);
   if (ask)
      _lister->unregisterObserver(this);
   _lister->notifyCachePreChange();

   // We block notifications in setInstall() and friends, since it'd
   // kill the algorithm in a long loop with many selected packages.
   _lister->notifyPreChange(NULL);

   // do the work
   vector<RPackage *> exclude;
   vector<RPackage *> instPkgs;
   RPackage *pkg = NULL;

   while (li != NULL) {
      gtk_tree_model_get_iter(_pkgList, &iter, (GtkTreePath *) (li->data));
      gtk_tree_model_get(_pkgList, &iter, PKG_COLUMN, &pkg, -1);
      li = g_list_next(li);
      if (pkg == NULL)
         continue;

      pkg->setNotify(false);

      // needed for the stateChange 
      exclude.push_back(pkg);
      /* do the dirty deed */
      switch (action) {
         case PKG_KEEP:        // keep
            pkgKeepHelper(pkg);
            break;
         case PKG_INSTALL:     // install
            instPkgs.push_back(pkg);
            pkgInstallHelper(pkg, false);
            break;
         case PKG_REINSTALL:      // reinstall
	    instPkgs.push_back(pkg);
	    pkgInstallHelper(pkg, false, true);
	    break;
         case PKG_DELETE:      // delete
            pkgRemoveHelper(pkg);
            break;
         case PKG_PURGE:       // purge
            pkgRemoveHelper(pkg, true);
            break;
         case PKG_DELETE_WITH_DEPS:
            pkgRemoveHelper(pkg, true, true);
            break;
         default:
            cout << "uh oh!!!!!!!!!" << endl;
            break;
      }

      pkg->setNotify(true);
   }

   // Do it just once, otherwise it'd kill a long installation list.
   if (!_lister->check())
      _lister->fixBroken();

   _lister->notifyPostChange(NULL);

   _lister->notifyCachePostChange();

   bool changed = askStateChange(state, exclude);

   if (changed) {
      _lister->saveUndoState(state);
      // check for failed installs
      if (action == PKG_INSTALL) 
	 checkForFailedInst(instPkgs);
   }

   if (ask)
      _lister->registerObserver(this);

   g_list_foreach(list, (void (*)(void *, void *))gtk_tree_path_free, NULL);
   g_list_free(list);

   _blockActions = FALSE;
   setInterfaceLocked(FALSE);
   refreshTable(pkg);
}

bool RGMainWindow::checkForFailedInst(vector<RPackage *> instPkgs)
{
   string failedReason;
   bool failed = false;
   for (unsigned int i = 0; i < instPkgs.size(); i++) {
      RPackage *pkg = instPkgs[i];
      if (pkg == NULL)
	 continue;
      if (!(pkg->getFlags() & RPackage::FInstall)) {
	 failed = true;
	 failedReason += string(pkg->name()) + ":\n";
	 failedReason += pkg->showWhyInstBroken();
	 failedReason += "\n";
	 pkg->setKeep();
	 pkg->unsetVersion();
	 _lister->notifyChange(pkg);
      }
   }
   if (failed) {
      RGGladeUserDialog dia(this,"unmet");
      GtkWidget *tv = glade_xml_get_widget(dia.getGladeXML(),
					   "textview");
      GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
      gtk_text_buffer_set_text(tb, utf8(failedReason.c_str()), -1);
      dia.run();
      // we informaed the user about the problem, we can clear the
      // apt error stack
      // CHECKME: is this discard here really needed?
      _error->Discard();
   }
      
   return failed;
}

RGMainWindow::RGMainWindow(RPackageLister *packLister, string name)
   : RGGladeWindow(NULL, name), _lister(packLister), _pkgList(0), 
     _treeView(0), _iconLegendPanel(0)
{
   assert(_win);

   _blockActions = false;
   _unsavedChanges = false;
   _interfaceLocked = 0;

   _lister->registerObserver(this);
   _busyCursor = gdk_cursor_new(GDK_WATCH);
   _tooltips = gtk_tooltips_new();

   _toolbarStyle = (GtkToolbarStyle) _config->FindI("Synaptic::ToolbarState",
                                                    (int)GTK_TOOLBAR_BOTH);


   buildInterface();
   _userDialog = new RGUserDialog(this);

   packLister->setUserDialog(_userDialog);

   packLister->setProgressMeter(_cacheProgress);
   _findWin = NULL;
   _setOptWin = NULL;
   _sourcesWin = NULL;
   _configWin = NULL;
   _aboutPanel = NULL;
   _fmanagerWin = NULL;

   GValue value = { 0, };
   g_value_init(&value, G_TYPE_STRING);
   g_object_get_property(G_OBJECT(gtk_settings_get_default()),
                         "gtk-font-name", &value);
   _config->Set("Volatile::orginalFontName", g_value_get_string(&value));
   if (_config->FindB("Synaptic::useUserFont")) {
      g_value_set_string(&value, _config->Find("Synaptic::FontName").c_str());
      g_object_set_property(G_OBJECT(gtk_settings_get_default()),
                            "gtk-font-name", &value);
   }
   g_value_unset(&value);

   // apply the proxy settings
   RGPreferencesWindow::applyProxySettings();
}



// needed for the buildTreeView function
struct mysort {
   bool operator() (const pair<int, GtkTreeViewColumn *> &x,
                    const pair<int, GtkTreeViewColumn *> &y) {
      return x.first < y.first;
}};


void RGMainWindow::buildTreeView()
{
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column, *name_column = NULL;
   GtkTreeSelection *selection;
   vector<pair<int, GtkTreeViewColumn *> > all_columns;
   int pos = 0;

   // remove old tree columns
   if (_treeView) {
      GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(_treeView));
      for (GList * li = g_list_first(columns); li != NULL;
           li = g_list_next(li)) {
         gtk_tree_view_remove_column(GTK_TREE_VIEW(_treeView),
                                     GTK_TREE_VIEW_COLUMN(li->data));
      }
      // need to free the list here
      g_list_free(columns);
   }

   _treeView = glade_xml_get_widget(_gladeXML, "treeview_packages");
   assert(_treeView);
   gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(_treeView),TRUE);

   gtk_tree_view_set_search_column(GTK_TREE_VIEW(_treeView), NAME_COLUMN);
   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
   gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

   /* Status(pixmap) column */
   pos = _config->FindI("Synaptic::statusColumnPos", 0);
   if (pos != -1) {
      renderer = gtk_cell_renderer_pixbuf_new();
      column = gtk_tree_view_column_new_with_attributes("S", renderer,
                                                        "pixbuf",
                                                        PIXMAP_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 20);
      //gtk_tree_view_insert_column(GTK_TREE_VIEW(_treeView), column, pos);
      all_columns.push_back(pair<int, GtkTreeViewColumn *>(pos, column));
   }


   /* Package name */
   pos = _config->FindI("Synaptic::nameColumnPos", 1);
   if (pos != -1) {
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      name_column = column =
         gtk_tree_view_column_new_with_attributes(_("Package"), renderer,
                                                  "markup", NAME_COLUMN,
                                                  //"text", NAME_COLUMN,
                                                  "background-gdk",
                                                  COLOR_COLUMN, NULL);
#if GTK_CHECK_VERSION(2,3,2)
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 200);
#else
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
#endif

      //gtk_tree_view_insert_column(GTK_TREE_VIEW(_treeView), column, pos);
      all_columns.push_back(pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_resizable(column, TRUE);
      gtk_tree_view_column_set_sort_column_id(column, NAME_COLUMN);
   }

   /* Installed Version */
   pos = _config->FindI("Synaptic::instVerColumnPos", 2);
   if (pos != -1) {
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      column =
         gtk_tree_view_column_new_with_attributes(_("Installed Version"),
                                                  renderer, "text",
                                                  INSTALLED_VERSION_COLUMN,
                                                  "background-gdk",
                                                  COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 130);
      //gtk_tree_view_insert_column (GTK_TREE_VIEW(_treeView), column, pos);
      all_columns.push_back(pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_resizable(column, TRUE);
   }

   /* Available Version */
   pos = _config->FindI("Synaptic::availVerColumnPos", 3);
   if (pos != -1) {
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      column =
         gtk_tree_view_column_new_with_attributes(_("Latest Version"),
                                                  renderer, "text",
                                                  AVAILABLE_VERSION_COLUMN,
                                                  "background-gdk",
                                                  COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 130);
      //gtk_tree_view_insert_column (GTK_TREE_VIEW (_treeView), column, pos);
      all_columns.push_back(pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_resizable(column, TRUE);
   }
   // installed size
   pos = _config->FindI("Synaptic::instSizeColumnPos", 4);
   if (pos != -1) {
      /* Installed size */
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      GValue value = { 0, };
      GValue value2 = { 0, };
      g_value_init(&value, G_TYPE_FLOAT);
      g_value_set_float(&value, 1.0);
      g_object_set_property(G_OBJECT(renderer), "xalign", &value);
      g_value_init(&value2, G_TYPE_INT);
      g_value_set_int(&value2, 10);
      g_object_set_property(G_OBJECT(renderer), "xpad", &value2);
      column = gtk_tree_view_column_new_with_attributes(_("Size"), renderer,
                                                        "text",
                                                        PKG_SIZE_COLUMN,
                                                        "background-gdk",
                                                        COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 80);
      //gtk_tree_view_insert_column (GTK_TREE_VIEW(_treeView), column, pos);
      all_columns.push_back(pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_resizable(column, TRUE);
      gtk_tree_view_column_set_sort_column_id(column, PKG_SIZE_COLUMN);
   }

   /* Description */
   pos = _config->FindI("Synaptic::descrColumnPos", 5);
   if (pos != -1) {
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      column =
         gtk_tree_view_column_new_with_attributes(_("Description"), renderer,
                                                  "text", DESCR_COLUMN,
                                                  "background-gdk",
                                                  COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 500);
      //gtk_tree_view_insert_column (GTK_TREE_VIEW (_treeView), column, pos);
      all_columns.push_back(pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_resizable(column, TRUE);
   }
   // now sort and insert in order
   sort(all_columns.begin(), all_columns.end(), mysort());
   for (unsigned int i = 0; i < all_columns.size(); i++) {
      gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView),
                                  GTK_TREE_VIEW_COLUMN(all_columns[i].second));
   }
   // now set name column to expander column
   if (name_column)
      gtk_tree_view_set_expander_column(GTK_TREE_VIEW(_treeView), name_column);

#if GTK_CHECK_VERSION(2,4,0)
#warning build with new fixed_height_mode
   GValue value = { 0, };
   g_value_init(&value, G_TYPE_BOOLEAN);
   g_value_set_boolean(&value, TRUE);
   g_object_set_property(G_OBJECT(_treeView), "fixed_height_mode", &value);
#endif

   _pkgList = GTK_TREE_MODEL(gtk_pkg_list_new(_lister));
   gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), _pkgList);
   gtk_tree_view_set_search_column(GTK_TREE_VIEW(_treeView), NAME_COLUMN);

   // LEAK!!! FIX THIS!!
   /*
   new RCacheActorPkgList(_lister, GTK_PKG_LIST(_pkgList),
                          GTK_TREE_VIEW(_treeView));
   new RPackageListActorPkgList(_lister, GTK_PKG_LIST(_pkgList),
                                GTK_TREE_VIEW(_treeView));
                                */

}

void RGMainWindow::buildInterface()
{
   GtkWidget *button;
   GtkWidget *widget;

   // here is a pointer to rgmainwindow for every widget that needs it
   g_object_set_data(G_OBJECT(_win), "me", this);


   GdkPixbuf *icon = gdk_pixbuf_new_from_xpm_data((const char **)
                                                  synaptic_mini_xpm);
   gtk_window_set_icon(GTK_WINDOW(_win), icon);

   gtk_window_resize(GTK_WINDOW(_win),
                     _config->FindI("Synaptic::windowWidth", 640),
                     _config->FindI("Synaptic::windowHeight", 480));
   gtk_window_move(GTK_WINDOW(_win),
                   _config->FindI("Synaptic::windowX", 100),
                   _config->FindI("Synaptic::windowY", 100));
   RGFlushInterface();


   glade_xml_signal_connect_data(_gladeXML,
                                 "on_about_activate",
                                 G_CALLBACK(cbShowAboutPanel), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_introduction_activate",
                                 G_CALLBACK(cbShowWelcomeDialog), this);


   glade_xml_signal_connect_data(_gladeXML,
                                 "on_icon_legend_activate",
                                 G_CALLBACK(cbShowIconLegendPanel), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_help_activate",
                                 G_CALLBACK(cbHelpAction), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_update_packages",
                                 G_CALLBACK(cbUpdateClicked), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_button_details_clicked",
                                 G_CALLBACK(cbDetailsWindow), this);

   _propertiesB = glade_xml_get_widget(_gladeXML, "button_details");
   assert(_propertiesB);
   _upgradeB = glade_xml_get_widget(_gladeXML, "button_upgrade");
   _upgradeM = glade_xml_get_widget(_gladeXML, "upgrade1");
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_upgrade_packages",
                                 G_CALLBACK(cbUpgradeClicked), this);

   if (_config->FindB("Synaptic::NoUpgradeButtons", false) == true) {
      gtk_widget_hide(_upgradeB);
      widget = glade_xml_get_widget(_gladeXML, "alignment_upgrade");
      gtk_widget_hide(widget);
   }

   _proceedB = glade_xml_get_widget(_gladeXML, "button_procceed");
   _proceedM = glade_xml_get_widget(_gladeXML, "menu_proceed");
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_proceed_clicked",
                                 G_CALLBACK(cbProceedClicked), this);

   _fixBrokenM = glade_xml_get_widget(_gladeXML, "fix_broken_packages");
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_fix_broken_packages",
                                 G_CALLBACK(cbFixBrokenClicked), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_preferences_activate",
                                 G_CALLBACK(cbShowConfigWindow), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_set_option_activate",
                                 G_CALLBACK(cbShowSetOptWindow), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_repositories_activate",
                                 G_CALLBACK(cbShowSourcesWindow), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_exit_activate",
                                 G_CALLBACK(closeWin), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_edit_filter_activate",
                                 G_CALLBACK(cbShowFilterManagerWindow), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_button_pkghelp_clicked",
                                 G_CALLBACK(cbPkgHelpClicked), this);
   _pkgHelpM = glade_xml_get_widget(_gladeXML, "menu_documentation");
   assert(_pkgHelpM);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_button_pkgreconfigure_clicked",
                                 G_CALLBACK(cbPkgReconfigureClicked), this);
   _pkgReconfigureM = glade_xml_get_widget(_gladeXML, "menu_configure");
   assert(_pkgReconfigureM);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_search_name",
                                 G_CALLBACK(cbFindToolClicked), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_undo1_activate",
                                 G_CALLBACK(cbUndoClicked), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_redo1_activate",
                                 G_CALLBACK(cbRedoClicked), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_clear_all_changes_activate",
                                 G_CALLBACK(cbClearAllChangesClicked), this);


   glade_xml_signal_connect_data(_gladeXML,
                                 "on_open_activate",
                                 G_CALLBACK(cbOpenClicked), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_save_activate",
                                 G_CALLBACK(cbSaveClicked), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_save_as_activate",
                                 G_CALLBACK(cbSaveAsClicked), this);


   widget = _keepM = glade_xml_get_widget(_gladeXML, "menu_keep");
   assert(_keepM);
   g_object_set_data(G_OBJECT(widget), "me", this);
   widget = _installM = glade_xml_get_widget(_gladeXML, "menu_install");
   assert(_installM);
   g_object_set_data(G_OBJECT(widget), "me", this);
   widget = _reinstallM = glade_xml_get_widget(_gladeXML, "menu_reinstall");
   assert(_reinstallM);
   g_object_set_data(G_OBJECT(widget), "me", this);
   widget = _pkgupgradeM = glade_xml_get_widget(_gladeXML, "menu_upgrade");
   assert(_upgradeM);
   g_object_set_data(G_OBJECT(widget), "me", this);
   widget = _removeM = glade_xml_get_widget(_gladeXML, "menu_remove");
   g_object_set_data(G_OBJECT(widget), "me", this);
   assert(_removeM);
   widget = _purgeM = glade_xml_get_widget(_gladeXML, "menu_purge");
   g_object_set_data(G_OBJECT(widget), "me", this);
   assert(_purgeM);
#if 0
   _remove_w_depsM = glade_xml_get_widget(_gladeXML, "menu_remove_with_deps");
   assert(_remove_w_depsM);
#endif
   _dl_changelogM = glade_xml_get_widget(_gladeXML, "menu_download_changelog");
   assert(_dl_changelogM);
#ifdef HAVE_RPM
   gtk_widget_hide(_purgeM);
   gtk_widget_hide(_dl_changelogM);
   gtk_widget_hide(glade_xml_get_widget(_gladeXML,"menu_changelog_separator"));
#endif

   // Workaround for a bug in libglade.
   button = glade_xml_get_widget(_gladeXML, "button_update");
   gtk_tooltips_set_tip(GTK_TOOLTIPS(_tooltips), button,
                        _("Refresh the list of known packages"), "");

   button = glade_xml_get_widget(_gladeXML, "button_upgrade");
   gtk_tooltips_set_tip(GTK_TOOLTIPS(_tooltips), button,
                        _("Mark all possible upgrades"), "");

   button = glade_xml_get_widget(_gladeXML, "button_procceed");
   gtk_tooltips_set_tip(GTK_TOOLTIPS(_tooltips), button,
                        _("Apply all marked changes"), "");

   _pkgCommonTextView = glade_xml_get_widget(_gladeXML, "textview_pkgcommon");
   assert(_pkgCommonTextView);
   gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_pkgCommonTextView),
                               GTK_WRAP_WORD);
   _pkgCommonTextBuffer = gtk_text_view_get_buffer(
                               GTK_TEXT_VIEW(_pkgCommonTextView));
   _pkgCommonBoldTag = gtk_text_buffer_create_tag(
                               _pkgCommonTextBuffer,
                               "bold", "weight", PANGO_WEIGHT_BOLD,
                               "scale", 1.1, NULL);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_menu_action_keep",
                                 G_CALLBACK(cbPkgAction),
                                 GINT_TO_POINTER(PKG_KEEP));

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_menu_action_install",
                                 G_CALLBACK(cbPkgAction),
                                 GINT_TO_POINTER(PKG_INSTALL));
   // callback same as for install
   widget = glade_xml_get_widget(_gladeXML, "menu_upgrade");
   assert(widget);
   g_object_set_data(G_OBJECT(widget), "me", this);

   widget = glade_xml_get_widget(_gladeXML, "menu_reinstall");
   assert(widget);
   g_object_set_data(G_OBJECT(widget), "me", this);
   glade_xml_signal_connect_data(_gladeXML,
				 "on_menu_action_reinstall",
				 G_CALLBACK(cbPkgAction),
				 GINT_TO_POINTER(PKG_REINSTALL));
   
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_menu_action_delete",
                                 G_CALLBACK(cbPkgAction),
                                 GINT_TO_POINTER(PKG_DELETE));
#if 0
   widget = glade_xml_get_widget(_gladeXML, "menu_remove_with_deps");
   assert(widget);
   g_object_set_data(G_OBJECT(widget), "me", this);
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_menu_action_delete_with_deps",
                                 G_CALLBACK(cbPkgAction),
                                 GINT_TO_POINTER(PKG_DELETE_WITH_DEPS));
#endif

   widget = glade_xml_get_widget(_gladeXML, "menu_purge");
   assert(widget);
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_menu_action_purge",
                                 G_CALLBACK(cbPkgAction),
                                 GINT_TO_POINTER(PKG_PURGE));

   _pinM = glade_xml_get_widget(_gladeXML, "menu_hold");
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_menu_pin",
                                 G_CALLBACK(cbMenuPinClicked), this);

   // only if pkg help is enabled
#ifndef SYNAPTIC_PKG_HOLD
   gtk_widget_hide(_pinM);
   widget = glade_xml_get_widget(_gladeXML, "separator_hold");
   if (widget != NULL)
      gtk_widget_hide(widget);
#endif

   _pkginfo = glade_xml_get_widget(_gladeXML, "box_pkginfo");
   assert(_pkginfo);

   _vpaned = glade_xml_get_widget(_gladeXML, "vpaned_main");
   assert(_vpaned);
   // If the pane position is restored before the window is shown, it's
   // not restored in the same place as it was.
   show();
   RGFlushInterface();
   gtk_paned_set_position(GTK_PANED(_vpaned),
                          _config->FindI("Synaptic::vpanedPos", 140));

   // build the treeview
   buildTreeView();

   g_signal_connect(G_OBJECT(_treeView), "button-press-event",
                    (GCallback) cbPackageListClicked, this);

   GtkTreeSelection *select;
   select = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
   //gtk_tree_selection_set_mode (select, GTK_SELECTION_MULTIPLE);
   g_signal_connect(G_OBJECT(select), "changed",
                    G_CALLBACK(cbSelectedRow), this);
   g_signal_connect(G_OBJECT(_treeView), "row-activated",
                    G_CALLBACK(cbPackageListRowActivated), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_add_cdrom_activate",
                                 G_CALLBACK(cbAddCDROM), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_download_changelog_activate",
                                 G_CALLBACK(cbChangelogDialog),
                                 this); 

   /* --------------------------------------------------------------- */

   // toolbar menu code
   button = glade_xml_get_widget(_gladeXML, "menu_toolbar_pixmaps");
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
   g_object_set_data(G_OBJECT(button), "me", this);
   g_signal_connect(G_OBJECT(button),
                    "activate",
                    G_CALLBACK(cbMenuToolbarClicked),
                    GINT_TO_POINTER(GTK_TOOLBAR_ICONS));
   if (_toolbarStyle == GTK_TOOLBAR_ICONS)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));

   button = glade_xml_get_widget(_gladeXML, "menu_toolbar_text");
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
   g_object_set_data(G_OBJECT(button), "me", this);
   g_signal_connect(G_OBJECT(button),
                    "activate",
                    G_CALLBACK(cbMenuToolbarClicked),
                    GINT_TO_POINTER(GTK_TOOLBAR_TEXT));
   if (_toolbarStyle == GTK_TOOLBAR_TEXT)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));

   button = glade_xml_get_widget(_gladeXML, "menu_toolbar_both");
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
   g_object_set_data(G_OBJECT(button), "me", this);
   g_signal_connect(G_OBJECT(button),
                    "activate",
                    G_CALLBACK(cbMenuToolbarClicked),
                    GINT_TO_POINTER(GTK_TOOLBAR_BOTH));
   if (_toolbarStyle == GTK_TOOLBAR_BOTH)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));

   button = glade_xml_get_widget(_gladeXML, "menu_toolbar_beside");
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
   g_object_set_data(G_OBJECT(button), "me", this);
   g_signal_connect(G_OBJECT(button),
                    "activate",
                    G_CALLBACK(cbMenuToolbarClicked),
                    GINT_TO_POINTER(GTK_TOOLBAR_BOTH_HORIZ));
   if (_toolbarStyle == GTK_TOOLBAR_BOTH_HORIZ)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));

   button = glade_xml_get_widget(_gladeXML, "menu_toolbar_hide");
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
   g_object_set_data(G_OBJECT(button), "me", this);
   g_signal_connect(G_OBJECT(button),
                    "activate",
                    G_CALLBACK(cbMenuToolbarClicked),
                    GINT_TO_POINTER(TOOLBAR_HIDE));
   if (_toolbarStyle == TOOLBAR_HIDE)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));

   // build popup-menu
   GtkWidget *menuitem, *img;
   _popupMenu = gtk_menu_new();
   menuitem = gtk_image_menu_item_new_with_label(_("Unmark"));
   img = gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_MENU);
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_KEEP);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_image_menu_item_new_with_label(_("Mark for Installation"));
   img = get_gtk_image("package-install");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_INSTALL);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_image_menu_item_new_with_label(_("Mark for Reinstallation"));
   img = get_gtk_image("package-reinstall");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),img);
   g_object_set_data(G_OBJECT(menuitem),"me",this);
   g_signal_connect(menuitem, "activate",
		    (GCallback) cbPkgAction, (void*)PKG_REINSTALL);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);


   menuitem = gtk_image_menu_item_new_with_label(_("Mark for Upgrade"));
   img = get_gtk_image("package-upgrade");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_INSTALL);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_image_menu_item_new_with_label(_("Mark for Removal"));
   img = get_gtk_image("package-remove");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_DELETE);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);


   menuitem = gtk_image_menu_item_new_with_label(_("Mark for Complete Removal"));
   img = get_gtk_image("package-remove");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_PURGE);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);
#ifdef HAVE_RPM
   gtk_widget_hide(menuitem);
#endif

#if 0  // disabled for now
   menuitem = gtk_image_menu_item_new_with_label(_("Remove Including Orphaned Dependencies"));
   img = get_gtk_image("package-remove");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction,
                    (void *)PKG_DELETE_WITH_DEPS);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_separator_menu_item_new();
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_check_menu_item_new_with_label(_("Hold Current Version"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate", (GCallback) cbMenuPinClicked, this);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);
#endif

   menuitem = gtk_separator_menu_item_new ();
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_image_menu_item_new_with_label(_("Properties"));
   img = gtk_image_new_from_stock(GTK_STOCK_PROPERTIES,GTK_ICON_SIZE_MENU);
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbDetailsWindow, this);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

#ifndef HAVE_RPM // recommends stuff
   menuitem = gtk_separator_menu_item_new ();
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_image_menu_item_new_with_label(_("Mark Recommended for Installation"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_image_menu_item_new_with_label(_("Mark Suggested for Installation"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);
#endif

   gtk_widget_show_all(_popupMenu);

   // attach progress bar
   _progressBar = glade_xml_get_widget(_gladeXML, "progressbar_main");
   assert(_progressBar);
   _statusL = glade_xml_get_widget(_gladeXML, "label_status");
   assert(_statusL);

   gtk_misc_set_alignment(GTK_MISC(_statusL), 0.0f, 0.0f);
   gtk_widget_set_usize(GTK_WIDGET(_statusL), 100, -1);
   _cacheProgress = new RGCacheProgress(_progressBar, _statusL);
   assert(_cacheProgress);

   // view stuff
   _viewPopup = glade_xml_get_widget(_gladeXML, "optionmenu_views");
   assert(_viewPopup);

   gtk_option_menu_remove_menu(GTK_OPTION_MENU(_viewPopup));
   gtk_option_menu_set_menu(GTK_OPTION_MENU(_viewPopup), createViewMenu());

   _subViewList = glade_xml_get_widget(_gladeXML, "treeview_subviews");
   assert(_subViewList);
   // Setup the selection handler 
   select = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
   gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
   g_signal_connect(G_OBJECT(select), "changed",
                    G_CALLBACK(cbChangedSubView), this);
}

void RGMainWindow::pkgInstallHelper(RPackage *pkg, bool fixBroken, 
				    bool reInstall)
{
   if (pkg->availableVersion() != NULL)
      pkg->setInstall();

   if(reInstall == true)
       pkg->setReInstall(true);

   // check whether something broke
   if (fixBroken && !_lister->check())
      _lister->fixBroken();
}

void RGMainWindow::pkgRemoveHelper(RPackage *pkg, bool purge, bool withDeps)
{
   if (pkg->getFlags() & RPackage::FImportant) {
      if (!_userDialog->confirm(_("Removing this package may render the "
                                  "system unusable.\n"
                                  "Are you sure you want to do that?"))) {
         _blockActions = TRUE;
         gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(this->_currentB),
                                      TRUE);
         _blockActions = FALSE;
         return;
      }
   }
   if (!withDeps)
      pkg->setRemove(purge);
   else
      pkg->setRemoveWithDeps(true, false);
}

void RGMainWindow::pkgKeepHelper(RPackage *pkg)
{
   pkg->setKeep();
}


void RGMainWindow::setStatusText(char *text)
{

   int listed, installed, broken;
   int toInstall, toReInstall, toRemove;
   double size;

   _lister->getStats(installed, broken, toInstall, toReInstall, toRemove, size);

   if (text) {
      gtk_label_set_text(GTK_LABEL(_statusL), text);
   } else {
      gchar *buffer;
      // we need to make this two strings for i18n reasons
      listed = _lister->viewPackagesSize();
      if (size < 0) {
         buffer =
            g_strdup_printf(_("%i packages listed, %i installed, %i broken. %i to install/upgrade, %i to remove; %s will be freed"),
                            listed, installed, broken, toInstall, toRemove,
                            SizeToStr(fabs(size)).c_str());
      } else if( size > 0) {
         buffer =
            g_strdup_printf(_
                            ("%i packages listed, %i installed, %i broken. %i to install/upgrade, %i to remove; %s will be used"),
                            listed, installed, broken, toInstall, toRemove,
                            SizeToStr(fabs(size)).c_str());
      } else {
         buffer =
            g_strdup_printf(_
                            ("%i packages listed, %i installed, %i broken. %i to install/upgrade, %i to remove"),
                            listed, installed, broken, toInstall, toRemove);
      }
      gtk_label_set_text(GTK_LABEL(_statusL), buffer);
      g_free(buffer);
   }

   gtk_widget_set_sensitive(_upgradeB, _lister->upgradable());
   gtk_widget_set_sensitive(_upgradeM, _lister->upgradable());
   gtk_widget_set_sensitive(_propertiesB, (bool)selectedPackage());

   gtk_widget_set_sensitive(_proceedB, (toInstall + toRemove) != 0);
   gtk_widget_set_sensitive(_proceedM, (toInstall + toRemove) != 0);
   _unsavedChanges = ((toInstall + toRemove) != 0);

   gtk_widget_queue_draw(_statusL);
}

GtkWidget *RGMainWindow::createViewMenu()
{
   GtkWidget *menu, *item;
   vector<string> views;
   views = _lister->getViews();

   menu = gtk_menu_new();

   int i = 0;
   for (vector<string>::const_iterator I = views.begin();
        I != views.end(); I++) {

      item = gtk_menu_item_new_with_label((char *)(*I).c_str());
      gtk_widget_show(item);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      gtk_object_set_data(GTK_OBJECT(item), "me", this);
      gtk_object_set_data(GTK_OBJECT(item), "index", (void *)i++);
      gtk_signal_connect(GTK_OBJECT(item), "activate",
                         (GtkSignalFunc) cbChangedView, this);
   }

   return menu;
}



void RGMainWindow::saveState()
{
   if (_config->FindB("Volatile::NoStateSaving", false) == true)
      return;
   _config->Set("Synaptic::vpanedPos",
                gtk_paned_get_position(GTK_PANED(_vpaned)));
   _config->Set("Synaptic::windowWidth", _win->allocation.width);
   _config->Set("Synaptic::windowHeight", _win->allocation.height);
   gint x, y;
   gtk_window_get_position(GTK_WINDOW(_win), &x, &y);
   _config->Set("Synaptic::windowX", x);
   _config->Set("Synaptic::windowY", y);
   _config->Set("Synaptic::ToolbarState", (int)_toolbarStyle);

   if (!RWriteConfigFile(*_config)) {
      _error->Error(_("An error occurred while saving configurations."));
      _userDialog->showErrors();
   }
   if (!_roptions->store())
      cerr << "Internal Error: error storing raptoptions" << endl;
}

bool RGMainWindow::restoreState()
{

   // see if we have broken packages (might be better in some
   // RGMainWindow::preGuiStart funktion)
   int installed, broken, toInstall, toReInstall, toRemove;
   double sizeChange;
   _lister->getStats(installed, broken, toInstall, toReInstall, toRemove, sizeChange);
   if (broken > 0) {
      gchar *msg;
      msg = ngettext("You have %d broken package on your system!\n\n"
                        "Use the \"Broken\" filter to locate it.",
                        "You have %i broken packages on your system!\n\n"
                        "Use the \"Broken\" filter to locate them.", broken);
      msg = g_strdup_printf(msg, broken);
      _userDialog->warning(msg);
      g_free(msg);
   }

   if(!_config->FindB("Volatile::Upgrade-Mode",false)) {
      int viewNr = _config->FindI("Synaptic::ViewMode", 0);
      gtk_option_menu_set_history(GTK_OPTION_MENU(_viewPopup), viewNr);
      _lister->setView(viewNr);
      refreshSubViewList();
   }
   setStatusText();
   return true;
}


bool RGMainWindow::close()
{
   if (_interfaceLocked > 0)
      return true;

   RGGladeUserDialog dia(this);
   if (_unsavedChanges == false || dia.run("quit")) {
      _error->Discard();
      saveState();
      showErrors();
      exit(0);
   }
   return true;
}



void RGMainWindow::setInterfaceLocked(bool flag)
{
   if (flag) {
      _interfaceLocked++;
      if (_interfaceLocked > 1)
         return;

      gtk_widget_set_sensitive(_win, FALSE);
      gdk_window_set_cursor(_win->window, _busyCursor);
   } else {
      assert(_interfaceLocked > 0);

      _interfaceLocked--;
      if (_interfaceLocked > 0)
         return;

      gtk_widget_set_sensitive(_win, TRUE);
      gdk_window_set_cursor(_win->window, NULL);
   }

#if GTK_CHECK_VERSION(2,4,0)
   // fast enough with the new fixed-height mode
   while (gtk_events_pending())
      gtk_main_iteration();
#else
   // because of the comment above, we only do 5 iterations now
   //FIXME: this is more a hack than a real solution
   for (int i = 0; i < 5; i++) {
      if (gtk_events_pending())
         gtk_main_iteration();
   }
#endif
}

void RGMainWindow::setTreeLocked(bool flag)
{
   if (flag == true) {
      updatePackageInfo(NULL);
      gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), NULL);
   } else {
      gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), _pkgList);
   }
}



// --------------------------------------------------------------------------
// Callbacks
//

void RGMainWindow::cbPkgAction(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) g_object_get_data(G_OBJECT(self), "me");
   assert(me);
   me->pkgAction((RGPkgAction)GPOINTER_TO_INT(data));
}

gboolean RGMainWindow::cbPackageListClicked(GtkWidget *treeview,
                                            GdkEventButton *event,
                                            gpointer data)
{
   //cout << "RGMainWindow::cbPackageListClicked()" << endl;

   RGMainWindow *me = (RGMainWindow *) data;
   RPackage *pkg = NULL;
   GtkTreePath *path;
   GtkTreeViewColumn *column;

   /* Single clicks only */
   if (event->type == GDK_BUTTON_PRESS) {
      GtkTreeSelection *selection;
      GtkTreeIter iter;

      if(!(event->window == gtk_tree_view_get_bin_window(GTK_TREE_VIEW(treeview))))
	 return false;

      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
      if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                                        (int)event->x, (int)event->y,
                                        &path, &column, NULL, NULL)) {

         /* Check if it's either a right-button click, or a left-button
          * click on the status column. */
         if (!(event->button == 3 ||
               (event->button == 1 && strcmp(column->title, "S") == 0)))
            return false;

         vector<RPackage *> selected_pkgs;
         GList *li = NULL;

         // Treat right click as additional selection, and left click
         // as single selection. 
         if (event->button == 1 &&
             ((event->state & GDK_CONTROL_MASK) != GDK_CONTROL_MASK))
            gtk_tree_selection_unselect_all(selection);
         gtk_tree_selection_select_path(selection, path);
         
         li = gtk_tree_selection_get_selected_rows(selection, &me->_pkgList);
         for (li = g_list_first(li); li != NULL; li = g_list_next(li)) {
            gtk_tree_model_get_iter(me->_pkgList, &iter,
                                    (GtkTreePath *) (li->data));

            gtk_tree_model_get(me->_pkgList, &iter, PKG_COLUMN, &pkg, -1);
            if (pkg)
               selected_pkgs.push_back(pkg);
         }

         cbTreeviewPopupMenu(treeview, event, me, selected_pkgs);
         return true;
      }
   }

   return false;
}

void RGMainWindow::cbChangelogDialog(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow*)data;

   RPackage *pkg = me->selectedPackage();
   if(pkg == NULL)
      return;
    
   me->setInterfaceLocked(TRUE);
   pkgAcquireStatus *status= new RGFetchProgress(me);;
   pkgAcquire fetcher(status);
   string filename = pkg->getChangelogFile(&fetcher);
   
   RGGladeUserDialog dia(me,"changelog");
   GtkWidget *textview = glade_xml_get_widget(dia.getGladeXML(),
					      "textview_changelog");
   assert(textview);
   GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
   GtkTextIter start,end;
   gtk_text_buffer_get_start_iter (buffer, &start);
   gtk_text_buffer_get_end_iter(buffer,&end);
   gtk_text_buffer_delete(buffer,&start,&end);
   
   ifstream in(filename.c_str());
   string s;
   while(getline(in, s)) {
      gtk_text_buffer_insert_at_cursor(buffer, s.c_str(), -1);
      gtk_text_buffer_insert_at_cursor(buffer, "\n", -1);
   }
   
   dia.run();
   unlink(filename.c_str());
   me->setInterfaceLocked(FALSE);
}


void RGMainWindow::cbPackageListRowActivated(GtkTreeView *treeview,
                                             GtkTreePath *path,
                                             GtkTreeViewColumn *arg2,
                                             gpointer data)
{
   //cout << "RGMainWindow::cbPackageListRowActivated()" << endl;
   
   RGMainWindow *me = (RGMainWindow *) data;
   GtkTreeIter iter;
   RPackage *pkg = NULL;

   if (!gtk_tree_model_get_iter(me->_pkgList, &iter, path))
      return;

   gtk_tree_model_get(me->_pkgList, &iter, PKG_COLUMN, &pkg, -1);
   assert(pkg);

   int flags = pkg->getFlags();

   if (!(flags & RPackage::FInstalled)) {
      if (flags & RPackage::FKeep)
         me->pkgAction(PKG_INSTALL);
      else if (flags & RPackage::FInstall)
         me->pkgAction(PKG_DELETE);
   } else if (flags & RPackage::FOutdated) {
      if (flags & RPackage::FKeep)
         me->pkgAction(PKG_INSTALL);
      else if (flags & RPackage::FUpgrade)
         me->pkgAction(PKG_KEEP);
   }

   gtk_tree_view_set_cursor(GTK_TREE_VIEW(me->_treeView), path, NULL, false);

   me->setStatusText();
}

void RGMainWindow::cbAddCDROM(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   RGCDScanner scan(me, me->_userDialog);
   me->setInterfaceLocked(TRUE);
   bool updateCache = false;
   bool dontStop = true;
   while (dontStop) {
      if (scan.run() == false)
         me->showErrors();
      else
         updateCache = true;
      dontStop =
         me->_userDialog->confirm(_("Do you want to add another CD-ROM?"));
   }
   scan.hide();
   if (updateCache) {
      me->_lister->openCache(TRUE);
      me->refreshTable(me->selectedPackage());
   }
   me->setInterfaceLocked(FALSE);
}


void RGMainWindow::cbOpenSelections(GtkWidget *file_selector, gpointer data)
{
   //cout << "void RGMainWindow::doOpenSelections()" << endl;
   RGMainWindow *me = (RGMainWindow *) g_object_get_data(G_OBJECT(data), "me");
   const gchar *file;

   file = gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));
   //cout << "selected file: " << file << endl;
   me->selectionsFilename = file;

   ifstream in(file);
   if (!in != 0) {
      _error->Error(_("Can't read %s"), file);
      me->_userDialog->showErrors();
      return;
   }
   me->_lister->unregisterObserver(me);
   me->_lister->readSelections(in);
   me->_lister->registerObserver(me);
   me->setStatusText();
}

void RGMainWindow::cbOpenClicked(GtkWidget *self, void *data)
{
   //std::cout << "RGMainWindow::openClicked()" << endl;
   //RGMainWindow *me = (RGMainWindow*)data;

   GtkWidget *filesel;
   filesel = gtk_file_selection_new(_("Open changes"));
   g_object_set_data(G_OBJECT(filesel), "me", data);

   g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
                    "clicked", G_CALLBACK(cbOpenSelections), filesel);

   g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
                            "clicked",
                            G_CALLBACK(gtk_widget_destroy),
                            (gpointer) filesel);

   g_signal_connect_swapped(GTK_OBJECT
                            (GTK_FILE_SELECTION(filesel)->cancel_button),
                            "clicked", G_CALLBACK(gtk_widget_destroy),
                            (gpointer) filesel);

   gtk_widget_show(filesel);
}

void RGMainWindow::cbSaveClicked(GtkWidget *self, void *data)
{
   //std::cout << "RGMainWindow::saveClicked()" << endl;
   RGMainWindow *me = (RGMainWindow *) data;

   if (me->selectionsFilename == "") {
      me->cbSaveAsClicked(self, data);
      return;
   }

   ofstream out(me->selectionsFilename.c_str());
   if (!out != 0) {
      _error->Error(_("Can't write %s"), me->selectionsFilename.c_str());
      me->_userDialog->showErrors();
      return;
   }

   me->_lister->unregisterObserver(me);
   me->_lister->writeSelections(out, me->saveFullState);
   me->_lister->registerObserver(me);
   me->setStatusText();

}


void RGMainWindow::cbSaveAsClicked(GtkWidget *self, void *data)
{
   //std::cout << "RGMainWindow::saveAsClicked()" << endl;
   //RGMainWindow *me = (RGMainWindow*)data;

   GtkWidget *filesel;
   filesel = gtk_file_selection_new(_("Save changes"));
   g_object_set_data(G_OBJECT(filesel), "me", data);

   g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
                    "clicked", G_CALLBACK(cbSaveSelections), filesel);

   g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
                            "clicked",
                            G_CALLBACK(gtk_widget_destroy),
                            (gpointer) filesel);

   g_signal_connect_swapped(GTK_OBJECT
                            (GTK_FILE_SELECTION(filesel)->cancel_button),
                            "clicked", G_CALLBACK(gtk_widget_destroy),
                            (gpointer) filesel);

   GtkWidget *checkButton =
      gtk_check_button_new_with_label(_("Save full state, not only changes"));
   gtk_box_pack_start_defaults(GTK_BOX(GTK_FILE_SELECTION(filesel)->main_vbox),
                               checkButton);
   g_object_set_data(G_OBJECT(filesel), "checkButton", checkButton);
   gtk_widget_show(checkButton);
   gtk_widget_show(filesel);
}

void RGMainWindow::cbSaveSelections(GtkWidget *file_selector_button,
                                    gpointer data)
{
   GtkWidget *checkButton;
   const gchar *file;

   RGMainWindow *me = (RGMainWindow *) g_object_get_data(G_OBJECT(data), "me");

   file = gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));
   me->selectionsFilename = file;

   // do we want the full state?
   checkButton =
      (GtkWidget *) g_object_get_data(G_OBJECT(data), "checkButton");
   me->saveFullState =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton));

   ofstream out(file);
   if (!out != 0) {
      _error->Error(_("Can't write %s"), file);
      me->_userDialog->showErrors();
      return;
   }
   me->_lister->unregisterObserver(me);
   me->_lister->writeSelections(out, me->saveFullState);
   me->_lister->registerObserver(me);
   me->setStatusText();
}

void RGMainWindow::cbShowConfigWindow(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   if (me->_configWin == NULL) {
      me->_configWin = new RGPreferencesWindow(me, me->_lister);
   }

   me->_configWin->show();
}

void RGMainWindow::cbShowSetOptWindow(GtkWidget *self, void *data)
{
   RGMainWindow *win = (RGMainWindow *) data;

   if (win->_setOptWin == NULL)
      win->_setOptWin = new RGSetOptWindow(win);

   win->_setOptWin->show();
}

void RGMainWindow::cbDetailsWindow(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   assert(data);

   RPackage *pkg = me->selectedPackage();
   if (pkg != NULL) {
      RGPkgDetailsWindow win(me, pkg);
      win.show();
      gtk_main();
   }
}

void RGMainWindow::cbShowSourcesWindow(GtkWidget *self, void *data)
{
   RGMainWindow *win = (RGMainWindow *) data;

   bool Ok = false;
   {
      RGRepositoryEditor w(win);
      Ok = w.Run();
   }
   RGFlushInterface();
   if (Ok == true && _config->FindB("Synaptic::UpdateAfterSrcChange")) {
      win->cbUpdateClicked(NULL, data);
   }
}

void RGMainWindow::cbMenuToolbarClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) g_object_get_data(G_OBJECT(self), "me");
   GtkWidget *widget;
   // save new toolbar state
   me->_toolbarStyle = (GtkToolbarStyle) GPOINTER_TO_INT(data);
   GtkWidget *toolbar = glade_xml_get_widget(me->_gladeXML, "toolbar_main");
   assert(toolbar);
   if (me->_toolbarStyle == TOOLBAR_HIDE) {
      widget = glade_xml_get_widget(me->_gladeXML, "handlebox_button_toolbar");
      gtk_widget_hide(widget);
      return;
   } else {
      widget = glade_xml_get_widget(me->_gladeXML, "handlebox_button_toolbar");
      gtk_widget_show(widget);
   }
   gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), me->_toolbarStyle);
}

void RGMainWindow::cbFindToolClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   if (me->_findWin == NULL) {
      me->_findWin = new RGFindWindow(me);
   }

   int res = gtk_dialog_run(GTK_DIALOG(me->_findWin->window()));
   if (res == GTK_RESPONSE_OK) {
      gdk_window_set_cursor(me->window()->window, me->_busyCursor);
      me->setStatusText(_("Searching..."));
#if GTK_CHECK_VERSION(2,4,0)
      // if we don't iterate here, the busy cursor is not shown
      while (gtk_events_pending())
	 gtk_main_iteration();
#endif
      string str = me->_findWin->getFindString();
      int type = me->_findWin->getSearchType();
      int found = me->_lister->searchView()->setSearch(str, type);
      me->changeView(4,true,str);

      gdk_window_set_cursor(me->window()->window, NULL);
      gchar *statusstr = g_strdup_printf(_("Found %i packages"), found);
      me->setStatusText(statusstr);
      g_free(statusstr);
   }

}

void RGMainWindow::cbShowAboutPanel(GtkWidget *self, void *data)
{
   RGMainWindow *win = (RGMainWindow *) data;

   if (win->_aboutPanel == NULL)
      win->_aboutPanel = new RGAboutPanel(win);
   win->_aboutPanel->show();
}

void RGMainWindow::cbShowIconLegendPanel(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   if (me->_iconLegendPanel == NULL)
      me->_iconLegendPanel = new RGIconLegendPanel(me);
   me->_iconLegendPanel->show();
}

void RGMainWindow::cbHelpAction(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   me->setStatusText(_("Starting help viewer..."));

   if (is_binary_in_path("yelp"))
      system("yelp ghelp:synaptic &");
   else if(is_binary_in_path("khelpcenter")) {
      system("konqueror ghelp:///" PACKAGE_DATA_DIR "/gnome/help/synaptic/C/synaptic.xml &");
   } else if (is_binary_in_path("mozilla")) {
      // mozilla eats bookmarks when run under sudo (because it does not
      // change $HOME)
      if(getenv("SUDO_USER") != NULL) {
         struct passwd *pw = getpwuid(0);
         setenv("HOME", pw->pw_dir, 1);
      }
      system("mozilla " PACKAGE_DATA_DIR "/synaptic/html/index.html &");
   } else if (is_binary_in_path("konqueror"))
      system("konqueror " PACKAGE_DATA_DIR "/synaptic/html/index.html &");
   else
      me->_userDialog->error(_("No help viewer is installed!\n\n"
                               "You need either the GNOME help viewer 'yelp', "
                               "the 'konqueror' browser or the 'mozilla' "
                               "browser to view the synaptic manual.\n\n"
                               "Alternativly you can open the man page "
                               "with 'man synaptic' from the "
                               "command line or view the html version located "
                               "in the 'synaptic/html' folder."));
}

void RGMainWindow::cbCloseFilterManagerAction(void *self, bool okcancel)
{
   RGMainWindow *me = (RGMainWindow *) self;

   me->_lister->filterView()->refreshFilters();
   string currentSubView = me->selectedSubView();
   me->refreshSubViewList();
   me->refreshTable();
}


void RGMainWindow::cbShowFilterManagerWindow(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   if (me->_fmanagerWin == NULL) {
      me->_fmanagerWin = new RGFilterManagerWindow(me, me->_lister->filterView());

      me->_fmanagerWin->setCloseCallback(cbCloseFilterManagerAction, me);
   }

   me->_fmanagerWin->show();

}

void RGMainWindow::cbSelectedRow(GtkTreeSelection *selection, gpointer data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   GtkTreeIter iter;
   RPackage *pkg;
   GList *li, *list;

   //cout << "RGMainWindow::cbSelectedRow()" << endl;

   if (me->_pkgList == NULL) {
      cerr << "selectedRow(): me->_pkgTree == NULL " << endl;
      return;
   }
#if GTK_CHECK_VERSION(2,2,0)
   list = li = gtk_tree_selection_get_selected_rows(selection, &me->_pkgList);
#else
   li = list = NULL;
   gtk_tree_selection_selected_foreach(selection, cbGetSelectedRows, &list);
   li = list;
#endif

   // list is empty
   if (li == NULL) {
      me->updatePackageInfo(NULL);
      return;
   }
   // we are only interessted in the last element
   li = g_list_last(li);
   gtk_tree_model_get_iter(me->_pkgList, &iter, (GtkTreePath *) (li->data));

   gtk_tree_model_get(me->_pkgList, &iter, PKG_COLUMN, &pkg, -1);
   if (pkg == NULL)
      return;

   // free the list
   g_list_foreach(list, (void (*)(void *, void *))gtk_tree_path_free, NULL);
   g_list_free(list);

   me->updatePackageInfo(pkg);
}

void RGMainWindow::cbClearAllChangesClicked(GtkWidget *self, void *data)
{
   //cout << "clearAllChangesClicked" << endl;
   RGMainWindow *me = (RGMainWindow *) data;
   me->setInterfaceLocked(TRUE);
   me->_lister->unregisterObserver(me);
   me->setTreeLocked(TRUE);

   // reset
   me->_lister->openCache(TRUE);

   me->setTreeLocked(FALSE);
   me->_lister->registerObserver(me);
   me->refreshTable();
   me->setInterfaceLocked(FALSE);
}


void RGMainWindow::cbUndoClicked(GtkWidget *self, void *data)
{
   //cout << "undoClicked" << endl;
   RGMainWindow *me = (RGMainWindow *) data;
   me->setInterfaceLocked(TRUE);

   me->_lister->unregisterObserver(me);

   // undo
   me->_lister->undo();

   me->_lister->registerObserver(me);
   me->refreshTable();
   me->setInterfaceLocked(FALSE);
}

void RGMainWindow::cbRedoClicked(GtkWidget *self, void *data)
{
   //cout << "redoClicked" << endl;
   RGMainWindow *me = (RGMainWindow *) data;
   me->setInterfaceLocked(TRUE);

   me->_lister->unregisterObserver(me);

   // redo
   me->_lister->redo();

   me->_lister->registerObserver(me);
   me->refreshTable();
   me->setInterfaceLocked(FALSE);
}

void RGMainWindow::cbPkgReconfigureClicked(GtkWidget *self, void *data)
{
   char frontend[] = "gnome";
   char *cmd;
   RGMainWindow *me = (RGMainWindow *) data;
   //cout << "RGMainWindow::pkgReconfigureClicked()" << endl;

   RPackage *pkg = NULL;
   pkg = me->_lister->getPackage("libgnome2-perl");
   if (pkg && pkg->installedVersion() == NULL) {
      me->_userDialog->error(_("Cannot start configuration tool!\n"
                               "You have to install the required package "
                               "'libgnome2-perl'."));
      return;
   }

   me->setStatusText(_("Starting package configuration tool..."));
   cmd = g_strdup_printf("/usr/sbin/dpkg-reconfigure -f%s %s &",
                         frontend, me->selectedPackage()->name());
   system(cmd);
}


void RGMainWindow::cbPkgHelpClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   if(me->selectedPackage() == NULL)
      return;

   //cout << "RGMainWindow::pkgHelpClicked()" << endl;
   me->setStatusText(_("Starting package documentation viewer..."));

   // mozilla eats bookmarks when run under sudo (because it does not
   // change $HOME) so we better play safe here
   if(getenv("SUDO_USER") != NULL) {
      struct passwd *pw = getpwuid(0);
      setenv("HOME", pw->pw_dir, 1);
   }

   if (is_binary_in_path("dwww"))
      system(g_strdup_printf("dwww %s &", me->selectedPackage()->name()));
   else
      me->_userDialog->error(_("You have to install the package \"dwww\" "
			       "to browse the documentation of a package"));

}


void RGMainWindow::cbChangedView(GtkWidget *self)
{
   RGMainWindow *me =
      (RGMainWindow *) gtk_object_get_data(GTK_OBJECT(self), "me");
   int view = (int)gtk_object_get_data(GTK_OBJECT(self), "index");
   me->changeView(view, false);
}

void RGMainWindow::cbChangedSubView(GtkTreeSelection *selection,
                                    gpointer data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   gdk_window_set_cursor(me->window()->window, me->_busyCursor);
#if GTK_CHECK_VERSION(2,4,0)
   // if we don't iterate here, the busy cursor is not shown
   while (gtk_events_pending())
      gtk_main_iteration();
#endif
   me->refreshTable(NULL);
   gdk_window_set_cursor(me->window()->window, NULL);
}

void RGMainWindow::cbProceedClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   RGSummaryWindow *summ;

   // check whether we can really do it
   if (!me->_lister->check()) {
      me->_userDialog->error(_("Could not apply changes!\n"
                               "Fix broken packages first."));
      return;
   }

   if (_config->FindB("Volatile::Non-Interactive", false) == false) {
      // show a summary of what's gonna happen
      summ = new RGSummaryWindow(me, me->_lister);
      if (!summ->showAndConfirm()) {
         // canceled operation
         delete summ;
         return;
      }
      delete summ;
   }

   me->setInterfaceLocked(TRUE);
   me->updatePackageInfo(NULL);

   me->setStatusText(_("Applying marked changes. This may take a while..."));

   // fetch packages
   RGFetchProgress *fprogress = new RGFetchProgress(me);
   fprogress->setTitle(_("Downloading Package Files"));

   // Do not let the treeview access the cache during the update.
   me->setTreeLocked(TRUE);

   // save selections to temporary file
   const gchar *file =
      g_strdup_printf("%s/selections.proceed", RConfDir().c_str());
   ofstream out(file);
   if (!out != 0) {
      _error->Error(_("Can't write %s"), file);
      me->_userDialog->showErrors();
      return;
   }
   me->_lister->writeSelections(out, false);


   RInstallProgress *iprogress;
#ifdef HAVE_TERMINAL
#ifdef HAVE_RPM
   bool UseTerminal = false;
#else
   bool UseTerminal = true;
#endif
   RGZvtInstallProgress *zvt = NULL;
   if (_config->FindB("Synaptic::UseTerminal", UseTerminal) == true)
      iprogress = zvt = new RGZvtInstallProgress(me);
   else
#endif
#ifdef HAVE_RPM
      iprogress = new RGInstallProgress(me, me->_lister);
#else
      iprogress = new RGDummyInstallProgress();
#endif

   //bool result = me->_lister->commitChanges(fprogress, iprogress);
   me->_lister->commitChanges(fprogress, iprogress);

#ifdef HAVE_TERMINAL
   // wait until the zvt dialog is closed
   if (zvt != NULL) {
      while (GTK_WIDGET_VISIBLE(GTK_WIDGET(zvt->window()))) {
         RGFlushInterface();
         usleep(100000);
      }
   }
#endif
   delete fprogress;
   delete iprogress;

   if (_config->FindB("Synaptic::IgnorePMOutput", false) == false)
      me->showErrors();
   else
      _error->Discard();

   if (_config->FindB("Volatile::Non-Interactive", false) == true) {
      return;
   }

   if (_config->FindB("Synaptic::AskQuitOnProceed", false) == true
       && me->_userDialog->confirm(_("Do you want to quit Synaptic?"))) {
      _error->Discard();
      me->saveState();
      me->showErrors();
      exit(0);
   }

   if (_config->FindB("Synaptic::Download-Only", false) == false) {
      // reset the cache
      if (!me->_lister->openCache(true)) {
         me->showErrors();
         exit(1);
      }
   }
   // reread saved selections
   ifstream in(file);
   if (!in != 0) {
      _error->Error(_("Can't read %s"), file);
      me->_userDialog->showErrors();
      return;
   }
   me->_lister->readSelections(in);
   unlink(file);
   g_free((void *)file);


   me->setTreeLocked(FALSE);
   string currentSubView = me->selectedSubView();
   me->refreshSubViewList(currentSubView);
   me->setInterfaceLocked(FALSE);
   me->setStatusText();
}

void RGMainWindow::cbShowWelcomeDialog(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   RGGladeUserDialog dia(me);
   dia.run("welcome");
   GtkWidget *cb = glade_xml_get_widget(dia.getGladeXML(),
                                        "checkbutton_show_again");
   assert(cb);
   _config->Set("Synaptic::showWelcomeDialog",
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb)));
}


void RGMainWindow::cbUpdateClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   // need to delete dialogs, as they might have data pointing
   // to old stuff
//xxx    delete me->_fmanagerWin;
   me->_fmanagerWin = NULL;

   RGFetchProgress *progress = new RGFetchProgress(me);
   progress->setTitle(_("Downloading Index Files"));

   me->setStatusText(_("Refreshing package list..."));

   me->setInterfaceLocked(TRUE);
   me->setTreeLocked(TRUE);
   me->_lister->unregisterObserver(me);

   // save to temporary file
   const gchar *file =
      g_strdup_printf("%s/selections.update", RConfDir().c_str());
   ofstream out(file);
   if (!out != 0) {
      _error->Error(_("Can't write %s"), file);
      me->_userDialog->showErrors();
      return;
   }
   me->_lister->writeSelections(out, false);

   // update cache and forget about the previous new packages 
   // (only if no error occured)
   string error;
   if (!me->_lister->updateCache(progress,error)) {
      RGGladeUserDialog dia(me,"update_failed");
      GtkWidget *tv = glade_xml_get_widget(dia.getGladeXML(), "textview");
      GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
      gtk_text_buffer_set_text(tb, utf8(error.c_str()), -1);
      dia.run();
   } else
      me->forgetNewPackages();

   delete progress;

   if (me->_lister->openCache(TRUE)) {
      me->showErrors();
   }

   // reread saved selections
   ifstream in(file);
   if (!in != 0) {
      _error->Error(_("Can't read %s"), file);
      me->_userDialog->showErrors();
      return;
   }
   me->_lister->readSelections(in);
   unlink(file);
   g_free((void *)file);

   me->setTreeLocked(FALSE);
   string currentSubView = me->selectedSubView();
   me->refreshSubViewList(currentSubView);
   me->setInterfaceLocked(FALSE);
   me->setStatusText();
}

void RGMainWindow::cbFixBrokenClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   RPackage *pkg = me->selectedPackage();

   bool res = me->_lister->fixBroken();
   me->setInterfaceLocked(TRUE);
   me->refreshTable(pkg);

   if (!res)
      me->setStatusText(_("Failed to resolve dependency problems!"));
   else
      me->setStatusText(_("Successfully fixed dependency problems"));

   me->setInterfaceLocked(FALSE);
   me->showErrors();
}


void RGMainWindow::cbUpgradeClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   RPackage *pkg = me->selectedPackage();
   bool dist_upgrade;
   int res;

   if (!me->_lister->check()) {
      me->_userDialog->error(
                         _("Could not upgrade the system!\n"
                           "Fix broken packages first."));
      return;
   }
   // check if we have saved upgrade type
   UpgradeType upgrade =
      (UpgradeType) _config->FindI("Synaptic::UpgradeType", UPGRADE_ASK);

   if (upgrade == UPGRADE_ASK) {
      // ask what type of upgrade the user wants
      GladeXML *gladeXML;
      GtkWidget *button;

      RGGladeUserDialog dia(me);
      res = dia.run("upgrade", true);
      switch(res) {
      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_DELETE_EVENT:
	 return;
      case GTK_RESPONSE_YES:
	 dist_upgrade = true;
	 break;
      case GTK_RESPONSE_NO:
	 dist_upgrade = false;
	 break;
      default: 
	 cerr << "unknown return " << res
	      << " from UpgradeDialog, please report" << endl;
      }
      gladeXML = dia.getGladeXML();
      // see if the user wants the answer saved
      button = glade_xml_get_widget(gladeXML, "checkbutton_remember");
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
         _config->Set("Synaptic::upgradeType", dist_upgrade);
   } else {
      // use the saved answer (don't ask)
      dist_upgrade = upgrade;
   }

   // do the work
   me->setInterfaceLocked(TRUE);
   me->setStatusText(_("Marking all available upgrades..."));

   me->_lister->saveUndoState();

   if (dist_upgrade)
      res = me->_lister->distUpgrade();
   else
      res = me->_lister->upgrade();

   me->refreshTable(pkg);

   if (res)
      me->setStatusText(_("Successfully marked available upgrades"));
   else
      me->setStatusText(_("Failed to mark all available upgrades!"));

   me->setInterfaceLocked(FALSE);
   me->showErrors();
}

void RGMainWindow::cbMenuPinClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   bool active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self));
   GtkTreeSelection *selection;
   GtkTreeIter iter;
   RPackage *pkg;

   if (me->_blockActions)
      return;

   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_treeView));
   GList *li, *list;
#if GTK_CHECK_VERSION(2,2,0)
   list = li = gtk_tree_selection_get_selected_rows(selection, &me->_pkgList);
#else
   li = list = NULL;
   gtk_tree_selection_selected_foreach(selection, cbGetSelectedRows, &list);
   li = list;
#endif
   if (li == NULL)
      return;

   me->setInterfaceLocked(TRUE);
   me->setTreeLocked(TRUE);
   me->_lister->unregisterObserver(me);

   // save to temporary file
   const gchar *file =
      g_strdup_printf("%s/selections.hold", RConfDir().c_str());
   ofstream out(file);
   if (!out != 0) {
      _error->Error(_("Can't write %s"), file);
      me->_userDialog->showErrors();
      return;
   }
   me->_lister->writeSelections(out, false);

   while (li != NULL) {
      gtk_tree_model_get_iter(me->_pkgList, &iter, (GtkTreePath *) (li->data));
      gtk_tree_model_get(me->_pkgList, &iter, PKG_COLUMN, &pkg, -1);
      if (pkg == NULL) {
         li = g_list_next(li);
         continue;
      }

      pkg->setPinned(active);
      _roptions->setPackageLock(pkg->name(), active);
      li = g_list_next(li);
   }
   me->_lister->openCache(TRUE);

   // reread saved selections
   ifstream in(file);
   if (!in != 0) {
      _error->Error(_("Can't read %s"), file);
      me->_userDialog->showErrors();
      return;
   }
   me->_lister->readSelections(in);
   unlink(file);
   g_free((void *)file);

   // free the list
   g_list_foreach(list, (void (*)(void *, void *))gtk_tree_path_free, NULL);
   g_list_free(list);

   me->_lister->registerObserver(me);
   me->setTreeLocked(FALSE);
   me->refreshTable();
   me->setInterfaceLocked(FALSE);
}

void RGMainWindow::cbTreeviewPopupMenu(GtkWidget *treeview,
                                       GdkEventButton *event,
                                       RGMainWindow *me,
                                       vector<RPackage *> selected_pkgs)
{
   // Nothing selected, shouldn't happen, but we play safely.
   if (selected_pkgs.size() == 0)
      return;

   // FIXME: we take the first pkg and find out available actions,
   //        we should calc available actions from all selected pkgs.
   RPackage *pkg = selected_pkgs[0];

   int flags = pkg->getFlags();

   // Gray out buttons that don't make sense, and update image
   // if necessary.
   GList *item = gtk_container_get_children(GTK_CONTAINER(me->_popupMenu));
   gpointer oneclickitem = NULL;
   for (int i = 0; item != NULL; item = g_list_next(item), i++) {

      gtk_widget_set_sensitive(GTK_WIDGET(item->data), FALSE);
      gtk_widget_show(GTK_WIDGET(item->data));

      // This must be optimized. -- niemeyer

      // Keep button
      if (i == 0) {
         if (!(flags & RPackage::FKeep)) {
            gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
            oneclickitem = item->data;
         }

         GtkWidget *img;
         if (!(flags & RPackage::FInstalled))
            img = get_gtk_image("package-available");
         else
            img = get_gtk_image("package-installed-updated");
         gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item->data), img);
         gtk_widget_show(img);
      }

      // Install button
      if (i == 1 && !(flags & RPackage::FInstalled)
          && !(flags & RPackage::FInstall)) {
         gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
         if (oneclickitem == NULL)
            oneclickitem = item->data;
      }

      // Re-install button
      if (i == 2 && (flags & RPackage::FInstalled)
          && !(flags & RPackage::FOutdated)) {
         gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
      }

      // Upgrade button
      if (i == 3 && (flags & RPackage::FOutdated)
          && !(flags & RPackage::FInstall)) {
         gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
         if (oneclickitem == NULL)
            oneclickitem = item->data;
      }

      if ((flags & RPackage::FInstalled) && !(flags & RPackage::FRemove)) {

         // Remove buttons (remove)
         if (i == 4) {
            gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
            if (i == 4 && oneclickitem == NULL)
               oneclickitem = item->data;
         }

         // Purge
         if (i == 5) {
            gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
	 }
      }

      if (i == 5 && (flags & RPackage::FResidualConfig)
          && !(flags & RPackage::FRemove)) {
         gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
      }

      // Seperator is i==6 (hide on left click)
      if(i == 6 && event->button == 1)
	 gtk_widget_hide(GTK_WIDGET(item->data));
      // Properties is i==7 (available if only one pkg is selected)
      if (i == 7) {
	 if(event->button == 1)
	    gtk_widget_hide(GTK_WIDGET(item->data));
	 else if(selected_pkgs.size() == 1)
	    gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
      }

      // i==8 is sperator, hide on left click
      if(i == 8 && event->button == 1)
	 gtk_widget_hide(GTK_WIDGET(item->data));
      // recommends
      if(i == 9) {
	 if(event->button == 1)
	    gtk_widget_hide(GTK_WIDGET(item->data));
	 else if(selected_pkgs.size() == 1) {
	    GtkWidget *menu;
	    menu = me->buildWeakDependsMenu(pkg, pkgCache::Dep::Recommends);
	    if(menu != NULL) {
	       gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);	    
	       gtk_menu_item_set_submenu(GTK_MENU_ITEM(item->data), menu);
	    } else
	       gtk_widget_set_sensitive(GTK_WIDGET(item->data), FALSE);	    
	 }
      }
      if(i == 10) {
	 if(event->button == 1)
	    gtk_widget_hide(GTK_WIDGET(item->data));
	 else if(selected_pkgs.size() == 1) {
	    gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
	    GtkWidget *menu;
	    menu = me->buildWeakDependsMenu(pkg, pkgCache::Dep::Suggests); 
	    if( menu != NULL) {
	       gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);	    
	       gtk_menu_item_set_submenu(GTK_MENU_ITEM(item->data), menu);
	    } else
	       gtk_widget_set_sensitive(GTK_WIDGET(item->data), FALSE);	    
	 }
      }
   }

   if (event->button == 1 && oneclickitem != NULL &&
       _config->FindB("Synaptic::OneClickOnStatusActions", false) == true) {
      gtk_menu_item_activate(GTK_MENU_ITEM(oneclickitem));
   } else {
      gtk_menu_popup(GTK_MENU(me->_popupMenu), NULL, NULL, NULL, NULL,
                     (event != NULL) ? event->button : 0,
                     gdk_event_get_time((GdkEvent *) event));
   }
}

GtkWidget* RGMainWindow::buildWeakDependsMenu(RPackage *pkg, 
					      pkgCache::Dep::DepType type)
{
   // safty first
   if(pkg == NULL) return NULL;
   bool found=false;

   GtkWidget *menu = gtk_menu_new();
   GtkWidget *item;
   vector<RPackage::DepInformation> deps = pkg->enumDeps();
   for(int i=0;i<deps.size();i++) {
      if(deps[i].type == type) {
	 // not virtual
	 if(!deps[i].isVirtual) {
	    found = true;
	    item = gtk_menu_item_new_with_label(deps[i].name);
	    g_object_set_data(G_OBJECT(item), "me", this);
	    gtk_widget_show(item);
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	    if(deps[i].isSatisfied)
	       gtk_widget_set_sensitive(item, false);
	    else
	       g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(pkgInstallByNameHelper), 
				(void*)deps[i].name);
	 } else {
	    // TESTME: expand virutal packages (expensive!?!)
	    const vector<RPackage *> pkgs = _lister->getPackages();
	    for(int k=0;k<pkgs.size();k++) {
	       vector<string> d = pkgs[k]->provides();
	       for(int j=0;j<d.size();j++)
		  if(strcoll(deps[i].name, d[j].c_str()) == 0) {
		     found = true;
		     item = gtk_menu_item_new_with_label(pkgs[k]->name());
		     g_object_set_data(G_OBJECT(item), "me", this);
		     gtk_widget_show(item);
		     gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		     int f = pkgs[k]->getFlags();
 		     if((f & RPackage::FInstall) || (f & RPackage::FInstalled))
 			gtk_widget_set_sensitive(item, false);
 		     else
			g_signal_connect(G_OBJECT(item), "activate",
					 G_CALLBACK(pkgInstallByNameHelper), 
					 (void*)pkgs[k]->name());
		  }
	    }
	 }
      }
   }
   gtk_widget_show(menu);
   if(found)
      return menu;
   else
      return NULL;
}

void RGMainWindow::pkgInstallByNameHelper(GtkWidget *self, void *data)
{
   const char *name = (const char*)data;
   //cout << "pkgInstallByNameHelper: " << name << endl;
   
   RGMainWindow *me = (RGMainWindow*)g_object_get_data(G_OBJECT(self), "me");

   RPackage *newpkg = (RPackage *) me->_lister->getPackage(name);
   if (newpkg) {
      RPackageLister::pkgState state;
      vector<RPackage *> exclude;
      vector<RPackage *> instPkgs;

      // we always save the state (for undo)
      me->_lister->saveState(state);
      me->_lister->notifyCachePreChange();

      // actual action
      newpkg->setNotify(false);
      me->pkgInstallHelper(newpkg);
      newpkg->setNotify(true);

      exclude.push_back(newpkg);
      instPkgs.push_back(newpkg);

      // ask for additional changes
      if(me->askStateChange(state, exclude)) {
	 me->_lister->saveUndoState(state);
	 me->checkForFailedInst(instPkgs);
      }
      me->_lister->notifyPostChange(NULL);
      me->_lister->notifyCachePostChange();
      
      RPackage *pkg = me->selectedPackage();
      me->refreshTable(pkg);
      me->updatePackageInfo(pkg);
   }
}

// vim:ts=3:sw=3:et
