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
#include "rgiconlegend.h"
#include "gtkpkgtree.h"
#include "gtkpkglist.h"
#include "gtktagtree.h"

#define TOOLBAR_HIDE -1

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

class RGMainWindow : public RGGladeWindow, public RPackageObserver {
   enum {
      DoNothing,
      InstallRecommended,
      InstallSuggested,
      InstallSelected
   };

   typedef enum {
      UPGRADE_ASK = -1,
      UPGRADE_NORMAL = 0,
      UPGRADE_DIST = 1
   } UpgradeType;

   RPackageLister *_lister;

   bool _unsavedChanges;

   bool _blockActions;          // block signals from the action and hold buttons
   GtkToolbarStyle _toolbarStyle;
   int _interfaceLocked;
   GdkCursor *_busyCursor;
   GtkTooltips *_tooltips;

   GtkWidget *_sview;           // scrolled window for table

   // the active model
   GtkTreeModel *_activeTreeModel;
   // the view
   GtkWidget *_treeView;

   // the left-side view
   GtkWidget *_viewPopup;
   GtkWidget *_subViewList;

   int _viewMode;
   //RPackageLister::treeDisplayMode _menuDisplayMode;

   GtkWidget *_statusL;
   GtkWidget *_progressBar;

   GtkWidget *_currentB;        // ptr to one of below
   GtkWidget *_actionB[3];      // keep, install, delete
   // menu items 
   GtkWidget *_keepM, *_installM, *_pkgupgradeM, *_removeM;
   GtkWidget *_remove_w_depsM, *_purgeM;

   // popup-menu
   GtkWidget *_popupMenu;

   GtkWidget *_actionBInstallLabel;
   GtkWidget *_pinM;
   GtkWidget *_pkgHelp;
   GtkWidget *_pkgReconfigure;

   GtkWidget *_proceedB;
   GtkWidget *_proceedM;
   GtkWidget *_upgradeB;
   GtkWidget *_upgradeM;
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

   GtkWidget *_importP;

   GtkWidget *_filesView;
   GtkTextBuffer *_filesBuffer;

   RGFilterManagerWindow *_fmanagerWin;
   RGSourcesWindow *_sourcesWin;
   RGPreferencesWindow *_configWin;
   RGFilterWindow *_filterWin;
   RGFindWindow *_findWin;
   RGSetOptWindow *_setOptWin;
   RGAboutPanel *_aboutPanel;
   RGIconLegendPanel *_iconLegendPanel;

   RGCacheProgress *_cacheProgress;
   RGUserDialog *_userDialog;

   // init stuff 
   void buildInterface();
   void buildTreeView();

 private:
   // display/table releated
   void refreshTable(RPackage *selectedPkg = NULL);
   void changeFilter(int filter, bool sethistory = true);
   static void changedFilter(GtkWidget *self);

   void changeView(int view, bool sethistory = true);
   static void subViewListSelectionChanged(GtkTreeSelection *selection,
                                           gpointer data);
   static void changedView(GtkWidget *self);
   GtkWidget *createViewMenu();
   void refreshSubViewList();

   static void rowExpanded(GtkTreeView *treeview, GtkTreeIter *arg1,
                           GtkTreePath *arg2, gpointer data);

   static void treeviewPopupMenu(GtkWidget *treeview,
                                 GdkEventButton * event,
                                 RGMainWindow *me,
                                 vector<RPackage *> selected_pkgs);


   virtual bool close();
   static bool closeWin(GtkWidget *self, void *me) {
      return ((RGMainWindow *) me)->close();
   };

   // misc
   GtkWidget *createFilterMenu();
   void refreshFilterMenu();
   void forgetNewPackages();

   // package info
   void updatePackageInfo(RPackage *pkg);
   RPackage *selectedPackage();


   // helpers
   void pkgAction(RGPkgAction action);
   void pkgInstallHelper(RPackage *pkg, bool fixBroken = true);
   void pkgRemoveHelper(RPackage *pkg, bool purge = false,
		   	bool withDeps = false);
   void pkgKeepHelper(RPackage *pkg);

   // install a non-standard version (data is a char* of the version)
   static void installFromVersion(GtkWidget *self, void *data);

   // RPackageObserver
   virtual void notifyChange(RPackage *pkg);
   virtual void notifyPreFilteredChange() {
   };
   virtual void notifyPostFilteredChange() {
   };

 public:
   RGMainWindow(RPackageLister *packLister, string name);
   virtual ~RGMainWindow() {};

   void setInterfaceLocked(bool flag);
   void setTreeLocked(bool flag);
   void rebuildTreeView() {
      buildTreeView();
   };

   void setStatusText(char *text = NULL);

   void saveState();
   bool restoreState();

   bool initDebtags();

   bool showErrors();


   void proceed();
   void showRepositoriesWindow();


   // --------------------------------------------------------------------
   // Callbacks
   //

   static void cbPkgAction(GtkWidget *self, void *data);

   static gboolean cbPackageListClicked(GtkWidget *treeview,
                                        GdkEventButton * event,
                                        gpointer userdata);

   static void selectedRow(GtkTreeSelection *selection, gpointer data);
   static void cbPackageListRowActivated(GtkTreeView *treeview,
                                         GtkTreePath *arg1,
                                         GtkTreeViewColumn *arg2,
                                         gpointer user_data);

   // menu stuff

   // file menu
   static void openClicked(GtkWidget *self, void *data);
   static void doOpenSelections(GtkWidget *file_selector, gpointer data);
   static void saveClicked(GtkWidget *self, void *data);
   static void saveAsClicked(GtkWidget *self, void *data);
   static void doSaveSelections(GtkWidget *file_selector, gpointer data);
   string selectionsFilename;
   bool saveFullState;

   // actions menu
   static void undoClicked(GtkWidget *self, void *data);
   static void redoClicked(GtkWidget *self, void *data);
   static void clearAllChangesClicked(GtkWidget *self, void *data);
   static void updateClicked(GtkWidget *self, void *data);
   static void cbAddCDROM(GtkWidget *self, void *data);
   static void fixBrokenClicked(GtkWidget *self, void *data);
   static void upgradeClicked(GtkWidget *self, void *data);
   //static void distUpgradeClicked(GtkWidget *self, void *data);
   static void proceedClicked(GtkWidget *self, void *data);

   // packages menu
   static void menuPinClicked(GtkWidget *self, void *data);

   // filter menu
   static void showFilterManagerWindow(GtkWidget *self, void *data);
   static void saveFilterAction(void *self, RGFilterWindow * rwin);
   static void closeFilterAction(void *self, RGFilterWindow * rwin);
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
   static void showIconLegendPanel(GtkWidget *self, void *data);
   static void showAboutPanel(GtkWidget *self, void *data);
   static void showWelcomeDialog(GtkWidget *self, void *data);

   // end menu


   // the buttons 
   static void pkgHelpClicked(GtkWidget *self, void *data);
   static void pkgReconfigureClicked(GtkWidget *self, void *data);

};


#endif
