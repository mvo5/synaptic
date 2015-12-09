/* rgpackagestatus.h - UI elements for the package status
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

#ifndef _RGPACKAGESTATUS_H_
#define _RGPACKAGESTATUS_H_


#include "rpackage.h"
#include "rpackagestatus.h"

class RGPackageStatus : public RPackageStatus {
 protected:
   GdkPixbuf *StatusPixbuf[N_STATUS_COUNT];
   GdkRGBA *StatusColors[N_STATUS_COUNT];

   GdkPixbuf *supportedPix;

   void initColors();
   void initPixbufs();

 public:
   // this static object is used for all access
   static RGPackageStatus pkgStatus;

   virtual void init();
   
   // this is what the package listers use
   GdkRGBA *getBgColor(RPackage *pkg);
   GdkPixbuf *getSupportedPix(RPackage *pkg);
   GdkPixbuf *getPixbuf(RPackage *pkg);
   GdkPixbuf *getPixbuf(int i) {
      return StatusPixbuf[i];
   }

   // this is for the configuration of the colors
   void setColor(int i, GdkRGBA * new_color);
   GdkRGBA *getColor(int i) {
      return StatusColors[i];
   };
   // save color configuration to disk
   void saveColors();
};


#endif
