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

#include "config.h"

#include <cassert>
#include <stdio.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <apt-pkg/strutl.h>
#include <apt-pkg/fileutl.h>
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
#include "rgpkgcdrom.h"
#include "rgsetoptwindow.h"
#include "rgchangelogdialog.h"
#include "rgfetchprogress.h"
#include "rgpkgdetails.h"
#include "rgcacheprogress.h"
#include "rguserdialog.h"
#include "rginstallprogress.h"
#include "rgdummyinstallprogress.h"
#include "rgdebinstallprogress.h"
#include "rgterminstallprogress.h"
#include "rgutils.h"
#include "sections_trans.h"
#include "rgpkgtreeview.h"

#include "i18n.h"

// include it here because depcache.h hates us if we have it before
#include <gdk/gdkx.h>

const char *relOptions[] = {
   N_("Dependencies"),
   N_("Dependants"),
   N_("Dependencies of the Latest Version"),
   N_("Provided Packages"),
   NULL
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

GtkCssProvider *RGMainWindow::_fastSearchCssProvider = NULL;

void RGMainWindow::changeView(int view, string subView)
{
   if(_config->FindB("Debug::Synaptic::View",false))
      ioprintf(clog, "RGMainWindow::changeView(): view '%i' subView '%s'\n", 
	       view, subView.size() > 0 ? subView.c_str() : "(empty)");

   if(view >= N_PACKAGE_VIEWS) {
      //cerr << "changeView called with invalid view NR: " << view << endl;
      view=0;
   }

   _blockActions = TRUE;
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_viewButtons[view]), TRUE);

   // we need to set a empty model first so that gtklistview
   // can do its cleanup, if we do not do that, then the cleanup
   // code in gtktreeview gets confused and throws
   // Gtk-CRITICAL **: gtk_tree_view_unref_tree_helper: assertion `node != NULL' failed
   // at us, see LP: #38397 for more information
   gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), NULL);
      
   RPackage *pkg = selectedPackage();

   _lister->setView(view);

   refreshSubViewList();

   GtkTreeSelection* selection;
   setBusyCursor(true);
   setInterfaceLocked(TRUE);
   GtkWidget *tview = GTK_WIDGET(gtk_builder_get_object(_builder, "treeview_subviews"));
   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tview));
   if(!subView.empty()) {
      GtkTreeModel *model;
      GtkTreeIter iter;
      char *str;

      model = gtk_tree_view_get_model(GTK_TREE_VIEW(tview));
      if(gtk_tree_model_get_iter_first(model, &iter)) {
         do {
            gtk_tree_model_get(model, &iter, 0, &str, -1);
            if(strcoll(str, MarkupEscapeString(subView).c_str()) == 0) {
               gtk_tree_selection_select_iter(selection, &iter);
               break;
            }
         } while(gtk_tree_model_iter_next(model, &iter));
      }
   } else {
      GtkTreePath * path = gtk_tree_path_new_from_string( "0" );
      gtk_tree_selection_select_path( selection, path );
   }
   _lister->setSubView(subView);
   refreshTable(pkg,false);
   setInterfaceLocked(FALSE);     
   setBusyCursor(false);
   _blockActions = FALSE;
   setStatusText();
}

void RGMainWindow::refreshSubViewList()
{
   string selected = selectedSubView();
   if(_config->FindB("Debug::Synaptic::View",false))
      ioprintf(clog, "RGMainWindow::refreshSubViewList(): selectedView '%s'\n", 
	       selected.size() > 0 ? selected.c_str() : "(empty)");

   vector<string> subViews = _lister->getSubViews();

   for(unsigned int i=0; i<subViews.size(); i++)
       subViews[i] = MarkupEscapeString(subViews[i]);

   gchar *str = g_strdup_printf("<b>%s</b>", _("All"));
   subViews.insert(subViews.begin(), str);
   g_free(str);
   setTreeList("treeview_subviews", subViews, true);

   if(!selected.empty()) {
      GtkTreeSelection *selection;
      GtkTreeModel *model;
      GtkTreeIter iter;
      const char *str = NULL;

      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(_subViewList));
      bool ok =  gtk_tree_model_get_iter_first(model, &iter); 
      while(ok) {
	 gtk_tree_model_get(model, &iter, 0, &str, -1);
	 if(str && strcoll(str, selected.c_str()) == 0) {
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
   static string ret = "(no selection)";

   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
   if (selection != NULL) {
      if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
         gtk_tree_model_get(model, &iter, 0, &subView, -1);
	 
	 // check if first item is selected ("All")
	 gchar *str=gtk_tree_model_get_string_from_iter(model, &iter);
	 if(str[0] == '0' || subView == NULL)
	    ret = "";
	 else
	    ret = subView;
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
   if(_config->FindB("Debug::Synaptic::View",false))
      ioprintf(clog, "RGMainWindow::notifyChange(): '%s'\n",
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
      if (elem->getFlags() && RPackage::FNew)
         elem->setNew(false);
   }
   _roptions->forgetNewPackages();
}

void RGMainWindow::refreshTable(RPackage *selectedPkg, bool setAdjustment)
{
   if(_config->FindB("Debug::Synaptic::View",false))
      ioprintf(clog, "RGMainWindow::refreshTable(): pkg: '%s' adjust '%i'\n", 
	       selectedPkg != NULL ? selectedPkg->name() : "(no pkg)", 
	       setAdjustment);

   const gchar *str = gtk_entry_get_text(GTK_ENTRY(_entry_fast_search));
   if(str != NULL && strlen(str) > 1) {
      if(_config->FindB("Debug::Synaptic::View",false))
	 cerr << "RGMainWindow::refreshTable: rerun limitBySearch" << endl;
      _lister->limitBySearch(str);
   }

   if(_pkgList == NULL)
   {
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

   //cout << "RGMainWindow::updatePackageInfo(): " << pkg << endl;

   // get required widgets from gtkbuilder
   GtkWidget *pkginfo = GTK_WIDGET(gtk_builder_get_object(_builder, "notebook_pkginfo"));
   assert(pkginfo);

   // set everything to non-sensitive (for both pkg != NULL && pkg == NULL)
   gtk_widget_set_sensitive(_keepM, FALSE);
   gtk_widget_set_sensitive(_installM, FALSE);
   gtk_widget_set_sensitive(_reinstallM, FALSE);
   gtk_widget_set_sensitive(_pkgupgradeM, FALSE);
   gtk_widget_set_sensitive(_removeM, FALSE);
   gtk_widget_set_sensitive(_purgeM, FALSE);
   gtk_widget_set_sensitive(_pkgReconfigureM, FALSE);
   gtk_widget_set_sensitive(_pkgHelpM, FALSE);
   gtk_widget_set_sensitive(pkginfo, FALSE);
   gtk_widget_set_sensitive(_dl_changelogM, FALSE);
   gtk_widget_set_sensitive(_detailsM, FALSE);
   gtk_widget_set_sensitive(_propertiesB, FALSE);
   gtk_widget_set_sensitive(_overrideVersionM, FALSE);
   gtk_widget_set_sensitive(_pinM, FALSE);
   gtk_widget_set_sensitive(_autoM, FALSE);
   gtk_text_buffer_set_text(_pkgCommonTextBuffer,
			    _("No package is selected.\n"), -1);

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
   gtk_widget_set_sensitive(_dl_changelogM, TRUE);
   gtk_widget_set_sensitive(_detailsM, TRUE);
   gtk_widget_set_sensitive(_propertiesB, TRUE);
   // activate for root only
   if(getuid() == 0) {
       gtk_widget_set_sensitive(_pinM, TRUE);
       gtk_widget_set_sensitive(_autoM, TRUE);
   }    

   // set info
   gtk_widget_set_sensitive(pkginfo, true);
   RGPkgDetailsWindow::fillInValues(this, pkg);
   // work around a stupid gtk-bug (see debian #279447)
   gtk_widget_queue_resize(GTK_WIDGET(gtk_builder_get_object
                                      (_builder, "viewport_pkginfo")));

   if(_pkgDetails != NULL)
      RGPkgDetailsWindow::fillInValues(_pkgDetails,pkg, true);

   // Pin, if a pin is set, we skip all other checks and return
   if( flags & RPackage::FPinned) {
      _blockActions = TRUE;
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(_pinM), true);
      _blockActions = FALSE;
      return;
   } else {
      _blockActions = TRUE;
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(_pinM), false);
      _blockActions = FALSE;
   }

   // Auto-Flag
   _blockActions = true;
   if( flags & RPackage::FIsAuto)
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(_autoM), true);
   else
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(_autoM), false);
   _blockActions = false;

   // enable unmark if a action is performed with the pkg
   if((flags & RPackage::FInstall)   || (flags & RPackage::FNewInstall) || 
      (flags & RPackage::FReInstall) || (flags & RPackage::FUpgrade) || 
      (flags & RPackage::FDowngrade) || (flags & RPackage::FRemove) || 
      (flags & RPackage::FPurge))
      gtk_widget_set_sensitive(_keepM, TRUE);
   // enable install if outdated or not insalled
   if(!(flags & RPackage::FInstalled))
      gtk_widget_set_sensitive(_installM, TRUE);
   // enable reinstall if installed and installable and not outdated
   if(flags & RPackage::FInstalled 
      && !(flags & RPackage::FNotInstallable)
      && !(flags & RPackage::FOutdated))
      gtk_widget_set_sensitive(_reinstallM, TRUE);
   // enable upgrade is outdated
   if(flags & RPackage::FOutdated)
      gtk_widget_set_sensitive(_pkgupgradeM, TRUE);
   // enable remove if package is installed
   if(flags & RPackage::FInstalled)
      gtk_widget_set_sensitive(_removeM, TRUE);

   // enable purge if package is installed or has residual config
   if(flags & RPackage::FInstalled || flags & RPackage::FResidualConfig)
      gtk_widget_set_sensitive(_purgeM, TRUE);
   // enable help if package is installed
   if( flags & RPackage::FInstalled)
      gtk_widget_set_sensitive(_pkgHelpM, TRUE);
   // enable debconf if package is installed and depends on debconf
   if( flags & RPackage::FInstalled && (pkg->dependsOn("debconf") || 
					pkg->dependsOn("debconf-i18n")))
       gtk_widget_set_sensitive(_pkgReconfigureM, TRUE);

   if(pkg->getAvailableVersions().size() > 1)
      gtk_widget_set_sensitive(_overrideVersionM, TRUE);

}

void RGMainWindow::cbDependsMenuChanged(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *)data;

   int nr =  gtk_combo_box_get_active(GTK_COMBO_BOX(self));
   GtkWidget *notebook = GTK_WIDGET(gtk_builder_get_object
                                    (me->_builder, "notebook_dep_tab"));
   assert(notebook);
   gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), nr);
}

void RGMainWindow::cbMenuAutoInstalledClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   if (me->_blockActions)
      return;
   
   bool active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self));

   GtkTreeSelection *selection;
   GtkTreeIter iter;
   GList *list, *li;
   RPackage *pkg;

   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_treeView));
   list = li = gtk_tree_selection_get_selected_rows(selection, &me->_pkgList);
   while (li != NULL) {
      gtk_tree_model_get_iter(me->_pkgList, &iter, (GtkTreePath *) (li->data));
      gtk_tree_model_get(me->_pkgList, &iter, PKG_COLUMN, &pkg, -1);
      if (pkg == NULL) {
         li = g_list_next(li);
         continue;
      }

      pkg->setAuto(active);
      li = g_list_next(li);
   }

   // write it
   GtkWidget *progress = GTK_WIDGET(gtk_builder_get_object
                                    (me->_builder, "progressbar_main"));
   GtkWidget *label = GTK_WIDGET(gtk_builder_get_object
                                 (me->_builder, "label_status"));
   RGCacheProgress cacheProgress(progress, label);
   me->_lister->getCache()->deps()->writeStateFile(&cacheProgress,true);

   // refresh
   me->setInterfaceLocked(TRUE);
   me->_lister->unregisterObserver(me);

   me->_lister->getCache()->deps()->MarkAndSweep();
   me->_lister->refreshView();

   me->_lister->registerObserver(me);
   me->refreshTable();
   me->refreshSubViewList();
   me->setInterfaceLocked(FALSE);
   
}

// install a specific version
void RGMainWindow::cbInstallFromVersion(GtkWidget *self, void *data)
{
   //cout << "RGMainWindow::cbInstallFromVersion()" << endl;

   RGMainWindow *me = (RGMainWindow *) data;
   RPackage *pkg = me->selectedPackage();
   if(pkg == NULL)
      return;

   RGGtkBuilderUserDialog dia(me,"change_version");

   GtkWidget *label = GTK_WIDGET(gtk_builder_get_object(dia.getGtkBuilder(),
					                "label_text"));
   gchar *str_name = g_strdup_printf(_("Select the version of %s that should be forced for installation"), pkg->name());
   gchar *str = g_strdup_printf("<big><b>%s</b></big>\n\n%s", str_name,
				_("The package manager always selects the most applicable version available. If you force a different version from the default one, errors in the dependency handling can occur."));
   gtk_label_set_markup(GTK_LABEL(label), str);
   g_free(str_name);
   g_free(str);
   
   GtkWidget *available_versions_combo = GTK_WIDGET(gtk_builder_get_object
                                                    (dia.getGtkBuilder(),
                                                     "combobox_available_versions"));
   int canidateNr = 0;
   vector<pair<string, string> > versions = pkg->getAvailableVersions();
   for(unsigned int i=0;i<versions.size();i++) {
      gchar *str = g_strdup_printf("%s (%s)", 
				   versions[i].first.c_str(), 
				   versions[i].second.c_str() );
      const char *verStr = pkg->availableVersion();
      if(verStr && versions[i].first == string(verStr))
         canidateNr = i;
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(available_versions_combo),
                                     str);
      //cout << "got: " << str << endl;
      g_free(str);
   }
   gtk_combo_box_set_active(GTK_COMBO_BOX(available_versions_combo), 
                            canidateNr);
   if(!dia.run()) {
      //cout << "cancel" << endl;
      return;    // user clicked cancel
   }

   int nr = gtk_combo_box_get_active(GTK_COMBO_BOX(available_versions_combo));

   pkg->setNotify(false);
   // nr-1 here as we add a "do not override" to the option menu
   pkg->setVersion(versions[nr].first.c_str());
   me->pkgAction(PKG_INSTALL_FROM_VERSION);
   

   if (!(pkg->getFlags() & RPackage::FInstall))
      pkg->unsetVersion();   // something went wrong

   pkg->setNotify(true);
}

bool RGMainWindow::askStateChange(RPackageLister::pkgState state,
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
   if (ask && _lister->getStateChanges(state, toKeep, toInstall, toReInstall,
				       toUpgrade, toRemove, toDowngrade,
				       notAuthenticated, exclude)) {
      RGChangesWindow changes(this);
      changes.confirm(_lister, toKeep, toInstall, toReInstall,
		      toUpgrade, toRemove, toDowngrade, notAuthenticated);
      int res = gtk_dialog_run(GTK_DIALOG(changes.window()));
      if( res != GTK_RESPONSE_OK) {
         // canceled operation
         _lister->restoreState(state);
	 // if a operation was canceled, we discard all errors from this
	 // operation too
	 _error->Discard();
         changed = false;
      }
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
      gtk_tree_model_get_iter(_pkgList, &iter, (GtkTreePath *) (li->data));
      gtk_tree_model_get(_pkgList, &iter, PKG_COLUMN, &pkg, -1);
      li = g_list_next(li);
      if (pkg == NULL)
         continue;

      flags = pkg->getFlags();

      pkg->setNotify(false);

      // needed for the stateChange 
      exclude.push_back(pkg);
      switch (action) {
         case PKG_KEEP:        // keep
            pkgKeepHelper(pkg);
            break;
         case PKG_INSTALL:     // install
            // install only if not installed or outdated (upgrade)
            if(!(flags & RPackage::FInstalled) 
               || (flags & RPackage::FOutdated)) {
               instPkgs.push_back(pkg);
               pkgInstallHelper(pkg, false);
            }
            break;
         case PKG_INSTALL_FROM_VERSION:     // install with specific version
            pkgInstallHelper(pkg, false);
            break;
         case PKG_REINSTALL:      // reinstall
            // Only reinstall installable packages and non outdated packages
            if(flags & RPackage::FInstalled 
               && !(flags & RPackage::FNotInstallable)
               && !(flags & RPackage::FOutdated)) {
               instPkgs.push_back(pkg);
               pkgInstallHelper(pkg, false, true);
            }
            break;
         case PKG_DELETE:      // delete
            if(flags & RPackage::FInstalled)
               pkgRemoveHelper(pkg);
            break;
         case PKG_PURGE:       // purge
            if(flags & RPackage::FInstalled || flags & RPackage::FResidualConfig)
               pkgRemoveHelper(pkg, true);
            break;
         case PKG_DELETE_WITH_DEPS:
            if(flags & RPackage::FInstalled || flags & RPackage::FResidualConfig)
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
      bool failed=false;
      // check for failed installs, if a installs fails, restore old state
      // as the Fixer may do wired thinks when trying to resolve the problem
      if (action == PKG_INSTALL) {
	 failed = checkForFailedInst(instPkgs);
	 if(failed)
	    _lister->restoreState(state);
      }
      // if everything is fine, save it as new undo state 
      if(!failed)
	 _lister->saveUndoState(state);
   }

   if (ask)
      _lister->registerObserver(this);

   g_list_foreach(list, (void (*)(void *, void *))gtk_tree_path_free, NULL);
   g_list_free(list);

   refreshSubViewList();
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
      RGGtkBuilderUserDialog dia(this,"unmet");
      GtkWidget *tv = GTK_WIDGET(gtk_builder_get_object(dia.getGtkBuilder(),
					                "textview"));
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
   : RGGtkBuilderWindow(NULL, name), _lister(packLister), _pkgList(0), 
     _treeView(0), _tasksWin(0), _iconLegendPanel(0), _pkgDetails(0),
     _logView(0), _installProgress(0), _fetchProgress(0), 
     _fastSearchEventID(-1)
{
   assert(_win);

   _blockActions = false;
   _unsavedChanges = false;
   _interfaceLocked = 0;

   _lister->registerObserver(this);

   _toolbarStyle = (GtkToolbarStyle) _config->FindI("Synaptic::ToolbarState",
                                                    (int)GTK_TOOLBAR_BOTH);

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

   xapianDoIndexUpdate(this);

   // apply the proxy settings
   RGPreferencesWindow::applyProxySettings();
}

#ifdef WITH_EPT
gboolean RGMainWindow::xapianDoIndexUpdate(void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   if(_config->FindB("Debug::Synaptic::Xapian",false))
      std::cerr << "xapianDoIndexUpdate()" << std::endl;

   // no need to update if we run non-interactive
   if(_config->FindB("Volatile::Non-Interactive", false) == true)
      return false;

   // check if we need a update
   if(!me->_lister->xapianIndexNeedsUpdate()) {
      // if the cache is not open, check back when it is
      if (me->_lister->packagesSize() == 0)
	 g_timeout_add_seconds(30, xapianDoIndexUpdate, me);
      return false;
   }

   // do not run if we don't have it
   if(!FileExists("/usr/sbin/update-apt-xapian-index"))
      return false;
   // no permission
   if (getuid() != 0)
      return false;

   // if we make it to this point, we need a xapian update
   if(_config->FindB("Debug::Synaptic::Xapian",false))
      std::cerr << "running update-apt-xapian-index" << std::endl;
   GPid pid;
   const char *argp[] = {"/usr/bin/nice",
		   "/usr/bin/ionice","-c3",
		   "/usr/sbin/update-apt-xapian-index", 
		   "--update", "-q",
		   NULL};
   if(g_spawn_async(NULL, const_cast<char **>(argp), NULL, 
		    (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD),
		    NULL, NULL, &pid, NULL)) {
      g_child_watch_add(pid,  (GChildWatchFunc)xapianIndexUpdateFinished, me);
      gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(me->_builder, 
							  "label_fast_search")),
			 _("Rebuilding search index"));
   }
   return false;
}
#else
gboolean RGMainWindow::xapianDoIndexUpdate(void *data)
{
   return false;
}
#endif

void RGMainWindow::xapianIndexUpdateFinished(GPid pid, gint status, void* data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   if(_config->FindB("Debug::Synaptic::Xapian",false))
      std::cerr << "xapianIndexUpdateFinished: "  
		<< WEXITSTATUS(status) << std::endl;
#ifdef WITH_EPT
   me->_lister->openXapianIndex();
#endif
   gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(me->_builder, 
						     "label_fast_search")),
		      _("Quick filter"));
   gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object
                            (me->_builder, "entry_fast_search")), TRUE);
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
      for (GList * li = g_list_first(columns); 
           li != NULL;
           li = g_list_next(li)) {
         int i = gtk_tree_view_remove_column(GTK_TREE_VIEW(_treeView),
                                             GTK_TREE_VIEW_COLUMN(li->data));
      }
      // need to free the list here
      g_list_free(columns);
   }

   _treeView = GTK_WIDGET(gtk_builder_get_object
                          (_builder, "treeview_packages"));
   assert(_treeView);
   setupTreeView(_treeView);
   _pkgList = GTK_TREE_MODEL(gtk_pkg_list_new(_lister));
   gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), _pkgList);

}

void RGMainWindow::buildInterface()
{
   GtkWidget *img, *menuitem, *widget, *button;

   // here is a pointer to rgmainwindow for every widget that needs it
   g_object_set_data(G_OBJECT(_win), "me", this);

   GdkPixbuf *icon = get_gdk_pixbuf( "synaptic" );
   gtk_window_set_icon(GTK_WINDOW(_win), icon);

   gtk_window_resize(GTK_WINDOW(_win),
                     _config->FindI("Synaptic::windowWidth", 640),
                     _config->FindI("Synaptic::windowHeight", 480));
   gtk_window_move(GTK_WINDOW(_win),
                   _config->FindI("Synaptic::windowX", 100),
                   _config->FindI("Synaptic::windowY", 100));
   if(_config->FindB("Synaptic::Maximized",false))
      gtk_window_maximize(GTK_WINDOW(_win));
   RGFlushInterface();

   if (_fastSearchCssProvider == NULL) {
      _fastSearchCssProvider = gtk_css_provider_new();
      gtk_css_provider_load_from_data(_fastSearchCssProvider,
                                      "GtkEntry:not(:selected) { background: #F7F7BE; }", -1, NULL);
   }

   g_signal_connect(gtk_builder_get_object(_builder, "menu_about"),
                    "activate",
                    G_CALLBACK(cbShowAboutPanel), this);

   g_signal_connect(gtk_builder_get_object(_builder, "quick_introduction"),
                    "activate",
                    G_CALLBACK(cbShowWelcomeDialog), this);


   g_signal_connect(gtk_builder_get_object(_builder, "icon_legend"),
                    "activate",
                    G_CALLBACK(cbShowIconLegendPanel), this);

   g_signal_connect(gtk_builder_get_object(_builder, "menu_help"),
                    "activate",
                    G_CALLBACK(cbHelpAction), this);

   g_signal_connect(gtk_builder_get_object(_builder, "button_update"),
                    "clicked",
                    G_CALLBACK(cbUpdateClicked), this);
   g_signal_connect(gtk_builder_get_object(_builder, "menu_update_packages"),
                    "activate",
                    G_CALLBACK(cbUpdateClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "button_details"),
                    "clicked",
                    G_CALLBACK(cbDetailsWindow), this);
   g_signal_connect(gtk_builder_get_object(_builder, "entry_fast_search"),
                    "changed",
                    G_CALLBACK(cbSearchEntryChanged), this);

   _propertiesB = GTK_WIDGET(gtk_builder_get_object(_builder, "button_details"));
   assert(_propertiesB);
   _upgradeB = GTK_WIDGET(gtk_builder_get_object(_builder, "button_upgrade"));
   gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(_upgradeB), "system-upgrade");
   _upgradeM = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_upgrade_all"));
   g_signal_connect(G_OBJECT(_upgradeB),
                    "clicked",
                    G_CALLBACK(cbUpgradeClicked), this);
   g_signal_connect(G_OBJECT(_upgradeM),
                    "activate",
                    G_CALLBACK(cbUpgradeClicked), this);

   if (_config->FindB("Synaptic::NoUpgradeButtons", false) == true) {
      gtk_widget_hide(_upgradeB);
      widget = GTK_WIDGET(gtk_builder_get_object(_builder, "alignment_upgrade"));
      gtk_widget_hide(widget);
   }

   _proceedB = GTK_WIDGET(gtk_builder_get_object(_builder, "button_procceed"));
   _proceedM = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_proceed"));
   g_signal_connect(G_OBJECT(_proceedB),
                    "clicked",
                    G_CALLBACK(cbProceedClicked), this);
   g_signal_connect(G_OBJECT(_proceedM),
                    "activate",
                    G_CALLBACK(cbProceedClicked), this);

   _fixBrokenM = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_fix_broken_packages"));
   g_signal_connect(G_OBJECT(_fixBrokenM),
                    "activate",
                    G_CALLBACK(cbFixBrokenClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "menu_preferences"),
                    "activate",
                    G_CALLBACK(cbShowConfigWindow), this);

   g_signal_connect(gtk_builder_get_object(_builder, "menu_set_option"),
                    "activate",
                    G_CALLBACK(cbShowSetOptWindow), this);

   g_signal_connect(gtk_builder_get_object(_builder, "menu_repositories"),
                    "activate",
                    G_CALLBACK(cbShowSourcesWindow), this);

   g_signal_connect(gtk_builder_get_object(_builder, "menu_exit"),
                    "activate",
                    G_CALLBACK(closeWin), this);

   g_signal_connect(gtk_builder_get_object(_builder, "edit_filter"),
                    "activate",
                    G_CALLBACK(cbShowFilterManagerWindow), this);

   _pkgHelpM = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_documentation"));
   assert(_pkgHelpM);
   g_signal_connect(G_OBJECT(_pkgHelpM),
                    "activate",
                    G_CALLBACK(cbPkgHelpClicked), this);

   _pkgReconfigureM = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_configure"));
   assert(_pkgReconfigureM);
   g_signal_connect(G_OBJECT(_pkgReconfigureM),
                    "activate",
                    G_CALLBACK(cbPkgReconfigureClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "button_search"),
                    "clicked",
                    G_CALLBACK(cbFindToolClicked), this);
   g_signal_connect(gtk_builder_get_object(_builder, "menu_search"),
                    "activate",
                    G_CALLBACK(cbFindToolClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "undo1"),
                    "activate",
                    G_CALLBACK(cbUndoClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "redo1"),
                    "activate",
                    G_CALLBACK(cbRedoClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "clear_all_changes"),
                    "activate",
                    G_CALLBACK(cbClearAllChangesClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "menu_tasks"),
                    "activate",
                    G_CALLBACK(cbTasksClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "menu_open"),
                    "activate",
                    G_CALLBACK(cbOpenClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "menu_save"),
                    "activate",
                    G_CALLBACK(cbSaveClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "view_commit_log"),
                    "activate",
                    G_CALLBACK(cbViewLogClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "menu_save_as"),
                      "activate",
                      G_CALLBACK(cbSaveAsClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "generate_download_script1"),
                    "activate",
                    G_CALLBACK(cbGenerateDownloadScriptClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "menu_add_downloadedfiles"),
                    "activate",
                    G_CALLBACK(cbAddDownloadedFilesClicked), this);

   widget = _detailsM = GTK_WIDGET(gtk_builder_get_object
                                   (_builder, "menu_details"));
   assert(_detailsM);
   g_object_set_data(G_OBJECT(widget), "me", this);
   g_signal_connect(G_OBJECT(_detailsM),
                    "activate",
                    G_CALLBACK(cbDetailsWindow), this);

   widget = _keepM = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_keep"));
   assert(_keepM);
   g_object_set_data(G_OBJECT(widget), "me", this);

   widget = _installM = GTK_WIDGET(gtk_builder_get_object
                                   (_builder, "menu_install"));
   assert(_installM);
   g_object_set_data(G_OBJECT(widget), "me", this);

   widget = _reinstallM = GTK_WIDGET(gtk_builder_get_object
                                     (_builder, "menu_reinstall"));
   assert(_reinstallM);
   g_object_set_data(G_OBJECT(widget), "me", this);

   widget = _pkgupgradeM = GTK_WIDGET(gtk_builder_get_object
                                      (_builder, "menu_upgrade"));
   assert(_pkgupgradeM);
   g_object_set_data(G_OBJECT(widget), "me", this);

   widget = _removeM = GTK_WIDGET(gtk_builder_get_object
                                  (_builder, "menu_remove"));
   assert(_removeM);
   g_object_set_data(G_OBJECT(widget), "me", this);

   widget = _purgeM = GTK_WIDGET(gtk_builder_get_object
                                 (_builder, "menu_purge"));
   assert(_purgeM);
   g_object_set_data(G_OBJECT(widget), "me", this);

#if 0
   _remove_w_depsM = GTK_WIDGET(gtk_builder_get_object
                                (_builder, "menu_remove_with_deps"));
   assert(_remove_w_depsM);
#endif

   _dl_changelogM = GTK_WIDGET(gtk_builder_get_object
                               (_builder, "menu_download_changelog"));
   assert(_dl_changelogM);
#ifdef HAVE_RPM
   gtk_widget_hide(_purgeM);
   gtk_widget_hide(_pkgReconfigureM);
   gtk_widget_hide(_pkgHelpM);
   gtk_widget_hide(_dl_changelogM);
   gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object
                              (_builder, "menu_changelog_separator")));
   gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object
                              (_builder, "separator_debian")));
#endif
   
   if(!FileExists(_config->Find("Synaptic::taskHelperProg","/usr/bin/tasksel")))
      gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(_builder, "menu_tasks")));

   button = GTK_WIDGET(gtk_builder_get_object(_builder, "button_update"));
   gtk_widget_set_tooltip_text(button,
                               _("Reload the package information to become "
                                 "informed about new, removed or upgraded "
                                 "software packages."));

   button = GTK_WIDGET(gtk_builder_get_object(_builder, "button_upgrade"));
   gtk_widget_set_tooltip_text(button,
                               _("Mark all possible upgrades"));

   button = GTK_WIDGET(gtk_builder_get_object(_builder, "button_procceed"));
   gtk_widget_set_tooltip_text(button,
                               _("Apply all marked changes"));

   button = GTK_WIDGET(gtk_builder_get_object(_builder, "button_details"));
   gtk_widget_set_tooltip_text(button,
                               _("View package properties"));

   button = GTK_WIDGET(gtk_builder_get_object(_builder, "button_search"));
   gtk_widget_set_tooltip_text(button,
                               _("Search for packages"));

   GtkWidget *pkgCommonTextView;
   pkgCommonTextView = GTK_WIDGET(gtk_builder_get_object(_builder, "text_descr"));
   assert(pkgCommonTextView);
   _pkgCommonTextBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pkgCommonTextView));

   g_signal_connect(gtk_builder_get_object(_builder, "menu_keep"),
                    "activate",
                    G_CALLBACK(cbPkgAction), GINT_TO_POINTER(PKG_KEEP));

   g_signal_connect(gtk_builder_get_object(_builder, "menu_install"),
                    "activate",
                    G_CALLBACK(cbPkgAction), GINT_TO_POINTER(PKG_INSTALL));

   // callback same as for install
   widget = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_upgrade"));
   assert(widget);
   g_object_set_data(G_OBJECT(widget), "me", this);

   widget = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_reinstall"));
   assert(widget);
   g_object_set_data(G_OBJECT(widget), "me", this);
   g_signal_connect(G_OBJECT(widget),
                      "activate",
                      G_CALLBACK(cbPkgAction), GINT_TO_POINTER(PKG_REINSTALL));
   
   g_signal_connect(gtk_builder_get_object(_builder, "menu_remove"),
                    "activate",
                    G_CALLBACK(cbPkgAction), GINT_TO_POINTER(PKG_DELETE));
#if 0
   widget = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_remove_with_deps"));
   assert(widget);
   g_object_set_data(G_OBJECT(widget), "me", this);
   g_signal_connect(G_OBJECT(widget),
                    "activate",
                    G_CALLBACK(cbPkgAction),
                                 GINT_TO_POINTER(PKG_DELETE_WITH_DEPS));
#endif

   widget = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_purge"));
   assert(widget);
   g_signal_connect(G_OBJECT(widget),
                    "activate",
                    G_CALLBACK(cbPkgAction), GINT_TO_POINTER(PKG_PURGE));

   _pinM = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_hold"));
   g_signal_connect(G_OBJECT(_pinM),
                    "activate",
                    G_CALLBACK(cbMenuPinClicked), this);

   _autoM = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_auto_installed"));
   g_signal_connect(G_OBJECT(_autoM),
                    "activate",
                    G_CALLBACK(cbMenuAutoInstalledClicked), this);

   _overrideVersionM = GTK_WIDGET(gtk_builder_get_object(_builder, 
                                  "menu_override_version"));
   assert(_overrideVersionM);
   g_signal_connect(G_OBJECT(_overrideVersionM),
                    "activate",
                    G_CALLBACK(cbInstallFromVersion), this);


   // only if pkg help is enabled
#ifndef SYNAPTIC_PKG_HOLD
   gtk_widget_hide(_pinM);
//    widget = GTK_WIDGET(gtk_builder_get_object(_builder, "separator_hold"));
//    if (widget != NULL)
//       gtk_widget_hide(widget);
#endif

   // soc
   GtkWidget *notebook = GTK_WIDGET(gtk_builder_get_object
                                   (_builder, "notebook_pkginfo"));
   if(_config->FindB("Synaptic::ShowAllPkgInfoInMain", false)) {
      gtk_widget_set_margin_top(notebook, 6);
      gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), TRUE);
      gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(_builder, "button_details")));
   } else {
      gtk_widget_set_margin_top(notebook, 0);
      gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
      gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(_builder, "button_details")));
   }
#ifndef HAVE_RPM
   gtk_widget_show(GTK_WIDGET(gtk_builder_get_object
                              (_builder, "scrolledwindow_filelist")));
#endif

   // Handle the combobox stuff for dependencies in PpkgInfoInMain mode
   // ourselves
   GtkWidget *comboDepends = GTK_WIDGET(gtk_builder_get_object
                                        (_builder, "combobox_depends"));
   g_signal_connect(G_OBJECT(comboDepends),
                    "changed",
                    G_CALLBACK(cbDependsMenuChanged), this); 
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
   gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(comboDepends),
                              relRenderText, FALSE);
   gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(comboDepends),
                                 relRenderText, "text", 0);
   gtk_combo_box_set_active(GTK_COMBO_BOX(comboDepends), 0);

   GtkWidget *vpaned = GTK_WIDGET(gtk_builder_get_object
                                  (_builder, "vpaned_main"));
   assert(vpaned);
   GtkWidget *hpaned = GTK_WIDGET(gtk_builder_get_object
                                  (_builder, "hpaned_main"));
   assert(hpaned);
   // If the pane position is restored before the window is shown, it's
   // not restored in the same place as it was.
   if(!_config->FindB("Volatile::HideMainwindow", false))
      show();
   RGFlushInterface();
   gtk_paned_set_position(GTK_PANED(vpaned),
                          _config->FindI("Synaptic::vpanedPos", 140));
   gtk_paned_set_position(GTK_PANED(hpaned),
                          _config->FindI("Synaptic::hpanedPos", 200));


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

   g_signal_connect(gtk_builder_get_object(_builder, "add_cdrom"),
                    "activate",
                    G_CALLBACK(cbAddCDROM), this);

   g_signal_connect(gtk_builder_get_object(_builder, "menu_download_changelog"),
                    "activate",
                    G_CALLBACK(cbChangelogDialog), this); 

   /* --------------------------------------------------------------- */

   // toolbar menu code
   button = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_toolbar_pixmaps"));
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
   g_object_set_data(G_OBJECT(button), "me", this);
   g_signal_connect(G_OBJECT(button),
                    "activate",
                    G_CALLBACK(cbMenuToolbarClicked),
                    GINT_TO_POINTER(GTK_TOOLBAR_ICONS));
   if (_toolbarStyle == GTK_TOOLBAR_ICONS)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));

   button = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_toolbar_text"));
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
   g_object_set_data(G_OBJECT(button), "me", this);
   g_signal_connect(G_OBJECT(button),
                    "activate",
                    G_CALLBACK(cbMenuToolbarClicked),
                    GINT_TO_POINTER(GTK_TOOLBAR_TEXT));
   if (_toolbarStyle == GTK_TOOLBAR_TEXT)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));

   button = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_toolbar_both"));
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
   g_object_set_data(G_OBJECT(button), "me", this);
   g_signal_connect(G_OBJECT(button),
                    "activate",
                    G_CALLBACK(cbMenuToolbarClicked),
                    GINT_TO_POINTER(GTK_TOOLBAR_BOTH));
   if (_toolbarStyle == GTK_TOOLBAR_BOTH)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));

   button = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_toolbar_beside"));
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
   g_object_set_data(G_OBJECT(button), "me", this);
   g_signal_connect(G_OBJECT(button),
                    "activate",
                    G_CALLBACK(cbMenuToolbarClicked),
                    GINT_TO_POINTER(GTK_TOOLBAR_BOTH_HORIZ));
   if (_toolbarStyle == GTK_TOOLBAR_BOTH_HORIZ)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));

   button = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_toolbar_hide"));
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
   g_object_set_data(G_OBJECT(button), "me", this);
   g_signal_connect(G_OBJECT(button),
                    "activate",
                    G_CALLBACK(cbMenuToolbarClicked),
                    GINT_TO_POINTER(TOOLBAR_HIDE));
   if (_toolbarStyle == TOOLBAR_HIDE)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));

   // build popup-menu
   _popupMenu = gtk_menu_new();
   menuitem = gtk_menu_item_new_with_label(_("Unmark"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_KEEP);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_menu_item_new_with_label(_("Mark for Installation"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_INSTALL);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_menu_item_new_with_label(_("Mark for Reinstallation"));
   g_object_set_data(G_OBJECT(menuitem),"me",this);
   g_signal_connect(menuitem, "activate",
		    (GCallback) cbPkgAction, (void*)PKG_REINSTALL);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);


   menuitem = gtk_menu_item_new_with_label(_("Mark for Upgrade"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_INSTALL);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_menu_item_new_with_label(_("Mark for Removal"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_DELETE);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);


   menuitem = gtk_menu_item_new_with_label(_("Mark for Complete Removal"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbPkgAction, (void *)PKG_PURGE);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);
#ifdef HAVE_RPM
   gtk_widget_hide(menuitem);
#endif

#if 0  // disabled for now
   menuitem = gtk_menu_item_new_with_label(_("Remove Including Orphaned Dependencies"));
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

   menuitem = gtk_menu_item_new_with_label(_("Properties"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbDetailsWindow, this);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

#ifndef HAVE_RPM // recommends stuff
   menuitem = gtk_separator_menu_item_new ();
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_menu_item_new_with_label(_("Mark Recommended for Installation"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);

   menuitem = gtk_menu_item_new_with_label(_("Mark Suggested for Installation"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);
#endif

   gtk_widget_show(_popupMenu);

   //FIXME/MAYBE: create this dynmaic?!?
   //    for (vector<string>::const_iterator I = views.begin();
   // I != views.end(); I++) {
   // item = gtk_radiobutton_new((char *)(*I).c_str());
   GtkWidget *w;

   // section
   w=_viewButtons[PACKAGE_VIEW_SECTION] = GTK_WIDGET(gtk_builder_get_object
                                                     (_builder,
                                                      "radiobutton_sections"));
   g_object_set_data(G_OBJECT(w), "index", 
		     GINT_TO_POINTER(PACKAGE_VIEW_SECTION));
   g_signal_connect(G_OBJECT(w),
                    "toggled",
                    (GCallback) cbChangedView, this);
   // status
   w=_viewButtons[PACKAGE_VIEW_STATUS] = GTK_WIDGET(gtk_builder_get_object
                                                    (_builder,
                                                     "radiobutton_status"));
   g_object_set_data(G_OBJECT(w), "index", 
		     GINT_TO_POINTER(PACKAGE_VIEW_STATUS));
   g_signal_connect(G_OBJECT(w),
                    "toggled",
                    (GCallback) cbChangedView, this);
   // origin
   w=_viewButtons[PACKAGE_VIEW_ORIGIN] = GTK_WIDGET(gtk_builder_get_object
                                                    (_builder,
                                                     "radiobutton_origin"));
   g_object_set_data(G_OBJECT(w), "index", 
		     GINT_TO_POINTER(PACKAGE_VIEW_ORIGIN));
   g_signal_connect(G_OBJECT(w),
                    "toggled",
                    (GCallback) cbChangedView, this);
   // custom
   w=_viewButtons[PACKAGE_VIEW_CUSTOM] = GTK_WIDGET(gtk_builder_get_object
                                                    (_builder,
                                                     "radiobutton_custom"));
   g_object_set_data(G_OBJECT(w), "index", 
		     GINT_TO_POINTER(PACKAGE_VIEW_CUSTOM));
   g_signal_connect(G_OBJECT(w),
                    "toggled",
                    (GCallback) cbChangedView, this);
   // find
   w=_viewButtons[PACKAGE_VIEW_SEARCH] = GTK_WIDGET(gtk_builder_get_object
                                                    (_builder,
                                                     "radiobutton_find"));
   g_object_set_data(G_OBJECT(w), "index", 
		     GINT_TO_POINTER(PACKAGE_VIEW_SEARCH));
   g_signal_connect(G_OBJECT(w),
                    "toggled",
                    (GCallback) cbChangedView, this);

   // architecture
   w=_viewButtons[PACKAGE_VIEW_ARCHITECTURE] = GTK_WIDGET(gtk_builder_get_object
                                                    (_builder,
                                                     "radiobutton_architecture"));
   g_object_set_data(G_OBJECT(w), "index", 
		     GINT_TO_POINTER(PACKAGE_VIEW_ARCHITECTURE));
   g_signal_connect(G_OBJECT(w),
                    "toggled",
                    (GCallback) cbChangedView, this);

   _subViewList = GTK_WIDGET(gtk_builder_get_object
                             (_builder, "treeview_subviews"));
   assert(_subViewList);
   setTreeList("treeview_subviews", vector<string>(), true);
   // Setup the selection handler 
   select = gtk_tree_view_get_selection(GTK_TREE_VIEW(_subViewList));
   gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
   g_signal_connect(G_OBJECT(select), "changed",
                    G_CALLBACK(cbChangedSubView), this);

   GtkBindingSet *binding_set = gtk_binding_set_find("GtkTreeView");
   gtk_binding_entry_add_signal(binding_set, GDK_s, GDK_CONTROL_MASK,
				"start_interactive_search", 0);

   _entry_fast_search = GTK_WIDGET(gtk_builder_get_object
                                   (_builder, "entry_fast_search"));

   // only enable fast search if its usable
#ifdef WITH_EPT
   if(!_lister->xapiandatabase() ||
      !FileExists("/usr/sbin/update-apt-xapian-index")) {
      gtk_widget_hide(GTK_WIDGET(
            gtk_builder_get_object(_builder, "toolitem_fast_search")));
      gtk_box_set_center_widget(GTK_BOX(
            gtk_builder_get_object(_builder, "hbox_button_toolbar")), NULL);
   }
#else
   gtk_widget_hide(GTK_WIDGET(
         gtk_builder_get_object(_builder, "toolitem_fast_search")));
   gtk_box_set_center_widget(GTK_BOX(
         gtk_builder_get_object(_builder, "hbox_button_toolbar")), NULL);
#endif
   // stuff for the non-root mode
   if(getuid() != 0) {
      GtkWidget *menu;
      gtk_widget_set_sensitive(_proceedB, false);
      gtk_widget_set_sensitive(_proceedM, false);
      button = GTK_WIDGET(gtk_builder_get_object(_builder, "button_update"));
      gtk_widget_set_sensitive(button, false);
      menu = GTK_WIDGET(gtk_builder_get_object
                        (_builder, "menu_add_downloadedfiles"));
      gtk_widget_set_sensitive(menu, false);
      menu = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_repositories"));
      gtk_widget_set_sensitive(menu, false);
      menu = GTK_WIDGET(gtk_builder_get_object(_builder, "view_commit_log"));
      gtk_widget_set_sensitive(menu, false);
      menu = GTK_WIDGET(gtk_builder_get_object
                        (_builder, "menu_update_packages"));
      gtk_widget_set_sensitive(menu, false);
      menu = GTK_WIDGET(gtk_builder_get_object(_builder, "add_cdrom"));
      gtk_widget_set_sensitive(menu, false);
      menu = GTK_WIDGET(gtk_builder_get_object(_builder, "menu_hold"));
      gtk_widget_set_sensitive(menu, false);
   }

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
      gchar* warning = g_strdup_printf(_( "Removing package \"%s\" may render the "
                                          "system unusable.\n"
                                          "Are you sure you want to do that?"), 
                                       pkg->name());
      bool confirmed = _userDialog->confirm(warning, false);
      g_free(warning);
      if (!confirmed) {
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
   int toInstall, toRemove;
   double size;


   GtkWidget *_statusL = GTK_WIDGET(gtk_builder_get_object(_builder, "label_status"));
   assert(_statusL);

   _lister->getStats(installed,broken,toInstall,toRemove,size);

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

   if (getuid() == 0) {
      gtk_widget_set_sensitive(_proceedB, (toInstall + toRemove) != 0);
      gtk_widget_set_sensitive(_proceedM, (toInstall + toRemove) != 0);
   }
   _unsavedChanges = ((toInstall + toRemove) != 0);

   gtk_widget_queue_draw(_statusL);
}


void RGMainWindow::saveState()
{
   if (_config->FindB("Volatile::NoStateSaving", false) == true)
      return;

   GtkWidget *vpaned = GTK_WIDGET(gtk_builder_get_object
                                  (_builder, "vpaned_main"));
   GtkWidget *hpaned = GTK_WIDGET(gtk_builder_get_object
                                  (_builder, "hpaned_main"));
   _config->Set("Synaptic::vpanedPos",
                gtk_paned_get_position(GTK_PANED(vpaned)));
   _config->Set("Synaptic::hpanedPos",
                gtk_paned_get_position(GTK_PANED(hpaned)));

   GtkAllocation allocation;
   gtk_widget_get_allocation(_win, &allocation);
   _config->Set("Synaptic::windowWidth", allocation.width);
   _config->Set("Synaptic::windowHeight", allocation.height);
   gint x, y;
   gtk_window_get_position(GTK_WINDOW(_win), &x, &y);
   _config->Set("Synaptic::windowX", x);
   _config->Set("Synaptic::windowY", y);
   _config->Set("Synaptic::ToolbarState", (int)_toolbarStyle);
   if(gdk_window_get_state(gtk_widget_get_window(_win)) & GDK_WINDOW_STATE_MAXIMIZED)
      _config->Set("Synaptic::Maximized", true);
   else
      _config->Set("Synaptic::Maximized", false);

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
   int installed, broken, toInstall, toRemove;
   double sizeChange;
   _lister->getStats(installed, broken, toInstall, toRemove, sizeChange);
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
      changeView(viewNr);

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
   return true;
}


bool RGMainWindow::close()
{
   if (_interfaceLocked > 0)
      return true;

   RGGtkBuilderUserDialog dia(this);
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
      if(gtk_widget_get_visible(_win))
	 gdk_window_set_cursor(gtk_widget_get_window(_win), _busyCursor);
   } else {
      assert(_interfaceLocked > 0);

      _interfaceLocked--;
      if (_interfaceLocked > 0)
         return;

      gtk_widget_set_sensitive(_win, TRUE);
      if(gtk_widget_get_visible(_win))
	 gdk_window_set_cursor(gtk_widget_get_window(_win), NULL);
   }

   // fast enough with the new fixed-height mode
   while (gtk_events_pending())
      gtk_main_iteration();
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
   // Ignore DEL accelerator when fastsearch has focus
   GtkWidget *entry = GTK_WIDGET(gtk_builder_get_object
                                 (me->_builder, "entry_fast_search"));
   if (gtk_widget_has_focus (entry) && GPOINTER_TO_INT(data) == PKG_DELETE) {
      return;
   }
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
               (event->button == 1 && 
                strcmp(gtk_tree_view_column_get_title(column), "S") == 0)))
            return false;

         vector<RPackage *> selected_pkgs;
         GList *li = NULL;

         // Treat click with CONTROL as additional selection
	 if((event->state & GDK_CONTROL_MASK) != GDK_CONTROL_MASK
	    && !gtk_tree_selection_path_is_selected(selection, path))
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
   ShowChangelogDialog(me, pkg);
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

   if(flags & RPackage::FPinned)
      return;

   if (!(flags & RPackage::FInstalled)) {
      if (flags & RPackage::FKeep)
         me->pkgAction(PKG_INSTALL);
      else if (flags & RPackage::FInstall)
         me->pkgAction(PKG_KEEP);
   } else if (flags & RPackage::FOutdated) {
      if (flags & RPackage::FKeep)
         me->pkgAction(PKG_INSTALL);
      else if (flags & RPackage::FUpgrade)
         me->pkgAction(PKG_KEEP);
   }

   // make sure we do not lose the keyboard focus (this happens in
   // pkgAction otherwise)
   gtk_widget_grab_focus (GTK_WIDGET(treeview));
   gtk_tree_view_set_cursor(GTK_TREE_VIEW(treeview), path, NULL, false);

   GtkTreePath *start = gtk_tree_path_new();
   bool ok = gtk_tree_view_get_visible_range(GTK_TREE_VIEW(treeview), &start, NULL);
   if (ok && gtk_tree_model_get_iter_first(me->_pkgList, &iter)) {
      gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview), start, NULL, true, 0.0, 0.0);
   }
   gtk_tree_path_free(start);

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
      if(_config->FindB("APT::CDROM::NoMount", false))
	 dontStop=false;
      else
	 dontStop = me->_userDialog->confirm(_("Do you want to add another CD-ROM?"));
   }
   scan.hide();
   if (updateCache) {
      me->setTreeLocked(TRUE);
      if (!me->_lister->openCache()) {
         me->showErrors();
         exit(1);
      }
      me->setTreeLocked(FALSE);
      me->refreshTable(me->selectedPackage());
   }
   me->setInterfaceLocked(FALSE);
}



void RGMainWindow::cbTasksClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow*)data;

   me->setBusyCursor(true);

   if (me->_tasksWin == NULL) {   
      me->_tasksWin = new RGTasksWin(me);
   }
   me->_tasksWin->show();

   me->setBusyCursor(false);
}

void RGMainWindow::cbOpenClicked(GtkWidget *self, void *data)
{
   //std::cout << "RGMainWindow::openClicked()" << endl;
   RGMainWindow *me = (RGMainWindow*)data;

   GtkWidget *filesel;
   filesel = gtk_file_chooser_dialog_new(_("Open changes"), 
					 GTK_WINDOW(me->window()),
					 GTK_FILE_CHOOSER_ACTION_OPEN,
					 _("_Cancel"), GTK_RESPONSE_CANCEL,
					 _("_Open"), GTK_RESPONSE_ACCEPT,
					 NULL);
   if(gtk_dialog_run(GTK_DIALOG(filesel)) == GTK_RESPONSE_ACCEPT) {
      me->setInterfaceLocked(TRUE);
      gtk_widget_hide(filesel);
      RGFlushInterface();

      RPackageLister::pkgState state;
      me->_lister->saveState(state);

      const char *file;
      file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
      me->selectionsFilename = file;

      ifstream in(file);
      if (!in != 0) {
	 _error->Error(_("Can't read %s"), file);
	 me->_userDialog->showErrors();
	 return;
      }
      me->_lister->unregisterObserver(me);
      // read the selections from the file
      me->_lister->readSelections(in);
      me->askStateChange(state);

      // refresh to ensure that broken dependencies are displayed
      me->_lister->registerObserver(me);
      me->refreshTable();
      me->refreshSubViewList();
      me->setStatusText();
      me->setInterfaceLocked(FALSE);
   }
   gtk_widget_destroy(filesel);
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
   RGMainWindow *me = (RGMainWindow*)data;

   GtkWidget *filesel;
   filesel = gtk_file_chooser_dialog_new(_("Save changes"), 
					 GTK_WINDOW(me->window()),
					 GTK_FILE_CHOOSER_ACTION_SAVE,
					 _("_Cancel"), GTK_RESPONSE_CANCEL,
					 _("_Save"), GTK_RESPONSE_ACCEPT,
					 NULL);
   GtkWidget *checkButton =
      gtk_check_button_new_with_label(_("Save full state, not only changes"));
   gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(filesel), checkButton);

   if(gtk_dialog_run(GTK_DIALOG(filesel)) == GTK_RESPONSE_ACCEPT) {
      const char *file;
      file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
      me->selectionsFilename = file;
      me->saveFullState =
	 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton));
      // now call save for the actual saving
      me->cbSaveClicked(self, me);
   }
   gtk_widget_destroy(filesel);
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
   if (pkg == NULL) 
      return;

   if(me->_pkgDetails == NULL)
      me->_pkgDetails = new RGPkgDetailsWindow(me);

   RGPkgDetailsWindow::fillInValues(me->_pkgDetails, pkg, true);
   me->_pkgDetails->show();
}

// helper to hide the "please wait" message
static void plug_added(GtkWidget *sock, void *data)
{
   gtk_widget_show(sock);
   gtk_widget_hide(GTK_WIDGET(data));
}

static gboolean kill_repos(GtkWidget *self, GdkEvent *event, void *data)
{
   GPid pid = *(GPid*)data;
   kill(pid, SIGQUIT);
   return TRUE;
}

void RGMainWindow::cbShowSourcesWindow(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   // FIXME: make this all go into the repository window
   bool Changed = false;
   bool ForceReload = _config->FindB("Synaptic::UpdateAfterSrcChange",false);
   
   if(!g_file_test("/usr/bin/software-properties-gtk", 
		   G_FILE_TEST_IS_EXECUTABLE) 
      || _config->FindB("Synaptic::dontUseGnomeSoftwareProperties", false)) 
   {
      RGRepositoryEditor w(me);
      Changed = w.Run();
   } else {
      // use gnome-software-properties window
      me->setInterfaceLocked(TRUE);
      GPid pid;
      int status;
      const char *argv[5];
      argv[0] = "/usr/bin/software-properties-gtk";
      argv[1] = "-n";
      argv[2] = "-t";
      argv[3] = g_strdup_printf("%lu", GDK_WINDOW_XID(gtk_widget_get_window(me->_win)));
      argv[4] = NULL;
      g_spawn_async(NULL, const_cast<char **>(argv), NULL,
		    (GSpawnFlags)G_SPAWN_DO_NOT_REAP_CHILD,
		    NULL, NULL, &pid, NULL);
      // kill the child if the window is deleted
      while(waitpid(pid, &status, WNOHANG) == 0) {
	 usleep(50000);
	 RGFlushInterface();
      }
      Changed = WEXITSTATUS(status);    
      me->setInterfaceLocked(FALSE);
   }
   
   RGFlushInterface();

   // auto update after repostitory change
   if (Changed == true && ForceReload) {
      me->cbUpdateClicked(NULL, data);
   } else if(Changed == true && 
	     _config->FindB("Synaptic::AskForUpdateAfterSrcChange",true)) {
      // ask for update after repo change
      GtkWidget *cb, *dialog;
      dialog = gtk_message_dialog_new (GTK_WINDOW(me->window()),
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
      gtk_dialog_add_buttons(GTK_DIALOG(dialog), _("_Cancel"), GTK_RESPONSE_REJECT, _("_Reload"), GTK_RESPONSE_ACCEPT, NULL);
      GtkWidget* reload_button = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
      GtkWidget* refresh_image = gtk_image_new_from_icon_name("view-refresh", GTK_ICON_SIZE_BUTTON);
      gtk_button_set_image(GTK_BUTTON(reload_button), refresh_image);
      cb = gtk_check_button_new_with_label(_("Never show this message again"));
      gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), cb, true, true, 0);
      gtk_widget_show(cb);
      gint response = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_hide(dialog);
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb))) {
	    _config->Set("Synaptic::AskForUpdateAfterSrcChange", false);
      }
      if (response == GTK_RESPONSE_ACCEPT) {
         me->cbUpdateClicked(NULL, data);
      }
      gtk_widget_destroy (dialog);
   }
}

void RGMainWindow::cbMenuToolbarClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) g_object_get_data(G_OBJECT(self), "me");
   GtkWidget *widget;
   // save new toolbar state
   me->_toolbarStyle = (GtkToolbarStyle) GPOINTER_TO_INT(data);
   GtkWidget *toolbar_main =
         GTK_WIDGET(gtk_builder_get_object(me->_builder, "toolbar_main"));
   GtkWidget *toolbar_search =
         GTK_WIDGET(gtk_builder_get_object(me->_builder, "toolbar_search"));
   assert(toolbar_main);
   assert(toolbar_search);

   if (me->_toolbarStyle == TOOLBAR_HIDE) {
      widget = GTK_WIDGET(gtk_builder_get_object
                          (me->_builder, "hbox_button_toolbar"));
      gtk_widget_hide(widget);
      return;
   } else {
      widget = GTK_WIDGET(gtk_builder_get_object
                          (me->_builder, "hbox_button_toolbar"));
      gtk_widget_show(widget);
   }
   gtk_toolbar_set_style(GTK_TOOLBAR(toolbar_main), me->_toolbarStyle);
   gtk_toolbar_set_style(GTK_TOOLBAR(toolbar_search), me->_toolbarStyle);
}

void RGMainWindow::cbFindToolClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   if (me->_findWin == NULL) {
      me->_findWin = new RGFindWindow(me);
   }
   
   me->_findWin->selectText();
   int res = gtk_dialog_run(GTK_DIALOG(me->_findWin->window()));
   if (res == GTK_RESPONSE_OK) {

      // clear the quick search, otherwise both apply and that is
      // confusing
      gtk_entry_set_text(GTK_ENTRY(me->_entry_fast_search), "");

      string str = me->_findWin->getFindString();
      me->setBusyCursor(true);

      // we need to convert here as the DDTP project does not use utf-8
      const char *locale_str = utf8_to_locale(str.c_str());
      if(locale_str == NULL) // invalid utf-8
	 locale_str = str.c_str();

      int type = me->_findWin->getSearchType();
      GtkWidget *progress = GTK_WIDGET(gtk_builder_get_object
                                       (me->_builder, "progressbar_main"));
      GtkWidget *label = GTK_WIDGET(gtk_builder_get_object
                                    (me->_builder, "label_status"));
      RGCacheProgress searchProgress(progress, label);
      int found = me->_lister->searchView()->setSearch(str,type, 
						       locale_str,
						       searchProgress);
      me->changeView(PACKAGE_VIEW_SEARCH, str);

      me->setBusyCursor(false);
      gchar *statusstr = g_strdup_printf(_("Found %i packages"), found);
      me->setStatusText(statusstr);
      me->updatePackageInfo(NULL);
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

void RGMainWindow::cbViewLogClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   if (me->_logView == NULL)
      me->_logView = new RGLogView(me);
   me->_logView->readLogs();
   me->_logView->show();
}


void RGMainWindow::cbHelpAction(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   me->setStatusText(_("Starting help viewer..."));

   // FIXME: move this into rgutils as well (or rgspawn.cc)
   vector<const gchar*> cmd;
   if (is_binary_in_path("yelp")) {
      cmd.push_back("yelp");
      cmd.push_back("ghelp:synaptic");
   } else {
      cmd = GetBrowserCommand(PACKAGE_DATA_DIR "/synaptic/html/index.html");
   }

   if (cmd.empty()) {
      me->_userDialog->error(_("No help viewer is installed!\n\n"
                               "You need either the GNOME help viewer 'yelp', "
                               "the 'konqueror' browser or the 'firefox' "
                               "browser to view the synaptic manual.\n\n"
                               "Alternatively you can open the man page "
                               "with 'man synaptic' from the "
                               "command line or view the html version located "
                               "in the 'synaptic/html' folder."));
      return;
   }
   RunAsSudoUserCommand(cmd);
}

void RGMainWindow::cbCloseFilterManagerAction(void *self, bool okcancel)
{
   RGMainWindow *me = (RGMainWindow *) self;

   // FIXME: only do all this if the user didn't click "cancel" in the dialog

   me->setInterfaceLocked(TRUE);

   me->_lister->filterView()->refreshFilters();
   me->refreshTable();
   me->refreshSubViewList();

   me->setInterfaceLocked(FALSE);
}


void RGMainWindow::cbShowFilterManagerWindow(GtkWidget *self, void *data)
{

   RGMainWindow *me = (RGMainWindow *) data;

   if (me->_fmanagerWin == NULL) {
      me->_fmanagerWin = new RGFilterManagerWindow(me, me->_lister->filterView());
   }

   me->_fmanagerWin->readFilters();
   int res = gtk_dialog_run(GTK_DIALOG(me->_fmanagerWin->window()));
   if(res == GTK_RESPONSE_OK) {
      me->setInterfaceLocked(TRUE);

      me->_lister->filterView()->refreshFilters();
      me->refreshTable();
      me->refreshSubViewList();

      me->setInterfaceLocked(FALSE);
   }
   
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
   list = li = gtk_tree_selection_get_selected_rows(selection, &me->_pkgList);

   // list is empty
   if (li == NULL) {
      me->updatePackageInfo(NULL);
      return;
   }
   // we are only interested in the last element
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
   if (!me->_lister->openCache()) {
      me->showErrors();
      exit(1);
   }

   me->_lister->registerObserver(me);
   me->setTreeLocked(FALSE);
   me->refreshTable();
   me->refreshSubViewList();
   me->setInterfaceLocked(FALSE);
   me->setStatusText();
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
   RGMainWindow *me = (RGMainWindow *) data;
   //cout << "RGMainWindow::pkgReconfigureClicked()" << endl;

   if(me->selectedPackage() == NULL)
      return;

   RPackage *pkg = NULL;
   pkg = me->_lister->getPackage("libgnome2-perl");
   if (pkg && pkg->installedVersion() == NULL) {
      me->_userDialog->error(_("Cannot start configuration tool!\n"
                               "You have to install the required package "
                               "'libgnome2-perl'."));
      return;
   }

   me->setStatusText(_("Starting package configuration tool..."));
   const gchar *cmd[] = { "/usr/sbin/dpkg-reconfigure",
                    "-fgnome",
                    me->selectedPackage()->name(),
                    NULL };
   GError *error = NULL;
   g_spawn_async("/", const_cast<gchar **>(cmd), NULL, (GSpawnFlags)0, NULL, NULL, NULL, &error);
   if(error != NULL) {
      std::cerr << "failed to run dpkg-reconfigure cmd" << std::endl;
   }
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

   if (is_binary_in_path("dwww")) {
      const gchar *cmd[5];
      cmd[0] = "dwww";
      cmd[1] = me->selectedPackage()->name();
      cmd[2] = NULL;
      g_spawn_async("/tmp", const_cast<gchar **>(cmd), NULL, (GSpawnFlags)0, NULL, NULL, NULL, NULL);
   } else {
      me->_userDialog->error(_("You have to install the package \"dwww\" "
			       "to browse the documentation of a package"));
   }
}


void RGMainWindow::cbChangedView(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data; 

   // only act on the active buttons
   if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self)) ||
      me->_blockActions == TRUE)
      return;

   long view = (long)g_object_get_data(G_OBJECT(self), "index");
   me->changeView(view);
}

void RGMainWindow::cbChangedSubView(GtkTreeSelection *selection,
                                    gpointer data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   if(me->_blockActions)
      return;

   me->setBusyCursor(true);
   // we need to set a empty model first so that gtklistview
   // can do its cleanup, if we do not do that, then the cleanup
   // code in gtktreeview gets confused and throws
   // Gtk-CRITICAL **: gtk_tree_view_unref_tree_helper: assertion `node != NULL' failed
   // at us, see LP: #38397 for more information
   gtk_tree_view_set_model(GTK_TREE_VIEW(me->_treeView), NULL);

   string selected = MarkupUnescapeString(me->selectedSubView());
   me->_lister->setSubView(utf8(selected.c_str()));
   me->refreshTable(NULL, false);
   me->setBusyCursor(false);
   me->updatePackageInfo(NULL);
}

void RGMainWindow::activeWindowToForeground()
{
   //cout << "activeWindowToForeground: " << getpid() << endl;

   // easy, we have a main window
   if(_config->FindB("Volatile::HideMainwindow", false) == false) {
      gtk_window_present(GTK_WINDOW(window()));
      return;
   }

   // harder, we run without mainWindow (in non-interactive mode most likly)
   if( _fetchProgress && gtk_widget_get_visible(_fetchProgress->window()))
      gtk_window_present(GTK_WINDOW(_fetchProgress->window()));
   else if(_installProgress && gtk_widget_get_visible(_installProgress->window()))
      gtk_window_present(GTK_WINDOW(_installProgress->window()));
   else
      g_critical("activeWindowToForeground(): no active window found\n");
}

void RGMainWindow::cbProceedClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   RGSummaryWindow *summ;

   // nothing to do
   int listed, installed, broken;
   int toInstall, toRemove;
   double size;
   me->_lister->getStats(installed, broken, toInstall, toRemove, size);
   if((toInstall + toRemove) == 0)
      return;

   // check whether we can really do it
   if (!me->_lister->check()) {
      me->_userDialog->error(_("Could not apply changes!\n"
                               "Fix broken packages first."));
      return;
   }

   int a,b,c,d,e,f,g,h,unAuthenticated;
   double s;
   me->_lister->getSummary(a,b,c,d,e,f,g,h,unAuthenticated,s);
   if(unAuthenticated ||
      _config->FindB("Volatile::Non-Interactive", false) == false) {
      // show a summary of what's gonna happen
      RGSummaryWindow summ(me, me->_lister);
      if (!summ.showAndConfirm()) {
         // canceled operation
         return;
      }
   }

   me->setInterfaceLocked(TRUE);
   me->updatePackageInfo(NULL);

   me->setStatusText(_("Applying marked changes. This may take a while..."));

   // fetch packages
   RGFetchProgress *fprogress=me->_fetchProgress = new RGFetchProgress(me);
   fprogress->setDescription(_("Downloading Package Files"), "");
//			     _("The package files will be cached locally for installation."));

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
   // no RPM
   #ifdef WITH_DPKG_STATUSFD
   bool UseTerminal = false;
   #else
   bool UseTerminal = true;
   #endif // DPKG
#endif // HAVE_RPM
   RGTermInstallProgress *term = NULL;
   if (_config->FindB("Synaptic::UseTerminal", UseTerminal) == true)
      iprogress = term = new RGTermInstallProgress(me);
   else
#endif // HAVE_TERMINAL


#ifdef HAVE_RPM
      iprogress = new RGInstallProgress(me, me->_lister);
#else 
  #ifdef WITH_DPKG_STATUSFD
      iprogress = new RGDebInstallProgress(me,me->_lister);
  #else 
   iprogress = new RGDummyInstallProgress();
  #endif // WITH_DPKG_STATUSFD
#endif // HAVE_RPM
   me->_installProgress = dynamic_cast<RGWindow*>(iprogress);

   //bool result = me->_lister->commitChanges(fprogress, iprogress);
   me->_lister->commitChanges(fprogress, iprogress);

   // FIXME: move this into the terminal class
#ifdef HAVE_TERMINAL
   // wait until the term dialog is closed
   if (term != NULL) {
      while (gtk_widget_get_visible(GTK_WIDGET(term->window()))) {
         RGFlushInterface();
         usleep(100000);
      }
   }
#endif
   delete fprogress;
   me->_fetchProgress = NULL;
   delete iprogress;
   me->_installProgress = NULL;

   if (_config->FindB("Synaptic::IgnorePMOutput", false) == false) {
      me->showErrors();
   } else {
      _error->Discard();
   }
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

   if (_config->FindB("Volatile::Download-Only", false) == false) {
      // reset the cache
      if (!me->_lister->openCache()) {
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
   me->refreshSubViewList();
   me->setInterfaceLocked(FALSE);
   me->updatePackageInfo(NULL);
}

void RGMainWindow::cbShowWelcomeDialog(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   RGGtkBuilderUserDialog dia(me);
   dia.run("welcome");
   GtkWidget *cb = GTK_WIDGET(gtk_builder_get_object
                              (dia.getGtkBuilder(), "checkbutton_show_again"));
   assert(cb);
   _config->Set("Synaptic::showWelcomeDialog",
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb)));
}

gboolean RGMainWindow::xapianDoSearch(void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
   const gchar *str = gtk_entry_get_text(GTK_ENTRY(me->_entry_fast_search));
   GtkStyleContext *styleContext = gtk_widget_get_style_context(me->_entry_fast_search);

   me->_fastSearchEventID = -1;
   me->setBusyCursor(true);
   RGFlushInterface();
   if(str == NULL || strlen(str) <= 1) {
      // reset the color
      gtk_style_context_remove_provider(styleContext, GTK_STYLE_PROVIDER(_fastSearchCssProvider));
      // if the user has cleared the search, refresh the view
      // Gtk-CRITICAL **: gtk_tree_view_unref_tree_helper: assertion `node != NULL' failed
      // at us, see LP: #38397 for more information
      gtk_tree_view_set_model(GTK_TREE_VIEW(me->_treeView), NULL);
      me->_lister->reapplyFilter();
      me->refreshTable();
      me->setBusyCursor(false);
   } else if(strlen(str) > 1) {
      // only search when there is more than one char entered, single
      // char searches tend to be very slow
      me->setBusyCursor(true);
      RGFlushInterface();
      gtk_tree_view_set_model(GTK_TREE_VIEW(me->_treeView), NULL);
      me->refreshTable();
      // set color to a light yellow to make it more obvious that a search
      // is performed
      gtk_style_context_add_provider(styleContext, GTK_STYLE_PROVIDER(_fastSearchCssProvider),
                                     GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
   }
   me->setBusyCursor(false);

   return FALSE;
}

void RGMainWindow::cbSearchEntryChanged(GtkWidget *edit, void *data)
{
   //cerr << "RGMainWindow::cbSearchEntryChanged()" << endl;
   RGMainWindow *me = (RGMainWindow *) data;
   if(me->_fastSearchEventID > 0) {
      g_source_remove(me->_fastSearchEventID);
      me->_fastSearchEventID = -1;
   }
   me->_fastSearchEventID = g_timeout_add(500, xapianDoSearch, me);
}

void RGMainWindow::cbUpdateClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;

   // need to delete dialogs, as they might have data pointing
   // to old stuff
//xxx    delete me->_fmanagerWin;
   me->_fmanagerWin = NULL;

   RGFetchProgress *progress=me->_fetchProgress= new RGFetchProgress(me);
   progress->setDescription(_("Downloading Package Information"),
			    _("The repositories will be checked for new, removed "
               "or upgraded software packages."));

   me->setStatusText(_("Reloading package information..."));

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
   // (only if no error occurred)
   string error;
   if (!me->_lister->updateCache(progress,error)) {
      RGGtkBuilderUserDialog dia(me,"update_failed");
      GtkWidget *tv = GTK_WIDGET(gtk_builder_get_object(dia.getGtkBuilder(),
                                                        "textview"));
      GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
      gtk_text_buffer_set_text(tb, utf8(error.c_str()), -1);
      dia.run();
   } else {
      me->forgetNewPackages();
      _config->Set("Synaptic::update::last",time(NULL));
   }
   delete progress;
   me->_fetchProgress=NULL;

   // show errors and warnings (like the gpg failures for the package list)
   me->showErrors();

   if(!me->_lister->openCache()) {
      me->showErrors();
      exit(1);
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

   // check if the index needs to be rebuild
   me->xapianDoIndexUpdate(me);

   me->setTreeLocked(FALSE);
   me->refreshTable();
   me->refreshSubViewList();
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
      (UpgradeType) _config->FindI("Synaptic::UpgradeType", UPGRADE_DIST);

   // special case for non-interactive upgrades
   if(_config->FindB("Volatile::Non-Interactive", false)) 
      if(_config->FindB("Volatile::Upgrade-Mode", false))
	 upgrade = UPGRADE_NORMAL;
      else if(_config->FindB("Volatile::DistUpgrade-Mode", false))
	 upgrade = UPGRADE_DIST;
   

   if (upgrade == UPGRADE_ASK) {
      // ask what type of upgrade the user wants
      GtkBuilder *builder;
      GtkWidget *button;

      RGGtkBuilderUserDialog dia(me);
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
      builder = dia.getGtkBuilder();
      // see if the user wants the answer saved
      button = GTK_WIDGET(gtk_builder_get_object
                          (builder, "checkbutton_remember"));
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
   
   RPackageLister::pkgState state;
   me->_lister->saveState(state);

   if (dist_upgrade)
      res = me->_lister->distUpgrade();
   else
      res = me->_lister->upgrade();

   if(me->askStateChange(state))
   {
      me->refreshTable(pkg);

      if (res)
         me->setStatusText(_("Successfully marked available upgrades"));
      else
         me->setStatusText(_("Failed to mark all available upgrades!"));
   } else {
      // if the user canceled the action, just show the default message
      me->setStatusText();
   }

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

   list = li = gtk_tree_selection_get_selected_rows(selection, &me->_pkgList);
   if (li == NULL)
      return;

   me->setInterfaceLocked(TRUE);
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
   me->setTreeLocked(TRUE);
   if (!me->_lister->openCache()) {
      me->showErrors();
      exit(1);
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

   // free the list
   g_list_foreach(list, (void (*)(void *, void *))gtk_tree_path_free, NULL);
   g_list_free(list);

   me->_lister->registerObserver(me);
   me->setTreeLocked(FALSE);
   me->refreshTable();
   me->refreshSubViewList();
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

   if( flags & RPackage::FPinned) 
      return;

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
	  && !(flags & RPackage::FOutdated) 
	  && !(flags & RPackage::FNotInstallable)) {
         gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
      }

      // Upgrade button
      if (i == 3 && (flags & RPackage::FOutdated)
          && !(flags & RPackage::FInstall)) {
         gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
         if (oneclickitem == NULL)
            oneclickitem = item->data;
      }

      // remove
      if (i == 4 &&  (flags & RPackage::FInstalled) 
	  && (!(flags & RPackage::FRemove) || (flags & RPackage::FPurge)) ) {
            gtk_widget_set_sensitive(GTK_WIDGET(item->data), TRUE);
            if (oneclickitem == NULL)
               oneclickitem = item->data;
      }

      // Purge
      if (i == 5 
	  && (flags&RPackage::FInstalled || flags&RPackage::FResidualConfig) 
	  && !(flags & RPackage::FPurge) ) {
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
   vector<DepInformation> deps = pkg->enumDeps();
   for(unsigned int i=0;i<deps.size();i++) {
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
	    for(unsigned int k=0;k<pkgs.size();k++) {
	       vector<string> d = pkgs[k]->provides();
	       for(unsigned int j=0;j<d.size();j++)
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

void RGMainWindow::selectToInstall(vector<string> packagenames)
{
   RGMainWindow *me = this;

   RPackageLister::pkgState state;
   vector<RPackage *> exclude;
   vector<RPackage *> instPkgs;

   // we always save the state (for undo)
   me->_lister->saveState(state);
   me->_lister->notifyCachePreChange();

   for(unsigned int i=0;i<packagenames.size();i++) {
      RPackage *newpkg = (RPackage *) me->_lister->getPackage(packagenames[i]);
      if (newpkg) {
	 // only install the package if it is not already installed or if
	 // it is outdated
	 if(!(newpkg->getFlags()&RPackage::FInstalled) ||
	     (newpkg->getFlags()&RPackage::FOutdated)) {
	    // actual action
	    newpkg->setNotify(false);
	    me->pkgInstallHelper(newpkg);
	    newpkg->setNotify(true);
	    //exclude.push_back(newpkg);
	    instPkgs.push_back(newpkg);
	 }
      }
   }

   // ask for additional changes
   me->setBusyCursor(true);
   if(me->askStateChange(state, exclude)) {
      me->_lister->saveUndoState(state);
      if(me->checkForFailedInst(instPkgs))
	 me->_lister->restoreState(state);
   }
   me->setBusyCursor(false);
   me->_lister->notifyPostChange(NULL);
   me->_lister->notifyCachePostChange();
   
   RPackage *pkg = me->selectedPackage();
   me->refreshTable(pkg);
   me->updatePackageInfo(pkg);
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
	 if(me->checkForFailedInst(instPkgs))
	    me->_lister->restoreState(state);
      }
      me->_lister->notifyPostChange(NULL);
      me->_lister->notifyCachePostChange();
      
      RPackage *pkg = me->selectedPackage();
      me->refreshTable(pkg);
      me->updatePackageInfo(pkg);
   }
}

void RGMainWindow::cbGenerateDownloadScriptClicked(GtkWidget *self, void *data)
{
   //cout << "cbGenerateDownloadScriptClicked()" << endl;
   RGMainWindow *me = (RGMainWindow *) data;

   int installed, broken, toInstall, toRemove;
   double sizeChange;
   me->_lister->getStats(installed, broken, toInstall, toRemove, sizeChange);
   if(toInstall== 0) {
      me->_userDialog->message("Nothing to install/upgrade\n\n"
			       "Please select the \"Mark all Upgrades\" "
			       "button or some packages to install/upgrade.");
      return;
   }

   vector<string> uris;
   if(!me->_lister->getDownloadUris(uris))
      return;

   GtkWidget *filesel;
   filesel = gtk_file_chooser_dialog_new(_("Save script"), 
					 GTK_WINDOW(me->window()),
					 GTK_FILE_CHOOSER_ACTION_SAVE,
					 _("_Cancel"), GTK_RESPONSE_CANCEL,
					 _("_Save"), GTK_RESPONSE_ACCEPT,
					 NULL);
   int res = gtk_dialog_run(GTK_DIALOG(filesel));
   const char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
   gtk_widget_destroy(filesel);
   if(res != GTK_RESPONSE_ACCEPT) 
      return;

   // FIXME: this is prototype code, hardcoding wget here suckx
   ofstream out(file);
   out << "#!/bin/sh" << endl;
   for(int i=0;i<uris.size();i++) {
      out << "wget -c " << uris[i] << endl;
   }
   chmod(file, 0755);
}

void RGMainWindow::cbAddDownloadedFilesClicked(GtkWidget *self, void *data)
{
   RGMainWindow *me = (RGMainWindow *) data;
#ifndef HAVE_RPM
   //cout << "cbAddDownloadedFilesClicked()" << endl;
   GtkWidget *filesel;
   filesel = gtk_file_chooser_dialog_new(_("Select directory"), 
					 GTK_WINDOW(me->window()),
					 GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					 _("_Cancel"), GTK_RESPONSE_CANCEL,
					 _("_Open"), GTK_RESPONSE_ACCEPT,
					 NULL);
   int res = gtk_dialog_run(GTK_DIALOG(filesel));
   const char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
   gtk_widget_destroy(filesel);
   if(res != GTK_RESPONSE_ACCEPT) 
      return;
   if (!g_file_test(path, G_FILE_TEST_IS_DIR)) {
      me->_userDialog->error(_("Please select a directory"));
      return;
   }
   // now read the dir for debs
   const gchar *file;
   string pkgname;
   stringstream pkgs;
   GDir *dir = g_dir_open(path, 0, NULL);
   while ( (file=g_dir_read_name(dir)) != NULL) {
      if(g_pattern_match_simple("*_*.deb", file)) {
	 if(me->_lister->addArchiveToCache(string(path)+"/"+string(file),
					   pkgname))
	    pkgs << pkgname << "\t install" << endl;
      }
   }
   g_dir_close(dir);

   // and set what we found as selection
   pkgs.seekg(0);
   if (pkgs.str() == "")
      return;

   me->_lister->unregisterObserver(me);
   me->_lister->readSelections(pkgs);
   me->_lister->registerObserver(me);
   me->refreshTable();

   // show any errors 
   me->_userDialog->showErrors();
   
   // click proceed
   me->cbProceedClicked(NULL, me);

#else
   me->_userDialog->error("Sorry, not implemented for rpm, patches welcome");
#endif
}

// vim:ts=3:sw=3:et
