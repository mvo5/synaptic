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

#ifdef HAVE_DEBTAGS
#include <TagcollParser.h>
#include <StdioParserInput.h>
#include <SmartHierarchy.h>
#include <TagcollBuilder.h>
#include <HandleMaker.h>
#include <TagCollection.h>
#endif

#include "rggladewindow.h"
#include "gtkpkgtree.h"
#include "gtkpkglist.h"
#include "gtktagtree.h"

class RGSourcesWindow;
class RGPreferencesWindow;
class RGFilterManagerWindow;
class RGFilterWindow;
class RGFindWindow;
class RGSetOptWindow;
class RGAboutPanel;

class RGUserDialog;
class RGCacheProgress;

typedef enum {
  PKG_KEEP, 
  PKG_INSTALL, 
  PKG_DELETE, 
  PKG_PURGE,
  PKG_DELETE_WITH_DEPS
} RGPkgAction;


class RGMainWindow : public RGGladeWindow, public RPackageObserver
{
  enum {
    DoNothing, 
    InstallRecommended, 
    InstallSuggested, 
    InstallSelected
  };
  
  enum {
      TOOLBAR_HIDE=-1
  };

  typedef enum {
      UPGRADE_ASK=-1,
      UPGRADE_NORMAL=0,
      UPGRADE_DIST=1
  } UpgradeType;
  
   RPackageLister *_lister;

   bool _unsavedChanges;
   
   bool _blockActions; // block signals from the action and hold buttons
   GtkToolbarStyle _toolbarStyle;
   int _interfaceLocked;
   GdkCursor *_busyCursor;
   GtkTooltips *_tooltips;
   
   GtkWidget *_sview; // scrolled window for table

   // the active model
   GtkTreeModel *_activeTreeModel;
   // the view
   GtkWidget *_treeView;

   RPackageLister::treeDisplayMode _treeDisplayMode;
   RPackageLister::treeDisplayMode _menuDisplayMode;

   GtkWidget *_statusL;

   GtkWidget *_currentB; // ptr to one of below
   GtkWidget *_actionB[3]; // keep, install, delete
   // menu items 
   GtkWidget *_keepM, *_installM, *_pkgupgradeM, *_removeM;
   GtkWidget *_remove_w_depsM, *_purgeM;

   GtkWidget *_actionBInstallLabel;
   GtkWidget *_pinM;
   GtkWidget *_pkgHelp;
   GtkWidget *_pkgReconfigure;

   GtkWidget *_proceedB;
   GtkWidget *_proceedM;
   GtkWidget *_upgradeB;
   GtkWidget *_upgradeM;
   //GtkWidget *_distUpgradeB;
   //GtkWidget *_distUpgradeM;
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
   GtkWidget *_pkginfo;
   GtkWidget *_vpaned;

   GtkWidget *_pkgCommonTextView;
   GtkTextBuffer *_pkgCommonTextBuffer;
   GtkTextTag *_pkgCommonBoldTag; 


   GtkWidget *_stateL;
   GtkImage *_stateP;

   GdkPixbuf *_alertPix;
   
   GtkWidget *_importP;
   GtkWidget *_importL;   
   GtkWidget *_infoL;
   
   GtkWidget *_descrView;
   GtkTextBuffer *_descrBuffer;

   GtkWidget *_filesView;
   GtkTextBuffer *_filesBuffer;
  
   GtkWidget *_depP;
   
   GtkWidget *_depTab;

   GtkWidget *_removeDepsB;
   
   GtkStyle *_redStyle;
   GtkStyle *_yellowStyle;
   GtkStyle *_blackStyle;
   
   GtkWidget *_rdepList; /* gtktreeview */
   GtkListStore *_rdepListStore;

   GtkWidget *_depList; /* gtktreeview */
   GtkListStore *_depListStore;
   GtkWidget *_depInfoL;

   GtkWidget *_recList; /* gtktreeview */
   GtkListStore *_recListStore;

   GtkWidget *_providesList; /* gtktreeview */
   GtkListStore *_providesListStore;


   GtkWidget *_availDepList; /* gtktreeview */
   GtkListStore *_availDepListStore;
   GtkWidget *_availDepInfoL;

   
   RGFilterManagerWindow *_fmanagerWin;
   RGSourcesWindow *_sourcesWin;
   RGPreferencesWindow *_configWin;
   RGFilterWindow *_filterWin;
   RGFindWindow *_findWin;
   RGSetOptWindow *_setOptWin;
   RGAboutPanel *_aboutPanel;
   
   RGCacheProgress *_cacheProgress;
   RGUserDialog *_userDialog;

   // init stuff 
   void buildInterface();
   void buildTreeView();

 private:
   // display/table releated
   void refreshTable(RPackage *selectedPkg=NULL); 
   void changeFilter(int filter, bool sethistory=true);
   void changeTreeDisplayMode(RPackageLister::treeDisplayMode mode);
   
   static void rowExpanded(GtkTreeView *treeview,  GtkTreeIter *arg1,
		    GtkTreePath *arg2, gpointer data);

   // pop-up menu
   static gboolean onButtonPressed(GtkWidget *treeview, 
				   GdkEventButton *event, 
				   gpointer userdata);
   static void treeviewPopupMenu(GtkWidget *treeview, 
				 GdkEventButton *event, 
				 RGMainWindow *me,
				 RPackage *pkg);

   
   virtual bool close();
   static bool closeWin(GtkWidget *self, void *me) { 
       return ((RGMainWindow*)me)->close(); 
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
   static void onTagTree(GtkWidget *self, void *data);   

   // this is called when typing
   guint searchLackId;
   static void searchLackAction(GtkWidget *self, void *data);
   static gboolean searchLackHelper(void *data);
   static void searchAction(GtkWidget *self, void *data);
   static void searchNextAction(GtkWidget *self, void *data);
   static void searchBeginAction(GtkWidget *self, void *data);
   static void changedFilter(GtkWidget *self);
   
   static void changedDepView(GtkWidget *self, void *data);
   static void clickedRecInstall(GtkWidget *self, void *data);

   static void clickedDepList(GtkTreeSelection *sel, gpointer data);
   static void clickedAvailDepList(GtkTreeSelection *sel, gpointer data);

   // misc
   GtkWidget *createFilterMenu();
   void refreshFilterMenu();
   void forgetNewPackages();
   
   // package info
   void updatePackageInfo(RPackage *pkg);
   void updatePackageStatus(RPackage *pkg);
   void updateDynPackageInfo(RPackage *pkg);
   void updateVersionButtons(RPackage *pkg);
   RPackage *selectedPackage();

   // menu stuff
   
   // file menu
   static void openClicked(GtkWidget *self, void *data);
   static void doOpenSelections(GtkWidget *file_selector, 
				gpointer data);
   static void saveClicked(GtkWidget *self, void *data);
   static void saveAsClicked(GtkWidget *self, void *data);
   static void doSaveSelections(GtkWidget *file_selector, 
				gpointer data);
   string selectionsFilename;
   bool saveFullState;

   // actions menu
   static void undoClicked(GtkWidget *self, void *data);
   static void redoClicked(GtkWidget *self, void *data);
   static void clearAllChangesClicked(GtkWidget *self, void *data);
   static void updateClicked(GtkWidget *self, void *data);
   static void onAddCDROM(GtkWidget *self, void *data);
   static void fixBrokenClicked(GtkWidget *self, void *data);
   static void upgradeClicked(GtkWidget *self, void *data);
   //static void distUpgradeClicked(GtkWidget *self, void *data);
   static void proceedClicked(GtkWidget *self, void *data);
   
   // packages menu
   static void menuActionClicked(GtkWidget *self, void *data);
   static void menuPinClicked(GtkWidget *self, void *data);

   // filter menu
   static void showFilterManagerWindow(GtkWidget *self, void *data);   
   static void saveFilterAction(void *self, RGFilterWindow *rwin);
   static void closeFilterAction(void *self, RGFilterWindow *rwin);
   static void closeFilterManagerAction(void *self, bool okcancel);
   
   // search menu
   static void findToolClicked(GtkWidget *self, void *data);

   // preferences menu
   static void showConfigWindow(GtkWidget *self, void *data);
   static void showSetOptWindow(GtkWidget *self, void *data);
   static void showSourcesWindow(GtkWidget *self, void *data);
   static void menuToolbarClicked(GtkWidget *self, void *data);

   // help menu
   static void helpAction(GtkWidget *self, void *data); 
   static void showAboutPanel(GtkWidget *self, void *data); 

   // end menu


   // the buttons 
   static void actionClicked(GtkWidget *clickedB, void *data);
   static void pkgHelpClicked(GtkWidget *self, void *data);
   static void pkgReconfigureClicked(GtkWidget *self, void *data);


   // helpers

   // this does the actual pkgAction work (install, remove, upgrade)
   static void doPkgAction(RGMainWindow *me, RGPkgAction action);
   void pkgInstallHelper(RPackage *pkg, bool fixBroken=true);
   void pkgRemoveHelper(RPackage *pkg, bool purge=false, bool withDeps=false);
   void pkgKeepHelper(RPackage *pkg);

   // install a non-standard version (data is a char* of the version)
   static void installFromVersion(GtkWidget *self, void *data);

   // RPackageObserver
   virtual void notifyChange(RPackage *pkg);
   virtual void notifyPreFilteredChange() {};
   virtual void notifyPostFilteredChange() {};

   // obsolete
   //static void removeDepsClicked(GtkWidget *self, void *data);
   //static void actionVisibleClicked(GtkWidget *self, void *data);

public:
   RGMainWindow(RPackageLister *packLister, string name);
   virtual ~RGMainWindow() {};

   void setInterfaceLocked(bool flag);
   void setTreeLocked(bool flag);
   void setColors(bool useColors);
   void rebuildTreeView() { buildTreeView(); };

   void setStatusText(char *text = NULL);
   
   void saveState();
   bool restoreState();
   
   bool showErrors();
   

   void proceed();
   void showRepositoriesWindow();
};


#endif

