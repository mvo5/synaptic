/* rpackagelister.h - package cache and list manipulation
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2002 Michael Vogt <mvo@debian.org>
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

#ifndef _RPACKAGELISTER_H_
#define _RPACKAGELISTER_H_

#include "config.h"  // IWYU pragma: associated

#include "rpackagecache.h"
#include "rpackagestatus.h"

#include <apt-pkg/pkgcache.h>
#include <apt-pkg/progress.h>
#include <ctime>
#include <istream>
#include <list>
#include <regex.h>
#include <set>
#include <string>
#include <vector>

#ifdef HAVE_RPM
#include <apt-pkg/depcache.h>
#endif

#ifdef WITH_QUICK_FILTER
#include <sqlite3.h>
#endif

class FileFd;
class OpProgress;
class RCacheActor;
class RInstallProgress;
class RPackage;
class RPackageView;
class RPackageViewFilter;
class RPackageViewSearch;
class RUserDialog;
class pkgAcquireStatus;
class pkgRecords;

class RPackageObserver {
 public:
   virtual void notifyChange(RPackage *pkg) = 0;
   virtual void notifyPreFilteredChange() = 0;
   virtual void notifyPostFilteredChange() = 0;
};

class RCacheObserver {
 public:
   virtual void notifyCacheOpen() = 0;
   virtual void notifyCachePreChange() = 0;
   virtual void notifyCachePostChange() = 0;
};

// base sort class
// for a example use see sortPackages()
template<class T>
class sortFunc {
 protected:
   bool _ascent;
   T cmp;
 public:
   sortFunc(bool ascent) : _ascent(ascent) {}
   bool operator() (RPackage *x, RPackage *y) {
      if(_ascent)
	 return cmp(x,y);
      else
	 return cmp(y,x);
   }
};

class RPackageLister {

   protected:

   // Internal APT stuff.
   RPackageCache * _cache;
   pkgRecords *_records;
   OpProgress *_progMeter;

   #ifdef WITH_QUICK_FILTER
      sqlite3 *_quickFilterDb = nullptr;
   #endif

   // Other members.
   std::vector<RPackage *> _packages;
   std::vector<int> _packagesIndex;

   std::vector<RPackage *> _viewPackages;
   std::vector<int> _viewPackagesIndex;

   // this is what we feed to the views as "all packages" to avoid
   // to show all the multiarch versions by default, the user can
   // turn that off with a config option
   std::vector<RPackage *> _nativeArchPackages;

   // It shouldn't be needed to control this inside this class. -- niemeyer
   bool _updating;

   // all known packages (needed identifing "new" pkgs)
   std::set<std::string> packageNames;

   bool _cacheValid;            // is the cache valid?

   int _installedCount;         // # of installed packages

   std::vector<RCacheActor *> _actors;

   RPackageViewFilter *_filterView; // the package view that does the filtering
   RPackageViewSearch *_searchView; // the package view that does the (simple) search
   
   public:

#ifdef WITH_QUICK_FILTER
   bool applyQuickFilter(std::string searchString);
#endif
   
   unsigned int _viewMode;

   typedef enum {
      LIST_SORT_DEFAULT,
      LIST_SORT_NAME_ASC,
      LIST_SORT_NAME_DES,
      LIST_SORT_SIZE_ASC,
      LIST_SORT_SIZE_DES,
      LIST_SORT_SUPPORTED_ASC,
      LIST_SORT_SUPPORTED_DES,
      LIST_SORT_SECTION_ASC,
      LIST_SORT_SECTION_DES,
      LIST_SORT_COMPONENT_ASC,
      LIST_SORT_COMPONENT_DES,
      LIST_SORT_DLSIZE_ASC,
      LIST_SORT_DLSIZE_DES,
      LIST_SORT_STATUS_ASC,
      LIST_SORT_STATUS_DES,
      LIST_SORT_VERSION_ASC,
      LIST_SORT_VERSION_DES,
      LIST_SORT_INST_VERSION_ASC,
      LIST_SORT_INST_VERSION_DES
   } listSortMode;
   listSortMode _sortMode;

#ifdef HAVE_RPM
   typedef pkgDepCache::State pkgState;
#else
   typedef std::vector<int> pkgState;
#endif

   private:

   std::vector<RPackageView *> _views;
   RPackageView *_selectedView;
   RPackageStatus _pkgStatus;

   void applyInitialSelection();

   bool lockPackageCache(FileFd &lock);

   void sortPackages(std::vector<RPackage *> &packages,listSortMode mode);

   struct {
      char *pattern;
      regex_t regex;
      bool isRegex;
      int last;
   } _searchData;

   std::vector<RPackageObserver *> _packageObservers;
   std::vector<RCacheObserver *> _cacheObservers;

   RUserDialog *_userDialog;

   void makeCommitLog();
   void writeCommitLog();
   std::string _logEntry;
   time_t _logTime;

   // undo/redo stuff
   std::list<pkgState> undoStack;
   std::list<pkgState> redoStack;

   public:

   // clean files older than "Synaptic::delHistory"
   void cleanCommitLog();

   void sortPackages(listSortMode mode) {
      sortPackages(_viewPackages, mode);
   }

   void setView(unsigned int index);
   std::vector<std::string> getViews();
   std::vector<std::string> getSubViews();

   // set subView (if newView is empty, set to all packages)
   bool setSubView(std::string newView="");

   // this needs a different name, something like refresh
   void reapplyFilter();

   // refresh view
   void refreshView();

   // is is exposed for the stuff like filter manager window
   RPackageViewFilter *filterView() { return _filterView; }
   RPackageViewSearch *searchView() { return _searchView; }

   // find
   int findPackage(const char *pattern);
   int findNextPackage();

   const std::vector<RPackage *> &getPackages() { return _packages; }
   const std::vector<RPackage *> &getViewPackages() { return _viewPackages; }
   RPackage *getPackage(int index) { return _packages.at(index); }
   RPackage *getViewPackage(int index) { return _viewPackages.at(index); }
   RPackage *getPackage(pkgCache::PkgIterator &pkg);
   RPackage *getPackage(std::string name);
   int getPackageIndex(RPackage *pkg);
   int getViewPackageIndex(RPackage *pkg);

   int packagesSize() { return _packages.size(); }
   int viewPackagesSize() { return _updating ? 0 : _viewPackages.size(); }

   void getStats(int &installed, int &broken, int &toInstall,
		 int &toRemove, double &sizeChange);

   void getSummary(int &held, int &kept, int &essential,
                   int &toInstall, int &toReInstall, int &toUpgrade,
		   int &toRemove,  int &toDowngrade,
		   int &unAuthenticated,  double &sizeChange);


   void getDetailedSummary(std::vector<RPackage *> &held,
                           std::vector<RPackage *> &kept,
                           std::vector<RPackage *> &essential,
                           std::vector<RPackage *> &toInstall,
                           std::vector<RPackage *> &toReInstall,
                           std::vector<RPackage *> &toUpgrade,
                           std::vector<RPackage *> &toRemove,
                           std::vector<RPackage *> &toPurge,
                           std::vector<RPackage *> &toDowngrade,
#ifdef WITH_APT_AUTH
			   std::vector<std::string> &notAuthenticated,
#endif
                           double &sizeChange);

   void getDownloadSummary(int &dlCount, double &dlSize);

   void saveUndoState(pkgState &state);
   void saveUndoState();
   void undo();
   void redo();
   void saveState(pkgState &state);
   void restoreState(pkgState &state);
   bool getStateChanges(pkgState &state,
                        std::vector<RPackage *> &kept,
                        std::vector<RPackage *> &toInstall,
                        std::vector<RPackage *> &toReInstall,
                        std::vector<RPackage *> &toUpgrade,
                        std::vector<RPackage *> &toRemove,
                        std::vector<RPackage *> &toDowngrade,
                        std::vector<RPackage *> &notAuthenticated,
                        const std::vector<RPackage *> &exclude,
                        bool sorted = true);

   // open (lock if run as root)
   bool openCache();
   bool fixBroken();
   bool check();
   bool upgradable();
   bool upgrade();
   bool distUpgrade();
   bool cleanPackageCache(bool forceClean = false);
   bool updateCache(pkgAcquireStatus *status, std::string &error);
   bool commitChanges(pkgAcquireStatus *status, RInstallProgress *iprog);

   // some information
   bool getDownloadUris(std::vector<std::string> &uris);
   bool addArchiveToCache(std::string archiveDir, std::string &pkgname);

   void setProgressMeter(OpProgress *progMeter) {
      if(_progMeter != NULL)
	 delete _progMeter;
      _progMeter = progMeter;
   }

   void setUserDialog(RUserDialog *dialog) {
      _userDialog = dialog;
   }

   // policy stuff
   std::vector<std::string> getPolicyArchives(bool filenames_only=false) {
      if (_cacheValid)
         return _cache->getPolicyArchives(filenames_only);
      else
         return std::vector<std::string>();
   }

   // multiarch
   bool isMultiarchSystem();

   // notification stuff about changes in packages
   void notifyPreChange(RPackage *pkg);
   void notifyPostChange(RPackage *pkg);
   void notifyChange(RPackage *pkg);
   void registerObserver(RPackageObserver *observer);
   void unregisterObserver(RPackageObserver *observer);

   // notification stuff about changes in cache
   void notifyCacheOpen();
   void notifyCachePreChange();
   void notifyCachePostChange();
   void registerCacheObserver(RCacheObserver *observer);
   void unregisterCacheObserver(RCacheObserver *observer);

   bool readSelections(std::istream &in);
   bool writeSelections(std::ostream &out, bool fullState);

   RPackageCache* getCache() { return _cache; }

   #ifdef WITH_QUICK_FILTER
   
      // Use the sqlite based quick search feature.
   
      bool openQuickFilterDb();
   
      static bool BeginQuickFilterDbInsertion(sqlite3 *db);
      static bool EndQuickFilterDbInsertion(sqlite3 *db);
      static bool AddPackageToQuickFilterDb(sqlite3 *db, const std::string_view &name, const std::string_view &description);
      static bool populateQuickFilterDb(sqlite3 *db, std::vector<RPackage *> &packages);

   #endif

   RPackageLister();
   ~RPackageLister();
};

#endif

// vim:ts=3:sw=3:et
