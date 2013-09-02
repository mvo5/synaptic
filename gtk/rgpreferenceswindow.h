/* rgconfigwindow.h
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

#include "rggtkbuilderwindow.h"
#include "rgmainwindow.h"

class RGPreferencesWindow:public RGGtkBuilderWindow {
   bool _blockAction;

   enum {TREE_CHECKBOX_COLUMN, TREE_VISIBLE_NAME_COLUMN, TREE_NAME_COLUMN};

   struct column_struct {
      gboolean visible;
      const char *name;
      const char *visible_name;
   };

   GtkCssProvider *_css_provider;

   // the names for the VisibleColumnsTreeView
   static const char *column_names[];
   static const char *column_visible_names[];
   static const gboolean column_visible_defaults[];

   static const char *removal_actions[];
   static const char *update_ask[];
   static const char *upgrade_method[];

   RGMainWindow *_mainWin;
   RPackageLister *_lister;
   // option buttons
   GtkWidget *_optionShowAllPkgInfoInMain;
   GtkWidget *_optionUseStatusColors;
   GtkWidget *_optionAskRelated;
   GtkWidget *_optionUseTerminal;
   GtkWidget *_optionCheckRecom;
   GtkWidget *_optionAskQuit;
   GtkWidget *_optionOneClick;

   // cache settings
   GtkWidget *_cacheLeave;
   GtkWidget *_cacheClean;
   GtkWidget *_cacheAutoClean;

   GtkWidget *_delHistory;
   GtkWidget *_keepHistory;
   GtkWidget *_spinDelHistory;

   GtkWidget *_pathT;
   GtkWidget *_sizeT;
   GtkWidget *_maxUndoE;
   GtkWidget *_comboRemovalAction;
   GtkWidget *_comboUpdateAsk;
   GtkWidget *_comboUpgradeMethod;
   GtkWidget *_useProxy;

   // policy settings
   GtkWidget *_comboDefaultDistro;
   string _defaultDistro;

   bool _dirty;

   int columnPos[6];

   // distro selection
   static void cbArchiveSelection(GtkWidget *self, void *data);
   static void cbRadioDistributionChanged(GtkWidget *self, void *data);
   bool distroChanged;
   // the http proxy configuration
   static void cbHttpProxyEntryChanged(GtkWidget *self, void *data);

   // treeview stuff
   void readTreeViewValues();
   GtkListStore *_listColumns;
   GtkWidget *_treeView;
   static void cbMoveColumnUp(GtkWidget *self, void *data);
   static void cbMoveColumnDown(GtkWidget *self, void *data);
   static void cbToggleColumn(GtkWidget *self, char *path, void *data);

   // callbacks
   static void changeFontAction(GtkWidget *self, void *data);
   static void checkbuttonUserFontToggled(GtkWidget *self, void *data);
   static void checkbuttonUserTerminalFontToggled(GtkWidget *self,
                                                  void *data);

   static void saveAction(GtkWidget *self, void *data);
   void saveGeneral();
   void saveColumnsAndFonts();
   void saveColors();
   void saveFiles();
   void saveNetwork();
   void saveDistribution();

   void readGeneral();
   void readColumnsAndFonts();
   void readColors();
   void readFiles();
   void readNetwork();
   void readDistribution();


   static void closeAction(GtkWidget *self, void *data);
   static void doneAction(GtkWidget *self, void *data);
   static void clearCacheAction(GtkWidget *self, void *data);

   static void colorClicked(GtkWidget *self, void *data);
   static void buttonAuthenticationClicked(GtkWidget *self, void *data);

   static void useProxyToggled(GtkWidget *self, void *data);

 public:
   RGPreferencesWindow(RGWindow *owner, RPackageLister *lister);
   virtual ~RGPreferencesWindow();
   virtual void show();

   // call this to set the proxy stuff for apt
   static void applyProxySettings();
};
