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

#include "rggladewindow.h"
#include "rgmainwindow.h"

class RGPreferencesWindow : public RGGladeWindow {
    enum {LAYOUT_VPANED, LAYOUT_HPANED} _synapticLayout;

    RGMainWindow *_mainWin;
    RPackageLister *_lister;
    // option buttons
    GtkWidget *_optionUseRegexp;
    GtkWidget *_optionUseStatusColors;
    GtkWidget *_optionAskRelated;
    GtkWidget *_optionUseTerminal;
    GtkWidget *_optionCheckRecom;
    GtkWidget *_optionAskQuit;

    // cache settings
    GtkWidget *_cacheLeave;
    GtkWidget *_cacheClean;
    GtkWidget *_cacheAutoClean;

    GtkWidget *_pathT;
    GtkWidget *_sizeT;
    GtkWidget *_maxUndoE;
    GtkWidget *_optionmenuDel;
    GtkWidget *_useProxy;

    // policy settings
    GtkWidget *_optionmenuDefaultDistro;

    /* the color buttons */
    static char * color_buttons[];
    int columnPos[6];
    
    void readColors();

    // distro selection
    static void onArchiveSelection(GtkWidget *self, void *data);
    bool distroChanged;

    // treeview stuff
    void readTreeViewValues();
    void saveTreeViewValues();
    
    // callbacks
    static void changeFontAction(GtkWidget *self, void *data);

    static void saveAction(GtkWidget *self, void *data);
    static void closeAction(GtkWidget *self, void *data);
    static void doneAction(GtkWidget *self, void *data);
    static void clearCacheAction(GtkWidget *self, void *data);
    
    static void hpanedClickedAction(GtkWidget *self, void *data);
    static void vpanedClickedAction(GtkWidget *self, void *data);

    static void colorClicked(GtkWidget *self, void *data);
    static void saveColor(GtkWidget *self, void *data);

    static void useProxyToggled(GtkWidget *self, void *data);

 public:
    RGPreferencesWindow(RGWindow *owner, RPackageLister *lister);
    virtual ~RGPreferencesWindow() {};
    virtual void show();

    // call this to set the proxy stuff for apt
    static void applyProxySettings();
};
