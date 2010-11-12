/* rgfiltermanager.h
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2002,2003 Michael Vogt <mvo@debian.org>
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

#ifndef _RGFILTERMANAGER_H_
#define _RGFILTERMANAGER_H_

#include <gtk/gtk.h>
#include "rggtkbuilderwindow.h"
#include "rpackagefilter.h"
#include "rgutils.h"

#include "i18n.h"

class RPackageViewFilter;
class RGFilterManagerWindow;

// must be in the same order as of the check buttons
static const RStatusPackageFilter::Types StatusMasks[] = {
   RStatusPackageFilter::NotInstalled,
   RStatusPackageFilter::Upgradable,
   RStatusPackageFilter::Installed,
   RStatusPackageFilter::MarkKeep,
   RStatusPackageFilter::MarkInstall,
   RStatusPackageFilter::MarkRemove,
   RStatusPackageFilter::Broken,
   RStatusPackageFilter::NewPackage,
   RStatusPackageFilter::PinnedPackage,
   RStatusPackageFilter::OrphanedPackage,       // debian only
   RStatusPackageFilter::ResidualConfig,        // debian only
   RStatusPackageFilter::NotInstallable,
   RStatusPackageFilter::UpstreamUpgradable,
   RStatusPackageFilter::AutoInstalled,
   RStatusPackageFilter::Garbage,
   RStatusPackageFilter::NowPolicyBroken,
   RStatusPackageFilter::ManualInstalled,
};

static const int NrOfStatusBits = sizeof(StatusMasks)/sizeof(RStatusPackageFilter::Types);

static char *ActOptions[] = {
   _("Includes"),
   _("Excludes"),
   NULL
};


static char *DepOptions[] = {
   _("Package name"),
   _("Description"),
   _("Maintainer"),
   _("Version number"),
   _("Dependencies"),           // depends, predepends etc
   _("Provided packages"),      // provides and name
   _("Conflicting packages"),   // conflicts
   _("Replaced packages"),      // replaces/obsoletes
   _("Recommendations"), ///recommends
   _("Suggestions"),   // suggests
   _("Dependent packages"),   // Reverse Depends
   _("Origin"),                 // Origin (e.g. security.debian.org)
   _("Component"),                 // Component (e.g. main, universe)
   NULL
};



typedef void RGFilterEditorCloseAction(void *self, bool okcancel);

class RGFilterManagerWindow:public RGGtkBuilderWindow {
   static void addFilterAction(GtkWidget *self, void *data);
   static void removeFilterAction(GtkWidget *self, void *data);

   static void applyFilterAction(GtkWidget *self, void *data);

   static void cancelAction(GtkWidget *self, void *data);
   static void okAction(GtkWidget *self, void *data);

   static void includeTagAction(GtkWidget *self, void *data);
   static void excludeTagAction(GtkWidget *self, void *data);

   static gint deleteEventAction(GtkWidget *widget, GdkEvent * event,
                                 gpointer data);

   static void selectAction(GtkTreeSelection *selection, gpointer data);

   // load a given filter
   void editFilter(RFilter *filter);
   // no argument -> load selected filter
   void editFilter();

   // helpers
   void setSectionFilter(RSectionPackageFilter & f);
   void getSectionFilter(RSectionPackageFilter & f);

   void setStatusFilter(RStatusPackageFilter & f);
   void getStatusFilter(RStatusPackageFilter & f);

   void setPatternFilter(RPatternPackageFilter &f);
   void getPatternFilter(RPatternPackageFilter &f);

   GtkTreePath *treeview_find_path_from_text(GtkTreeModel *model, char *text);

   GtkWidget *_comboPatternWhat;
   GtkWidget *_comboPatternDo;
   GtkWidget *_filterEntry;     /* GtkEntry */
   GdkCursor *_busyCursor;

   GtkWidget *_filterDetailsBox;        // detail box

   // the filter list
   GtkWidget *_filterList;      /* GtkTreeView */
   GtkListStore *_filterListStore;
   GtkTreePath *_selectedPath;
   RFilter *_selectedFilter;
   enum {
      NAME_COLUMN,
      FILTER_COLUMN,
      N_COLUMNS
   };

   // the section list
   GtkWidget *_sectionList;     /* GtkTreeView */
   GtkListStore *_sectionListStore;
   enum {
      SECTION_COLUMN,
      SECTION_N_COLUMNS
   };

   // status filter buttons
   GtkWidget *_statusB[NrOfStatusBits];

   // the pattern list
   enum {
      PATTERN_DO_COLUMN,
      PATTERN_WHAT_COLUMN,
      PATTERN_TEXT_COLUMN,
      PATTERN_N_COLUMNS
   };
   GtkWidget *_patternList;     /* GtkTreeView */
   GtkListStore *_patternListStore;
   bool setPatternRow(int row, bool exclude,
                      RPatternPackageFilter::DepType type, string pattern);
   static void patternSelectionChanged(GtkTreeSelection *selection,
                                       gpointer data);
   static void patternChanged(GObject *o, gpointer data);
   static void patternNew(GObject *o, gpointer data);
   static void patternDelete(GObject *o, gpointer data);
   static void statusAllClicked(GObject *o, gpointer data);
   static void statusInvertClicked(GObject *o, gpointer data);
   static void statusNoneClicked(GObject *o, gpointer data);
   void applyChanges(RFilter *filter);
   static void filterNameChanged(GObject *o, gpointer data);
   void filterAvailableTags();

   // the view menu
   GtkWidget *_optionmenu_view_mode;
   GtkWidget *_optionmenu_expand_mode;

   // the lister is always needed
   RPackageViewFilter *_filterview;
   vector<RFilter *> _saveFilters;

#ifdef HAVE_DEBTAGS
   // the tags stuff
   GtkTreeView *_availableTagsView;
   GtkListStore *_availableTagsList;

   GtkTreeView *_includedTagsView;
   GtkListStore *_includedTagsList;

   GtkTreeView *_excludedTagsView;
   GtkListStore *_excludedTagsList;

   static void treeViewExcludeClicked(GtkTreeView *treeview,
                                      GtkTreePath *arg1,
                                      GtkTreeViewColumn *arg2,
                                      gpointer user_data);
   static void treeViewIncludeClicked(GtkTreeView *treeview,
                                      GtkTreePath *arg1,
                                      GtkTreeViewColumn *arg2,
                                      gpointer user_data);
#endif

 public:
   RGFilterManagerWindow(RGWindow *win, RPackageViewFilter *filterview);

   // need to be called before show or run_dialog
   void readFilters();

};


#endif
