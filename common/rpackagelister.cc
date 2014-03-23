/* rpackagelister.cc - package cache and list manipulation
 * 
 * Copyright (c) 2000-2003 Conectiva S/A 
 *               2002-2004 Michael Vogt <mvo@debian.org>
 * 
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <mvo@debian.org>
 * 
 * Portions Taken from apt-get
 *   Copyright (C) Jason Gunthorpe
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

#include <cassert>
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <algorithm>

#include "rpackagelister.h"
#include "rpackagecache.h"
#include "rpackagefilter.h"
#include "rconfiguration.h"
#include "raptoptions.h"
#include "rinstallprogress.h"
#include "rcacheactor.h"

#include <apt-pkg/error.h>
#include <apt-pkg/progress.h>
#include <apt-pkg/algorithms.h>
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/acquire.h>
#include <apt-pkg/acquire-item.h>
#include <apt-pkg/clean.h>
#include <apt-pkg/version.h>

#include <apt-pkg/sourcelist.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/md5.h>
#ifndef HAVE_RPM
#include <apt-pkg/debfile.h>
#endif

#ifdef WITH_LUA
#include <apt-pkg/luaiface.h>
#endif

#include <algorithm>
#include <cstdio>

#include "sections_trans.h"

#include "i18n.h"

using namespace std;

RPackageLister::RPackageLister()
   : _records(0), _progMeter(new OpProgress)
#ifdef WITH_EPT
   , _xapianDatabase(0)
#endif
{
   _cache = new RPackageCache();

   _searchData.pattern = NULL;
   _searchData.isRegex = false;
   _viewMode = _config->FindI("Synaptic::ViewMode", 0);
   _updating = true;
   _sortMode = LIST_SORT_DEFAULT;

   // keep order in sync with rpackageview.h 
   _views.push_back(new RPackageViewSections(_nativeArchPackages));
   _views.push_back(new RPackageViewStatus(_nativeArchPackages));
   _views.push_back(new RPackageViewOrigin(_nativeArchPackages));
   _filterView = new RPackageViewFilter(_nativeArchPackages);
   _views.push_back(_filterView);
   _searchView =  new RPackageViewSearch(_nativeArchPackages);
   _views.push_back(_searchView);
   // its import that we use "_packages" here instead of _nativeArchPackages
   _views.push_back(new RPackageViewArchitecture(_packages));
#ifdef WITH_EPT
   openXapianIndex();
#endif

   if (_viewMode >= _views.size())
      _viewMode = 0;
   _selectedView = _views[_viewMode];

   memset(&_searchData, 0, sizeof(_searchData));

   _pkgStatus.init();

   cleanCommitLog();
#if 0
   string Recommends = _config->Find("Synaptic::RecommendsFile",
                                     "/etc/apt/metadata");
   if (FileExists(Recommends))
      _actors.push_back(new RCacheActorRecommends(this, Recommends));
#endif
}

RPackageLister::~RPackageLister()
{
   for (vector<RCacheActor *>::iterator I = _actors.begin();
        I != _actors.end(); I++)
      delete(*I);

   delete _cache;
}

void RPackageLister::setView(unsigned int index)
{
   // only save this config if it is not a search
   if(index != PACKAGE_VIEW_SEARCH)
      _config->Set("Synaptic::ViewMode", index);

   if(index < _views.size())
      _selectedView = _views[index];
   else
      _selectedView = _views[0];
}

vector<string> RPackageLister::getViews()
{
   vector<string> views;
   for (unsigned int i = 0; i != _views.size(); i++)
      views.push_back(_views[i]->getName());
   return views;
}

vector<string> RPackageLister::getSubViews()
{
   return _selectedView->getSubViews();
}

bool RPackageLister::setSubView(string newSubView)
{
   if(_config->FindB("Debug::Synaptic::View",false))
      ioprintf(clog, "RPackageLister::setSubView(): newSubView '%s'\n", 
	       newSubView.size() > 0 ? newSubView.c_str() : "(empty)");

   if(newSubView.empty())
      _selectedView->showAll();
   else
      _selectedView->setSelected(newSubView);

   notifyChange(NULL);

   if(_config->FindB("Debug::Synaptic::View",false))
      ioprintf(clog, "/RPackageLister::setSubView(): newSubView '%s'\n", 
	       newSubView.size() > 0 ? newSubView.c_str() : "(empty)");

   return true;
}

static string getServerErrorMessage(string errm)
{
   string msg;
   string::size_type pos = errm.find("server said");
   if (pos != string::npos) {
      msg = string(errm.c_str() + pos + sizeof("server said"));
      if (msg[0] == ' ')
         msg = msg.c_str() + 1;
   }
   return msg;
}

void RPackageLister::notifyPreChange(RPackage *pkg)
{
   for (vector<RPackageObserver *>::const_iterator I =
        _packageObservers.begin(); I != _packageObservers.end(); I++) {
      (*I)->notifyPreFilteredChange();
   }
}

void RPackageLister::notifyPostChange(RPackage *pkg)
{
   if(_config->FindB("Debug::Synaptic::View",false))
      ioprintf(clog, "RPackageLister::notifyPostChange(): '%s'\n",
	       pkg == NULL ? "NULL" : pkg->name());

   reapplyFilter();

   for (vector<RPackageObserver *>::const_iterator I =
        _packageObservers.begin(); I != _packageObservers.end(); I++) {
      (*I)->notifyPostFilteredChange();
   }

   if (pkg != NULL) {
      for (vector<RPackageObserver *>::const_iterator I =
           _packageObservers.begin(); I != _packageObservers.end(); I++) {
         (*I)->notifyChange(pkg);
      }
   }
}

void RPackageLister::notifyChange(RPackage *pkg)
{
   notifyPreChange(pkg);
   notifyPostChange(pkg);
}

void RPackageLister::unregisterObserver(RPackageObserver *observer)
{

   vector<RPackageObserver *>::iterator I;
   I = find(_packageObservers.begin(), _packageObservers.end(), observer);
   if (I != _packageObservers.end())
      _packageObservers.erase(I);
}

void RPackageLister::registerObserver(RPackageObserver *observer)
{
   _packageObservers.push_back(observer);
}


void RPackageLister::notifyCacheOpen()
{
   undoStack.clear();
   redoStack.clear();
   for (vector<RCacheObserver *>::const_iterator I =
        _cacheObservers.begin(); I != _cacheObservers.end(); I++) {
      (*I)->notifyCacheOpen();
   }

}

void RPackageLister::notifyCachePreChange()
{
   for (vector<RCacheObserver *>::const_iterator I =
        _cacheObservers.begin(); I != _cacheObservers.end(); I++) {
      (*I)->notifyCachePreChange();
   }
}

void RPackageLister::notifyCachePostChange()
{
   for (vector<RCacheObserver *>::const_iterator I =
        _cacheObservers.begin(); I != _cacheObservers.end(); I++) {
      (*I)->notifyCachePostChange();
   }
}

void RPackageLister::unregisterCacheObserver(RCacheObserver *observer)
{
   vector<RCacheObserver *>::iterator I;
   I = find(_cacheObservers.begin(), _cacheObservers.end(), observer);
   if (I != _cacheObservers.end())
      _cacheObservers.erase(I);
}

void RPackageLister::registerCacheObserver(RCacheObserver *observer)
{
   _cacheObservers.push_back(observer);
}

bool RPackageLister::check()
{
   if (_cache->deps() == NULL)
      return false;
   // Nothing is broken
   if (_cache->deps()->BrokenCount() != 0) {
      return false;
   }

   return true;
}

bool RPackageLister::upgradable()
{
   return _cache != NULL && _cache->deps() != NULL;
}

bool RPackageLister::openCache()
{
   static bool firstRun = true;

   if(_config->FindB("Debug::Synaptic::View",false))
      clog << "RPackageLister::openCache()" << endl;

   // Flush old errors
   _error->Discard();

   // only lock if we run as root
   bool lock = true;
   if(getuid() != 0)
      lock = false;

   if (!_cache->open(*_progMeter,lock)) {
      _progMeter->Done();
      _cacheValid = false;
      return _error->Error("_cache->open() failed, please report.");
   }
   _progMeter->Done();

   pkgDepCache *deps = _cache->deps();

   // Apply corrections for half-installed packages
   if (pkgApplyStatus(*deps) == false) {
      _cacheValid = false;
      return _error->Error(_("Internal error opening cache (%d). "
                             "Please report."), 1);
   }

   if (_error->PendingError()) {
      _cacheValid = false;
      return _error->Error(_("Internal error opening cache (%d). "
                             "Please report."), 2);
   }

   if (_records)
      delete _records;
   _records = new pkgRecords(*deps);

   if (_error->PendingError()) {
      _cacheValid = false;
      return _error->Error(_("Internal error opening cache (%d). "
                             "Please report."), 3);
   }

   for (vector<RPackage *>::iterator I = _packages.begin();
        I != _packages.end(); I++)
      delete (*I);

   int packageCount = deps->Head().PackageCount;
   _packages.clear();
   _packages.reserve(packageCount);

   _nativeArchPackages.clear();
   _nativeArchPackages.reserve(packageCount);

   _packagesIndex.clear();
   _packagesIndex.resize(packageCount, -1);

   string pkgName;
   int count = 0;

   bool showAllMultiArch = _config->FindB("Synaptic::ShowAllMultiArch", false);

   _installedCount = 0;

   map<string, RPackage *> pkgmap;
   set<string> sectionSet;

   for (unsigned int i = 0; i != _views.size(); i++)
      _views[i]->clear();

   pkgCache::PkgIterator I;
   for (I = deps->PkgBegin(); I.end() != true; I++) {

      if (I->CurrentVer != 0)
         _installedCount++;
      else if (I->VersionList == 0)
         continue; // Exclude virtual packages.

      RPackage *pkg = new RPackage(this, deps, _records, I);
      _packagesIndex[I->ID] = count;
      _packages.push_back(pkg);
      count++;

      // this is what is feed to the views
      if (showAllMultiArch || !pkg->isMultiArchDuplicate())
         _nativeArchPackages.push_back(pkg);

      pkgName = pkg->name();

      pkgmap[pkgName] = pkg;

      // Find out about new packages.
      if (firstRun) {
         packageNames.insert(pkgName);
	 // check for saved-new status
	 if (_roptions->getPackageNew(pkgName.c_str()))
	    pkg->setNew(true);
      } else if (packageNames.find(pkgName) == packageNames.end()) {
         pkg->setNew();
         _roptions->setPackageNew(pkgName.c_str());
         packageNames.insert(pkgName);
      } else  if (_roptions->getPackageNew(pkgName.c_str())) {
	 pkg->setNew(true);
      }

      if (_roptions->getPackageLock(pkgName.c_str())) 
	 pkg->setPinned(true);

      // Gather list of sections.
      if (I.Section())
         sectionSet.insert(I.Section());
#if 0 // may confuse users
      else
         cerr << "Package " << I.Name() << " has no section?!" << endl;
#endif
   }

   // refresh the views
   for (unsigned int i = 0; i != _views.size(); i++)
      _views[i]->refresh();

   applyInitialSelection();

   _updating = false;

   reapplyFilter();

   // mvo: put it here for now
   notifyCacheOpen();

   firstRun = false;

   _cacheValid = true;
   return true;
}

#ifdef WITH_EPT
bool RPackageLister::xapianIndexNeedsUpdate()
{
   struct stat buf;
   
   if(_config->FindB("Debug::Synaptic::Xapian",false))
      std::cerr << "xapainIndexNeedsUpdate()" << std::endl;

   // check the xapian index
   if(FileExists("/usr/sbin/update-apt-xapian-index") && 
      (!_xapianDatabase )) {
      if(_config->FindB("Debug::Synaptic::Xapian",false))
	 std::cerr << "xapain index not build yet" << std::endl;
      return true;
   } 

   // compare timestamps, rebuild everytime, its now cheap(er) 
   // because we use u-a-x-i --update
   stat(_config->FindFile("Dir::Cache::pkgcache").c_str(), &buf);
   if(ept::axi::timestamp() < buf.st_mtime) {
      if(_config->FindB("Debug::Synaptic::Xapian",false))
	 std::cerr << "xapian outdated " 
		   << buf.st_mtime - ept::axi::timestamp()  << std::endl;
      return true;
   }

   return false;
}

bool RPackageLister::openXapianIndex()
{
   if(_xapianDatabase)
      delete _xapianDatabase;
   try {
      _xapianDatabase = new Xapian::Database(ept::axi::path_db());
   } catch (Xapian::DatabaseOpeningError) {
      return false;
   };
   return true;
}
#endif

void RPackageLister::applyInitialSelection()
{
   _roptions->rereadOrphaned();
   _roptions->rereadDebconf();

   for (unsigned i = 0; i < _packages.size(); i++) {

      if (_roptions->getPackageOrphaned(_packages[i]->name()))
         _packages[i]->setOrphaned(true);
   }
}

RPackage *RPackageLister::getPackage(pkgCache::PkgIterator &iter)
{
   if (iter->ID > _packagesIndex.size()) {
      //std::cerr << "requesting ID " << iter->ID << " but packageIndex size is only " << _packagesIndex.size() << std::endl;
      return NULL;
   }
   int index = _packagesIndex[iter->ID];
   if (index != -1)
      return _packages[index];
   return NULL;
}

RPackage *RPackageLister::getPackage(string name)
{
   pkgCache::PkgIterator pkg = _cache->deps()->FindPkg(name);
   if (pkg.end() == false)
      return getPackage(pkg);
   return NULL;
}

int RPackageLister::getPackageIndex(RPackage *pkg)
{
   return _packagesIndex[(*pkg->package())->ID];
}

int RPackageLister::getViewPackageIndex(RPackage *pkg)
{
   return _viewPackagesIndex[(*pkg->package())->ID];
}

bool RPackageLister::fixBroken()
{
   if (_cache->deps() == NULL)
      return false;

   if (_cache->deps()->BrokenCount() == 0)
      return true;

   if (pkgFixBroken(*_cache->deps()) == false
       || _cache->deps()->BrokenCount() != 0)
      return _error->Error(_("Unable to correct dependencies"));
   if (pkgMinimizeUpgrade(*_cache->deps()) == false)
      return _error->Error(_("Unable to mark upgrades\nCheck your system for errors."));

   reapplyFilter();

   return true;
}


bool RPackageLister::upgrade()
{
   if (pkgAllUpgrade(*_cache->deps()) == false) {
      return _error->
         Error(_("Internal Error, AllUpgrade broke stuff. Please report."));
   }
#ifdef WITH_LUA
   _lua->SetDepCache(_cache->deps());
   _lua->RunScripts("Scripts::Synaptic::Upgrade", false);
   _lua->ResetCaches();
#endif

   //reapplyFilter();
   notifyChange(NULL);

   return true;
}


bool RPackageLister::distUpgrade()
{
   if (pkgDistUpgrade(*_cache->deps()) == false) {
      cout << _("dist upgrade Failed") << endl;
      return false;
   }
#ifdef WITH_LUA
   _lua->SetDepCache(_cache->deps());
   _lua->RunScripts("Scripts::Synaptic::DistUpgrade", false);
   _lua->ResetCaches();
#endif

   //reapplyFilter();
   notifyChange(NULL);

   return true;
}

void RPackageLister::reapplyFilter()
{
   // PORTME
   if (_updating)
      return;

   if(_config->FindB("Debug::Synaptic::View",false))
      clog << "RPackageLister::reapplyFilter()" << endl;

   _selectedView->refresh();
   _viewPackages.clear();
   _viewPackagesIndex.clear();
   _viewPackagesIndex.resize(_packagesIndex.size(), -1);

   for (RPackageView::iterator I = _selectedView->begin();
        I != _selectedView->end(); I++) {
      if (*I) {
         _viewPackagesIndex[(*(*I)->package())->ID] = _viewPackages.size();
         _viewPackages.push_back(*I);
      }
   }


   sortPackages(_sortMode);
}

static const int status_sort_magic = (  RPackage::FInstalled 
				      | RPackage::FOutdated 
				      | RPackage::FNew);
struct statusSortFunc {
   bool operator() (RPackage *x, RPackage *y) {
      return (x->getFlags() & (status_sort_magic))  <
	     (y->getFlags() & (status_sort_magic));
}};

struct instSizeSortFunc {
   bool operator() (RPackage *x, RPackage *y) {
      return x->installedSize() < y->installedSize();
}};

struct dlSizeSortFunc {
   bool operator() (RPackage *x, RPackage *y) {
      return x->availablePackageSize() < y->availablePackageSize();
}};

struct componentSortFunc {
   bool operator() (RPackage *x, RPackage *y) {
      return x->component() < y->component();
}};

struct sectionSortFunc {
   bool operator() (RPackage *x, RPackage *y) {
      return std::strcmp(x->section(), y->section())<0;
}};

// version string compare
int verstrcmp(const char *x, const char *y)
{
   if(x && y) 
      return _system->VS->CmpVersion(x, y) < 0;
   
   // if we compare with a non-existring version
   if(y == NULL)
      return false;
   return true;
}

struct versionSortFunc {
   bool operator() (RPackage *x, RPackage *y) {
      return verstrcmp(y->availableVersion(),
		       x->availableVersion());
   }
};

struct instVersionSortFunc {
   bool operator() (RPackage *x, RPackage *y) {
      return verstrcmp(y->installedVersion(),
		       x->installedVersion());
}};

struct supportedPartFunc {
 protected:
   bool _ascent;
   RPackageStatus _status;
 public:
   supportedPartFunc(bool ascent, RPackageStatus &s) 
      : _ascent(ascent), _status(s) {};
   bool operator() (RPackage *x) {
      return (_ascent==_status.isSupported(x));
   }
};

struct nameSortFunc {
   bool operator() (RPackage *x, RPackage *y) {
      return std::strcmp(x->name(), y->name())<0;
}};



void RPackageLister::sortPackages(vector<RPackage *> &packages, 
				  listSortMode mode)
{
   _sortMode = mode;
   if (packages.empty())
      return;

   if(_config->FindB("Debug::Synaptic::View",false))
      clog << "RPackageLister::sortPackages(): " << packages.size() << endl;

   /* Always sort by name to have packages ordered inside another sort 
    * criteria */
   sort(packages.begin(), packages.end(), 
      sortFunc<nameSortFunc>(true));

   switch(mode) {
   case LIST_SORT_NAME_ASC:
   case LIST_SORT_DEFAULT:
      // Do nothing, already done
      break;
   case LIST_SORT_NAME_DES:
      sort(packages.begin(), packages.end(), 
		  sortFunc<nameSortFunc>(false));
      break;
   case LIST_SORT_SIZE_ASC:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<instSizeSortFunc>(true));
      break;
   case LIST_SORT_SIZE_DES:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<instSizeSortFunc>(false));
      break;
   case LIST_SORT_DLSIZE_ASC:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<dlSizeSortFunc>(true));
      break;
   case LIST_SORT_DLSIZE_DES:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<dlSizeSortFunc>(false));
      break;
   case LIST_SORT_COMPONENT_ASC:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<componentSortFunc>(true));
      break;
   case LIST_SORT_COMPONENT_DES:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<componentSortFunc>(false));
      break;
   case LIST_SORT_SECTION_ASC:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<sectionSortFunc>(true));
      break;
   case LIST_SORT_SECTION_DES:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<sectionSortFunc>(false));
      break;
   case LIST_SORT_STATUS_ASC:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<statusSortFunc>(true));
      break;
   case LIST_SORT_STATUS_DES:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<statusSortFunc>(false));
      break;
   case LIST_SORT_SUPPORTED_ASC:
      stable_partition(packages.begin(), packages.end(), 
		  supportedPartFunc(false, _pkgStatus));
      break;
   case LIST_SORT_SUPPORTED_DES:
      stable_partition(packages.begin(), packages.end(), 
		  supportedPartFunc(true, _pkgStatus));
      break;
   case LIST_SORT_VERSION_ASC:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<versionSortFunc>(true));
      break;
   case LIST_SORT_VERSION_DES:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<versionSortFunc>(false));
      break;
   case LIST_SORT_INST_VERSION_ASC:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<instVersionSortFunc>(true));
      break;
   case LIST_SORT_INST_VERSION_DES:
      stable_sort(packages.begin(), packages.end(), 
		  sortFunc<instVersionSortFunc>(false));
      break;
   }
}

int RPackageLister::findPackage(const char *pattern)
{
   if (_searchData.isRegex)
      regfree(&_searchData.regex);

   if (_searchData.pattern)
      free(_searchData.pattern);

   _searchData.pattern = strdup(pattern);

   if (!_config->FindB("Synaptic::UseRegexp", false) ||
       regcomp(&_searchData.regex, pattern, REG_EXTENDED | REG_ICASE) != 0) {
      _searchData.isRegex = false;
   } else {
      _searchData.isRegex = true;
   }
   _searchData.last = -1;

   return findNextPackage();
}


int RPackageLister::findNextPackage()
{
   if (!_searchData.pattern) {
      if (_searchData.last >= (int)_viewPackages.size())
         _searchData.last = -1;
      return ++_searchData.last;
   }

   int len = strlen(_searchData.pattern);

   for (unsigned i = _searchData.last + 1; i < _viewPackages.size(); i++) {
      if (_searchData.isRegex) {
         if (regexec(&_searchData.regex, _viewPackages[i]->name(),
                     0, NULL, 0) == 0) {
            _searchData.last = i;
            return i;
         }
      } else {
         if (strncasecmp(_searchData.pattern, _viewPackages[i]->name(),
                         len) == 0) {
            _searchData.last = i;
            return i;
         }
      }
   }
   return -1;
}

void RPackageLister::getStats(int &installed, int &broken,
                              int &toInstall, int &toReInstall,
			      int &toRemove,  double &sizeChange)
{
   pkgDepCache *deps = _cache->deps();

   if (deps != NULL) {
      sizeChange = deps->UsrSize();
      
      //FIXME: toReInstall not reported?
      installed = _installedCount;
      broken = deps->BrokenCount();
      toInstall = deps->InstCount();
      toRemove = deps->DelCount();
   } else
      sizeChange = installed = broken = toInstall = toRemove = 0;
}


void RPackageLister::getDownloadSummary(int &dlCount, double &dlSize)
{
   dlCount = 0;
   dlSize = _cache->deps()->DebSize();

   pkgAcquire Fetcher;
   pkgPackageManager *PM = _system->CreatePM(_cache->deps());
   if (!PM->GetArchives(&Fetcher, _cache->list(), _records)) {
      delete PM;
      return;
   }
   dlSize = Fetcher.FetchNeeded();
   delete PM;
}


void RPackageLister::getSummary(int &held, int &kept, int &essential,
                                int &toInstall, int &toReInstall,
				int &toUpgrade, int &toRemove,
                                int &toDowngrade, int &unauthenticated,
				double &sizeChange)
{
   pkgDepCache *deps = _cache->deps();
   unsigned i;

   held = 0;
   kept = 0;
   essential = 0;
   toInstall = 0;
   toReInstall = 0;
   toUpgrade = 0;
   toDowngrade = 0;
   toRemove = 0;
   unauthenticated =0;

   for (i = 0; i < _packages.size(); i++) {
      int flags = _packages[i]->getFlags();

      // These flags will never be set together.
      int status = flags & (RPackage::FKeep |
                            RPackage::FNewInstall |
                            RPackage::FReInstall |
                            RPackage::FUpgrade |
                            RPackage::FDowngrade |
                            RPackage::FRemove);

#ifdef WITH_APT_AUTH
      switch(status) {
      case RPackage::FNewInstall:
      case RPackage::FInstall:
      case RPackage::FReInstall:
      case RPackage::FUpgrade:
	 if(!_packages[i]->isTrusted()) 
	    unauthenticated++;
	 break;
      }
#endif

      switch (status) {
         case RPackage::FKeep:
            if (flags & RPackage::FHeld)
               held++;
            else
               kept++;
            break;
         case RPackage::FNewInstall:
            toInstall++;
            break;
         case RPackage::FReInstall:
            toReInstall++;
            break;
         case RPackage::FUpgrade:
            toUpgrade++;
            break;
         case RPackage::FDowngrade:
            toDowngrade++;
            break;
         case RPackage::FRemove:
            if (flags & RPackage::FImportant)
               essential++;
            toRemove++;
            break;
      }
   }

   sizeChange = deps->UsrSize();
}



struct bla:public binary_function<RPackage *, RPackage *, bool> {
   bool operator() (RPackage *a, RPackage *b) {
      return strcmp(a->name(), b->name()) < 0;
   }
};

void RPackageLister::saveUndoState(pkgState &state)
{
   undoStack.push_front(state);
   redoStack.clear();

#ifdef HAVE_RPM
   unsigned int maxStackSize = _config->FindI("Synaptic::undoStackSize", 3);
#else
   unsigned int maxStackSize = _config->FindI("Synaptic::undoStackSize", 20);
#endif
   while (undoStack.size() > maxStackSize)
      undoStack.pop_back();
}

void RPackageLister::saveUndoState()
{
   pkgState state;
   saveState(state);
   saveUndoState(state);
}


void RPackageLister::undo()
{
   pkgState state;
   if (undoStack.empty())
      return;

   saveState(state);
   redoStack.push_front(state);

   state = undoStack.front();
   undoStack.pop_front();

   restoreState(state);
}

void RPackageLister::redo()
{
   //cout << "RPackageLister::redo()" << endl;
   pkgState state;
   if (redoStack.empty()) {
      //cout << "redoStack empty" << endl;
      return;
   }
   saveState(state);
   undoStack.push_front(state);

   state = redoStack.front();
   redoStack.pop_front();
   restoreState(state);
}


#ifdef HAVE_RPM
void RPackageLister::saveState(RPackageLister::pkgState &state)
{
   state.Save(_cache->deps());
}

void RPackageLister::restoreState(RPackageLister::pkgState &state)
{
   state.Restore();
}

bool RPackageLister::getStateChanges(RPackageLister::pkgState &state,
                                     vector<RPackage *> &toKeep,
                                     vector<RPackage *> &toInstall,
                                     vector<RPackage *> &toReInstall,
                                     vector<RPackage *> &toUpgrade,
                                     vector<RPackage *> &toRemove,
                                     vector<RPackage *> &toDowngrade,
				     vector<RPackage *> &notAuthenticated,
                                     const vector<RPackage *> &exclude,
                                     bool sorted)
{
   bool changed = false;

   state.UnIgnoreAll();
   if (exclude.empty() == false) {
      for (vector<RPackage *>::iterator I = exclude.begin();
           I != exclude.end(); I++)
         state.Ignore(*(*I)->package());
   }

   pkgDepCache &Cache = *_cache->deps();
   for (unsigned int i = 0; i != _packages.size(); i++) {
      RPackage *RPkg = _packages[i];
      pkgCache::PkgIterator &Pkg = *RPkg->package();
      pkgDepCache::StateCache & PkgState = Cache[Pkg];
      if (PkgState.Mode != state[Pkg].Mode && state.Ignored(Pkg) == false) {
         if (PkgState.Upgrade()) {
            toUpgrade.push_back(RPkg);
            changed = true;
         } else if (PkgState.NewInstall()) {
            toInstall.push_back(RPkg);
            changed = true;
         } else if (PkgState.Install()) {
	     toReInstall.push_back(RPkg);
	     changed = true;
	 } else if (PkgState.Delete()) {
            toRemove.push_back(RPkg);
            changed = true;
         } else if (PkgState.Keep()) {
            toKeep.push_back(RPkg);
            changed = true;
         } else if (PkgState.Downgrade()) {
            toDowngrade.push_back(RPkg);
            changed = true;
         }
      }
   }

   if (sorted == true && changed == true) {
      if (toKeep.empty() == false)
         sort(toKeep.begin(), toKeep.end(), bla());
      if (toInstall.empty() == false)
         sort(toInstall.begin(), toInstall.end(), bla());
      if (toReInstall.empty() == false)
         sort(toReInstall.begin(), toReInstall.end(), bla());
      if (toUpgrade.empty() == false)
         sort(toUpgrade.begin(), toUpgrade.end(), bla());
      if (toRemove.empty() == false)
         sort(toRemove.begin(), toRemove.end(), bla());
      if (toDowngrade.empty() == false)
         sort(toDowngrade.begin(), toDowngrade.end(), bla());
   }

   return changed;
}
#else
void RPackageLister::saveState(RPackageLister::pkgState &state)
{
   //cout << "RPackageLister::saveState()" << endl;
   state.clear();
   state.reserve(_packages.size());
   for (unsigned i = 0; i < _packages.size(); i++) {
      state.push_back(_packages[i]->getFlags());
   }
}

void RPackageLister::restoreState(RPackageLister::pkgState &state)
{
   pkgDepCache *deps = _cache->deps();
   pkgDepCache::ActionGroup group(*deps);

   for (unsigned i = 0; i < _packages.size(); i++) {
      RPackage *pkg = _packages[i];
      int flags = pkg->getFlags();
      int oldflags = state[i];

      if (oldflags != flags) {
         if (oldflags & RPackage::FReInstall) {
            deps->MarkInstall(*(pkg->package()), true);
            deps->SetReInstall(*(pkg->package()), false);
         } else if (oldflags & RPackage::FInstall) {
            deps->MarkInstall(*(pkg->package()), true);
         } else if (oldflags & RPackage::FRemove) {
            deps->MarkDelete(*(pkg->package()), oldflags & RPackage::FPurge);
         } else if (oldflags & RPackage::FKeep) {
            deps->MarkKeep(*(pkg->package()), false);
	 }
	 // fix the auto flag
	 deps->MarkAuto(*pkg->package(), (oldflags & RPackage::FIsAuto));
      }
   }
   notifyChange(NULL);
}


bool RPackageLister::getStateChanges(RPackageLister::pkgState &state,
                                     vector<RPackage *> &toKeep,
                                     vector<RPackage *> &toInstall,
                                     vector<RPackage *> &toReInstall,
                                     vector<RPackage *> &toUpgrade,
                                     vector<RPackage *> &toRemove,
                                     vector<RPackage *> &toDowngrade,
				     vector<RPackage *> &notAuthenticated,
                                     const vector<RPackage *> &exclude,
                                     bool sorted)
{
   bool changed = false;

   for (unsigned i = 0; i < _packages.size(); i++) {
      int flags = _packages[i]->getFlags();

      if (state[i] != flags) {

         // These flags will never be set together.
         int status = flags & (RPackage::FHeld |
                               RPackage::FNewInstall |
                               RPackage::FReInstall |
                               RPackage::FUpgrade |
                               RPackage::FDowngrade |
                               RPackage::FRemove);

	 // add packages to the not-authenticated list if they are going to
	 // be installed in some way
	 switch(status) {
	 case RPackage::FNewInstall:
	 case RPackage::FReInstall:
	 case RPackage::FUpgrade:
	 case RPackage::FDowngrade:
	    if(!_packages[i]->isTrusted()) {
	       notAuthenticated.push_back(_packages[i]);
	       changed = true;
	    }
	 }
	 
	 if(find(exclude.begin(), exclude.end(),_packages[i]) != exclude.end())
	    continue;

         switch (status) {
            case RPackage::FNewInstall:
               toInstall.push_back(_packages[i]);
               changed = true;
               break;

            case RPackage::FReInstall:
               toReInstall.push_back(_packages[i]);
               changed = true;
               break;

            case RPackage::FUpgrade:
               toUpgrade.push_back(_packages[i]);
               changed = true;
               break;

            case RPackage::FRemove:
               toRemove.push_back(_packages[i]);
               changed = true;
               break;

            case RPackage::FKeep:
               toKeep.push_back(_packages[i]);
               changed = true;
               break;

            case RPackage::FDowngrade:
               toDowngrade.push_back(_packages[i]);
               changed = true;
               break;
         }
      }
   }

   if (sorted == true && changed == true) {
      if (toKeep.empty() == false)
         sort(toKeep.begin(), toKeep.end(), bla());
      if (toInstall.empty() == false)
         sort(toInstall.begin(), toInstall.end(), bla());
      if (toReInstall.empty() == false)
         sort(toReInstall.begin(), toReInstall.end(), bla());
      if (toUpgrade.empty() == false)
         sort(toUpgrade.begin(), toUpgrade.end(), bla());
      if (toRemove.empty() == false)
         sort(toRemove.begin(), toRemove.end(), bla());
      if (toDowngrade.empty() == false)
         sort(toDowngrade.begin(), toDowngrade.end(), bla());

   }

   return changed;
}
#endif

void RPackageLister::getDetailedSummary(vector<RPackage *> &held,
                                        vector<RPackage *> &kept,
                                        vector<RPackage *> &essential,
                                        vector<RPackage *> &toInstall,
                                        vector<RPackage *> &toReInstall,
                                        vector<RPackage *> &toUpgrade,
                                        vector<RPackage *> &toRemove,
                                        vector<RPackage *> &toPurge,
                                        vector<RPackage *> &toDowngrade,
#ifdef WITH_APT_AUTH
					vector<string> &notAuthenticated,
#endif
                                        double &sizeChange)
{
   pkgDepCache *deps = _cache->deps();

   for (unsigned int i = 0; i < _packages.size(); i++) {
      RPackage *pkg = _packages[i];
      int flags = pkg->getFlags();

      // These flags will never be set together.
      int status = flags & (RPackage::FKeep |
                            RPackage::FNewInstall |
                            RPackage::FReInstall |
                            RPackage::FUpgrade |
                            RPackage::FDowngrade |
                            RPackage::FRemove);

      switch (status) {

         case RPackage::FKeep:
            if (flags & RPackage::FHeld)
               held.push_back(pkg);
            else
               kept.push_back(pkg);
            break;

         case RPackage::FNewInstall:
            toInstall.push_back(pkg);
            break;

         case RPackage::FReInstall:
            toReInstall.push_back(pkg);
            break;

         case RPackage::FUpgrade:
            toUpgrade.push_back(pkg);
            break;

         case RPackage::FDowngrade:
            toDowngrade.push_back(pkg);
            break;

         case RPackage::FRemove:
            if (flags & RPackage::FImportant)
               essential.push_back(pkg);
            else if(flags & RPackage::FPurge)
	       toPurge.push_back(pkg);
	    else
               toRemove.push_back(pkg);
            break;
      }
   }

   sort(kept.begin(), kept.end(), bla());
   sort(toInstall.begin(), toInstall.end(), bla());
   sort(toReInstall.begin(), toReInstall.end(), bla());
   sort(toUpgrade.begin(), toUpgrade.end(), bla());
   sort(essential.begin(), essential.end(), bla());
   sort(toRemove.begin(), toRemove.end(), bla());
   sort(toPurge.begin(), toPurge.end(), bla());
   sort(held.begin(), held.end(), bla());
#ifdef WITH_APT_AUTH
   pkgAcquire Fetcher(NULL);
   pkgPackageManager *PM = _system->CreatePM(_cache->deps());
   if (!PM->GetArchives(&Fetcher, _cache->list(), _records)) {
      delete PM;
      return;
   }
   for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin(); 
	I < Fetcher.ItemsEnd(); ++I) {
      if (!(*I)->IsTrusted()) {
         notAuthenticated.push_back(string((*I)->ShortDesc()));
      }
   }
   delete PM;
#endif
   sizeChange = deps->UsrSize();
}

bool RPackageLister::updateCache(pkgAcquireStatus *status, string &error)
{
   assert(_cache->list() != NULL);
   // Get the source list
   //pkgSourceList List;
   _cache->list()->ReadMainList();

   // Lock the list directory
   FileFd Lock;
   if (_config->FindB("Debug::NoLocking", false) == false) {
      Lock.Fd(GetLock(_config->FindDir("Dir::State::Lists") + "lock"));
      //cout << "lock in : " << _config->FindDir("Dir::State::Lists") << endl;
      if (_error->PendingError() == true)
         return _error->Error(_("Unable to lock the list directory"));
   }

   _updating = true;


#ifndef HAVE_RPM
// apt-0.7.10 has the new UpdateList code in algorithms, we use it
   string s;
   bool res = ListUpdate(*status, *_cache->list(), 5000);
   if(res == false)
   {
      while(!_error->empty())
      {
	 bool isError = _error->PopMessage(s);
	 error += s;
      }
   }
   return res;
#else
   // Create the download object
   pkgAcquire Fetcher(status);

   bool Failed = false;

   if (_cache->list()->GetReleases(&Fetcher) == false)
      return false;
   Fetcher.Run();
   for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin();
        I != Fetcher.ItemsEnd(); I++) {
      if ((*I)->Status == pkgAcquire::Item::StatDone)
         continue;
      (*I)->Finished();
      Failed = true;
   }
   if (Failed == true)
      _error->Warning(_("Release files for some repositories could not be "
                        "retrieved or authenticated. Such repositories are "
                        "being ignored."));

   if (!_cache->list()->GetIndexes(&Fetcher))
      return false;

   // Run it
   if (Fetcher.Run() == pkgAcquire::Failed)
      return false;

   //bool AuthFailed = false;
   Failed = false;
   string failedURI;/* = _("Some index files failed to download, they "
			"have been ignored, or old ones used instead:\n");
		    */
   for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin();
        I != Fetcher.ItemsEnd(); I++) {
      if ((*I)->Status == pkgAcquire::Item::StatDone)
         continue;
      (*I)->Finished();
      if((*I)->ErrorText.empty())
	 failedURI += (*I)->DescURI() + "\n";
      else
	 failedURI += (*I)->DescURI() + ": " + (*I)->ErrorText + "\n";
      Failed = true;
   }

   // Clean out any old list files
   if (!Failed && _config->FindB("APT::Get::List-Cleanup", true) == true) {
      if (Fetcher.Clean(_config->FindDir("Dir::State::lists")) == false ||
          Fetcher.Clean(_config->FindDir("Dir::State::lists") + "partial/") ==
          false)
         return false;
   }
   if (Failed == true) {
      //cout << failedURI << endl;
      error = failedURI;
      //_error->Error(failedURI.c_str());
      return false; 
   }
   return true;
#endif
}

bool RPackageLister::getDownloadUris(vector<string> &uris)
{
   pkgAcquire fetcher;
   pkgPackageManager *PM = _system->CreatePM(_cache->deps());
   if (!PM->GetArchives(&fetcher, _cache->list(), _records)) {
      delete PM;
      return false;
   }
   for (pkgAcquire::ItemIterator I = fetcher.ItemsBegin();
	I != fetcher.ItemsEnd(); I++) {
      uris.push_back((*I)->DescURI());
   }

   delete PM;
   return true;
}

bool RPackageLister::commitChanges(pkgAcquireStatus *status,
                                   RInstallProgress *iprog)
{
   FileFd lock;
   int numPackages = 0;
   int numPackagesTotal;
   bool Ret = true;

   _updating = true;

   if (!lockPackageCache(lock))
      return false;

   if(_config->FindB("Synaptic::Log::Changes",true))
      makeCommitLog();

   pkgAcquire fetcher(status);

   assert(_cache->list() != NULL);
   // Read the source list
   if (_cache->list()->ReadMainList() == false) {
      _userDialog->
         warning(_("Ignoring invalid record(s) in sources.list file!"));
   }

   pkgPackageManager *rPM;
   rPM = _system->CreatePM(_cache->deps());

   if (!rPM->GetArchives(&fetcher, _cache->list(), _records) ||
       _error->PendingError())
      goto gave_wood;

   // ripped from apt-get
   while (1) {
      bool Transient = false;

#ifdef HAVE_RPM
      if (fetcher.Run() == pkgAcquire::Failed)
	 goto gave_wood;
#else
      if (fetcher.Run(50000) == pkgAcquire::Failed)
	 goto gave_wood;
#endif

      string serverError;

      numPackagesTotal = 0;

      // Print out errors
      bool Failed = false;
      for (pkgAcquire::ItemIterator I = fetcher.ItemsBegin();
           I != fetcher.ItemsEnd(); I++) {

         numPackagesTotal += 1;
         
         if ((*I)->Status == pkgAcquire::Item::StatDone && (*I)->Complete) {
            numPackages += 1;
            continue;
         }

         if ((*I)->Status == pkgAcquire::Item::StatIdle) {
            Transient = true;
            continue;
         }

	 (*I)->Finished();

         string errm = (*I)->ErrorText;
	 ostringstream tmp;
    // TRANSLATORS: Error message after a failed download.
    //              The first %s is the URL and the second 
    //              one is a detailed error message that
    //              is provided by apt
	 ioprintf(tmp, _("Failed to fetch %s\n  %s\n\n"), 
		  (*I)->DescURI().c_str(), errm.c_str());

         serverError = getServerErrorMessage(errm);

         _error->Warning("%s", tmp.str().c_str());
         Failed = true;
      }

      if (_config->FindB("Volatile::Download-Only", false)) {
         _updating = false;
         return !Failed;
      }

      if (Failed) {
         string message;

         if (Transient)
            goto gave_wood;

	 if (numPackages == 0)
	     goto gave_wood;

         message =
            _("Some of the packages could not be retrieved from the server(s).\n");
         if (!serverError.empty())
            message += "(" + serverError + ")\n";
         message += _("Do you want to continue, ignoring these packages?");

         if (!_userDialog->confirm(message.c_str()))
            goto gave_wood;
      }
      // Try to deal with missing package files
      if (Failed == true && rPM->FixMissing() == false) {
         _error->Error(_("Unable to correct missing packages"));
         goto gave_wood;
      }
      // need this so that we first fetch everything and then install (for CDs)
      if (Transient == false
          || _config->FindB("Acquire::CDROM::Copy-All", false) == false) {

         if (Transient == true) {
            // We must do that in a system independent way. */
            _config->Set("RPM::Install-Options::", "--nodeps");
         }

         _cache->releaseLock();
         pkgPackageManager::OrderResult Res =
                   iprog->start(rPM, numPackages, numPackagesTotal);
         if (Res == pkgPackageManager::Failed || _error->PendingError()) {
            if (Transient == false)
               goto gave_wood;
            Ret = false;
            // TODO: We must not discard errors here. The right
            //       solution is to use an "error stack", as
            //       implemented in apt-rpm.
            //_error->DumpErrors();
            _error->Discard();
         }
         if (Res == pkgPackageManager::Completed)
            break;

         numPackages = 0;

         _cache->lock();
      }
      // Reload the fetcher object and loop again for media swapping
      fetcher.Shutdown();

      if (!rPM->GetArchives(&fetcher, _cache->list(), _records))
         goto gave_wood;
   }

   //cout << _("Finished.")<<endl;


   // erase downloaded packages
   cleanPackageCache();

   if(_config->FindB("Synaptic::Log::Changes",true))
      writeCommitLog();

   delete rPM;
   return Ret;

 gave_wood:
   delete rPM;
   return false;
}

void RPackageLister::writeCommitLog()
{
   struct tm *t = localtime(&_logTime);
   ostringstream tmp;
   ioprintf(tmp, "%.4i-%.2i-%.2i.%.2i%.2i%.2i.log", 1900+t->tm_year, 
	    t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

   string logfile = RLogDir() + tmp.str();
   FILE *f = fopen(logfile.c_str(),"w+");
   if(f == NULL) {
      _error->Error("Failed to write commit log");
      return;
   }
   fputs(_logEntry.c_str(), f);
   fclose(f);

}

void RPackageLister::cleanCommitLog()
{
   int maxKeep = _config->FindI("Synaptic::delHistory", -1);
   //cout << "RPackageLister::cleanCommitLog(): " << maxKeep << endl;
   if(maxKeep < 0)
      return;
   
   string logfile, entry;
   struct stat buf;
   struct dirent *dent;
   time_t now = time(NULL);
   DIR *dir = opendir(RLogDir().c_str());
   while((dent=readdir(dir)) != NULL) {
      entry = string(dent->d_name);
      if(logfile == "." || logfile == "..")
	 continue;
      logfile = RLogDir()+entry;
      if(stat(logfile.c_str(), &buf) != 0) {
	 cerr << "logfile: " << logfile << endl;
	 perror("cleanCommitLog(), stat: ");
	 continue;
      }
      if((buf.st_mtime+(60*60*24*maxKeep)) < now) {
// 	 cout << "older than " << maxKeep << " days: " << logfile << endl;
// 	 cout << "now: " << now 
// 	      << " ctime: " << buf.st_mtime+(60*60*24*maxKeep) << endl;
	 unlink(logfile.c_str());
      }

   }
   closedir(dir);
}

void RPackageLister::makeCommitLog()
{
   time(&_logTime);
   _logEntry = string("Commit Log for ") + string(ctime(&_logTime)) + string("\n");
   _logEntry.reserve(2*8192); // make it big by default 

   vector<RPackage *> held;
   vector<RPackage *> kept;
   vector<RPackage *> essential;
   vector<RPackage *> toInstall;
   vector<RPackage *> toReInstall;
   vector<RPackage *> toUpgrade;
   vector<RPackage *> toRemove;
   vector<RPackage *> toPurge;
   vector<RPackage *> toDowngrade;
#ifdef WITH_APT_AUTH
   vector<string> notAuthenticated;
#endif

   double sizeChange;

   getDetailedSummary(held, kept, essential,
		      toInstall,toReInstall,toUpgrade, 
		      toRemove, toPurge, toDowngrade, 
#ifdef WITH_APT_AUTH
		      notAuthenticated,
#endif
		      sizeChange);

   if(essential.size() > 0) {
      //_logEntry += _("\n<b>Removed the following ESSENTIAL packages:</b>\n");
      _logEntry += _("\nRemoved the following ESSENTIAL packages:\n");
      for (vector<RPackage *>::const_iterator p = essential.begin();
	   p != essential.end(); p++) {
	 _logEntry += (*p)->name() + string("\n");
      }
   }
   
   if(toDowngrade.size() > 0) {
      //_logEntry += _("\n<b>Downgraded the following packages:</b>\n");
      _logEntry += _("\nDowngraded the following packages:\n");
      for (vector<RPackage *>::const_iterator p = toDowngrade.begin();
	   p != toDowngrade.end(); p++) {
	 _logEntry += (*p)->name() + string("\n");
      }
   }

   if(toPurge.size() > 0) {
      //_logEntry += _("\n<b>Completely removed the following packages:</b>\n");
      _logEntry += _("\nCompletely removed the following packages:\n");
      for (vector<RPackage *>::const_iterator p = toPurge.begin();
	   p != toPurge.end(); p++) {
	 _logEntry += (*p)->name() + string("\n");
      }
   }

   if(toRemove.size() > 0) {
      //_logEntry += _("\n<b>Removed the following packages:</b>\n");
      _logEntry += _("\nRemoved the following packages:\n");
      for (vector<RPackage *>::const_iterator p = toRemove.begin();
	   p != toRemove.end(); p++) {
	 _logEntry += (*p)->name() + string("\n");
      }
   }

   if(toUpgrade.size() > 0) {
      //_logEntry += _("\n<b>Upgraded the following packages:</b>\n");
      _logEntry += _("\nUpgraded the following packages:\n");
      for (vector<RPackage *>::const_iterator p = toUpgrade.begin();
	   p != toUpgrade.end(); p++) {
	 _logEntry += (*p)->name() + string(" (") + (*p)->installedVersion() 
	    + string(")") + string(" to ") + (*p)->availableVersion() 
	    + string("\n");
      }
   }

   if(toInstall.size() > 0) {
      //_logEntry += _("\n<b>Installed the following packages:</b>\n");
      _logEntry += _("\nInstalled the following packages:\n");
      for (vector<RPackage *>::const_iterator p = toInstall.begin();
	   p != toInstall.end(); p++) {
	 _logEntry += (*p)->name() + string(" (") + (*p)->availableVersion() 
	    + string(")") + string("\n");
      }
   }

   if(toReInstall.size() > 0) {
      //_logEntry += _("\n<b>Reinstalled the following packages:</b>\n");
      _logEntry += _("\nReinstalled the following packages:\n");
      for (vector<RPackage*>::const_iterator p = toReInstall.begin(); 
	   p != toReInstall.end(); p++) {
	 _logEntry += (*p)->name() + string(" (") + (*p)->availableVersion() 
	    + string(")") + string("\n");
      }
   }
}

bool RPackageLister::lockPackageCache(FileFd &lock)
{
   // Lock the archive directory

   if (_config->FindB("Debug::NoLocking", false) == false) {
      lock.Fd(GetLock(_config->FindDir("Dir::Cache::Archives") + "lock"));
      //cout << "lock is: " << _config->FindDir("Dir::Cache::Archives") << endl;
      if (_error->PendingError() == true)
         return _error->Error(_("Unable to lock the download directory"));
   }

   return true;
}


bool RPackageLister::cleanPackageCache(bool forceClean)
{
   FileFd lock;

   if (_config->FindB("Synaptic::CleanCache", false) || forceClean) {

      lockPackageCache(lock);

      pkgAcquire Fetcher;
      Fetcher.Clean(_config->FindDir("Dir::Cache::archives"));
      Fetcher.Clean(_config->FindDir("Dir::Cache::archives") + "partial/");
   } else if (_config->FindB("Synaptic::AutoCleanCache", false)) {

      lockPackageCache(lock);

      pkgArchiveCleaner cleaner;

      bool res;

      res = cleaner.Go(_config->FindDir("Dir::Cache::archives"),
                       *_cache->deps());

      if (!res)
         return false;

      res = cleaner.Go(_config->FindDir("Dir::Cache::archives") + "partial/",
                       *_cache->deps());

      if (!res)
         return false;

   } else {
      // mvo: nothing?
   }

   return true;
}

void RPackageLister::refreshView()
{
   _selectedView->refresh();
}

bool RPackageLister::writeSelections(ostream &out, bool fullState)
{
   for (unsigned i = 0; i < _packages.size(); i++) {
      int flags = _packages[i]->getFlags();

      // Full state saves all installed packages.
      if (flags & RPackage::FInstall ||
          fullState && (flags &RPackage::FInstalled)) {
         out << _packages[i]->name() << "\t\tinstall" << endl;
      } else if (flags & RPackage::FPurge) {
         out << _packages[i]->name() << "\t\tpurge" << endl;
      } else if (flags & RPackage::FRemove) {
         out << _packages[i]->name() << "\t\tdeinstall" << endl;
      }
   }

   return true;
}

bool RPackageLister::readSelections(istream &in)
{
   char Buffer[300];
   int CurLine = 0;
   enum Action {
      ACTION_INSTALL,
      ACTION_UNINSTALL,
      ACTION_PURGE
   };
   map<string, int> actionMap;
   pkgDepCache::ActionGroup group(*_cache->deps());

   while (in.eof() == false) {

      in.getline(Buffer, sizeof(Buffer));
      CurLine++;

      if (in.fail() && !in.eof())
         return _error->Error(_("Line %u too long in markings file."),
                              CurLine);

      _strtabexpand(Buffer, sizeof(Buffer));

      const char *C = _strstrip(Buffer);

      // Comment or blank
      if (C[0] == '#' || C[0] == 0)
         continue;

      string PkgName;
      if (ParseQuoteWord(C, PkgName) == false)
         return _error->Error(_("Malformed line %u in markings file"),
                              CurLine);
      string Action;
      if (ParseQuoteWord(C, Action) == false)
         return _error->Error(_("Malformed line %u in markings file"),
                              CurLine);

      // install
      if (Action[0] == 'i') {
         actionMap[PkgName] = ACTION_INSTALL;
         // uninstall, deinstall, remove
      } else if (Action[0] == 'u' || Action[0] == 'd' || Action[0] == 'r') {
         actionMap[PkgName] = ACTION_UNINSTALL;
         // purge
      } else if (Action[0] == 'p') {
         actionMap[PkgName] = ACTION_PURGE;
      }
   }

   if (actionMap.empty() == false) {
      int Size = actionMap.size();
      _progMeter->OverallProgress(0, Size, Size, _("Setting markings..."));
      _progMeter->SubProgress(Size);
      pkgDepCache &Cache = *_cache->deps();
      pkgProblemResolver Fix(&Cache);
      // XXX Should protect whatever is already selected in the cache.
      pkgCache::PkgIterator Pkg;
      int Pos = 0;
      for (map<string, int>::const_iterator I = actionMap.begin();
           I != actionMap.end(); I++) {
         Pkg = Cache.FindPkg((*I).first);
         if (Pkg.end() == false) {
	    Fix.Clear(Pkg);
	    Fix.Protect(Pkg);
            switch ((*I).second) {
               case ACTION_INSTALL:
		  if(_config->FindB("Volatile::SetSelectionDoReInstall",false))
		     Cache.SetReInstall(Pkg, true);
		  if(_config->FindB("Volatile::SetSelectionsNoFix",false))
		     Cache.MarkInstall(Pkg, false);
		  else
		     Cache.MarkInstall(Pkg, true);
                  break;

               case ACTION_UNINSTALL:
		  Fix.Remove(Pkg);
                  Cache.MarkDelete(Pkg, false);
                  break;

               case ACTION_PURGE:
		  Fix.Remove(Pkg);
                  Cache.MarkDelete(Pkg, true);
                  break;

            }
         }
         if ((Pos++) % 5 == 0)
            _progMeter->Progress(Pos);
      }
#ifdef WITH_LUA
      _lua->SetDepCache(_cache->deps());
      _lua->RunScripts("Scripts::Synaptic::ReadSelections", false);
      _lua->ResetCaches();
#endif
      _progMeter->Done();
      Fix.InstallProtect();
      Fix.Resolve(true);

      // refresh all views
      for (unsigned int i = 0; i != _views.size(); i++)
         _views[i]->refresh();

   }

   return true;
}

bool RPackageLister::addArchiveToCache(string archive, string &pkgname)
{
#ifndef HAVE_RPM
   //cout << "addArchiveToCache() " << archive << endl;

   // do sanity checking on the file (do we need this 
   // version, arch, or a different one etc)
   FileFd in(archive, FileFd::ReadOnly);
   debDebFile deb(in);
   debDebFile::MemControlExtract Extract("control");
   if(!Extract.Read(deb)) {
      cerr << "read failed" << endl;
      return false;
   }
   pkgTagSection tag;
   if(!tag.Scan(Extract.Control,Extract.Length+2)) {
      cerr << "scan failed" << endl;
      return false;
   }
   // do we have the pkg
   pkgname = tag.FindS("Package");
   RPackage *pkg = this->getPackage(pkgname);
   if(pkg == NULL) {
      cerr << "Can't find pkg " << pkgname << endl;
      return false;
   }
   // is it the right architecture?
   if(tag.FindS("Architecture") != "all" &&
      tag.FindS("Architecture") != _config->Find("APT::Architecture")) {
      cerr << "Ignoring different architecture for " << pkgname << endl;
      return false;
   }
   
   // correct version?
   string debVer = tag.FindS("Version");
   string candVer = "_invalid_";
   if(pkg->availableVersion() != NULL)
      candVer = pkg->availableVersion();
   if(debVer != candVer) {
      cerr << "Ignoring " << pkgname << " (different versions: "
	   << debVer << " != " << candVer  << endl;
      return false;
   }

   // md5sum check
   // first get the md5 of the candidate
   pkgDepCache *dcache = _cache->deps();
   pkgCache::VerIterator ver = dcache->GetCandidateVer(*pkg->package());
   pkgCache::VerFileIterator Vf = ver.FileList(); 
   pkgRecords::Parser &Parse = _records->Lookup(Vf);
   string MD5 = Parse.MD5Hash();
   // then calc the md5 of the pkg
   MD5Summation debMD5;
   in.Seek(0);
   debMD5.AddFD(in.Fd(),in.Size());
   if(MD5 != debMD5.Result().Value()) {
      cerr << "Ignoring " << pkgname << " MD5 does not match"<< endl;
      return false;
   }
      
   // copy to the cache
   in.Seek(0);
   FileFd out(_config->FindDir("Dir::Cache::archives")+string(flNotDir(archive)),
	      FileFd::WriteAny);
   CopyFile(in, out);

   return true;
#else
   return false;
#endif
}


#ifdef WITH_EPT
bool RPackageLister::limitBySearch(string searchString)
{
   //cerr << "limitBySearch(): " << searchString << endl;
    if (ept::axi::timestamp() == 0)
        return false;
   return xapianSearch(searchString);
}

bool RPackageLister::xapianSearch(string unsplitSearchString)
{
   //std::cerr << "RPackageLister::xapianSearch()" << std::endl;
   static const int defaultQualityCutoff = 15;
   int qualityCutoff = _config->FindI("Synaptic::Xapian::qualityCutoff", 
                                      defaultQualityCutoff);
    if (ept::axi::timestamp() == 0) 
        return false;

   try {
      int maxItems = _xapianDatabase->get_doccount();
      Xapian::Enquire enquire(*_xapianDatabase);
      Xapian::QueryParser parser;
      parser.set_database(*_xapianDatabase);
      parser.add_prefix("name","XP");
      parser.add_prefix("section","XS");
      // default op is AND to narrow down the resultset
      parser.set_default_op( Xapian::Query::OP_AND );
   
      /* Workaround to allow searching an hyphenated package name using a prefix (name:)
       * LP: #282995
       * Xapian currently doesn't support wildcard for boolean prefix and 
       * doesn't handle implicit wildcards at the end of hypenated phrases.
       *
       * e.g searching for name:ubuntu-res will be equivalent to 'name:ubuntu res*'
       * however 'name:(ubuntu* res*) won't return any result because the 
       * index is built with the full package name
       */
      // Always search for the package name
      string xpString = "name:";
      string::size_type pos = unsplitSearchString.find_first_of(" ,;");
      if (pos > 0) {
          xpString += unsplitSearchString.substr(0,pos);
      } else {
          xpString += unsplitSearchString;
      }
      Xapian::Query xpQuery = parser.parse_query(xpString);

      pos = 0;
      while ( (pos = unsplitSearchString.find("-", pos)) != string::npos ) {
         unsplitSearchString.replace(pos, 1, " ");
         pos+=1;
      }

      if(_config->FindB("Debug::Synaptic::Xapian",false)) 
         std::cerr << "searching for : " << unsplitSearchString << std::endl;
      
      // Build the query
      // apply a weight factor to XP term to increase relevancy on package name
      Xapian::Query query = parser.parse_query(unsplitSearchString, 
         Xapian::QueryParser::FLAG_WILDCARD |
         Xapian::QueryParser::FLAG_BOOLEAN |
         Xapian::QueryParser::FLAG_PARTIAL);
      query = Xapian::Query(Xapian::Query::OP_OR, query, 
              Xapian::Query(Xapian::Query::OP_SCALE_WEIGHT, xpQuery, 3));
      enquire.set_query(query);
      Xapian::MSet matches = enquire.get_mset(0, maxItems);

      if(_config->FindB("Debug::Synaptic::Xapian",false)) {
         cerr << "enquire: " << enquire.get_description() << endl;
         cerr << "matches estimated: " << matches.get_matches_estimated() << " results found" << endl;
      }

      // Retrieve the results
      int top_percent = 0;
      _viewPackages.clear();
      for (Xapian::MSetIterator i = matches.begin(); i != matches.end(); ++i)
      {
         RPackage* pkg = getPackage(i.get_document().get_data());
         // Filter out results that apt doesn't know
         if (!pkg || !_selectedView->hasPackage(pkg))
            continue;

         // Save the confidence interval of the top value, to use it as
         // a reference to compute an adaptive quality cutoff
         if (top_percent == 0)
            top_percent = i.get_percent();
   
         // Stop producing if the quality goes below a cutoff point
         if (i.get_percent() < qualityCutoff * top_percent / 100)
         {
            cerr << "Discarding: " << i.get_percent() << " over " << qualityCutoff * top_percent / 100 << endl;
            break;
         }
   
         if(_config->FindB("Debug::Synaptic::Xapian",false)) 
            cerr << i.get_rank() + 1 << ": " << i.get_percent() << "% docid=" << *i << "	[" << i.get_document().get_data() << "]" << endl;
         _viewPackages.push_back(pkg);
         }
      // re-apply sort criteria only if an explicit search is set
      if (_sortMode != LIST_SORT_DEFAULT)
          sortPackages(_sortMode);
      return true;
   } catch (const Xapian::Error & error) {
      /* We are here if a Xapian call failed. The main cause is a parser exception.
       * The error message is always in English currently. 
       * The possible parser errors are:
       *    Unknown range operation
       *    parse error
       *    Syntax: <expression> AND <expression>
       *    Syntax: <expression> AND NOT <expression>
       *    Syntax: <expression> NOT <expression>
       *    Syntax: <expression> OR <expression>
       *    Syntax: <expression> XOR <expression>
       */
      cerr << "Exception in RPackageLister::xapianSearch():" << error.get_msg() << endl;
      return false;
   }
}
#else
bool RPackageLister::limitBySearch(string searchString)
{
   return false;
}

bool RPackageLister::xapianSearch(string searchString) 
{ 
   return false; 
}
#endif

bool RPackageLister::isMultiarchSystem()
{
#ifdef WITH_APT_MULTIARCH_SUPPORT
   return (APT::Configuration::getArchitectures().size() > 1);
#else
   return false;
#endif
}

// vim:ts=3:sw=3:et
