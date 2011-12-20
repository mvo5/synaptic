/* rpackagestatus.h - wrapper for accessing packagestatus information
 * 
 * Copyright (c) 2000, 2001 Conectiva S/A 
 *               2002-2008 Michael Vogt <mvo@debian.org>
 * 
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <mvo@debian.org>
 * 
 * Portions Taken from Gnome APT
 *   Copyright (C) 1998 Havoc Pennington <hp@pobox.com>
 * 
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

#ifndef _RPACKAGESTATUS_H_
#define _RPACKAGESTATUS_H_

#include <time.h>
#include <vector>
#include <string>
#include <sstream>
#include <apt-pkg/configuration.h>
#include <apt-pkg/fileutl.h>

#include "i18n.h"
#include "rpackage.h"

using namespace std;

class RPackageStatus {
 public:
   enum PkgStatus {
      ToInstall, ToReInstall, ToUpgrade, ToDowngrade, ToRemove, ToPurge,
      NotInstalled, NotInstalledLocked,
      InstalledUpdated, InstalledOutdated, InstalledLocked,
      IsBroken, IsNew,
      N_STATUS_COUNT
   };

 protected:
   static char release[255];

   // the supported archive-labels and components
   vector<string> supportedLabels;
   vector<string> supportedOrigins;
   vector<string> supportedComponents;
   bool markUnsupported;

   // this is the short string to load the icons
   const char *PackageStatusShortString[N_STATUS_COUNT];
   // this is the long string for the gui description of the state
   const char *PackageStatusLongString[N_STATUS_COUNT];

   // this does the actual work
   int getStatus(RPackage *pkg);


 public:
   RPackageStatus() : markUnsupported(false) {};
   virtual ~RPackageStatus() {};

   // this reads the pixmaps and the colors
   virtual void init();

   // here we get the description for the States
   const char *getLongStatusString(int i) {
      return PackageStatusLongString[i];
   };
   const char *getLongStatusString(RPackage *pkg) {
      return PackageStatusLongString[getStatus(pkg)];
   };

   const char *getShortStatusString(int i) {
      return PackageStatusShortString[i];
   };

   bool isSupported(RPackage *pkg);

   // return the time until the package is supported
   bool maintenanceEndTime(RPackage *pkg, struct tm *support_end_tm);
};

#endif
