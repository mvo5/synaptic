/* rwmainwindow.h - main window of application
 * 
 * Copyright (c) 2000, 2001 Conectiva S/A 
 * 
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
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


#ifndef _RWMAINWINDOW_H_
#define _RWMAINWINDOW_H_


#include "rpackagelister.h"

#include <vector>
#include <WINGs/WINGs.h>
#include <WINGs/wtableview.h>

#include "rwwindow.h"


class RWSourcesWindow;
class RWConfigWindow;
class RWFilterManagerWindow;
class RWFilterWindow;
class RWAboutPanel;


class RWUserDialog;
class RWCacheProgress;

class RWMainWindow : public RWWindow, public RPackageObserver
{
   RPackageLister *_lister;

   bool _showUpdateInfo;
   
   bool _unsavedChanges;
   
   bool _tempKluge; // yet another permanent temporary kluge.. 

   int _interfaceLocked;
   Window _lockWindow;
   Cursor _busyCursor;
   
   WMTableView *_table;
   
   WMLabel *_statusL;

   WMButton *_currentB; // ptr to one of below
   WMButton *_actionB[3];
   WMButton *_holdB;

   WMButton *_proceedB;

   // filter/find panel   
   bool _panelSwitched;
   WMTabView *_cmdPanel;
   
   WMTextField *_findText;
   WMButton *_findNextB;

   WMButton *_editFilterB;
   WMButton *_filtersB;
   WMPopUpButton *_popup;

   // package info tabs   
   WMTabView *_tabview;

   WMLabel *_nameL;
   WMLabel *_summL;
   WMLabel *_stateL;

   WMPixmap *_alertPix;
   
   WMLabel *_importL;   
   WMLabel *_infoL;
   
   
   WMText *_text;
   
   WMPopUpButton *_depP;
   
   WMTabView *_depTab;

   WMButton *_removeDepsB;
   
   WMList *_rdepList;
   WMList *_depList;
   WMList *_recList;
   WMLabel *_depInfoL;
   
   RWFilterManagerWindow *_fmanagerWin;
   RWSourcesWindow *_sourcesWin;
   RWConfigWindow *_configWin;
   RWFilterWindow *_filterWin;
   RWAboutPanel *_aboutPanel;
   
   RWCacheProgress *_cacheProgress;
   RWUserDialog *_userDialog;

   
   void buildInterface(WMScreen *scr);
   
   void refreshFilterMenu();

   void updatePackageInfo(RPackage *pkg);
   
   void updatePackageStatus(RPackage *pkg);
   void updateDynPackageInfo(RPackage *pkg);
   
   RPackage *selectedPackage();
   void refreshTable(RPackage *selectedPkg);
      
   void navigateTable(char direction);

   virtual void close();
   
   static void dependListDraw(WMList *lPtr, int index, Drawable d, char *text,
			      int state, WMRect *rect);

   static int numberOfRows(WMTableViewDelegate *self, WMTableView *table);
   static void *valueForCell(WMTableViewDelegate *self,
			     WMTableColumn *column, int row);

   static void showSearchPanel(WMWidget *self, void *data);
   static void showAboutPanel(WMWidget *self, void *data);
   static void showFilterWindow(WMWidget *self, void *data);
   static void showFilterManagerWindow(WMWidget *self, void *data);   
   static void showSourcesWindow(WMWidget *self, void *data);
   static void showConfigWindow(WMWidget *self, void *data);

   static void findPackageObserver(void *self, WMNotification *notif);
   
   static void makeFinderFilterAction(WMWidget *self, void *data);
   static void findNextAction(WMWidget *self, void *data);
   
   static void switchCommandPanel(WMWidget *self, void *data);

   static void clickedRow(WMWidget *self, void *data);
   static void changedFilter(WMWidget *self, void *data);
   
   static void changedDepView(WMWidget *self, void *data);

   static void clickedDepList(WMWidget *self, void *data);

   static void updateClicked(WMWidget *self, void *data);
   static void fixBrokenClicked(WMWidget *self, void *data);
   static void upgradeClicked(WMWidget *self, void *data);
   static void distUpgradeClicked(WMWidget *self, void *data);
   static void proceedClicked(WMWidget *self, void *data);
   
   static void actionClicked(WMWidget *self, void *data);
   static void holdClicked(WMWidget *self, void *data);

   static void removeDepsClicked(WMWidget *self, void *data);

   static void actionVisibleClicked(WMWidget *self, void *data);

   static void saveFilterAction(void *self, RWFilterWindow *rwin);
   static void closeFilterAction(void *self, RWFilterWindow *rwin);
   
   static void closeFilterManagerAction(void *self, RWFilterManagerWindow *win);

   static void keyPressHandler(XEvent *event, void *data);
   
   // RPackageObserver
   virtual void notifyChange(RPackage *pkg);
   virtual void notifyPreFilteredChange() {};
   virtual void notifyPostFilteredChange() {};
   
public:
   RWMainWindow(WMScreen *scr, RPackageLister *packLister);
   virtual ~RWMainWindow() {};

   void setInterfaceLocked(bool flag);

   void setStatusText(char *text = NULL);
   
   void saveState();
   bool restoreState();
   
   bool showErrors();
};


#endif

