/* rgmainwindow.h - main window of application
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


#ifndef _RGMAINWINDOW_H_
#define _RGMAINWINDOW_H_

using namespace std;

#include "rpackagelister.h"

#include <gtk/gtk.h>
#include <vector>
#include <set>

#include "rgwindow.h"
#include "gtkpkgtree.h"

class RGSourcesWindow;
class RGConfigWindow;
class RGFilterManagerWindow;
class RGFilterWindow;
class RGFindWindow;
class RGAboutPanel;

class RGUserDialog;
class RGCacheProgress;

typedef enum {
  PKG_KEEP, 
  PKG_INSTALL, 
  PKG_DELETE, 
  PKG_PURGE
} RGPkgAction;


class RGMainWindow : public RGWindow, public RPackageObserver
{
  friend class SynapticInterface;
  enum {
    DoNothing, 
    InstallRecommended, 
    InstallSuggested, 
    InstallSelected
  };
  
  typedef enum {
    TOOLBAR_PIXMAPS,
    TOOLBAR_TEXT,
    TOOLBAR_BOTH,
    TOOLBAR_HIDE
  } ToolbarState;

   RPackageLister *_lister;

   bool _showUpdateInfo;
   bool _unsavedChanges;
   
   bool _blockActions; // block signals from the action and hold buttons
   ToolbarState _toolbarState;

   int _interfaceLocked;
   GdkCursor *_busyCursor;
   GtkTooltips *_tooltips;
   
   GtkWidget *_sview; // scrolled window for table

   GtkPkgTree *_pkgTree;
   GtkWidget *_treeView;
   RPackageLister::treeDisplayMode _treeDisplayMode;

   GtkWidget *_statusL;

   GtkWidget *_currentB; // ptr to one of below
   GtkWidget *_actionB[3];
   GtkWidget *_actionBInstallLabel;
   GtkWidget *_pinB;
   GtkWidget *_pinM;
   GtkWidget *_pkgHelp;
   GtkWidget *_pkgReconfigure;

   GtkWidget *_proceedB;
   GtkWidget *_proceedM;
   GtkWidget *_upgradeB;
   GtkWidget *_upgradeM;
   GtkWidget *_distUpgradeB;
   GtkWidget *_distUpgradeM;
   GtkWidget *_fixBrokenM;

   // filter/find panel   
   GtkWidget *_cmdPanel;
   
   GtkWidget *_findText;
   GtkWidget *_findSearchB;

   GtkWidget *_editFilterB;
   GtkWidget *_filtersB;
   GtkWidget *_filterPopup;
   GtkWidget *_filterMenu;

   // package info tabs   
   GtkWidget *_tabview;
   GtkWidget *_vpaned;

   GtkWidget *_nameL;
   GtkWidget *_summL;
   GtkWidget *_stateL;
   GtkImage *_stateP;

   GdkPixbuf *_alertPix;
   
   GtkWidget *_importP;
   GtkWidget *_importL;   
   GtkWidget *_infoL;
   
   
   GtkWidget *_textView;
   GtkTextBuffer *_textBuffer;
   
   GtkWidget *_depP;
   
   GtkWidget *_depTab;

   GtkWidget *_removeDepsB;
   
   GtkStyle *_redStyle;
   GtkStyle *_yellowStyle;
   GtkStyle *_blackStyle;
   
   GtkWidget *_rdepList;
   GtkWidget *_depList;
   GtkWidget *_recList;
   GtkWidget *_depInfoL;
   GtkWidget *_availDepList;
   GtkWidget *_availDepInfoL;
   
   RGFilterManagerWindow *_fmanagerWin;
   RGSourcesWindow *_sourcesWin;
   RGConfigWindow *_configWin;
   RGFilterWindow *_filterWin;
   RGFindWindow *_findWin;
   RGAboutPanel *_aboutPanel;
   
   RGCacheProgress *_cacheProgress;
   RGUserDialog *_userDialog;

   // init stuff 
   void buildInterface();

   // display/table releated
   void refreshTable(RPackage *selectedPkg=NULL);
   void restoreTableState(vector<string>& expanded_sections, GtkTreeIter it);
   GtkTreeIter saveTableState(vector<string>& expanded_sections);
   void changeFilter(int filter, bool sethistory=true);
   void changeTreeDisplayMode(RPackageLister::treeDisplayMode mode);
   
   virtual void close();
   static void closeWin(GtkWidget *self, void *me) { 
       ((RGMainWindow*)me)->close(); 
     };

   static void selectedRow(GtkTreeSelection *selection, gpointer data);
   static void doubleClickRow(GtkTreeView *treeview,
			      GtkTreePath *arg1,
			      GtkTreeViewColumn *arg2,
			      gpointer user_data);
   static void onExpandAll(GtkWidget *self, void *data);
   static void onCollapseAll(GtkWidget *self, void *data);   

   static void onSectionTree(GtkWidget *self, void *data);
   static void onAlphabeticTree(GtkWidget *self, void *data);   
   static void onStatusTree(GtkWidget *self, void *data);   
   static void onFlatList(GtkWidget *self, void *data);   

   static void searchAction(GtkWidget *self, void *data);
   static void changedFilter(GtkWidget *self);
   
   static void changedDepView(GtkWidget *self, void *data);
   static void clickedRecInstall(GtkWidget *self, void *data);

   static void clickedDepList(GtkWidget *self, int row, int col, GdkEvent *ev);
   static void clickedAvailDepList(GtkWidget *self, int row, int col, 
				   GdkEvent *ev, void* data);

   // misc
   GtkWidget *createFilterMenu();
   void refreshFilterMenu();
   void forgetNewPackages();
   
   // package info
   void updatePackageInfo(RPackage *pkg);
   void updatePackageStatus(RPackage *pkg);
   void updateDynPackageInfo(RPackage *pkg);
   RPackage *selectedPackage();

   
   

   // menu stuff

   // actions menu
   static void undoClicked(GtkWidget *self, void *data);
   static void redoClicked(GtkWidget *self, void *data);
   static void updateClicked(GtkWidget *self, void *data);
   static void onAddCDROM(GtkWidget *self, void *data);
   static void fixBrokenClicked(GtkWidget *self, void *data);
   static void upgradeClicked(GtkWidget *self, void *data);
   static void distUpgradeClicked(GtkWidget *self, void *data);
   static void proceedClicked(GtkWidget *self, void *data);
   
   // packages menu
   static void menuActionClicked(GtkWidget *self, void *data);
   static void menuPinClicked(GtkWidget *self, void *data);

   // filter menu
   static void showFilterManagerWindow(GtkWidget *self, void *data);   
   static void saveFilterAction(void *self, RGFilterWindow *rwin);
   static void closeFilterAction(void *self, RGFilterWindow *rwin);
   static void closeFilterManagerAction(void *self,RGFilterManagerWindow *win);
   
   // search menu
   static void searchPkgDescriptionClicked(GtkWidget *self, void *data);
   static void searchPkgNameClicked(GtkWidget *self, void *data);
   static void searchPkgAction(void *self, RGFindWindow *data);

   // preferences menu
   static void showConfigWindow(GtkWidget *self, void *data);
   static void showSourcesWindow(GtkWidget *self, void *data);
   static void menuToolbarClicked(GtkWidget *self, void *data);

   // help menu
   static void showAboutPanel(GtkWidget *self, void *data); 

   // end menu


   // the buttons 
   static void actionClicked(GtkWidget *clickedB, void *data);
   static void pinClicked(GtkWidget *self, void *data);
   static void pkgHelpClicked(GtkWidget *self, void *data);
   static void pkgReconfigureClicked(GtkWidget *self, void *data);


   // helpers

   // this does the actual pkgAction work (install, remove, upgrade)
   static void doPkgAction(RGMainWindow *me, RGPkgAction action);
   void pkgInstallHelper(RPackage *pkg);
   void pkgRemoveHelper(RPackage *pkg, bool purge = false);
   void pkgKeepHelper(RPackage *pkg);

   // RPackageObserver
   virtual void notifyChange(RPackage *pkg);

   // obsolete
   //static void removeDepsClicked(GtkWidget *self, void *data);
   //static void actionVisibleClicked(GtkWidget *self, void *data);

public:
   RGMainWindow(RPackageLister *packLister);
   virtual ~RGMainWindow() {};

   void setInterfaceLocked(bool flag);
   void setColors(bool useColors);
   void setStatusText(char *text = NULL);
   
   void saveState();
   bool restoreState();
   
   bool showErrors();
   

   void proceed();
};


#endif

