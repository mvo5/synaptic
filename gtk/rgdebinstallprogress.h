/* rgdebinstallprogress.h
 *
 * Copyright (c) 2004 Canonical 
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


#ifndef _RGDEBINSTALLPROGRESS_H_
#define _RGDEBINSTALLPROGRESS_H_


#include "rinstallprogress.h"
#include "rggladewindow.h"
#include<map>

class RGMainWindow;


class RGDebInstallProgress:public RInstallProgress, public RGGladeWindow {

   GtkWidget *_label;
   GtkWidget *_label_status;
   GtkWidget *_labelSummary;

   GtkWidget *_pbar;
   GtkWidget *_pbarTotal;

   bool _startCounting;
   set<string> _unpackSeen;
   set<string> _configuredSeen;
   set<string> _installedSeen;
   set<string> _removeSeen;

 protected:
#if 0
   virtual pkgPackageManager::OrderResult start(pkgPackageManager *pm,
						int numPackages,
						int numPackagesTotal);
#endif
   virtual void startUpdate();
   virtual void updateInterface();
   virtual void finishUpdate();

   virtual void prepare(RPackageLister *lister);

 public:
   RGDebInstallProgress(RGMainWindow *main, RPackageLister *lister);

};

#endif
