/* rpackage.h - wrapper for accessing package information
 * 
 * Copyright (c) 2000, 2001 Conectiva S/A 
 *               2002 Michael Vogt <mvo@debian.org>
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



#ifndef _RPACKAGE_H_
#define _RPACKAGE_H_

#include <vector>

#include <apt-pkg/pkgcache.h>

using namespace std;

class pkgDepCache;
class RPackageLister;
class pkgRecords;

enum { NO_PARSER, DEB_PARSER, STRIP_WS_PARSER, RPM_PARSER };

class RPackage {

   protected:

   RPackageLister *_lister;

   pkgRecords *_records;
   pkgDepCache *_depcache;
   pkgCache::PkgIterator *_package;

   // save the default candidate version to undo version selection
   string _defaultCandVer;

   bool _notify;

   // Virtual pkgs provided by this one.
   vector<pkgCache::PkgIterator> _virtualPackages;

   vector<const char *> _provides;

   // Stuff for enumerators
   int _vpackI;
   pkgCache::DepIterator _rdepI;

   pkgCache::DepIterator _wdepI;
   pkgCache::DepIterator _wdepStart;
   pkgCache::DepIterator _wdepEnd;

   pkgCache::DepIterator _depI;
   pkgCache::DepIterator _depStart;
   pkgCache::DepIterator _depEnd;

   bool isShallowDependency(RPackage *pkg);

   bool isWeakDep(pkgCache::DepIterator &dep);

   int _boolFlags;

 public:

   enum Flags {
      FKeep             = 1 << 0,
      FInstall          = 1 << 1,
      FNewInstall       = 1 << 2,
      FReInstall        = 1 << 3,
      FUpgrade          = 1 << 4,
      FDowngrade        = 1 << 5,
      FRemove           = 1 << 6,
      FHeld             = 1 << 7,
      FInstalled        = 1 << 8,
      FOutdated         = 1 << 9,
      FNowBroken        = 1 << 10,
      FInstBroken       = 1 << 11,
      FOrphaned         = 1 << 12,
      FPinned           = 1 << 13,
      FNew              = 1 << 14,
      FResidualConfig   = 1 << 15,
      FNotInstallable   = 1 << 16,
      FPurge            = 1 << 17,
      FImportant        = 1 << 18,
   };

   enum UpdateImportance {
      IUnknown,
      INormal,
      ICritical,
      ISecurity
   };

   pkgCache::PkgIterator *package() { return _package; };

   inline const char *name() { return _package->Name(); };

   const char *section();
   const char *priority();

   const char *summary();
   const char *description();
   const char *installedFiles();

   vector<const char *> provides();

   // get all available versions (version, release)
   vector<pair<string, string> > getAvailableVersions();

   const char *maintainer();
   const char *vendor();

   const char *installedVersion();
   long installedSize();

   // if this is an update
   UpdateImportance updateImportance();
   const char *updateSummary();
   const char *updateDate();
   const char *updateURL();

   // relative to version that would be installed
   const char *availableVersion();
   long availableInstalledSize();
   long availablePackageSize();

   // special case: alway get the deps of the latest available version
   // (not necessary the installed one)
   bool enumAvailDeps(const char *&type, const char *&what, const char *&pkg,
                      const char *&which, char *&summary, bool &satisfied);

   // this gives the dependencies for the installed package
   vector<RPackage *> getInstalledDeps();

   // installed package if installed, scheduled/candidate if not or if marked
   bool enumDeps(const char *&type, const char *&what, const char *&pkg,
                 const char *&which, char *&summary, bool &satisfied);
   bool nextDeps(const char *&type, const char *&what, const char *&pkg,
                 const char *&which, char *&summary, bool &satisfied);

   // does the pkg depends on this one?
   bool dependsOn(const char *pkgname);

   // reverse dependencies
   bool enumRDeps(const char *&dep, const char *&what);
   bool nextRDeps(const char *&dep, const char *&what);

   // weak dependencies
   bool enumWDeps(const char *&type, const char *&what, bool &satisfied);
   bool nextWDeps(const char *&type, const char *&what, bool &satisfied);

   int getFlags();

   bool wouldBreak();

   void setKeep();
   void setInstall();
   void setReInstall(bool flag);
   void setRemove(bool purge = false);

   void setPinned(bool flag);

   void setNew(bool flag = true) {
      _boolFlags = flag ? (_boolFlags & FNew) : (_boolFlags | FNew);
   };
   void setOrphaned(bool flag = true) {
      _boolFlags = flag ? (_boolFlags | FOrphaned) : (_boolFlags & FOrphaned);
   };

   void setNotify(bool flag = true);

   // Shallow doesnt remove things other pkgs depend on.
   void setRemoveWithDeps(bool shallow, bool purge = false);

   void addVirtualPackage(pkgCache::PkgIterator dep);

   bool setVersion(string verTag);
   void unsetVersion() { setVersion(_defaultCandVer); };

   string showWhyInstBroken();

   RPackage(RPackageLister *lister, pkgDepCache *depcache,
            pkgRecords *records, pkgCache::PkgIterator &pkg);
   ~RPackage();
};


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


   // this does the actual work
   int getStatus(RPackage *pkg);


 public:
   RPackageStatus() {
   };

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
};

#endif

// vim:ts=3:sw=3:et
