/* rinstallprogress.h
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


#ifndef _RINSTALLPROGRESS_H_
#define _RINSTALLPROGRESS_H_

#include <apt-pkg/packagemanager.h>
#include "config.h"

class RInstallProgress {
 protected:
   int _stdout;
   int _stderr;
   int _childin;

   int _donePackages;
   int _numPackages;
   int _donePackagesTotal;
   int _numPackagesTotal;

   // update is finished, we can close the window
   bool _updateFinished;

   static std::string finishMsg;
   static std::string errorMsg;
   static std::string incompleteMsg; 

   virtual void startUpdate() {
   };
   virtual void updateInterface() {
   };
   virtual void finishUpdate() {
   };

 public:
   // get a str feed to the user with the result of the install run
   virtual const char* getResultStr(pkgPackageManager::OrderResult);
   virtual pkgPackageManager::OrderResult start(pkgPackageManager *pm,
                                                int numPackages = 0,
                                                int numPackagesTotal = 0);


   RInstallProgress():_donePackagesTotal(0), _numPackagesTotal(0),_updateFinished(false) {};
};


#endif
