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
#include <apt-pkg/acquire.h>
#include "rconfiguration.h"
#include "i18n.h"

using namespace std;

class pkgDepCache;
class RPackageLister;
class pkgRecords;

enum { NO_PARSER, DEB_PARSER, STRIP_WS_PARSER, RPM_PARSER };

// taken from apt (pkgcache.cc) to make our life easier 
// (and added "RDepends" as last element)
static const char *DepTypeStr[] = 
   {"",
    _("Depends"),
    _("PreDepends"),
    _("Suggests"),
    _("Recommends"),
    _("Conflicts"),
    _("Replaces"),
    _("Obsoletes"), 
    _("Breaks"),
    _("Enhances"),
    /* padding */
    "", 
    "",
    "",
    "",
    "",
    // make sure this is always the last member
    _("Dependency of"),
};

typedef struct  {
   pkgCache::Dep::DepType type; // type as enum
   const char* name;            // target pkg name
   const char* version;         // target version  
   const char* versionComp;     // target version compare type ( << , > etc)
   bool isSatisfied;            // dependecy is satified 
   bool isVirtual;              // package is virtual
   bool isOr;                   // or dependency (with next pkg)
} DepInformation;


class RPackage {

 public:
   RPackageLister *_lister;

   protected:

   string fullname;
   pkgRecords *_records;
   pkgDepCache *_depcache;
   pkgCache::PkgIterator *_package;

   // save the default candidate version to undo version selection
   string _defaultCandVer;

   bool _notify;

   // Virtual pkgs provided by this one.
   // FIXME: broken right now 
   bool isShallowDependency(RPackage *pkg);
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
      FOverrideVersion  = 1 << 19,
      FIsAuto           = 1 << 20,
      FIsGarbage        = 1 << 21,
      FNowPolicyBroken  = 1 << 22,
      FInstPolicyBroken  = 1 << 23,
   };

   enum UpdateImportance {
      IUnknown,
      INormal,
      ICritical,
      ISecurity
   };

   pkgCache::PkgIterator *package() { return _package; };

   const char *name();

   const char *section();
   const char *priority();

   const char *summary();
   const char *description();
   const char *installedFiles();

   string arch();

   // package is also available for the native architecture
   // (note that packages installed are never considered a duplicate
   bool isMultiArchDuplicate();

   // get changelog file from the debian server 
   string getChangelogFile(pkgAcquire *fetcher);
   // get screenshot file from the debian server 
   string getScreenshotFile(pkgAcquire *fetcher, bool thumb = true);

   vector<string> provides();

   // get all available versions (version, release)
   vector<pair<string, string> > getAvailableVersions();

   // get origins url of the package (e.g. http://security.ubuntu.com)
   vector<string> getCandidateOriginSiteUrls();
   // get origin "archive" release header (e.g. karmic, karmic-updates)
   vector<string> getCandidateOriginSuites();
   // get origin "origin" release header (e.g. Ubuntu, 
   string getCandidateOriginStr();

   // get the release file for the givel origin label string
   string getReleaseFileForOrigin(string label, string release);

   // get installed component (like main, contrib, non-free)
   string component();
   
   // get label of download site
   string label();

   // get origin (Origin tag from the release file)
   string origin();
   
   const char *maintainer();
   const char *homepage();
   const char *vendor();

   const char *installedVersion();
   long installedSize();

   // get tag from pkg record
   string findTagFromPkgRecord(const char *tag);

   // get the raw package record
   string getRawRecord(bool useCandidateVersion=true);

   // sourcepkg
   const char *srcPackage();

   // relative to version that would be installed
   const char *availableVersion();
   long availableInstalledSize();
   long availablePackageSize();

   // does the pkg depends on this one?
   bool dependsOn(const char *pkgname);

   // getDeps of installed pkg
   vector<DepInformation> enumDeps(bool useCanidateVersion=false);

   // reverse dependencies
   vector<DepInformation> enumRDeps();

   int getFlags();

   bool wouldBreak();

   bool isTrusted();

   void setKeep();
   void setInstall();
   void setReInstall(bool flag);
   void setRemove(bool purge = false);

   void setPinned(bool flag);

   void setNew(bool flag = true) {
      _boolFlags = flag ? (_boolFlags | FNew) : (_boolFlags & ~FNew);
   };
   void setOrphaned(bool flag = true) {
      _boolFlags = flag ? (_boolFlags | FOrphaned) : (_boolFlags & ~FOrphaned);
   };

   // set/unset the auto-installed flag
   void setAuto(bool flag = true);

   void setNotify(bool flag = true);

   // Shallow doesnt remove things other pkgs depend on.
   void setRemoveWithDeps(bool shallow, bool purge = false);

   // mainpulate the candiate version
   bool setVersion(string verTag);
   void unsetVersion(); 
   string showWhyInstBroken();

   RPackage(RPackageLister *lister, pkgDepCache *depcache,
            pkgRecords *records, pkgCache::PkgIterator &pkg);
   ~RPackage();

   private:
   string getChangelogURI();
};


#endif

// vim:ts=3:sw=3:et
