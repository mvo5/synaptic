/* rgmainwindow.cc - main window of the app
 * 
 * Copyright (c) 2001 Alfredo K. Kojima
 *               2002 Michael Vogt <mvo@debian.org>
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


#define DEBUGUI

#include "config.h"
#include "i18n.h"


#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <glade/glade.h>
#include <gdk/gdk.h>
#include <cmath>

#include <apt-pkg/strutl.h>
#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>

#include <X11/keysym.h>
#include <gdk/gdkkeysyms.h>

#include "raptoptions.h"
#include "rconfiguration.h"
#include "rgmainwindow.h"
#include "rgfindwindow.h"
#include "rpackagefilter.h"
#include "galertpanel.h"
#include "raptoptions.h"

#include "rgfiltermanager.h"
#include "rgfilterwindow.h"
#include "rgsrcwindow.h"
#include "rgconfigwindow.h"
#include "rgaboutpanel.h"
#include "rgsummarywindow.h"

#include "rgfetchprogress.h"
#include "rgcacheprogress.h"
#include "rguserdialog.h"
#include "rginstallprogress.h"
#include "rgdummyinstallprogress.h"
#include "rgzvtinstallprogress.h"
#include "conversion.h"

// icons and pixmaps

#include "logo.xpm"
#include "synaptic_mini.xpm"

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



extern void RGFlushInterface();


static char *ImportanceNames[] = {
    _("Unknown"),
    _("Normal"),
    _("Critical"),
    _("Security")
};


enum {WHAT_IT_DEPENDS_ON, 
      WHAT_DEPENDS_ON_IT,
      WHAT_IT_WOULD_DEPEND_ON,
      WHAT_IT_SUGGESTS};      


static GdkPixmap *StatusPixmaps[12];
static GdkBitmap *StatusMasks[12];
static GdkColor *StatusColors[12];


#define SELECTED_MENU_INDEX(popup) \
    (int)gtk_object_get_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(gtk_option_menu_get_menu(GTK_OPTION_MENU(popup))))), "index")



void RGMainWindow::changedDepView(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    int index = SELECTED_MENU_INDEX(me->_depP);

    gtk_notebook_set_page(GTK_NOTEBOOK(me->_depTab), index);
}

void RGMainWindow::clickedRecInstall(GtkWidget *self, void *data)
{
  RGMainWindow *me = (RGMainWindow*)gtk_object_get_data(GTK_OBJECT(self), "me");
  RPackage *pkg, *newpkg;
  RFilter *filter;
  GList *selection;
  const char *depType, *depPkg;
  char *recstr;
  bool ok=false;
  int i;

  me->_lister->unregisterObserver(me);
  me->setInterfaceLocked(TRUE);
  
  // we need to go into "all package" mode 
  filter = me->_lister->getFilter();
  me->_lister->setFilter();

  pkg = (RPackage*)gtk_object_get_data(GTK_OBJECT(me->_recList),"pkg");

  switch((int)data) {
  case InstallRecommended:
    if(pkg->enumWDeps(depType, depPkg, ok)) {
      do {
	if (!ok && !strcmp(depType,"Recommends")) {
	  i = me->_lister->findPackage((char*)depPkg);
	  if(i<0) {
	    cerr << depPkg << " not found" << endl;
	    continue;
	  }
	  newpkg=me->_lister->getElement(i);
	  assert(newpkg);
	  newpkg->setInstall();
	  if (!me->_lister->check()) {
	    me->_lister->fixBroken();
	  }
	}
      } while(pkg->nextWDeps(depType, depPkg, ok));
    }
    break;
  case InstallSuggested:
    if(pkg->enumWDeps(depType, depPkg, ok)) {
      do {
	if (!ok && !strcmp(depType,"Suggests")) {
	  i = me->_lister->findPackage((char*)depPkg);
	  if(i<0) {
	    cerr << depPkg << " not found" << endl;
	    continue;
	  }
	  newpkg=me->_lister->getElement(i);
	  assert(newpkg);
	  newpkg->setInstall();
	  if (!me->_lister->check()) {
	    me->_lister->fixBroken();
	  }
	}
      } while(pkg->nextWDeps(depType, depPkg, ok));
    }
    break;
  case InstallSelected:
    selection = GTK_CLIST(me->_recList)->selection;
    while(selection!=NULL) {
      int row = GPOINTER_TO_INT(selection->data);
      gtk_clist_get_text(GTK_CLIST(me->_recList), row, 0, &recstr);
      //cout << "selected row: " << row  << " is " << recstr << endl;
      if(!recstr) { 
	cerr <<"gtk_clist_get_text() returned no text" << endl;
	selection=g_list_next(selection);
	continue;
      }
      // mvo: "parser" should be more robust :-/
      depPkg = index(recstr, ':') + 2;
      if(!depPkg) {
	selection=g_list_next(selection);
	continue;
      }
      i = me->_lister->findPackage((char*)depPkg);
      if(i<0) {
	cerr << depPkg << " not found" << endl;
	selection=g_list_next(selection);
	continue;
      }
      newpkg=(RPackage*)me->_lister->getElement(i);
      assert(newpkg);

      newpkg->setInstall();
      if (!me->_lister->check()) {
	me->_lister->fixBroken();
      }
      selection = g_list_next(selection);
    }
    break;
  default:
    cerr << "clickedRecInstall called with invalid parm: " << data << endl;
  }

  me->_lister->setFilter(filter);
  me->_lister->registerObserver(me);
  me->setInterfaceLocked(FALSE);
  me->refreshTable(pkg);
}

void RGMainWindow::clickedDepList(GtkWidget *self, int row, int col,
				  GdkEvent *event)
{
    RGMainWindow *me = (RGMainWindow*)gtk_object_get_data(GTK_OBJECT(self),"me");
    assert(me);
    gchar *text=NULL;
    text = (gchar*)gtk_clist_get_row_data(GTK_CLIST(me->_depList), row);
    //cout << "clickedDepList: " << "row: " << row << " " << text << endl;
    assert(text);
    gtk_label_set_text(GTK_LABEL(me->_depInfoL), text);
}

void RGMainWindow::clickedAvailDepList(GtkWidget *self, int row, int col,
				       GdkEvent *event, void* data)
{
  RGMainWindow *me = (RGMainWindow*)gtk_object_get_data(GTK_OBJECT(self),"me");
  char *text;
  text = (char*)gtk_clist_get_row_data(GTK_CLIST(me->_availDepList), row);
  if(text != NULL)
    gtk_label_set_text(GTK_LABEL(me->_availDepInfoL), text);
}

void RGMainWindow::showAboutPanel(GtkWidget *self, void *data)
{
    RGMainWindow *win = (RGMainWindow*)data;
    
    if (win->_aboutPanel == NULL)
	win->_aboutPanel = new RGAboutPanel(win);
    win->_aboutPanel->show();
}


void RGMainWindow::findPackageObserver(GtkWidget *self)
{
    RGMainWindow *me = (RGMainWindow*)gtk_object_get_data(GTK_OBJECT(self), "me");
    const char *text = gtk_entry_get_text(GTK_ENTRY(me->_findText));
    int row;

    row = me->_lister->findPackage(text);
    if (row >= 0) {
      gtk_clist_unselect_all(GTK_CLIST(me->_table));

      // mvo: this is needed! otherwise the selection handling
      // after searching will act funny (see debian BTS #171207 for
      // details)
      GTK_CLIST(me->_table)->anchor = row;

      gtk_clist_select_row(GTK_CLIST(me->_table), row, 0);
      gtk_clist_moveto(GTK_CLIST(me->_table), row, 0, 0.5, 0.0);
      me->setStatusText();
    } else {
      me->setStatusText(_("No match."));
    }
}


void RGMainWindow::findNextAction(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    
    int row = me->_lister->findNextPackage();
    if (row >= 0) {
      gtk_clist_unselect_all(GTK_CLIST(me->_table));
      gtk_clist_select_row(GTK_CLIST(me->_table), row, 0);
      gtk_clist_moveto(GTK_CLIST(me->_table), row, 0, 0.5, 0.0);
      me->setStatusText();
    } else {
      me->setStatusText(_("No more matches."));
    }
}


void RGMainWindow::makeFinderFilterAction(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    RFilter *filter;
    
    filter = me->_lister->findFilter(0);

    const char *pattern = gtk_entry_get_text(GTK_ENTRY(me->_findText));

    if (pattern && *pattern) {
	filter->reset();
	filter->pattern.addPattern(RPatternPackageFilter::Name,
				   string(pattern), FALSE);
	me->changeFilter(1);
    }
}


void RGMainWindow::saveFilterAction(void *self, RGFilterWindow *rwin)
{
    RGMainWindow *me = (RGMainWindow*)self;
    RPackage *pkg = me->selectedPackage();
    bool filterEditable;
    
    filterEditable = SELECTED_MENU_INDEX(me->_filterPopup) != 0;    
    
    gtk_widget_set_sensitive(me->_filtersB, TRUE);
    gtk_widget_set_sensitive(me->_editFilterB, filterEditable);
    gtk_widget_set_sensitive(me->_filterPopup, TRUE);
    gtk_widget_set_sensitive(me->_filterMenu, TRUE);

    me->_lister->reapplyFilter();
    me->setStatusText();

    me->refreshTable(pkg);
    
    rwin->hide();
}



void RGMainWindow::closeFilterAction(void *self, RGFilterWindow *rwin)
{
    RGMainWindow *me = (RGMainWindow*)self;
    bool filterEditable;
    
    filterEditable = SELECTED_MENU_INDEX(me->_filterPopup) != 0;

    gtk_widget_set_sensitive(me->_editFilterB, filterEditable);
    gtk_widget_set_sensitive(me->_filtersB, TRUE);
    gtk_widget_set_sensitive(me->_filterPopup, TRUE);
    gtk_widget_set_sensitive(me->_filterMenu, TRUE);

    rwin->hide();
}


void RGMainWindow::showFilterWindow(GtkWidget *self, void *data)
{
    RGMainWindow *win = (RGMainWindow*)data;

    if (win->_filterWin == NULL) {
	win->_filterWin = new RGFilterWindow(win, win->_lister);

	win->_filterWin->setSaveCallback(saveFilterAction, win);
	win->_filterWin->setCloseCallback(closeFilterAction, win);
    }
    
    if (win->_lister->getFilter() == 0)
	return;
    
    win->_filterWin->editFilter(win->_lister->getFilter());
    
    gtk_widget_set_sensitive(win->_editFilterB, FALSE);
    gtk_widget_set_sensitive(win->_filtersB, FALSE);
    gtk_widget_set_sensitive(win->_filterPopup, FALSE);
    gtk_widget_set_sensitive(win->_filterMenu, FALSE);
}


void RGMainWindow::closeFilterManagerAction(void *self, 
					    RGFilterManagerWindow *win)
{
    RGMainWindow *me = (RGMainWindow*)self;

    // select 0, because they may have deleted the current filter
    gtk_option_menu_set_history(GTK_OPTION_MENU(me->_filterPopup), 0);
    //    changedFilter(me->_filterPopup);

    me->refreshFilterMenu();
    
    gtk_widget_set_sensitive(me->_editFilterB, FALSE);
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
    gtk_widget_set_sensitive(win->_editFilterB, FALSE);
    gtk_widget_set_sensitive(win->_filterPopup, FALSE);
    gtk_widget_set_sensitive(win->_filterMenu, FALSE);
}


void RGMainWindow::showSourcesWindow(GtkWidget *self, void *data)
{
    RGMainWindow *win = (RGMainWindow*)data;
        
    RGSrcEditor w(win);
    w.Run();
      
}


void RGMainWindow::pkgHelpClicked(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;

    //cout << "RGMainWindow::pkgHelpClicked()" << endl;
    
    system(g_strdup_printf("dwww %s &", me->selectedPackage()->name()));
}


void RGMainWindow::showConfigWindow(GtkWidget *self, void *data)
{
    RGMainWindow *win = (RGMainWindow*)data;
    
    if (win->_configWin == NULL)
	win->_configWin = new RGConfigWindow(win);

    win->_configWin->show();
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
  if (sethistory)
    gtk_option_menu_set_history(GTK_OPTION_MENU(_filterPopup), filter);
  
  RPackage *pkg = selectedPackage();
    
  setInterfaceLocked(TRUE);    
  if (filter == 0) { // no filter
    _lister->setFilter();
    gtk_widget_set_sensitive(_editFilterB, FALSE);
  } else {
    _lister->setFilter(filter-1);
    gtk_widget_set_sensitive(_editFilterB, TRUE);
  }
  
  refreshTable(pkg);

  int index = -1;

  if (pkg) {	
    index = _lister->getElementIndex(pkg);
  }    
    
  if (index >= 0) 
    gtk_clist_moveto(GTK_CLIST(_table), index, 0, 0.5, 0.0);
  else
    updatePackageInfo(NULL);
  
  setInterfaceLocked(FALSE);
  
  setStatusText();
}


void RGMainWindow::switchCommandPanel(GtkWidget *self, void *data)
{
    RGMainWindow *mainw = (RGMainWindow*)data;
    
    if (mainw->_panelSwitched) {
	gtk_notebook_set_page(GTK_NOTEBOOK(mainw->_cmdPanel), 0);
	mainw->_panelSwitched = FALSE;
    } else {
	gtk_notebook_set_page(GTK_NOTEBOOK(mainw->_cmdPanel), 1);
	mainw->_panelSwitched = TRUE;	
    }
}


void RGMainWindow::updateClicked(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    RPackage *pkg = me->selectedPackage();
    gchar *pkgname = NULL;
    
    if(pkg != NULL) 
      pkgname = g_strdup(pkg->name());


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
    
    if(pkgname != NULL) {
      int row = me->_lister->findPackage(pkgname);
      pkg = me->_lister->getElement(row);
    } else {
      pkg = NULL;
    }
    
    
    me->refreshTable(pkg);
    me->setInterfaceLocked(FALSE);
    me->setStatusText();

    g_free(pkgname);
}


RPackage *RGMainWindow::selectedPackage()
{
  if(g_list_length(GTK_CLIST(_table)->selection) > 0) { 
    int row;
    row = GPOINTER_TO_INT(g_list_last(GTK_CLIST(_table)->selection)->data);
    return _lister->getElement(row);
  } else {
    return NULL;
  }
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
	gtk_run_alert_panel(me->window(), _("Error"), 
			    _("Automatic upgrade selection not possible\n"
			      "with broken packages. Please fix them first."),
			    _("Ok"), NULL, NULL);
	return;
    }
    
    me->setInterfaceLocked(TRUE);
    me->setStatusText(_("Performing automatic selection of upgradadable packages..."));
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
	gtk_run_alert_panel(me->window(),
			_("Error"), _("Automatic upgrade selection not possible\n"
			"with broken packages. Please fix them first."),
			_("Ok"), NULL, NULL);
	return;
    }

    me->setInterfaceLocked(TRUE);
     me->setStatusText(_("Performing selection for distribution upgrade..."));

    res = me->_lister->distUpgrade();
    me->refreshTable(pkg);

    if (res)
	me->setStatusText(_("Selection for distribution upgrade done."));
    else
	me->setStatusText(_("Selection for distribution upgrade failed."));

    me->setInterfaceLocked(FALSE);
    me->showErrors();
}



void RGMainWindow::proceedClicked(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    RPackage *pkg = me->selectedPackage();
    gchar *pkgname = NULL;
    RGSummaryWindow *summ;

    if(pkg != NULL) 
      pkgname = g_strdup(pkg->name());

    // check whether we can really do it
    if (!me->_lister->check()) {
	gtk_run_alert_panel(me->window(), _("Error"), 
			    _("Operation not possible with broken packages.\n"
			      "Please fix them first."),
			    _("Ok"), NULL, NULL);
	return;
    }
    
    // show a summary of what's gonna happen
    summ = new RGSummaryWindow(me, me->_lister);
    if (!summ->showAndConfirm()) {
	// canceled operation
	delete summ;
	return;
    }
    delete summ;

    me->setInterfaceLocked(TRUE);
    me->updatePackageInfo(NULL);

    me->setStatusText(_("Performing selected changes... See the terminal from where you started Synaptic for more information."));

    // fetch packages
    RGFetchProgress *fprogress = new RGFetchProgress(me);
    fprogress->setTitle(_("Retrieving Package Files"));

#ifdef HAVE_ZVT
    RGZvtInstallProgress *iprogress = new RGZvtInstallProgress(me);
#else
 #ifdef HAVE_RPM
    RGInstallProgress *iprogress = new RGInstallProgress(me);
 #else
    RGDummyInstallProgress *iprogress = new RGDummyInstallProgress();
 #endif
#endif
    
    //bool result = me->_lister->commitChanges(fprogress, iprogress);
    me->_lister->commitChanges(fprogress, iprogress);

    delete fprogress;
#ifdef HAVE_ZVT
    // wait until the zvt dialog is closed
    while(GTK_WIDGET_VISIBLE(GTK_WIDGET(iprogress->window()))) {
      RGFlushInterface();
      usleep(1000);
    }
#endif
    delete iprogress;

    me->showErrors();

    if (_config->FindB("Synaptic::Download-Only", FALSE) == FALSE) {    
	// reset the cache
	if (!me->_lister->openCache(TRUE)) {
	    me->showErrors();
	    exit(1);
	}
    }

    if(pkgname != NULL) {
      int row = me->_lister->findPackage(pkgname);
      pkg = me->_lister->getElement(row);
    } else {
      pkg = NULL;
    }

    me->refreshTable(pkg);
    me->setInterfaceLocked(FALSE);
    me->setStatusText();
    
    g_free(pkgname);
}


bool RGMainWindow::showErrors()
{
    string message;
    int lines;
    char *type;
    
    if (_error->empty())
	return FALSE;
    
    if (_error->PendingError())
	type = _("Error");
    else
	type = _("Warning");

        
    lines = 0;
    message = "";
    while (!_error->empty()) {
	string tmp;

	_error->PopMessage(tmp);
       
        // ignore some stupid error messages
	if (tmp == "Tried to dequeue a fetching object")
	   continue;

	if (message.empty())
	    message = tmp;
	else
	    message = message + "\n\n" + tmp;
    }

    gtk_run_errors_panel(_win, type, message.c_str(),
			_("OK"), NULL, NULL);
    
    return TRUE;
}


static void appendTag(char *&buf, unsigned &size, const char *tag, 
			const char *value)
{
    if (!value)
	value = _("N/A");
    if (strlen(tag) + strlen(value) + 8 > size)
	return;
    
    buf = buf + sprintf(buf, "%s: %s\n", tag, value);
}


static void appendText(char *&buf, unsigned &size, const char *text)
{
    unsigned l = strlen(text);
    if (l > size)
	return;
    
    strcpy(buf, text);
    buf = buf + l;
}



void RGMainWindow::notifyChange(RPackage *pkg)
{
  //cout << "RGMainWindow::notifyChange("<<pkg->name()<<")" << endl;
    // we changed pkg's status
    updateDynPackageInfo(pkg);

    refreshTable(pkg);

    setStatusText();
}

void RGMainWindow::forgetNewPackages()
{
  //cout << "forgetNewPackages called" << endl;
  int row=0;
  while (row < _lister->count()) {
    RPackage *elem = _lister->getElement(row);
    if(elem->getOtherStatus() && RPackage::ONew)
      elem->setNew(false);
  }
  _roptions->forgetNewPackages();
}

void RGMainWindow::refreshTable(RPackage *selectedPkg)
{
  static bool first_run = true;

  gtk_clist_freeze(GTK_CLIST(_table));
  gtk_clist_clear(GTK_CLIST(_table));
  
  unsigned int row = 0;

  while (row < _lister->count()) {
    const char *array[8];
    RPackage *elem = _lister->getElement(row);
    assert(elem);
    string pkgName = elem->name();

    array[0] = NULL;
    //	array[1] = elem->section();
    array[1] = pkgName.c_str();
    array[2] = elem->installedVersion();
    array[3] = elem->availableDownloadableVersion();
    array[4] = NULL;
    
    gtk_clist_insert(GTK_CLIST(_table), row, (char**)array);

    /* status */
    RPackage::MarkedStatus s = elem->getMarkedStatus();
    int other = elem->getOtherStatus();

    if (elem->wouldBreak()) {
      gtk_clist_set_pixmap(GTK_CLIST(_table), row, 0,
			   StatusPixmaps[(int)RPackage::MBroken], 
			   StatusMasks[(int)RPackage::MBroken]);
      gtk_clist_set_background(GTK_CLIST(_table), row, 
			       StatusColors[(int)RPackage::MBroken]);
    } else if (other & RPackage::OPinned) {
      gtk_clist_set_pixmap(GTK_CLIST(_table), row, 0,
			   StatusPixmaps[(int)RPackage::MPinned],
			   StatusMasks[(int)RPackage::MPinned]);
      gtk_clist_set_background(GTK_CLIST(_table), row, 
			       StatusColors[(int)RPackage::MPinned]);
    } else if (s == RPackage::MKeep 
	       && elem->getStatus() == RPackage::SInstalledOutdated) {
      gtk_clist_set_pixmap(GTK_CLIST(_table), row, 0,
			   StatusPixmaps[RPackage::MHeld],
			   StatusMasks[RPackage::MHeld]);
      gtk_clist_set_background(GTK_CLIST(_table), row, 
			       StatusColors[RPackage::MHeld]);
    } else if ((other & RPackage::ONew) && 
	       !elem->getMarkedStatus() == RPackage::MInstall) {
      gtk_clist_set_pixmap(GTK_CLIST(_table), row, 0,
			   StatusPixmaps[(int)RPackage::MNew],
			   StatusMasks[(int)RPackage::MNew]);
      gtk_clist_set_background(GTK_CLIST(_table), row, 
			       StatusColors[(int)RPackage::MNew]);
    } else {
       gtk_clist_set_pixmap(GTK_CLIST(_table), row, 0,
 			   StatusPixmaps[(int)s],
 			   StatusMasks[(int)s]);
       gtk_clist_set_background(GTK_CLIST(_table), row, StatusColors[(int)s]);
    }

    const char *str = elem->summary().c_str();
	
    if (_showUpdateInfo && 
	elem->updateImportance() == RPackage::ISecurity)
      gtk_clist_set_pixtext(GTK_CLIST(_table), row, 4, _iconv.convert(str,strlen(str)), 4,
			    StatusPixmaps[8],
			    StatusMasks[8]);
    else
      gtk_clist_set_text(GTK_CLIST(_table), row, 4, _iconv.convert(str,strlen(str)));

    row++;	
  }

  while (row < GTK_CLIST(_table)->rows)
    gtk_clist_remove(GTK_CLIST(_table), row);
  
  gtk_clist_thaw(GTK_CLIST(_table));
  if (selectedPkg != NULL) {
    int index = _lister->getElementIndex(selectedPkg);
    //cout << "selectedPkg != NULL " << index << endl;
    if (index >= 0) {
      gtk_clist_select_row(GTK_CLIST(_table), index, 0);
      updatePackageInfo(selectedPkg);
      if (!gtk_clist_row_is_visible(GTK_CLIST(_table), index))
	gtk_clist_moveto(GTK_CLIST(_table), index, 0, 0.5, 0.0);
    } else {
      // package no longer in listing
      updatePackageInfo(NULL);
    }
  }

  if(first_run) {
    first_run=false;
  }
}


void RGMainWindow::updatePackageStatus(RPackage *pkg)
{
    bool installed = FALSE;
    RPackage::PackageStatus status = pkg->getStatus();
    RPackage::MarkedStatus mstatus = pkg->getMarkedStatus();
    int other = pkg->getOtherStatus();

    switch (status) {
     case RPackage::SInstalledUpdated:
	gtk_widget_set_sensitive(_actionB[1], FALSE);
	gtk_widget_set_sensitive(_actionB[2], TRUE);
	gtk_widget_set_sensitive(_pinB, TRUE);
	gtk_widget_set_sensitive(_pinM, TRUE);
	gtk_widget_set_sensitive(_pkgHelp, TRUE);
	installed = TRUE;
	break;
	
     case RPackage::SInstalledOutdated:
	gtk_widget_set_sensitive(_actionB[1], TRUE);
	gtk_widget_set_sensitive(_actionB[2], TRUE);
	gtk_widget_set_sensitive(_pinB, TRUE);
	gtk_widget_set_sensitive(_pinM, TRUE);
	gtk_widget_set_sensitive(_pkgHelp, TRUE);
	installed = TRUE;
	break;
	
     case RPackage::SInstalledBroken:
	gtk_widget_set_sensitive(_actionB[1], FALSE);
	gtk_widget_set_sensitive(_actionB[2], TRUE);
	gtk_widget_set_sensitive(_pinB, TRUE);
	gtk_widget_set_sensitive(_pinM, TRUE);
	gtk_widget_set_sensitive(_pkgHelp, TRUE);
	installed = TRUE;
	break;
	
     case RPackage::SNotInstalled:
	gtk_widget_set_sensitive(_actionB[1], TRUE);
	gtk_widget_set_sensitive(_actionB[2], FALSE);
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
	// not handled yet
	puts("OH SHIT!!");
	break;
	
     case RPackage::MRemove:
	_currentB = _actionB[2];
	gtk_label_set_text(GTK_LABEL(_stateL), _("Package will be uninstalled."));
	break;
    case RPackage::MBroken:
      gtk_label_set_text(GTK_LABEL(_stateL), _("Package is broken."));
      break;
    }

    switch(other){
    case RPackage::OPinned:
      gtk_label_set_text(GTK_LABEL(_stateL), _("Package is pinned."));
      break;
    case RPackage::ONew:
      gtk_label_set_text(GTK_LABEL(_stateL), _("Package is new."));
      break;
    }

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
      gtk_image_set_from_pixmap(GTK_IMAGE(_stateP), 
		     StatusPixmaps[RPackage::MBroken],
		     StatusMasks[RPackage::MBroken]);
    } else if (mstatus==RPackage::MKeep && status==RPackage::SInstalledOutdated) {
      gtk_image_set_from_pixmap(GTK_IMAGE(_stateP), 
				StatusPixmaps[RPackage::MHeld],
				StatusMasks[RPackage::MHeld]);
    }  else if(other & RPackage::ONew) {
      gtk_image_set_from_pixmap(GTK_IMAGE(_stateP), 
				StatusPixmaps[RPackage::MNew], 
				StatusMasks[RPackage::MNew]);
    } else if (locked) {
      gtk_image_set_from_pixmap(GTK_IMAGE(_stateP), 
				StatusPixmaps[RPackage::MPinned], 
				StatusMasks[RPackage::MPinned]);
    } else {
      gtk_image_set_from_pixmap(GTK_IMAGE(_stateP), 
				StatusPixmaps[(int)mstatus],
				StatusMasks[(int)mstatus]);
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

    updatePackageStatus(pkg);

    // dependencies
    gtk_label_set_text(GTK_LABEL(_depInfoL), "");    

    gtk_clist_clear(GTK_CLIST(_depList));
    
    bool byProvider = TRUE;

    const char *depType, *depPkg, *depName, *depVer;
    char *summary;
    bool ok;
    if (pkg->enumDeps(depType, depName, depPkg, depVer, summary, ok)) {
	do {
	    char buffer[512];

	    if (byProvider) {
		snprintf(buffer, sizeof(buffer), "%s: %s %s", 
			 depType, depPkg ? depPkg : depName, depVer);
	    } else {
		snprintf(buffer, sizeof(buffer), "%s: %s %s[%s]",
			 depType, depName, depVer,
			 depPkg ? depPkg : "-");
	    }

	    // check if this item is duplicated
	    bool dup = FALSE;
	    for (int i = 0; i < GTK_CLIST(_depList)->rows; i++) {
		char *str;
		
		gtk_clist_get_text(GTK_CLIST(_depList), i, 0, &str);
		if (strcmp(str, buffer) == 0) {
		    dup = TRUE;
		    break;
		}
	    }
	    
	    if (!dup) {
		char *array[1];
		int row;
		array[0] = buffer;
		row = gtk_clist_append(GTK_CLIST(_depList), array);
		//mvo: maybe use gtk_clist_set_background() instead?
 		gtk_clist_set_row_style(GTK_CLIST(_depList), row,
 					ok ? _blackStyle : _redStyle);
		gtk_clist_set_row_data(GTK_CLIST(_depList), row,
				       strdup(summary));
	    }
	    
	} while (pkg->nextDeps(depType, depName, depPkg, depVer, summary, ok));
    }

    // reverse dependencies
    gtk_clist_clear(GTK_CLIST(_rdepList));
    if (pkg->enumRDeps(depName, depPkg)) {
	do {
	    char buffer[512];
	    char *array[1];

	    snprintf(buffer, sizeof(buffer), "%s (%s)", depName, depPkg);
	    array[0] = buffer;
	    gtk_clist_append(GTK_CLIST(_rdepList), array);

	} while (pkg->nextRDeps(depName, depPkg));
    }

    // weak dependencies
    gtk_clist_clear(GTK_CLIST(_recList));
    gtk_object_set_data(GTK_OBJECT(_recList), "pkg", pkg);
    if (pkg->enumWDeps(depType, depPkg, ok)) {
	do {
	  char buffer[512];
	  char *array[1];
	  int row;

	  snprintf(buffer, sizeof(buffer), "%s: %s", 
		   depType, depPkg);
	  array[0] = buffer;
	  row = gtk_clist_append(GTK_CLIST(_recList), array);
	  gtk_clist_set_row_style(GTK_CLIST(_recList), row, 
				  ok ? _blackStyle : _redStyle);
	} while (pkg->nextWDeps(depName, depPkg, ok));
    }    

    // dependencies of the available package
    gtk_clist_clear(GTK_CLIST(_availDepList));
    gtk_label_set_text(GTK_LABEL(_availDepInfoL), "");    
    byProvider = TRUE;
    if (pkg->enumAvailDeps(depType, depName, depPkg, depVer, summary, ok)) {
	do {
	    char buffer[512];
	    if (byProvider) {
		snprintf(buffer, sizeof(buffer), "%s: %s %s", 
			 depType, depPkg ? depPkg : depName, depVer);
	    } else {
		snprintf(buffer, sizeof(buffer), "%s: %s %s[%s]",
			 depType, depName, depVer,
			 depPkg ? depPkg : "-");
	    }
	    // check if this item is duplicated
	    bool dup = FALSE;
	    for (int i = 0; i < GTK_CLIST(_availDepList)->rows; i++) {
		char *str;
		gtk_clist_get_text(GTK_CLIST(_availDepList), i, 0, &str);
		if (strcmp(str, buffer) == 0) {
		    dup = TRUE;
		    break;
		}
	    }
	    if (!dup) {
		char *array[1];
		int row;
		array[0] = buffer;
		row = gtk_clist_append(GTK_CLIST(_availDepList), array);
		//mvo: maybe use gtk_clist_set_background() instead?
		gtk_clist_set_row_style(GTK_CLIST(_availDepList), row,
					ok ? _blackStyle : _redStyle);
		gtk_clist_set_row_data(GTK_CLIST(_availDepList), row,
				       summary);
	    }
	} while (pkg->nextDeps(depType, depName, depPkg, depVer, summary, ok));
    }


// PORTME
#if 0
    if (pkg->getStatus() != RPackage::SNotInstalled)
	gtk_widget_set_sensitive(_removeDepsB, TRUE);
    else
	gtk_widget_set_sensitive(_removeDepsB, FALSE);
#endif
}


void RGMainWindow::updatePackageInfo(RPackage *pkg)
{
    char buffer[512] = "";
    char *bufPtr = (char*)buffer;
    unsigned bufSize = sizeof(buffer);
    long size;
    RPackage::UpdateImportance importance;
    RPackage::PackageStatus status;
    
    if (!pkg) {
	gtk_label_set_text(GTK_LABEL(_nameL), "");
	gtk_label_set_text(GTK_LABEL(_summL), "");
	gtk_label_set_text(GTK_LABEL(_infoL), "");
	gtk_label_set_text(GTK_LABEL(_stateL), "");
	gtk_label_set_text(GTK_LABEL(_stateL), "");
	gtk_image_set_from_pixmap(GTK_IMAGE(_stateP), 
				  StatusPixmaps[0],
				  StatusMasks[0]);
	if (_showUpdateInfo)
	    gtk_label_set_text(GTK_LABEL(_importL), "");

	gtk_widget_set_sensitive(_tabview, FALSE);
	
	return;
    }
    gtk_widget_set_sensitive(_tabview, TRUE);

    status = pkg->getStatus();

    if (status == RPackage::SNotInstalled) {
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(GTK_BIN(_actionB[1])->child),
			   _("_Install"));
    } else {
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(GTK_BIN(_actionB[1])->child),
			   _("_Upgrade"));
    }
    
    importance = pkg->updateImportance();
    
    // name/summary
    gtk_label_set_text(GTK_LABEL(_nameL), (char*)pkg->name());
    //gtk_label_set_text(GTK_LABEL(_summL), (char*)pkg->summary().c_str());
    gtk_label_set_text(GTK_LABEL(_summL), _iconv.convert(pkg->summary().c_str(),pkg->summary().size()));
    
    // package info
    
    // common information regardless of state    
    appendTag(bufPtr, bufSize, _("Section"), pkg->section());
    appendTag(bufPtr, bufSize, _("Priority"), pkg->priority());
    /*XXX
    appendTag(bufPtr, bufSize, "Maintainer", pkg->maintainer().c_str());
    appendTag(bufPtr, bufSize, "Vendor", pkg->vendor());
     */
    
    // installed version info
     
    appendText(bufPtr, bufSize, _("\nInstalled Package:\n"));

    appendTag(bufPtr, bufSize, _("    Version"), pkg->installedVersion());
    size = pkg->installedSize();
    appendTag(bufPtr, bufSize, _("    Size"),
	      size < 0 ?  _("N/A") : SizeToStr(size).c_str());

    
    appendText(bufPtr, bufSize, _("\nAvailable Package:\n"));
    
    appendTag(bufPtr, bufSize, _("    Version"),
	      pkg->availableDownloadableVersion());
    size = pkg->availableDownloadableSize();
    appendTag(bufPtr, bufSize, _("    Size"),
	      size < 0 ?  _("N/A") : SizeToStr(size).c_str());

    size = pkg->packageDownloadableSize();
    appendTag(bufPtr, bufSize, _("    Package Size"), 
	      size < 0 ?  _("N/A") : SizeToStr(size).c_str());

    gtk_label_set_text(GTK_LABEL(_infoL), buffer);

    if (_showUpdateInfo) {
	// importance of update
	char *text;
	
	bufPtr = (char*)buffer;
	bufSize = sizeof(buffer);
	*bufPtr = 0;
	
	switch (status) {
	 case RPackage::SNotInstalled:
	    text = _("Not Relevant");
	    break;
	 case RPackage::SInstalledBroken:
	    text = _("Unknown");
	    break;
	 default:
	    if (importance == RPackage::ISecurity ||
		importance == RPackage::ICritical)
		text = _("Recommended");
	    else
		text = _("Possible");
	    break;
	}
	appendTag(bufPtr, bufSize, _("Package Upgrade"), text);
	
	appendTag(bufPtr, bufSize, _("    Importance"),
		  _(ImportanceNames[(int)importance]));
	appendTag(bufPtr, bufSize, _("    Date"), pkg->updateDate());	
	gtk_label_set_text(GTK_LABEL(_importL), buffer);

	if (importance == RPackage::ISecurity) {
	    gtk_widget_show(_importP);
	} else {
	    gtk_widget_hide(_importP);
	}
    }    

    // update information
    if (_showUpdateInfo) {
//	const char *updateText = pkg->updateSummary();
	
    }
        
    // description
    //gtk_text_buffer_set_text(_textBuffer, pkg->description(), -1);
    gtk_text_buffer_set_text(_textBuffer, _iconv.convert(pkg->description(), strlen(pkg->description())), -1);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset(_textBuffer, &iter, 0);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(_textView), &iter,0,FALSE,0,0);
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
    RPackage *pkg;

    if(me->_blockActions)
      return;

    if (g_list_length(GTK_CLIST(me->_table)->selection) == 0)
	return;

    if (me->_unsavedChanges == true && 
	gtk_run_alert_panel(me->_win, _("Warning"),
			    _("There are unsaved changes.\n"
			      "Synaptic must reopen its cache.\n"
			      "Your changes will be lost. Are you sure?"),
			    _("Apply"), _("Cancel"), 
			    NULL) != GTK_ALERT_DEFAULT) 
      {
	me->_blockActions = TRUE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self), !active);
	me->_blockActions = FALSE;
	return;
      }

    GList *selection = g_list_copy(GTK_CLIST(me->_table)->selection);
    GList *it = g_list_first(selection);
    while(it!=NULL) {
      int row = GPOINTER_TO_INT(it->data);
      pkg = me->_lister->getElement(row);
      if (pkg == NULL)
	continue;

      // set pkg according to status
      pkg->setPinned(active);
      _roptions->setPackageLock(pkg->name(), active);
      it=g_list_next(it);
    }
    g_list_free(selection);
    me->setInterfaceLocked(TRUE);
    me->_lister->openCache(TRUE);
    me->refreshTable(pkg);
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

  // do the work!
  doPkgAction(me, (RGPkgAction)i);

  me->_currentB = clickedB;
}


void RGMainWindow::doPkgAction(RGMainWindow *me, RGPkgAction action)
{
  RPackage *pkg;

  if (g_list_length(GTK_CLIST(me->_table)->selection) == 0) 
    return;

  me->setInterfaceLocked(TRUE);
  me->_blockActions = TRUE;

  GList *selection = g_list_copy(GTK_CLIST(me->_table)->selection);
  GList *it = g_list_first(selection);
  while(it != NULL) {
    int row = GPOINTER_TO_INT(it->data);
    //cout << "row is " << row << endl;
    pkg = me->_lister->getElement(row);
    //cout << "working on: " << pkg->name() << endl;
    if (pkg == NULL)
      continue;

    switch (action) {
    case PKG_KEEP: // keep
      pkg->setKeep();
      break;
    case PKG_INSTALL: // install
      me->pkgInstallHelper(pkg);
      break;
    case PKG_DELETE: // delete
      me->pkgRemoveHelper(pkg);
      break;
    case PKG_PURGE:  // purge
      me->pkgRemoveHelper(pkg, true);
      break;
    default:
      cout <<"uh oh!!!!!!!!!"<<endl;
      break;
    }
  it=it->next;
  }
  g_list_free(selection);


  me->_blockActions = FALSE;
  me->setInterfaceLocked(FALSE);
}




void RGMainWindow::removeDepsClicked(GtkWidget *self, void *data)
{
    RGMainWindow *me = (RGMainWindow*)data;
    RPackage *pkg;
    
    pkg = me->selectedPackage();
    if (!pkg)
	return;
    
    me->setInterfaceLocked(TRUE);
    
    if (pkg->isImportant()) {
	int res;
	res = gtk_run_alert_panel(me->window(), _("Warning"),
			      _("Removing this package may render the system unusable.\n"
				"Are you sure you want to do that?"),
			      _("Cancel"), _("Remove"), NULL);
	if (res == GTK_ALERT_DEFAULT) {
	  me->_blockActions = TRUE;
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(me->_currentB), TRUE);
	  me->_blockActions = FALSE;

	} else {
	  pkg->setRemoveWithDeps(TRUE);
	}
    } else {
	pkg->setRemoveWithDeps(TRUE);
    }
    
    gtk_clist_select_row(GTK_CLIST(me->_table), 
			 me->_lister->getElementIndex(pkg), 0);
    
    me->setInterfaceLocked(FALSE);
}


void RGMainWindow::getColor(const char *cpp, GdkColor **colp){
   GdkColor *new_color;
   int result;

   // "" means no color
   if(strlen(cpp) == 0) {
     *colp = NULL;
     return;
   }
   
   GdkColormap *colormap = gdk_colormap_get_system ();

   new_color = g_new (GdkColor, 1);
   result = gdk_color_parse (cpp, new_color);
   gdk_colormap_alloc_color(colormap, new_color, FALSE, TRUE);
   *colp = new_color;
}


RGMainWindow::RGMainWindow(RPackageLister *packLister)
    : RGWindow("main", false, true), _lister(packLister), _iconv("UTF8")
{
#if !defined(DEBUGUI) || defined(HAVE_RPM)
    //_showUpdateInfo = true; // xxx conectiva only, for now
    _showUpdateInfo = false;
#else
    _showUpdateInfo = false;
#endif
    _blockActions = false;
    _unsavedChanges = false;
    _interfaceLocked = 0;
    _lister->registerObserver(this);
    _busyCursor = gdk_cursor_new(GDK_WATCH);
    _tooltips = gtk_tooltips_new();

    StatusPixmaps[(int)RPackage::MKeep] = 	
	gdk_pixmap_create_from_xpm_d(_win->window, 
				  &StatusMasks[(int)RPackage::MKeep],
				  NULL, keepM_xpm);
    
    StatusPixmaps[(int)RPackage::MInstall] = 
	gdk_pixmap_create_from_xpm_d(_win->window, 
				  &StatusMasks[(int)RPackage::MInstall],
				  NULL, installM_xpm);
    
    StatusPixmaps[(int)RPackage::MUpgrade] =
	gdk_pixmap_create_from_xpm_d(_win->window, 
				  &StatusMasks[(int)RPackage::MUpgrade],
				  NULL, upgradeM_xpm);

    StatusPixmaps[(int)RPackage::MDowngrade] =
	gdk_pixmap_create_from_xpm_d(_win->window, 
				  &StatusMasks[(int)RPackage::MDowngrade],
				  NULL, downgradeM_xpm);

    StatusPixmaps[(int)RPackage::MRemove] =
	gdk_pixmap_create_from_xpm_d(_win->window, 
				  &StatusMasks[(int)RPackage::MRemove],
				  NULL, removeM_xpm);

    // don't upgrade package 
    StatusPixmaps[(int)RPackage::MHeld] =
	gdk_pixmap_create_from_xpm_d(_win->window, 
				  &StatusMasks[(int)RPackage::MHeld],
				  NULL, heldM_xpm);
    // broken
    StatusPixmaps[(int)RPackage::MBroken] =
	gdk_pixmap_create_from_xpm_d(_win->window, 
				  &StatusMasks[(int)RPackage::MBroken],
				  NULL, brokenM_xpm);
    // pin = use pining to prevent upgrade
    StatusPixmaps[(int)RPackage::MPinned] =
	gdk_pixmap_create_from_xpm_d(_win->window, 
				     &StatusMasks[(int)RPackage::MPinned],
				     NULL, pinM_xpm);
    // new
    StatusPixmaps[(int)RPackage::MNew] =
	gdk_pixmap_create_from_xpm_d(_win->window, 
				     &StatusMasks[(int)RPackage::MNew],
				     NULL, newM_xpm);


    StatusPixmaps[10] = _alertPix;

    _toolbarState = (ToolbarState)_config->FindI("Synaptic::ToolbarState",
						(int)TOOLBAR_BOTH);
    
    // get some color values
    if(_config->FindB("Synaptic::UseStatusColors", TRUE)) {
      getColor(_config->Find("Synaptic::MKeepColor", "").c_str(),
	       &StatusColors[(int)RPackage::MKeep]);
      getColor(_config->Find("Synaptic::MInstallColor", "#ccffcc").c_str(),
	       &StatusColors[(int)RPackage::MInstall]); 
      getColor(_config->Find("Synaptic::MUpgradeColor", "#ffff00").c_str(),
	       &StatusColors[(int)RPackage::MUpgrade]);
      getColor(_config->Find("Synaptic::MDowngradeColor", "").c_str(),
	       &StatusColors[(int)RPackage::MDowngrade]);
      getColor(_config->Find("Synaptic::MRemoveColor", "#f44e80").c_str(),
	       &StatusColors[(int)RPackage::MRemove]);
      getColor(_config->Find("Synaptic::MHeldColor", "").c_str(),
	       &StatusColors[(int)RPackage::MHeld]);
      getColor(_config->Find("Synaptic::MBrokenColor", "#e00000").c_str(),
	       &StatusColors[(int)RPackage::MBroken/*broken*/]);
      getColor(_config->Find("Synaptic::MPinColor", "#ccccff").c_str(),
	       &StatusColors[(int)RPackage::MPinned/*hold=pinned*/]);
      getColor(_config->Find("Synaptic::MNewColor", "#ffffaa").c_str(),
	       &StatusColors[(int)RPackage::MNew/*new*/]);
    }

    buildInterface();
    
    refreshFilterMenu();

    _userDialog = new RGUserDialog();

    packLister->setUserDialog(_userDialog);
    
    packLister->setProgressMeter(_cacheProgress);

    _filterWin = NULL;
    _findWin = NULL;
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
}


void RGMainWindow::searchPkgAction(void *self, RGFindWindow *findWin)
{
  RGMainWindow *me = (RGMainWindow*)self;
  RFilter *filter;
    
  filter = me->_lister->findFilter(0);
  filter->reset();
  filter->pattern.addPattern(me->_findWin->getSearchType(),
			     me->_findWin->getFindString(),
			     FALSE);
  me->changeFilter(1);

  findWin->hide();
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

//   if(me->_toolbarState == GPOINTER_TO_INT(data))
//     return;
  
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

void RGMainWindow::searchPkgNameClicked(GtkWidget *self, void *data)
{
  RGMainWindow *me = (RGMainWindow*)data;
  
  if(me->_findWin == NULL) {
    me->_findWin = new RGFindWindow(me);
    me->_findWin->setFindCallback(searchPkgAction, me);
  }

  me->_findWin->setSearchType(RPatternPackageFilter::Name);
  me->_findWin->show();
}

void RGMainWindow::searchPkgDescriptionClicked(GtkWidget *self, void *data)
{
  RGMainWindow *me = (RGMainWindow*)data;
  
  if(me->_findWin == NULL) {
    me->_findWin = new RGFindWindow(me);
    me->_findWin->setFindCallback(searchPkgAction, me);
  }

  me->_findWin->setSearchType(RPatternPackageFilter::Description);
  me->_findWin->show();
}

void RGMainWindow::buildInterface()
{
    GtkWidget *button;
    GtkWidget *widget;
    GtkWidget *item;
    PangoFontDescription *font;
    PangoFontDescription *bfont;

    // here is a pointer to rgmainwindow for every widget that needs it
    g_object_set_data(G_OBJECT(_win), "me", this);

    _alertPix = gdk_pixmap_create_from_xpm_d(_win->window, &_alertMask,
					     NULL, alert_xpm);

    GdkPixbuf *icon = gdk_pixbuf_new_from_xpm_data(synaptic_mini_xpm);
    gtk_window_set_icon(GTK_WINDOW(_win), icon);

    font = pango_font_description_from_string ("helvetica 10");
    bfont = pango_font_description_from_string ("helvetica bold 12");    

    gtk_window_resize(GTK_WINDOW(_win),
 				_config->FindI("Synaptic::windowWidth", 640),
 				_config->FindI("Synaptic::windowHeight", 480));
    RGFlushInterface();


    glade_xml_signal_connect_data(_gladeXML,
				  "on_about_activate",
				  G_CALLBACK(showAboutPanel),
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
				  "on_search_description",
				  G_CALLBACK(searchPkgDescriptionClicked),
				  this); 

    glade_xml_signal_connect_data(_gladeXML,
				  "on_search_name",
				  G_CALLBACK(searchPkgNameClicked),
				  this); 



    // workaround for a bug in libglade
    button = glade_xml_get_widget(_gladeXML, "button_update");
    gtk_tooltips_set_tip(GTK_TOOLTIPS (_tooltips), button,
			 _("Update package cache"),"");
			 
    button = glade_xml_get_widget(_gladeXML, "button_upgrade");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(_tooltips), button,
			 _("Mark packages for upgrade, except those with new dependencies"),"");
    
    button = glade_xml_get_widget(_gladeXML, "button_dist_upgrade");
    gtk_tooltips_set_tip(GTK_TOOLTIPS (_tooltips), button,
			 _("Upgrade installed distribution, including packages with new dependencies"),"");
    
    button = glade_xml_get_widget(_gladeXML, "button_procceed");
    gtk_tooltips_set_tip(GTK_TOOLTIPS (_tooltips), button,
			 _("Commit (and download if necessary) selected changes on packages"),"");

    
    _nameL = glade_xml_get_widget(_gladeXML, "label_pkgname"); 
    assert(_nameL);
    gtk_widget_modify_font(_nameL, bfont);
    _summL = glade_xml_get_widget(_gladeXML, "label_summary"); 
    assert(_summL);
    gtk_widget_modify_font(_summL, font);

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
    gtk_widget_hide(glade_xml_get_widget(_gladeXML, "hseparator_hold"));
#endif

    // only for debian 
    _pkgHelp = glade_xml_get_widget(_gladeXML, "button_pkghelp");
    assert(_pkgHelp);

    if(!FileExists("/usr/bin/dwww"))
      gtk_widget_hide(_pkgHelp);
    
    _tabview = glade_xml_get_widget(_gladeXML, "notebook_info");
    assert(_tabview);

    _vpaned =  glade_xml_get_widget(_gladeXML, "vpaned_main");
    assert(_vpaned);
    gtk_paned_set_position(GTK_PANED(_vpaned), 
 			   _config->FindI("Synaptic::vpanedPos", 140));
    RGFlushInterface();

    _stateP = GTK_IMAGE(glade_xml_get_widget(_gladeXML, "image_state"));
    assert(_stateP);
    _stateL = glade_xml_get_widget(_gladeXML, "label_state");
    assert(_stateL);
    gtk_misc_set_alignment(GTK_MISC(_stateL), 0.0, 0.0);
    _infoL =  glade_xml_get_widget(_gladeXML, "label_info");
    assert(_infoL);
    gtk_misc_set_alignment(GTK_MISC(_infoL), 0.0, 0.0);
    gtk_label_set_justify(GTK_LABEL(_infoL), GTK_JUSTIFY_LEFT);

    _textView = glade_xml_get_widget(_gladeXML, "text_descr");
    assert(_textView);
    _textBuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (_textView));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(_textView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_textView), GTK_WRAP_WORD);

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

#ifndef HAVE_RPM
    item = glade_xml_get_widget(_gladeXML, "menu_suggested");
    assert(item);
    gtk_object_set_data(GTK_OBJECT(item), "index", 
			(void*)WHAT_IT_SUGGESTS);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
		       (GtkSignalFunc)changedDepView, this);
#endif
    gtk_option_menu_set_history(GTK_OPTION_MENU(_depP), 0);

    _depTab = glade_xml_get_widget(_gladeXML, "notebook_dep_tab");
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(_depTab), FALSE);
    assert(_depTab);
    _depList = glade_xml_get_widget(_gladeXML, "clist_deplist");
    assert(_depList);
    gtk_object_set_data(GTK_OBJECT(_depList), "me", this);
    glade_xml_signal_connect(_gladeXML, 
			     "on_clist_deplist_select_row",
			     G_CALLBACK(clickedDepList));

    _depInfoL = glade_xml_get_widget(_gladeXML, "label_dep_info");
    assert(_depInfoL);
    _rdepList = glade_xml_get_widget(_gladeXML, "clist_rdeps");
    assert(_rdepList);
    _availDepList = glade_xml_get_widget(_gladeXML, "clist_availdep_list");
    assert(_availDepList);
    gtk_object_set_data(GTK_OBJECT(_availDepList), "me", this);
    gtk_signal_connect(GTK_OBJECT(_availDepList), "select_row",
		       (GtkSignalFunc)clickedAvailDepList, (void*)1);
    gtk_signal_connect(GTK_OBJECT(_availDepList), "unselect_row",
		       (GtkSignalFunc)clickedAvailDepList, (void*)0);
    _availDepInfoL = glade_xml_get_widget(_gladeXML, "label_availdep_info");
    assert(_availDepInfoL);
    _recList = glade_xml_get_widget(_gladeXML, "clist_rec_list");
    assert(_recList);

    _filtersB = button = glade_xml_get_widget(_gladeXML, "button_filters");
    assert(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       (GtkSignalFunc)showFilterManagerWindow, this);

    _filterPopup = glade_xml_get_widget(_gladeXML, "optionmenu_filters");
    assert(_filterPopup);
    _filterMenu = glade_xml_get_widget(_gladeXML, "menu_apply");
    assert(_filterMenu);

    _editFilterB = button = glade_xml_get_widget(_gladeXML, "button_edit_filter");
    assert(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       (GtkSignalFunc)showFilterWindow, this);
    _findText = glade_xml_get_widget(_gladeXML, "entry_find");
    assert(_findText);
    gtk_object_set_data(GTK_OBJECT(_findText), "me", this);
    gtk_signal_connect(GTK_OBJECT(_findText), "changed",
		       (GtkSignalFunc)findPackageObserver, this);
    gtk_signal_connect(GTK_OBJECT(_findText), "activate",
		       (GtkSignalFunc)findPackageObserver, this);

    _findNextB = glade_xml_get_widget(_gladeXML, "button_find_next");
    assert(_findNextB);
    gtk_signal_connect(GTK_OBJECT(_findNextB), "clicked",
		       (GtkSignalFunc)findNextAction, this);
    button = glade_xml_get_widget(_gladeXML, "button_to_filter");
    assert(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)makeFinderFilterAction, this);

    _table = glade_xml_get_widget(_gladeXML, "_table");
    assert(_table);
    gtk_clist_set_selection_mode(GTK_CLIST(_table),
				 GTK_SELECTION_MULTIPLE);
    gtk_signal_connect(GTK_OBJECT(_table), "select_row", 
		       (GtkSignalFunc)selectedClistRow, this);
    gtk_signal_connect(GTK_OBJECT(_table), "unselect_row",
		       (GtkSignalFunc)unselectedClistRow, this);
    gtk_clist_set_column_width(GTK_CLIST(_table), 0, 15);
    gtk_clist_set_column_width(GTK_CLIST(_table), 1, 140);
    gtk_clist_set_column_width(GTK_CLIST(_table), 2, 80);
    gtk_clist_set_column_width(GTK_CLIST(_table), 3, 80);
    gtk_clist_set_column_width(GTK_CLIST(_table), 4, 600);
    gtk_object_set_data(GTK_OBJECT(_table), "me", this);

    // toolbar menu code
    button = glade_xml_get_widget(_gladeXML, "menu_toolbar_pixmaps");
    g_object_set_data(G_OBJECT(button), "me", this);
    g_signal_connect(G_OBJECT(button), 
		       "activate",
		       G_CALLBACK(menuToolbarClicked), 
		       GINT_TO_POINTER(TOOLBAR_PIXMAPS)); 
    if(_toolbarState == TOOLBAR_PIXMAPS)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));


    button = glade_xml_get_widget(_gladeXML, "menu_toolbar_text");
    g_object_set_data(G_OBJECT(button), "me", this);
    g_signal_connect(G_OBJECT(button), 
		       "activate",
		       G_CALLBACK(menuToolbarClicked), 
		       GINT_TO_POINTER(TOOLBAR_TEXT)); 
    if(_toolbarState == TOOLBAR_TEXT)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));


    button = glade_xml_get_widget(_gladeXML, "menu_toolbar_both");
    g_object_set_data(G_OBJECT(button), "me", this);
    g_signal_connect(G_OBJECT(button), 
		       "activate",
		       G_CALLBACK(menuToolbarClicked), 
		       GINT_TO_POINTER(TOOLBAR_BOTH)); 
    if(_toolbarState == TOOLBAR_BOTH)
      gtk_menu_item_activate(GTK_MENU_ITEM(button));


    button = glade_xml_get_widget(_gladeXML, "menu_toolbar_hide");
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

void RGMainWindow::pkgInstallHelper(RPackage *pkg)
{
  pkg->setInstall();
  // check whether something broke
  if (!this->_lister->check()) {
    this->_lister->fixBroken();
    this->refreshTable(pkg);
  }
}

void RGMainWindow::pkgRemoveHelper(RPackage *pkg, bool purge)
{
  if (pkg->isImportant()) {
    int res = gtk_run_alert_panel(this->window(), 
				  _("Warning"),
				  _("Removing this package may render the "
				    "system unusable.\n"
				    "Are you sure you want to do that?"),
				  _("Cancel"), _("Remove"), NULL);
    // default is "Cancel"
    if (res == GTK_ALERT_DEFAULT) {
      _blockActions = TRUE;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(this->_currentB),
				   TRUE);
      _blockActions = FALSE;
      return;
    } 
  }
  pkg->setRemove(purge);
}

void RGMainWindow::selectedClistRow(GtkWidget *self, int row, int column,
				    GdkEvent *event)
{ 
  //cout << "selectedClistRow(): " << row << endl;
  
  RGMainWindow *me = (RGMainWindow*)gtk_object_get_data(GTK_OBJECT(self),
							  "me");
  RPackage *pkg = me->_lister->getElement(row);
  if (pkg == NULL)
    return;    

  if ( event != NULL && event->type == 5 ) {
    // double click
    me->setInterfaceLocked(TRUE);

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
	me->pkgInstallHelper(pkg);
      }
      if (mstatus == RPackage::MInstall) 
	// marked install -> marked don't install
	me->pkgRemoveHelper(pkg);
    }
    
    if( pstatus == RPackage::SInstalledOutdated ) {
      if ( mstatus == RPackage::MKeep ) {
	// keep -> upgrade
	me->pkgInstallHelper(pkg);
      }
      if( mstatus == RPackage::MUpgrade) {
	// upgrade -> keep
	pkg->setKeep();
      }
    }
    // end double-click
    me->setInterfaceLocked(FALSE);
    return;
  }
  
  // not double-click
  me->updatePackageInfo(pkg);
}

void RGMainWindow::unselectedClistRow(GtkWidget *self, int row, int column,
				      GdkEvent *event)
{
    RGMainWindow *me = (RGMainWindow*)gtk_object_get_data(GTK_OBJECT(self),
							  "me");
    
    if(g_list_length(GTK_CLIST(me->_table)->selection) != 0) {
      int row = GPOINTER_TO_INT(g_list_last(GTK_CLIST(me->_table)->selection)->data);
      RPackage *pkg = me->_lister->getElement(row);
      me->updatePackageInfo(pkg);
    } else 
      me->updatePackageInfo(NULL);
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
	char buffer[256];

	listed = _lister->count();
	snprintf(buffer, sizeof(buffer), 
		 _("%i packages listed, %i installed, %i broken. %i to install/upgrade, %i to remove; %sB will be %s"),
		 listed, installed, broken, toinstall, toremove,
		 SizeToStr(fabs(size)).c_str(),
		 size < 0 ? _("freed") : _("used"));
	
	gtk_label_set_text(GTK_LABEL(_statusL), buffer);
    }
    
    gtk_widget_set_sensitive(_upgradeB, _lister->upgradable() );
    gtk_widget_set_sensitive(_upgradeM, _lister->upgradable() );
    gtk_widget_set_sensitive(_distUpgradeB, _lister->upgradable() );
    gtk_widget_set_sensitive(_distUpgradeM, _lister->upgradable() );

    gtk_widget_set_sensitive(_proceedB, (toinstall + toremove) != 0);
    gtk_widget_set_sensitive(_proceedM, (toinstall + toremove) != 0);
    _unsavedChanges = (toinstall + toremove) != 0;
    
    gtk_widget_queue_draw(_statusL);
}


void RGMainWindow::refreshFilterMenu()
{
  GtkWidget *menu, *item;
  vector<string> filters;
  _lister->getFilterNames(filters);
  int i;

  gtk_option_menu_remove_menu(GTK_OPTION_MENU(_filterPopup));
  gtk_menu_item_remove_submenu(GTK_MENU_ITEM(_filterMenu));

  menu = gtk_menu_new();
  gtk_widget_ref(menu); //importend! remove this -> random segfaults
     
  item = gtk_menu_item_new_with_label(_("All Packages"));
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  gtk_object_set_data(GTK_OBJECT(item), "me", this);
  gtk_object_set_data(GTK_OBJECT(item), "index", (void*)0);
  gtk_signal_connect(GTK_OBJECT(item), "activate", 
		     (GtkSignalFunc)changedFilter, this);
  
  i = 1;
  for (vector<string>::const_iterator iter = filters.begin();
       iter != filters.end();
       iter++) {
    
    item = gtk_menu_item_new_with_label((char*)(*iter).c_str());
    gtk_widget_show(item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_object_set_data(GTK_OBJECT(item), "me", this);
    gtk_object_set_data(GTK_OBJECT(item), "index", (void*)i++);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
		       (GtkSignalFunc)changedFilter, this);
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(_filterPopup), menu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(_filterMenu), menu);
}


void RGMainWindow::saveState()
{
    _lister->storeFilters();
    _config->Set("Synaptic::vpanedPos",
		 gtk_paned_get_position(GTK_PANED(_vpaned)));
    _config->Set("Synaptic::windowWidth", _win->allocation.width);
    _config->Set("Synaptic::windowHeight", _win->allocation.height);
    _config->Set("Synaptic::ToolbarState", (int)_toolbarState);
    if (!RWriteConfigFile(*_config)) {
      _error->DumpErrors();
      gtk_run_alert_panel(_win,
			  _("Error"), 
			  _("An error occurred while saving configurations."),
			  _("OK"), NULL, NULL);
    }
    if(!_roptions->store())
      cerr << "error storing raptoptions" << endl;
}


bool RGMainWindow::restoreState()
{
  _lister->restoreFilters();

  refreshFilterMenu();
  
  int filterNr = _config->FindI("synaptic_nosave::initialFilter", 0);
  changeFilter(filterNr);
  refreshTable(NULL);

  setStatusText();
  return true;
}


void RGMainWindow::close()
{
    if (_interfaceLocked > 0)
	return;
    
    if (_unsavedChanges == false || 
	gtk_run_alert_panel(_win, _("Warning"),
			    _("There are unsaved changes, are you sure\n"
			      "you want to quit Synaptic?"),
			    _("Quit"), _("Cancel"), NULL) == GTK_ALERT_DEFAULT) {
	
	_error->Discard();
	
	saveState();
	showErrors();

	exit(0);
    }
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
  while (gtk_events_pending())
    gtk_main_iteration();
}
