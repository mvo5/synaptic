/* rpackagemanager.h
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *                     2002 Michael Vogt
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

// We need a different package manager, since we need to do the
// DoInstall() process in two steps when forking. Without that,
// the forked package manager would be updated with the new
// information, and the original Synaptic process wouldn't be
// aware about it.
//
// Very unfortunately, we *do* need to access stuff which is 
// declared as protected in the packagemanager.h header. To do
// that, we need this awful hack. Make sure to include all headers
// included by packagemanager.h before the hack, so that we reduce
// the impact of it. In the future, we must ask the APT maintainers
// to export the functionality we need, so that we may avoid this
// ugly hack.

#include <string>
#include <apt-pkg/pkgcache.h>

#define protected public
#include <apt-pkg/packagemanager.h>
#undef protected

#ifndef RPACKAGEMANAGER_H
#define RPACKAGEMANAGER_H

class RPackageManager {

   protected:

   pkgPackageManager::OrderResult Res;
   
   public:

   pkgPackageManager *pm;

   pkgPackageManager::OrderResult DoInstallPreFork() {
      Res = pm->OrderInstall();
      return Res;
   };
#ifdef WITH_DPKG_STATUSFD
   pkgPackageManager::OrderResult DoInstallPostFork(int statusFd=-1) {
      return (pm->Go(statusFd) == false) ? pkgPackageManager::Failed : Res;
   };
#else
   pkgPackageManager::OrderResult DoInstallPostFork() {
      return (pm->Go() == false) ? pkgPackageManager::Failed : Res;
   };
#endif

   RPackageManager(pkgPackageManager *pm) : pm(pm) {};
   
};

#endif

// vim:ts=3:sw=3:et
