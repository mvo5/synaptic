/* rfetchstatus.cc - libapt download callbacks as FetchEvents
 *
 * Copyright (c) 2026 Michael Vogt <mvo@debian.org>
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

#include "config.h" // IWYU pragma: associated

#include "rfetchstatus.h"

#include <apt-pkg/acquire-item.h>
#include <apt-pkg/acquire-worker.h>

FetchItem RFetchStatus::toItem(pkgAcquire::ItemDesc &itm)
{
   // libapt leaves ID to the frontend; it keys the GUI's table rows
   if (itm.Owner->ID == 0)
      itm.Owner->ID = _nextId++;

   FetchItem item;
   item.id = itm.Owner->ID;
   item.uri = itm.URI;
   item.description = itm.Description;
   item.shortDescription = itm.ShortDesc;
   item.size = itm.Owner->FileSize;
   return item;
}

void RFetchStatus::Start()
{
   pkgAcquireStatus::Start();
   FetchEvent ev;
   ev.kind = FetchEvent::Start;
   handleFetchEvent(ev);
}

void RFetchStatus::Stop()
{
   pkgAcquireStatus::Stop();
   FetchEvent ev;
   ev.kind = FetchEvent::Stop;
   handleFetchEvent(ev);
}

void RFetchStatus::Fetch(pkgAcquire::ItemDesc &itm)
{
   FetchEvent ev;
   ev.kind = FetchEvent::Fetch;
   ev.hasItem = true;
   ev.item = toItem(itm);
   handleFetchEvent(ev);
}

void RFetchStatus::IMSHit(pkgAcquire::ItemDesc &itm)
{
   FetchEvent ev;
   ev.kind = FetchEvent::Hit;
   ev.hasItem = true;
   ev.item = toItem(itm);
   handleFetchEvent(ev);
}

void RFetchStatus::Done(pkgAcquire::ItemDesc &itm)
{
   FetchEvent ev;
   ev.kind = FetchEvent::Done;
   ev.hasItem = true;
   ev.item = toItem(itm);
   handleFetchEvent(ev);
}

void RFetchStatus::Fail(pkgAcquire::ItemDesc &itm)
{
   // an idle item was never tried, e.g. it waits for a media change
   if (itm.Owner->Status == pkgAcquire::Item::StatIdle)
      return;
   FetchEvent ev;
   ev.kind = FetchEvent::Fail;
   ev.hasItem = true;
   ev.item = toItem(itm);
   ev.error = itm.Owner->ErrorText;
   handleFetchEvent(ev);
}

bool RFetchStatus::Pulse(pkgAcquire *owner)
{
   pkgAcquireStatus::Pulse(owner);

   FetchEvent ev;
   ev.kind = FetchEvent::Pulse;
   ev.currentBytes = CurrentBytes;
   ev.totalBytes = TotalBytes;
   ev.currentCPS = CurrentCPS;
   ev.currentItems = CurrentItems;
   ev.totalItems = TotalItems;
   for (pkgAcquire::Worker *w = owner->WorkersBegin(); w != nullptr;
        w = owner->WorkerStep(w)) {
      if (w->CurrentItem == nullptr)
         continue;
      FetchWorker fw;
      fw.item = toItem(*w->CurrentItem);
      fw.current = w->CurrentItem->CurrentSize;
      fw.total = w->CurrentItem->TotalSize;
      ev.workers.push_back(fw);
   }
   return handleFetchEvent(ev);
}
