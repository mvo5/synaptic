/* rwfetchprogress.h
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


#include <apt-pkg/acquire.h>

#include <vector>

#include "rwwindow.h"

#include <WINGs/wtableview.h>


class RWFetchProgress : public pkgAcquireStatus, public RWWindow
{
   struct Item {
       string uri;
       string size;
       int status;
   };
   
   vector<Item> _items;

   WMTableView *_table;
   WMLabel *_statusL;
   
   bool _cancelled;

   void updateStatus(pkgAcquire::ItemDesc &Itm, int status);
   
   static void stopDownload(WMWidget *self, void *data);

   static int numberOfRows(WMTableViewDelegate *self, WMTableView *table);

   static void *valueForCell(WMTableViewDelegate *self, 
			     WMTableColumn *column, int row);
public:
   
   virtual bool MediaChange(string Media,string Drive);
   virtual void IMSHit(pkgAcquire::ItemDesc &Itm);
   virtual void Fetch(pkgAcquire::ItemDesc &Itm);
   virtual void Done(pkgAcquire::ItemDesc &Itm);
   virtual void Fail(pkgAcquire::ItemDesc &Itm);
   virtual void Start();
   virtual void Stop();

   bool Pulse(pkgAcquire *Owner);

   RWFetchProgress(RWWindow *win);
};
