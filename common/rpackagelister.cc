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

#include "rpackagelister.h"
#include "rpackagecache.h"
#include "rpackagefilter.h"
#include "rconfiguration.h"
#include "raptoptions.h"
#include "rinstallprogress.h"
#include "rcacheactor.h"

#include <apt-pkg/error.h>
#include <apt-pkg/algorithms.h>
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/acquire.h>
#include <apt-pkg/acquire-item.h>
#include <apt-pkg/clean.h>

#include <apt-pkg/sourcelist.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/strutl.h>

#ifdef WITH_LUA
#include <apt-pkg/luaiface.h>
#endif

#include <algorithm>
#include <cstdio>

#include "sections_trans.h"

#include "i18n.h"

using namespace std;

RPackageLister::RPackageLister()
   : _records(0), _filter(0)
{
   _cache = new RPackageCache();

   _searchData.pattern = NULL;
   _searchData.isRegex = false;
   _viewMode = _config->FindI("Synaptic::ViewMode", 0);
   _updating = true;
   _sortMode = LIST_SORT_NAME;

   _views.push_back(new RPackageViewSections());
   _views.push_back(new RPackageViewStatus());
   _views.push_back(new RPackageViewAlphabetic());
   _views.push_back(new RPackageViewAll());

   if (_viewMode >= _views.size())
      _viewMode = 0;
   _selectedView = _views[_viewMode];

   memset(&_searchData, 0, sizeof(_searchData));

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

void RPackageLister::setView(int index)
{
   _config->Set("Synaptic::ViewMode", index);
   _selectedView = _views[index];
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
   _selectedView->setSelected(newSubView);

   notifyPreChange(NULL);

   reapplyFilter();

   notifyPostChange(NULL);

   return true;
}

static string getServerErrorMessage(string errm)
{
   string msg;
   unsigned int pos = errm.find("server said");
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


void RPackageLister::makePresetFilters()
{
   RFilter *filter;

   // Notice that there's a little hack in filter names below. They're
   // saved *without* i18n, but there's an i18n version for i18n. This
   // allows i18n to be done in RFilter.getName().
   {
      filter = new RFilter(this);
      filter->preset = true;
      filter->setName("Search Filter");
      _("Search Filter");
      registerFilter(filter);
   }
   {
      filter = new RFilter(this);
      filter->preset = true;
      filter->status.setStatus((int)RStatusPackageFilter::Installed);
      filter->setName("Installed");
      _("Installed");
      registerFilter(filter);
   }
   {
      filter = new RFilter(this);
      filter->preset = true;
      filter->status.setStatus((int)RStatusPackageFilter::NotInstalled);
      filter->setName("Not Installed");
      _("Not Installed");
      registerFilter(filter);
   }
#ifdef HAVE_RPM
   {
      filter = new RFilter(this);
      filter->pattern.addPattern(RPatternPackageFilter::Name,
                                 "^task-.*", false);
      filter->setName("Tasks");
      _("Tasks");
      registerFilter(filter);
   }
   {
      filter = new RFilter(this);
      filter->reducedview.enable();
      filter->setName("Reduced View");
      _("Reduced View");
      registerFilter(filter);
   }
#endif
   {
      filter = new RFilter(this);
      filter->preset = true;
      filter->status.setStatus((int)RStatusPackageFilter::Upgradable);
      filter->setName("Upgradable");
      _("Upgradable");
      registerFilter(filter);
   }
   {
      filter = new RFilter(this);
      filter->preset = true;
      filter->status.setStatus(RStatusPackageFilter::Broken);
      filter->setName("Broken");
      _("Broken");
      registerFilter(filter);
   }
   {
      filter = new RFilter(this);
      filter->preset = true;
      filter->status.setStatus(RStatusPackageFilter::MarkInstall
                               | RStatusPackageFilter::MarkRemove
                               | RStatusPackageFilter::Broken);
      filter->setName("Queued Changes");
      _("Queued Changes");
      registerFilter(filter);
   }
   {
      filter = new RFilter(this);
      filter->preset = true;
      filter->status.setStatus(RStatusPackageFilter::NewPackage);
      filter->setName("New in archive");
      _("New in archive");
      registerFilter(filter);
   }
#ifndef HAVE_RPM
   {
      filter = new RFilter(this);
      filter->preset = true;
      filter->status.setStatus(RStatusPackageFilter::ResidualConfig);
      filter->setName("Residual config");
      _("Residual config");
      registerFilter(filter);
   }
   {
      filter = new RFilter(this);
      filter->preset = true;
      filter->pattern.addPattern(RPatternPackageFilter::Depends,
                                 "^debconf", false);
      filter->setName("Pkg with Debconf");
      _("Pkg with Debconf");
      registerFilter(filter);
   }
   {
      filter = new RFilter(this);
      filter->preset = true;
      filter->status.setStatus(RStatusPackageFilter::NotInstallable);
      filter->setName("Obsolete or locally installed");
      _("Obsolete or locally installed");
      registerFilter(filter);
   }
#endif
}


void RPackageLister::restoreFilters()
{
   Configuration config;
   RReadFilterData(config);

   RFilter *filter;
   const Configuration::Item *top = config.Tree("filter");
   for (top = (top == 0 ? 0 : top->Child); top != 0; top = top->Next) {
      filter = new RFilter(this);
      filter->setName(top->Tag);

      string filterkey = "filter::" + top->Tag;
      if (filter->read(config, filterkey)) {
         registerFilter(filter);
      } else {
         delete filter;
      }
   }

   // Introduce new preset filters in the current config file.
   // Already existent filters will be ignored, since the name
   // will clash.
   makePresetFilters();
}


void RPackageLister::storeFilters()
{
   ofstream out;

   if (!RFilterDataOutFile(out))
      return;

   for (vector<RFilter *>::const_iterator iter = _filterL.begin();
        iter != _filterL.end(); iter++) {

      (*iter)->write(out);
   }

   out.close();
}


bool RPackageLister::registerFilter(RFilter *filter)
{
   string Name = filter->getName();
   for (vector<RFilter *>::const_iterator I = _filterL.begin();
        I != _filterL.end(); I++) {
      if ((*I)->getName() == Name) {
         delete filter;
         return false;
      }
   }
   _filterL.push_back(filter);
   return true;
}


void RPackageLister::unregisterFilter(RFilter *filter)
{
   for (vector<RFilter *>::iterator I = _filterL.begin();
        I != _filterL.end(); I++) {
      if (*I == filter) {
         _filterL.erase(I);
         return;
      }
   }
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

bool RPackageLister::openCache(bool reset)
{
   static bool firstRun = true;

   // Flush old errors
   _error->Discard();

   if (reset) {
      if (!_cache->reset(*_progMeter)) {
         _progMeter->Done();
         _cacheValid = false;
         return false;
      }
   } else {
      if (!_cache->open(*_progMeter)) {
         _progMeter->Done();
         _cacheValid = false;
         return false;
      }
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

#ifdef HAVE_RPM
   // APT used to have a bug where the destruction order made
   // this code segfault. APT-RPM has the right fix for this. -- niemeyer
   if (_records)
      delete _records;
#endif

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
   _packages.resize(packageCount);

   _packagesIndex.clear();
   _packagesIndex.resize(packageCount, -1);

   pkgCache::PkgIterator I = deps->PkgBegin();

   string pkgName;
   int count = 0;

   _installedCount = 0;

   map<string, RPackage *> pkgmap;
   set<string> sectionSet;

   for (unsigned int i = 0; i != _views.size(); i++)
      _views[i]->clear();

   for (; I.end() != true; I++) {

      if (I->CurrentVer != 0)
         _installedCount++;
      else if (I->VersionList == 0)
         continue; // Exclude virtual packages.

      RPackage *pkg = new RPackage(this, deps, _records, I);
      _packagesIndex[I->ID] = count;
      _packages[count++] = pkg;

      pkgName = pkg->name();

      pkgmap[pkgName] = pkg;

      for (unsigned int i = 0; i != _views.size(); i++)
         _views[i]->addPackage(pkg);

      // Find out about new packages.
      if (firstRun) {
         allPackages.insert(pkgName);
      } else if (allPackages.find(pkgName) == allPackages.end()) {
         pkg->setNew();
         _roptions->setPackageNew(pkgName.c_str());
         allPackages.insert(pkgName);
      }

      // Gather list of sections.
      if (I.Section())
         sectionSet.insert(I.Section());
      else
         cerr << "Package " << I.Name() << " has no section?!" << endl;
   }

   // Truncate due to virtual packages which were skipped above.
   _packages.resize(count);

   _sectionList.resize(sectionSet.size());
   copy(sectionSet.begin(), sectionSet.end(), _sectionList.begin());

   for (I = deps->PkgBegin(); I.end() == false; I++) {
      // This is a virtual package.
      if (I->VersionList == 0) {
         // This is a virtual package. Find the owner and attach it there.
         if (I->ProvidesList == 0)
            continue;
         string name = I.ProvidesList().OwnerPkg().Name();
         map<string, RPackage *>::iterator I2 = pkgmap.find(name);
         if (I2 != pkgmap.end())
            (*I2).second->addVirtualPackage(I);
      }
   }

   applyInitialSelection();

   _updating = false;

   if (reset)
      reapplyFilter();
   else
      setFilter(); // Set default filter (no filter)

   // mvo: put it here for now
   notifyCacheOpen();

   firstRun = false;

   _cacheValid = true;
   return true;
}



void RPackageLister::applyInitialSelection()
{
   _roptions->rereadOrphaned();
   _roptions->rereadDebconf();

   for (unsigned i = 0; i < _packages.size(); i++) {

      if (_roptions->getPackageLock(_packages[i]->name()))
         _packages[i]->setPinned(true);

      if (_roptions->getPackageNew(_packages[i]->name()))
         _packages[i]->setNew(true);

      if (_roptions->getPackageOrphaned(_packages[i]->name()))
         _packages[i]->setOrphaned(true);
   }
}

RPackage *RPackageLister::getPackage(pkgCache::PkgIterator &iter)
{
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
      return _error->Error(_("Unable to minimize the upgrade set"));

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



void RPackageLister::setFilter(int index)
{
   if (index < 0 || (unsigned int)index >= _filterL.size()) {
      _filter = NULL;
   } else {
      _filter = findFilter(index);
   }

   reapplyFilter();
}

void RPackageLister::setFilter(RFilter *filter)
{
   _filter = NULL;
   for (unsigned int i = 0; i < _filterL.size(); i++) {
      if (filter == _filterL[i]) {
         _filter = _filterL[i];
         break;
      }
   }

   reapplyFilter();
}

int RPackageLister::getFilterIndex(RFilter *filter)
{
   if (filter == NULL)
      filter = _filter;
   for (unsigned int i = 0; i < _filterL.size(); i++) {
      if (filter == _filterL[i])
         return i;
   }
   return -1;
}

vector<string> RPackageLister::getFilterNames()
{
   vector<string> filters;
   for (unsigned int i = 0; i != _filterL.size(); i++)
      filters.push_back(_filterL[i]->getName());
   return filters;
}

bool RPackageLister::applyFilters(RPackage *package)
{
   if (_filter == NULL)
      return true;

   return _filter->apply(package);
}


void RPackageLister::reapplyFilter()
{
   if (_updating)
      return;

   _viewPackages.clear();
   _viewPackagesIndex.clear();
   _viewPackagesIndex.resize(_packagesIndex.size(), -1);

   for (RPackageView::iterator I = _selectedView->begin();
        I != _selectedView->end(); I++) {
      if (applyFilters(*I)) {
         _viewPackagesIndex[(*(*I)->package())->ID] = _viewPackages.size();
         _viewPackages.push_back(*I);
      }
   }

   // sort now according to the latest used sort method
   switch (_sortMode) {

      case LIST_SORT_NAME:
         sortPackagesByName();
         break;

      case LIST_SORT_SIZE_ASC:
         sortPackagesByInstSize(0);
         break;

      case LIST_SORT_SIZE_DES:
         sortPackagesByInstSize(1);
         break;
   }
}

static void qsSortByName(vector<RPackage *> &packages, int start, int end)
{
   int i, j;
   RPackage *pivot, *tmp;

   i = start;
   j = end;
   pivot = packages[(i + j) / 2];
   do {
      while (strcoll(packages[i]->name(), pivot->name()) < 0)
         i++;
      while (strcoll(pivot->name(), packages[j]->name()) < 0)
         j--;
      if (i <= j) {
         tmp = packages[i];
         packages[i] = packages[j];
         packages[j] = tmp;
         i++;
         j--;
      }
   } while (i <= j);

   if (start < j)
      qsSortByName(packages, start, j);
   if (i < end)
      qsSortByName(packages, i, end);
}

void RPackageLister::sortPackagesByName(vector<RPackage *> &packages)
{
   _sortMode = LIST_SORT_NAME;
   if (!packages.empty())
      qsSortByName(packages, 0, packages.size() - 1);
}

struct instSizeSortFuncAsc {
   bool operator() (RPackage *x, RPackage *y) {
      return x->installedSize() < y->installedSize();
}};
struct instSizeSortFuncDes {
   bool operator() (RPackage *x, RPackage *y) {
      return x->installedSize() > y->installedSize();
}};


void RPackageLister::sortPackagesByInstSize(vector<RPackage *> &packages,
                                            int order)
{
   //cout << "RPackageLister::sortPackagesByInstSize()"<<endl;
   if (order == 0)
      _sortMode = LIST_SORT_SIZE_ASC;
   else
      _sortMode = LIST_SORT_SIZE_DES;

   if (!packages.empty()) {
      if (order == 0)
         sort(packages.begin(), packages.end(), instSizeSortFuncAsc());
      else
         sort(packages.begin(), packages.end(), instSizeSortFuncDes());
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
}


void RPackageLister::getSummary(int &held, int &kept, int &essential,
                                int &toInstall, int &toReInstall,
				int &toUpgrade, int &toRemove,
                                int &toDowngrade, double &sizeChange)
{
   pkgDepCache *deps = _cache->deps();
   unsigned i;

   held = 0;
   kept = deps->KeepCount();
   essential = 0;
   toInstall = 0;
   toReInstall = 0;
   toUpgrade = 0;
   toDowngrade = 0;
   toRemove = 0;

   for (i = 0; i < _packages.size(); i++) {
      int flags = _packages[i]->getFlags();

      // These flags will never be set together.
      int status = flags & (RPackage::FHeld |
                            RPackage::FNewInstall |
                            RPackage::FReInstall |
                            RPackage::FUpgrade |
                            RPackage::FDowngrade |
                            RPackage::FRemove);

      switch (status) {
         case RPackage::FHeld:
            held++;
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
                                     vector<RPackage *> &exclude,
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
                                     vector<RPackage *> &exclude,
                                     bool sorted)
{
   bool changed = false;

   for (unsigned i = 0; i < _packages.size(); i++) {
      int flags = _packages[i]->getFlags();

      if (state[i] != flags &&
          find(exclude.begin(), exclude.end(),
               _packages[i]) == exclude.end()) {

         // These flags will never be set together.
         int status = flags & (RPackage::FHeld |
                               RPackage::FNewInstall |
                               RPackage::FReInstall |
                               RPackage::FUpgrade |
                               RPackage::FDowngrade |
                               RPackage::FRemove);

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
                                        vector<RPackage *> &toDowngrade,
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
   sort(held.begin(), held.end(), bla());

   sizeChange = deps->UsrSize();
}

bool RPackageLister::updateCache(pkgAcquireStatus *status)
{
   assert(_cache->list() != NULL);
   // Get the source list
   //pkgSourceList List;
#ifdef HAVE_RPM
   _cache->list()->ReadMainList();
#else
   _cache->list()->Read(_config->FindFile("Dir::Etc::sourcelist"));
#endif
   // Lock the list directory
   FileFd Lock;
   if (_config->FindB("Debug::NoLocking", false) == false) {
      Lock.Fd(GetLock(_config->FindDir("Dir::State::Lists") + "lock"));
      if (_error->PendingError() == true)
         return _error->Error(_("Unable to lock the list directory"));
   }

   _updating = true;

   // Create the download object
   pkgAcquire Fetcher(status);

   bool Failed = false;

#if HAVE_RPM
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
      _error->
         Warning(_
                 ("Release files for some repositories could not be retrieved or authenticated. Such repositories are being ignored."));
#endif /* HAVE_RPM */

   if (!_cache->list()->GetIndexes(&Fetcher))
      return false;

   // Run it
   if (Fetcher.Run() == pkgAcquire::Failed)
      return false;

   //bool AuthFailed = false;
   Failed = false;
   for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin();
        I != Fetcher.ItemsEnd(); I++) {
      if ((*I)->Status == pkgAcquire::Item::StatDone)
         continue;
      (*I)->Finished();
//      cerr << _("Failed to fetch ") << (*I)->DescURI() << endl;
//      cerr << "  " << (*I)->ErrorText << endl;
      Failed = true;
   }

   // Clean out any old list files
   if (_config->FindB("APT::Get::List-Cleanup", true) == true) {
      if (Fetcher.Clean(_config->FindDir("Dir::State::lists")) == false ||
          Fetcher.Clean(_config->FindDir("Dir::State::lists") + "partial/") ==
          false)
         return false;
   }
   if (Failed == true)
      return _error->
         Error(_
               ("Some index files failed to download, they have been ignored, or old ones used instead."));

   return true;
}



bool RPackageLister::commitChanges(pkgAcquireStatus *status,
                                   RInstallProgress *iprog)
{
   FileFd lock;
   int numPackages = 0;
   int numPackagesTotal = 0;
   bool Ret = true;

   _updating = true;

   if (!lockPackageCache(lock))
      return false;

   pkgAcquire fetcher(status);

   assert(_cache->list() != NULL);
   // Read the source list
#ifdef HAVE_RPM
   if (_cache->list()->ReadMainList() == false) {
      _userDialog->
         warning(_("Ignoring invalid record(s) in sources.list file!"));
   }
#else
   if (_cache->list()->Read(_config->FindFile("Dir::Etc::sourcelist")) ==
       false) {
      _userDialog->
         warning(_("Ignoring invalid record(s) in sources.list file!"));
   }
#endif

   pkgPackageManager *PM;
   PM = _system->CreatePM(_cache->deps());
   RPackageManager rPM(PM);
   if (!PM->GetArchives(&fetcher, _cache->list(), _records) ||
       _error->PendingError())
      goto gave_wood;

   // ripped from apt-get
   while (1) {
      bool Transient = false;

      if (fetcher.Run() == pkgAcquire::Failed)
         goto gave_wood;

      string serverError;

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

         string errm = (*I)->ErrorText;
         string tmp = _("Failed to fetch ") + (*I)->DescURI() + "\n"
            "  " + errm;

         serverError = getServerErrorMessage(errm);

         _error->Warning(tmp.c_str());
         Failed = true;
      }

      if (_config->FindB("Synaptic::Download-Only", false)) {
         _updating = false;
         return !Failed;
      }

      if (Failed) {
         string message;

         if (Transient)
            goto gave_wood;

         message =
            _
            ("Some of the packages could not be retrieved from the server(s).\n");
         if (!serverError.empty())
            message += "(" + serverError + ")\n";
         message += _("Do you want to continue, ignoring these packages?");

         if (!_userDialog->confirm(message.c_str()))
            goto gave_wood;
      }
      // Try to deal with missing package files
      if (Failed == true && PM->FixMissing() == false) {
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
		    iprog->start(&rPM, numPackages, numPackagesTotal);
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

      if (!PM->GetArchives(&fetcher, _cache->list(), _records))
         goto gave_wood;
   }

   //cout << _("Finished.")<<endl;


   // erase downloaded packages
   cleanPackageCache();

   delete PM;
   return Ret;

 gave_wood:
   delete PM;
   return false;
}


bool RPackageLister::lockPackageCache(FileFd &lock)
{
   // Lock the archive directory

   if (_config->FindB("Debug::NoLocking", false) == false) {
      lock.Fd(GetLock(_config->FindDir("Dir::Cache::Archives") + "lock"));
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


bool RPackageLister::writeSelections(ostream &out, bool fullState)
{
   for (unsigned i = 0; i < _packages.size(); i++) {
      int flags = _packages[i]->getFlags();

      // Full state saves all installed packages.
      if (flags & RPackage::FInstall ||
          fullState && (flags &RPackage::FInstalled)) {
         out << _packages[i]->name() << "\t\tinstall" << endl;
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
      ACTION_UNINSTALL
   };
   map<string, int> actionMap;

   while (in.eof() == false) {
      in.getline(Buffer, sizeof(Buffer));
      CurLine++;

      if (in.fail() && !in.eof())
         return _error->Error(_("Line %u too long in selection file."),
                              CurLine);

      _strtabexpand(Buffer, sizeof(Buffer));
      _strstrip(Buffer);

      const char *C = Buffer;

      // Comment or blank
      if (C[0] == '#' || C[0] == 0)
         continue;

      string PkgName;
      if (ParseQuoteWord(C, PkgName) == false)
         return _error->Error(_("Malformed line %u in selection file"),
                              CurLine);
      string Action;
      if (ParseQuoteWord(C, Action) == false)
         return _error->Error(_("Malformed line %u in selection file"),
                              CurLine);

      if (Action[0] == 'i') {
         actionMap[PkgName] = ACTION_INSTALL;
      } else if (Action[0] == 'u' || Action[0] == 'd') {
         actionMap[PkgName] = ACTION_UNINSTALL;
      }
   }

   if (actionMap.empty() == false) {
      int Size = actionMap.size();
      _progMeter->OverallProgress(0, Size, Size, _("Setting selections..."));
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
                  Cache.MarkInstall(Pkg, true);
                  break;

               case ACTION_UNINSTALL:
                  Fix.Remove(Pkg);
                  Cache.MarkDelete(Pkg, false);
                  break;
            }
         }
         if ((Pos++) % 5 == 0)
            _progMeter->Progress(Pos);
      }
      _progMeter->Done();
      Fix.InstallProtect();
      Fix.Resolve(true);
   }

   return true;
}

// vim:ts=3:sw=3:et
