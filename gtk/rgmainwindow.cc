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
#include "i18n.h"


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
#include "gsynaptic.h"

// icons and pixmaps
#include "alert.xpm"
#include "keepM.xpm"
#include "brokenM.xpm"
#include "downgradeM.xpm"
#include "heldM.xpm"
#include "installM.xpm"
#include "removeM.xpm"
#include "upgradeM.xpm"
#include "newM.xpm"
#include "holdM.xpm"
#include "synaptic_mini.xpm"


static char *ImportanceNames[] = {
    _("Unknown"),
    _("Normal"),
    _("Critical"),
    _("Security")
};


enum {WHAT_IT_DEPENDS_ON, 
      WHAT_DEPENDS_ON_IT,
      WHAT_IT_WOULD_DEPEND_ON,
      WHAT_IT_PROVIDES,
      WHAT_IT_SUGGESTS};      

enum {DEP_NAME_COLUMN,             /* text */
      DEP_IS_NOT_AVAILABLE,        /* foreground-set */
      DEP_IS_NOT_AVAILABLE_COLOR,  /* foreground */
      DEP_PKG_INFO};               /* additional info (install 
				      not installed) as text */

GdkPixbuf *StatusPixbuf[13];
GdkColor *StatusColors[13];


#define SELECTED_MENU_INDEX(popup) \
    (int)gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(gtk_option_menu_get_menu(GTK_OPTION_MENU(popup))))), "index")



#if ! GTK_CHECK_VERSION(2,2,0)
// this function is needed to be compatible with gtk2.0
// data takes a GList** and fills the list with GtkTreePathes 
// (just like the return of gtk_tree_selection_get_selected_rows())
void multipleSelectionHelper(GtkTreeModel *model,
			     GtkTreePath *path,
			     GtkTreeIter *iter,
			     gpointer data)
{
    //cout << "multipleSelectionHelper()"<<endl;
    GList **list;
    list = (GList**)data;
    *list = g_list_append(*list, gtk_tree_path_copy(path));
}
#endif


void RGMainWindow::changedDepView(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    int index = SELECTED_MENU_INDEX(me->_depP);

    gtk_notebook_set_page(GTK_NOTEBOOK(me->_depTab), index);
}

void RGMainWindow::clickedRecInstall(GtkWidget *self, void *data)
{
  RGMainWindow *me = (RGMainWindow*)g_object_get_data(G_OBJECT(self), "me");
  assert(me);

  RPackage *pkg, *newpkg=NULL;
  const char *depType=NULL, *depPkg=NULL;
  char *recstr=NULL;
  bool ok=false;

  me->_lister->unregisterObserver(me);
  me->setInterfaceLocked(TRUE);
  me->_blockActions = TRUE;

  // save undo state
  RPackageLister::pkgState state;
  me->_lister->saveState(state);
  me->_lister->saveUndoState(state);


  pkg = (RPackage*)g_object_get_data(G_OBJECT(me->_recList),"pkg");
  assert(pkg);

  switch((int)data) {
  case InstallRecommended:
    if(pkg->enumWDeps(depType, depPkg, ok)) {
      do {
	if (!ok && !strcmp(depType,"Recommends")) {
	    newpkg = me->_lister->getElement(depPkg);
	    if(newpkg)
		me->pkgInstallHelper(newpkg);
	    else
		cerr << depPkg << _(" not found") << endl;
	}
      } while(pkg->nextWDeps(depType, depPkg, ok));
    }
    break;
  case InstallSuggested:
    if(pkg->enumWDeps(depType, depPkg, ok)) {
      do {
	if (!ok && !strcmp(depType,"Suggests")) {
	  newpkg=me->_lister->getElement(depPkg);
	  if(newpkg)
	      me->pkgInstallHelper(newpkg);
	  else
	    cerr << depPkg << _(" not found") << endl;
	}
      } while(pkg->nextWDeps(depType, depPkg, ok));
    }
    break;
  case InstallSelected:
      GtkTreeSelection *selection;
      GtkTreeIter iter;
      GList *list, *li;
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (me->_recList));
#if GTK_CHECK_VERSION(2,2,0)
      list = li  = gtk_tree_selection_get_selected_rows(selection, NULL);
#else
      li = list = NULL;
      gtk_tree_selection_selected_foreach(selection, multipleSelectionHelper,
					  &list);
      li = list;
#endif

      while(li != NULL) {
	  // 	  cout << "path is " 
 	  //     << gtk_tree_path_to_string((GtkTreePath*)(li->data)) << endl;
	  gtk_tree_model_get_iter(GTK_TREE_MODEL(me->_recListStore), &iter, 
				  (GtkTreePath*)(li->data));
	  gtk_tree_model_get(GTK_TREE_MODEL(me->_recListStore), &iter, 
			     DEP_NAME_COLUMN, &recstr, 
			     -1);
	  //cout << "selected row is " << recstr << endl;
	  if(!recstr) { 
	      cerr << _("gtk_tree_model_get returned no text") << endl;
	      li=g_list_next(li);
	      continue;
	  }
	  depPkg = index(recstr, ':') + 2;
	  if(!depPkg) {
	      cout << "\":\" not found"<<endl;
	      li=g_list_next(li);
	      continue;
	  }
	  newpkg=(RPackage*)me->_lister->getElement(depPkg);
	  if(newpkg)
	      me->pkgInstallHelper(newpkg);
	  else
	      cerr << depPkg << _(" not found") << endl;
	  li = g_list_next(li);
      }

      // free the list
      g_list_foreach(list, (void (*)(void*,void*))gtk_tree_path_free, NULL);
      g_list_free (list);

    break;
  default:
    cerr << _("clickedRecInstall called with invalid parm: ") << data << endl;
  }

  me->_blockActions = FALSE;
  me->_lister->registerObserver(me);
  me->refreshTable(pkg);
  me->setInterfaceLocked(FALSE);
}

void RGMainWindow::clickedDepList(GtkTreeSelection *selection, gpointer data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    assert(me);

    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *text;

    if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
	gtk_tree_model_get(model, &iter, DEP_PKG_INFO, &text, -1);
	//cout << "clickedDepList: " << text << endl;
	assert(text);
	gtk_label_set_text(GTK_LABEL(me->_depInfoL), utf8(text));
    }
}

void RGMainWindow::clickedAvailDepList(GtkTreeSelection *selection, 
				       gpointer data)
{
  RGMainWindow *me = (RGMainWindow*)data;
  assert(me);

  GtkTreeIter iter;
  GtkTreeModel *model;
  char *text;

  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
      gtk_tree_model_get(model, &iter, DEP_PKG_INFO, &text, -1);
      //cout << "clickedAvailDepList: " << text << endl;
      if(text != NULL)
	  gtk_label_set_text(GTK_LABEL(me->_availDepInfoL), text);
  }
}

void RGMainWindow::showAboutPanel(GtkWidget *self, void *data)
{
    RGMainWindow *win = (RGMainWindow*)data;
    
    if (win->_aboutPanel == NULL)
	win->_aboutPanel = new RGAboutPanel(win);
    win->_aboutPanel->show();
}

void RGMainWindow::helpAction(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;

    me->setStatusText(_("Starting help system"));
 
    // TODO: check for more help systems and fall back to mozilla
    if(FileExists("/usr/bin/yelp"))
	system("yelp man:synaptic.8 &");
    else
	me->_userDialog->warning("Unable to start yelp");
}


void RGMainWindow::searchBeginAction(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    GtkTreeIter iter;

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_treeView));
    gtk_tree_model_get_iter_first(me->_activeTreeModel, &iter);
    gtk_tree_selection_unselect_all(selection);
    gtk_tree_model_iter_next(me->_activeTreeModel, &iter);
    gtk_tree_selection_select_iter(selection, &iter);
    me->searchAction(me->_findText, me);

}

void RGMainWindow::searchNextAction(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    GtkTreeIter iter;
    GList *li, *list=NULL;
    gboolean ok;

    //cout << "searchNextAction()"<<endl;

    // get last selected row
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_treeView));
#if GTK_CHECK_VERSION(2,2,0)
    list = li = gtk_tree_selection_get_selected_rows(selection,
 						     &me->_activeTreeModel);
#else
    li = list = NULL;
    gtk_tree_selection_selected_foreach(selection, multipleSelectionHelper,
					&list);
    li = list;
#endif

    // if not -> take the first row
    if(list==NULL) {
	return;
    } else {
	li = g_list_last(li);
	ok =  gtk_tree_model_get_iter(me->_activeTreeModel, &iter, 
			    (GtkTreePath*)(li->data));
    }
    gtk_tree_selection_unselect_all(selection);
    gtk_tree_model_iter_next(me->_activeTreeModel, &iter);
    gtk_tree_selection_select_iter(selection, &iter);
    me->searchAction(me->_findText, me);
}

void RGMainWindow::searchLackAction(GtkWidget *self, void *data)
{
    GtkTreeIter iter;
    //cout << "search lack called" << endl;
    RGMainWindow *me = (RGMainWindow*)data;

    // check if search string is empty
    const gchar *searchtext = gtk_entry_get_text (GTK_ENTRY (self));
    int len = strlen (searchtext);
    // entry is empty, set search from the begining
    if(len < 1) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_treeView));
	gtk_tree_model_get_iter_first(me->_activeTreeModel, &iter);
	gtk_tree_selection_unselect_all(selection);
	gtk_tree_model_iter_next(me->_activeTreeModel, &iter);
	gtk_tree_selection_select_iter(selection, &iter);
	return;
    }

    if(me->searchLackId)
	gtk_timeout_remove(me->searchLackId);

    me->searchLackId = gtk_timeout_add(_config->FindI("Synaptic::searchLack",1000), searchLackHelper, self);
}

gboolean RGMainWindow::searchLackHelper(void* self)
{
    //cout << "searchLackHelper()" << endl;
    searchAction((GtkWidget*)self, NULL);
    return FALSE;
}

void RGMainWindow::searchAction(GtkWidget *self, void *data)
{
    // search interativly in the tree
    GtkTreeIter iter, parent = {0,};
    GtkTreePath *path;
    GList *li, *list=NULL;
    char *name;
    gboolean ok;

    RGMainWindow *me = (RGMainWindow*)g_object_get_data(G_OBJECT(self),"me");

    // get search string
    const gchar *searchtext = gtk_entry_get_text (GTK_ENTRY (self));
    int len = strlen (searchtext);
    // entry is empty, set search from the begining
    if(len < 1) {
	return;
    }

    // get last selected row
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_treeView));
#if GTK_CHECK_VERSION(2,2,0)
    list = li = gtk_tree_selection_get_selected_rows(selection,
 						     &me->_activeTreeModel);
#else
    li = list = NULL;
    gtk_tree_selection_selected_foreach(selection, multipleSelectionHelper,
					&list);
    li = list;
#endif

    // if not -> take the first row
    if(list==NULL) {
	ok = gtk_tree_model_get_iter_first(me->_activeTreeModel, &iter);
    } else {
	li = g_list_last(li);
	ok =  gtk_tree_model_get_iter(me->_activeTreeModel, &iter, 
			    (GtkTreePath*)(li->data));
	// set parent to a valid location
	gtk_tree_model_iter_parent(me->_activeTreeModel, &parent, &iter);
    }
    gtk_tree_selection_unselect_all(selection);

    // do the real search
    while(1) {
	// 1. search in the current iter-range
	while(ok) {  
	    gtk_tree_model_get(me->_activeTreeModel, &iter,
			       NAME_COLUMN, &name,
			       -1);
	    if(strncasecmp(name, searchtext, len) == 0) {
		if(parent.user_data != NULL) {
		    path = gtk_tree_model_get_path(me->_activeTreeModel, 
						   &parent);
		    gtk_tree_view_expand_row(GTK_TREE_VIEW(me->_treeView), 
					     path, false);
		    // work around a bug in expand_row (it steals the focus)
		    gtk_widget_grab_focus(self);
		    gtk_editable_select_region(GTK_EDITABLE(self),0,0);
		    gtk_editable_set_position (GTK_EDITABLE(self),-1);
		}
		path = gtk_tree_model_get_path(me->_activeTreeModel, &iter);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(me->_treeView), 
					     path, NULL, TRUE, 0.5, 0.0);
		gtk_tree_selection_select_iter(selection, &iter);
		return;
	    }
	    ok = gtk_tree_model_iter_next(me->_activeTreeModel, &iter);
	} 
	
	// 2. if it is not found there, look into the next toplevel subtree

	// special case for the first tree
	if(parent.user_data == NULL) {
	    if(!gtk_tree_model_get_iter_first(me->_activeTreeModel, &parent))
		return;
	} else if(!gtk_tree_model_iter_next(me->_activeTreeModel, &parent))
	    return;
	if(!gtk_tree_model_iter_children(me->_activeTreeModel,&iter,&parent)) 
	    return;
	ok=true;
    }
#if 0
	gtk_tree_model_get(me->_activeTreeModel, &iter,
			   NAME_COLUMN, &name,
			   -1);
	cout << "iter is: " << name << endl;
	// if iter is toplevel -> next toplevel item and search its children
	if(current_subtree.user_data == NULL) {
	    cout << "hello" << endl;
	    ok = gtk_tree_model_get_iter_first(me->_activeTreeModel, &current_subtree);
	} else
	    if(!gtk_tree_model_iter_parent(me->_activeTreeModel, &current_subtree, &selected)) {
		cout << "no parent" << endl;
		current_subtree=selected;
	    }
	// go to next toplevel item
	if(!gtk_tree_model_iter_next(me->_activeTreeModel, &current_subtree)) {
	    cout << "!gtk_tree_model_iter_next()"<<endl;
	    gtk_tree_model_get(me->_activeTreeModel, &current_subtree,
			       NAME_COLUMN, &name,
			       -1);
	    if(name!=NULL)
		cout << "name: " << name << endl;
	    return;
	}
	if(!gtk_tree_model_iter_children(me->_activeTreeModel, &iter, &current_subtree)) {
	    cout << "!gtk_tree_model_iter_children()"<<endl;
	    return;
	}
	ok=true;
    }
#endif
}


void RGMainWindow::closeFilterManagerAction(void *self, bool okcancel)
{
    //cout << "RGMainWindow::closeFilterManagerAction()"<<endl;
    RGMainWindow *me = (RGMainWindow*)self;

    // user clicked ok
    if(okcancel) {
	int i = gtk_option_menu_get_history(GTK_OPTION_MENU(me->_filterPopup));
	me->refreshFilterMenu();
	gtk_option_menu_set_history(GTK_OPTION_MENU(me->_filterPopup), i);
	me->changeFilter(i);
    }

    gtk_widget_set_sensitive(me->_filtersB, TRUE);
    gtk_widget_set_sensitive(me->_filterPopup, TRUE);
    gtk_widget_set_sensitive(me->_filterMenu, TRUE);
}


void RGMainWindow::showFilterManagerWindow(GtkWidget *self, void *data)
{
    RGMainWindow *win = (RGMainWindow*)data;

    if (win->_fmanagerWin == NULL) {
	win->_fmanagerWin = new RGFilterManagerWindow(win, win->_lister);

	win->_fmanagerWin->setCloseCallback(closeFilterManagerAction, win); 
    }

    win->_fmanagerWin->show();

    gtk_widget_set_sensitive(win->_filtersB, FALSE);
    gtk_widget_set_sensitive(win->_filterPopup, FALSE);
    gtk_widget_set_sensitive(win->_filterMenu, FALSE);
}


void RGMainWindow::showSourcesWindow(GtkWidget *self, void *data)
{
    RGMainWindow *win = (RGMainWindow*)data;
        
    bool Ok = false;
    {
    RGRepositoryEditor w(win);
    Ok = w.Run();
    }
    RGFlushInterface();
    if (Ok == true && _config->FindB("Synaptic::UpdateAfterSrcChange")) {
	win->updateClicked(NULL, data);
    }
}

void RGMainWindow::showRepositoriesWindow()
{
    showSourcesWindow(NULL, this);
}

void RGMainWindow::pkgReconfigureClicked(GtkWidget *self, void *data)
{
    char frontend[] = "gnome";
    char *cmd;
    RGMainWindow *me = (RGMainWindow*)data;
    //cout << "RGMainWindow::pkgReconfigureClicked()" << endl;

    RPackage *pkg=NULL;
    pkg = me->_lister->getElement("libgnome-perl");
    if(pkg && pkg->installedVersion() == NULL) {
	me->_userDialog->error(_("No libgnome-perl installed\n\n"
				 "You have to install libgnome-perl to "
				 "use dpkg-reconfigure with synaptic"));
	return;
    }

    me->setStatusText(_("Starting dpkg-reconfigure"));
    cmd = g_strdup_printf("/usr/sbin/dpkg-reconfigure -f%s %s &", 
			  frontend, me->selectedPackage()->name());
    system(cmd);
}


void RGMainWindow::pkgHelpClicked(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;

    //cout << "RGMainWindow::pkgHelpClicked()" << endl;
    me->setStatusText(_("Starting package help"));
    
    system(g_strdup_printf("dwww %s &", me->selectedPackage()->name()));
}


void RGMainWindow::showConfigWindow(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    
    if (me->_configWin == NULL) {
	me->_configWin = new RGPreferencesWindow(me, me->_lister);
    }

    me->_configWin->show();
}

void RGMainWindow::showSetOptWindow(GtkWidget *self, void *data)
{
    RGMainWindow *win = (RGMainWindow*)data;
    
    if (win->_setOptWin == NULL)
	win->_setOptWin = new RGSetOptWindow(win);

    win->_setOptWin->show();
}

void RGMainWindow::changedFilter(GtkWidget *self)
{
    int filter;
    RGMainWindow *mainw = (RGMainWindow*)gtk_object_get_data(GTK_OBJECT(self), "me");
    
    filter = (int)gtk_object_get_data(GTK_OBJECT(self), "index");
    
    mainw->changeFilter(filter, false);
}


void RGMainWindow::changeFilter(int filter, bool sethistory)
{
    //cout << "RGMainWindow::changeFilter()"<<endl;
    
    if (sethistory) {
	gtk_option_menu_set_history(GTK_OPTION_MENU(_filterPopup), filter);
    }

    RPackage *pkg = selectedPackage();
    setInterfaceLocked(TRUE); 
    _blockActions = TRUE;

    // try to set filter
    if(filter > 0) {
	_lister->setFilter(filter-1);
	RFilter *pkgfilter = _lister->getFilter();
	if(pkgfilter != NULL) {
	    // -2 because "0" is "unchanged" and "1" is the spacer in the menu
	    // FIXME: yes I know this sucks
	    int mode = pkgfilter->getViewMode().viewMode-2; 
	    if(mode>=0)
		changeTreeDisplayMode((RPackageLister::treeDisplayMode)mode);
	    else
		changeTreeDisplayMode(_menuDisplayMode);

	    // FIXME: same problem as above, this magic numbers suck
	    int expand=pkgfilter->getViewMode().expandMode;
	    if(expand == 2)
		gtk_tree_view_expand_all(GTK_TREE_VIEW(_treeView));
	    if(expand == 3)
		gtk_tree_view_collapse_all(GTK_TREE_VIEW(_treeView));
	} else {
	    filter = 0;
	}
    }

    // no filter given or not available from above
    if (filter == 0) { // no filter
	_lister->setFilter();
	changeTreeDisplayMode(_menuDisplayMode);
    }

    refreshTable(pkg);
    _blockActions = FALSE;
    setInterfaceLocked(FALSE);
    setStatusText();
}




void RGMainWindow::updateClicked(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;

    // need to delete dialogs, as they might have data pointing
    // to old stuff
//xxx    delete me->_filterWin;
//xxx    delete me->_fmanagerWin;
    me->_fmanagerWin = NULL;
    me->_filterWin = NULL;

    RGFetchProgress *progress = new RGFetchProgress(me);
    progress->setTitle(_("Retrieving Index Files"));

    me->setStatusText(_("Updating Package Lists from Servers..."));

    me->setInterfaceLocked(TRUE);
    me->setTreeLocked(TRUE);
    me->_lister->unregisterObserver(me);

    // save to temporary file
    const gchar *file = g_strdup_printf("%s/selections.update",RConfDir().c_str());
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
    g_free((void*)file);

    me->setTreeLocked(FALSE);
    me->refreshTable();
    me->setInterfaceLocked(FALSE);
    me->setStatusText();
}


RPackage *RGMainWindow::selectedPackage()
{
    if(_activeTreeModel == NULL)
	return NULL;

    //cout << "RGMainWindow::selectedPackage()" << endl;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    RPackage *pkg = NULL;
    GList *li, *list;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (_treeView));
#if GTK_CHECK_VERSION(2,2,0)
    list = li = gtk_tree_selection_get_selected_rows(selection,
						     (GtkTreeModel**)(&_activeTreeModel));
#else
    list = li = NULL;
    gtk_tree_selection_selected_foreach(selection, multipleSelectionHelper,
					&list);
    li = list;
#endif
    // list is empty
    if(li == NULL) 
	return NULL;
  
    // we are only interessted in the last element
    li = g_list_last(li);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(_activeTreeModel), &iter, 
			    (GtkTreePath*)(li->data));

    gtk_tree_model_get(GTK_TREE_MODEL(_activeTreeModel), &iter, 
		       PKG_COLUMN, &pkg, -1);
  

    // free the list
    g_list_foreach(list, (void (*)(void*,void*))gtk_tree_path_free, NULL);
    g_list_free (list);


    return pkg;
}


void RGMainWindow::fixBrokenClicked(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
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


void RGMainWindow::upgradeClicked(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    RPackage *pkg = me->selectedPackage();
    bool res;

    if (!me->_lister->check()) {
	me->_userDialog->error(
		_("Automatic upgrade selection not possible\n"
		  "with broken packages. Please fix them first."));
	return;
    }
    
    me->setInterfaceLocked(TRUE);
    me->setStatusText(_("Performing automatic selection of upgradadable packages..."));

    me->_lister->saveUndoState();

    res = me->_lister->upgrade();
    me->refreshTable(pkg);

    if (res)
	me->setStatusText(_("Automatic selection of upgradadable packages done."));
    else
	me->setStatusText(_("Automatic upgrade selection failed."));
    
    me->setInterfaceLocked(FALSE);
    me->showErrors();
}



void RGMainWindow::distUpgradeClicked(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    RPackage *pkg = me->selectedPackage();
    bool res;

    if (!me->_lister->check()) {
	me->_userDialog->error(
			_("Automatic upgrade selection not possible\n"
			  "with broken packages. Please fix them first."));
	return;
    }

    me->setInterfaceLocked(TRUE);
    me->setStatusText(_("Performing selection for distribution upgrade..."));

    // save state
    me->_lister->saveUndoState();

    res = me->_lister->distUpgrade();
     me->refreshTable(pkg);

    if (res)
	me->setStatusText(_("Selection for distribution upgrade done."));
    else
	me->setStatusText(_("Selection for distribution upgrade failed."));

    me->setInterfaceLocked(FALSE);
    me->showErrors();
}

void RGMainWindow::proceed()
{
    proceedClicked(NULL, (void*)this);
}

void RGMainWindow::proceedClicked(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    RGSummaryWindow *summ;

    // check whether we can really do it
    if (!me->_lister->check()) {
	me->_userDialog->error(
			    _("Operation not possible with broken packages.\n"
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

    me->setStatusText(_("Performing selected changes... this may take a while"));

    // fetch packages
    RGFetchProgress *fprogress = new RGFetchProgress(me);
    fprogress->setTitle(_("Retrieving Package Files"));

    // Do not let the treeview access the cache during the update.
    me->setTreeLocked(TRUE);

    // save selections to temporary file
    const gchar *file = g_strdup_printf("%s/selections.proceed",RConfDir().c_str());
    ofstream out(file);
    if (!out != 0) {
	_error->Error(_("Can't write %s"), file);
	me->_userDialog->showErrors();
	return;
    }
    me->_lister->writeSelections(out, false);


    RInstallProgress *iprogress;
#ifdef HAVE_ZVT
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

#ifdef HAVE_ZVT
    // wait until the zvt dialog is closed
    if (zvt != NULL) {
	while(GTK_WIDGET_VISIBLE(GTK_WIDGET(zvt->window()))) {
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
    g_free((void*)file);
    
    me->setTreeLocked(FALSE);
    me->refreshTable();
    me->setInterfaceLocked(FALSE);
    me->setStatusText();
}


bool RGMainWindow::showErrors()
{
    return _userDialog->showErrors();
}


static void appendTag(GString *str, const char *tag, const char *value)
{
    if (!value)
	value = _("N/A");

    g_string_append_printf(str, "%s: %s\n", tag, utf8(value));
}

static void appendTag(GString *str, const char *tag, const int value)
{
    string strVal;
    // we can never have values of zero or less
    if (value <= 0)
	strVal = _("N/A");
    else
	strVal = SizeToStr(value);
    g_string_append_printf(str, "%s: %s\n", tag, utf8(strVal.c_str()));
}


void RGMainWindow::notifyChange(RPackage *pkg)
{
  //cout << "RGMainWindow::notifyChange("<<pkg->name()<<")" << endl;
    // we changed pkg's status
    if(pkg!=NULL) {
	updateDynPackageInfo(pkg);
	refreshTable(pkg);
    }
    setStatusText();
}

void RGMainWindow::forgetNewPackages()
{
    //cout << "forgetNewPackages called" << endl;
    unsigned int row=0;
    while (row < _lister->count()) {
	RPackage *elem = _lister->getElement(row);
	if(elem->getOtherStatus() && RPackage::ONew)
	    elem->setNew(false);
    }
    _roptions->forgetNewPackages();
}


void RGMainWindow::refreshTable(RPackage *selectedPkg)
{
    //cout << "RGMainWindow::refreshTable(RPackage *selectedPkg)"<<endl;
  
  updatePackageInfo(selectedPkg);
  
  setStatusText();
}

void RGMainWindow::updatePackageStatus(RPackage *pkg)
{
    if(pkg==NULL) {
	gtk_widget_set_sensitive(_installM, FALSE);
	gtk_widget_set_sensitive(_pkgupgradeM, FALSE);
	gtk_widget_set_sensitive(_removeM, FALSE);
	gtk_widget_set_sensitive(_remove_w_depsM, FALSE);
	gtk_widget_set_sensitive(_purgeM, FALSE);
	gtk_widget_set_sensitive(_pinM, FALSE);
	return;
    }

    bool installed = FALSE;
    RPackage::PackageStatus status = pkg->getStatus();
    RPackage::MarkedStatus mstatus = pkg->getMarkedStatus();
    int other = pkg->getOtherStatus();

    switch (status) {
     case RPackage::SInstalledUpdated:
	gtk_widget_set_sensitive(_actionB[1], FALSE);
	gtk_widget_set_sensitive(_actionB[2], TRUE);
	gtk_widget_set_sensitive(_installM, FALSE);
	gtk_widget_set_sensitive(_pkgupgradeM, FALSE);
	gtk_widget_set_sensitive(_removeM, TRUE);
	gtk_widget_set_sensitive(_remove_w_depsM, TRUE);
	gtk_widget_set_sensitive(_purgeM, TRUE);
	gtk_widget_set_sensitive(_pinB, TRUE);
	gtk_widget_set_sensitive(_pinM, TRUE);
	gtk_widget_set_sensitive(_pkgHelp, TRUE);
	installed = TRUE;
	break;
	
     case RPackage::SInstalledOutdated:
	gtk_widget_set_sensitive(_actionB[1], TRUE);
	gtk_widget_set_sensitive(_actionB[2], TRUE);
	gtk_widget_set_sensitive(_installM, FALSE);
	gtk_widget_set_sensitive(_pkgupgradeM, TRUE);
	gtk_widget_set_sensitive(_removeM, TRUE);
	gtk_widget_set_sensitive(_purgeM, TRUE);
	gtk_widget_set_sensitive(_remove_w_depsM, TRUE);
	gtk_widget_set_sensitive(_pinB, TRUE);
	gtk_widget_set_sensitive(_pinM, TRUE);
	gtk_widget_set_sensitive(_pkgHelp, TRUE);
	installed = TRUE;
	break;
	
     case RPackage::SInstalledBroken:
	gtk_widget_set_sensitive(_actionB[1], FALSE);
	gtk_widget_set_sensitive(_actionB[2], TRUE);
	gtk_widget_set_sensitive(_installM, FALSE);
	gtk_widget_set_sensitive(_pkgupgradeM, FALSE);
	gtk_widget_set_sensitive(_removeM, TRUE);
	gtk_widget_set_sensitive(_purgeM, TRUE);
	gtk_widget_set_sensitive(_remove_w_depsM, TRUE);
	gtk_widget_set_sensitive(_pinB, TRUE);
	gtk_widget_set_sensitive(_pinM, TRUE);
	gtk_widget_set_sensitive(_pkgHelp, TRUE);
	installed = TRUE;
	break;
	
     case RPackage::SNotInstalled:
	gtk_widget_set_sensitive(_actionB[1], TRUE);
	gtk_widget_set_sensitive(_actionB[2], FALSE);
	gtk_widget_set_sensitive(_installM, TRUE);
	gtk_widget_set_sensitive(_pkgupgradeM, FALSE);
	gtk_widget_set_sensitive(_removeM, FALSE);
	gtk_widget_set_sensitive(_purgeM, FALSE);
	gtk_widget_set_sensitive(_remove_w_depsM, FALSE);
	gtk_widget_set_sensitive(_pinB, FALSE);
	gtk_widget_set_sensitive(_pinM, FALSE);
	gtk_widget_set_sensitive(_pkgHelp, FALSE);
	break;
    }

    _currentB = NULL;
    switch (mstatus) {
     case RPackage::MHeld:
     case RPackage::MKeep:
	_currentB = _actionB[0];
	if (installed) 
	    gtk_label_set_text(GTK_LABEL(_stateL), _("Package is installed."));
	else
	    gtk_label_set_text(GTK_LABEL(_stateL), _("Package is not installed."));
	break;

     case RPackage::MInstall:
	gtk_label_set_text(GTK_LABEL(_stateL), _("Package will be installed."));
	_currentB = _actionB[1];
	break;
	
     case RPackage::MUpgrade:
	_currentB = _actionB[1];
	gtk_label_set_text(GTK_LABEL(_stateL), _("Package will be upgraded."));
	break;
	
     case RPackage::MDowngrade:
	gtk_widget_set_sensitive(_actionB[1], TRUE);
	_currentB = _actionB[1];
	gtk_label_set_text(GTK_LABEL(_stateL), _("Package will be downgraded."));
	break;
	
     case RPackage::MRemove:
	_currentB = _actionB[2];
	gtk_label_set_text(GTK_LABEL(_stateL), _("Package will be uninstalled."));
	break;
    case RPackage::MBroken:
      gtk_label_set_text(GTK_LABEL(_stateL), _("Package is broken."));
      break;
    case RPackage::MPinned:
    case RPackage::MNew:
	/* nothing */
	break;
    }

    gtk_widget_set_sensitive(_pkgReconfigure, FALSE);
    switch(other){
    case RPackage::OPinned:
	gtk_label_set_text(GTK_LABEL(_stateL), _("Package is pinned."));
	break;
    case RPackage::ONew:
	gtk_label_set_text(GTK_LABEL(_stateL), _("Package is new."));
	break;
    }

    if((pkg->dependsOn("debconf")||pkg->dependsOn("debconf-i18n")) && 
       (status == RPackage::SInstalledUpdated || 
	status == RPackage::SInstalledOutdated)) 
    {
	gtk_widget_set_sensitive(_pkgReconfigure, TRUE);
    }

    if(RPackage::OResidualConfig & pkg->getOtherStatus())
	gtk_widget_set_sensitive(_purgeM, TRUE);

    // set button, but disable toggle signal
    bool locked = (RPackage::OPinned & pkg->getOtherStatus());
    _blockActions = TRUE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_pinB), locked);
    _blockActions = FALSE;
    gtk_widget_set_sensitive(_actionB[0], TRUE);

    // gray out "remove" button
    if(locked) {
      gtk_label_set_text(GTK_LABEL(_stateL), _("Package is pinned."));
      gtk_widget_set_sensitive(_actionB[2], FALSE);
    }

    if (pkg->wouldBreak()) {
      gtk_label_set_text(GTK_LABEL(_stateL), _("Broken dependencies."));
      gtk_image_set_from_pixbuf(GTK_IMAGE(_stateP), 
				StatusPixbuf[RPackage::MBroken]);

    } else if (mstatus==RPackage::MKeep && status==RPackage::SInstalledOutdated) {
      gtk_image_set_from_pixbuf(GTK_IMAGE(_stateP), 
				StatusPixbuf[RPackage::MHeld]);
    }  else if(other & RPackage::ONew) {
      gtk_image_set_from_pixbuf(GTK_IMAGE(_stateP), 
				StatusPixbuf[RPackage::MNew]);
    } else if (locked) {
      gtk_image_set_from_pixbuf(GTK_IMAGE(_stateP), 
				StatusPixbuf[RPackage::MPinned]);
    } else {
      gtk_image_set_from_pixbuf(GTK_IMAGE(_stateP), 
				StatusPixbuf[(int)mstatus]);
    }
    
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_currentB)) == FALSE) {
	_blockActions = TRUE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_currentB), TRUE);
	_blockActions = FALSE;
    }
}


void RGMainWindow::updateDynPackageInfo(RPackage *pkg)
{
    //cout << "updateDynPackageInfo()" << endl;

    if(_blockActions)
	return;
    
    updatePackageStatus(pkg);
    
    if(pkg == NULL)
	return;

    // dependencies
    gtk_label_set_text(GTK_LABEL(_depInfoL), "");    

    gtk_list_store_clear(_depListStore);
    
    bool byProvider = TRUE;

    char buffer[512];
    GtkTreeIter iter;

    const char *depType, *depPkg, *depName, *depVer;
    char *summary; // XXX Why not const?
    bool ok;
    if (pkg->enumDeps(depType, depName, depPkg, depVer, summary, ok)) {
	do {
	    //cout << "got: " << depType << " " << depName << endl;
	    if (byProvider) {
#ifdef HAVE_RPM
		snprintf(buffer, sizeof(buffer), "%s: %s %s", 
			 utf8(depType), depPkg ? depPkg : depName, depVer);
#else
		snprintf(buffer, sizeof(buffer), "%s: %s %s", 
			 utf8(depType), depName, depVer); 
#endif
	    } else {
		snprintf(buffer, sizeof(buffer), "%s: %s %s[%s]",
			 utf8(depType), depName, depVer,
			 depPkg ? depPkg : "-");
	    }
	    //cout << "buffer is: " << buffer << endl;
	    
	    // check if this item is duplicated
	    bool dup = FALSE;
	    bool valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(_depListStore), &iter);

	    while(valid) {
		char *str;
		
		gtk_tree_model_get(GTK_TREE_MODEL(_depListStore), &iter,
				   DEP_NAME_COLUMN, &str, 
				   -1);
		if (g_strcasecmp(str, buffer) == 0) {
		    dup = TRUE;
		    g_free(str);
		    break;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(_depListStore),&iter);
		g_free(str);
	    }

	    if (!dup) {
		gtk_list_store_append(_depListStore, &iter);
		gtk_list_store_set(_depListStore, &iter,
				   DEP_NAME_COLUMN, buffer,
				   DEP_IS_NOT_AVAILABLE, !ok,  
				   DEP_IS_NOT_AVAILABLE_COLOR, "#E0E000000000",
				   DEP_PKG_INFO, summary,
				   -1);
	    }
	} while (pkg->nextDeps(depType, depName, depPkg, depVer, summary, ok));
    }

    // reverse dependencies
    gtk_list_store_clear(_rdepListStore);
    if (pkg->enumRDeps(depName, depPkg)) {
	do {
	    snprintf(buffer, sizeof(buffer), "%s (%s)", depName, depPkg);
	    gtk_list_store_append(_rdepListStore, &iter);
	    gtk_list_store_set(_rdepListStore, &iter,
			       DEP_NAME_COLUMN, buffer,
			       -1);
	} while (pkg->nextRDeps(depName, depPkg));
    }

    // weak dependencies
    //cout << "building weak dependencies list" << endl;
    gtk_list_store_clear(_recListStore);
    g_object_set_data(G_OBJECT(_recList), "pkg", pkg);
    if (pkg->enumWDeps(depType, depPkg, ok)) {
	do {
	    GtkTreeIter iter;
	    snprintf(buffer, sizeof(buffer), "%s: %s", 
		     utf8(depType), depPkg);
	    gtk_list_store_append(_recListStore, &iter);
	    gtk_list_store_set(_recListStore, &iter, 
			       DEP_NAME_COLUMN, buffer, 
			       DEP_IS_NOT_AVAILABLE, !ok,  
			       DEP_IS_NOT_AVAILABLE_COLOR,  "#E0E000000000",
			       -1); 
	} while (pkg->nextWDeps(depName, depPkg, ok));
    }    

    //provides list
    gtk_list_store_clear(_providesListStore);
    vector<const char *> provides = pkg->provides();
    for(unsigned int i=0; i<provides.size();i++) {
	//cout << "got: " << provides[i] << endl;
	GtkTreeIter iter;
	gtk_list_store_append(_providesListStore, &iter);
	gtk_list_store_set(_providesListStore, &iter, 
			   DEP_NAME_COLUMN, utf8(provides[i]), 
			   -1); 
    }

    // dependencies of the available package
    gtk_list_store_clear(_availDepListStore);
    gtk_label_set_text(GTK_LABEL(_availDepInfoL), "");    
    byProvider = TRUE;
    if (pkg->enumAvailDeps(depType, depName, depPkg, depVer, summary, ok)) {
	do {
	    char buffer[512];
	    if (byProvider) {
		snprintf(buffer, sizeof(buffer), "%s: %s %s", 
			 utf8(depType), depPkg ? depPkg : depName, depVer);
	    } else {
		snprintf(buffer, sizeof(buffer), "%s: %s %s[%s]",
			 utf8(depType), depName, depVer,
			 depPkg ? depPkg : "-");
	    }
	    // check if this item is duplicated
	    bool dup = FALSE;
	    bool valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(_availDepListStore), &iter);
	    while(valid) {
		char *str;
		gtk_tree_model_get(GTK_TREE_MODEL(_availDepListStore), &iter,
				   DEP_NAME_COLUMN, &str, 
				   -1);

		if (g_strcasecmp(str, buffer) == 0) {
		    dup = TRUE;
		    g_free(str);
		    break;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(_availDepListStore),&iter);
		g_free(str);
	    }
	    if (!dup) {
		gtk_list_store_append(_availDepListStore, &iter);
		gtk_list_store_set(_availDepListStore, &iter,
				   DEP_NAME_COLUMN, buffer,
				   DEP_IS_NOT_AVAILABLE, !ok,  
				   DEP_IS_NOT_AVAILABLE_COLOR, "#E0E000000000",
				   DEP_PKG_INFO, summary,
				   -1);
	    }
	} while (pkg->nextDeps(depType, depName, depPkg, depVer, summary, ok));
    }
}

void RGMainWindow::updateVersionButtons(RPackage *pkg)
{
    // remove old radiobuttons
    GtkWidget *vbox = glade_xml_get_widget(_gladeXML, "vbox_versions");
    gtk_container_foreach(GTK_CONTAINER(vbox),
			  (void (*)(GtkWidget*, void*))(gtk_widget_destroy),
			  NULL);

    if(pkg == NULL)
	return;

    // get available versions
    static vector<pair<string,string> > versions;
    versions.clear();
    versions = pkg->getAvailableVersions();

    int mstatus = pkg->getMarkedStatus();

    // build new checkboxes
    GtkWidget *button;
    bool found=false;
    for(unsigned int i=0;i<versions.size();i++) {
	// first radiobutton makes the radio-button group 
	if(i==0)
	    button = gtk_radio_button_new_with_label(NULL,g_strdup_printf("%s (%s)",versions[i].first.c_str(),versions[i].second.c_str()));
	else
	    button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button),g_strdup_printf("%s (%s)",versions[i].first.c_str(),versions[i].second.c_str()));
	g_object_set_data(G_OBJECT(button),"me",this);
	g_object_set_data(G_OBJECT(button),"pkg",pkg);
	// check what version is installed or will installed
	if(mstatus == RPackage::MKeep) {
	    if(pkg->installedVersion() &&
	       (string(pkg->installedVersion()) == versions[i].first)) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), true);
		found=true;
	    }
	} else if(mstatus != RPackage::MRemove) {
	    if(pkg->availableVersion() &&
	       (string(pkg->availableVersion()) == versions[i].first)) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), true);
		found=true;
	    }
	} 
	g_signal_connect(G_OBJECT(button),"toggled",
			 G_CALLBACK(installFromVersion),
			 (gchar*)versions[i].first.c_str());
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 6);
	gtk_widget_show(button);
    }
    button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button),_("not installed"));
    g_object_set_data(G_OBJECT(button),"me",this);
    g_object_set_data(G_OBJECT(button),"pkg",pkg);
    if(!found)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), true);
    g_signal_connect(G_OBJECT(button),"toggled",
		     G_CALLBACK(installFromVersion),
		     NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 6);
    gtk_widget_show(button);

    gtk_widget_show(vbox);

    // mvo: we work around a stupid bug in libglade/gtk here (hide/show scrolledwin)
    gtk_widget_hide(glade_xml_get_widget(_gladeXML,"scrolledwindow_versions"));
    gtk_widget_show(glade_xml_get_widget(_gladeXML,"scrolledwindow_versions"));
}

void RGMainWindow::updatePackageInfo(RPackage *pkg)
{
    RPackage::UpdateImportance importance;
    RPackage::PackageStatus status;
    RPackage::MarkedStatus mstatus;
    
    if(_blockActions)
	return;

    if (!pkg) {
	gtk_label_set_markup(GTK_LABEL(_nameL), "<span size=\"large\"><b></b></span>");
	gtk_label_set_text(GTK_LABEL(_summL), "");
	gtk_label_set_text(GTK_LABEL(_infoL), "");
	gtk_label_set_text(GTK_LABEL(_stateL), "");
	gtk_label_set_text(GTK_LABEL(_depInfoL), "");
	gtk_image_set_from_pixbuf(GTK_IMAGE(_stateP), StatusPixbuf[0]);

	gtk_widget_set_sensitive(_pkginfo, FALSE);

	updateVersionButtons(NULL);
	updatePackageStatus(NULL);
	return;
    }
    gtk_widget_set_sensitive(_pkginfo, TRUE);

    status = pkg->getStatus();
    mstatus = pkg->getMarkedStatus();

    updateVersionButtons(pkg);

    if (mstatus == RPackage::MDowngrade) {
	gtk_label_set_markup_with_mnemonic(
		GTK_LABEL(_actionBInstallLabel), _("_Downgrade"));
    } else if (status == RPackage::SNotInstalled) {
	gtk_label_set_markup_with_mnemonic(
		GTK_LABEL(_actionBInstallLabel), _("_Install Latest Version"));
    } else {
	gtk_label_set_markup_with_mnemonic(
		GTK_LABEL(_actionBInstallLabel), _("_Upgrade"));
    }
    
    // not used right now
    importance = pkg->updateImportance();
    
    // name/summary
    gchar *msg = g_strdup_printf("<span size=\"large\"><b>%s</b></span>",utf8(pkg->name()));
    gtk_label_set_markup(GTK_LABEL(_nameL), msg);
    gtk_label_set_text(GTK_LABEL(_summL), utf8(pkg->summary()));
    g_free(msg);
    // package info
    
    // common information regardless of state
    GString *info = g_string_new("");
    appendTag(info, _("Section"), pkg->section());
    appendTag(info,_("Priority"), pkg->priority());
#ifdef HAVE_DEBTAGS
    appendTag(info,_("Tags"), pkg->tags());
#endif
    appendTag(info,_("Maintainer"), pkg->maintainer());

    g_string_append(info, _("\nInstalled Package:\n"));
    appendTag(info,_("\tVersion"), pkg->installedVersion());
    appendTag(info, _("\tSize"), pkg->installedSize());

    g_string_append(info, _("\nAvailable Package:\n"));
    appendTag(info,_("\tVersion"),pkg->availableVersion());
    appendTag(info, _("\tSize"), pkg->availableInstalledSize());
    appendTag(info, _("\tPackage Size"), pkg->availablePackageSize());

    gtk_label_set_text(GTK_LABEL(_infoL), info->str);
    g_string_free(info, TRUE);

    // description
    GtkTextIter iter;
    gtk_text_buffer_set_text(_descrBuffer, utf8(pkg->description()), -1);
    gtk_text_buffer_get_iter_at_offset(_descrBuffer, &iter, 0);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(_descrView), &iter,0,FALSE,0,0);

#ifndef HAVE_RPM
   // files
    gtk_text_buffer_set_text(_filesBuffer, utf8(pkg->installedFiles()), -1);
    gtk_text_buffer_get_iter_at_offset(_filesBuffer, &iter, 0);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(_filesView), &iter,0,FALSE,0,0);
#endif

    updateDynPackageInfo(pkg);
    setStatusText();
}

void RGMainWindow::menuPinClicked(GtkWidget *self, void *data)
{
  RGMainWindow *me = (RGMainWindow*)data;
  
  gtk_signal_emit_by_name(GTK_OBJECT(me->_pinB),"clicked", me);
}

void RGMainWindow::pinClicked(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    bool active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_pinB));
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    RPackage *pkg;

    if(me->_blockActions)
      return;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (me->_treeView));
    GList *li, *list;
#if GTK_CHECK_VERSION(2,2,0)
    list = li = gtk_tree_selection_get_selected_rows(selection,
 						     &me->_activeTreeModel);
#else
    li = list = NULL;
    gtk_tree_selection_selected_foreach(selection, multipleSelectionHelper,
					&list);
    li = list;
#endif
    if(li == NULL)
	return;
    
    me->setInterfaceLocked(TRUE);
    me->setTreeLocked(TRUE);
    me->_lister->unregisterObserver(me);

    // save to temporary file
    const gchar *file = g_strdup_printf("%s/selections.hold",RConfDir().c_str());
    ofstream out(file);
    if (!out != 0) {
	_error->Error(_("Can't write %s"), file);
	me->_userDialog->showErrors();
	return;
    }
    me->_lister->writeSelections(out, false);

    while(li != NULL) {
	gtk_tree_model_get_iter(me->_activeTreeModel, &iter, 
				(GtkTreePath*)(li->data));
	gtk_tree_model_get(me->_activeTreeModel, &iter, 
			   PKG_COLUMN, &pkg, -1);
	if (pkg == NULL) {
	    li=g_list_next(li);
	    continue;    
	}

	pkg->setPinned(active);
	_roptions->setPackageLock(pkg->name(), active);
	li=g_list_next(li);
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
    g_free((void*)file);

    // free the list
    g_list_foreach(list, (void (*)(void*,void*))gtk_tree_path_free, NULL);
    g_list_free (list);

    me->_lister->registerObserver(me);
    me->setTreeLocked(FALSE);
    me->refreshTable();
    me->setInterfaceLocked(FALSE);
}



void RGMainWindow::menuActionClicked(GtkWidget *self, void *data)
{
  RGMainWindow *me = (RGMainWindow*)g_object_get_data(G_OBJECT(self), "me");

  // do the work!
  doPkgAction(me, (RGPkgAction)GPOINTER_TO_INT(data));
}

void RGMainWindow::actionClicked(GtkWidget *clickedB, void *data)
{
  RGMainWindow *me = (RGMainWindow*)data;
  int i;

  if (me->_currentB == clickedB || me->_blockActions) 
    return;

  // what button was pressed?
  for (i = 0; i < 3 && me->_actionB[i] != clickedB; i++);

  // can bind any of the three delete actions (delete, purge, del_with_deps)
  // to the button
  if(i == PKG_DELETE) 
      i = _config->FindI("Synaptic::delAction", PKG_DELETE);

  // do the work!
  doPkgAction(me, (RGPkgAction)i);

  me->_currentB = clickedB;
}

// install a specific version
void RGMainWindow::installFromVersion(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)g_object_get_data(G_OBJECT(self),"me");
    RPackage *pkg = (RPackage*)g_object_get_data(G_OBJECT(self),"pkg");
    gchar *verInfo = (gchar*)data;

    // check if it's a interessting event
    if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self))) {
	return;
    }

    // set pkg to "not installed" 
    if(verInfo == NULL) {
	pkg->unsetVersion();
	me->doPkgAction(me,PKG_DELETE);
	return;
    }

    string instVer;
    if(pkg->installedVersion() == NULL)
	instVer = "";
    else 
	instVer = pkg->installedVersion();

    pkg->setNotify(false);
    if(instVer == string(verInfo)) {
	pkg->unsetVersion();
	me->doPkgAction(me,PKG_KEEP);
    } else {
	pkg->setVersion(verInfo);
	me->doPkgAction(me,PKG_INSTALL);
	// check if this version was possible to install/upgrade/downgrade
	// if not, unset the candiate version 
	int mstatus = pkg->getMarkedStatus();
	if(!(mstatus == RPackage::MInstall ||
	     mstatus == RPackage::MUpgrade ||
	     mstatus == RPackage::MDowngrade) ) {
	    pkg->unsetVersion();
	}
    }
    pkg->setNotify(true);
//     cout << "mstatus " << pkg->getMarkedStatus() << endl;
//     cout << "status " << pkg->getStatus() << endl;
}

void RGMainWindow::doPkgAction(RGMainWindow *me, RGPkgAction action)
{
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GList *li, *list;

  //cout << "RGMainWindow::doPkgAction()" << endl;

  me->setInterfaceLocked(TRUE);
  me->_blockActions = TRUE;

  // get list of selected pkgs
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (me->_treeView));
#if GTK_CHECK_VERSION(2,2,0)
   list = li = gtk_tree_selection_get_selected_rows(selection,
						    &me->_activeTreeModel);
#else
   li = list = NULL;
   gtk_tree_selection_selected_foreach(selection, multipleSelectionHelper,&list);
   li = list;
#endif

  // save pkg state
  RPackageLister::pkgState state;
  bool ask = _config->FindB("Synaptic::AskRelated", true);

  // we always save the state (for undo)
  me->_lister->saveState(state);
  if (ask) {
    me->_lister->unregisterObserver(me);
  }
  me->_lister->notifyCachePreChange();

  // We block notifications in setInstall() and friends, since it'd
  // kill the algorithm in a long loop with many selected packages.
  me->_lister->notifyPreChange(NULL);

  // do the work
  vector<RPackage*> exclude;
  vector<RPackage*> instPkgs;
  RPackage *pkg = NULL;

  while(li != NULL) {
      //cout << "doPkgAction()/loop" << endl;
      gtk_tree_model_get_iter(me->_activeTreeModel, &iter, 
			      (GtkTreePath*)(li->data));
      gtk_tree_model_get(me->_activeTreeModel, &iter, 
			 PKG_COLUMN, &pkg, -1);
      li = g_list_next(li);
      if (pkg == NULL)
	  continue;

      pkg->setNotify(false);
      
      // needed for the stateChange 
      exclude.push_back(pkg);
      /* do the dirty deed */
      switch (action) {
      case PKG_KEEP: // keep
	  me->pkgKeepHelper(pkg);
	  break;
      case PKG_INSTALL: // install
	  instPkgs.push_back(pkg);
	  me->pkgInstallHelper(pkg, false);
	  if(_config->FindB("Synaptic::UseRecommends",0)) {
	      //cout << "auto installing recommended" << endl;
	      me->clickedRecInstall(me->_win, GINT_TO_POINTER(InstallRecommended));
	  }
	  if(_config->FindB("Synaptic::UseSuggests",0)) {
	      //cout << "auto installing suggested" << endl;
	      me->clickedRecInstall(me->_win, GINT_TO_POINTER(InstallSuggested));
	  }
	  break;
      case PKG_DELETE: // delete
	  me->pkgRemoveHelper(pkg);
	  break;
      case PKG_PURGE:  // purge
	  me->pkgRemoveHelper(pkg, true);
	  break;
      case PKG_DELETE_WITH_DEPS:  
	  me->pkgRemoveHelper(pkg, true, true);
	  break;
      default:
	  cout <<"uh oh!!!!!!!!!"<<endl;
	  break;
      }

      pkg->setNotify(true);
  }

  // Do it just once, otherwise it'd kill a long installation list.
  if (!me->_lister->check())
      me->_lister->fixBroken();

  me->_lister->notifyPostChange(NULL);

  me->_lister->notifyCachePostChange();

  vector<RPackage*> toKeep;
  vector<RPackage*> toInstall; 
  vector<RPackage*> toUpgrade; 
  vector<RPackage*> toDowngrade; 
  vector<RPackage*> toRemove;

  // ask if the user really want this changes
  bool changed=true;
  if (ask && me->_lister->getStateChanges(state, toKeep, toInstall,
					  toUpgrade, toRemove, toDowngrade,
					  exclude)) {
      RGChangesWindow *chng;
      // show a summary of what's gonna happen
      chng = new RGChangesWindow(me);
      if (!chng->showAndConfirm(me->_lister, toKeep, toInstall,
      			  toUpgrade, toRemove, toDowngrade)) {
          // canceled operation
          me->_lister->restoreState(state);
	  changed=false;
      }
      delete chng;
  }

  if(changed) {
      // standard header in case the installing fails
      string failedReason(_("Some packages could not be installed.\n\n"
			    "The following packages have unmet "
			    "dependencies:\n"));
      me->_lister->saveUndoState(state);
      // check for failed installs
      if(action == PKG_INSTALL) {
	  //cout << "action install" << endl;
	  bool failed = false;
	  for(unsigned int i=0;i < instPkgs.size();i++) {
	      //cout << "checking pkg nr " << i << endl;
	      pkg = instPkgs[i];
	      if (pkg == NULL)
		  continue;
	      int mstatus = pkg->getMarkedStatus();
	      //cout << "mstatus: " << mstatus << endl;
	      if(!(mstatus == RPackage::MInstall ||
		   mstatus == RPackage::MUpgrade ||
		   mstatus == RPackage::MDowngrade) ) {
		  failed = true;
		  failedReason += string(pkg->name()) + ":\n";
		  failedReason += pkg->showWhyInstBroken();
		  failedReason += "\n";
		  pkg->unsetVersion();
		  me->_lister->notifyChange(pkg);
	      }
	  }
	  if(failed) {
	      // TODO: make this a special dialog with TextView
	      me->_userDialog->warning(utf8(failedReason.c_str()));
	  }
      }
  }

  if (ask) {
      me->_lister->registerObserver(me);
  }
  me->refreshTable(pkg);
  
  // free the list
  g_list_foreach(list, (void (*)(void*,void*))gtk_tree_path_free, NULL);
  g_list_free (list);

  me->_blockActions = FALSE;
  me->setInterfaceLocked(FALSE);
  me->updatePackageInfo(pkg);
}


void RGMainWindow::setColors(bool useColors)
{
    if(useColors) {
	/* we use colors */
	gtk_get_color_from_string(_config->Find("Synaptic::MKeepColor", "").c_str(),
		      &StatusColors[(int)RPackage::MKeep]);
	gtk_get_color_from_string(_config->Find("Synaptic::MInstallColor", 
				    "#ccffcc").c_str(),
		      &StatusColors[(int)RPackage::MInstall]); 
	gtk_get_color_from_string(_config->Find("Synaptic::MUpgradeColor", 
				    "#ffff00").c_str(),
		      &StatusColors[(int)RPackage::MUpgrade]);
	gtk_get_color_from_string(_config->Find("Synaptic::MDowngradeColor", "").c_str(),
		      &StatusColors[(int)RPackage::MDowngrade]);
	gtk_get_color_from_string(_config->Find("Synaptic::MRemoveColor", 
				    "#f44e80").c_str(),
		      &StatusColors[(int)RPackage::MRemove]);
	gtk_get_color_from_string(_config->Find("Synaptic::MHeldColor", "").c_str(),
		      &StatusColors[(int)RPackage::MHeld]);
	gtk_get_color_from_string(_config->Find("Synaptic::MBrokenColor", 
				    "#e00000").c_str(),
		      &StatusColors[(int)RPackage::MBroken/*broken*/]);
	gtk_get_color_from_string(_config->Find("Synaptic::MPinColor", 
				    "#ccccff").c_str(),
		      &StatusColors[(int)RPackage::MPinned/*hold=pinned*/]);
	gtk_get_color_from_string(_config->Find("Synaptic::MNewColor", 
				    "#ffffaa").c_str(),
		      &StatusColors[(int)RPackage::MNew/*new*/]);
    } else {
	/* no colors */
	for(int i=0;i<(int)RPackage::MNew;i++)
	    StatusColors[i] = NULL;
    }
}


RGMainWindow::RGMainWindow(RPackageLister *packLister, string name)
    : RGGladeWindow(NULL, name), _lister(packLister),  _activeTreeModel(0),
      _treeView(0)
{
    assert(_win);

    _blockActions = false;
    _unsavedChanges = false;
    _interfaceLocked = 0;

    _lister->registerObserver(this);
    _busyCursor = gdk_cursor_new(GDK_WATCH);
    _tooltips = gtk_tooltips_new();

    _toolbarState = (ToolbarState)_config->FindI("Synaptic::ToolbarState",
						(int)TOOLBAR_BOTH);

    // get the pixbufs
    StatusPixbuf[(int)RPackage::MKeep] = 	
      gdk_pixbuf_new_from_xpm_data(keepM_xpm);
  
    StatusPixbuf[(int)RPackage::MInstall] = 
      gdk_pixbuf_new_from_xpm_data(installM_xpm);
  
    StatusPixbuf[(int)RPackage::MUpgrade] =
      gdk_pixbuf_new_from_xpm_data(upgradeM_xpm);
  
    StatusPixbuf[(int)RPackage::MDowngrade] =
      gdk_pixbuf_new_from_xpm_data(downgradeM_xpm);
  
    StatusPixbuf[(int)RPackage::MRemove] =
      gdk_pixbuf_new_from_xpm_data(removeM_xpm);
  
    StatusPixbuf[(int)RPackage::MHeld] =
      gdk_pixbuf_new_from_xpm_data(heldM_xpm);
  
    StatusPixbuf[(int)RPackage::MBroken] =
      gdk_pixbuf_new_from_xpm_data(brokenM_xpm);
  
    StatusPixbuf[(int)RPackage::MPinned] =
      gdk_pixbuf_new_from_xpm_data(pinM_xpm);
  
    StatusPixbuf[(int)RPackage::MNew] =
      gdk_pixbuf_new_from_xpm_data(newM_xpm);
  
    StatusPixbuf[10] =
      gdk_pixbuf_new_from_xpm_data(alert_xpm);


    // get some color values
    setColors(_config->FindB("Synaptic::UseStatusColors", TRUE));

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
    
    _blackStyle = gtk_widget_get_style(_win);
    _redStyle = gtk_style_copy(_blackStyle);
    GdkColor color;
    color.red = 0xaa00;
    color.green = 0x2000;
    color.blue = 0x2000;
    gdk_colormap_alloc_color(gtk_widget_get_colormap(_win), &color, FALSE, TRUE);
    _redStyle->fg[0] = color;

    // create preferences window and apply the proxy settings
    RGPreferencesWindow::applyProxySettings();
}


// this code does heavy depend on the button layout that libglade uses
void RGMainWindow::menuToolbarClicked(GtkWidget *self, void *data)
{
  RGMainWindow *me = (RGMainWindow*)g_object_get_data(G_OBJECT(self), "me");
  GtkWidget *widget;
  GList *child;
  // the buttons we want to show or hide
  char* buttons[] = {"button_update",
		     "button_upgrade",
		     "button_dist_upgrade",
		     "button_procceed"
  };

  // save new toolbar state
  me->_toolbarState = (ToolbarState)GPOINTER_TO_INT(data);

  if(me->_toolbarState == TOOLBAR_HIDE) {
    widget = glade_xml_get_widget(me->_gladeXML, "handlebox_button_toolbar");
    gtk_widget_hide(widget);
  } else {
    widget = glade_xml_get_widget(me->_gladeXML, "handlebox_button_toolbar");
    gtk_widget_show(widget);
  }

  for(int i=0;i<4;i++) {
    widget = glade_xml_get_widget(me->_gladeXML, buttons[i]);
    //first get GtkVBox of button
    child = gtk_container_get_children(GTK_CONTAINER(widget));
    
    // now we get the real children (first GtkImage)
    child = gtk_container_get_children(GTK_CONTAINER(child->data));
    if(me->_toolbarState == TOOLBAR_TEXT)
      gtk_widget_hide(GTK_WIDGET(child->data));
    else
      gtk_widget_show(GTK_WIDGET(child->data));
    // then GtkLabel
    child = g_list_next(child);
    if(me->_toolbarState == TOOLBAR_PIXMAPS)
      gtk_widget_hide(GTK_WIDGET(child->data));
    else
      gtk_widget_show(GTK_WIDGET(child->data));
    
  }
}

void RGMainWindow::findToolClicked(GtkWidget *self, void *data)
{
  RGMainWindow *me = (RGMainWindow*)data;
  
  if(me->_findWin == NULL) {
    me->_findWin = new RGFindWindow(me);
  }

  int res = gtk_dialog_run(GTK_DIALOG(me->_findWin->window()));
  if(res == 0) {
      gdk_window_set_cursor(me->_findWin->window()->window, me->_busyCursor);
      RFilter *filter = me->_lister->findFilter(0);
      filter->reset();
      int searchType = me->_findWin->getSearchType();
      string S(me->_findWin->getFindString());

      if(searchType & 1<<RPatternPackageFilter::Name) {
	  RPatternPackageFilter::DepType type = RPatternPackageFilter::Name;
	  filter->pattern.addPattern(type, S, false);
      }
      if(searchType & 1<<RPatternPackageFilter::Version) {
	  RPatternPackageFilter::DepType type = RPatternPackageFilter::Version;
	  filter->pattern.addPattern(type, S, false);
      }
      if(searchType & 1<<RPatternPackageFilter::Description) {
	  RPatternPackageFilter::DepType type = RPatternPackageFilter::Description;
	  filter->pattern.addPattern(type, S, false);
      }
      if(searchType & 1<<RPatternPackageFilter::Maintainer) {
	  RPatternPackageFilter::DepType type = RPatternPackageFilter::Maintainer;
	  filter->pattern.addPattern(type, S, false);
      }
      if(searchType & 1<<RPatternPackageFilter::Depends) {
	  RPatternPackageFilter::DepType type = RPatternPackageFilter::Depends;
	  filter->pattern.addPattern(type, S, false);
      }
      if(searchType & 1<<RPatternPackageFilter::Provides) {
	  RPatternPackageFilter::DepType type = RPatternPackageFilter::Provides;
	  filter->pattern.addPattern(type, S, false);
      }

      me->changeFilter(1);
      gdk_window_set_cursor(me->_findWin->window()->window, NULL);
      me->_findWin->hide();
  }

}

void RGMainWindow::undoClicked(GtkWidget *self, void *data)
{
    //cout << "undoClicked" << endl;
    RGMainWindow *me = (RGMainWindow*)data;
    me->setInterfaceLocked(TRUE); 

    me->_lister->unregisterObserver(me);

    // undo
    me->_lister->undo();

    me->_lister->registerObserver(me);
    me->refreshTable();
    me->setInterfaceLocked(FALSE); 
}


void RGMainWindow::doOpenSelections(GtkWidget *file_selector, 
				    gpointer data) 
{
    //cout << "void RGMainWindow::doOpenSelections()" << endl;
    RGMainWindow *me = (RGMainWindow*)g_object_get_data(G_OBJECT(data), "me");
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


void RGMainWindow::openClicked(GtkWidget *self, void *data)
{
    //std::cout << "RGMainWindow::openClicked()" << endl;
    //RGMainWindow *me = (RGMainWindow*)data;
    
    GtkWidget *filesel;
    filesel = gtk_file_selection_new (_("Open changes"));
    g_object_set_data(G_OBJECT(filesel), "me", data);

    g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
                     "clicked", G_CALLBACK (doOpenSelections), filesel);
   			   
    g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
                             "clicked",
			     G_CALLBACK (gtk_widget_destroy), 
                             (gpointer)filesel); 

    g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
                             "clicked",
                             G_CALLBACK (gtk_widget_destroy),
                             (gpointer)filesel); 
   
   gtk_widget_show (filesel);
}

void RGMainWindow::saveClicked(GtkWidget *self, void *data)
{
    //std::cout << "RGMainWindow::saveClicked()" << endl;
    RGMainWindow *me = (RGMainWindow*)data;

    if(me->selectionsFilename == "") {
	me->saveAsClicked(self,data);
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

void RGMainWindow::rowExpanded(GtkTreeView *treeview,  GtkTreeIter *arg1,
		 GtkTreePath *arg2, gpointer data)
{
  RGMainWindow *me = (RGMainWindow *)data;

  me->setInterfaceLocked(TRUE); 
  
  me->setStatusText("Expanding row..");

  while(gtk_events_pending())
    gtk_main_iteration();

  me->setStatusText();

  me->setInterfaceLocked(FALSE); 
}

void RGMainWindow::doSaveSelections(GtkWidget *file_selector_button, 
				    gpointer data) 
{
    GtkWidget *checkButton;
    const gchar *file;

    //cout << "void RGMainWindow::doSaveSelections()" << endl;
    RGMainWindow *me = (RGMainWindow*)g_object_get_data(G_OBJECT(data), "me");

    file = gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));
    //cout << "selected file: " << file << endl;
    me->selectionsFilename = file;
    
    // do we want the full state?
    checkButton = (GtkWidget*)g_object_get_data(G_OBJECT(data), "checkButton");
    me->saveFullState = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton));
    //cout << "fullState: " << me->saveFullState << endl;

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


void RGMainWindow::saveAsClicked(GtkWidget *self, void *data)
{
    //std::cout << "RGMainWindow::saveAsClicked()" << endl;
    //RGMainWindow *me = (RGMainWindow*)data;
    
    GtkWidget *filesel;
    filesel = gtk_file_selection_new (_("Save changes"));
    g_object_set_data(G_OBJECT(filesel), "me", data);
    
    g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
                     "clicked", G_CALLBACK (doSaveSelections), filesel);
    
    g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
                             "clicked",
			     G_CALLBACK (gtk_widget_destroy), 
                             (gpointer)filesel); 

    g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
                             "clicked",
                             G_CALLBACK (gtk_widget_destroy),
                             (gpointer)filesel); 
   
    GtkWidget *checkButton = gtk_check_button_new_with_label(_("Save full state, not only changes"));
    gtk_box_pack_start_defaults(GTK_BOX(GTK_FILE_SELECTION(filesel)->main_vbox), 
				checkButton);
    g_object_set_data(G_OBJECT(filesel), "checkButton", checkButton);
    gtk_widget_show(checkButton);
    gtk_widget_show (filesel);
}

void RGMainWindow::redoClicked(GtkWidget *self, void *data)
{
    //cout << "redoClicked" << endl;
    RGMainWindow *me = (RGMainWindow*)data;
    me->setInterfaceLocked(TRUE); 

    me->_lister->unregisterObserver(me);

    // redo
    me->_lister->redo();

    me->_lister->registerObserver(me);
    me->refreshTable();
    me->setInterfaceLocked(FALSE); 
}

// needed for the buildTreeView function
struct mysort {
    bool operator()(const pair<int,GtkTreeViewColumn *> &x, 
		    const pair<int,GtkTreeViewColumn *> &y) {
	return x.first < y.first;
    }    
};


void RGMainWindow::buildTreeView()
{
    GtkCellRenderer *renderer; 
    GtkTreeViewColumn *column, *name_column=NULL; 
    GtkTreeSelection *selection;
    vector<pair<int,GtkTreeViewColumn *> > all_columns;
    int pos=0;

// remove old tree columns
    if(_treeView) {
	GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(_treeView));
	for(GList *li=g_list_first(columns);li != NULL; li=g_list_next(li)) {
	    gtk_tree_view_remove_column(GTK_TREE_VIEW(_treeView), 
					GTK_TREE_VIEW_COLUMN(li->data));
	}
	// need to free the list here
	g_list_free(columns);
    }



    _treeView = glade_xml_get_widget(_gladeXML, "treeview_packages");
    assert(_treeView);

    int mode = _config->FindI("Synaptic::TreeDisplayMode", 0);
    _treeDisplayMode = (RPackageLister::treeDisplayMode)mode;
    _menuDisplayMode = _treeDisplayMode;

    gtk_tree_view_set_search_column (GTK_TREE_VIEW(_treeView), NAME_COLUMN);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (_treeView));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

    /* Status(pixmap) column */
    pos = _config->FindI("Synaptic::statusColumnPos",0);
    if(pos != -1) {
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new_with_attributes("S", renderer,
							  "pixbuf", PIXMAP_COLUMN,
							  NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(column, 20);
	//gtk_tree_view_insert_column(GTK_TREE_VIEW(_treeView), column, pos);
	all_columns.push_back(pair<int,GtkTreeViewColumn *>(pos,column));
    }

    
    /* Package name */
    pos = _config->FindI("Synaptic::nameColumnPos",1);
    if(pos != -1) {
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
	name_column = column = 
	    gtk_tree_view_column_new_with_attributes(_("Package"), renderer,
						     "text", NAME_COLUMN,
						     "background-gdk", COLOR_COLUMN,
						     NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	//gtk_tree_view_column_set_fixed_width(column, 200);

	//gtk_tree_view_insert_column(GTK_TREE_VIEW(_treeView), column, pos);
	all_columns.push_back(pair<int,GtkTreeViewColumn *>(pos,column));
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, NAME_COLUMN);
    }

    /* Installed Version */
    pos = _config->FindI("Synaptic::instVerColumnPos",2);
    if(pos != -1) {
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
	column = gtk_tree_view_column_new_with_attributes(_("Installed Version"), renderer,
							  "text", INSTALLED_VERSION_COLUMN,
							  "background-gdk", COLOR_COLUMN,
							  NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(column, 130);
	//gtk_tree_view_insert_column (GTK_TREE_VIEW(_treeView), column, pos);
	all_columns.push_back(pair<int,GtkTreeViewColumn *>(pos,column));
	gtk_tree_view_column_set_resizable(column, TRUE);
    }

    /* Available Version */
    pos = _config->FindI("Synaptic::availVerColumnPos",3);
    if(pos != -1) {
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
	column = gtk_tree_view_column_new_with_attributes(_("Available Version"), renderer,
							  
							  "text", AVAILABLE_VERSION_COLUMN,
							  "background-gdk", COLOR_COLUMN,
							  NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(column, 130);
	//gtk_tree_view_insert_column (GTK_TREE_VIEW (_treeView), column, pos);
	all_columns.push_back(pair<int,GtkTreeViewColumn *>(pos,column));
	gtk_tree_view_column_set_resizable(column, TRUE);
    }

    // installed size
    pos = _config->FindI("Synaptic::instSizeColumnPos",4);
    if(pos != -1) {
	/* Installed size */
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
	GValue value = {0,};
	GValue value2 = {0,};
	g_value_init (&value, G_TYPE_FLOAT);
	g_value_set_float(&value, 1.0);
	g_object_set_property(G_OBJECT(renderer), "xalign", &value);
	g_value_init(&value2, G_TYPE_INT);
	g_value_set_int(&value2, 10);
	g_object_set_property(G_OBJECT(renderer),"xpad", &value2);
	column = gtk_tree_view_column_new_with_attributes(_("Inst. Size"), renderer,
							  "text", PKG_SIZE_COLUMN,
							  "background-gdk", COLOR_COLUMN,
							  NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(column, 80);
	//gtk_tree_view_insert_column (GTK_TREE_VIEW(_treeView), column, pos);
	all_columns.push_back(pair<int,GtkTreeViewColumn *>(pos,column));
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, PKG_SIZE_COLUMN);
    }

    /* Description */
    pos = _config->FindI("Synaptic::descrColumnPos",5);
    if(pos != -1) {
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
	column = gtk_tree_view_column_new_with_attributes(_("Description"), renderer,
							  "text", DESCR_COLUMN,
							  "background-gdk", COLOR_COLUMN,
							  NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(column, 500);
	//gtk_tree_view_insert_column (GTK_TREE_VIEW (_treeView), column, pos);
	all_columns.push_back(pair<int,GtkTreeViewColumn *>(pos,column));
	gtk_tree_view_column_set_resizable(column, TRUE);
    }

    // now sort and insert in order
    sort(all_columns.begin(),all_columns.end(),mysort());
    for(unsigned int i=0;i<all_columns.size();i++) {
	gtk_tree_view_append_column(GTK_TREE_VIEW (_treeView),
				    GTK_TREE_VIEW_COLUMN(all_columns[i].second));
    }
    // now set name column to expander column
    if(name_column)
	gtk_tree_view_set_expander_column(GTK_TREE_VIEW(_treeView),  name_column);

#if GTK_CHECK_VERSION(2,3,0)
    //#warning build with new fixed_height_mode
    GValue value = {0,};
    g_value_init (&value, G_TYPE_BOOLEAN);
    g_value_set_boolean(&value, TRUE);
    g_object_set_property(G_OBJECT(_treeView), "fixed_height_mode", &value);
#endif


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
    
    
    GdkPixbuf *icon = gdk_pixbuf_new_from_xpm_data(synaptic_mini_xpm);
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
				  G_CALLBACK(showAboutPanel),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_help_activate",
				  G_CALLBACK(helpAction),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_update_packages",
				  G_CALLBACK(updateClicked),
				  this); 

    _upgradeB = glade_xml_get_widget(_gladeXML, "button_upgrade");
    _upgradeM = glade_xml_get_widget(_gladeXML, "upgrade1");
    glade_xml_signal_connect_data(_gladeXML,
				  "on_upgrade_packages",
				  G_CALLBACK(upgradeClicked),
				  this); 

    _distUpgradeB = glade_xml_get_widget(_gladeXML, "button_dist_upgrade");
    _distUpgradeM = glade_xml_get_widget(_gladeXML, "distribution_upgrade1");
    glade_xml_signal_connect_data(_gladeXML,
				  "on_dist_upgrade_packages",
				  G_CALLBACK(distUpgradeClicked),
				  this); 

    if (_config->FindB("Synaptic::NoUpgradeButtons", false) == true) {
	gtk_widget_hide(_upgradeB);
	gtk_widget_hide(_distUpgradeB);
	widget = glade_xml_get_widget(_gladeXML, "alignment_upgrade");
	gtk_widget_hide(widget);
    }

    _proceedB = glade_xml_get_widget(_gladeXML, "button_procceed");
    _proceedM = glade_xml_get_widget(_gladeXML, "menu_proceed");
    glade_xml_signal_connect_data(_gladeXML,
				  "on_proceed_clicked",
				  G_CALLBACK(proceedClicked),
				  this); 

    _fixBrokenM = glade_xml_get_widget(_gladeXML, "fix_broken_packages");
    glade_xml_signal_connect_data(_gladeXML,
				  "on_fix_broken_packages",
				  G_CALLBACK(fixBrokenClicked),
				  this); 
    
    glade_xml_signal_connect_data(_gladeXML,
				  "on_preferences_activate",
				  G_CALLBACK(showConfigWindow),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_set_option_activate",
				  G_CALLBACK(showSetOptWindow),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_repositories_activate",
				  G_CALLBACK(showSourcesWindow),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_exit_activate",
				  G_CALLBACK(closeWin),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_edit_filter_activate",
				  G_CALLBACK(showFilterManagerWindow),
				  this); 
    
    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_pkghelp_clicked",
				  G_CALLBACK(pkgHelpClicked),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_pkgreconfigure_clicked",
				  G_CALLBACK(pkgReconfigureClicked),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_search_name",
				  G_CALLBACK(findToolClicked),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_undo1_activate",
				  G_CALLBACK(undoClicked),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_redo1_activate",
				  G_CALLBACK(redoClicked),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_open_activate",
				  G_CALLBACK(openClicked),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_save_activate",
				  G_CALLBACK(saveClicked),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_save_as_activate",
				  G_CALLBACK(saveAsClicked),
				  this); 


    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_rec_install_clicked",
				  G_CALLBACK(clickedRecInstall),
				  GINT_TO_POINTER(InstallRecommended)); 
    g_object_set_data(G_OBJECT(glade_xml_get_widget(_gladeXML, 
						    "button_rec_install")),
		      "me", this);

    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_suc_install_clicked",
				  G_CALLBACK(clickedRecInstall),
				  GINT_TO_POINTER(InstallSuggested)); 
    g_object_set_data(G_OBJECT(glade_xml_get_widget(_gladeXML, 
						    "button_suc_install")),
		      "me", this);


    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_sel_install_clicked",
				  G_CALLBACK(clickedRecInstall),
				  GINT_TO_POINTER(InstallSelected)); 
    g_object_set_data(G_OBJECT(glade_xml_get_widget(_gladeXML, 
						    "button_sel_install")),
		      "me", this);

    _keepM = glade_xml_get_widget(_gladeXML,"menu_keep");
    assert(_keepM);
    _installM = glade_xml_get_widget(_gladeXML,"menu_install");
    assert(_installM);
    _pkgupgradeM = glade_xml_get_widget(_gladeXML,"menu_upgrade");
    assert(_upgradeM);
    _removeM = glade_xml_get_widget(_gladeXML,"menu_remove");
    assert(_removeM);
    _remove_w_depsM = glade_xml_get_widget(_gladeXML,"menu_remove_with_deps");
    assert(_remove_w_depsM);
    _purgeM = glade_xml_get_widget(_gladeXML,"menu_purge");
    assert(_purgeM);
#ifdef HAVE_RPM
    gtk_widget_hide(_purgeM);
#endif

    // workaround for a bug in libglade
    button = glade_xml_get_widget(_gladeXML, "button_update");
    gtk_tooltips_set_tip(GTK_TOOLTIPS (_tooltips), button,
			 _("Update the list of available packages"),"");
			 
    button = glade_xml_get_widget(_gladeXML, "button_upgrade");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(_tooltips), button,
			 _("Upgrade every installed package to the latest version"),"");
    
    button = glade_xml_get_widget(_gladeXML, "button_dist_upgrade");
    gtk_tooltips_set_tip(GTK_TOOLTIPS (_tooltips), button,
			 _("Upgrade every installed package to the latest version. Also include upgrades depending on not yet installed packages"),"");
    
    button = glade_xml_get_widget(_gladeXML, "button_procceed");
    gtk_tooltips_set_tip(GTK_TOOLTIPS (_tooltips), button,
			 _("Execute the selected changes"),"");

    
    _nameL = glade_xml_get_widget(_gladeXML, "label_pkgname"); 
    assert(_nameL);
    _summL = glade_xml_get_widget(_gladeXML, "label_summary"); 
    assert(_summL);

    _actionB[0] = glade_xml_get_widget(_gladeXML, "radiobutton_keep");
    glade_xml_signal_connect_data(_gladeXML,
				  "on_action_clicked",
				  G_CALLBACK(actionClicked),
				  this);
    widget = glade_xml_get_widget(_gladeXML, "menu_keep");
    assert(widget);
    g_object_set_data(G_OBJECT(widget), "me", this);
    glade_xml_signal_connect_data(_gladeXML,
				  "on_menu_action_keep",
				  G_CALLBACK(menuActionClicked),
				  GINT_TO_POINTER(PKG_KEEP));


    _actionB[1] = glade_xml_get_widget(_gladeXML, "radiobutton_install");
    glade_xml_signal_connect_data(_gladeXML,
				  "on_action_clicked",
				  G_CALLBACK(actionClicked),
				  this);
    _actionBInstallLabel = glade_xml_get_widget(_gladeXML,
						"radiobutton_install_label");
    widget = glade_xml_get_widget(_gladeXML, "menu_install");
    assert(widget);
    g_object_set_data(G_OBJECT(widget), "me", this);
    glade_xml_signal_connect_data(_gladeXML,
				  "on_menu_action_install",
				  G_CALLBACK(menuActionClicked),
				  GINT_TO_POINTER(PKG_INSTALL));
    // callback same as for install
    widget = glade_xml_get_widget(_gladeXML, "menu_upgrade");
    assert(widget);
    g_object_set_data(G_OBJECT(widget), "me", this);

    
    _actionB[2] = glade_xml_get_widget(_gladeXML, "radiobutton_delete");
    assert(widget);
    g_object_set_data(G_OBJECT(_actionB[2]), "me", this);
    glade_xml_signal_connect_data(_gladeXML,
				  "on_action_clicked_delete",
				  G_CALLBACK(actionClicked),
				  this);

    widget = glade_xml_get_widget(_gladeXML, "menu_remove");
    assert(widget);
    g_object_set_data(G_OBJECT(widget), "me", this);
    glade_xml_signal_connect_data(_gladeXML,
				  "on_menu_action_delete",
				  G_CALLBACK(menuActionClicked),
				  GINT_TO_POINTER(PKG_DELETE));

    
    widget = glade_xml_get_widget(_gladeXML, "menu_remove_with_deps");
    assert(widget);
    g_object_set_data(G_OBJECT(widget), "me", this);
    glade_xml_signal_connect_data(_gladeXML,
				  "on_menu_action_delete_with_deps",
				  G_CALLBACK(menuActionClicked),
				  GINT_TO_POINTER(PKG_DELETE_WITH_DEPS));


    widget = glade_xml_get_widget(_gladeXML, "menu_purge");
    assert(widget);
    g_object_set_data(G_OBJECT(widget), "me", this);
    glade_xml_signal_connect_data(_gladeXML,
				  "on_menu_action_purge",
				  G_CALLBACK(menuActionClicked),
				  GINT_TO_POINTER(PKG_PURGE));


    _pinB = glade_xml_get_widget(_gladeXML, "checkbutton_pin");
    glade_xml_signal_connect_data(_gladeXML,
				  "on_pin_clicked",
				  G_CALLBACK(pinClicked),
				  this);

    _pinM = glade_xml_get_widget(_gladeXML, "menu_hold");
    glade_xml_signal_connect_data(_gladeXML,
				  "on_menu_pin",
				  G_CALLBACK(menuPinClicked),
				  this);
    // only if pkg help is enabled
#ifndef SYNAPTIC_PKG_HOLD
    gtk_widget_hide(_pinB);
    gtk_widget_hide(_pinM);
    widget = glade_xml_get_widget(_gladeXML, "hseparator_hold");
    if(widget != NULL)
	gtk_widget_hide(widget);
#endif

    // only for debian 
    _pkgHelp = glade_xml_get_widget(_gladeXML, "button_pkghelp");
    assert(_pkgHelp);
    _pkgReconfigure = glade_xml_get_widget(_gladeXML, "button_pkgreconfigure");
    assert(_pkgHelp);

    if(!FileExists("/usr/bin/dwww")) 
	gtk_widget_hide(_pkgHelp);
    if(!FileExists("/usr/sbin/dpkg-reconfigure"))
	gtk_widget_hide(_pkgReconfigure);
#ifdef HAVE_RPM
    gtk_widget_hide(glade_xml_get_widget(_gladeXML, "hseparator_hold"));
#endif    

    _pkginfo = glade_xml_get_widget(_gladeXML, "box_pkginfo");
    assert(_pkginfo);

    _vpaned =  glade_xml_get_widget(_gladeXML, "vpaned_main");
    assert(_vpaned);
    // If the pane position is restored before the window is shown, it's
    // not restored in the same place as it was.
    show(); RGFlushInterface();
    gtk_paned_set_position(GTK_PANED(_vpaned), 
 			   _config->FindI("Synaptic::vpanedPos", 140));
    

    _stateP = GTK_IMAGE(glade_xml_get_widget(_gladeXML, "image_state"));
    assert(_stateP);
    _stateL = glade_xml_get_widget(_gladeXML, "label_state");
    assert(_stateL);
    gtk_misc_set_alignment(GTK_MISC(_stateL), 0.0, 0.0);
    _infoL =  glade_xml_get_widget(_gladeXML, "label_info");
    assert(_infoL);
    gtk_misc_set_alignment(GTK_MISC(_infoL), 0.0, 0.0);
    gtk_label_set_justify(GTK_LABEL(_infoL), GTK_JUSTIFY_LEFT);

    _descrView = glade_xml_get_widget(_gladeXML, "text_descr");
    assert(_descrView);
    _descrBuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (_descrView));
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_descrView), GTK_WRAP_WORD);

    _filesView = glade_xml_get_widget(_gladeXML, "textview_files");
    assert(_filesView);
    _filesBuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (_filesView));
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_filesView), GTK_WRAP_WORD);


    _depP = glade_xml_get_widget(_gladeXML, "optionmenu_depends");
    assert(_depP);
    item = glade_xml_get_widget(_gladeXML, "menu_what_it_depends");
    assert(item);
    gtk_object_set_data(GTK_OBJECT(item), "index", (void*)WHAT_IT_DEPENDS_ON);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
		       (GtkSignalFunc)changedDepView, this);

    item = glade_xml_get_widget(_gladeXML, "menu_what_depends_on_it");
    assert(item);
    gtk_object_set_data(GTK_OBJECT(item), "index", (void*)WHAT_DEPENDS_ON_IT);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
		       (GtkSignalFunc)changedDepView, this);

    item = glade_xml_get_widget(_gladeXML, "menu_what_it_would_depend_on");
    assert(item);
    gtk_object_set_data(GTK_OBJECT(item), "index",
			(void*)WHAT_IT_WOULD_DEPEND_ON);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
		       (GtkSignalFunc)changedDepView, this);

    item = glade_xml_get_widget(_gladeXML, "menu_provides");
    assert(item);
    gtk_object_set_data(GTK_OBJECT(item), "index",
			(void*)WHAT_IT_PROVIDES);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
		       (GtkSignalFunc)changedDepView, this);
    
    // recommends and suggests are only available on debian,
    // so we activate the signal there and hide it on rpm systems
    item = glade_xml_get_widget(_gladeXML, "menu_suggested");
    assert(item);
#ifndef HAVE_RPM
    gtk_object_set_data(GTK_OBJECT(item), "index", 
			(void*)WHAT_IT_SUGGESTS);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
		       (GtkSignalFunc)changedDepView, this);
#else
    gtk_widget_hide(item);
#endif
    gtk_option_menu_set_history(GTK_OPTION_MENU(_depP), 0);

    _depTab = glade_xml_get_widget(_gladeXML, "notebook_dep_tab");
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(_depTab), FALSE);
    assert(_depTab);
    
    /* create the dependency list */
    _depList = glade_xml_get_widget(_gladeXML, "treeview_deplist");
    assert(_depList);
    _depListStore = gtk_list_store_new(4,G_TYPE_STRING, /* text */
				       G_TYPE_BOOLEAN,  /* foreground-set */
				       G_TYPE_STRING,   /* foreground */
				       G_TYPE_STRING);  /* extra info */
    gtk_tree_view_set_model(GTK_TREE_VIEW(_depList), GTK_TREE_MODEL(_depListStore));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Packages", renderer,
						      "text", DEP_NAME_COLUMN,
						      "foreground-set", DEP_IS_NOT_AVAILABLE,
						      "foreground", DEP_IS_NOT_AVAILABLE_COLOR,
						      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_depList), column);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (_depList));
    g_signal_connect (G_OBJECT (selection), "changed",
		      G_CALLBACK(clickedDepList),
		      this);
    _depInfoL = glade_xml_get_widget(_gladeXML, "label_dep_info");
    assert(_depInfoL);
    gtk_label_set_text(GTK_LABEL(_depInfoL), "");    

    /* build rdep list */
    _rdepList = glade_xml_get_widget(_gladeXML, "treeview_rdeps");
    assert(_rdepList);
    _rdepListStore = gtk_list_store_new(1,G_TYPE_STRING); /* text */
    gtk_tree_view_set_model(GTK_TREE_VIEW(_rdepList), GTK_TREE_MODEL(_rdepListStore));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Packages", renderer,
						      "text", DEP_NAME_COLUMN,
						      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_rdepList), column);



    _availDepList = glade_xml_get_widget(_gladeXML, "treeview_availdep_list");
    assert(_availDepList);
    _availDepListStore = gtk_list_store_new(4,G_TYPE_STRING, /* text */
					    G_TYPE_BOOLEAN,  /*foreground-set*/
					    G_TYPE_STRING,   /* foreground */
					    G_TYPE_STRING);  /* extra info */
    gtk_tree_view_set_model(GTK_TREE_VIEW(_availDepList), GTK_TREE_MODEL(_availDepListStore));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Packages", renderer,
						      "text", DEP_NAME_COLUMN,
						      "foreground-set", DEP_IS_NOT_AVAILABLE,
						      "foreground", DEP_IS_NOT_AVAILABLE_COLOR,
						      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_availDepList), column);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (_availDepList));
    g_signal_connect (G_OBJECT (selection), "changed",
		      G_CALLBACK(clickedAvailDepList),
		      this);

    _availDepInfoL = glade_xml_get_widget(_gladeXML, "label_availdep_info");
    assert(_availDepInfoL);



    /* build provides list */
    _providesList = glade_xml_get_widget(_gladeXML, "treeview_provides_list");
    assert(_availDepList);
    _providesListStore = gtk_list_store_new(1,G_TYPE_STRING, /* text */
					    -1); 
    gtk_tree_view_set_model(GTK_TREE_VIEW(_providesList), 
			    GTK_TREE_MODEL(_providesListStore));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Packages", renderer,
						      "text", DEP_NAME_COLUMN,
						      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_providesList), column);
    
    /* create the recommended/suggested list */
    _recList = glade_xml_get_widget(_gladeXML, "treeview_rec_list");
    assert(_recList);
    _recListStore = gtk_list_store_new(3,G_TYPE_STRING, /* text */
				       G_TYPE_BOOLEAN,  /* foreground-set */
				       G_TYPE_STRING);  /* foreground */
    gtk_tree_view_set_model(GTK_TREE_VIEW(_recList), GTK_TREE_MODEL(_recListStore));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Packages", renderer,
						      "text", DEP_NAME_COLUMN,
						      "foreground-set", DEP_IS_NOT_AVAILABLE,
						      "foreground", DEP_IS_NOT_AVAILABLE_COLOR,
						      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_recList), column);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_recList));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

    _filtersB = button = glade_xml_get_widget(_gladeXML, "button_filters");
    assert(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       (GtkSignalFunc)showFilterManagerWindow, this);

    _filterPopup = glade_xml_get_widget(_gladeXML, "optionmenu_filters");
    assert(_filterPopup);
    _filterMenu = glade_xml_get_widget(_gladeXML, "menu_apply");
    assert(_filterMenu);

#if 0
    _editFilterB = button = glade_xml_get_widget(_gladeXML, "button_edit_filter");
    assert(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       (GtkSignalFunc)showFilterWindow, this);
#endif

    _findText = glade_xml_get_widget(_gladeXML, "entry_find");
    g_object_set_data(G_OBJECT(_findText), "me", this);
    assert(_findText);
    gtk_object_set_data(GTK_OBJECT(_findText), "me", this);
    gtk_signal_connect(GTK_OBJECT(_findText), "changed",
		       (GtkSignalFunc)searchLackAction, this);

    _findSearchB = glade_xml_get_widget(_gladeXML, "button_search_next");
    assert(_findSearchB);
    gtk_signal_connect(GTK_OBJECT(_findSearchB), "clicked",
		       G_CALLBACK(searchNextAction),
		       this);

    _findSearchB = glade_xml_get_widget(_gladeXML, "button_search_begin");
    assert(_findSearchB);
    gtk_signal_connect(GTK_OBJECT(_findSearchB), "clicked",
		       G_CALLBACK(searchBeginAction),
		       this);

    // build the treeview
    buildTreeView();

    // connect the treeview signals
    g_signal_connect(G_OBJECT(_treeView), "row-expanded",
		     G_CALLBACK(rowExpanded), this);

    
    GtkTreeSelection *select;
    select = gtk_tree_view_get_selection (GTK_TREE_VIEW (_treeView));
    //gtk_tree_selection_set_mode (select, GTK_SELECTION_MULTIPLE);
    g_signal_connect (G_OBJECT (select), "changed",
		      G_CALLBACK(selectedRow),
		      this);
    g_signal_connect (G_OBJECT (_treeView), "row-activated",
		      G_CALLBACK(doubleClickRow),
		      this);

    // treeview signals
    glade_xml_signal_connect_data(_gladeXML,
				  "on_expand_all_activate",
				  G_CALLBACK(onExpandAll),
				  this); 
    glade_xml_signal_connect_data(_gladeXML,
				  "on_collapse_all_activate",
				  G_CALLBACK(onCollapseAll),
				  this); 
    
    int mode = _treeDisplayMode;
    widget = glade_xml_get_widget(_gladeXML, "section_tree");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget),
				   mode == 0 ? TRUE : FALSE);

    widget = glade_xml_get_widget(_gladeXML, "alphabetic_tree");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget),
				   mode == 1 ? TRUE : FALSE);

    widget = glade_xml_get_widget(_gladeXML, "status_tree");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget),
				   mode == 2 ? TRUE : FALSE);

    widget = glade_xml_get_widget(_gladeXML, "flat_list");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget),
				   mode == 3 ? TRUE : FALSE);

    widget = glade_xml_get_widget(_gladeXML, "tag_tree");
#ifdef HAVE_DEBTAGS
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget),
				   mode == 4 ? TRUE : FALSE);
#else
    gtk_widget_hide(widget);
    widget = glade_xml_get_widget(_gladeXML, "separator_tag_tree");
    gtk_widget_hide(widget);
#endif

    glade_xml_signal_connect_data(_gladeXML,
				  "on_section_tree_activate",
				  G_CALLBACK(onSectionTree),
				  this); 
    glade_xml_signal_connect_data(_gladeXML,
				  "on_alphabetic_tree_activate",
				  G_CALLBACK(onAlphabeticTree),
				  this); 
    glade_xml_signal_connect_data(_gladeXML,
				  "on_status_tree_activate",
				  G_CALLBACK(onStatusTree),
				  this); 
    glade_xml_signal_connect_data(_gladeXML,
				  "on_flat_list_activate",
				  G_CALLBACK(onFlatList),
				  this); 
    glade_xml_signal_connect_data(_gladeXML,
				  "on_tag_tree_activate",
				  G_CALLBACK(onTagTree),
				  this); 
    
    glade_xml_signal_connect_data(_gladeXML,
				  "on_add_cdrom_activate",
				  G_CALLBACK(onAddCDROM),
				  this); 
    
    /* --------------------------------------------------------------- */

    // we don't support "installed files" on rpm currently
#ifdef HAVE_RPM 
    widget = glade_xml_get_widget(_gladeXML, "notebook_info");
    assert(widget);
    // page of the "installed files"
    gtk_notebook_remove_page(GTK_NOTEBOOK(widget), 3);
#endif    

    // toolbar menu code
    button = glade_xml_get_widget(_gladeXML, "menu_toolbar_pixmaps");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
    g_object_set_data(G_OBJECT(button), "me", this);
    g_signal_connect(G_OBJECT(button), 
		       "activate",
		       G_CALLBACK(menuToolbarClicked), 
		       GINT_TO_POINTER(TOOLBAR_PIXMAPS)); 
    if(_toolbarState == TOOLBAR_PIXMAPS)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));


    button = glade_xml_get_widget(_gladeXML, "menu_toolbar_text");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
    g_object_set_data(G_OBJECT(button), "me", this);
    g_signal_connect(G_OBJECT(button), 
		       "activate",
		       G_CALLBACK(menuToolbarClicked), 
		       GINT_TO_POINTER(TOOLBAR_TEXT)); 
    if(_toolbarState == TOOLBAR_TEXT)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));


    button = glade_xml_get_widget(_gladeXML, "menu_toolbar_both");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
    g_object_set_data(G_OBJECT(button), "me", this);
    g_signal_connect(G_OBJECT(button), 
		       "activate",
		       G_CALLBACK(menuToolbarClicked), 
		       GINT_TO_POINTER(TOOLBAR_BOTH)); 
    if(_toolbarState == TOOLBAR_BOTH)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));


    button = glade_xml_get_widget(_gladeXML, "menu_toolbar_hide");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(button), FALSE);
    g_object_set_data(G_OBJECT(button), "me", this);
    g_signal_connect(G_OBJECT(button), 
		       "activate",
		       G_CALLBACK(menuToolbarClicked), 
		       GINT_TO_POINTER(TOOLBAR_HIDE)); 
    if(_toolbarState == TOOLBAR_HIDE)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));

    // attach progress bar
    GtkWidget *boxy = glade_xml_get_widget(_gladeXML, "hbox_status");    
    assert(boxy);
    _statusL = glade_xml_get_widget(_gladeXML, "label_status");
    assert(_statusL);
    gtk_misc_set_alignment(GTK_MISC(_statusL), 0.0f, 0.0f); 
    gtk_widget_set_usize(GTK_WIDGET(_statusL), 100,-1);
    _cacheProgress = new RGCacheProgress(boxy, _statusL);
    assert(_cacheProgress);
}


void RGMainWindow::onExpandAll(GtkWidget *self, void *data) 
{
  RGMainWindow *me = (RGMainWindow *)data;

  gtk_tree_view_expand_all(GTK_TREE_VIEW(me->_treeView));
}

void RGMainWindow::onCollapseAll(GtkWidget *self, void *data) 
{
  RGMainWindow *me = (RGMainWindow *)data;

  gtk_tree_view_collapse_all(GTK_TREE_VIEW(me->_treeView));
}

void RGMainWindow::changeTreeDisplayMode(RPackageLister::treeDisplayMode mode)
{
    static RCacheActorPkgTree *_pkgCacheObserver=NULL;
    static RPackageListActorPkgTree *_pkgTreePackageListObserver=NULL;
    static RCacheActorPkgList *_pkgListCacheObserver=NULL;
    static RPackageListActorPkgList *_pkgListPackageListObserver=NULL;

    
    setInterfaceLocked(TRUE);
    _blockActions = TRUE;

    //cout << "void RGMainWindow::changeTreeDisplayMode()" << mode << endl;

    _lister->setTreeDisplayMode(mode);
    _lister->reapplyFilter();
    _treeDisplayMode = mode;

    // remove any unused observer
    if(_pkgCacheObserver) {
	delete _pkgCacheObserver; 
	_pkgCacheObserver=NULL;
    }
    if(_pkgTreePackageListObserver) {
	delete _pkgTreePackageListObserver;
	_pkgTreePackageListObserver =NULL;
    }
    if(_pkgListCacheObserver) {
	delete _pkgListCacheObserver;
	 _pkgListCacheObserver = NULL;
    }
    if(_pkgListPackageListObserver) {
	delete _pkgListPackageListObserver;
	 _pkgListPackageListObserver = NULL;
    }

    // now do the real work
    GtkPkgList *_pkgList;
    GtkPkgTree *_pkgTree;

#ifdef HAVE_DEBTAGS
    GtkTagTree *_tagTree;
#endif

    //cout << "display mode: " << _treeDisplayMode << endl;
    gtk_widget_set_sensitive(GTK_WIDGET(_filterPopup), true);
    switch(_treeDisplayMode) {
	case RPackageLister::TREE_DISPLAY_FLAT: 
	_pkgList = gtk_pkg_list_new(_lister);
	_activeTreeModel = GTK_TREE_MODEL(_pkgList);
	gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), 
				GTK_TREE_MODEL(_pkgList));
	gtk_tree_view_set_search_column (GTK_TREE_VIEW(_treeView), NAME_COLUMN);
	_pkgListCacheObserver = new RCacheActorPkgList(_lister, _pkgList, 
						     GTK_TREE_VIEW(_treeView));
	_pkgListPackageListObserver = new RPackageListActorPkgList(_lister, _pkgList, GTK_TREE_VIEW(_treeView));
	break;
#ifdef HAVE_DEBTAGS
    case RPackageLister::TREE_DISPLAY_TAGS:
	_tagTree = gtk_tag_tree_new(_lister,  _lister->_tagroot, 
				    _lister->_hmaker);

	//test_tag_tree(_tagTree);
	gtk_widget_set_sensitive(GTK_WIDGET(_filterPopup), false);

	_activeTreeModel = GTK_TREE_MODEL(_tagTree);
	gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), 
				GTK_TREE_MODEL(_tagTree));
	gtk_tree_view_set_search_column (GTK_TREE_VIEW(_treeView), NAME_COLUMN);
	break;
#endif
    default:
	_pkgTree = gtk_pkg_tree_new (_lister);
	_activeTreeModel = GTK_TREE_MODEL(_pkgTree);
	gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), 
				GTK_TREE_MODEL(_pkgTree));
	gtk_tree_view_set_search_column (GTK_TREE_VIEW(_treeView), NAME_COLUMN);
	// it may not be needed to use this actors for the tree,
	// it should be fast enough with the traditional refreshTable
	// approach, but this is just a educated guess
	_pkgCacheObserver = new RCacheActorPkgTree(_lister, _pkgTree, 
						 GTK_TREE_VIEW(_treeView));
	_pkgTreePackageListObserver = new RPackageListActorPkgTree(_lister, _pkgTree, GTK_TREE_VIEW(_treeView));
    } 
  
    _blockActions = FALSE;
    setInterfaceLocked(FALSE);

    setStatusText();
}

void RGMainWindow::onSectionTree(GtkWidget *self, void *data) 
{
  RGMainWindow *me = (RGMainWindow *)data;

  if(me->_treeDisplayMode != RPackageLister::TREE_DISPLAY_SECTIONS) {
    me->changeTreeDisplayMode(RPackageLister::TREE_DISPLAY_SECTIONS);
    me->_menuDisplayMode = RPackageLister::TREE_DISPLAY_SECTIONS;
  }
}

void RGMainWindow::onAlphabeticTree(GtkWidget *self, void *data) 
{
  RGMainWindow *me = (RGMainWindow *)data;

  if(me->_treeDisplayMode != RPackageLister::TREE_DISPLAY_ALPHABETIC) {
    me->changeTreeDisplayMode(RPackageLister::TREE_DISPLAY_ALPHABETIC);
    me->_menuDisplayMode = RPackageLister::TREE_DISPLAY_ALPHABETIC;
  }
}

void RGMainWindow::onStatusTree(GtkWidget *self, void *data) 
{
  RGMainWindow *me = (RGMainWindow *)data;

  if(me->_treeDisplayMode != RPackageLister::TREE_DISPLAY_STATUS) {
    me->changeTreeDisplayMode(RPackageLister::TREE_DISPLAY_STATUS);
    me->_menuDisplayMode = RPackageLister::TREE_DISPLAY_STATUS;
  }
}

void RGMainWindow::onFlatList(GtkWidget *self, void *data) 
{
  RGMainWindow *me = (RGMainWindow *)data;

  if(me->_treeDisplayMode != RPackageLister::TREE_DISPLAY_FLAT) {
    me->changeTreeDisplayMode(RPackageLister::TREE_DISPLAY_FLAT);
    me->_menuDisplayMode = RPackageLister::TREE_DISPLAY_FLAT;
  }
}


void RGMainWindow::onTagTree(GtkWidget *self, void *data) 
{
  RGMainWindow *me = (RGMainWindow *)data;
  
  // reset to all packages
  me->changeFilter(0);
  if(me->_treeDisplayMode != RPackageLister::TREE_DISPLAY_TAGS) {
    me->changeTreeDisplayMode(RPackageLister::TREE_DISPLAY_TAGS);
    me->_menuDisplayMode = RPackageLister::TREE_DISPLAY_TAGS;
  }
}


void RGMainWindow::onAddCDROM(GtkWidget *self, void *data) 
{
  RGMainWindow *me = (RGMainWindow *)data;
  RGCDScanner scan(me, me->_userDialog);
  me->setInterfaceLocked(TRUE);
  bool updateCache = false;
  bool dontStop = true;
  while (dontStop) {
    if (scan.run() == false) {
      me->showErrors();
    } else {
      updateCache = true;
    }
    dontStop = me->_userDialog->confirm(
			_("Do you want to add another CD-ROM?"));
  }
  scan.hide();
  if (updateCache) {
    me->_lister->openCache(TRUE);
    me->refreshTable(me->selectedPackage());
  }
  me->setInterfaceLocked(FALSE);
}

void RGMainWindow::pkgInstallHelper(RPackage *pkg, bool fixBroken)
{
    //cout << "pkgInstallHelper()/start" << endl;
    // do the work
    pkg->setInstall();

    // check whether something broke
    if (fixBroken && !_lister->check()) {
	cout << "inside fixBroken" << endl;
	_lister->fixBroken();
    }

    //cout << "pkgInstallHelper()/end" << endl;
}

void RGMainWindow::pkgRemoveHelper(RPackage *pkg, bool purge, bool withDeps)
{
  if (pkg->isImportant()) {
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
  if(!withDeps)
      pkg->setRemove(purge);
  else
      pkg->setRemoveWithDeps(true, false);
}

void RGMainWindow::pkgKeepHelper(RPackage *pkg)
{
  pkg->setKeep();
}


void RGMainWindow::selectedRow(GtkTreeSelection *selection, gpointer data)
{ 
    RGMainWindow *me = (RGMainWindow*)data;
    GtkTreeIter iter;
    RPackage *pkg;
    GList *li, *list;

    //cout << "selectedRow()" << endl;

    if (me->_activeTreeModel == NULL) {
	cerr << "selectedRow(): me->_pkgTree == NULL " << endl;
	return;
    }

#if GTK_CHECK_VERSION(2,2,0)
    list = li = gtk_tree_selection_get_selected_rows(selection,
 						     &me->_activeTreeModel);
#else
    li = list = NULL;
    gtk_tree_selection_selected_foreach(selection, multipleSelectionHelper,
					&list);
    li = list;
#endif

    // list is empty
    if(li == NULL) {
	me->updatePackageInfo(NULL);
	return;
    }

    // we are only interessted in the last element
    li = g_list_last(li);
    gtk_tree_model_get_iter(me->_activeTreeModel, &iter, 
			    (GtkTreePath*)(li->data));

    gtk_tree_model_get(me->_activeTreeModel, &iter, 
		       PKG_COLUMN, &pkg, -1);
    if (pkg == NULL)
	return;    
//     cout << "selected: " << pkg->name() 
// 	 << " path: " << (li->data)
// 	 << endl;

    // free the list
    g_list_foreach(list, (void (*)(void*,void*))gtk_tree_path_free, NULL);
    g_list_free (list);

    me->updatePackageInfo(pkg);
}

void RGMainWindow::doubleClickRow(GtkTreeView *treeview,
				  GtkTreePath *path,
				  GtkTreeViewColumn *arg2,
				  gpointer data)
{
  RGMainWindow *me = (RGMainWindow*)data;
  GtkTreeIter iter;
  RPackage *pkg = NULL;

  //  cout << "double click" << endl;
  if(!gtk_tree_model_get_iter(me->_activeTreeModel,
			      &iter,path)) {
    return;
  }
  gtk_tree_model_get(me->_activeTreeModel, &iter, 
		     PKG_COLUMN, &pkg, -1);

  /* pkg is only NULL for secions */
  if(pkg == NULL) {
      if(!gtk_tree_view_row_expanded(GTK_TREE_VIEW(me->_treeView), path))
	  gtk_tree_view_expand_row(GTK_TREE_VIEW(me->_treeView), path, false);
      else
	  gtk_tree_view_collapse_row(GTK_TREE_VIEW(me->_treeView), path);
      return;
  }
  // double click
  //me->setInterfaceLocked(TRUE);
  
  RPackage::PackageStatus pstatus =  pkg->getStatus();
  //   SInstalledUpdated,
  //   SInstalledOutdated,
  //   SInstalledBroken,
  //   SNotInstalled

  RPackage::MarkedStatus  mstatus = pkg->getMarkedStatus();
  //   MKeep,
  //   MInstall,
  //   MUpgrade,
  //   MDowngrade,
  //   MRemove,
  //   MHeld
  
  if( pstatus == RPackage::SNotInstalled) {
    if (mstatus == RPackage::MKeep) {
      // not installed -> installed
	me->doPkgAction(me, PKG_INSTALL);
    }
    if (mstatus == RPackage::MInstall) 
	// marked install -> marked don't install
	me->doPkgAction(me, PKG_DELETE);
  }
  
  if( pstatus == RPackage::SInstalledOutdated ) {
    if ( mstatus == RPackage::MKeep ) {
	// keep -> upgrade
	me->doPkgAction(me, PKG_INSTALL);
    }
    if( mstatus == RPackage::MUpgrade) {
	// upgrade -> keep
	me->doPkgAction(me, PKG_KEEP);
    }
  }
  // end double-click
  gtk_tree_view_set_cursor(GTK_TREE_VIEW(me->_treeView), path,
			   NULL, false);

  me->setStatusText();
  return;
}


void RGMainWindow::setStatusText(char *text)
{

    int listed, installed, broken;
    int toinstall, toremove;
    double size;

    _lister->getStats(installed, broken, toinstall, toremove, size);

    if (text) {
	gtk_label_set_text(GTK_LABEL(_statusL), text);
    } else {
	gchar *buffer;
	// we need to make this two strings for i18n reasons
	listed = _lister->count();
	if(size < 0) {
	    buffer = g_strdup_printf(_("%i packages listed, %i installed, %i broken. %i to install/upgrade, %i to remove; %sB will be freed"),
				     listed, installed, broken, toinstall, toremove,
				     SizeToStr(fabs(size)).c_str());
	} else {
	    buffer = g_strdup_printf(_("%i packages listed, %i installed, %i broken. %i to install/upgrade, %i to remove; %sB will be used"),
				     listed, installed, broken, toinstall, toremove,
				     SizeToStr(fabs(size)).c_str());
	}; 		    
	gtk_label_set_text(GTK_LABEL(_statusL), buffer);
	g_free(buffer);
    }
    
    gtk_widget_set_sensitive(_upgradeB, _lister->upgradable() );
    gtk_widget_set_sensitive(_upgradeM, _lister->upgradable() );
    gtk_widget_set_sensitive(_distUpgradeB, _lister->upgradable() );
    gtk_widget_set_sensitive(_distUpgradeM, _lister->upgradable() );

    gtk_widget_set_sensitive(_proceedB, (toinstall + toremove) != 0);
    gtk_widget_set_sensitive(_proceedM, (toinstall + toremove) != 0);
    //cout << "toinstall: " << toinstall << " toremove: " << toremove << endl;
    _unsavedChanges = ((toinstall + toremove) != 0);
    
    gtk_widget_queue_draw(_statusL);
}

GtkWidget *RGMainWindow::createFilterMenu()
{
    GtkWidget *menu, *item;
    vector<string> filters;
    _lister->getFilterNames(filters);

    menu = gtk_menu_new();

    item = gtk_menu_item_new_with_label(_("All Packages"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_object_set_data(GTK_OBJECT(item), "me", this);
    gtk_object_set_data(GTK_OBJECT(item), "index", (void*)0);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
		       (GtkSignalFunc)changedFilter, this);
  
    int i = 1;
    for (vector<string>::const_iterator iter = filters.begin();
	 iter != filters.end(); iter++) {
	
	item = gtk_menu_item_new_with_label((char*)(*iter).c_str());
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_object_set_data(GTK_OBJECT(item), "me", this);
	gtk_object_set_data(GTK_OBJECT(item), "index", (void*)i++);
	gtk_signal_connect(GTK_OBJECT(item), "activate", 
			   (GtkSignalFunc)changedFilter, this);
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

    // Menubar
    gtk_menu_item_remove_submenu(GTK_MENU_ITEM(_filterMenu));
    menu = createFilterMenu();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(_filterMenu), menu);
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
    _config->Set("Synaptic::ToolbarState", (int)_toolbarState);
    _config->Set("Synaptic::TreeDisplayMode", _menuDisplayMode);

    if (!RWriteConfigFile(*_config)) {
	_error->Error(_("An error occurred while saving configurations."));
	_userDialog->showErrors();
    }
    if(!_roptions->store())
      cerr << "error storing raptoptions" << endl;
}


bool RGMainWindow::restoreState()
{
#ifdef HAVE_DEBTAGS
    // FIXME: this is not a good place! find a better
    _lister->_hmaker = new HandleMaker<RPackage*>();
    RTagcollBuilder builder(*_lister->_hmaker, _lister);

    if(FileExists("/var/lib/debtags/package-tags") &&
       FileExists("/var/lib/debtags/implications") &&
       FileExists("/var/lib/debtags/derived-tags") ) 
    {

	StdioParserInput in("/var/lib/debtags/package-tags");
	StdioParserInput impl("/var/lib/debtags/implications");
	StdioParserInput derv("/var/lib/debtags/derived-tags");
	TagcollParser::parseTagcoll(in, impl, derv,  builder);
    } else {
	_userDialog->warning(_("The debtags database is not installed\n\n"
			       "Please call \"debtags update\" in a shell "
			       "to get the database\n\n"
			       "Sorry for this inconvenience, it will go away "
			       "in one of the next versions "
			       "(patches are welcome)!"
			       ) );
    }
    _lister->_coll = builder.collection();
    TagCollection<int> filtered = _lister->_coll;
    filtered.removeTagsWithCardinalityLessThan(8);
    _lister->_tagroot = new CleanSmartHierarchyNode<int>("_top", _lister->_coll, 0);

    // HOWTO do the filtering for a limited range
    // Create a builder
    // Create your filter
    // Set the builder as the filter output
    //coll.output(filter);
    //TagCollection<int> 
#endif

    // see if we have broken packages (might be better in some
    // RGMainWindow::preGuiStart funktion)
    int installed, broken, toinstall,  toremove;
    double sizeChange;
    _lister->getStats(installed, broken, toinstall, toremove,sizeChange);
    if(broken > 0) {
	gchar *msg;
	if(broken == 1)
	    msg = g_strdup_printf(("You have %i broken package on your system\n\n"
				      "Please try to fix it by visiting the \"Broken\" filter"), broken);
	else
	    msg = g_strdup_printf(("You have %i broken package on your system\n\n"
				      "Please try to fix them by visiting the \"Broken\" filter"), broken);
	_userDialog->warning(msg);
	g_free(msg);
    }
    


    // this is real restore stuff
    _lister->restoreFilters();
    refreshFilterMenu();
    
    int filterNr = _config->FindI("Volatile::initialFilter", 0);
    changeFilter(filterNr);
    refreshTable();
    
    setStatusText();
    return true;
}


void RGMainWindow::close()
{
    if (_interfaceLocked > 0)
	return;

    RGGladeUserDialog dia(this);
    if (_unsavedChanges == false || dia.run("quit")) {
	_error->Discard();
	saveState();
	showErrors();
	exit(0);
    }

	
    //cout << "no exit" << endl;
    //BUG/FIXME: don't know why this is needed, but if I ommit it, the 
    //     window will be closed even if I click on "no" in the confirm dialog
    cout<<endl;
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

  // this sucks for the new gtktreeview -- it updates itself via 
  // such events, while the gui is perfetly usable 
#if 0
  while (gtk_events_pending())
    gtk_main_iteration();
#endif
  // because of the comment above, we only do 5 iterations now
  //FIXME: this is more a hack than a real solution
  for(int i=0;i<5;i++) {
      if(gtk_events_pending())
	  gtk_main_iteration();
  }
}

void RGMainWindow::setTreeLocked(bool flag)
{
    //cout << "setTreeLocked()" << endl;
    if (flag == true) {
	updatePackageInfo(NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), NULL);
    } else {
	changeTreeDisplayMode(_treeDisplayMode);
    }
}

// vim:sts=4:sw=4
