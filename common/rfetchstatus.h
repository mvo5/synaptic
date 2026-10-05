/* rfetchstatus.h - libapt download callbacks as FetchEvents
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

#pragma once

#include "config.h" // IWYU pragma: associated

#include "rfetchevent.h"

#include <apt-pkg/acquire.h>

// Turns libapt's pkgAcquireStatus callbacks into FetchEvents. The
// daemon sends them to the GUI, the GUI shows them, and the same
// translation serves both.
class RFetchStatus : public pkgAcquireStatus
{
   unsigned long _nextId = 1;

   FetchItem toItem(pkgAcquire::ItemDesc &itm);

 public:
   void Start() override;
   void Stop() override;
   void Fetch(pkgAcquire::ItemDesc &itm) override;
   void IMSHit(pkgAcquire::ItemDesc &itm) override;
   void Done(pkgAcquire::ItemDesc &itm) override;
   void Fail(pkgAcquire::ItemDesc &itm) override;
   bool Pulse(pkgAcquire *owner) override;

   // false cancels the download
   virtual bool handleFetchEvent(const FetchEvent &ev) = 0;
};
