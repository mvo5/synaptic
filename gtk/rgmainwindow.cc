/* rgmainwindow.cc - main window of the app
 *
 * Copyright (c) 2001-2003 Conectiva S/A
 *               2002-2004 Michael Vogt <mvo@debian.org>
 *               2004 Canonical

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

#include "config.h" // IWYU pragma: associated

#include "rgmainwindow.h"

#include "gtkpkglist.h"
#include "i18n.h"
#include "raptoptions.h"
#include "rconfiguration.h"
#include "rgcacheprogress.h"
#include "rgchangelogdialog.h"
#include "rgchangeswindow.h"
#include "rgdebinstallprogress.h"
#include "rgfetchprogress.h"
#include "rgfiltermanager.h"
#include "rgfindwindow.h"
#include "rggtkbuilderwindow.h"
#include "rgiconlegend.h"
#include "rglogview.h"
#include "rgpkgcdrom.h"
#include "rgpkgdetails.h"
#include "rgpkgtreeview.h"
#include "rgpreferenceswindow.h"
#include "rgrepositorywin.h"
#include "rgsetoptwindow.h"
#include "rgsummarywindow.h"
#include "rgtaskswin.h"
#include "rgterminstallprogress.h"
#include "rguserdialog.h"
#include "rgutils.h"
#include "rgwindow.h"
#include "rinstallprogress.h"
#include "rpackage.h"
#include "rpackagecache.h"
#include "rpackagelister.h"
#include "rpackageview.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/depcache.h>
#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/strutl.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <gtk/gtk.h>
#include <iostream>
#include <libintl.h>
#include <pwd.h>
#include <signal.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

using namespace std;

const char *relOptions[] = {N_("Dependencies"),
                            N_("Dependants"),
                            N_("Dependencies of the Latest Version"),
                            N_("Provided Packages"),
                            NULL};

enum {
   WHAT_IT_DEPENDS_ON,
   WHAT_DEPENDS_ON_IT,
   WHAT_IT_WOULD_DEPEND_ON,
   WHAT_IT_PROVIDES,
   WHAT_IT_SUGGESTS
};

enum {
   DEP_NAME_COLUMN,            /* text */
   DEP_IS_NOT_AVAILABLE,       /* foreground-set */
   DEP_IS_NOT_AVAILABLE_COLOR, /* foreground */
   DEP_PKG_INFO
}; /* additional info (install
      not installed) as text */

GtkCssProvider *RGMainWindow::_fastSearchCssProvider = NULL;

task<void> RGMainWindow::changeView(int view, string subView)
{
   if (_config->FindB("Debug::Synaptic::View", false))
      ioprintf(clog,
               "RGMainWindow::changeView(): view '%i' subView '%s'\n",
               view,
               subView.size() > 0 ? subView.c_str() : "(empty)");

   if (view >= N_PACKAGE_VIEWS) {
      // cerr << "changeView called with invalid view NR: " << view << endl;
      view = 0;
   }

   _blockActions = TRUE;
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_viewButtons[view]), TRUE);

   // we need to set a empty model first so that gtklistview
   // can do its cleanup, if we do not do that, then the cleanup
   // code in gtktreeview gets confused and throws
   // Gtk-CRITICAL **: gtk_tree_view_unref_tree_helper: assertion `node != NULL'
   // failed at us, see LP: #38397 for more information
   gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), NULL);

   RPackage *pkg = selectedPackage();

   _lister->setView(view);

   refreshSubViewList();

   GtkTreeSelection *selection;
   setBusyCursor(true);
   co_await setInterfaceLocked(TRUE);
   GtkWidget *tview =
      GTK_WIDGET(gtk_builder_get_object(_builder, "treeview_subviews"));
   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tview));
   if (!subView.empty()) {
      GtkTreeModel *model;
      GtkTreeIter iter;
      char *str;

      model = gtk_tree_view_get_model(GTK_TREE_VIEW(tview));
      if (gtk_tree_model_get_iter_first(model, &iter)) {
         do {
            gtk_tree_model_get(model, &iter, 0, &str, -1);
            if (strcoll(str, MarkupEscapeString(subView).c_str()) == 0) {
               gtk_tree_selection_select_iter(selection, &iter);
               break;
            }
         } while (gtk_tree_model_iter_next(model, &iter));
      }
   } else {
      GtkTreePath *path = gtk_tree_path_new_from_string("0");
      gtk_tree_selection_select_path(selection, path);
   }
   _lister->setSubView(subView);
   refreshTable(pkg, false);
   co_await setInterfaceLocked(FALSE);
   setBusyCursor(false);
   _blockActions = FALSE;
   setStatusText();
}

void RGMainWindow::refreshSubViewList()
{
   string selected = selectedSubView();
   if (_config->FindB("Debug::Synaptic::View", false))
      ioprintf(clog,
               "RGMainWindow::refreshSubViewList(): selectedView '%s'\n",
               selected.size() > 0 ? selected.c_str() : "(empty)");

   vector<string> subViews = _lister->getSubViews();

   for (unsigned int i = 0; i < subViews.size(); i++)
      subViews[i] = MarkupEscapeString(subViews[i]);

   gchar *str = g_strdup_printf("<b>%s</b>", _("All"));
   subViews.insert(subViews.begin(), str);
   g_free(str);
   setTreeList("treeview_subviews", subViews, true);

   if (!selected.empty()) {
      GtkTreeSelection *selection;
      GtkTreeModel *model;
      GtkTreeIter iter;
      const char *str = NULL;

      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(_subViewList));
      bool ok = gtk_tree_model_get_iter_first(model, &iter);
      while (ok) {
         gtk_tree_model_get(model, &iter, 0, &str, -1);
         if (str && strcoll(str, selected.c_str()) == 0) {
            gtk_tree_selection_select_iter(selection, &iter);
            return;
         }
         ok = gtk_tree_model_iter_next(model, &iter);
      }
   } else {
      GtkTreeModel *model;
      GtkTreeSelection *selection;
      GtkTreeIter iter;

      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(_subViewList));
      gtk_tree_model_get_iter_first(model, &iter);
      gtk_tree_selection_select_iter(selection, &iter);
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
   list = gtk_tree_selection_get_selected_rows(selection, &_pkgList);
   if (list == NULL) // Empty.
      return NULL;

   // We are only interested in the last element
   li = g_list_last(list);
   gtk_tree_model_get_iter(_pkgList, &iter, (GtkTreePath *)(li->data));

   gtk_tree_model_get(_pkgList, &iter, PKG_COLUMN, &pkg, -1);


   // free the list
   g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
   g_list_free(list);


   return pkg;
}

string RGMainWindow::selectedSubView()
{
   GtkTreeSelection *selection;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gchar *subView = NULL;
   static string ret = "(no selection)";

   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
   if (selection != NULL) {
      if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
         gtk_tree_model_get(model, &iter, 0, &subView, -1);

         // check if first item is selected ("All")
         gchar *str = gtk_tree_model_get_string_from_iter(model, &iter);
         if (str[0] == '0' || subView == NULL)
            ret = "";
         else
            ret = subView;
         g_free(str);
         g_free(subView);
      }
   }

   return ret;
}

task<bool> RGMainWindow::showErrors()
{
   co_return co_await _userDialog->showErrors();
}

void RGMainWindow::notifyChange(RPackage *pkg)
{
   if (_config->FindB("Debug::Synaptic::View", false))
      ioprintf(clog,
               "RGMainWindow::notifyChange(): '%s'\n",
               pkg != NULL ? pkg->name() : "(no pkg)");

   if (pkg != NULL)
      refreshTable(pkg);

   setStatusText();
}

void RGMainWindow::forgetNewPackages()
{
   int row = 0;
   while (row < _lister->viewPackagesSize()) {
      RPackage *elem = _lister->getViewPackage(row);
      if (elem->getFlags() & RPackage::FNew)
         elem->setNew(false);
   }
   _roptions->forgetNewPackages();
}

void RGMainWindow::refreshTable(RPackage *selectedPkg, bool setAdjustment)
{
   if (_config->FindB("Debug::Synaptic::View", false))
      ioprintf(clog,
               "RGMainWindow::refreshTable(): pkg: '%s' adjust '%i'\n",
               selectedPkg != NULL ? selectedPkg->name() : "(no pkg)",
               setAdjustment);

   const gchar *str = gtk_editable_get_text(GTK_EDITABLE(_entry_fast_search));
   if (str != NULL && strlen(str) > 1) {
      if (_config->FindB("Debug::Synaptic::View", false))
         cerr << "RGMainWindow::refreshTable: rerun limitBySearch" << endl;
      _lister->limitBySearch(str);
   }

   if (_pkgList == NULL) {
      _pkgList = GTK_TREE_MODEL(gtk_pkg_list_new(_lister));
      gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView),
                              GTK_TREE_MODEL(_pkgList));
   }

   // debian bug #747566
   gtk_widget_queue_draw(_treeView);

#if 0
   // set selected pkg to be selected again
   if(selectedPkg != NULL) {
      GtkTreeIter iter;
      RPackage *pkg;
      GtkTreePath *start = gtk_tree_path_new();
      // make sure we have the keyboard focus after the refresh
      gtk_widget_grab_focus(_treeView);

      bool ok = gtk_tree_view_get_visible_range(GTK_TREE_VIEW(_treeView), &start, NULL);
      // find and select the pkg we are looking for
      ok &=  gtk_tree_model_get_iter_first(_pkgList, &iter); 
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
      if (ok && gtk_tree_model_get_iter_first(_pkgList, &iter)) {
          gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(_treeView), start, NULL, true, 0.0, 0.0);
      }
      gtk_tree_path_free(start);
   }
#endif
   setStatusText();
}

void RGMainWindow::updatePackageInfo(RPackage *pkg)
{
   if (_blockActions)
      return;

   // cout << "RGMainWindow::updatePackageInfo(): " << pkg << endl;

   // get required widgets from gtkbuilder
   GtkWidget *pkginfo =
      GTK_WIDGET(gtk_builder_get_object(_builder, "notebook_pkginfo"));
   assert(pkginfo);

   // set everything to non-sensitive (for both pkg != NULL && pkg == NULL)
   setActionEnabled("unmark", false);
   setActionEnabled("mark-install", false);
   setActionEnabled("mark-reinstall", false);
   setActionEnabled("mark-upgrade", false);
   setActionEnabled("mark-delete", false);
   setActionEnabled("mark-purge", false);
   setActionEnabled("configure", false);
   setActionEnabled("browse-documentation", false);
   gtk_widget_set_sensitive(pkginfo, FALSE);
   setActionEnabled("download-changelog", false);
   setActionEnabled("package-properties", false);
   setActionEnabled("override-version", false);
   setActionEnabled("lock-version", false);
   setActionEnabled("auto-installed", false);
   gtk_text_buffer_set_text(
      _pkgCommonTextBuffer, _("No package is selected.\n"), -1);

   setStatusText();

   // return if no pkg is selected
   if (!pkg)
      return;

   //    cout <<   pkg->label() << endl;
   //    cout <<   pkg->component() << endl;
   //   cout << "trusted: " << pkg->isTrusted() << endl;

   // set menu according to pkg status
   int flags = pkg->getFlags();

   // changelog and properties are always visible
   setActionEnabled("download-changelog", true);
   setActionEnabled("package-properties", true);
   // activate for root only
   if (getuid() == 0) {
      setActionEnabled("lock-version", true);
      setActionEnabled("auto-installed", true);
   }

   // set info
   gtk_widget_set_sensitive(pkginfo, true);
   RGPkgDetailsWindow::fillInValues(this, pkg);
   // work around a stupid gtk-bug (see debian #279447)
   gtk_widget_queue_resize(
      GTK_WIDGET(gtk_builder_get_object(_builder, "viewport_pkginfo")));

   if (_pkgDetails != NULL)
      RGPkgDetailsWindow::fillInValues(_pkgDetails, pkg, true);

   // Pin, if a pin is set, we skip all other checks and return
   if (flags & RPackage::FPinned) {
      _blockActions = TRUE;
      setActionStateBool("lock-version", true);
      _blockActions = FALSE;
      return;
   } else {
      _blockActions = TRUE;
      setActionStateBool("lock-version", false);
      _blockActions = FALSE;
   }

   // Auto-Flag
   _blockActions = true;
   setActionStateBool("auto-installed", (flags & RPackage::FIsAuto));
   _blockActions = false;

   // enable unmark if a action is performed with the pkg
   if ((flags & RPackage::FInstall) || (flags & RPackage::FNewInstall) ||
       (flags & RPackage::FReInstall) || (flags & RPackage::FUpgrade) ||
       (flags & RPackage::FDowngrade) || (flags & RPackage::FRemove) ||
       (flags & RPackage::FPurge))
      setActionEnabled("unmark", true);
   // enable install if outdated or not insalled
   if (!(flags & RPackage::FInstalled) && !(flags & RPackage::FInstall))
      setActionEnabled("mark-install", true);
   // enable reinstall if installed and installable and not outdated
   if (flags & RPackage::FInstalled && !(flags & RPackage::FNotInstallable) &&
       !(flags & RPackage::FOutdated))
      setActionEnabled("mark-reinstall", true);
   // enable upgrade is outdated
   if ((flags & RPackage::FOutdated) && !(flags & RPackage::FInstall))
      setActionEnabled("mark-upgrade", true);
   // enable remove if package is installed
   if ((flags & RPackage::FInstalled) &&
       (!(flags & RPackage::FRemove) || (flags & RPackage::FPurge)))
      setActionEnabled("mark-delete", true);

   // enable purge if package is installed or has residual config
   if (((flags & RPackage::FInstalled) ||
        (flags & RPackage::FResidualConfig)) &&
       !(flags & RPackage::FPurge))
      setActionEnabled("mark-purge", true);
   // enable help if package is installed
   if (flags & RPackage::FInstalled)
      setActionEnabled("browse-documentation", true);
   // enable debconf if package is installed and depends on debconf
   if (flags & RPackage::FInstalled &&
       (pkg->dependsOn("debconf") || pkg->dependsOn("debconf-i18n")))
      setActionEnabled("mark-purge", true);

   if (pkg->getAvailableVersions().size() > 1)
      setActionEnabled("override-version", true);

   // Detect the most likely action
   if (isActionEnabled("unmark"))
      setActionStateInt("mark-default", PKG_KEEP);
   else if (isActionEnabled("mark-install") || isActionEnabled("mark-upgrade"))
      setActionStateInt("mark-default", PKG_INSTALL);
   else if (isActionEnabled("mark-delete"))
      setActionStateInt("mark-default", PKG_DELETE);
}

void RGMainWindow::cbDependsMenuChanged(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *)data;

   int nr = gtk_combo_box_get_active(GTK_COMBO_BOX(self));
   GtkWidget *notebook =
      GTK_WIDGET(gtk_builder_get_object(me->_builder, "notebook_dep_tab"));
   assert(notebook);
   gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), nr);
}

void RGMainWindow::cbMenuAutoInstalledClicked(GSimpleAction *action,
                                              GVariant *parameter,
                                              gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   if (me->_blockActions)
      return;

   // activating a stateful action without parameter type passes
   // parameter == NULL, so toggle the current state ourselves
   GVariant *state = g_action_get_state(G_ACTION(action));
   bool active = !g_variant_get_boolean(state);
   g_variant_unref(state);
   g_simple_action_set_state(action, g_variant_new_boolean(active));

   start_task(
      [me, active]() -> task<void> { co_await me->autoInstalled(active); });
}

task<void> RGMainWindow::autoInstalled(bool active)
{
   if (_blockActions)
      co_return;

   GtkTreeSelection *selection;
   GtkTreeIter iter;
   GList *list, *li;
   RPackage *pkg;

   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
   li = gtk_tree_selection_get_selected_rows(selection, &_pkgList);
   while (li != NULL) {
      gtk_tree_model_get_iter(_pkgList, &iter, (GtkTreePath *)(li->data));
      gtk_tree_model_get(_pkgList, &iter, PKG_COLUMN, &pkg, -1);
      if (pkg == NULL) {
         li = g_list_next(li);
         continue;
      }

      pkg->setAuto(active);
      li = g_list_next(li);
   }
   g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
   g_list_free(list);

   // write it
   GtkWidget *progress =
      GTK_WIDGET(gtk_builder_get_object(_builder, "progressbar_main"));
   GtkWidget *label =
      GTK_WIDGET(gtk_builder_get_object(_builder, "label_status"));
   RGCacheProgress cacheProgress(progress, label);
   _lister->getCache()->deps()->writeStateFile(&cacheProgress, true);

   // refresh
   co_await setInterfaceLocked(TRUE);
   _lister->unregisterObserver(this);

   _lister->getCache()->deps()->MarkAndSweep();
   _lister->refreshView();

   _lister->registerObserver(this);
   refreshTable();
   refreshSubViewList();
   co_await setInterfaceLocked(FALSE);
}

// install a specific version
void RGMainWindow::cbInstallFromVersion(GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer data)
{
   // cout << "RGMainWindow::cbInstallFromVersion()" << endl;

   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->installFromVersion(); });
}

// install a specific version
task<void> RGMainWindow::installFromVersion()
{
   RPackage *pkg = selectedPackage();
   if (pkg == NULL)
      co_return;

   RGGtkBuilderUserDialog dia(this, "change_version");

   GtkWidget *label =
      GTK_WIDGET(gtk_builder_get_object(dia.getGtkBuilder(), "label_text"));
   gchar *str_name = g_strdup_printf(
      _("Select the version of %s that should be forced for installation"),
      pkg->name());
   gchar *str = g_strdup_printf(
      "<big><b>%s</b></big>\n\n%s",
      str_name,
      _("The package manager always selects the most applicable version "
        "available. If you force a different version from the default one, "
        "errors in the dependency handling can occur."));
   gtk_label_set_markup(GTK_LABEL(label), str);
   g_free(str_name);
   g_free(str);

   GtkWidget *available_versions_combo = GTK_WIDGET(gtk_builder_get_object(
      dia.getGtkBuilder(), "combobox_available_versions"));
   int canidateNr = 0;
   vector<pair<string, string>> versions = pkg->getAvailableVersions();
   for (unsigned int i = 0; i < versions.size(); i++) {
      gchar *str = g_strdup_printf(
         "%s (%s)", versions[i].first.c_str(), versions[i].second.c_str());
      const char *verStr = pkg->availableVersion();
      if (verStr && versions[i].first == string(verStr))
         canidateNr = i;
      gtk_combo_box_text_append_text(
         GTK_COMBO_BOX_TEXT(available_versions_combo), str);
      // cout << "got: " << str << endl;
      g_free(str);
   }
   gtk_combo_box_set_active(GTK_COMBO_BOX(available_versions_combo),
                            canidateNr);
   if (!co_await dia.co_run()) {
      // cout << "cancel" << endl;
      co_return; // user clicked cancel
   }

   int nr = gtk_combo_box_get_active(GTK_COMBO_BOX(available_versions_combo));

   pkg->setNotify(false);
   // nr-1 here as we add a "do not override" to the option menu
   pkg->setVersion(versions[nr].first.c_str());
   co_await pkgAction(PKG_INSTALL_FROM_VERSION);


   if (!(pkg->getFlags() & RPackage::FInstall))
      pkg->unsetVersion(); // something went wrong

   pkg->setNotify(true);
}

task<bool> RGMainWindow::askStateChange(RPackageLister::pkgState state,
                                        const vector<RPackage *> &exclude)
{
   vector<RPackage *> toKeep;
   vector<RPackage *> toInstall;
   vector<RPackage *> toReInstall;
   vector<RPackage *> toUpgrade;
   vector<RPackage *> toDowngrade;
   vector<RPackage *> toRemove;
   vector<RPackage *> notAuthenticated;

   bool ask = _config->FindB("Synaptic::AskRelated", true);

   // ask if the user really want this changes
   bool changed = true;
   if (ask && _lister->getStateChanges(state,
                                       toKeep,
                                       toInstall,
                                       toReInstall,
                                       toUpgrade,
                                       toRemove,
                                       toDowngrade,
                                       notAuthenticated,
                                       exclude)) {
      RGChangesWindow changes(this);
      changes.confirm(_lister,
                      toKeep,
                      toInstall,
                      toReInstall,
                      toUpgrade,
                      toRemove,
                      toDowngrade,
                      notAuthenticated);
      int res = co_await co_run_dialog(GTK_DIALOG(changes.window()));
      if (res != GTK_RESPONSE_OK) {
         // canceled operation
         _lister->restoreState(state);
         // if a operation was canceled, we discard all errors from this
         // operation too
         _error->Discard();
         changed = false;
      }
   }

   co_return changed;
}

task<void> RGMainWindow::pkgAction(RGPkgAction action)
{
   GtkTreeSelection *selection;
   GtkTreeIter iter;
   GList *li, *list;

   co_await setInterfaceLocked(TRUE);
   _blockActions = TRUE;

   // get list of selected pkgs
   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
   list = li = gtk_tree_selection_get_selected_rows(selection, &_pkgList);

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
   int flags;

   while (li != NULL) {
      pkgDepCache::ActionGroup group(*_lister->getCache()->deps());
      gtk_tree_model_get_iter(_pkgList, &iter, (GtkTreePath *)(li->data));
      gtk_tree_model_get(_pkgList, &iter, PKG_COLUMN, &pkg, -1);
      li = g_list_next(li);
      if (pkg == NULL)
         continue;

      flags = pkg->getFlags();

      pkg->setNotify(false);

      // needed for the stateChange
      exclude.push_back(pkg);
      switch (action) {
         case PKG_KEEP: // keep
            pkgKeepHelper(pkg);
            break;
         case PKG_INSTALL: // install
            // install only if not installed or outdated (upgrade)
            if (!(flags & RPackage::FInstalled) ||
                (flags & RPackage::FOutdated)) {
               instPkgs.push_back(pkg);
               pkgInstallHelper(pkg, false);
            }
            break;
         case PKG_INSTALL_FROM_VERSION: // install with specific version
            pkgInstallHelper(pkg, false);
            break;
         case PKG_REINSTALL: // reinstall
            // Only reinstall installable packages and non outdated packages
            if (flags & RPackage::FInstalled &&
                !(flags & RPackage::FNotInstallable) &&
                !(flags & RPackage::FOutdated)) {
               instPkgs.push_back(pkg);
               pkgInstallHelper(pkg, false, true);
            }
            break;
         case PKG_DELETE: // delete
            if (flags & RPackage::FInstalled)
               co_await pkgRemoveHelper(pkg);
            break;
         case PKG_PURGE: // purge
            if (flags & RPackage::FInstalled ||
                flags & RPackage::FResidualConfig)
               co_await pkgRemoveHelper(pkg, true);
            break;
         case PKG_DELETE_WITH_DEPS:
            if (flags & RPackage::FInstalled ||
                flags & RPackage::FResidualConfig)
               co_await pkgRemoveHelper(pkg, true, true);
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

   bool changed = co_await askStateChange(state, exclude);

   if (changed) {
      bool failed = false;
      // check for failed installs, if a installs fails, restore old state
      // as the Fixer may do wired thinks when trying to resolve the problem
      if (action == PKG_INSTALL) {
         failed = co_await checkForFailedInst(instPkgs);
         if (failed)
            _lister->restoreState(state);
      }
      // if everything is fine, save it as new undo state
      if (!failed)
         _lister->saveUndoState(state);
   }

   if (ask)
      _lister->registerObserver(this);

   g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
   g_list_free(list);

   refreshSubViewList();
   _blockActions = FALSE;
   co_await setInterfaceLocked(FALSE);
   refreshTable(pkg);
}

task<bool> RGMainWindow::checkForFailedInst(vector<RPackage *> instPkgs)
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
      RGGtkBuilderUserDialog dia(this, "unmet");
      GtkWidget *tv =
         GTK_WIDGET(gtk_builder_get_object(dia.getGtkBuilder(), "textview"));
      GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
      gtk_text_buffer_set_text(tb, utf8(failedReason.c_str()), -1);
      co_await dia.co_run();
      // we informaed the user about the problem, we can clear the
      // apt error stack
      // CHECKME: is this discard here really needed?
      _error->Discard();
   }

   co_return failed;
}

static void setActionShortcut(RGMainWindow *me,
                              GtkShortcutController *shortcuts,
                              const char *action_name,
                              guint key,
                              GdkModifierType mods)
{
   gtk_shortcut_controller_add_shortcut(
      shortcuts,
      gtk_shortcut_new(
         gtk_keyval_trigger_new(key, mods),
         gtk_named_action_new((std::string("win.") + action_name).c_str())));
}

RGMainWindow::RGMainWindow(GtkApplication *app,
                           RPackageLister *packLister,
                           string name)
   : RGGtkBuilderWindow(NULL, name), _lister(packLister), _pkgList(0),
     _treeView(0), _tasksWin(0), _iconLegendPanel(0), _pkgDetails(0),
     _logView(0), _fetchProgress(0), _installProgress(0), _fastSearchEventID(-1)
{
   assert(_win);
   gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(_win));

   const GActionEntry entries[] = {
      {"read-markings", cbOpenClicked},
      {"save-markings", cbSaveClicked},
      {"save-markings-as", cbSaveAsClicked},
      {"generate-download-script", cbGenerateDownloadScriptClicked},
      {"add-downloaded-packages", cbAddDownloadedFilesClicked},
      {"view-commit-log", cbViewLogClicked},
      {"quit", closeWin},

      {"undo", cbUndoClicked},
      {"redo", cbRedoClicked},
      {"unmark-all", cbClearAllChangesClicked},
      {"search", cbFindToolClicked},
      {"reload", cbUpdateClicked},
      {"add-cdrom", cbAddCDROM},
      {"mark-all-upgrades", cbUpgradeClicked},
      {"fix-broken-packages", cbFixBrokenClicked},
      {"mark-packages-by-task", cbTasksClicked},
      {"apply", cbProceedClicked},

      {"unmark", cbPkgActionUnmark},
      {"mark-install", cbPkgActionMarkInstall},
      {"mark-reinstall", cbPkgActionMarkReinstall},
      {"mark-upgrade", cbPkgActionMarkUpgrade},
      {"mark-delete", cbPkgActionMarkDelete},
      {"mark-purge", cbPkgActionMarkPurge},
      {"mark-default", cbPkgActionDefault, nullptr, "int32 0"},

      {"lock-version", cbMenuPinClicked, nullptr, "boolean false"},
      {"auto-installed", cbMenuAutoInstalledClicked, nullptr, "boolean false"},
      {"override-version", cbInstallFromVersion},
      {"configure", cbPkgReconfigureClicked},
      {"browse-documentation", cbPkgHelpClicked},
      {"download-changelog", cbChangelogDialog},
      {"package-properties", cbDetailsWindow},
      {"install-by-name", pkgInstallByNameHelper, "s"},

      {"preferences", cbShowConfigWindow},
      {"repositories", cbShowSourcesWindow},
      {"filters", cbShowFilterManagerWindow},
      {"set-internal-option", cbShowSetOptWindow},
      {"toolbar-style", cbMenuToolbarClicked, "s", "string 'hide'"},

      {"help", cbHelpAction},
      {"quick-intro", cbShowWelcomeDialog},
      {"icon-legend", cbShowIconLegendPanel},
      {"about", cbShowAboutPanel}};

   win_actions = G_ACTION_GROUP(g_simple_action_group_new());
   g_action_map_add_action_entries(
      G_ACTION_MAP(win_actions), entries, G_N_ELEMENTS(entries), this);
   gtk_widget_insert_action_group(
      GTK_WIDGET(_win), "win", G_ACTION_GROUP(win_actions));

   GtkShortcutController *shortcuts = getShortcutController();
   setActionShortcut(this, shortcuts, "quit", GDK_KEY_Q, GDK_CONTROL_MASK);
   setActionShortcut(this, shortcuts, "undo", GDK_KEY_Z, GDK_CONTROL_MASK);
   setActionShortcut(this, shortcuts, "redo", GDK_KEY_Z, GDK_SHIFT_MASK);
   setActionShortcut(this, shortcuts, "search", GDK_KEY_F, GDK_CONTROL_MASK);
   setActionShortcut(this, shortcuts, "reload", GDK_KEY_R, GDK_CONTROL_MASK);
   setActionShortcut(
      this, shortcuts, "mark-all-upgrades", GDK_KEY_G, GDK_CONTROL_MASK);
   setActionShortcut(this, shortcuts, "apply", GDK_KEY_P, GDK_CONTROL_MASK);
   setActionShortcut(this, shortcuts, "unmark", GDK_KEY_N, GDK_CONTROL_MASK);
   setActionShortcut(
      this, shortcuts, "mark-install", GDK_KEY_I, GDK_CONTROL_MASK);
   setActionShortcut(
      this, shortcuts, "mark-upgrade", GDK_KEY_U, GDK_CONTROL_MASK);
   setActionShortcut(
      this, shortcuts, "mark-delete", GDK_KEY_Delete, (GdkModifierType)0);
   setActionShortcut(
      this, shortcuts, "mark-purge", GDK_KEY_Delete, GDK_SHIFT_MASK);
   setActionShortcut(
      this, shortcuts, "override-version", GDK_KEY_E, GDK_CONTROL_MASK);
   setActionShortcut(
      this, shortcuts, "download-changelog", GDK_KEY_L, GDK_CONTROL_MASK);
   setActionShortcut(
      this, shortcuts, "package-properties", GDK_KEY_Return, GDK_ALT_MASK);
   setActionShortcut(this, shortcuts, "help", GDK_KEY_F1, (GdkModifierType)0);

   _blockActions = false;
   _unsavedChanges = false;
   _interfaceLocked = 0;

   _lister->registerObserver(this);

   _toolbarStyle = (RGToolbarStyle)_config->FindI("Synaptic::ToolbarState",
                                                  (int)RG_TOOLBAR_BOTH);

   // create all the interface stuff
   buildInterface();
   _userDialog = new RGUserDialog(this);

   packLister->setUserDialog(_userDialog);

   // build the progress stuff
   GtkWidget *progress, *label;
   progress = GTK_WIDGET(gtk_builder_get_object(_builder, "progressbar_main"));
   assert(progress);
   label = GTK_WIDGET(gtk_builder_get_object(_builder, "label_status"));
   assert(label);
   RGCacheProgress *_cacheProgress;
   _cacheProgress = new RGCacheProgress(progress, label);
   assert(_cacheProgress);
   packLister->setProgressMeter(_cacheProgress);

   // defaults for the various windows
   _findWin = NULL;
   _setOptWin = NULL;
   _sourcesWin = NULL;
   _configWin = NULL;
   _aboutPanel = NULL;
   _fmanagerWin = NULL;

   GValue value = {
      0,
   };
   g_value_init(&value, G_TYPE_STRING);
   g_object_get_property(
      G_OBJECT(gtk_settings_get_default()), "gtk-font-name", &value);
   _config->Set("Volatile::orginalFontName", g_value_get_string(&value));
   if (_config->FindB("Synaptic::useUserFont")) {
      g_value_set_string(&value, _config->Find("Synaptic::FontName").c_str());
      g_object_set_property(
         G_OBJECT(gtk_settings_get_default()), "gtk-font-name", &value);
   }
   g_value_unset(&value);

   xapianDoIndexUpdate(this);

   // apply the proxy settings
   RGPreferencesWindow::applyProxySettings();
}

#ifdef HAVE_XAPIAN
gboolean RGMainWindow::xapianDoIndexUpdate(void *data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   if (_config->FindB("Debug::Synaptic::Xapian", false))
      std::cerr << "xapianDoIndexUpdate()" << std::endl;

   // no need to update if we run non-interactive
   if (_config->FindB("Volatile::Non-Interactive", false) == true)
      return false;

   // check if we need a update
   if (!me->_lister->xapianIndexNeedsUpdate()) {
      // if the cache is not open, check back when it is
      if (me->_lister->packagesSize() == 0)
         g_timeout_add_seconds(30, xapianDoIndexUpdate, me);
      return false;
   }

   // do not run if we don't have it
   if (!FileExists("/usr/sbin/update-apt-xapian-index"))
      return false;
   // no permission
   if (getuid() != 0)
      return false;

   // if we make it to this point, we need a xapian update
   if (_config->FindB("Debug::Synaptic::Xapian", false))
      std::cerr << "running update-apt-xapian-index" << std::endl;
   GPid pid;
   const char *argp[] = {"/usr/bin/nice",
                         "/usr/bin/ionice",
                         "-c3",
                         "/usr/sbin/update-apt-xapian-index",
                         "--update",
                         "-q",
                         NULL};
   if (g_spawn_async(NULL,
                     const_cast<char **>(argp),
                     NULL,
                     (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD),
                     NULL,
                     NULL,
                     &pid,
                     NULL)) {
      g_child_watch_add(pid, (GChildWatchFunc)xapianIndexUpdateFinished, me);
      gtk_label_set_text(
         GTK_LABEL(gtk_builder_get_object(me->_builder, "label_fast_search")),
         _("Rebuilding search index"));
      gtk_widget_set_sensitive(
         GTK_WIDGET(gtk_builder_get_object(me->_builder, "toolbar_filter")),
         FALSE);
   }
   return false;
}
#else
gboolean RGMainWindow::xapianDoIndexUpdate(void *data)
{
   return false;
}
#endif

void RGMainWindow::xapianIndexUpdateFinished(GPid pid, gint status, void *data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   if (_config->FindB("Debug::Synaptic::Xapian", false))
      std::cerr << "xapianIndexUpdateFinished: " << WEXITSTATUS(status)
                << std::endl;
#ifdef HAVE_XAPIAN
   me->_lister->openXapianIndex();
#endif
   gtk_label_set_text(
      GTK_LABEL(gtk_builder_get_object(me->_builder, "label_fast_search")),
      _("Quick filter"));
   gtk_widget_set_sensitive(
      GTK_WIDGET(gtk_builder_get_object(me->_builder, "toolbar_filter")), TRUE);
   g_spawn_close_pid(pid);
}

void RGMainWindow::buildTreeView()
{
   // remove old tree columns
   if (_treeView) {
      // unset model fist, otherwise the _remove_column takes *ages*
      // (within the seconds range for each call)
      gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), NULL);
      GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(_treeView));
      for (GList *li = g_list_first(columns); li != NULL;
           li = g_list_next(li)) {
         gtk_tree_view_remove_column(GTK_TREE_VIEW(_treeView),
                                     GTK_TREE_VIEW_COLUMN(li->data));
      }
      // need to free the list here
      g_list_free(columns);
   }

   _treeView =
      GTK_WIDGET(gtk_builder_get_object(_builder, "treeview_packages"));
   assert(_treeView);
   setupTreeView(_treeView);
   _pkgList = GTK_TREE_MODEL(gtk_pkg_list_new(_lister));
   gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), _pkgList);
}

void RGMainWindow::buildInterface()
{
   GtkWidget *widget;

   // here is a pointer to rgmainwindow for every widget that needs it
   g_object_set_data(G_OBJECT(_win), "me", this);

   gtk_window_set_icon_name(GTK_WINDOW(_win), "synaptic");

   gtk_window_set_default_size(GTK_WINDOW(_win),
                               _config->FindI("Synaptic::windowWidth", 640),
                               _config->FindI("Synaptic::windowHeight", 480));

   if (_config->FindB("Synaptic::Maximized", false))
      gtk_window_maximize(GTK_WINDOW(_win));

   if (_fastSearchCssProvider == NULL) {
      _fastSearchCssProvider = gtk_css_provider_new();
      gtk_css_provider_load_from_data(
         _fastSearchCssProvider,
         "GtkEntry:not(:selected) { background: #F7F7BE; }",
         -1);
   }

   if (getuid() != 0) {
      gtk_widget_show(
         GTK_WIDGET(gtk_builder_get_object(_builder, "no_root_info")));
   }

   g_signal_connect(gtk_builder_get_object(_builder, "entry_fast_search"),
                    "changed",
                    G_CALLBACK(cbSearchEntryChanged),
                    this);

   if (_config->FindB("Synaptic::NoUpgradeButtons", false) == true) {
      setActionEnabled("mark-all-upgrades", false); // TODO: hide?
      widget =
         GTK_WIDGET(gtk_builder_get_object(_builder, "alignment_upgrade"));
      gtk_widget_hide(widget);
   }

#ifdef HAVE_RPM
   // TODO: hide?
   setActionEnabled("mark-purge", false);
   setActionEnabled("configure", false);
   setActionEnabled("browse-documentation", false);
   setActionEnabled("download-changelog", false);
#endif

   if (!FileExists(
          _config->Find("Synaptic::taskHelperProg", "/usr/bin/tasksel")))
      setActionEnabled("mark-packages-by-task", false); // TODO: hide?

   GtkWidget *pkgCommonTextView;
   pkgCommonTextView =
      GTK_WIDGET(gtk_builder_get_object(_builder, "text_descr"));
   assert(pkgCommonTextView);
   _pkgCommonTextBuffer =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(pkgCommonTextView));

   // only if pkg help is enabled
#ifndef SYNAPTIC_PKG_HOLD
   setActionEnabled("lock-version", false); // TODO: hide?
#endif

   // soc
   GtkWidget *notebook =
      GTK_WIDGET(gtk_builder_get_object(_builder, "notebook_pkginfo"));
   if (_config->FindB("Synaptic::ShowAllPkgInfoInMain", false)) {
      gtk_widget_set_margin_top(notebook, 6);
      gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), TRUE);
      gtk_widget_hide(
         GTK_WIDGET(gtk_builder_get_object(_builder, "button_details")));
   } else {
      gtk_widget_set_margin_top(notebook, 0);
      gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
      gtk_widget_show(
         GTK_WIDGET(gtk_builder_get_object(_builder, "button_details")));
   }
#ifndef HAVE_RPM
   gtk_widget_show(
      GTK_WIDGET(gtk_builder_get_object(_builder, "scrolledwindow_filelist")));
#endif

   // Handle the combobox stuff for dependencies in PpkgInfoInMain mode
   // ourselves
   GtkWidget *comboDepends =
      GTK_WIDGET(gtk_builder_get_object(_builder, "combobox_depends"));
   g_signal_connect(G_OBJECT(comboDepends),
                    "changed",
                    G_CALLBACK(cbDependsMenuChanged),
                    this);
   GtkListStore *relTypes = gtk_list_store_new(1, G_TYPE_STRING);
   GtkTreeIter relIter;
   for (int i = 0; relOptions[i] != NULL; i++) {
      gtk_list_store_append(relTypes, &relIter);
      gtk_list_store_set(relTypes, &relIter, 0, _(relOptions[i]), -1);
   }
   gtk_combo_box_set_model(GTK_COMBO_BOX(comboDepends),
                           GTK_TREE_MODEL(relTypes));
   GtkCellRenderer *relRenderText = gtk_cell_renderer_text_new();
   gtk_cell_layout_clear(GTK_CELL_LAYOUT(comboDepends));
   gtk_cell_layout_pack_start(
      GTK_CELL_LAYOUT(comboDepends), relRenderText, FALSE);
   gtk_cell_layout_add_attribute(
      GTK_CELL_LAYOUT(comboDepends), relRenderText, "text", 0);
   gtk_combo_box_set_active(GTK_COMBO_BOX(comboDepends), 0);

   GtkWidget *vpaned =
      GTK_WIDGET(gtk_builder_get_object(_builder, "vpaned_main"));
   assert(vpaned);
   GtkWidget *hpaned =
      GTK_WIDGET(gtk_builder_get_object(_builder, "hpaned_main"));
   assert(hpaned);
   // If the pane position is restored before the window is shown, it's
   // not restored in the same place as it was.
   if (!_config->FindB("Volatile::HideMainwindow", false))
      show();

   gtk_paned_set_position(GTK_PANED(vpaned),
                          _config->FindI("Synaptic::vpanedPos", 140));
   gtk_paned_set_position(GTK_PANED(hpaned),
                          _config->FindI("Synaptic::hpanedPos", 200));


   // build the treeview
   buildTreeView();

   GtkGesture *click_controller = gtk_gesture_click_new();
   gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_controller), 0);
   g_signal_connect(
      click_controller, "pressed", (GCallback)cbPackageListClicked, this);
   gtk_widget_add_controller(GTK_WIDGET(_treeView),
                             GTK_EVENT_CONTROLLER(click_controller));

   GtkTreeSelection *select;
   select = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
   // gtk_tree_selection_set_mode (select, GTK_SELECTION_MULTIPLE);
   g_signal_connect(
      G_OBJECT(select), "changed", G_CALLBACK(cbSelectedRow), this);
   g_signal_connect(G_OBJECT(_treeView),
                    "row-activated",
                    G_CALLBACK(cbPackageListRowActivated),
                    this);

   /* --------------------------------------------------------------- */

   // toolbar menu code
   if (_toolbarStyle == RG_TOOLBAR_ICONS)
      activateAction("toolbar-style", g_variant_new_string("icons-only"));
   else if (_toolbarStyle == RG_TOOLBAR_TEXT)
      activateAction("toolbar-style", g_variant_new_string("text-only"));
   else if (_toolbarStyle == RG_TOOLBAR_BOTH)
      activateAction("toolbar-style", g_variant_new_string("below"));
   else if (_toolbarStyle == RG_TOOLBAR_BOTH_HORIZ)
      activateAction("toolbar-style", g_variant_new_string("beside"));
   else
      activateAction("toolbar-style", g_variant_new_string("hide"));

   // FIXME/MAYBE: create this dynmaic?!?
   //     for (vector<string>::const_iterator I = views.begin();
   //  I != views.end(); I++) {
   //  item = gtk_radiobutton_new((char *)(*I).c_str());
   GtkWidget *w;

   // section
   w = _viewButtons[PACKAGE_VIEW_SECTION] =
      GTK_WIDGET(gtk_builder_get_object(_builder, "radiobutton_sections"));
   g_object_set_data(
      G_OBJECT(w), "index", GINT_TO_POINTER(PACKAGE_VIEW_SECTION));
   g_signal_connect(G_OBJECT(w), "toggled", (GCallback)cbChangedView, this);
   // status
   w = _viewButtons[PACKAGE_VIEW_STATUS] =
      GTK_WIDGET(gtk_builder_get_object(_builder, "radiobutton_status"));
   g_object_set_data(
      G_OBJECT(w), "index", GINT_TO_POINTER(PACKAGE_VIEW_STATUS));
   g_signal_connect(G_OBJECT(w), "toggled", (GCallback)cbChangedView, this);
   // origin
   w = _viewButtons[PACKAGE_VIEW_ORIGIN] =
      GTK_WIDGET(gtk_builder_get_object(_builder, "radiobutton_origin"));
   g_object_set_data(
      G_OBJECT(w), "index", GINT_TO_POINTER(PACKAGE_VIEW_ORIGIN));
   g_signal_connect(G_OBJECT(w), "toggled", (GCallback)cbChangedView, this);
   // custom
   w = _viewButtons[PACKAGE_VIEW_CUSTOM] =
      GTK_WIDGET(gtk_builder_get_object(_builder, "radiobutton_custom"));
   g_object_set_data(
      G_OBJECT(w), "index", GINT_TO_POINTER(PACKAGE_VIEW_CUSTOM));
   g_signal_connect(G_OBJECT(w), "toggled", (GCallback)cbChangedView, this);
   // find
   w = _viewButtons[PACKAGE_VIEW_SEARCH] =
      GTK_WIDGET(gtk_builder_get_object(_builder, "radiobutton_find"));
   g_object_set_data(
      G_OBJECT(w), "index", GINT_TO_POINTER(PACKAGE_VIEW_SEARCH));
   g_signal_connect(G_OBJECT(w), "toggled", (GCallback)cbChangedView, this);

   // architecture
   w = _viewButtons[PACKAGE_VIEW_ARCHITECTURE] =
      GTK_WIDGET(gtk_builder_get_object(_builder, "radiobutton_architecture"));
   g_object_set_data(
      G_OBJECT(w), "index", GINT_TO_POINTER(PACKAGE_VIEW_ARCHITECTURE));
   g_signal_connect(G_OBJECT(w), "toggled", (GCallback)cbChangedView, this);

   _subViewList =
      GTK_WIDGET(gtk_builder_get_object(_builder, "treeview_subviews"));
   assert(_subViewList);
   setTreeList("treeview_subviews", vector<string>(), true);
   // Setup the selection handler
   select = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
   gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
   g_signal_connect(
      G_OBJECT(select), "changed", G_CALLBACK(cbChangedSubView), this);

   //   GtkBindingSet *binding_set = gtk_binding_set_find("GtkTreeView");
   //   gtk_binding_entry_add_signal(binding_set, GDK_KEY_s, GDK_CONTROL_MASK,
   //				"start_interactive_search", 0);

   _entry_fast_search =
      GTK_WIDGET(gtk_builder_get_object(_builder, "entry_fast_search"));

   // only enable fast search if its usable
#ifdef HAVE_XAPIAN
   if (!FileExists("/usr/sbin/update-apt-xapian-index"))
#endif
   {
      gtk_widget_hide(
         GTK_WIDGET(gtk_builder_get_object(_builder, "toolbar_filter")));
   }

   // stuff for the non-root mode
   if (getuid() != 0) {
      setActionEnabled("apply", false);
      setActionEnabled("add-downloaded-packages", false);
      setActionEnabled("repositories", false);
      setActionEnabled("view-commit-log", false);
      setActionEnabled("reload", false);
      setActionEnabled("add-cdrom", false);
      setActionEnabled("lock-version", false);
   }
}

bool RGMainWindow::isActionEnabled(const char *action_name)
{
   return g_action_group_get_action_enabled(win_actions, action_name);
}

void RGMainWindow::setActionEnabled(const char *action_name, bool enabled)
{
   GAction *action =
      g_action_map_lookup_action(G_ACTION_MAP(win_actions), action_name);
   g_simple_action_set_enabled(G_SIMPLE_ACTION(action), enabled);
}

void RGMainWindow::setActionState(const char *action_name, GVariant *value)
{
   GAction *action =
      g_action_map_lookup_action(G_ACTION_MAP(win_actions), action_name);
   g_simple_action_set_state(G_SIMPLE_ACTION(action), value);
}

void RGMainWindow::activateAction(const char *action_name, GVariant *value)
{
   g_action_group_activate_action(win_actions, action_name, value);
}

void RGMainWindow::pkgInstallHelper(RPackage *pkg,
                                    bool fixBroken,
                                    bool reInstall)
{
   if (pkg->availableVersion() != NULL)
      pkg->setInstall();

   if (reInstall == true)
      pkg->setReInstall(true);

   // check whether something broke
   if (fixBroken && !_lister->check())
      _lister->fixBroken();
}

task<void> RGMainWindow::pkgRemoveHelper(RPackage *pkg,
                                         bool purge,
                                         bool withDeps)
{
   if (pkg->getFlags() & RPackage::FImportant) {
      gchar *warning =
         g_strdup_printf(_("Removing package \"%s\" may render the "
                           "system unusable.\n"
                           "Are you sure you want to do that?"),
                         pkg->name());
      bool confirmed = co_await _userDialog->confirm(warning, false);
      g_free(warning);
      if (!confirmed) {
         co_return;
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
   int toInstall, toRemove;
   double size;


   GtkWidget *_statusL =
      GTK_WIDGET(gtk_builder_get_object(_builder, "label_status"));
   assert(_statusL);

   _lister->getStats(installed, broken, toInstall, toRemove, size);

   if (text) {
      gtk_label_set_text(GTK_LABEL(_statusL), text);
   } else {
      gchar *buffer;
      // we need to make this two strings for i18n reasons
      listed = _lister->viewPackagesSize();
      if (size < 0) {
         buffer = g_strdup_printf(
            _("%i packages listed, %i installed, %i broken. %i to "
              "install/upgrade, %i to remove; %s will be freed"),
            listed,
            installed,
            broken,
            toInstall,
            toRemove,
            SizeToStr(fabs(size)).c_str());
      } else if (size > 0) {
         buffer = g_strdup_printf(
            _("%i packages listed, %i installed, %i broken. %i to "
              "install/upgrade, %i to remove; %s will be used"),
            listed,
            installed,
            broken,
            toInstall,
            toRemove,
            SizeToStr(fabs(size)).c_str());
      } else {
         buffer =
            g_strdup_printf(_("%i packages listed, %i installed, %i broken. %i "
                              "to install/upgrade, %i to remove"),
                            listed,
                            installed,
                            broken,
                            toInstall,
                            toRemove);
      }
      gtk_label_set_text(GTK_LABEL(_statusL), buffer);
      g_free(buffer);
   }

   setActionEnabled("mark-all-upgrades", _lister->upgradable());

   if (getuid() == 0) {
      setActionEnabled("apply", (toInstall + toRemove) != 0);
   }
   _unsavedChanges = ((toInstall + toRemove) != 0);

   gtk_widget_queue_draw(_statusL);
}


task<void> RGMainWindow::saveState()
{
   if (_config->FindB("Volatile::NoStateSaving", false) == true)
      co_return;

   GtkWidget *vpaned =
      GTK_WIDGET(gtk_builder_get_object(_builder, "vpaned_main"));
   GtkWidget *hpaned =
      GTK_WIDGET(gtk_builder_get_object(_builder, "hpaned_main"));
   _config->Set("Synaptic::vpanedPos",
                gtk_paned_get_position(GTK_PANED(vpaned)));
   _config->Set("Synaptic::hpanedPos",
                gtk_paned_get_position(GTK_PANED(hpaned)));

   _config->Set("Synaptic::windowWidth", gtk_widget_get_width(_win));
   _config->Set("Synaptic::windowHeight", gtk_widget_get_height(_win));
   _config->Set("Synaptic::ToolbarState", (int)_toolbarStyle);
   _config->Set("Synaptic::Maximized",
                gtk_window_is_maximized(GTK_WINDOW(_win)));

   if (!RWriteConfigFile(*_config)) {
      _error->Error(_("An error occurred while saving configurations."));
      co_await _userDialog->showErrors();
   }
   if (!_roptions->store())
      cerr << "Internal Error: error storing raptoptions" << endl;
}

task<bool> RGMainWindow::restoreState()
{

   // see if we have broken packages (might be better in some
   // RGMainWindow::preGuiStart funktion)
   int installed, broken, toInstall, toRemove;
   double sizeChange;
   _lister->getStats(installed, broken, toInstall, toRemove, sizeChange);
   if (broken > 0) {
      gchar *msg;
      msg = ngettext("You have %d broken package on your system!\n\n"
                     "Use the \"Broken\" filter to locate it.",
                     "You have %i broken packages on your system!\n\n"
                     "Use the \"Broken\" filter to locate them.",
                     broken);
      msg = g_strdup_printf(msg, broken);
      co_await _userDialog->warning(msg);
      g_free(msg);
   }

   if (!_config->FindB("Volatile::Upgrade-Mode", false)) {
      int viewNr = _config->FindI("Synaptic::ViewMode", 0);
      co_await changeView(viewNr);

      // we auto set to "All" on startup when we have gtk2.4 (without
      // the list is too slow)
      GtkTreeModel *model;
      GtkTreeSelection *selection;
      GtkTreeIter iter;

      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(_subViewList));
      gtk_tree_model_get_iter_first(model, &iter);
      gtk_tree_selection_select_iter(selection, &iter);
   }
   updatePackageInfo(NULL);
   co_return true;
}


void RGMainWindow::close()
{
   if (_interfaceLocked > 0)
      return;

   start_task([this]() -> task<void> {
      RGGtkBuilderUserDialog dia(this);
      if (_unsavedChanges == false || co_await dia.co_run("quit")) {
         _error->Discard();
         co_await saveState();
         co_await showErrors();
         exit(0);
      }
   });
}


task<void> RGMainWindow::setInterfaceLocked(bool flag)
{
   if (flag) {
      _interfaceLocked++;
      if (_interfaceLocked > 1)
         co_return;

      gtk_widget_set_sensitive(_win, FALSE);
      if (gtk_widget_get_visible(_win))
         gtk_widget_set_cursor_from_name(GTK_WIDGET(_win), "watch");
   } else {
      assert(_interfaceLocked > 0);

      _interfaceLocked--;
      if (_interfaceLocked > 0)
         co_return;

      gtk_widget_set_sensitive(_win, TRUE);
      if (gtk_widget_get_visible(_win))
         gtk_widget_set_cursor_from_name(GTK_WIDGET(_win), NULL);
   }

   // fast enough with the new fixed-height mode
   co_await RGFlushInterface();
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

void RGMainWindow::cbPkgAction(RGPkgAction action)
{
   // Ignore DEL accelerator when fastsearch has focus
   GtkWidget *entry =
      GTK_WIDGET(gtk_builder_get_object(_builder, "entry_fast_search"));
   if (gtk_widget_has_focus(entry) && action == PKG_DELETE) {
      return;
   }
   start_task([this, action]() -> task<void> { co_await pkgAction(action); });
}

void RGMainWindow::cbPkgActionUnmark(GSimpleAction *action,
                                     GVariant *parameter,
                                     gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   assert(me);
   me->cbPkgAction(PKG_KEEP);
}

void RGMainWindow::cbPkgActionMarkInstall(GSimpleAction *action,
                                          GVariant *parameter,
                                          gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   assert(me);
   me->cbPkgAction(PKG_INSTALL);
}

void RGMainWindow::cbPkgActionMarkReinstall(GSimpleAction *action,
                                            GVariant *parameter,
                                            gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   assert(me);
   me->cbPkgAction(PKG_REINSTALL);
}

void RGMainWindow::cbPkgActionMarkUpgrade(GSimpleAction *action,
                                          GVariant *parameter,
                                          gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   assert(me);
   // callback same as for install
   me->cbPkgAction(PKG_INSTALL);
}

void RGMainWindow::cbPkgActionMarkDelete(GSimpleAction *action,
                                         GVariant *parameter,
                                         gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   assert(me);
   me->cbPkgAction(PKG_DELETE);
}

void RGMainWindow::cbPkgActionMarkPurge(GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   assert(me);
   me->cbPkgAction(PKG_PURGE);
}

void RGMainWindow::cbPkgActionDefault(GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   assert(me);
   RGPkgAction pkgAction = (RGPkgAction)g_variant_get_int32(parameter);
   me->cbPkgAction(pkgAction);
}


void RGMainWindow::cbPackageListClicked(GtkGestureClick *gesture,
                                        int n_press,
                                        double x,
                                        double y,
                                        gpointer data)
{
   // cout << "RGMainWindow::cbPackageListClicked()" << endl;

   int button =
      gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
   GdkModifierType state = gdk_event_get_modifier_state(
      gtk_event_controller_get_current_event(GTK_EVENT_CONTROLLER(gesture)));

   RGMainWindow *me = (RGMainWindow *)data;
   RPackage *pkg = NULL;
   GtkTreePath *path;
   GtkTreeViewColumn *column;

   /* Single clicks only */
   if (n_press == 1) {
      GtkTreeSelection *selection;
      GtkTreeIter iter;

      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_treeView));

      int bx, by;
      gtk_tree_view_convert_widget_to_bin_window_coords(
         GTK_TREE_VIEW(me->_treeView), (int)x, (int)y, &bx, &by);

      if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(me->_treeView),
                                        bx,
                                        by,
                                        &path,
                                        &column,
                                        NULL,
                                        NULL)) {

         /* Check if it's either a right-button click, or a left-button
          * click on the status column. */
         if (!(button == 3 ||
               (button == 1 &&
                strcmp(gtk_tree_view_column_get_title(column), "S") == 0)))
            return;

         vector<RPackage *> selected_pkgs;
         GList *li = NULL;

         // Treat click with CONTROL as additional selection
         if ((state & GDK_CONTROL_MASK) != GDK_CONTROL_MASK &&
             !gtk_tree_selection_path_is_selected(selection, path)) {
            gtk_tree_selection_unselect_all(selection);
         }
         gtk_tree_selection_select_path(selection, path);

         li = gtk_tree_selection_get_selected_rows(selection, &me->_pkgList);
         for (li = g_list_first(li); li != NULL; li = g_list_next(li)) {
            gtk_tree_model_get_iter(
               me->_pkgList, &iter, (GtkTreePath *)(li->data));

            gtk_tree_model_get(me->_pkgList, &iter, PKG_COLUMN, &pkg, -1);
            if (pkg)
               selected_pkgs.push_back(pkg);
         }

         me->cbTreeviewPopupMenu(button, bx, by, selected_pkgs);
         return;
      }
   }
}

void RGMainWindow::cbChangelogDialog(GSimpleAction *action,
                                     GVariant *parameter,
                                     gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;

   start_task([me]() -> task<void> {
      RPackage *pkg = me->selectedPackage();
      if (pkg != NULL) {
         co_await me->setInterfaceLocked(TRUE);
         co_await ShowChangelogDialog(me, pkg);
         co_await me->setInterfaceLocked(FALSE);
      }
   });
}

void RGMainWindow::cbPackageListRowActivated(GtkTreeView *treeview,
                                             GtkTreePath *path,
                                             GtkTreeViewColumn *arg2,
                                             gpointer data)
{
   // cout << "RGMainWindow::cbPackageListRowActivated()" << endl;
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me, treeview, path] -> task<void> {
      co_await me->packageListRowActivated(treeview, path);
   });
}

task<void> RGMainWindow::packageListRowActivated(GtkTreeView *treeview,
                                                 GtkTreePath *path)
{
   GtkTreeIter iter;
   RPackage *pkg = NULL;

   if (!gtk_tree_model_get_iter(_pkgList, &iter, path))
      co_return;

   gtk_tree_model_get(_pkgList, &iter, PKG_COLUMN, &pkg, -1);
   assert(pkg);

   int flags = pkg->getFlags();

   if (flags & RPackage::FPinned)
      co_return;

   if (!(flags & RPackage::FInstalled)) {
      if (flags & RPackage::FKeep)
         co_await pkgAction(PKG_INSTALL);
      else if (flags & RPackage::FInstall)
         co_await pkgAction(PKG_KEEP);
   } else if (flags & RPackage::FOutdated) {
      if (flags & RPackage::FKeep)
         co_await pkgAction(PKG_INSTALL);
      else if (flags & RPackage::FUpgrade)
         co_await pkgAction(PKG_KEEP);
   }

   // make sure we do not lose the keyboard focus (this happens in
   // pkgAction otherwise)
   // gtk_widget_grab_focus (GTK_WIDGET(treeview));
   // gtk_tree_view_set_cursor(GTK_TREE_VIEW(treeview), path, NULL, false);

   // GtkTreePath *start = gtk_tree_path_new();
   // bool ok = gtk_tree_view_get_visible_range(GTK_TREE_VIEW(treeview), &start,
   // NULL); if (ok && gtk_tree_model_get_iter_first(_pkgList, &iter)) {
   //    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview), start, NULL,
   //    true, 0.0, 0.0);
   // }
   // gtk_tree_path_free(start);

   setStatusText();
}

void RGMainWindow::cbAddCDROM(GSimpleAction *action,
                              GVariant *parameter,
                              gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->addCDROM(); });
}

task<void> RGMainWindow::addCDROM()
{
   RGCDScanner scan(this, _userDialog);
   co_await setInterfaceLocked(TRUE);
   bool updateCache = false;
   bool dontStop = true;
   while (dontStop) {
      if (co_await scan.run() == false)
         co_await showErrors();
      else
         updateCache = true;
      if (_config->FindB("APT::CDROM::NoMount", false))
         dontStop = false;
      else
         dontStop = co_await _userDialog->confirm(
            _("Do you want to add another CD-ROM?"));
   }
   scan.hide();
   if (updateCache) {
      setTreeLocked(TRUE);
      if (!_lister->openCache()) {
         co_await showErrors();
         exit(1);
      }
      setTreeLocked(FALSE);
      refreshTable(selectedPackage());
   }
   co_await setInterfaceLocked(FALSE);
}

void RGMainWindow::cbTasksClicked(GSimpleAction *action,
                                  GVariant *parameter,
                                  gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->tasksClicked(); });
}

task<void> RGMainWindow::tasksClicked()
{
   setBusyCursor(true);

   if (_tasksWin == NULL) {
      _tasksWin = new RGTasksWin(this);
   }
   auto packages = co_await _tasksWin->selectTasks();
   co_await selectToInstall(packages);

   setBusyCursor(false);
}

void RGMainWindow::cbOpenClicked(GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer data)
{
   // std::cout << "RGMainWindow::openClicked()" << endl;
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->openClicked(); });
}

task<void> RGMainWindow::openClicked()
{
   GtkWidget *filesel;
   filesel = gtk_file_chooser_dialog_new(_("Open changes"),
                                         GTK_WINDOW(window()),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         _("_Cancel"),
                                         GTK_RESPONSE_CANCEL,
                                         _("_Open"),
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);
   if (co_await co_run_dialog(GTK_DIALOG(filesel)) == GTK_RESPONSE_ACCEPT) {
      co_await setInterfaceLocked(TRUE);
      gtk_widget_hide(filesel);
      co_await RGFlushInterface();

      RPackageLister::pkgState state;
      _lister->saveState(state);

      GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(filesel));
      char *filename = g_file_get_path(file);
      g_object_unref(file);

      selectionsFilename = filename;
      ifstream in(filename);
      if (!in != 0) {
         _error->Error(_("Can't read %s"), filename);
         co_await _userDialog->showErrors();
         g_free(filename);
         co_return;
      }

      g_free(filename);

      _lister->unregisterObserver(this);
      // read the selections from the file
      _lister->readSelections(in);
      co_await askStateChange(state);

      // refresh to ensure that broken dependencies are displayed
      _lister->registerObserver(this);
      refreshTable();
      refreshSubViewList();
      setStatusText();
      co_await setInterfaceLocked(FALSE);
   }
   gtk_window_destroy(GTK_WINDOW(filesel));
}

void RGMainWindow::cbSaveClicked(GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer data)
{
   // std::cout << "RGMainWindow::saveClicked()" << endl;
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->saveClicked(); });
}

task<void> RGMainWindow::saveClicked()
{
   if (selectionsFilename == "") {
      co_await saveAsClicked();
      co_return;
   }

   ofstream out(selectionsFilename.c_str());
   if (!out != 0) {
      _error->Error(_("Can't write %s"), selectionsFilename.c_str());
      co_await _userDialog->showErrors();
      co_return;
   }

   _lister->unregisterObserver(this);
   _lister->writeSelections(out, saveFullState);
   _lister->registerObserver(this);
   setStatusText();
}


void RGMainWindow::cbSaveAsClicked(GSimpleAction *action,
                                   GVariant *parameter,
                                   gpointer data)
{
   // std::cout << "RGMainWindow::saveAsClicked()" << endl;
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->saveAsClicked(); });
}

task<void> RGMainWindow::saveAsClicked()
{
   GtkWidget *filesel;
   filesel = gtk_file_chooser_dialog_new(_("Save changes"),
                                         GTK_WINDOW(window()),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         _("_Cancel"),
                                         GTK_RESPONSE_CANCEL,
                                         _("_Save"),
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);
   gtk_file_chooser_add_choice(GTK_FILE_CHOOSER(filesel),
                               "full",
                               _("Save full state, not only changes"),
                               NULL,
                               NULL);

   if (co_await co_run_dialog(GTK_DIALOG(filesel)) == GTK_RESPONSE_ACCEPT) {
      GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(filesel));
      char *filename = g_file_get_path(file);
      selectionsFilename = filename;
      g_free(filename);
      g_object_unref(file);
      saveFullState = g_strcmp0("true",
                                gtk_file_chooser_get_choice(
                                   GTK_FILE_CHOOSER(filesel), "full")) == 0;
      // now call save for the actual saving
      co_await saveClicked();
   }
   gtk_window_destroy(GTK_WINDOW(filesel));
}


void RGMainWindow::cbShowConfigWindow(GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;

   if (me->_configWin == NULL) {
      me->_configWin = new RGPreferencesWindow(me, me->_lister);
   }

   me->_configWin->show();
}

void RGMainWindow::cbShowSetOptWindow(GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer data)
{
   RGMainWindow *win = (RGMainWindow *)data;

   if (win->_setOptWin == NULL)
      win->_setOptWin = new RGSetOptWindow(win);

   win->_setOptWin->show();
}

void RGMainWindow::cbDetailsWindow(GSimpleAction *action,
                                   GVariant *parameter,
                                   gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   assert(data);

   RPackage *pkg = me->selectedPackage();
   if (pkg == NULL)
      return;

   if (me->_pkgDetails == NULL)
      me->_pkgDetails = new RGPkgDetailsWindow(me);

   RGPkgDetailsWindow::fillInValues(me->_pkgDetails, pkg, true);
   me->_pkgDetails->show();
}

void RGMainWindow::cbShowSourcesWindow(GSimpleAction *action,
                                       GVariant *parameter,
                                       gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->showSourcesWindow(); });
}

task<void> RGMainWindow::showSourcesWindow()
{
   // FIXME: make this all go into the repository window
   bool Changed = false;
   bool ForceReload = _config->FindB("Synaptic::UpdateAfterSrcChange", false);

   if (!g_file_test("/usr/bin/software-properties-gtk",
                    G_FILE_TEST_IS_EXECUTABLE) ||
       _config->FindB("Synaptic::dontUseGnomeSoftwareProperties", false)) {
      RGRepositoryEditor w(this);
      Changed = co_await w.Run();
   } else {
      // use gnome-software-properties window
      co_await setInterfaceLocked(TRUE);
      GPid pid;
      int status;
      const char *argv[4];
      argv[0] = "/usr/bin/software-properties-gtk";
      argv[1] = "-n";
      argv[2] = "-t";
      argv[3] = NULL;
      g_spawn_async(NULL,
                    const_cast<char **>(argv),
                    NULL,
                    (GSpawnFlags)G_SPAWN_DO_NOT_REAP_CHILD,
                    NULL,
                    NULL,
                    &pid,
                    NULL);
      // kill the child if the window is deleted
      while (waitpid(pid, &status, WNOHANG) == 0) {
         usleep(50000);
         co_await RGFlushInterface();
      }
      Changed = WEXITSTATUS(status);
      co_await setInterfaceLocked(FALSE);
   }

   co_await RGFlushInterface();

   // auto update after repostitory change
   if (Changed == true && ForceReload) {
      co_await updateClicked();
   } else if (Changed == true &&
              _config->FindB("Synaptic::AskForUpdateAfterSrcChange", true)) {
      // ask for update after repo change
      GtkWidget *cb, *dialog;
      dialog = gtk_message_dialog_new(GTK_WINDOW(window()),
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_INFO,
                                      GTK_BUTTONS_NONE,
                                      _("Repositories changed"));
      // TRANSLATORS: this message appears when the user added/removed
      // a repository (sources.list entry) a reload (apt-get update) is
      // needed then
      gchar *msgstr = _("The repository information "
                        "has changed. "
                        "You have to click on the "
                        "\"Reload\" button for your changes to "
                        "take effect");
      gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), msgstr);
      gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                             _("_Cancel"),
                             GTK_RESPONSE_REJECT,
                             _("_Reload"),
                             GTK_RESPONSE_ACCEPT,
                             NULL);
      GtkWidget *reload_button = gtk_dialog_get_widget_for_response(
         GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
      GtkWidget *refresh_image = gtk_image_new_from_icon_name("view-refresh");
      gtk_button_set_child(GTK_BUTTON(reload_button), refresh_image);
      cb = gtk_check_button_new_with_label(_("Never show this message again"));
      gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     cb);
      gtk_widget_show(cb);
      gint response = co_await co_run_dialog(GTK_DIALOG(dialog));
      gtk_widget_hide(dialog);
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb))) {
         _config->Set("Synaptic::AskForUpdateAfterSrcChange", false);
      }
      if (response == GTK_RESPONSE_ACCEPT) {
         co_await updateClicked();
      }
      gtk_window_destroy(GTK_WINDOW(dialog));
   }
}

static void traverseToolbarButtons(
   GtkWidget *toolbar,
   std::function<void(GtkWidget *, GtkWidget *, GtkWidget *)> cb)
{
   if (!GTK_IS_WIDGET(toolbar))
      return;

   for (GtkWidget *child = gtk_widget_get_first_child(toolbar); child != NULL;
        child = gtk_widget_get_next_sibling(child)) {
      if (GTK_IS_BUTTON(child)) {
         GtkWidget *box = gtk_button_get_child(GTK_BUTTON(child));
         GtkWidget *image = nullptr;
         GtkWidget *label = nullptr;

         for (GtkWidget *box_child = gtk_widget_get_first_child(box);
              box_child != NULL;
              box_child = gtk_widget_get_next_sibling(box_child)) {
            if (GTK_IS_IMAGE(box_child))
               image = GTK_WIDGET(box_child);
            else if (GTK_IS_LABEL(box_child))
               label = GTK_WIDGET(box_child);
         }

         cb(box, image, label);
      }
   }
}

void RGMainWindow::cbMenuToolbarClicked(GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   std::string style = g_variant_get_string(parameter, nullptr);

   g_simple_action_set_state(action, parameter);

   GtkWidget *toolbar =
      GTK_WIDGET(gtk_builder_get_object(me->_builder, "hbox_button_toolbar"));
   assert(toolbar);

   // save new toolbar state
   if (style == "icons-only") {
      me->_toolbarStyle = RG_TOOLBAR_ICONS;
      traverseToolbarButtons(
         toolbar, [](GtkWidget *box, GtkWidget *image, GtkWidget *label) {
            gtk_widget_show(image);
            gtk_widget_hide(label);
         });
      gtk_widget_show(toolbar);
   } else if (style == "text-only") {
      me->_toolbarStyle = RG_TOOLBAR_TEXT;
      traverseToolbarButtons(
         toolbar, [](GtkWidget *box, GtkWidget *image, GtkWidget *label) {
            gtk_widget_hide(image);
            gtk_widget_show(label);
         });
      gtk_widget_show(toolbar);
   } else if (style == "below") {
      me->_toolbarStyle = RG_TOOLBAR_BOTH;
      traverseToolbarButtons(
         toolbar, [](GtkWidget *box, GtkWidget *image, GtkWidget *label) {
            gtk_orientable_set_orientation(GTK_ORIENTABLE(box),
                                           GTK_ORIENTATION_VERTICAL);
            gtk_widget_show(image);
            gtk_widget_show(label);
         });
      gtk_widget_show(toolbar);
   } else if (style == "beside") {
      me->_toolbarStyle = RG_TOOLBAR_BOTH_HORIZ;
      traverseToolbarButtons(
         toolbar, [](GtkWidget *box, GtkWidget *image, GtkWidget *label) {
            gtk_orientable_set_orientation(GTK_ORIENTABLE(box),
                                           GTK_ORIENTATION_HORIZONTAL);
            gtk_widget_show(image);
            gtk_widget_show(label);
         });
      gtk_widget_show(toolbar);
   } else {
      me->_toolbarStyle = RG_TOOLBAR_HIDE;
      gtk_widget_hide(toolbar);
   }
}

void RGMainWindow::cbFindToolClicked(GSimpleAction *action,
                                     GVariant *parameter,
                                     gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->findTool(); });
}

task<void> RGMainWindow::findTool()
{
   if (_findWin == NULL) {
      _findWin = new RGFindWindow(this);
   }

   _findWin->selectText();
   int res = co_await co_run_dialog(GTK_DIALOG(_findWin->window()));
   if (res == GTK_RESPONSE_OK) {
      // clear the quick search, otherwise both apply and that is
      // confusing
      gtk_editable_set_text(GTK_EDITABLE(_entry_fast_search), "");

      string str = _findWin->getFindString();
      setBusyCursor(true);

      // we need to convert here as the DDTP project does not use utf-8
      const char *locale_str = utf8_to_locale(str.c_str());
      if (locale_str == NULL) // invalid utf-8
         locale_str = str.c_str();

      int type = _findWin->getSearchType();
      GtkWidget *progress =
         GTK_WIDGET(gtk_builder_get_object(_builder, "progressbar_main"));
      GtkWidget *label =
         GTK_WIDGET(gtk_builder_get_object(_builder, "label_status"));
      RGCacheProgress searchProgress(progress, label);
      int found = _lister->searchView()->setSearch(
         str, type, locale_str, searchProgress);
      changeView(PACKAGE_VIEW_SEARCH, str);

      setBusyCursor(false);
      gchar *statusstr = g_strdup_printf(_("Found %i packages"), found);
      setStatusText(statusstr);
      updatePackageInfo(NULL);
      g_free(statusstr);
   }
}

void RGMainWindow::cbShowAboutPanel(GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data)
{
   const char *authors[] = {"Alfredo K. Kojima",
                            "Michael Vogt",
                            "Gustavo Niemeyer",
                            "Sebastian Heinlein",
                            "Enrico Zini",
                            "Panu Matilainen",
                            "Sviatoslav Sviridov",
                            NULL};
   const char *documenters[] = {
      "Wybo Dekker", "Michael Vogt", "Sebastian Heinlein", NULL};

   gtk_show_about_dialog(
      NULL,
      "program-name",
      _("Synaptic Package Manager"),
      "version",
      VERSION,
      "logo-icon-name",
      "synaptic",
      "copyright",
      _("© 2001-2004 Connectiva S/A \n © 2002-2025 Michael Vogt"),
      "authors",
      authors,
      "documenters",
      documenters,
      "translator-credits",
      _("translator-credits"),
      "comments",
      _("Package management software using apt. \n"
        "https://github.com/mvo5/synaptic/wiki \n\n"
        "This program comes with absolutely no warranty. \n"
        "Released using the GNU General Public License, version 2 or later"),
      NULL);
}

void RGMainWindow::cbShowIconLegendPanel(GSimpleAction *action,
                                         GVariant *parameter,
                                         gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;

   if (me->_iconLegendPanel == NULL)
      me->_iconLegendPanel = new RGIconLegendPanel(me);
   me->_iconLegendPanel->show();
}

void RGMainWindow::cbViewLogClicked(GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;

   if (me->_logView == NULL)
      me->_logView = new RGLogView(me);
   me->_logView->readLogs();
   me->_logView->show();
}

void RGMainWindow::cbHelpAction(GSimpleAction *action,
                                GVariant *parameter,
                                gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->helpAction(); });
}

task<void> RGMainWindow::helpAction()
{
   setStatusText(_("Starting help viewer..."));

   // FIXME: move this into rgutils as well (or rgspawn.cc)
   vector<const gchar *> cmd;
   if (is_binary_in_path("yelp")) {
      cmd.push_back("yelp");
      cmd.push_back("ghelp:synaptic");
   } else {
      cmd.push_back("/usr/bin/xdg-open");
      cmd.push_back(PACKAGE_DATA_DIR "/html/index.html");
   }

   if (cmd.empty()) {
      co_await _userDialog->error(
         _("No help viewer is installed!\n\n"
           "You need either the GNOME help viewer 'yelp', "
           "or any browser setup to use xdg-open "
           "to view the synaptic manual.\n\n"
           "Alternatively you can open the man page "
           "with 'man synaptic' from the "
           "command line or view the html version located "
           "in the 'synaptic/html' folder."));
      co_return;
   }
   RunAsSudoUserCommand(cmd);
}

void RGMainWindow::cbShowFilterManagerWindow(GSimpleAction *action,
                                             GVariant *parameter,
                                             gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->showFilterManagerWindow(); });
}

task<void> RGMainWindow::showFilterManagerWindow()
{
   if (_fmanagerWin == NULL) {
      _fmanagerWin = new RGFilterManagerWindow(this, _lister->filterView());
   }

   _fmanagerWin->readFilters();
   int res = co_await co_run_dialog(GTK_DIALOG(_fmanagerWin->window()));
   if (res == GTK_RESPONSE_OK) {
      co_await setInterfaceLocked(TRUE);

      _lister->filterView()->refreshFilters();
      refreshTable();
      refreshSubViewList();

      co_await setInterfaceLocked(FALSE);
   }
}

void RGMainWindow::cbSelectedRow(GtkTreeSelection *selection, gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   GtkTreeIter iter;
   RPackage *pkg;
   GList *li, *list;


   // cout << "RGMainWindow::cbSelectedRow()" << endl;

   if (me->_pkgList == NULL) {
      cerr << "selectedRow(): me->_pkgTree == NULL " << endl;
      return;
   }
   list = li = gtk_tree_selection_get_selected_rows(selection, &me->_pkgList);

   // list is empty
   if (li == NULL) {
      me->updatePackageInfo(NULL);
      return;
   }
   // we are only interested in the last element
   li = g_list_last(li);
   gtk_tree_model_get_iter(me->_pkgList, &iter, (GtkTreePath *)(li->data));

   gtk_tree_model_get(me->_pkgList, &iter, PKG_COLUMN, &pkg, -1);
   if (pkg == NULL)
      return;

   // free the list
   g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
   g_list_free(list);

   me->updatePackageInfo(pkg);
}

void RGMainWindow::cbClearAllChangesClicked(GSimpleAction *action,
                                            GVariant *parameter,
                                            gpointer data)
{
   // cout << "clearAllChangesClicked" << endl;
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->clearAllChanges(); });
}

task<void> RGMainWindow::clearAllChanges()
{
   co_await setInterfaceLocked(TRUE);
   _lister->unregisterObserver(this);
   setTreeLocked(TRUE);

   // reset
   if (!_lister->openCache()) {
      co_await showErrors();
      exit(1);
   }

   _lister->registerObserver(this);
   setTreeLocked(FALSE);
   refreshTable();
   refreshSubViewList();
   co_await setInterfaceLocked(FALSE);
   setStatusText();
}


void RGMainWindow::cbUndoClicked(GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer data)
{
   // cout << "undoClicked" << endl;
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->undo(); });
}

task<void> RGMainWindow::undo()
{
   co_await setInterfaceLocked(TRUE);

   _lister->unregisterObserver(this);

   // undo
   _lister->undo();

   _lister->registerObserver(this);
   refreshTable();
   co_await setInterfaceLocked(FALSE);
}

void RGMainWindow::cbRedoClicked(GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer data)
{
   // cout << "redoClicked" << endl;
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->redo(); });
}

task<void> RGMainWindow::redo()
{
   co_await setInterfaceLocked(TRUE);

   _lister->unregisterObserver(this);

   // redo
   _lister->redo();

   _lister->registerObserver(this);
   refreshTable();
   co_await setInterfaceLocked(FALSE);
}

void RGMainWindow::cbPkgReconfigureClicked(GSimpleAction *action,
                                           GVariant *parameter,
                                           gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   // cout << "RGMainWindow::pkgReconfigureClicked()" << endl;
   start_task([me]() -> task<void> { co_await me->pkgReconfigureClicked(); });
}

task<void> RGMainWindow::pkgReconfigureClicked()
{
   if (selectedPackage() == NULL)
      co_return;

   RPackage *pkg = NULL;
   pkg = _lister->getPackage("libgnome2-perl");
   if (pkg && pkg->installedVersion() == NULL) {
      co_await _userDialog->error(_("Cannot start configuration tool!\n"
                                    "You have to install the required package "
                                    "'libgnome2-perl'."));
      co_return;
   }

   setStatusText(_("Starting package configuration tool..."));
   const gchar *cmd[] = {
      "/usr/sbin/dpkg-reconfigure", "-fgnome", selectedPackage()->name(), NULL};
   GError *error = NULL;
   g_spawn_async("/",
                 const_cast<gchar **>(cmd),
                 NULL,
                 (GSpawnFlags)0,
                 NULL,
                 NULL,
                 NULL,
                 &error);
   if (error != NULL) {
      std::cerr << "failed to run dpkg-reconfigure cmd" << std::endl;
   }
}


void RGMainWindow::cbPkgHelpClicked(GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->pkgHelpClicked(); });
}

task<void> RGMainWindow::pkgHelpClicked()
{
   if (selectedPackage() == NULL)
      co_return;

   // cout << "RGMainWindow::pkgHelpClicked()" << endl;
   setStatusText(_("Starting package documentation viewer..."));

   // mozilla eats bookmarks when run under sudo (because it does not
   // change $HOME) so we better play safe here
   if (getenv("SUDO_USER") != NULL) {
      struct passwd *pw = getpwuid(0);
      setenv("HOME", pw->pw_dir, 1);
   }

   if (is_binary_in_path("dwww")) {
      const gchar *cmd[5];
      cmd[0] = "dwww";
      cmd[1] = selectedPackage()->name();
      cmd[2] = NULL;
      g_spawn_async("/tmp",
                    const_cast<gchar **>(cmd),
                    NULL,
                    (GSpawnFlags)0,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
   } else {
      co_await _userDialog->error(
         _("You have to install the package \"dwww\" "
           "to browse the documentation of a package"));
   }
}


void RGMainWindow::cbChangedView(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([self, me]() -> task<void> {
      // only act on the active buttons
      if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self)) ||
          me->_blockActions == TRUE) {
         co_return;
      }

      long view = (long)g_object_get_data(G_OBJECT(self), "index");
      co_await me->changeView(view);
   });
}

void RGMainWindow::cbChangedSubView(GtkTreeSelection *selection, gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   if (me->_blockActions)
      return;

   me->setBusyCursor(true);
   // we need to set a empty model first so that gtklistview
   // can do its cleanup, if we do not do that, then the cleanup
   // code in gtktreeview gets confused and throws
   // Gtk-CRITICAL **: gtk_tree_view_unref_tree_helper: assertion `node != NULL'
   // failed at us, see LP: #38397 for more information
   gtk_tree_view_set_model(GTK_TREE_VIEW(me->_treeView), NULL);

   string selected = MarkupUnescapeString(me->selectedSubView());
   me->_lister->setSubView(utf8(selected.c_str()));
   me->refreshTable(NULL, false);
   me->setBusyCursor(false);
   me->updatePackageInfo(NULL);
}

void RGMainWindow::activeWindowToForeground()
{
   // cout << "activeWindowToForeground: " << getpid() << endl;

   // easy, we have a main window
   if (_config->FindB("Volatile::HideMainwindow", false) == false) {
      gtk_window_present(GTK_WINDOW(window()));
      return;
   }

   // harder, we run without mainWindow (in non-interactive mode most likly)
   if (_fetchProgress && gtk_widget_get_visible(_fetchProgress->window()))
      gtk_window_present(GTK_WINDOW(_fetchProgress->window()));
   else if (_installProgress &&
            gtk_widget_get_visible(_installProgress->window()))
      gtk_window_present(GTK_WINDOW(_installProgress->window()));
   else
      g_critical("activeWindowToForeground(): no active window found\n");
}

void RGMainWindow::cbProceedClicked(GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->applyChanges(); });
}

task<void> RGMainWindow::applyChanges()
{
   // nothing to do
   int listed, installed, broken;
   int toInstall, toRemove;
   double size;
   _lister->getStats(installed, broken, toInstall, toRemove, size);
   if ((toInstall + toRemove) == 0)
      co_return;

   // check whether we can really do it
   if (!_lister->check()) {
      co_await _userDialog->error(_("Could not apply changes!\n"
                                    "Fix broken packages first."));
      co_return;
   }

   int a, b, c, d, e, f, g, h, unAuthenticated;
   double s;
   _lister->getSummary(a, b, c, d, e, f, g, h, unAuthenticated, s);
   if (unAuthenticated ||
       _config->FindB("Volatile::Non-Interactive", false) == false) {
      // show a summary of what's gonna happen
      RGSummaryWindow summ(this, _lister);
      if (!co_await summ.showAndConfirm()) {
         // canceled operation
         co_return;
      }
   }

   co_await setInterfaceLocked(TRUE);
   updatePackageInfo(NULL);

   setStatusText(_("Applying marked changes. This may take a while..."));

   // fetch packages
   RGFetchProgress *fprogress = _fetchProgress = new RGFetchProgress(this);
   fprogress->setDescription(_("Downloading Package Files"), "");
   //			     _("The package files will be cached locally for
   // installation."));

   // Do not let the treeview access the cache during the update.
   setTreeLocked(TRUE);

   // save selections to temporary file
   const gchar *file =
      g_strdup_printf("%s/selections.proceed", RConfDir().c_str());
   ofstream out(file);
   if (!out != 0) {
      _error->Error(_("Can't write %s"), file);
      co_await _userDialog->showErrors();
      co_return;
   }
   _lister->writeSelections(out, false);


   RInstallProgress *iprogress;
#ifdef HAVE_TERMINAL
#   ifdef HAVE_RPM
   bool UseTerminal = false;
#   else
// no RPM
#      ifdef WITH_DPKG_STATUSFD
   bool UseTerminal = false;
#      else
   bool UseTerminal = true;
#      endif // DPKG
#   endif    // HAVE_RPM
   RGTermInstallProgress *term = NULL;
   if (_config->FindB("Synaptic::UseTerminal", UseTerminal) == true)
      iprogress = term = new RGTermInstallProgress(this);
   else
#endif // HAVE_TERMINAL


#ifdef HAVE_RPM
      iprogress = new RGInstallProgress(this, _lister);
#else
#   ifdef WITH_DPKG_STATUSFD
   iprogress = new RGDebInstallProgress(this, _lister);
#   else
   iprogress = new RGDummyInstallProgress();
#   endif // WITH_DPKG_STATUSFD
#endif    // HAVE_RPM
   _installProgress = dynamic_cast<RGWindow *>(iprogress);

   co_await _lister->commitChanges(fprogress, iprogress);

   // FIXME: move this into the terminal class
#ifdef HAVE_TERMINAL
   // wait until the term dialog is closed
   if (term != NULL) {
      while (gtk_widget_get_visible(GTK_WIDGET(term->window()))) {
         co_await RGFlushInterface();
         co_await sleep_ms{100};
      }
   }
#endif
   delete fprogress;
   _fetchProgress = NULL;
   delete iprogress;
   _installProgress = NULL;

   if (_config->FindB("Synaptic::IgnorePMOutput", false) == false) {
      co_await showErrors();
   } else {
      _error->Discard();
   }
   if (_config->FindB("Volatile::Non-Interactive", false) == true) {
      co_return;
   }

   if (_config->FindB("Synaptic::AskQuitOnProceed", false) == true &&
       co_await _userDialog->confirm(_("Do you want to quit Synaptic?"))) {
      _error->Discard();
      co_await saveState();
      co_await showErrors();
      exit(0);
   }

   if (_config->FindB("Volatile::Download-Only", false) == false) {
      // reset the cache
      if (!_lister->openCache()) {
         co_await showErrors();
         exit(1);
      }
   }
   // reread saved selections
   ifstream in(file);
   if (!in != 0) {
      _error->Error(_("Can't read %s"), file);
      co_await _userDialog->showErrors();
      co_return;
   }
   _lister->readSelections(in);
   unlink(file);
   g_free((void *)file);


   setTreeLocked(FALSE);
   refreshTable();
   refreshSubViewList();
   co_await setInterfaceLocked(FALSE);
   updatePackageInfo(NULL);
}

void RGMainWindow::cbShowWelcomeDialog(GSimpleAction *action,
                                       GVariant *parameter,
                                       gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->showWelcomeDialog(); });
}

task<void> RGMainWindow::showWelcomeDialog()
{
   RGGtkBuilderUserDialog dia(this);
   co_await dia.co_run("welcome");
   GtkWidget *cb = GTK_WIDGET(
      gtk_builder_get_object(dia.getGtkBuilder(), "checkbutton_show_again"));
   assert(cb);
   _config->Set("Synaptic::showWelcomeDialog",
                gtk_check_button_get_active(GTK_CHECK_BUTTON(cb)));
}

void RGMainWindow::xapianDoSearch(void *data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> {
      const gchar *str =
         gtk_editable_get_text(GTK_EDITABLE(me->_entry_fast_search));
      GtkStyleContext *styleContext =
         gtk_widget_get_style_context(me->_entry_fast_search);

      me->_fastSearchEventID = -1;
      me->setBusyCursor(true);
      co_await RGFlushInterface();
      if (str == NULL || strlen(str) <= 1) {
         // reset the color
         gtk_style_context_remove_provider(
            styleContext, GTK_STYLE_PROVIDER(_fastSearchCssProvider));
         // if the user has cleared the search, refresh the view
         // Gtk-CRITICAL **: gtk_tree_view_unref_tree_helper: assertion `node !=
         // NULL' failed at us, see LP: #38397 for more information
         gtk_tree_view_set_model(GTK_TREE_VIEW(me->_treeView), NULL);
         me->_lister->reapplyFilter();
         me->refreshTable();
         me->setBusyCursor(false);
      } else if (strlen(str) > 1) {
         // only search when there is more than one char entered, single
         // char searches tend to be very slow
         me->setBusyCursor(true);
         co_await RGFlushInterface();
         gtk_tree_view_set_model(GTK_TREE_VIEW(me->_treeView), NULL);
         me->refreshTable();
         // set color to a light yellow to make it more obvious that a search
         // is performed
         gtk_style_context_add_provider(
            styleContext,
            GTK_STYLE_PROVIDER(_fastSearchCssProvider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      }
      me->setBusyCursor(false);
   });
}

void RGMainWindow::cbSearchEntryChanged(GtkWidget *edit, void *data)
{
   // cerr << "RGMainWindow::cbSearchEntryChanged()" << endl;
   RGMainWindow *me = (RGMainWindow *)data;
   if (me->_fastSearchEventID > 0) {
      g_source_remove(me->_fastSearchEventID);
      me->_fastSearchEventID = -1;
   }
   me->_fastSearchEventID = g_timeout_add_once(500, xapianDoSearch, me);
}

void RGMainWindow::cbUpdateClicked(GSimpleAction *action,
                                   GVariant *parameter,
                                   gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->updateClicked(); });
}

task<void> RGMainWindow::updateClicked()
{
   // need to delete dialogs, as they might have data pointing
   // to old stuff
   // xxx    delete me->_fmanagerWin;
   _fmanagerWin = NULL;

   RGFetchProgress *progress = _fetchProgress = new RGFetchProgress(this);
   progress->setDescription(
      _("Downloading Package Information"),
      _("The repositories will be checked for new, removed "
        "or upgraded software packages."));

   setStatusText(_("Reloading package information..."));

   co_await setInterfaceLocked(TRUE);
   setTreeLocked(TRUE);
   _lister->unregisterObserver(this);

   // save to temporary file
   const gchar *file =
      g_strdup_printf("%s/selections.update", RConfDir().c_str());
   ofstream out(file);
   if (!out != 0) {
      _error->Error(_("Can't write %s"), file);
      co_await _userDialog->showErrors();
      co_return;
   }
   _lister->writeSelections(out, false);

   // update cache and forget about the previous new packages
   // (only if no error occurred)
   string error;
   if (!_lister->updateCache(progress, error)) {
      RGGtkBuilderUserDialog dia(this, "update_failed");
      GtkWidget *tv =
         GTK_WIDGET(gtk_builder_get_object(dia.getGtkBuilder(), "textview"));
      GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
      gtk_text_buffer_set_text(tb, utf8(error.c_str()), -1);
      co_await dia.co_run();
   } else {
      forgetNewPackages();
      _config->Set("Synaptic::update::last", time(NULL));
   }
   delete progress;
   _fetchProgress = NULL;

   // show errors and warnings (like the gpg failures for the package list)
   co_await showErrors();

   if (!_lister->openCache()) {
      co_await showErrors();
      exit(1);
   }
   // reread saved selections
   ifstream in(file);
   if (!in != 0) {
      _error->Error(_("Can't read %s"), file);
      co_await _userDialog->showErrors();
      co_return;
   }
   _lister->readSelections(in);
   unlink(file);
   g_free((void *)file);

   // check if the index needs to be rebuild
   xapianDoIndexUpdate(this);

   setTreeLocked(FALSE);
   refreshTable();
   refreshSubViewList();
   co_await setInterfaceLocked(FALSE);
   setStatusText();
}

void RGMainWindow::cbFixBrokenClicked(GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->fixBroken(); });
}

task<void> RGMainWindow::fixBroken()
{
   RPackage *pkg = selectedPackage();

   bool res = _lister->fixBroken();
   co_await setInterfaceLocked(TRUE);
   refreshTable(pkg);

   if (!res)
      setStatusText(_("Failed to resolve dependency problems!"));
   else
      setStatusText(_("Successfully fixed dependency problems"));

   co_await setInterfaceLocked(FALSE);
   co_await showErrors();
}


void RGMainWindow::cbUpgradeClicked(GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->upgrade(); });
}

task<void> RGMainWindow::upgrade()
{
   RPackage *pkg = selectedPackage();
   bool dist_upgrade;
   int res;

   if (!_lister->check()) {
      co_await _userDialog->error(_("Could not upgrade the system!\n"
                                    "Fix broken packages first."));
      co_return;
   }
   // check if we have saved upgrade type
   UpgradeType upgrade =
      (UpgradeType)_config->FindI("Synaptic::UpgradeType", UPGRADE_DIST);

   // special case for non-interactive upgrades
   if (_config->FindB("Volatile::Non-Interactive", false)) {
      if (_config->FindB("Volatile::Upgrade-Mode", false))
         upgrade = UPGRADE_NORMAL;
      else if (_config->FindB("Volatile::DistUpgrade-Mode", false))
         upgrade = UPGRADE_DIST;
   }

   if (upgrade == UPGRADE_ASK) {
      // ask what type of upgrade the user wants
      GtkBuilder *builder;
      GtkWidget *button;

      RGGtkBuilderUserDialog dia(this);
      res = co_await dia.co_run("upgrade", true);
      switch (res) {
         case GTK_RESPONSE_CANCEL:
         case GTK_RESPONSE_DELETE_EVENT:
            co_return;
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
      builder = dia.getGtkBuilder();
      // see if the user wants the answer saved
      button =
         GTK_WIDGET(gtk_builder_get_object(builder, "checkbutton_remember"));
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
         _config->Set("Synaptic::upgradeType", dist_upgrade);
   } else {
      // use the saved answer (don't ask)
      dist_upgrade = upgrade;
   }

   // do the work
   co_await setInterfaceLocked(TRUE);
   setStatusText(_("Marking all available upgrades..."));

   _lister->saveUndoState();

   RPackageLister::pkgState state;
   _lister->saveState(state);

   if (dist_upgrade)
      res = _lister->distUpgrade();
   else
      res = _lister->upgrade();

   if (co_await askStateChange(state)) {
      refreshTable(pkg);

      if (res)
         setStatusText(_("Successfully marked available upgrades"));
      else
         setStatusText(_("Failed to mark all available upgrades!"));
   } else {
      // if the user canceled the action, just show the default message
      setStatusText();
   }

   co_await setInterfaceLocked(FALSE);
   co_await showErrors();
}

void RGMainWindow::cbMenuPinClicked(GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;

   if (me->_blockActions)
      return;

   // activating a stateful action without parameter type passes
   // parameter == NULL, so toggle the current state ourselves
   GVariant *state = g_action_get_state(G_ACTION(action));
   bool active = !g_variant_get_boolean(state);
   g_variant_unref(state);
   g_simple_action_set_state(action, g_variant_new_boolean(active));

   start_task([me, active]() -> task<void> { co_await me->pin(active); });
}

task<void> RGMainWindow::pin(bool active)
{
   if (_blockActions)
      co_return;

   GtkTreeSelection *selection;
   GtkTreeIter iter;
   RPackage *pkg;

   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
   GList *li, *list;

   list = li = gtk_tree_selection_get_selected_rows(selection, &_pkgList);
   if (li == NULL)
      co_return;

   co_await setInterfaceLocked(TRUE);
   _lister->unregisterObserver(this);

   // save to temporary file
   const gchar *file =
      g_strdup_printf("%s/selections.hold", RConfDir().c_str());
   ofstream out(file);
   if (!out != 0) {
      _error->Error(_("Can't write %s"), file);
      co_await _userDialog->showErrors();
      co_return;
   }
   _lister->writeSelections(out, false);

   while (li != NULL) {
      gtk_tree_model_get_iter(_pkgList, &iter, (GtkTreePath *)(li->data));
      gtk_tree_model_get(_pkgList, &iter, PKG_COLUMN, &pkg, -1);
      if (pkg == NULL) {
         li = g_list_next(li);
         continue;
      }

      pkg->setPinned(active);
      _roptions->setPackageLock(pkg->name(), active);
      li = g_list_next(li);
   }
   setTreeLocked(TRUE);
   if (!_lister->openCache()) {
      co_await showErrors();
      exit(1);
   }

   // reread saved selections
   ifstream in(file);
   if (!in != 0) {
      _error->Error(_("Can't read %s"), file);
      co_await _userDialog->showErrors();
      co_return;
   }
   _lister->readSelections(in);
   unlink(file);
   g_free((void *)file);

   // free the list
   g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
   g_list_free(list);

   _lister->registerObserver(this);
   setTreeLocked(FALSE);
   refreshTable();
   refreshSubViewList();
   refreshTable();
   co_await setInterfaceLocked(FALSE);
}

void RGMainWindow::cbTreeviewPopupMenu(int button,
                                       double x,
                                       double y,
                                       vector<RPackage *> selected_pkgs)
{
   // Nothing selected, shouldn't happen, but we play safely.
   if (selected_pkgs.size() == 0)
      return;

   // FIXME: we take the first pkg and find out available actions,
   //        we should calc available actions from all selected pkgs.
   RPackage *pkg = selected_pkgs[0];

   int flags = pkg->getFlags();

   if (flags & RPackage::FPinned)
      return;

   if (button == 1 &&
       _config->FindB("Synaptic::OneClickOnStatusActions", false) == true) {
      activateAction("mark-default", nullptr);
      return;
   }

   GMenu *popupMenuModel = g_menu_new();
   g_menu_append(popupMenuModel, _("Unmark"), "win.unmark");
   g_menu_append(
      popupMenuModel, _("Mark for Installation"), "win.mark-install");
   g_menu_append(
      popupMenuModel, _("Mark for Reinstallation"), "win.mark-reinstall");
   g_menu_append(popupMenuModel, _("Mark for Upgrade"), "win.mark-upgrade");
   g_menu_append(popupMenuModel, _("Mark for Removal"), "win.mark-delete");
#ifndef HAVE_RPM
   g_menu_append(
      popupMenuModel, _("Mark for Complete Removal"), "win.mark-purge");
#endif

#if 0 // disabled for now
   g_menu_append(popupMenuModel, _("Remove Including Orphaned Dependencies"), "win.mark-delete-with-deps");

   GMenu *lockSubmenu = g_menu_new();
   g_menu_append(lockSubmenu, _("Hold Current Version"), "win.lock-version");
   g_menu_append_section(popupMenuModel, nullptr, G_MENU_MODEL(lockSubmenu));
#endif

   GMenu *propsSubmenu = g_menu_new();
   g_menu_append(propsSubmenu, _("Properties"), "win.package-properties");
   g_menu_append_section(popupMenuModel, nullptr, G_MENU_MODEL(propsSubmenu));

#ifndef HAVE_RPM // recommends stuff
   if (selected_pkgs.size() == 1) {
      GMenu *recommendsSection = g_menu_new();

      if (GMenu *recommendedSubmenu =
             buildWeakDependsMenu(pkg, pkgCache::Dep::Recommends))
         g_menu_append_submenu(recommendsSection,
                               _("Mark Recommended for Installation"),
                               G_MENU_MODEL(recommendedSubmenu));
      else
         g_menu_append(
            recommendsSection, _("Mark Recommended for Installation"), nullptr);

      if (GMenu *suggestedSubmenu =
             buildWeakDependsMenu(pkg, pkgCache::Dep::Suggests))
         g_menu_append_submenu(recommendsSection,
                               _("Mark Suggested for Installation"),
                               G_MENU_MODEL(suggestedSubmenu));
      else
         g_menu_append(
            recommendsSection, _("Mark Suggested for Installation"), nullptr);

      g_menu_append_section(
         popupMenuModel, nullptr, G_MENU_MODEL(recommendsSection));
   }
#endif

   GdkRectangle rect{0, 0, 0, 0};
   gtk_tree_view_convert_bin_window_to_widget_coords(
      GTK_TREE_VIEW(_treeView), (int)x, (int)y, &rect.x, &rect.y);

   GtkWidget *popupMenu =
      gtk_popover_menu_new_from_model(G_MENU_MODEL(popupMenuModel));
   gtk_widget_set_parent(popupMenu, _treeView);
   gtk_popover_set_pointing_to(GTK_POPOVER(popupMenu), &rect);
   gtk_popover_present(GTK_POPOVER(popupMenu));
   gtk_popover_popup(GTK_POPOVER(popupMenu));
}

GMenu *RGMainWindow::buildWeakDependsMenu(RPackage *pkg,
                                          pkgCache::Dep::DepType type)
{
   // safty first
   if (pkg == NULL)
      return NULL;
   bool found = false;

   GMenu *menu = g_menu_new();
   GMenuItem *item;
   vector<DepInformation> deps = pkg->enumDeps();
   for (unsigned int i = 0; i < deps.size(); i++) {
      if (deps[i].type == type) {
         // not virtual
         if (!deps[i].isVirtual) {
            found = true;
            item = g_menu_item_new(deps[i].name, nullptr);
            if (!deps[i].isSatisfied)
               g_menu_item_set_action_and_target(
                  item, "win.install-by-name", "s", deps[i].name);
            g_menu_append_item(menu, item);
         } else {
            // TESTME: expand virutal packages (expensive!?!)
            const vector<RPackage *> pkgs = _lister->getPackages();
            for (unsigned int k = 0; k < pkgs.size(); k++) {
               vector<string> d = pkgs[k]->provides();
               for (unsigned int j = 0; j < d.size(); j++)
                  if (strcoll(deps[i].name, d[j].c_str()) == 0) {
                     found = true;
                     item = g_menu_item_new(pkgs[k]->name(), nullptr);
                     g_menu_append_item(menu, item);
                     int f = pkgs[k]->getFlags();
                     if (!((f & RPackage::FInstall) ||
                           (f & RPackage::FInstalled)))
                        g_menu_item_set_action_and_target(
                           item, "win.install-by-name", "s", pkgs[k]->name());
                  }
            }
         }
      }
   }
   if (found)
      return menu;
   else
      return NULL;
}

task<void> RGMainWindow::selectToInstall(vector<string> packagenames)
{
   RPackageLister::pkgState state;
   vector<RPackage *> exclude;
   vector<RPackage *> instPkgs;

   // we always save the state (for undo)
   _lister->saveState(state);
   _lister->notifyCachePreChange();

   for (unsigned int i = 0; i < packagenames.size(); i++) {
      RPackage *newpkg = (RPackage *)_lister->getPackage(packagenames[i]);
      if (newpkg) {
         // only install the package if it is not already installed or if
         // it is outdated
         if (!(newpkg->getFlags() & RPackage::FInstalled) ||
             (newpkg->getFlags() & RPackage::FOutdated)) {
            // actual action
            newpkg->setNotify(false);
            pkgInstallHelper(newpkg);
            newpkg->setNotify(true);
            // exclude.push_back(newpkg);
            instPkgs.push_back(newpkg);
         }
      }
   }

   // ask for additional changes
   setBusyCursor(true);
   if (co_await askStateChange(state, exclude)) {
      _lister->saveUndoState(state);
      if (co_await checkForFailedInst(instPkgs))
         _lister->restoreState(state);
   }
   setBusyCursor(false);
   _lister->notifyPostChange(NULL);
   _lister->notifyCachePostChange();

   RPackage *pkg = selectedPackage();
   refreshTable(pkg);
   updatePackageInfo(pkg);
}

void RGMainWindow::pkgInstallByNameHelper(GSimpleAction *action,
                                          GVariant *parameter,
                                          gpointer data)
{
   const char *name = g_variant_get_string(parameter, nullptr);
   // cout << "pkgInstallByNameHelper: " << name << endl;

   RGMainWindow *me = (RGMainWindow *)data;
   start_task(
      [me, name]() -> task<void> { co_await me->pkgInstallByName(name); });
}

task<void> RGMainWindow::pkgInstallByName(const char *name)
{
   RPackage *newpkg = (RPackage *)_lister->getPackage(name);
   if (newpkg) {
      RPackageLister::pkgState state;
      vector<RPackage *> exclude;
      vector<RPackage *> instPkgs;

      // we always save the state (for undo)
      _lister->saveState(state);
      _lister->notifyCachePreChange();

      // actual action
      newpkg->setNotify(false);
      pkgInstallHelper(newpkg);
      newpkg->setNotify(true);

      exclude.push_back(newpkg);
      instPkgs.push_back(newpkg);

      // ask for additional changes
      if (co_await askStateChange(state, exclude)) {
         _lister->saveUndoState(state);
         if (co_await checkForFailedInst(instPkgs))
            _lister->restoreState(state);
      }
      _lister->notifyPostChange(NULL);
      _lister->notifyCachePostChange();

      RPackage *pkg = selectedPackage();
      refreshTable(pkg);
      updatePackageInfo(pkg);
   }
}

void RGMainWindow::cbGenerateDownloadScriptClicked(GSimpleAction *action,
                                                   GVariant *parameter,
                                                   gpointer data)
{
   // cout << "cbGenerateDownloadScriptClicked()" << endl;
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->generateDownloadScript(); });
}

task<void> RGMainWindow::generateDownloadScript()
{
   int installed, broken, toInstall, toRemove;
   double sizeChange;
   _lister->getStats(installed, broken, toInstall, toRemove, sizeChange);
   if (toInstall == 0) {
      co_await _userDialog->message(
         "Nothing to install/upgrade\n\n"
         "Please select the \"Mark all Upgrades\" "
         "button or some packages to install/upgrade.");
      co_return;
   }

   vector<string> uris;
   if (!_lister->getDownloadUris(uris))
      co_return;

   GtkWidget *filesel;
   filesel = gtk_file_chooser_dialog_new(_("Save script"),
                                         GTK_WINDOW(window()),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         _("_Cancel"),
                                         GTK_RESPONSE_CANCEL,
                                         _("_Save"),
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);
   int res = co_await co_run_dialog(GTK_DIALOG(filesel));
   GFile *selected_file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(filesel));
   gtk_window_destroy(GTK_WINDOW(filesel));
   if (res != GTK_RESPONSE_ACCEPT) {
      g_object_unref(selected_file);
      co_return;
   }

   char *file = g_file_get_path(selected_file);
   g_object_unref(selected_file);

   // FIXME: this is prototype code, hardcoding wget here suckx
   ofstream out(file);
   out << "#!/bin/sh" << endl;
   for (size_t i = 0; i < uris.size(); i++) {
      out << "wget -c " << uris[i] << endl;
   }
   chmod(file, 0755);
   g_free(file);
}

void RGMainWindow::cbAddDownloadedFilesClicked(GSimpleAction *action,
                                               GVariant *parameter,
                                               gpointer data)
{
   RGMainWindow *me = (RGMainWindow *)data;
   start_task([me]() -> task<void> { co_await me->addDownloadedFiles(); });
}

task<void> RGMainWindow::addDownloadedFiles()
{
#ifndef HAVE_RPM
   GtkWidget *filesel;
   filesel = gtk_file_chooser_dialog_new(_("Select directory"),
                                         GTK_WINDOW(window()),
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         _("_Cancel"),
                                         GTK_RESPONSE_CANCEL,
                                         _("_Open"),
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);
   int res = co_await co_run_dialog(GTK_DIALOG(filesel));
   GFile *selected_file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(filesel));
   gtk_window_destroy(GTK_WINDOW(filesel));
   if (res != GTK_RESPONSE_ACCEPT) {
      g_object_unref(selected_file);
      co_return;
   }
   GFileType file_type = g_file_query_file_type(
      selected_file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
   if (file_type != G_FILE_TYPE_DIRECTORY) {
      co_await _userDialog->error(_("Please select a directory"));
      g_object_unref(selected_file);
      co_return;
   }

   // now read the dir for debs
   char *path = g_file_get_path(selected_file);
   const gchar *file;
   string pkgname;
   stringstream pkgs;
   GDir *dir = g_dir_open(path, 0, NULL);
   while ((file = g_dir_read_name(dir)) != NULL) {
      if (g_pattern_match_simple("*_*.deb", file)) {
         if (_lister->addArchiveToCache(string(path) + "/" + string(file),
                                        pkgname))
            pkgs << pkgname << "\t install" << endl;
      }
   }
   g_dir_close(dir);
   g_free(path);
   g_object_unref(selected_file);

   // and set what we found as selection
   pkgs.seekg(0);
   if (pkgs.str() == "")
      co_return;

   _lister->unregisterObserver(this);
   _lister->readSelections(pkgs);
   _lister->registerObserver(this);
   refreshTable();

   // show any errors
   co_await _userDialog->showErrors();

   // click proceed
   co_await applyChanges();
#else
   co_await _userDialog->error(
      "Sorry, not implemented for rpm, patches welcome");
#endif
}
