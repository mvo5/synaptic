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

#include "rgtaskswin.h"
#include "rggladewindow.h"
#include "rgiconlegend.h"
#include "gtkpkglist.h"
#include "rgpkgdetails.h"

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
   PKG_INSTALL_FROM_VERSION,
   PKG_DELETE,
   PKG_PURGE,
   PKG_DELETE_WITH_DEPS,
   PKG_REINSTALL
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

   GtkTreeModel *_pkgList;
   GtkWidget *_treeView;

   // the left-side view
   GtkWidget *_viewPopup;
   GtkWidget *_subViewList;

   GtkWidget *_statusL;
   GtkWidget *_progressBar;

   // menu items 
   GtkWidget *_keepM, *_installM, *_reinstallM, *_pkgupgradeM, *_removeM;
   GtkWidget *_remove_w_depsM, *_purgeM;
   GtkWidget *_dl_changelogM, *_detailsM;

   // popup-menu
   GtkWidget *_popupMenu;

   GtkWidget *_actionBInstallLabel;
   GtkWidget *_pinM;
   GtkWidget *_overrideVersionM;
   GtkWidget *_pkgHelpM;
   GtkWidget *_pkgReconfigureM;

   GtkWidget *_proceedB;
   GtkWidget *_proceedM;
   GtkWidget *_upgradeB;
   GtkWidget *_upgradeM;
   GtkWidget *_propertiesB;
   GtkWidget *_fixBrokenM;

   // filter/find panel   
   GtkWidget *_cmdPanel;

   GtkWidget *_findText;
   GtkWidget *_findSearchB;
#if 0
   GtkWidget *_editFilterB;
#endif
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
   RGFindWindow *_findWin;
   RGSetOptWindow *_setOptWin;
   RGAboutPanel *_aboutPanel;
   RGTasksWin *_tasksWin;
   RGIconLegendPanel *_iconLegendPanel;
   RGPkgDetailsWindow *_pkgDetails;

   RGCacheProgress *_cacheProgress;
   RGUserDialog *_userDialog;

   // init stuff 
   void buildInterface();
   void buildTreeView();

 private:
   // display/table releated
   void refreshTable(RPackage *selectedPkg = NULL);
#if 0
   void changeFilter(int filter, bool sethistory = true);
#endif

   GtkWidget *createViewMenu();
   void refreshSubViewList(string selectedSubView="");

   virtual bool close();
   static bool closeWin(GtkWidget *self, void *me) {
      return ((RGMainWindow *) me)->close();
   };

   // misc
#if 0
   GtkWidget *createFilterMenu();
   void refreshFilterMenu();
#endif
   void forgetNewPackages();

   // package info
   void updatePackageInfo(RPackage *pkg);
   RPackage *selectedPackage();
   string selectedSubView();

#if INTERACTIVE_SEARCH_ON_KEYPRESS
   // interactive search by pressing a key in treeview. 
   // sebastian thinks it's too confusing (but it works)
   static gboolean cbKeyPressedInTreeView(GtkWidget *widget, GdkEventKey *event, gpointer data);
#endif

   // helpers
   void pkgAction(RGPkgAction action);
   bool askStateChange(RPackageLister::pkgState, vector<RPackage *> exclude);
   bool checkForFailedInst(vector<RPackage *> instPkgs);
   void pkgInstallHelper(RPackage *pkg, bool fixBroken = true, 
			 bool reInstall = false);
   void pkgRemoveHelper(RPackage *pkg, bool purge = false,
		   	bool withDeps = false);
   void pkgKeepHelper(RPackage *pkg);

   // helper for recommends/suggests 
   // (data is the name of the pkg, self needs to have a pointer to "me" )
   static void pkgInstallByNameHelper(GtkWidget *self, void *data);

   // install a non-standard version (data is a char* of the version)
   static void cbInstallFromVersion(GtkWidget *self, void *data);

   // RPackageObserver
   virtual void notifyChange(RPackage *pkg);
   virtual void notifyPreFilteredChange() {
   };
   virtual void notifyPostFilteredChange() {
   };

 public:
   RGMainWindow(RPackageLister *packLister, string name);
   virtual ~RGMainWindow() {};

   void changeView(int view, bool sethistory = true, string subView="");

   // install the list of packagenames and display a changes window
   void selectToInstall(vector<string> packagenames);

   // show busy cursor over main window
   void setBusyCursor(bool flag=true);

   void setInterfaceLocked(bool flag);
   void setTreeLocked(bool flag);
   void rebuildTreeView() {
      buildTreeView();
   };

   void setStatusText(char *text = NULL);

   void saveState();
   bool restoreState();

   bool showErrors();

   GtkWidget* buildWeakDependsMenu(RPackage *pkg, pkgCache::Dep::DepType);


   // --------------------------------------------------------------------
   // Callbacks
   //

   static void cbPkgAction(GtkWidget *self, void *data);

   static gboolean cbPackageListClicked(GtkWidget *treeview,
                                        GdkEventButton *event,
                                        gpointer data);

   static void cbTreeviewPopupMenu(GtkWidget *treeview,
                                   GdkEventButton *event,
                                   RGMainWindow *me,
                                   vector<RPackage *> selected_pkgs);

   static void cbChangelogDialog(GtkWidget *self, void *data);

   static void cbSelectedRow(GtkTreeSelection *selection, gpointer data);
   static void cbPackageListRowActivated(GtkTreeView *treeview,
                                         GtkTreePath *arg1,
                                         GtkTreeViewColumn *arg2,
                                         gpointer user_data);

   static void cbChangedView(GtkWidget *self);
   static void cbChangedSubView(GtkTreeSelection *selection, gpointer data);

   static void cbDetailsWindow(GtkWidget *self, void *data);

   // file menu
   static void cbTasksClicked(GtkWidget *self, void *data);
   static void cbOpenClicked(GtkWidget *self, void *data);
   static void cbOpenSelections(GtkWidget *file_selector, gpointer data);
   static void cbSaveClicked(GtkWidget *self, void *data);
   static void cbSaveAsClicked(GtkWidget *self, void *data);
   static void cbSaveSelections(GtkWidget *file_selector, gpointer data);
   string selectionsFilename;
   bool saveFullState;

   // actions menu
   static void cbUndoClicked(GtkWidget *self, void *data);
   static void cbRedoClicked(GtkWidget *self, void *data);
   static void cbClearAllChangesClicked(GtkWidget *self, void *data);
   static void cbUpdateClicked(GtkWidget *self, void *data);
   static void cbAddCDROM(GtkWidget *self, void *data);
   static void cbFixBrokenClicked(GtkWidget *self, void *data);
   static void cbUpgradeClicked(GtkWidget *self, void *data);
   static void cbProceedClicked(GtkWidget *self, void *data);

   // packages menu
   static void cbMenuPinClicked(GtkWidget *self, void *data);

   // filter menu
   static void cbShowFilterManagerWindow(GtkWidget *self, void *data);
   static void cbSaveFilterAction(void *self, RGFilterWindow * rwin);
   static void cbCloseFilterAction(void *self, RGFilterWindow * rwin);
   static void cbCloseFilterManagerAction(void *self, bool okcancel);

   // search menu
   static void cbFindToolClicked(GtkWidget *self, void *data);

   // preferences menu
   static void cbShowConfigWindow(GtkWidget *self, void *data);
   static void cbShowSetOptWindow(GtkWidget *self, void *data);
   static void cbShowSourcesWindow(GtkWidget *self, void *data);
   static void cbMenuToolbarClicked(GtkWidget *self, void *data);

   // help menu
   static void cbHelpAction(GtkWidget *self, void *data);
   static void cbShowIconLegendPanel(GtkWidget *self, void *data);
   static void cbShowAboutPanel(GtkWidget *self, void *data);
   static void cbShowWelcomeDialog(GtkWidget *self, void *data);

   // the buttons 
   static void cbPkgHelpClicked(GtkWidget *self, void *data);
   static void cbPkgReconfigureClicked(GtkWidget *self, void *data);

};


#endif
