/* rwfiltereditor.h
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

#ifndef _RWFILTEREDITOR_H_
#define _RWFILTEREDITOR_H_

#include <vector>
#include <string>

#include <WINGs/WINGs.h>
#include <WINGs/wtableview.h>

#include "rpackagefilter.h"

struct RFilter;

class RWFilterEditor
{
private:
   struct PatternFilterItem {
       int type;
       string pattern;
       bool exclude;
   };
   
   static int numberOfRows(WMTableViewDelegate *self, WMTableView *table);
   static void *valueForCell(WMTableViewDelegate *self, 
			     WMTableColumn *column, int row);
   static void setValueForCell(WMTableViewDelegate *self, 
			       WMTableColumn *column, int row, 
			       void *data);

   static void newPatternAction(WMWidget *w, void *data);
   static void removePatternAction(WMWidget *w, void *data);
   
   static void selectedPatternRow(WMWidget *w, void *data);

   
   WMTabView *_tabview;

   WMTableViewDelegate *_tableDelegate;
   
   RFilter *_filter;

   // group/section list
   WMList *_groupL;
   WMButton *_inclGB;
   WMButton *_exclGB;

   void setSectionFilter(RSectionPackageFilter &f);
   void getSectionFilter(RSectionPackageFilter &f);

   // by status
   WMButton *_statusB[7];
   
   void setStatusFilter(RStatusPackageFilter &f);
   void getStatusFilter(RStatusPackageFilter &f);
   
   // by pattern
   WMTableView *_patT;
   WMButton *_removePB;

   vector<PatternFilterItem> _patternL;
   
   void setPatternFilter(RPatternPackageFilter &f);
   void getPatternFilter(RPatternPackageFilter &f);

   
   void makeSectionFilterPanel(WMTabView *tabview);   
   void makePatternFilterPanel(WMTabView *tabview);
   void makeStatusFilterPanel(WMTabView *tabview);

   
public:
   RWFilterEditor(WMWidget *parent);
   virtual ~RWFilterEditor();
   
   inline WMWidget *widget() { return _tabview; };

   void resetFilter();
   void editFilter(RFilter *filter);
//   void makeNewFilter();
  
   void applyChanges();

   void setPackageSections(vector<string> &sections);
};


#endif
