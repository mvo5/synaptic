/* rgfiltereditor.h
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2002 Michael Vogt <mvo@debian.org>
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

#ifndef _RGFILTEREDITOR_H_
#define _RGFILTEREDITOR_H_

#include <vector>
#include <string>

#include <gtk/gtk.h>

#include "rpackagefilter.h"

struct RFilter;

// must be in the same order as of the check buttons
static RStatusPackageFilter::Types StatusMasks[] =  {
	RStatusPackageFilter::NotInstalled,
	RStatusPackageFilter::Upgradable,
	RStatusPackageFilter::Installed,
	RStatusPackageFilter::MarkKeep,
	RStatusPackageFilter::MarkInstall,
	RStatusPackageFilter::MarkRemove,
	RStatusPackageFilter::Broken,
	RStatusPackageFilter::NewPackage,
	RStatusPackageFilter::PinnedPackage,
	RStatusPackageFilter::OrphanedPackage,
	RStatusPackageFilter::ResidualConfig
};
// FIXME: if you add a new status change this const! (calc automaticlly)
#ifndef HAVE_RPM
static const int NrOfStatusBits = 11;
#else
static const int NrOfStatusBits = 10;
#endif


class RGFilterEditor
{
private:   
   static void newPatternAction(GtkWidget *w, void *data);
   static void removePatternAction(GtkWidget *w, void *data);
   static void changedAction(GtkWidget *w, void *data);   
   static void selectedPatternRow(GtkWidget *w, int row, int col,
				  GdkEvent *event);
   static void unselectedPatternRow(GtkWidget *w, int row, int col,
				  GdkEvent *event);

   
   GtkWidget *_tabview;

   RFilter *_filter;

   // group/section list
   GtkWidget *_groupL;
   GtkWidget *_inclGB;
   GtkWidget *_exclGB;

   void setSectionFilter(RSectionPackageFilter &f);
   void getSectionFilter(RSectionPackageFilter &f);

   // by status
   GtkWidget *_statusB[NrOfStatusBits];
   
   void setStatusFilter(RStatusPackageFilter &f);
   void getStatusFilter(RStatusPackageFilter &f);
   
   // by pattern
   GtkWidget *_patT;
   int selected_row;
   GtkWidget *_removePB;
   
   GtkWidget *_actionP;
   GtkWidget *_whatP;
   GtkWidget *_patternT;

   
   void setPatternFilter(RPatternPackageFilter &f);
   void getPatternFilter(RPatternPackageFilter &f);


   void makeSectionFilterPanel(GtkWidget *tabview);
   void makePatternFilterPanel(GtkWidget *tabview);
   void makeStatusFilterPanel(GtkWidget *tabview);

   
   bool getPatternRow(int row, 
		      bool &exclude,
		      RPatternPackageFilter::DepType &type,
		      string &pattern);

   bool setPatternRow(int row, 
		      bool exclude,
		      RPatternPackageFilter::DepType type,
		      string pattern);

public:
   RGFilterEditor(GtkWidget *parent);
   
   inline GtkWidget *widget() { return _tabview; };
   
   void resetFilter();
   void editFilter(RFilter *filter);
//   void makeNewFilter();
  
   void applyChanges();

   void setPackageSections(vector<string> &sections);
};


#endif
