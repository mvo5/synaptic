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
#include "rgcacheprogress.h"
#include "rguserdialog.h"
#include "rginstallprogress.h"
#include "rgdummyinstallprogress.h"
#include "rgzvtinstallprogress.h"
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


void RGMainWindow::showRepositoriesWindow()
{
   cbShowSourcesWindow(NULL, (void *)this);
}

void RGMainWindow::proceed()
{
   cbProceedClicked(NULL, (void *)this);
}

void RGMainWindow::changeFilter(int filter, bool sethistory)
{
   //cout << "RGMainWindow::changeFilter()"<<endl;

   if (sethistory)
      gtk_option_menu_set_history(GTK_OPTION_MENU(_filterPopup), filter);

   RPackage *pkg = selectedPackage();
   setInterfaceLocked(TRUE);
   _blockActions = TRUE;

   // try to set filter
   if (filter > 0) {
      _lister->setFilter(filter - 1);
      RFilter *pkgfilter = _lister->getFilter();
      if (pkgfilter != NULL) {
         // -2 because "0" is "unchanged" and "1" is the spacer in the menu
         // FIXME: yes I know this sucks
         int expand = pkgfilter->getViewMode().expandMode;
         if (expand == 2)
            gtk_tree_view_expand_all(GTK_TREE_VIEW(_treeView));
         if (expand == 3)
            gtk_tree_view_collapse_all(GTK_TREE_VIEW(_treeView));
      } else {
         filter = 0;
      }
   }

   // No filter given or not available from above
   if (filter == 0)
      _lister->setFilter();

   refreshTable(pkg);
   _blockActions = FALSE;
   setInterfaceLocked(FALSE);
   setStatusText();
}

void RGMainWindow::changeView(int view, bool sethistory)
{
   if (sethistory)
      gtk_option_menu_set_history(GTK_OPTION_MENU(_viewPopup), view);

   RPackage *pkg = selectedPackage();
   setInterfaceLocked(TRUE);
   _blockActions = TRUE;

   _lister->setView(view);

   refreshSubViewList();

   _lister->reapplyFilter();

   refreshTable(pkg);
   _blockActions = FALSE;
   setInterfaceLocked(FALSE);
   setStatusText();
}

void RGMainWindow::refreshSubViewList()
{
   GtkTreeIter iter;
   vector<string> subViews = _lister->getSubViews();
   GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
   for (vector<string>::iterator I = subViews.begin();
        I != subViews.end(); I++) {
      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter, 0, (*I).c_str(), -1);
   }
   gtk_tree_view_set_model(GTK_TREE_VIEW(_subViewList), GTK_TREE_MODEL(store));
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
         ret = subView;
         g_free(subView);
      }
   }

   return ret;
}


bool RGMainWindow::showErrors()
{
   return _userDialog->showErrors();
}

static void setLabel(GladeXML *xml, const char *widget_name,
                     const char *value)
{
   GtkWidget *widget = glade_xml_get_widget(xml, widget_name);
   if (widget == NULL)
      cout << "widget == NULL with: " << widget_name << endl;

   if (!value)
      value = _("N/A");
   gtk_label_set_label(GTK_LABEL(widget), utf8(value));
}

static void setLabel(GladeXML *xml, const char *widget_name, const int value)
{
   string strVal;
   GtkWidget *widget = glade_xml_get_widget(xml, widget_name);
   if (widget == NULL)
      cout << "widget == NULL with: " << widget_name << endl;

   // we can never have values of zero or less
   if (value <= 0)
      strVal = _("N/A");
   else
      strVal = SizeToStr(value);
   gtk_label_set_label(GTK_LABEL(widget), utf8(strVal.c_str()));
}


void RGMainWindow::notifyChange(RPackage *pkg)
{
   if (pkg != NULL)
      refreshTable(pkg);

   setStatusText();
}

void RGMainWindow::forgetNewPackages()
{
   //cout << "forgetNewPackages called" << endl;
   unsigned int row = 0;
   while (row < _lister->viewPackagesSize()) {
      RPackage *elem = _lister->getViewPackage(row);
      if (elem->getFlags() && RPackage::FNew)
         elem->setNew(false);
   }
   _roptions->forgetNewPackages();
}


void RGMainWindow::refreshTable(RPackage *selectedPkg)
{
   _lister->setSubView(selectedSubView());

   _pkgList = GTK_TREE_MODEL(gtk_pkg_list_new(_lister));
   gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView),
                           GTK_TREE_MODEL(_pkgList));

   gtk_adjustment_value_changed(
         gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(_treeView)));
   gtk_adjustment_value_changed(
         gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(_treeView)));

   updatePackageInfo(selectedPkg);

   setStatusText();
}

void RGMainWindow::updatePackageInfo(RPackage *pkg)
{
   if (_blockActions)
      return;

   if (!pkg) {
      gtk_widget_set_sensitive(_pkginfo, false);
      gtk_text_buffer_set_text(_pkgCommonTextBuffer,
                               _("No package is selected.\n"), -1);
   } else {
      gtk_widget_set_sensitive(_pkginfo, true);
      gtk_text_buffer_set_text(_pkgCommonTextBuffer,
                               utf8(pkg->description()), -1);

      GtkTextIter start;
      gtk_text_buffer_get_iter_at_offset(_pkgCommonTextBuffer, &start, 0);
      gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(_pkgCommonTextView),
                                   &start, 0, false, 0, 0);
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
#if 0                           // PORTME!
            // This is segfaulting because clickedRecInstall can't find the
            // "pkg" data in _recList.
            if (_config->FindB("Synaptic::UseRecommends", 0)) {
               //cout << "auto installing recommended" << endl;
               me->cbInstallWDeps(me->_win, "Recommends");
            }
            if (_config->FindB("Synaptic::UseSuggests", 0)) {
               //cout << "auto installing suggested" << endl;
               me->cbInstallWDeps(me->_win, "Suggested");
            }
#endif
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

   vector<RPackage *> toKeep;
   vector<RPackage *> toInstall;
   vector<RPackage *> toReInstall;
   vector<RPackage *> toUpgrade;
   vector<RPackage *> toDowngrade;
   vector<RPackage *> toRemove;

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
         changed = false;
      }
      delete chng;
   }

   if (changed) {
      // standard header in case the installing fails
      string failedReason(_("Some packages could not be installed.\n\n"
                            "The following packages have unmet "
                            "dependencies:\n"));
      _lister->saveUndoState(state);
      // check for failed installs
      if (action == PKG_INSTALL) {
         bool failed = false;
         for (unsigned int i = 0; i < instPkgs.size(); i++) {
            pkg = instPkgs[i];
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
            // TODO: make this a special dialog with TextView
            _userDialog->warning(utf8(failedReason.c_str()));
         }
      }
   }

   if (ask)
      _lister->registerObserver(this);

   refreshTable(pkg);

   g_list_foreach(list, (void (*)(void *, void *))gtk_tree_path_free, NULL);
   g_list_free(list);

   _blockActions = FALSE;
   setInterfaceLocked(FALSE);
   updatePackageInfo(pkg);
}


RGMainWindow::RGMainWindow(RPackageLister *packLister, string name)
   : RGGladeWindow(NULL, name), _lister(packLister), _pkgList(0),
_treeView(0)
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

   refreshFilterMenu();

   _userDialog = new RGUserDialog(this);

   packLister->setUserDialog(_userDialog);

   packLister->setProgressMeter(_cacheProgress);

   _filterWin = NULL;
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

#if GTK_CHECK_VERSION(2,3,2)
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
   GtkWidget *item;
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;
   GtkTreeSelection *selection;

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

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_button_pkgreconfigure_clicked",
                                 G_CALLBACK(cbPkgReconfigureClicked), this);

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


   _keepM = glade_xml_get_widget(_gladeXML, "menu_keep");
   assert(_keepM);
   _installM = glade_xml_get_widget(_gladeXML, "menu_install");
   assert(_installM);
   _reinstallM = glade_xml_get_widget(_gladeXML, "menu_reinstall");
   assert(_reinstallM);
   _pkgupgradeM = glade_xml_get_widget(_gladeXML, "menu_upgrade");
   assert(_upgradeM);
   _removeM = glade_xml_get_widget(_gladeXML, "menu_remove");
   assert(_removeM);
   _remove_w_depsM = glade_xml_get_widget(_gladeXML, "menu_remove_with_deps");
   assert(_remove_w_depsM);
   _purgeM = glade_xml_get_widget(_gladeXML, "menu_purge");
   assert(_purgeM);
#ifdef HAVE_RPM
   gtk_widget_hide(_purgeM);
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

   widget = glade_xml_get_widget(_gladeXML, "menu_remove_with_deps");
   assert(widget);
   g_object_set_data(G_OBJECT(widget), "me", this);
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_menu_action_delete_with_deps",
                                 G_CALLBACK(cbPkgAction),
                                 GINT_TO_POINTER(PKG_DELETE_WITH_DEPS));


   widget = glade_xml_get_widget(_gladeXML, "menu_purge");
   assert(widget);
   g_object_set_data(G_OBJECT(widget), "me", this);
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


   _filterPopup = glade_xml_get_widget(_gladeXML, "optionmenu_filters");
   assert(_filterPopup);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_find_button_clicked",
                                 G_CALLBACK(cbFindToolClicked), this);

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
   menuitem = gtk_image_menu_item_new_with_label(_("No Changes"));
   img = gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_MENU);
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_KEEP);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_image_menu_item_new_with_label(_("Install"));
   img = get_gtk_image("package-install");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_INSTALL);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_image_menu_item_new_with_label(_("Reinstall"));
   img = get_gtk_image("package-reinstall");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),img);
   g_object_set_data(G_OBJECT(menuitem),"me",this);
   g_signal_connect(menuitem, "activate",
		    (GCallback) cbPkgAction, (void*)PKG_REINSTALL);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);


   menuitem = gtk_image_menu_item_new_with_label(_("Upgrade"));
   img = get_gtk_image("package-upgrade");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_INSTALL);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_image_menu_item_new_with_label(_("Remove"));
   img = get_gtk_image("package-remove");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_DELETE);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

#ifndef HAVE_RPM
   menuitem = gtk_image_menu_item_new_with_label(_("Remove Including Configuration"));
   img = get_gtk_image("package-remove");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), img);
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_PURGE);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);
   
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

   GtkTreeIter iter;
   _subViewList = glade_xml_get_widget(_gladeXML, "treeview_subviews");
   assert(_subViewList);

   renderer = gtk_cell_renderer_text_new();
   column = gtk_tree_view_column_new_with_attributes("SubView",
                                                     renderer,
                                                     "text", 0, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(_subViewList), column);

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
      } else {
         buffer =
            g_strdup_printf(_
                            ("%i packages listed, %i installed, %i broken. %i to install/upgrade, %i to remove; %s will be used"),
                            listed, installed, broken, toInstall, toRemove,
                            SizeToStr(fabs(size)).c_str());
      };
      gtk_label_set_text(GTK_LABEL(_statusL), buffer);
      g_free(buffer);
   }

   gtk_widget_set_sensitive(_upgradeB, _lister->upgradable());
   gtk_widget_set_sensitive(_upgradeM, _lister->upgradable());

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

GtkWidget *RGMainWindow::createFilterMenu()
{
   GtkWidget *menu, *item;
   vector<string> filters = _lister->getFilterNames();

   menu = gtk_menu_new();

   item = gtk_menu_item_new_with_label(_("All Packages"));
   gtk_widget_show(item);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
   gtk_object_set_data(GTK_OBJECT(item), "me", this);
   gtk_object_set_data(GTK_OBJECT(item), "index", (void *)0);
   gtk_signal_connect(GTK_OBJECT(item), "activate",
                      (GtkSignalFunc) cbChangedFilter, this);

   int i = 1;
   for (vector<string>::const_iterator iter = filters.begin();
        iter != filters.end(); iter++) {

      item = gtk_menu_item_new_with_label((char *)(*iter).c_str());
      gtk_widget_show(item);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      gtk_object_set_data(GTK_OBJECT(item), "me", this);
      gtk_object_set_data(GTK_OBJECT(item), "index", (void *)i++);
      gtk_signal_connect(GTK_OBJECT(item), "activate",
                         (GtkSignalFunc) cbChangedFilter, this);
   }

   return menu;
}

void RGMainWindow::refreshFilterMenu()
{
   GtkWidget *menu;

   // Toolbar
   gtk_option_menu_remove_menu(GTK_OPTION_MENU(_filterPopup));
   menu = createFilterMenu();
   gtk_option_menu_set_menu(GTK_OPTION_MENU(_filterPopup), menu);
}


void RGMainWindow::saveState()
{
   if (_config->FindB("Volatile::NoStateSaving", false) == true)
      return;
   _lister->storeFilters();
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
      if (broken == 1) {
         msg = ngettext("You have %d broken package on your system!\n\n"
                        "Use the \"Broken\" filter to locate it.",
                        "You have %i broken packages on your system!\n\n"
                        "Use the \"Broken\" filter to locate them.", broken);
         msg = g_strdup_printf(msg, broken);
      }
      _userDialog->warning(msg);
      g_free(msg);
   }

   int viewNr = _config->FindI("Synaptic::ViewMode", 0);
   gtk_option_menu_set_history(GTK_OPTION_MENU(_viewPopup), viewNr);
   _lister->setView(viewNr);
   refreshSubViewList();

   // this is real restore stuff
   _lister->restoreFilters();
   refreshFilterMenu();

   int filterNr = _config->FindI("Volatile::initialFilter", 0);
   changeFilter(filterNr);
   refreshTable();

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

#if 0
   // this sucks for the new gtktreeview -- it updates itself via 
   // such events, while the gui is perfetly usable 
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
   RGMainWindow *me = (RGMainWindow *) data;
   RPackage *pkg = NULL;
   GtkTreePath *path;
   GtkTreeViewColumn *column;

   /* Single clicks only */
   if (event->type == GDK_BUTTON_PRESS) {
      GtkTreeSelection *selection;
      GtkTreeIter iter;

      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
      // FIXME: this is gtk2.2
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

void RGMainWindow::cbPackageListRowActivated(GtkTreeView *treeview,
                                             GtkTreePath *path,
                                             GtkTreeViewColumn *arg2,
                                             gpointer data)
{
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


#ifdef NOTINUSE                 // These will be moved to the new Details dialog
void RGMainWindow::cbInstallWDeps(GtkWidget *self, void *data)
{
   const char *installDepType = (const char *)data;
   assert(data);

   RGMainWindow *me = (RGMainWindow *) g_object_get_data(G_OBJECT(self), "me");
   assert(me);

   me->_lister->unregisterObserver(me);
   me->setInterfaceLocked(TRUE);
   me->_blockActions = TRUE;

   RPackageLister::pkgState state;
   me->_lister->saveState(state);
   me->_lister->saveUndoState(state);

   // Who sets this?? -- niemeyer
   RPackage *pkg =
      (RPackage *) g_object_get_data(G_OBJECT(me->_recList), "pkg");
   assert(pkg);

   const char *depType = NULL, *depName = NULL;
   bool satisfied = false;
   if (pkg->enumWDeps(depType, depName, satisfied)) {
      do {
         if (!satisfied && strcmp(depType, installDepType) == 0) {
            RPackage *newpkg = me->_lister->getPackage(depName);
            if (newpkg)
               me->pkgInstallHelper(newpkg);
            else
               cerr << "Dependency " << depName << " not found." << endl;
         }
      } while (pkg->nextWDeps(depType, depName, satisfied));
   }

   me->_blockActions = FALSE;
   me->_lister->registerObserver(me);
   me->refreshTable(pkg);
   me->setInterfaceLocked(FALSE);
}

void RGMainWindow::cbInstallSelected(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) g_object_get_data(G_OBJECT(self), "me");
   assert(me);

   me->_lister->unregisterObserver(me);
   me->setInterfaceLocked(TRUE);
   me->_blockActions = TRUE;

   RPackageLister::pkgState state;
   me->_lister->saveState(state);
   me->_lister->saveUndoState(state);

   // Who sets this?? -- niemeyer
   RPackage *pkg =
      (RPackage *) g_object_get_data(G_OBJECT(me->_recList), "pkg");
   assert(pkg);

   GtkTreeSelection *selection;
   GtkTreeIter iter;
   GList *list, *li;
   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_recList));
#if GTK_CHECK_VERSION(2,2,0)
   list = li = gtk_tree_selection_get_selected_rows(selection, NULL);
#else
   li = list = NULL;
   gtk_tree_selection_selected_foreach(selection, cbGetSelectedRows, &list);
   li = list;
#endif

   while (li != NULL) {
      gtk_tree_model_get_iter(GTK_TREE_MODEL(me->_recListStore), &iter,
                              (GtkTreePath *) (li->data));
      gtk_tree_model_get(GTK_TREE_MODEL(me->_recListStore), &iter,
                         DEP_NAME_COLUMN, &recstr, -1);
      if (!recstr) {
         cerr << "Internal Error: gtk_tree_model_get returned no text" << endl;
         li = g_list_next(li);
         continue;
      }

      const char *depName = index(recstr, ':') + 2;
      if (!depName) {
         cerr << "\":\" not found" << endl;
         li = g_list_next(li);
         continue;
      }

      RPackage *newpkg = (RPackage *) me->_lister->getPackage(depName);
      if (newpkg)
         me->pkgInstallHelper(newpkg);
      else
         cerr << depName << " not found" << endl;
      li = g_list_next(li);
   }

   g_list_foreach(list, (void (*)(void *, void *))gtk_tree_path_free, NULL);
   g_list_free(list);

   me->_blockActions = FALSE;
   me->_lister->registerObserver(me);
   me->refreshTable(pkg);
   me->setInterfaceLocked(FALSE);
}
#endif

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
      gdk_window_set_cursor(me->_findWin->window()->window, me->_busyCursor);
      RFilter *filter = me->_lister->findFilter(0);
      filter->reset();
      int searchType = me->_findWin->getSearchType();
      string S(me->_findWin->getFindString());

      if (searchType & 1 << RPatternPackageFilter::Name) {
         RPatternPackageFilter::DepType type = RPatternPackageFilter::Name;
         filter->pattern.addPattern(type, S, false);
      }
      if (searchType & 1 << RPatternPackageFilter::Version) {
         RPatternPackageFilter::DepType type = RPatternPackageFilter::Version;
         filter->pattern.addPattern(type, S, false);
      }
      if (searchType & 1 << RPatternPackageFilter::Description) {
         RPatternPackageFilter::DepType type =
            RPatternPackageFilter::Description;
         filter->pattern.addPattern(type, S, false);
      }
      if (searchType & 1 << RPatternPackageFilter::Maintainer) {
         RPatternPackageFilter::DepType type =
            RPatternPackageFilter::Maintainer;
         filter->pattern.addPattern(type, S, false);
      }
      if (searchType & 1 << RPatternPackageFilter::Depends) {
         RPatternPackageFilter::DepType type = RPatternPackageFilter::Depends;
         filter->pattern.addPattern(type, S, false);
      }
      if (searchType & 1 << RPatternPackageFilter::Provides) {
         RPatternPackageFilter::DepType type = RPatternPackageFilter::Provides;
         filter->pattern.addPattern(type, S, false);
      }

      me->changeFilter(1);
      gdk_window_set_cursor(me->_findWin->window()->window, NULL);
      me->_findWin->hide();
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

   me->setStatusText(_("Starting help system..."));

   if (is_binary_in_path("yelp"))
      system("yelp ghelp:synaptic &");
   else if (is_binary_in_path("mozilla"))
      system("mozilla " PACKAGE_DATA_DIR "/synaptic/html/index.html &");
   else if (is_binary_in_path("konqueror"))
      system("konqueror " PACKAGE_DATA_DIR "/synaptic/html/index.html &");
   else
      me->_userDialog->error(_("No help viewer is installed\n\n"
                               "You need either the gnome viewer 'yelp', "
                               "'konquoror' or the 'mozilla' browser to "
                               "view the synaptic manual.\n\n"
                               "Alternativly you can open the man page "
                               "with 'man synaptic' from the "
                               "command line or view the html version located "
                               "in the 'synaptic/html' folder."));
}

void RGMainWindow::cbCloseFilterManagerAction(void *self, bool okcancel)
{
   RGMainWindow *me = (RGMainWindow *) self;

   // user clicked ok
   if (okcancel) {
      int i = gtk_option_menu_get_history(GTK_OPTION_MENU(me->_filterPopup));
      me->refreshFilterMenu();
      gtk_option_menu_set_history(GTK_OPTION_MENU(me->_filterPopup), i);
      me->changeFilter(i);
   }

   gtk_widget_set_sensitive(me->_filtersB, TRUE);
   gtk_widget_set_sensitive(me->_filterPopup, TRUE);
}


void RGMainWindow::cbShowFilterManagerWindow(GtkWidget *self, void *data)
{
   RGMainWindow *win = (RGMainWindow *) data;

   if (win->_fmanagerWin == NULL) {
      win->_fmanagerWin = new RGFilterManagerWindow(win, win->_lister);

      win->_fmanagerWin->setCloseCallback(cbCloseFilterManagerAction, win);
   }

   win->_fmanagerWin->show();

   gtk_widget_set_sensitive(win->_filtersB, FALSE);
   gtk_widget_set_sensitive(win->_filterPopup, FALSE);
}

void RGMainWindow::cbSelectedRow(GtkTreeSelection *selection, gpointer data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   GtkTreeIter iter;
   RPackage *pkg;
   GList *li, *list;

   //cout << "selectedRow()" << endl;

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
      me->_userDialog->error(_("No libgnome2-perl installed\n\n"
                               "You have to install libgnome2-perl to "
                               "use dpkg-reconfigure with synaptic"));
      return;
   }

   me->setStatusText(_("Starting dpkg-reconfigure..."));
   cmd = g_strdup_printf("/usr/sbin/dpkg-reconfigure -f%s %s &",
                         frontend, me->selectedPackage()->name());
   system(cmd);
}


void RGMainWindow::cbPkgHelpClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   //cout << "RGMainWindow::pkgHelpClicked()" << endl;
   me->setStatusText(_("Starting package help..."));

   system(g_strdup_printf("dwww %s &", me->selectedPackage()->name()));
}


void RGMainWindow::cbChangedFilter(GtkWidget *self)
{
   int filter;
   RGMainWindow *mainw =
      (RGMainWindow *) gtk_object_get_data(GTK_OBJECT(self), "me");

   filter = (int)gtk_object_get_data(GTK_OBJECT(self), "index");

   mainw->changeFilter(filter, false);
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
   me->refreshTable(NULL);
}

void RGMainWindow::cbProceedClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   RGSummaryWindow *summ;

   // check whether we can really do it
   if (!me->_lister->check()) {
      me->_userDialog->error(_("Operation not possible with broken packages.\n"
                               "Please fix them first."));
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

   me->
      setStatusText(_("Performing selected changes... this may take a while"));

   // fetch packages
   RGFetchProgress *fprogress = new RGFetchProgress(me);
   fprogress->setTitle(_("Retrieving Package Files..."));

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
      if (!me->_lister->openCache(TRUE)) {
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
   me->refreshTable();
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
//xxx    delete me->_filterWin;
//xxx    delete me->_fmanagerWin;
   me->_fmanagerWin = NULL;
   me->_filterWin = NULL;

   RGFetchProgress *progress = new RGFetchProgress(me);
   progress->setTitle(_("Retrieving Index Files..."));

   me->setStatusText(_("Updating Package Lists from Servers..."));

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
   if (!me->_lister->updateCache(progress))
      me->showErrors();
   else
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
   me->refreshTable();
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
      me->setStatusText(_("Dependency problem resolver failed."));
   else
      me->setStatusText(_("Dependency problems successfully fixed."));

   me->setInterfaceLocked(FALSE);
   me->showErrors();
}


void RGMainWindow::cbUpgradeClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   RPackage *pkg = me->selectedPackage();
   bool res, dist_upgrade;

   if (!me->_lister->check()) {
      me->_userDialog->error(_("Automatic upgrade selection not possible\n"
                               "with broken packages. Please fix them first."));
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
      dist_upgrade = dia.run("upgrade");
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
   me->
      setStatusText(_
                    ("Performing automatic selection of upgradadable packages..."));

   me->_lister->saveUndoState();

   if (dist_upgrade)
      res = me->_lister->distUpgrade();
   else
      res = me->_lister->upgrade();

   me->refreshTable(pkg);

   if (res)
      me->
         setStatusText(_
                       ("Automatic selection of upgradadable packages done."));
   else
      me->setStatusText(_("Automatic upgrade selection failed."));

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

      if ((flags & RPackage::FInstalled) && !(flags && RPackage::FRemove)) {

         // Remove buttons (remove, remove with dependencies)
         if (i == 4 || i == 6) {
            gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
            if (i == 4 && oneclickitem == NULL)
               oneclickitem = item->data;
         }

         // Purge
         if (i == 5)
            gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
      }
      
      if (i == 4 && (flags & RPackage::FResidualConfig)
          && !(flags && RPackage::FRemove)) {
         gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
      }

      // Seperator is i==6 (ignored)
      // Hold button 
      if (i == 8) {
         gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
         bool locked = (flags & RPackage::FPinned);
         me->_blockActions = TRUE;
         gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->data),
                                        locked);
         me->_blockActions = FALSE;
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


// vim:ts=3:sw=3:et
