/* rwmainwindow.cc - main window of the app
 * 
 * Copyright (c) 2000-2003 Conectiva S/A 
 *               2002 Michael Vogt <mvo@debian.org>
 * 
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <mvo@debian.org>
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
#include <cmath>

#include <apt-pkg/strutl.h>
#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>

#include <X11/cursorfont.h>
#include <X11/keysym.h>

//#include "raptoptions.h"

#include "rwmainwindow.h"

#include "rpackagefilter.h"


#include "rwfiltermanager.h"
#include "rwfilterwindow.h"
#include "rwsourceswindow.h"
#include "rwconfigwindow.h"
#include "rwaboutpanel.h"
#include "rwsummarywindow.h"

#include "rwfetchprogress.h"
#include "rwcacheprogress.h"
#include "rwuserdialog.h"
#include "rwinstallprogress.h"
#include "rwdummyinstallprogress.h"

#include <WINGs/wtabledelegates.h>


// icons and pixmaps

#include "logo.xpm"

#include "proceed.xpm"
#include "options.xpm"
#include "upgrade.xpm"
#include "distupgrade.xpm"
#include "update.xpm"
#include "sources.xpm"
#include "fixbroken.xpm"

#include "alert.xpm"

#include "brokenM.xpm"
#include "downgradeM.xpm"
#include "heldM.xpm"
#include "installM.xpm"
#include "removeM.xpm"
#include "upgradeM.xpm"


#include "find.xpm"
#include "filter.xpm"


extern void RWFlushInterface();


static char *ImportanceNames[] = {
    N_("Unknown"),
    N_("Normal"),
    N_("Critical"),
    N_("Security")
};


static WMPixmap *StatusPixmaps[10];



// list columns
enum {
    CStatus,
	CSection,
	CName,
	CInstalled,
	CNewest,
	CSummary
};


int RWMainWindow::numberOfRows(WMTableViewDelegate *self, WMTableView *table)
{
    RWMainWindow *me = (RWMainWindow*)WMGetTableViewDataSource(table);

    return me->_lister->packagesSize();
}


// TODO: add displayPackages set like in (rgmainwindow) 
void *RWMainWindow::valueForCell(WMTableViewDelegate *self,
				 WMTableColumn *column, int row)
{
    WMTableView *table = (WMTableView*)WMGetTableColumnTableView(column);
    RWMainWindow *me = (RWMainWindow*)WMGetTableViewDataSource(table);
    RPackage *elem = me->_lister->getPackage(row);
    const char *text = NULL;
    static char buffer[256];
    static void *data[2];
    
    assert(elem);
    
    switch ((int)WMGetTableColumnId(column)) {
     case CStatus:
	{
	    int s = elem->getFlags();
	    if (elem->wouldBreak())
		return StatusPixmaps[7];

	    if ((s & RPackage::FKeep) 
		&& (s & RPackage::FOutdated)) {
	         return StatusPixmaps[/*RPackage::FHeld*/0];
	    }

	    return StatusPixmaps[/*s*/0];
	}
     case CSection:
	text = elem->section();
	break;
	
     case CName:
	text = elem->name();
	break;
	
     case CInstalled:
	text = elem->installedVersion();
	if (!text)
	    text = "";
	break;

     case CNewest:
	 text = elem->availableVersion();
	 if (!text)
	     text = "";
	break;
	
     case CSummary:
	strncpy(buffer, elem->summary(), sizeof(buffer));	
	data[0] = (void*)buffer;
	data[1] = NULL;
	
	if (me->_showUpdateInfo && 
	    elem->updateImportance() == RPackage::ISecurity)
	    data[1] = (void*)StatusPixmaps[8];
	return data;

     default:
	text = "";
	break;
    }
    
    return (void*)text;
}


static void splitViewConstrain(WMSplitView *split, int index,
			       int *minSize, int *maxSize)
{
    switch (index) {
     case 1:
	*minSize = 400;
	break;
     case 0:
	*minSize = 150;
	break;
    }
}


void RWMainWindow::changedDepView(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    int index = WMGetPopUpButtonSelectedItem((WMPopUpButton*)self);
    
    WMSelectTabViewItemAtIndex(me->_depTab, index);
}


void RWMainWindow::clickedDepList(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    WMListItem *item = WMGetListSelectedItem(me->_depList);

    WMSetLabelText(me->_depInfoL, strchr(item->text, '\001')+1);
}


void RWMainWindow::showAboutPanel(WMWidget *self, void *data)
{
    RWMainWindow *win = (RWMainWindow*)data;
    
    if (win->_aboutPanel == NULL)
	win->_aboutPanel = new RWAboutPanel(win);
    win->_aboutPanel->show();
}


void RWMainWindow::findPackageObserver(void *self, WMNotification *notif)
{
    RWMainWindow *me = (RWMainWindow*)self;
    char *text = WMGetTextFieldText(me->_findText);
    int row;

    if (WMGetNotificationName(notif) == WMTextDidEndEditingNotification
	&& (int)WMGetNotificationClientData(notif) != WMReturnTextMovement)
	return;

    row = me->_lister->findPackage(text);
    if (row >= 0) {
	WMSelectTableViewRow(me->_table, row);
	WMScrollTableViewRowToVisible(me->_table, row);
	me->setStatusText();
    } else {
	me->setStatusText(_("No match."));
    }
}


void RWMainWindow::findNextAction(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    
    int row = me->_lister->findNextPackage();
    if (row >= 0) {
	WMSelectTableViewRow(me->_table, row);
	WMScrollTableViewRowToVisible(me->_table, row);
	me->setStatusText();
    } else {
	me->setStatusText(_("No more matches."));
    }
}


void RWMainWindow::makeFinderFilterAction(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    RFilter *filter;
    
    filter = me->_lister->findFilter(0);

    char *pattern = WMGetTextFieldText(me->_findText);

    if (pattern && *pattern) {
	filter->reset();
	filter->pattern.addPattern(RPatternPackageFilter::Name,
				   string(pattern), false);
	
	WMSetPopUpButtonSelectedItem(me->_popup, 1);
	changedFilter(me->_popup, me);
    }
}


void RWMainWindow::saveFilterAction(void *self, RWFilterWindow *rwin)
{
    RWMainWindow *me = (RWMainWindow*)self;
    RPackage *pkg = me->selectedPackage();
    Bool filterEditable;
    
    filterEditable = WMGetPopUpButtonSelectedItem(me->_popup) != 0;    
    
    WMSetButtonEnabled(me->_filtersB, True);
    WMSetButtonEnabled(me->_editFilterB, filterEditable);
    WMSetPopUpButtonEnabled(me->_popup, True);

    me->_lister->reapplyFilter();
    me->setStatusText();

    me->refreshTable(pkg);
    
    rwin->hide();
}



void RWMainWindow::closeFilterAction(void *self, RWFilterWindow *rwin)
{
    RWMainWindow *me = (RWMainWindow*)self;
    //RPackage *pkg = me->selectedPackage();
    Bool filterEditable;
    
    filterEditable = WMGetPopUpButtonSelectedItem(me->_popup) != 0;

    WMSetButtonEnabled(me->_editFilterB, filterEditable);
    WMSetButtonEnabled(me->_filtersB, True);
    WMSetPopUpButtonEnabled(me->_popup, True);

    rwin->hide();
}


void RWMainWindow::showFilterWindow(WMWidget *self, void *data)
{
    RWMainWindow *win = (RWMainWindow*)data;

    if (win->_filterWin == NULL) {
	win->_filterWin = new RWFilterWindow(win, win->_lister);

	win->_filterWin->setSaveCallback(saveFilterAction, win);
	win->_filterWin->setCloseCallback(closeFilterAction, win);
    }
    
    if (win->_lister->getFilter() == 0)
	return;
    
    win->_filterWin->editFilter(win->_lister->getFilter());
    
    WMSetButtonEnabled(win->_editFilterB, False);
    WMSetButtonEnabled(win->_filtersB, False);
    WMSetPopUpButtonEnabled(win->_popup, False);
}


void RWMainWindow::closeFilterManagerAction(void *self, 
					    RWFilterManagerWindow *win)
{
    RWMainWindow *me = (RWMainWindow*)self;
    Bool filterEditable;

    // select 0, because they may have deleted the current filter
    WMSetPopUpButtonSelectedItem(me->_popup, 0);
    changedFilter(me->_popup, me);

    me->refreshFilterMenu();
    
    filterEditable = WMGetPopUpButtonSelectedItem(me->_popup) != 0;
    
    WMSetButtonEnabled(me->_editFilterB, filterEditable);
    WMSetButtonEnabled(me->_filtersB, True);
    WMSetPopUpButtonEnabled(me->_popup, True);
}


void RWMainWindow::showFilterManagerWindow(WMWidget *self, void *data)
{
    RWMainWindow *win = (RWMainWindow*)data;

    if (win->_fmanagerWin == NULL) {
	win->_fmanagerWin = new RWFilterManagerWindow(win, win->_lister);

	win->_fmanagerWin->setCloseCallback(closeFilterManagerAction, win);
    }

    win->_fmanagerWin->show();

    WMSetButtonEnabled(win->_filtersB, False);
    WMSetButtonEnabled(win->_editFilterB, False);
    WMSetPopUpButtonEnabled(win->_popup, False);
}


void RWMainWindow::showSourcesWindow(WMWidget *self, void *data)
{
    RWMainWindow *win = (RWMainWindow*)data;
    
    if (win->_sourcesWin == NULL)
	win->_sourcesWin = new RWSourcesWindow(win);
    win->_sourcesWin->show();
}



void RWMainWindow::showConfigWindow(WMWidget *self, void *data)
{
    RWMainWindow *win = (RWMainWindow*)data;
    
    if (win->_configWin == NULL)
	win->_configWin = new RWConfigWindow(win);
    win->_configWin->show();
}



void RWMainWindow::changedFilter(WMWidget *self, void *data)
{
    int filter = WMGetPopUpButtonSelectedItem((WMPopUpButton*)self);
    RWMainWindow *mainw = (RWMainWindow*)data;
    RPackage *pkg = mainw->selectedPackage();
    //int i;
    
    mainw->setInterfaceLocked(true);
    
    if (filter == 0) { // no filter
	mainw->_lister->setFilter();
	
	WMSetButtonEnabled(mainw->_editFilterB, False);
    } else {
	mainw->_lister->setFilter(filter-1);
	
	WMSetButtonEnabled(mainw->_editFilterB, True);
    }

    mainw->refreshTable(pkg);

    int index = -1;

    if (pkg) {	
         index = mainw->_lister->getPackageIndex(pkg);
    }    
    
    if (index >= 0)
	WMScrollTableViewRowToVisible(mainw->_table, index);
    else
	mainw->updatePackageInfo(NULL);

    mainw->setInterfaceLocked(false);
    
    mainw->setStatusText();
}


void RWMainWindow::switchCommandPanel(WMWidget *self, void *data)
{
    RWMainWindow *mainw = (RWMainWindow*)data;
    
    if (mainw->_panelSwitched) {
	WMSelectPreviousTabViewItem(mainw->_cmdPanel);
	mainw->_panelSwitched = false;
    } else {
	WMSelectNextTabViewItem(mainw->_cmdPanel);
	mainw->_panelSwitched = true;	
    }
}


void RWMainWindow::updateClicked(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    RPackage *pkg = me->selectedPackage();

    
    // need to delete dialogs, as they might have data pointing
    // to old stuff
    delete me->_filterWin;
    delete me->_fmanagerWin;
    me->_fmanagerWin = NULL;
    me->_filterWin = NULL;

    
    RWFetchProgress *progress = new RWFetchProgress(me);
    progress->setTitle(_("Retrieving Index Files"));


    me->setStatusText(_("Updating Package Lists from Servers..."));

    me->setInterfaceLocked(true);

    if (!me->_lister->updateCache(progress)) {
	me->showErrors();
    }

    delete progress;

    if (me->_lister->openCache(true)) {
	me->showErrors();
    }

    me->refreshTable(pkg);
    
    me->setInterfaceLocked(false);
    
    me->setStatusText();
}


RPackage *RWMainWindow::selectedPackage()
{
    int index;
    WMArray *array;
    
    array = WMGetTableViewSelectedRows(_table);
    
    if (WMGetArrayItemCount(array) != 1)
	return NULL;
    
    index = (int)WMGetFromArray(array, 0);
    
    if (index >= 0)
	return _lister->getPackage(index);
    else
	return NULL;
}


void RWMainWindow::fixBrokenClicked(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    RPackage *pkg = me->selectedPackage();
    
    bool res = me->_lister->fixBroken();

    me->setInterfaceLocked(true);
    
    me->refreshTable(pkg);
    
    if (!res)
	me->setStatusText(_("Dependency problem resolver failed."));
    else
	me->setStatusText(_("Dependency problems successfully fixed."));
    
    me->setInterfaceLocked(false);

    me->showErrors();
}


void RWMainWindow::upgradeClicked(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    RPackage *pkg = me->selectedPackage();
    bool res;

    if (!me->_lister->check()) {
	WMRunAlertPanel(WMWidgetScreen(self), me->window(),
			_("Error"), _("Automatic upgrade selection not possible\n"
			"with broken packages. Please fix them first."),
			_("Ok"), NULL, NULL);
	return;
    }
    
    me->setInterfaceLocked(true);
    
    me->setStatusText(_("Performing automatic selection of upgradadable packages..."));

    res = me->_lister->upgrade();
    
    me->refreshTable(pkg);
    
    if (res)
	me->setStatusText(_("Automatic selection of upgradadable packages done."));
    else
	me->setStatusText(_("Automatic upgrade selection failed."));
    
    me->setInterfaceLocked(false);
    
    me->showErrors();
}



void RWMainWindow::distUpgradeClicked(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    RPackage *pkg = me->selectedPackage();
    bool res;


    if (!me->_lister->check()) {
	WMRunAlertPanel(WMWidgetScreen(self), me->window(),
			_("Error"), _("Automatic upgrade selection not possible\n"
			"with broken packages. Please fix them first."),
			_("Ok"), NULL, NULL);
	return;
    }

    me->setInterfaceLocked(true);
    
    me->setStatusText(_("Performing selection for distribution upgrade..."));

    res = me->_lister->distUpgrade();

    me->refreshTable(pkg);

    if (res)
	me->setStatusText(_("Selection for distribution upgrade done."));
    else
	me->setStatusText(_("Selection for distribution upgrade failed."));

    me->setInterfaceLocked(false);

    me->showErrors();
}



void RWMainWindow::proceedClicked(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    RPackage *pkg = me->selectedPackage();
    RWSummaryWindow *summ;

    // check whether we can really do it
    if (!me->_lister->check()) {
	WMRunAlertPanel(WMWidgetScreen(self), me->window(), _("Error"), 
			_("Operation not possible with broken packages.\n"
			"Please fix them first."),
			_("Ok"), NULL, NULL);
	return;
    }
    
    // show a summary of what's gonna happen
    summ = new RWSummaryWindow(me, me->_lister);
    if (!summ->showAndConfirm()) {
	// canceled operation
	delete summ;
	return;
    }
    delete summ;

    me->setInterfaceLocked(true);
    me->updatePackageInfo(NULL);

    me->setStatusText(_("Performing selected changes... See the terminal from where you started Synaptic for more information."));

    // fetch packages
    RWFetchProgress *fprogress = new RWFetchProgress(me);
    fprogress->setTitle(_("Retrieving Package Files"));
    
#ifdef HAVE_RPM
    RWInstallProgress *iprogress = new RWInstallProgress(me);
#else
    RWDummyInstallProgress *iprogress = 
	new RWDummyInstallProgress(WMWidgetScreen(me->_win));
#endif
    
    //bool result = me->_lister->commitChanges(fprogress, iprogress);
    me->_lister->commitChanges(fprogress, iprogress);
    
    delete fprogress;
    delete iprogress;

    me->showErrors();

    if (_config->FindB("Synaptic::Download-Only", false) == false) {    
	// reset the cache
	if (!me->_lister->openCache(true)) {
	    me->showErrors();
	    exit(1);
	}
    }
    
    me->refreshTable(pkg);

    me->setInterfaceLocked(false);
    
    me->setStatusText();
}


bool RWMainWindow::showErrors()
{
    WMGenericPanel *panel;
    WMScreen *scr = WMWidgetScreen(_win);
    WMScrollView *sbox;
    WMLabel *label;
    string message;
    WMFont *font;
    unsigned int widest = 0;
    int lines;
    char *type;
    int theight;
    
    if (_error->empty())
	return false;
    
    if (_error->PendingError())
	type = _("Error");
    else
	type = _("Warning");
    
    panel = WMCreateGenericPanel(scr, _win, type, _("OK"), NULL);

    sbox = WMCreateScrollView(panel->content);
    WMMapWidget(sbox);
    WMSetViewExpandsToParent(WMWidgetView(sbox), 10, 0, 10, 0);
    WMSetScrollViewRelief(sbox, WRSunken);

    label = WMCreateLabel(sbox);
    WMSetLabelWraps(label, True);
    WMSetScrollViewContentView(sbox, WMWidgetView(label));
    WMMapWidget(label);

    font = WMSystemFontOfSize(scr, 12);
    WMSetLabelFont(label, font);
    
    WMRealizeWidget(sbox);
    
    lines = 0;
    message = "";
    while (!_error->empty()) {
	string tmp;

	_error->PopMessage(tmp);
       
        // ignore some stupid error messages
	if (tmp == _("Tried to dequeue a fetching object"))
	   continue;

	if (message.empty())
	    message = tmp;
	else
	    message = message + "\n\n" + tmp;
    }
 
    {
	const char *b, *e = message.c_str();
	unsigned int w;

	while (e) {
	    b = e + 1;
	    e = strchr(b, '\n');
	    if (e) {
		w = WMWidthOfString(font, (char*)b, e-b);		
		if (w > widest)
		    widest = w;
		if (w > WMWidgetWidth(sbox) - 25)
		    lines += w / (WMWidgetWidth(sbox) - 25);
	    }
	    lines++;
	}
	w = WMWidthOfString(font, (char*)b, strlen(b));
	if (w > widest)
	    widest = w;
	if (w > WMWidgetWidth(sbox) - 25)
	    lines += w / (WMWidgetWidth(sbox) - 25);
    }

    WMSetLabelText(label, (char*)message.c_str());

    theight = lines * WMFontHeight(font);
    
    WMResizeWidget(label, WMWidgetWidth(sbox) - 25, theight + 10);
    
    if (theight > 40) {
	int dy;
	
	dy = WMWidgetHeight(panel->win) - WMWidgetHeight(panel->content);
	
	WMResizeWidget(panel->win, WMWidgetWidth(panel->win),
		       WMIN(theight + dy + 20, 340));
	if (theight + dy + 20 > 340) {
	    WMSetScrollViewHasVerticalScroller(sbox, True);
	    WMSetScrollViewRelief(sbox, WRSunken);
	}
    }
    
    WMMapWidget(panel->win);
    WMRunModalLoop(scr, WMWidgetView(panel->win));
    WMDestroyGenericPanel(panel);
    
    return true;
}


static WMButton *makeButton(WMWidget *parent, char *label, 
			    char **image, WMFont *font)
{
    WMButton *button;
    WMPixmap *pix;
    
    button = WMCreateCommandButton(parent);

    WMSetButtonFont(button, font);    
    WMSetButtonText(button, label);
    
    if (image) {
	pix = WMCreatePixmapFromXPMData(WMWidgetScreen(parent), image);
	WMSetButtonImagePosition(button, WIPAbove);
	WMSetButtonImage(button, pix);
	WMReleasePixmap(pix);
    }

    return button;
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


void RWMainWindow::clickedRow(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    int row = WMGetTableViewClickedRow((WMTableView*)self);
    
    if (me->_tempKluge)
	return;
    
    if (row < 0) {
	me->updatePackageInfo(NULL);
	return;
    }
    RPackage *pkg = me->_lister->getPackage(row);
    if (pkg == NULL)
	return;    
    
    me->updatePackageInfo(pkg);
}


void RWMainWindow::notifyChange(RPackage *pkg)
{
    // we changed pkg's status
    updateDynPackageInfo(pkg);

    refreshTable(pkg);

    setStatusText();
}


void RWMainWindow::refreshTable(RPackage *selectedPkg)
{    
  static bool first_run = true;
    _tempKluge = true;

    WMReloadTableView(_table);

    _tempKluge = false;

    if (selectedPkg != NULL) {
	int index = _lister->getPackageIndex(selectedPkg);
    
	if (index >= 0)
	    WMSelectTableViewRow(_table, index);
    }
    if(first_run)
      first_run=false;

}


void RWMainWindow::updatePackageStatus(RPackage *pkg)
{
    bool installed = false;
    int flags = pkg->getFlags();
//    bool locked = _config->getPackageLock(pkg->name());

//    WMSetButtonSelected(_holdB, locked ? True : False);
    
    WMSetButtonEnabled(_actionB[0], True);
    
    if(RPackage::FInstalled & flags) {
	WMSetButtonEnabled(_actionB[1], False);
	WMSetButtonEnabled(_actionB[2], True);
	installed = true;
    } else if(RPackage::FOutdated & flags) {
	WMSetButtonEnabled(_actionB[1], True);
	WMSetButtonEnabled(_actionB[2], True);
	installed = true;
    } else if(RPackage::FInstBroken & flags) {
	WMSetButtonEnabled(_actionB[1], False);
	WMSetButtonEnabled(_actionB[2], True);
	installed = true;
    } else  {
	WMSetButtonEnabled(_actionB[1], True);
	WMSetButtonEnabled(_actionB[2], False);
    }
    
    _currentB = NULL;
    if((RPackage::FKeep & flags) || (RPackage::FHeld & flags)) {
	_currentB = _actionB[0];
	if (installed) 
	    WMSetLabelText(_stateL, _("Package is installed."));
	else
	    WMSetLabelText(_stateL, _("Package is not installed."));
    } else if(RPackage::FInstall & flags) {
	WMSetLabelText(_stateL, _("Package will be installed."));
	_currentB = _actionB[1];
    } else if(RPackage::FUpgrade & flags) {
	_currentB = _actionB[1];
	WMSetLabelText(_stateL, _("Package will be upgraded."));
    } else if(RPackage::FDowngrade & flags) {
	// not handled yet
	puts(_("OH SHIT!!"));
    } else if(RPackage::FRemove & flags) {
	_currentB = _actionB[2];
	WMSetLabelText(_stateL, _("Package will be uninstalled."));
    }
    
    if (pkg->wouldBreak()) {
	WMSetLabelText(_stateL, _("Broken dependencies."));
	
	WMSetLabelImage(_stateL, StatusPixmaps[7]);
    } else {
       if ((flags & RPackage::FKeep) && (flags & RPackage::FOutdated)) {
	  WMSetLabelImage(_stateL, StatusPixmaps[0/*RPackage::FHeld*/]);
	} else
	   WMSetLabelImage(_stateL, StatusPixmaps[0/*flags*/]);
    }
    
    WMSetButtonSelected(_currentB, True);
}


void RWMainWindow::navigateTable(char direction)
{
    WMArray *array = WMGetTableViewSelectedRows(_table);
    unsigned int index;

    if (WMGetArrayItemCount(array) == 0)
	index = 0;
    else
	index = (int)WMGetFromArray(array, 0);
    
    switch (direction) {
     case 'u':
	index = index-1;
	if (index < 0)
	    index = 0;
	break;
     case 'd':
	index = index+1;
	if (index >= _lister->packagesSize())
	    index = _lister->packagesSize()-1;
	break;
     case 'n':
	index += 10;
	if (index >= _lister->packagesSize())
	    index = _lister->packagesSize()-1;
	break;
     case 'p':
	index -= 10;
	if (index < 0)
	    index = 0;
	break;
     case 'h':
	index = 0;
	break;
     case 'e':
	index = _lister->packagesSize()-1;
	break;
     default:
	assert(0);
    }

    WMSelectTableViewRow(_table, index);
    WMScrollTableViewRowToVisible(_table, index);
}


void RWMainWindow::keyPressHandler(XEvent *event, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    //Display *dpy = event->xany.display;
    char buffer[32];
    int count;
    KeySym ksym;
    
    if (event->type == KeyRelease)
	return;

    buffer[0] = 0;
    count = XLookupString(&event->xkey, buffer, sizeof(buffer), &ksym, NULL);
    
    if (event->xkey.state & ControlMask) {
	switch (ksym) {
	 case XK_k: // keep
	    WMPerformButtonClick(me->_actionB[0]);
	    break;
	 case XK_i: // install
	    WMPerformButtonClick(me->_actionB[1]);
	    break;
	 case XK_r: // remove
	    WMPerformButtonClick(me->_actionB[2]);
	    break;
	 case XK_g: // find next
	    WMPerformButtonClick(me->_findNextB);
	    break;
	 case XK_p: // proceed
	    WMPerformButtonClick(me->_proceedB);
	    break;
	}
    } else {
	switch (ksym) {
	 case XK_Up:
	    me->navigateTable('u');
	    break;
	 case XK_Down:
	    me->navigateTable('d');
	    break;
	 case XK_Next:
	    me->navigateTable('n');
	    break;
	 case XK_Prior:
	    me->navigateTable('p');
	    break;
	 case XK_Home:
	    me->navigateTable('h');
	    break;
	 case XK_End:
	    me->navigateTable('e');
	    break;
	}
    }
}


void RWMainWindow::updateDynPackageInfo(RPackage *pkg)
{
    updatePackageStatus(pkg);

    // dependencies
    WMSetLabelText(_depInfoL, "");    
    
    WMClearList(_depList);
    
    bool byProvider = true;
    
    const char *depType, *depPkg, *depName, *depVer;
    char *summary;
    bool ok;
    if (pkg->enumDeps(depType, depName, depPkg, depVer, summary, ok)) {
	do {
	    char buffer[512];
	    
	    if (byProvider) {
		snprintf(buffer, sizeof(buffer), "%c%s: %s %s%c%s", ok ? '+' : '*',
			 depType, depPkg ? depPkg : depName, depVer,
			 '\001', summary); // kluge
	    } else {
		snprintf(buffer, sizeof(buffer), "%c%s: %s %s[%s]%c%s", ok ? '+' : '*',
			 depType, depName, depVer,
			 depPkg ? depPkg : "-",
			 '\001', summary); // kluge
	    }
	    
	    // check if this item is duplicated
	    bool dup = false;
	    for (int i = 0; i < WMGetListNumberOfRows(_depList); i++) {
		char *str;
		
		str = WMGetListItem(_depList, i)->text;
		if (strcmp(str, buffer) == 0) {
		    dup = true;
		    break;
		}
	    }
	    
	    if (!dup)
		WMAddListItem(_depList, buffer);
	    
	} while (pkg->nextDeps(depType, depName, depPkg, depVer, summary, ok));
    }

    // reverse dependencies

    WMClearList(_rdepList);

    if (pkg->enumRDeps(depName, depPkg)) {
	do {
	    char buffer[512];

	    snprintf(buffer, sizeof(buffer), "%s (%s)", depName, depPkg);
	    WMAddListItem(_rdepList, buffer);

	} while (pkg->nextRDeps(depName, depPkg));
    }

    // weak dependencies

    WMClearList(_recList);

    if (pkg->enumWDeps(depType, depPkg, ok)) {
	do {
	    char buffer[512];

	    snprintf(buffer, sizeof(buffer), "%c%s: %s", ok ? '+' : '*',
		     depType, depPkg);
	    WMAddListItem(_recList, buffer);

	} while (pkg->nextWDeps(depName, depPkg, ok));
    }    
    
    int flags = pkg->getFlags();
    if (!((flags & RPackage::FInstalled) || (flags & RPackage::FOutdated))) 
	WMSetButtonEnabled(_removeDepsB, True);
    else
	WMSetButtonEnabled(_removeDepsB, False);
}


void RWMainWindow::updatePackageInfo(RPackage *pkg)
{
    char buffer[512] = "";
    char *bufPtr = (char*)buffer;
    unsigned bufSize = sizeof(buffer);
    long size;
    RPackage::UpdateImportance importance;
    int flags;
    
    if (!pkg) {
	WMSetLabelText(_nameL, NULL);
	WMSetLabelText(_summL, NULL);
	WMSetLabelText(_infoL, NULL);
	WMSetLabelText(_stateL, NULL);
	WMSetLabelImage(_stateL, NULL);
	if (_showUpdateInfo)
	    WMSetLabelText(_importL, NULL);

	WMSelectTabViewItemAtIndex(_tabview, 0);
	WMSetTabViewEnabled(_tabview, False);
	
	return;
    }

    WMSetTabViewEnabled(_tabview, True);

    flags = pkg->getFlags();


    if (!((flags&RPackage::FInstalled)||(flags&RPackage::FOutdated))) {
	WMSetButtonText(_actionB[1], _("Install"));
    } else {
	WMSetButtonText(_actionB[1], _("Upgrade"));
    }
    
    importance = pkg->updateImportance();
    
    // name/summary
    WMSetLabelText(_nameL, (char*)pkg->name());
    WMSetLabelText(_summL, (char*)pkg->summary());
    
    // package info
    
    // common information regardless of state    
    appendTag(bufPtr, bufSize, _("Section"), pkg->section());
    appendTag(bufPtr, bufSize, _("Priority"), pkg->priority());
    /*XXX
    appendTag(bufPtr, bufSize, _("Maintainer"), pkg->maintainer().c_str());
    appendTag(bufPtr, bufSize, _("Vendor"), pkg->vendor());
     */
    
    // installed version info
     
    appendText(bufPtr, bufSize, _("\nInstalled Package:\n"));

    appendTag(bufPtr, bufSize, _("    Version"), pkg->installedVersion());
    size = pkg->installedSize();
    appendTag(bufPtr, bufSize, _("    Size"),
	      size < 0 ?  _("N/A") : SizeToStr(size).c_str());

    
    appendText(bufPtr, bufSize, _("\nAvailable Package:\n"));
    
    
    appendTag(bufPtr, bufSize, _("    Version"), pkg->availableVersion());
    size = pkg->availableInstalledSize();
    appendTag(bufPtr, bufSize, _("    Size"),
	      size < 0 ?  _("N/A") : SizeToStr(size).c_str());

    size = pkg->availablePackageSize();
    appendTag(bufPtr, bufSize, _("    Package Size"), 
	      size < 0 ?  _("N/A") : SizeToStr(size).c_str());

    WMSetLabelText(_infoL, buffer);
#if 0
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
	WMSetLabelText(_importL, buffer);

	if (importance == RPackage::ISecurity) {
	    WMSetLabelImage(_importL, _alertPix);
	} else {
	    WMSetLabelImage(_importL, NULL);
	}
    }    

    // update information
    if (_showUpdateInfo) {
//	const char *updateText = pkg->updateSummary();
	
    }
#endif
        
    // description
    void *tmp;
    while ((tmp = WMRemoveTextBlock(_text)) != NULL)
	WMDestroyTextBlock(_text, tmp);
    
    WMAppendTextStream(_text, (char*)pkg->description());
    WMRedisplayWidget(_text);

    
    updateDynPackageInfo(pkg);
    
    setStatusText();
}



void RWMainWindow::holdClicked(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    int row = WMGetTableViewClickedRow(me->_table);
    
    if (row < 0)
	return;
    RPackage *pkg = me->_lister->getPackage(row);
    if (pkg == NULL)
	return;
/*XXX    
    if (WMGetButtonSelected((WMButton*)self)) {
	_options->setPackageLock(pkg->name(), true);
	pkg->setHeld(true);
    } else {
	_options->setPackageLock(pkg->name(), false);
	pkg->setHeld(false);
    }
 */
}


void RWMainWindow::actionClicked(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    WMButton *clickedB = (WMButton*)self;
    int row = WMGetTableViewClickedRow(me->_table);
    int i;

    if (me->_currentB == clickedB)
	return;
    
    if (row < 0)
	return;
    RPackage *pkg = me->_lister->getPackage(row);
    if (pkg == NULL)
	return;

    for (i = 0; i < 3 && me->_actionB[i] != clickedB; i++);

    me->setInterfaceLocked(true);
    
    switch (i) {
     case 0: // keep
	pkg->setKeep();
	break;
     case 1: // install
	pkg->setInstall();
	// check whether something broke
	if (!me->_lister->check()) {
	    me->_lister->fixBroken();
	    me->refreshTable(pkg);
	}
	break;
     case 2: // remove
	if (pkg->getFlags()&RPackage::FImportant) {
	    int res;
	    res = WMRunAlertPanel(WMWidgetScreen(self), me->window(), _("Warning"),
				  _("Removing this package may render the system unusable.\n"
				  "Are you sure you want to do that?"),
				  _("Cancel"), _("Remove"), NULL);
	    if (res == WAPRDefault) {
		clickedB = me->_currentB;
		WMSetButtonSelected(me->_currentB, True);
	    } else {
		pkg->setRemove();
	    }
	} else {
	    pkg->setRemove();
	}
	break;
    }
    me->_currentB = clickedB;
    
    me->setInterfaceLocked(false);
}



void RWMainWindow::removeDepsClicked(WMWidget *self, void *data)
{
    RWMainWindow *me = (RWMainWindow*)data;
    RPackage *pkg;
    
    pkg = me->selectedPackage();
    if (!pkg)
	return;
    
    me->setInterfaceLocked(true);
    
    if (pkg->getFlags() & RPackage::FImportant) {
	int res;
	res = WMRunAlertPanel(WMWidgetScreen(self), me->window(), _("Warning"),
			      _("Removing this package may render the system unusable.\n"
				"Are you sure you want to do that?"),
			      _("Cancel"), _("Remove"), NULL);
	if (res == WAPRDefault) {
	    WMSetButtonSelected(me->_currentB, True);
	} else {
	    pkg->setRemoveWithDeps(true);
	}
    } else {
	pkg->setRemoveWithDeps(true);
    }
    
    WMSelectTableViewRow(me->_table, me->_lister->getPackageIndex(pkg));
    
    me->setInterfaceLocked(false);
}


void RWMainWindow::dependListDraw(WMList *lPtr, int index, Drawable d, 
				  char *text, int state, WMRect *rect)
{
    WMScreen *scr = WMWidgetScreen(lPtr);
    Display *dpy = WMScreenDisplay(scr);
    static WMColor *yellow = NULL, *red = NULL;
    WMColor *color;
    static WMFont *font = NULL;
    
    if (!font) {
	font = WMSystemFontOfSize(scr, 12);
    }
    
    if (state & WLDSSelected) {
	XFillRectangle(dpy, d, WMColorGC(WMWhiteColor(scr)),
		       rect->pos.x, rect->pos.y, 
		       rect->size.width, rect->size.height);
    } else {
	XClearArea(dpy, d, rect->pos.x, rect->pos.y, 
		   rect->size.width, rect->size.height, False);
    }
    
    switch (*text) {
     case '+':
	color = WMBlackColor(scr);
	break;

     case '-':
	if (!yellow)
	    yellow = WMCreateNamedColor(scr, "yellow3", False);
	color = yellow;
	break;

     case '*':
	if (!red)
	    red = WMCreateNamedColor(scr, "red3", False);
	color = red;
	break;	
    }

    GC gc = WMColorGC(color);
    WMDrawString(scr, d, gc, font, rect->pos.x+4, rect->pos.y, 
		text+1, strchr(text, '\001') - (text + 1));
}



RWMainWindow::RWMainWindow(WMScreen *scr, RPackageLister *packLister)
    : RWWindow(scr, "main"), _lister(packLister)
{
#if !defined(DEBUGUI) || defined(HAVE_RPM)
    _showUpdateInfo = true; // xxx conectiva only, for now
#else
    _showUpdateInfo = false;
#endif
    _tempKluge = false;
    
    _unsavedChanges = false;
    
    _interfaceLocked = 0;
    _lockWindow = None;

    _lister->registerObserver(this);

    _busyCursor = XCreateFontCursor(WMScreenDisplay(scr), XC_watch);
    
    buildInterface(scr);
    
    refreshFilterMenu();

    _userDialog = new RWUserDialog(scr);

    packLister->setUserDialog(_userDialog);
    
    packLister->setProgressMeter(_cacheProgress);

    _filterWin = NULL;
    _sourcesWin = NULL;
    _configWin = NULL;
    _aboutPanel = NULL;
    _fmanagerWin = NULL;    
    
#if 0  // PORTME
    StatusPixmaps[(int)RPackage::MKeep] = NULL;
    StatusPixmaps[(int)RPackage::MInstall] = 
	WMCreatePixmapFromXPMData(scr, (char**)installM_xpm);
    StatusPixmaps[(int)RPackage::MUpgrade] =
	WMCreatePixmapFromXPMData(scr, (char**)upgradeM_xpm);
    StatusPixmaps[(int)RPackage::MDowngrade] =
	WMCreatePixmapFromXPMData(scr,  (char**)downgradeM_xpm);
    StatusPixmaps[(int)RPackage::MRemove] =
	WMCreatePixmapFromXPMData(scr,  (char**)removeM_xpm);
    StatusPixmaps[(int)RPackage::MHeld] =
	WMCreatePixmapFromXPMData(scr,  (char**)heldM_xpm);
    StatusPixmaps[7] = 
	WMCreatePixmapFromXPMData(scr,  (char**)brokenM_xpm);
    StatusPixmaps[8] = _alertPix;
#else
    StatusPixmaps[0] = NULL;
    StatusPixmaps[1] = NULL;
    StatusPixmaps[2] = NULL;
    StatusPixmaps[3] = NULL;
    StatusPixmaps[4] = NULL;
    StatusPixmaps[5] = NULL;
    StatusPixmaps[6] = NULL;
    StatusPixmaps[7] = NULL;
    StatusPixmaps[8] = NULL;
#endif

}



void RWMainWindow::buildInterface(WMScreen *scr)
{
    WMSplitView *hsplit;
    WMBox *hbox, *vbox;
    WMBox *ubox;
    WMButton *button;
    WMFont *font;
    WMFont *bfont;
    bool largeBtns = !_config->FindB("Synaptic::TextButtons", false);
    static WMTableViewDelegate delegate = {
	NULL,
	    numberOfRows,
	    valueForCell,
	    NULL
    };
    
    WMCreateEventHandler(WMWidgetView(_win), KeyPressMask|KeyReleaseMask,
			 keyPressHandler, this);
    
    
    _alertPix = WMCreatePixmapFromXPMData(scr,  (char**)alert_xpm);

    font = WMSystemFontOfSize(scr, 10);
    bfont = WMBoldSystemFontOfSize(scr, 12);
    
    
    WMSetWindowMinSize(_win, 550, 360);

    WMResizeWidget(_win, 640, 480);

    WMSetBoxBorderWidth(_topBox, 0);

    ubox = WMCreateBox(_topBox);
    WMMapWidget(ubox);
    WMSetBoxBorderWidth(ubox, 5);
    WMAddBoxSubview(_topBox, WMWidgetView(ubox), True, True, 64, 0, 0);
    
    // top button bar
    hbox = WMCreateBox(ubox);
    WMSetBoxHorizontal(hbox, True);
    WMAddBoxSubview(ubox, WMWidgetView(hbox), False, True, 
		    largeBtns ? 64 : 20, 0, 10);



    int buttonWidth = largeBtns ? 70 : 80;

    // about
    button = WMCreateCommandButton(hbox);
    WMSetButtonBordered(button, False);
    WMSetButtonFont(button, font);    
    WMSetButtonText(button, _("About"));
    WMAddBoxSubviewAtEnd(hbox, WMWidgetView(button), False, True, 
			 buttonWidth, 0, 5);
    WMSetButtonAction(button, showAboutPanel, this);
    if (largeBtns) {
	WMPixmap *pix = WMCreatePixmapFromXPMData(scr, logo_xpm);
	WMSetButtonImagePosition(button, WIPImageOnly);
	WMSetButtonImage(button, pix);
    }

    button = makeButton(hbox, _("Update"), largeBtns ? update_xpm : NULL, font);    
    WMSetButtonAction(button, updateClicked, this);
    WMSetBalloonTextForView(_("Update package cache"), 
			    WMWidgetView(button));
    WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 
		    buttonWidth, 0, 10);

    button = makeButton(hbox, _("Fix Broken"), largeBtns ? fixbroken_xpm : NULL, font);
    WMSetButtonAction(button, fixBrokenClicked, this);
    WMSetBalloonTextForView(_("Fix dependency problems"), 
			    WMWidgetView(button));
    WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 
		    buttonWidth, 0, 10);
    
    button = makeButton(hbox, _("Upgrade"), largeBtns ? upgrade_xpm : NULL, font);
    WMSetButtonAction(button, upgradeClicked, this);
    WMSetBalloonTextForView(_("Mark packages for upgrade, except those with new dependencies"),
			    WMWidgetView(button));
    
    WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 
		    buttonWidth, 0, 5);
    
    button = makeButton(hbox, _("DistUpgrade"), largeBtns ? distupgrade_xpm : NULL, font);
    WMSetButtonAction(button, distUpgradeClicked, this);
    WMSetBalloonTextForView(_("Upgrade installed distribution, including packages with new dependencies"),
			    WMWidgetView(button));
    
    WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 
		    buttonWidth, 0, 10);
    
    button = makeButton(hbox, _("Proceed!"), largeBtns ? proceed_xpm : NULL, font);
    WMSetButtonAction(button, proceedClicked, this);
    WMSetBalloonTextForView(_("Commit (and download if necessary) selected changes on packages"),
			    WMWidgetView(button));
    WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 
		    buttonWidth, 0, 5);
    _proceedB = button;

        
            
    button = makeButton(hbox, _("Options"), largeBtns ? options_xpm : NULL, font);
    WMSetButtonAction(button, showConfigWindow, this);
    WMSetBalloonTextForView(_("Configure Synaptic options"),
			    WMWidgetView(button));
    WMAddBoxSubviewAtEnd(hbox, WMWidgetView(button), False, True, 
			 buttonWidth, 0, 5);

//     button = makeButton(hbox, _("Repositories"), largeBtns ? sources_xpm : NULL, font);
//     WMSetButtonAction(button, showSourcesWindow, this);
//    WMSetBalloonTextForView(_("Manage package repositories"),
// 		    WMWidgetView(button));
//     WMAddBoxSubviewAtEnd(hbox, WMWidgetView(button), False, True,
// 		 buttonWidth, 0, 5); 

    WMReleaseFont(font);
    
    WMMapSubwidgets(hbox);
    WMMapWidget(hbox);

    // splitview
    hsplit = WMCreateSplitView(ubox);
    WMMapWidget(hsplit);
    WMAddBoxSubview(ubox, WMWidgetView(hsplit), True, True, 200, 0, 0);
    WMSetSplitViewVertical(hsplit, True);
    WMSetSplitViewConstrainProc(hsplit, splitViewConstrain);
    

    // right part
    vbox = WMCreateBox(_win);
    WMMapWidget(vbox);
    WMSetBoxHorizontal(vbox, False);
    WMAddSplitViewSubview(hsplit, WMWidgetView(vbox));


    _nameL = WMCreateLabel(vbox);
    WMAddBoxSubview(vbox, WMWidgetView(_nameL), False, True, 20, 0, 0);
    WMSetLabelFont(_nameL, bfont);
    WMSetLabelTextAlignment(_nameL, WACenter);


    _summL = WMCreateLabel(vbox);
    WMSetLabelFont(_summL, font);
    WMSetLabelWraps(_summL, True);
    WMAddBoxSubview(vbox, WMWidgetView(_summL), False, True, 25, 0, 5);
    WMSetLabelTextAlignment(_summL, WACenter);
    
    hbox = WMCreateBox(vbox);
    WMSetBoxHorizontal(hbox, True);
    WMAddBoxSubview(vbox, WMWidgetView(hbox), False, True, 20, 0, 5);
    
    _actionB[0] = button = WMCreateButton(hbox, WBTPushOnPushOff);
    WMSetButtonText(button, _("Keep"));
    WMSetButtonAction(button, actionClicked, this);
    WMSetBalloonTextForView(_("Keep selected package as is in the system, unmarking selections over it"),
			    WMWidgetView(button));
    WMSetButtonImagePosition(button, WIPLeft);
    WMAddBoxSubview(hbox, WMWidgetView(button), True, True, 30, 0, 2);
    
    _actionB[1] = button = WMCreateButton(hbox, WBTPushOnPushOff);
    WMSetButtonText(button, _("Install"));
    WMSetButtonAction(button, actionClicked, this);
    WMSetBalloonTextForView(_("Mark selected package for installation/upgrade"),
			    WMWidgetView(button));
    WMAddBoxSubview(hbox, WMWidgetView(button), True, True, 30, 0, 2);
    WMGroupButtons(_actionB[0], _actionB[1]);    
    
    _actionB[2] = button = WMCreateButton(hbox, WBTPushOnPushOff);
    WMSetButtonText(button, _("Remove"));
    WMSetBalloonTextForView(_("Mark selected package for removal from system"),
			    WMWidgetView(button));
    WMSetButtonAction(button, actionClicked, this);
    WMAddBoxSubview(hbox, WMWidgetView(button), True, True, 30, 0, 5);
    WMGroupButtons(_actionB[0], _actionB[2]);
    
    WMMapSubwidgets(hbox);
    WMMapWidget(hbox);

    _tabview = WMCreateTabView(vbox);
    WMAddBoxSubview(vbox, WMWidgetView(_tabview), True, True, 50, 0, 0);

    WMBox *infoBox = WMCreateBox(_win);
    WMSetBoxBorderWidth(infoBox, 10);
    WMSetBoxHorizontal(infoBox, False);
    WMMapWidget(infoBox);
    WMAddTabViewItemWithView(_tabview, WMWidgetView(infoBox), 0, _("General"));

    _stateL = WMCreateLabel(infoBox);
    WMMapWidget(_stateL);
    WMSetLabelText(_stateL, NULL);
    WMSetLabelImagePosition(_stateL, WIPLeft);
    WMAddBoxSubview(infoBox, WMWidgetView(_stateL), False, True, 14, 0, 0);
 
    _infoL = WMCreateLabel(infoBox);
    WMAddBoxSubview(infoBox, WMWidgetView(_infoL), False, True, 162, 0, 10);
    WMMapWidget(_infoL);

    
    if (_showUpdateInfo) {
	_importL = WMCreateLabel(infoBox);
	WMSetLabelImagePosition(_importL, WIPRight);
	WMAddBoxSubview(infoBox, WMWidgetView(_importL), False, True, 45, 0, 5);
	WMMapWidget(_importL);
    }

    _holdB = button = WMCreateSwitchButton(infoBox);
    WMSetButtonEnabled(button, False);
//    WMMapWidget(button);
    WMSetButtonText(button, _("No automatic upgrade (hold)"));
    WMSetButtonAction(button, holdClicked, this);
    WMAddBoxSubviewAtEnd(infoBox, WMWidgetView(button), False, True, 18, 0, 0);


    _text = WMCreateText(_win);
    WMSetViewNextResponder(WMWidgetView(_text), WMWidgetView(_win));
    WMSetTextEditable(_text, False);
    WMSetTextHasVerticalScroller(_text, True);
    WMAddTabViewItemWithView(_tabview, WMWidgetView(_text), 1, _("Descr."));

    {
	WMBox *vb = WMCreateBox(_win);
	WMBox *vb2;
	
	WMMapWidget(vb);
	
	WMSetBoxBorderWidth(vb, 5);
	WMSetBoxHorizontal(vb, False);

	WMAddTabViewItemWithView(_tabview, WMWidgetView(vb), 2, _("Depends."));

	_depP = WMCreatePopUpButton(vb);
	WMSetPopUpButtonAction(_depP, changedDepView, this);
	WMMapWidget(_depP);
	WMAddBoxSubview(vb, WMWidgetView(_depP), False, True, 20, 0, 5);
	WMAddPopUpButtonItem(_depP, _("What it depends on"));
	WMAddPopUpButtonItem(_depP, _("What depends on it"));
#ifndef HAVE_RPM
	WMAddPopUpButtonItem(_depP, _("Suggested and Recommended"));
#endif
	WMSetPopUpButtonSelectedItem(_depP, 0);

	_depTab = WMCreateTabView(vb);
	WMMapWidget(_depTab);
	WMSetTabViewType(_depTab, WTNoTabsNoBorder);
	WMAddBoxSubview(vb, WMWidgetView(_depTab), True, True, 100, 0, 0);

	// dependencies

	vb2 = WMCreateBox(vb);
	
	_depList = WMCreateList(vb2);
	WMSetListUserDrawProc(_depList, dependListDraw);
	WMSetListAction(_depList, clickedDepList, this);
	WMAddBoxSubview(vb2, WMWidgetView(_depList), True, True, 100, 0, 5);

	_depInfoL = WMCreateLabel(vb2);
	WMAddBoxSubview(vb2, WMWidgetView(_depInfoL), False, True, 20, 0, 0);

	_removeDepsB = button = WMCreateCommandButton(vb2);
	WMSetButtonText(button, _("Remove With Dependencies"));
	WMSetButtonAction(button, removeDepsClicked, this);
	WMSetBalloonTextForView(_("Remove all packages that this one depends on"),
				WMWidgetView(button));
	WMAddBoxSubview(vb2, WMWidgetView(button), False, True, 20, 0, 0);

	WMMapSubwidgets(vb2);
	
	WMAddTabViewItemWithView(_depTab, WMWidgetView(vb2), 0, NULL);

	// reverse dependencies
	
	vb2 = WMCreateBox(vb);
	
	_rdepList = WMCreateList(vb2);
	WMAddBoxSubview(vb2, WMWidgetView(_rdepList), True, True, 100, 0, 0);

	WMMapSubwidgets(vb2);

	WMAddTabViewItemWithView(_depTab, WMWidgetView(vb2), 1, NULL);

	// suggests and recommends
	vb2 = WMCreateBox(vb);
	_recList = WMCreateList(vb2);
	WMAddBoxSubview(vb2, WMWidgetView(_recList), True, True, 100, 0, 5);
	
	WMPopUpButton *pop;
	pop = WMCreatePopUpButton(vb2);
	WMSetPopUpButtonText(pop, _("Commands"));
	WMSetPopUpButtonPullsDown(pop, True);
	WMAddPopUpButtonItem(pop, _("Install Recommended"));
	WMAddPopUpButtonItem(pop, _("Install Suggested"));	
	WMAddPopUpButtonItem(pop, _("Install Selected"));

	WMAddBoxSubview(vb2, WMWidgetView(pop), False, True, 20, 0, 0);

	WMMapSubwidgets(vb2);

	WMAddTabViewItemWithView(_depTab, WMWidgetView(vb2), 2, NULL);
    }

    
    // left part
    {
	WMTableColumn *col;
	WMTableColumnDelegate *colDeleg;

	vbox = WMCreateBox(hsplit);
	WMSetBoxHorizontal(vbox, False);
	WMAddSplitViewSubview(hsplit, WMWidgetView(vbox));

	_panelSwitched = false;

	_cmdPanel = WMCreateTabView(vbox);
	WMMapWidget(_cmdPanel);
	WMSetTabViewType(_cmdPanel, WTNoTabsNoBorder);
	WMAddBoxSubview(vbox, WMWidgetView(_cmdPanel), False, True, 20, 0, 5);
	
	// filter stuff	
	hbox = WMCreateBox(_cmdPanel);
	WMMapWidget(hbox);
	WMSetBoxHorizontal(hbox, True);
	WMAddTabViewItemWithView(_cmdPanel, WMWidgetView(hbox), 0, NULL);
	

	button = makeButton(hbox, NULL, find_xpm, font);
	WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 20, 0, 5);
	WMSetBalloonTextForView(_("Switch to package search panel"),
				WMWidgetView(button));	
	WMSetButtonAction(button, switchCommandPanel, this);

	_filtersB = button = WMCreateCommandButton(hbox);
	WMSetButtonText(button, _("Filters..."));
	WMSetButtonAction(button, showFilterManagerWindow, this);
	WMSetBalloonTextForView(_("Open package filter editor"),
				WMWidgetView(button));
	WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 80, 0, 5);

	_popup = WMCreatePopUpButton(hbox);
	WMSetBalloonTextForView(_("Select package filter"),
				WMWidgetView(_popup));	
	WMAddPopUpButtonItem(_popup, _("All Packages"));

	WMAddBoxSubview(hbox, WMWidgetView(_popup), True, True, 80, 0, 5);
	WMSetPopUpButtonAction(_popup, changedFilter, this);


	_editFilterB = button = WMCreateCommandButton(hbox);
	WMSetButtonText(button, _("Edit Filter..."));
	WMSetButtonAction(button, showFilterWindow, this);
	WMSetBalloonTextForView(_("Edit selected filter"),
				WMWidgetView(button));
	WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 100, 0, 0);
	
	WMMapSubwidgets(hbox);
	
	// search stuff

	hbox = WMCreateBox(vbox);
	WMSetBoxHorizontal(hbox, True);
	WMAddTabViewItemWithView(_cmdPanel, WMWidgetView(hbox), 1, NULL);

	button = makeButton(hbox, NULL, filter_xpm, font);
	WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 20, 0, 5);
	WMSetBalloonTextForView(_("Switch to package filter panel"),
				WMWidgetView(button));
				
	
	WMSetButtonAction(button, switchCommandPanel, this);
	
	WMLabel *label = WMCreateLabel(hbox);
	WMSetLabelText(label, _("Find Package"));
	WMSetLabelTextAlignment(label, WARight);
	WMAddBoxSubview(hbox, WMWidgetView(label), False, True, 110, 0, 0);

	_findText = WMCreateTextField(hbox);
	WMAddBoxSubview(hbox, WMWidgetView(_findText), True, True, 40, 0, 5);
	WMAddNotificationObserver(findPackageObserver, this,
				  WMTextDidChangeNotification, _findText);
	WMAddNotificationObserver(findPackageObserver, this,
				  WMTextDidEndEditingNotification, _findText);
	WMSetViewNextResponder(WMWidgetView(_findText), WMWidgetView(_win));

	_findNextB = WMCreateCommandButton(hbox);
	WMSetButtonText(_findNextB, _("Next"));
	WMSetBalloonTextForView(_("Find next package matching pattern"),
				WMWidgetView(_findNextB));
	WMSetButtonAction(_findNextB, findNextAction, this);
	WMAddBoxSubview(hbox, WMWidgetView(_findNextB), False, True, 70, 0, 5);

	button = WMCreateCommandButton(hbox);
	WMSetButtonText(button, _("To Filter"));
	WMSetBalloonTextForView(_("Make a filter in 'Search Filter' showing all packages matching the pattern"),
				WMWidgetView(button));	
	WMSetButtonAction(button, makeFinderFilterAction, this);
	WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 80, 0, 0);

	WMMapSubwidgets(hbox);

	// table
	_table = WMCreateTableView(vbox);
	WMMapWidget(_table);
	WMSetTableViewDataSource(_table, this);
	WMSetTableViewBackgroundColor(_table, WMWhiteColor(scr));
	WMSetTableViewHeaderHeight(_table, 20);
	WMSetTableViewDelegate(_table, &delegate);
	WMAddBoxSubview(vbox, WMWidgetView(_table), True, True, 200, 0, 5);
	
	WMSetTableViewAction(_table, clickedRow, this);
/*
	// special action buttons
	hbox = WMCreateBox(vbox);
	WMSetBoxHorizontal(hbox, True);
	WMAddBoxSubview(vbox, WMWidgetView(hbox), False, False, 22, 0, 0);

	button = WMCreateCommandButton(hbox);
	WMSetButtonText(button, _("Unselect All"));
	WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 100, 0, 10);

	WMPopUpButton *advCmd = WMCreatePopUpButton(hbox);
	WMSetPopUpButtonPullsDown(advCmd, True);
	WMSetPopUpButtonText(advCmd, _("Advanced Dynamic Turbo Filters"));
	WMAddBoxSubview(hbox, WMWidgetView(advCmd), True, True, 40, 0, 0);

	WMMapSubwidgets(hbox);
	WMMapWidget(hbox);
*/
	WMMapWidget(vbox);

	colDeleg = WTCreatePixmapDelegate(_table);
	
	col = WMCreateTableColumn("");
	WMAddTableViewColumn(_table, col);
	WMSetTableColumnWidth(col, 20);
	WMSetTableColumnConstraints(col, 20, 20);
	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)CStatus);

	colDeleg = WTCreateStringDelegate(_table);
	
	col = WMCreateTableColumn(_("Package"));
	WMSetTableColumnWidth(col, 140);
	WMSetTableColumnConstraints(col, 2, 0);
	WMAddTableViewColumn(_table, col);	
	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)CName);
	/*
	col = WMCreateTableColumn(_("Section"));
	WMSetTableColumnWidth(col, 80);
	WMAddTableViewColumn(_table, col);	
	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)CSection);	
	*/
	col = WMCreateTableColumn(_("Installed"));
	WMSetTableColumnWidth(col, 80);
	WMSetTableColumnConstraints(col, 2, 0);
	WMAddTableViewColumn(_table, col);
	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)CInstalled);
	
	col = WMCreateTableColumn(_("Available"));
	WMSetTableColumnWidth(col, 80);
	WMSetTableColumnConstraints(col, 2, 0);	
	WMAddTableViewColumn(_table, col);
	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)CNewest);

	colDeleg = WTCreatePixmapStringDelegate(_table);

	col = WMCreateTableColumn(_("Summary"));
	WMSetTableColumnWidth(col, 400);	
	WMAddTableViewColumn(_table, col);
	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)CSummary);
    }
    
    
    
    WMAdjustSplitViewSubviews(hsplit);

    WMMapWidget(_nameL);
    WMMapWidget(_summL);
    WMMapWidget(_tabview);
    WMMapWidget(vbox);
  

    WMBox *boxy;
    boxy = WMCreateBox(_topBox);
    WMMapWidget(boxy);
    WMSetBoxHorizontal(boxy, True);
    WMAddBoxSubview(_topBox, WMWidgetView(boxy), False, True, 20, 0, 0);
 
    // status bar
    _statusL = WMCreateLabel(boxy);
    WMSetLabelRelief(_statusL, WRSunken);
    WMMapWidget(_statusL);
    WMAddBoxSubview(boxy, WMWidgetView(_statusL), True, True, 80, 0, 0);

    _cacheProgress = new RWCacheProgress(boxy, _statusL);

    WMMapWidget(_statusL);
    
    WMMapWidget(_topBox);
    
    WMRealizeWidget(_win);
}


void RWMainWindow::setStatusText(char *text)
{

    int listed, installed, broken;
    int toinstall, toreinstall, toremove;
    double size;

    _lister->getStats(installed, broken, toinstall, toreinstall, toremove, size);

    if (text) {
	WMSetLabelText(_statusL, text);
    } else {
	char buffer[256];

	listed = _lister->packagesSize();
	snprintf(buffer, sizeof(buffer), 
		 _("%i packages listed, %i installed, %i broken. %i to install/upgrade, %i to remove; %sB will be %s"),
		 listed, installed, broken, toinstall, toremove,
		 SizeToStr(fabs(size)).c_str(),
		 size < 0 ? _("freed") : _("used"));
	
	WMSetLabelText(_statusL, buffer);
    }
    
    WMSetButtonEnabled(_proceedB, (toinstall + toremove) != 0);
    _unsavedChanges = (toinstall + toremove) != 0;
    
    XFlush(WMScreenDisplay(WMWidgetScreen(_win)));
}


void RWMainWindow::refreshFilterMenu()
{
    vector<string> filters;
    filters = _lister->getFilterNames();

    int count = WMGetPopUpButtonNumberOfItems(_popup);
    for (int i = 1; i < count; i++)
	WMRemovePopUpButtonItem(_popup, 1);

    for (vector<string>::const_iterator iter = filters.begin();
	 iter != filters.end();
	 iter++) {
	WMAddPopUpButtonItem(_popup, (char*)(*iter).c_str());
    }
}


void RWMainWindow::saveState()
{
    _lister->storeFilters();
}


bool RWMainWindow::restoreState()
{
    _lister->restoreFilters();

    refreshFilterMenu();

    WMSetPopUpButtonSelectedItem(_popup, 0);
    changedFilter(_popup, this);
    setStatusText();

    return false;
}


void RWMainWindow::close()
{
    if (_interfaceLocked > 0)
	return;
    
    if (_unsavedChanges == false ||
	WMRunAlertPanel(WMWidgetScreen(_win), _win, _("Warning"),
			_("There are unsaved changes, are you sure\n"
			  "you want to quit Synaptic?"),
			_("Quit"), _("Cancel"), NULL) == WAPRDefault) {

	_error->Discard();
	
	saveState();
	
	showErrors();

	exit(0);
    }
}



void RWMainWindow::setInterfaceLocked(bool flag)
{
    WMScreen *scr = WMWidgetScreen(_win);
    Display *dpy = WMScreenDisplay(scr);

    if (flag) {
      //WMRect rect;

	_interfaceLocked++;
	if (_interfaceLocked > 1)
	    return;

	if (_lockWindow == None) {
	    _lockWindow = XCreateWindow(dpy, WMWidgetXID(_win),
					0, 0, 16000, 16000, 0,
					CopyFromParent,
					InputOnly, CopyFromParent,
					0, NULL);
	    XSelectInput(dpy, _lockWindow, ButtonPressMask);
	    XDefineCursor(dpy, _lockWindow, _busyCursor);
	}
	XMapRaised(dpy, _lockWindow);
    } else {
	assert(_interfaceLocked > 0);

	_interfaceLocked--;
	if (_interfaceLocked > 0)
	    return;

	XUnmapWindow(dpy, _lockWindow);
    }

    XFlush(dpy);
}
