/* rcacheactor.h
 *
 * Copyright (c) 2000-2003 Conectiva S/A
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

#pragma once

#include "config.h" // IWYU pragma: associated

#include "rpackage.h"
#include "rpackagelister.h"

#include <regex.h>
#include <vector>

class RCacheActor : public RCacheObserver
{
 public:
   enum Action { ACTION_KEEP, ACTION_INSTALL, ACTION_REINSTALL, ACTION_REMOVE };

 protected:
   RPackageLister *_lister;
   RPackageLister::pkgState *_laststate;

 public:
   virtual void run(std::vector<RPackage *> &List, int Action) = 0;

   virtual void notifyCachePreChange()
   {
      updateState();
   }

   virtual void notifyCachePostChange();

   virtual void notifyCacheOpen()
   {}

   virtual void updateState()
   {
      delete _laststate;
      _laststate = new RPackageLister::pkgState;
      _lister->saveState(*_laststate);
   }

   RCacheActor(RPackageLister *lister) : _lister(lister), _laststate(0)
   {
      _lister->registerCacheObserver(this);
   }

   virtual ~RCacheActor()
   {
      _lister->unregisterCacheObserver(this);
   }
};
