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
#include "rgwindow.h"
#include "rpackagefilter.h"

class RPackageLister;
class RGFilterManagerWindow;

// must be in the same order as of the check buttons
static const RStatusPackageFilter::Types StatusMasks[] =  {
    RStatusPackageFilter::NotInstalled,
    RStatusPackageFilter::Upgradable,
    RStatusPackageFilter::Installed,
    RStatusPackageFilter::MarkKeep,
    RStatusPackageFilter::MarkInstall,
    RStatusPackageFilter::MarkRemove,
    RStatusPackageFilter::Broken,
    RStatusPackageFilter::NewPackage,
    RStatusPackageFilter::PinnedPackage,
    RStatusPackageFilter::OrphanedPackage, // debian only (for now)
    RStatusPackageFilter::ResidualConfig,
    RStatusPackageFilter::DebconfPackage   // debian only
};
// FIXME: if you add a new status change this const! (calc automaticlly)
#ifndef HAVE_RPM
static const int NrOfStatusBits = 12;
#else
static const int NrOfStatusBits = 10;
#endif

static char  *ActOptions[] = {
    _("Include"),
    _("Exclude"),
    NULL
};


static char *DepOptions[] = {
    _("are Named"),
    _("in Version"),
    _("in Description"),
    _("in Maintainer"),
    _("Depends on"), // depends, predepends etc
    _("Provides"), // provides and name
    _("Conflicts with"), // conflicts
    _("Replaces"), // replaces/obsoletes
    _("Suggests or Recommends"), // suggests/recommends
    _("Reverse Depends"), // Reverse Depends
    NULL
};



typedef void RGFilterEditorCloseAction(void *self, bool okcancel);

class RGFilterManagerWindow : public RGWindow 
{
   static void addFilterAction(GtkWidget *self, void *data);
   static void removeFilterAction(GtkWidget *self, void *data);

   static void applyFilterAction(GtkWidget *self, void *data);

   static void cancelAction(GtkWidget *self, void *data);
   static void okAction(GtkWidget *self, void *data);

   static gint deleteEventAction(GtkWidget *widget, GdkEvent  *event,
				 gpointer   data );

   static void selectAction(GtkTreeSelection *selection, gpointer data);

   // load a given filter
   void editFilter(RFilter *filter);
   // no argument -> load selected filter
   void editFilter();

   // helpers
   void setSectionFilter(RSectionPackageFilter &f);
   void getSectionFilter(RSectionPackageFilter &f);
   
   void setStatusFilter(RStatusPackageFilter &f);
   void getStatusFilter(RStatusPackageFilter &f);
   
   void setPatternFilter(RPatternPackageFilter &f);
   void getPatternFilter(RPatternPackageFilter &f);

   void setFilterView(RFilter *f);
   void getFilterView(RFilter *f);

   GtkTreePath* treeview_find_path_from_text(GtkTreeModel *model, char *text);

   // close callbacks
   RGFilterEditorCloseAction *_closeAction;
   void *_closeData;
   bool _okcancel;            // did the user click ok or cancel

   GtkWidget *_filterEntry;  /* GtkEntry */

   // the filter list
   GtkWidget *_filterList;   /* GtkTreeView */
   GtkListStore *_filterListStore;
   GtkTreePath *_selectedPath;
   RFilter *_selectedFilter;
   enum {
       NAME_COLUMN,
       FILTER_COLUMN,
       N_COLUMNS
   };

   // the section list
   GtkWidget *_sectionList;   /* GtkTreeView */
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
   GtkWidget *_patternList;   /* GtkTreeView */
   GtkListStore *_patternListStore;
   bool setPatternRow(int row, bool exclude,
		      RPatternPackageFilter::DepType type, string pattern);
   static void patternSelectionChanged(GtkTreeSelection *selection,
				       gpointer data);
   static void patternChanged(GObject *o, gpointer data);
   static void patternNew(GObject *o, gpointer data);
   static void patternDelete(GObject *o, gpointer data);
   static void statusAllClicked(GObject *o, gpointer data);
   static void statusNoneClicked(GObject *o, gpointer data);
   void applyChanges(RFilter *filter);
   static void filterNameChanged(GObject *o, gpointer data);

   // the lister is always needed
   RPackageLister *_lister;  
   vector<RFilter*> _saveFilters;

public:
   RGFilterManagerWindow(RGWindow *win, RPackageLister *lister);

   void setCloseCallback(RGFilterEditorCloseAction *action, void *data);
   virtual void close();
   virtual void show();
  
};


#endif

