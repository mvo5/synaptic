/* rpackagecache.h - package cache wrapper
 * 
 * Copyright (c) 2000, 2001 Conectiva S/A 
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


#ifndef _RPACKAGECACHE_H_
#define _RPACKAGECACHE_H_

#include <map>

#include <apt-pkg/depcache.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/policy.h>

class OpProgress;

class pkgCache;


class RPackageCache {
   MMap *_map;

   pkgCache *_cache;
   pkgPolicy *_policy;

   pkgDepCache *_dcache;
   pkgSourceList *_list;

   // speed up IsTrusted()
   std::map<pkgCache::PkgFileIterator, pkgIndexFile*> _trust_cache;

   bool _locked;

 public:
   inline pkgDepCache *deps() {
      return _dcache;
   };
   inline pkgSourceList *list() {
      return _list;
   };
   inline std::map<pkgCache::PkgFileIterator, pkgIndexFile*>& trust_cache() {
      return _trust_cache;
   };

   bool open(OpProgress &progress, bool lock=true);

   vector<string> getPolicyArchives(bool filenames_only);

   bool lock();
   void releaseLock();

   RPackageCache() 
     : _map(0), _cache(0), _policy(0), _dcache(0), _locked(false) 
   {
      _list = new pkgSourceList();
   };
   ~RPackageCache() {
      delete _list;
   };
};


#endif
