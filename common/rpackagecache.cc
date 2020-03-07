/* rpackagecache.cc - package cache wrapper
 * 
 * Copyright (c) 2000-2003 Conectiva S/A 
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


#include "config.h"

#include "rpackagecache.h"
#include "rconfiguration.h"
#include "i18n.h"

#include <assert.h>
#include <algorithm>
#include <apt-pkg/error.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/pkgcachegen.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/policy.h>
#include <apt-pkg/fileutl.h>


bool RPackageCache::open(OpProgress *progress, bool locking)
{
   if(locking)
      lock();

   if (_error->PendingError())
      return false;

   cache.Close();

   if (!cache.Open(progress, false))
      return false;

   if (ReadPinFile(*cache.GetPolicy(), RStateDir() + "/preferences") == false)
      return false;

   _trust_cache.clear();

   //progress.Done();
   if (_error->PendingError())
      return false;

   // Check that the system is OK
   if (deps()->DelCount() != 0 || deps()->InstCount() != 0)
      return _error->Error(_("Internal Error, non-zero counts"));

   return true;
}

vector<string> RPackageCache::getPolicyArchives(bool filenames_only=false)
{
   //std::cout << "RPackageCache::getPolicyComponents() " << std::endl;

   vector<string> archives;
   for (pkgCache::PkgFileIterator F = pkgCache(cache).FileBegin(); F.end() == false;
        F++) {
      pkgIndexFile *Indx;
      list()->FindIndex(F, Indx);
      _system->FindIndex(F, Indx);

      if(filenames_only) {
	 if(F.FileName())
	    archives.push_back(F.FileName());
      } else {
	 if (!F.RelStr().empty()) {
	    //printf("Archive: %s, Origin: %s, Component: %s, Filename: %s\n", 
	    //       F.Archive(), F.Origin(), F.Component(), F.FileName());
	    if (F.Archive() != NULL) {
	       if (find(archives.begin(), archives.end(), F.Archive())
		   == archives.end()) {
		  archives.push_back(F.Archive());
	       }
	    }
	 }
      }
   }
   return archives;
}


bool RPackageCache::lock()
{
   if (_locked)
      return true;

   _system->Lock();
   _locked = true;

   //FIXME: should depend on the result of _system->lock()
   return true;
}


void RPackageCache::releaseLock()
{
   if (!_locked)
      return;

   _system->UnLock();
   _locked = false;
}
