/* rgmisc.h
 * 
 * Copyright (c) 2003 Michael Vogt
 * 
 * Author: Michael Vogt <mvo@debian.org>
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

#ifndef _RGMISC_H_
#define _RGMISC_H_

#include <apt-pkg/configuration.h>
#include "rpackage.h"

enum {
   PIXMAP_COLUMN,
   NAME_COLUMN,
   PKG_SIZE_COLUMN,
   INSTALLED_VERSION_COLUMN,
   AVAILABLE_VERSION_COLUMN,
   DESCR_COLUMN,
   COLOR_COLUMN,
   PKG_COLUMN,
   N_COLUMNS
};

void RGFlushInterface();

bool is_binary_in_path(char *program);

char *gtk_get_string_from_color(GdkColor * colp);
void gtk_get_color_from_string(const char *cpp, GdkColor ** colp);

const char *utf8_to_locale(const char *str);
const char *utf8(const char *str);

string SizeToStr(double Bytes);

// the information for the treeview
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
   // this is the short string to load the icons
   const char *PackageStatusShortString[N_STATUS_COUNT];
   // this is the long string for the gui description of the state
   const char *PackageStatusLongString[N_STATUS_COUNT];

   GdkPixbuf *StatusPixbuf[N_STATUS_COUNT];
   GdkColor *StatusColors[N_STATUS_COUNT];

   // this does the actual work
   int getStatus(RPackage *pkg);

   void initColors();
   void initPixbufs();

 public:
   // this static object is used for all access
   static RPackageStatus pkgStatus;

   RPackageStatus() {
   };

   // this reads the pixmaps and the colors
   void init();

   // this is what the package listers use
   GdkColor *getBgColor(RPackage *pkg);
   GdkPixbuf *getPixbuf(RPackage *pkg);
   GdkPixbuf *getPixbuf(int i) {
      return StatusPixbuf[i];
   }

   // here we get the description for the States
   const char *getLongStatusString(int i) {
      return PackageStatusLongString[i];
   };
   const char *getLongStatusString(RPackage *pkg) {
      return PackageStatusLongString[getStatus(pkg)];
   };

   // this is for the configuration of the colors
   void setColor(int i, GdkColor * new_color);
   GdkColor *getColor(int i) {
      return StatusColors[i];
   };
   // save color configuration to disk
   void saveColors();
   const char *getShortStatusString(int i) {
      return PackageStatusShortString[i];
   };
};


#endif
