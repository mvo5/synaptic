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

#pragma once

#include "config.h" // IWYU pragma: associated

#include "rggtkbuilderwindow.h"
#include "rpackagelister.h"
#include "rpackageview.h"
#include "coroutines.h"

#include <apt-pkg/pkgcache.h>
#include <cstddef>
#include <gdk/gdk.h>
#include <gio/gio.h>
#include <gio/gmenu.h>
#include <glib.h>
#include <glib/gtypes.h>
#include <gtk/gtk.h>
#include <gtk/gtkcssprovider.h>
#include <string>
#include <vector>

typedef enum {
   RG_TOOLBAR_HIDE = -1,
   RG_TOOLBAR_ICONS = 0,
   RG_TOOLBAR_TEXT = 1,
   RG_TOOLBAR_BOTH = 2,
   RG_TOOLBAR_BOTH_HORIZ = 3
} RGToolbarStyle;

class RGAboutPanel;
class RGFetchProgress;
class RGFilterManagerWindow;
class RGFilterWindow;
class RGFindWindow;
class RGIconLegendPanel;
class RGLogView;
class RGPkgDetailsWindow;
class RGPreferencesWindow;
class RGSetOptWindow;
class RGSourcesWindow;
class RGTasksWin;
class RGUserDialog;
class RGWindow;
class RPackage;

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

class RGMainWindow : public RGGtkBuilderWindow, public RPackageObserver
{

   typedef enum {
      UPGRADE_ASK = -1,
      UPGRADE_NORMAL = 0,
      UPGRADE_DIST = 1
   } UpgradeType;

   bool _unsavedChanges;
   bool _blockActions; // block signals from the action and hold buttons
   int _interfaceLocked;

   // the central class that has all the package information
   RPackageLister *_lister;

   // interface stuff
   RGToolbarStyle _toolbarStyle; // hide, small, normal toolbar

   GtkTreeModel *_pkgList; // the custom list model for the packages
   GtkWidget *_treeView;   // the display widget

   // the left-side view
   GtkWidget *_subViewList;

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
   static GtkCssProvider *_fastSearchCssProvider;

   // the buttons for the various views
   GtkWidget *_viewButtons[N_PACKAGE_VIEWS];

   // init stuff
   void buildInterface();
   void buildTreeView();
   bool isActionEnabled(const char *action_name);
   void setActionEnabled(const char *action_name, bool enabled);
   void setActionState(const char *action_name, GVariant *value);

   void setActionStateBool(const char *action_name, bool value)
   {
      setActionState(action_name, g_variant_new_boolean(value));
   }

   void setActionStateInt(const char *action_name, int value)
   {
      setActionState(action_name, g_variant_new_int32(value));
   }

 public:
   void activateAction(const char *action_name, GVariant *value);

 private:
   // display/table releated
   void refreshSubViewList();

   virtual void close() override;
   static void closeWin(GSimpleAction *action, GVariant *parameter, gpointer me)
   {
      ((RGMainWindow *)me)->close();
   };

   // misc
   void forgetNewPackages();

   // package info
   void updatePackageInfo(RPackage *pkg);
   RPackage *selectedPackage();
   std::string selectedSubView();

   // helpers
   [[nodiscard]] task<void> pkgAction(RGPkgAction action);
   [[nodiscard]] task<bool> askStateChange(
      RPackageLister::pkgState,
      const std::vector<RPackage *> &exclude = std::vector<RPackage *>());
   [[nodiscard]] task<bool> checkForFailedInst(
      std::vector<RPackage *> instPkgs);
   void pkgInstallHelper(RPackage *pkg,
                         bool fixBroken = true,
                         bool reInstall = false);
   [[nodiscard]] task<void> pkgRemoveHelper(RPackage *pkg,
                                            bool purge = false,
                                            bool withDeps = false);
   void pkgKeepHelper(RPackage *pkg);

   // helper for recommends/suggests
   static void pkgInstallByNameHelper(GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer data);
   [[nodiscard]] task<void> pkgInstallByName(const char *name);
   // install a non-standard version
   static void cbInstallFromVersion(GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data);
   [[nodiscard]] task<void> installFromVersion();

   // helpers for search-as-you-type
   static void cbSearchEntryChanged(GtkWidget *editable, void *data);
   static void xapianIndexUpdateFinished(GPid pid, gint status, void *data);
   static void xapianDoSearch(void *data);
   static gboolean xapianDoIndexUpdate(void *data);

   // RPackageObserver
   virtual void notifyChange(RPackage *pkg);
   virtual void notifyPreFilteredChange() {};
   virtual void notifyPostFilteredChange() {};

 public:
   RGMainWindow(GtkApplication *app,
                RPackageLister *packLister,
                std::string name);
   virtual ~RGMainWindow() {};

   void refreshTable(RPackage *selectedPkg = NULL, bool setAdjustments = true);

   [[nodiscard]] task<void> changeView(int view, std::string subView = "");

   // install the list of packagenames and display a changes window
   [[nodiscard]] task<void> selectToInstall(
      std::vector<std::string> packagenames);

   [[nodiscard]] task<void> setInterfaceLocked(bool flag);
   void setTreeLocked(bool flag);
   void rebuildTreeView()
   {
      buildTreeView();
   };

   void setStatusText(char *text = NULL);

   // this helper will bring the current active window to the foreground
   void activeWindowToForeground();

   [[nodiscard]] task<void> saveState();
   [[nodiscard]] task<bool> restoreState();

   [[nodiscard]] task<bool> showErrors();

   GMenu *buildWeakDependsMenu(RPackage *pkg, pkgCache::Dep::DepType);


   // --------------------------------------------------------------------
   // Callbacks
   //

   static void cbDependsMenuChanged(GtkWidget *self, void *data);

   void cbPkgAction(RGPkgAction action);
   static void cbPkgActionUnmark(GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer data);
   static void cbPkgActionMarkInstall(GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer data);
   static void cbPkgActionMarkReinstall(GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer data);
   static void cbPkgActionMarkUpgrade(GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer data);
   static void cbPkgActionMarkDelete(GSimpleAction *action,
                                     GVariant *parameter,
                                     gpointer data);
   static void cbPkgActionMarkPurge(GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data);
   static void cbPkgActionDefault(GSimpleAction *action,
                                  GVariant *parameter,
                                  gpointer data);

   static gboolean cbPackageListClicked(GtkWidget *treeview,
                                        GdkEventButton *event,
                                        gpointer data);

   static void cbTreeviewPopupMenu(GtkWidget *treeview,
                                   GdkEventButton *event,
                                   RGMainWindow *me,
                                   std::vector<RPackage *> selected_pkgs);

   static void cbChangelogDialog(GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer data);

   static void cbSelectedRow(GtkTreeSelection *selection, gpointer data);
   static void cbPackageListRowActivated(GtkTreeView *treeview,
                                         GtkTreePath *arg1,
                                         GtkTreeViewColumn *arg2,
                                         gpointer user_data);
   [[nodiscard]] task<void> packageListRowActivated(GtkTreeView *treeview,
                                                    GtkTreePath *path);

   static void cbChangedView(GtkWidget *self, void *);
   static void cbChangedSubView(GtkTreeSelection *selection, gpointer data);

   static void cbDetailsWindow(GSimpleAction *action,
                               GVariant *parameter,
                               gpointer data);

   // file menu
   static void cbTasksClicked(GSimpleAction *action,
                              GVariant *parameter,
                              gpointer data);
   [[nodiscard]] task<void> tasksClicked();
   static void cbOpenClicked(GSimpleAction *action,
                             GVariant *parameter,
                             gpointer data);
   [[nodiscard]] task<void> openClicked();
   static void cbSaveClicked(GSimpleAction *action,
                             GVariant *parameter,
                             gpointer data);
   [[nodiscard]] task<void> saveClicked();
   static void cbSaveAsClicked(GSimpleAction *action,
                               GVariant *parameter,
                               gpointer data);
   [[nodiscard]] task<void> saveAsClicked();
   std::string selectionsFilename;
   bool saveFullState;
   static void cbGenerateDownloadScriptClicked(GSimpleAction *action,
                                               GVariant *parameter,
                                               gpointer data);
   [[nodiscard]] task<void> generateDownloadScript();
   static void cbAddDownloadedFilesClicked(GSimpleAction *action,
                                           GVariant *parameter,
                                           gpointer data);
   [[nodiscard]] task<void> addDownloadedFiles();
   static void cbViewLogClicked(GSimpleAction *action,
                                GVariant *parameter,
                                gpointer data);

   // actions menu
   static void cbUndoClicked(GSimpleAction *action,
                             GVariant *parameter,
                             gpointer data);
   [[nodiscard]] task<void> undo();
   static void cbRedoClicked(GSimpleAction *action,
                             GVariant *parameter,
                             gpointer data);
   [[nodiscard]] task<void> redo();
   static void cbClearAllChangesClicked(GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer data);
   [[nodiscard]] task<void> clearAllChanges();
   static void cbUpdateClicked(GSimpleAction *action,
                               GVariant *parameter,
                               gpointer data);
   [[nodiscard]] task<void> updateClicked();
   static void cbAddCDROM(GSimpleAction *action,
                          GVariant *parameter,
                          gpointer data);
   [[nodiscard]] task<void> addCDROM();
   static void cbFixBrokenClicked(GSimpleAction *action,
                                  GVariant *parameter,
                                  gpointer data);
   [[nodiscard]] task<void> fixBroken();
   static void cbUpgradeClicked(GSimpleAction *action,
                                GVariant *parameter,
                                gpointer data);
   [[nodiscard]] task<void> upgrade();
   static void cbProceedClicked(GSimpleAction *action,
                                GVariant *parameter,
                                gpointer data);
   [[nodiscard]] task<void> applyChanges();

   // packages menu
   static void cbMenuPinClicked(GSimpleAction *action,
                                GVariant *parameter,
                                gpointer data);
   [[nodiscard]] task<void> pin(bool active);
   static void cbMenuAutoInstalledClicked(GSimpleAction *action,
                                          GVariant *parameter,
                                          gpointer data);
   [[nodiscard]] task<void> autoInstalled(bool active);

   // filter menu
   static void cbShowFilterManagerWindow(GSimpleAction *action,
                                         GVariant *parameter,
                                         gpointer data);
   [[nodiscard]] task<void> showFilterManagerWindow();
   static void cbSaveFilterAction(void *self, RGFilterWindow *rwin);
   static void cbCloseFilterAction(void *self, RGFilterWindow *rwin);

   // search menu
   static void cbFindToolClicked(GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer data);
   [[nodiscard]] task<void> findTool();

   // preferences menu
   static void cbShowConfigWindow(GSimpleAction *action,
                                  GVariant *parameter,
                                  gpointer data);
   static void cbShowSetOptWindow(GSimpleAction *action,
                                  GVariant *parameter,
                                  gpointer data);
   static void cbShowSourcesWindow(GSimpleAction *action,
                                   GVariant *parameter,
                                   gpointer data);
   [[nodiscard]] task<void> showSourcesWindow();
   static void cbMenuToolbarClicked(GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data);

   // help menu
   static void cbHelpAction(GSimpleAction *action,
                            GVariant *parameter,
                            gpointer data);
   [[nodiscard]] task<void> helpAction();
   static void cbShowIconLegendPanel(GSimpleAction *action,
                                     GVariant *parameter,
                                     gpointer data);
   static void cbShowAboutPanel(GSimpleAction *action,
                                GVariant *parameter,
                                gpointer data);
   static void cbShowWelcomeDialog(GSimpleAction *action,
                                   GVariant *parameter,
                                   gpointer data);
   [[nodiscard]] task<void> showWelcomeDialog();

   // the buttons
   static void cbPkgHelpClicked(GSimpleAction *action,
                                GVariant *parameter,
                                gpointer data);
   [[nodiscard]] task<void> pkgHelpClicked();
   static void cbPkgReconfigureClicked(GSimpleAction *action,
                                       GVariant *parameter,
                                       gpointer data);
   [[nodiscard]] task<void> pkgReconfigureClicked();
};
