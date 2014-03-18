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
#include "rgfetchprogress.h"
#include "rinstallprogress.h"
#include "rggtkbuilderwindow.h"
#include "rgiconlegend.h"
#include "gtkpkglist.h"
#include "rgpkgdetails.h"
#include "rglogview.h"

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

extern const char *relOptions[];

class RGMainWindow : public RGGtkBuilderWindow, public RPackageObserver {

   typedef enum {
      UPGRADE_ASK = -1,
      UPGRADE_NORMAL = 0,
      UPGRADE_DIST = 1
   } UpgradeType;


   bool _unsavedChanges;
   bool _blockActions;        // block signals from the action and hold buttons
   int _interfaceLocked;      

   // the central class that has all the package information
   RPackageLister *_lister;

   // interface stuff
   GtkToolbarStyle _toolbarStyle; // hide, small, normal toolbar

   GtkTreeModel *_pkgList;   // the custom list model for the packages
   GtkWidget *_treeView;     // the display widget

   // the left-side view
   GtkWidget *_subViewList;


   // menu items 
   GtkWidget *_keepM, *_installM, *_reinstallM, *_pkgupgradeM, *_removeM;
   GtkWidget *_remove_w_depsM, *_purgeM;
   GtkWidget *_dl_changelogM, *_detailsM;

   GtkWidget *_pinM;
   GtkWidget *_autoM;
   GtkWidget *_overrideVersionM;
   GtkWidget *_pkgHelpM;
   GtkWidget *_pkgReconfigureM;

   GtkWidget *_proceedB;
   GtkWidget *_proceedM;
   GtkWidget *_upgradeB;
   GtkWidget *_upgradeM;
   GtkWidget *_propertiesB;
   GtkWidget *_fixBrokenM;

   // popup-menu in the treeview
   GtkWidget *_popupMenu;

   // the description buffer
   GtkTextBuffer *_pkgCommonTextBuffer;

   // the various dialogs
   RGFilterManagerWindow *_fmanagerWin;
   RGSourcesWindow *_sourcesWin;
   RGPreferencesWindow *_configWin;
   RGFindWindow *_findWin;
   RGSetOptWindow *_setOptWin;
   RGAboutPanel *_aboutPanel;
   RGTasksWin *_tasksWin;
   RGIconLegendPanel *_iconLegendPanel;
   RGPkgDetailsWindow *_pkgDetails;
   RGLogView *_logView;
   RGUserDialog *_userDialog;
   RGFetchProgress *_fetchProgress;
   RGWindow *_installProgress;

   // fast search stuff
   int _fastSearchEventID;
   GtkWidget *_entry_fast_search;

   // the buttons for the various views
   GtkWidget *_viewButtons[N_PACKAGE_VIEWS];

   // init stuff 
   void buildInterface();
   void buildTreeView();

 private:
   // display/table releated
   void refreshSubViewList();

   virtual bool close();
   static bool closeWin(GtkWidget *self, void *me) {
      return ((RGMainWindow *) me)->close();
   };

   // misc
   void forgetNewPackages();

   // package info
   void updatePackageInfo(RPackage *pkg);
   RPackage *selectedPackage();
   string selectedSubView();

   // helpers
   void pkgAction(RGPkgAction action);
   bool askStateChange(RPackageLister::pkgState, 
                       const vector<RPackage *> &exclude = vector<RPackage*>());
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

   // helpers for search-as-you-type 
   static void cbSearchEntryChanged(GtkWidget *editable, void *data);
   static void xapianIndexUpdateFinished(GPid pid, gint status, void* data);
   static gboolean xapianDoSearch(void *data);
   static gboolean xapianDoIndexUpdate(void *data);

   // RPackageObserver
   virtual void notifyChange(RPackage *pkg);
   virtual void notifyPreFilteredChange() {
   };
   virtual void notifyPostFilteredChange() {
   };

 public:
   RGMainWindow(RPackageLister *packLister, string name);
   virtual ~RGMainWindow() {};

   void refreshTable(RPackage *selectedPkg = NULL,bool setAdjustments=true);

   void changeView(int view, string subView="");

   // install the list of packagenames and display a changes window
   void selectToInstall(vector<string> packagenames);

   void setInterfaceLocked(bool flag);
   void setTreeLocked(bool flag);
   void rebuildTreeView() {
      buildTreeView();
   };

   void setStatusText(char *text = NULL);

   // this helper will bring the current active window to the foreground
   void activeWindowToForeground();

   void saveState();
   bool restoreState();

   bool showErrors();

   GtkWidget* buildWeakDependsMenu(RPackage *pkg, pkgCache::Dep::DepType);


   // --------------------------------------------------------------------
   // Callbacks
   //

   static void cbDependsMenuChanged(GtkWidget *self, void *data);

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

   static void cbChangedView(GtkWidget *self, void *);
   static void cbChangedSubView(GtkTreeSelection *selection, gpointer data);

   static void cbDetailsWindow(GtkWidget *self, void *data);

   // file menu
   static void cbTasksClicked(GtkWidget *self, void *data);
   static void cbOpenClicked(GtkWidget *self, void *data);
   static void cbSaveClicked(GtkWidget *self, void *data);
   static void cbSaveAsClicked(GtkWidget *self, void *data);
   string selectionsFilename;
   bool saveFullState;
   static void cbGenerateDownloadScriptClicked(GtkWidget *self, void *data);
   static void cbAddDownloadedFilesClicked(GtkWidget *self, void *data);
   static void cbViewLogClicked(GtkWidget *self, void *data);

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
   static void cbMenuAutoInstalledClicked(GtkWidget *self, void *data);

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
