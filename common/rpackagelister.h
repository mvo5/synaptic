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


#include <vector>
#include <list>
#include <map>
#include <set>
#include <regex.h>
#include <apt-pkg/depcache.h>

#include "rpackagecache.h"
#include "rpackage.h"
#include "rpackageview.h"
#include "ruserdialog.h"
#include "tree.hh"
#include "config.h"

#ifdef HAVE_DEBTAGS
#include <TagcollParser.h>
#include <StdioParserInput.h>
#include <SmartHierarchy.h>
#include <TagcollBuilder.h>
#include <HandleMaker.h>
#include <TagCollection.h>
#endif


using namespace std;

class OpProgress;
class RPackageCache;
class RPackageFilter;
class RCacheActor;
class pkgRecords;
class pkgAcquireStatus;
class pkgPackageManager;


struct RFilter;


class RInstallProgress;

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

class RPackageLister {

   protected:

   // Internal APT stuff.
   RPackageCache * _cache;
   pkgRecords *_records;
   OpProgress *_progMeter;

   // Other members.
   vector<RPackage *> _packages;
   vector<int> _packagesIndex;

   vector<RPackage *> _viewPackages;
   vector<int> _viewPackagesIndex;

   // It shouldn't be needed to control this inside this class. -- niemeyer
   bool _updating;

   // Need a better name here. -- niemeyer
   set<string> allPackages;  // all known packages (needed identifing "new" pkgs)

   bool _cacheValid;            // is the cache valid?

   int _installedCount;         // # of installed packages

   vector<RFilter *> _filterL;
   RFilter *_filter;            // effective filter, NULL for none

   vector<string> _sectionList;      // list of all available package sections

   vector<RCacheActor *> _actors;

   public:

   int _viewMode;

   typedef enum {
      LIST_SORT_NAME,
      LIST_SORT_SIZE_ASC,
      LIST_SORT_SIZE_DES
   } listSortMode;
   listSortMode _sortMode;

#ifdef HAVE_RPM
   typedef pkgDepCache::State pkgState;
#else
   typedef vector<RPackage::MarkedStatus> pkgState;
#endif

   private:

   vector<RPackageView *> _views;
   RPackageView *_selectedView;

   void applyInitialSelection();

   void makePresetFilters();

   bool applyFilters(RPackage *package);

   bool lockPackageCache(FileFd &lock);

   void sortPackagesByName(vector<RPackage *> &packages);
   void sortPackagesByInstSize(vector<RPackage *> &packages, int order);

   struct {
      char *pattern;
      regex_t regex;
      bool isRegex;
      int last;
   } _searchData;

   vector<RPackageObserver *> _packageObservers;
   vector<RCacheObserver *> _cacheObservers;

   RUserDialog *_userDialog;

   // undo/redo stuff
   list<pkgState> undoStack;
   list<pkgState> redoStack;

   public:

   void sortPackagesByName() {
      sortPackagesByName(_viewPackages);
   };
   void sortPackagesByInstSize(int order) {
      sortPackagesByInstSize(_viewPackages, order);
   };

   const vector<string> &getSections() { return _sectionList; };

   void setView(int index);
   vector<string> getViews();
   vector<string> getSubViews();
   bool setSubView(string newView);

   void storeFilters();
   void restoreFilters();

   // filter management
   void setFilter(int index = -1);
   // note that setFilter will only set it if the filter is already registered
   void setFilter(RFilter *filter);
   inline RFilter *getFilter() {
      return _filter;
   };
   int getFilterIndex(RFilter *filter = NULL);
   unsigned int nrOfFilters() {
      return _filterL.size();
   };

   bool registerFilter(RFilter *filter);
   void unregisterFilter(RFilter *filter);
   void getFilterNames(vector<string> &filters);
   inline RFilter *findFilter(unsigned int index) {
      if (index > _filterL.size())
         return NULL;
      else
         return _filterL[index];
   };

   void reapplyFilter();

   // find 
   int findPackage(const char *pattern);
   int findNextPackage();

   const vector<RPackage *> &getPackages() { return _packages; };
   const vector<RPackage *> &getViewPackages() { return _viewPackages; };
   RPackage *getPackage(int index) { return _packages.at(index); };
   RPackage *getViewPackage(int index) { return _viewPackages.at(index); };
   RPackage *getPackage(pkgCache::PkgIterator &pkg);
   RPackage *getPackage(string name);
   int getPackageIndex(RPackage *pkg);
   int getViewPackageIndex(RPackage *pkg);

   int packagesSize() { return _packages.size(); };
   int viewPackagesSize() { return _updating ? 0 : _viewPackages.size(); };

#if 0
   inline unsigned count() {
      return _updating ? 0 : _viewPackages.size();
   };
   inline RPackage *getElement(int row) {
      if (!_updating && row < (int)_viewPackages.size())
         return _viewPackages[row];
      else
         return NULL;
   };
#endif

   void getStats(int &installed, int &broken, int &toInstall, int &toReInstall,
		 int &toRemove, double &sizeChange);

   void getSummary(int &held, int &kept, int &essential,
                   int &toInstall, int &toReInstall, int &toUpgrade, 
		   int &toRemove,  int &toDowngrade, double &sizeChange);


   void getDetailedSummary(vector<RPackage *> &held,
                           vector<RPackage *> &kept,
                           vector<RPackage *> &essential,
                           vector<RPackage *> &toInstall,
                           vector<RPackage *> &toReInstall,
                           vector<RPackage *> &toUpgrade,
                           vector<RPackage *> &toRemove,
                           vector<RPackage *> &toDowngrade,
                           double &sizeChange);

   void getDownloadSummary(int &dlCount, double &dlSize);

   void saveUndoState(pkgState &state);
   void saveUndoState();
   void undo();
   void redo();
   void saveState(pkgState &state);
   void restoreState(pkgState &state);
   bool getStateChanges(pkgState &state,
                        vector<RPackage *> &kept,
                        vector<RPackage *> &toInstall,
			vector<RPackage *> &toReInstall,
                        vector<RPackage *> &toUpgrade,
                        vector<RPackage *> &toRemove,
                        vector<RPackage *> &toDowngrade,
                        vector<RPackage *> &exclude, bool sorted = true);

   bool openCache(bool reset);

   bool fixBroken();

   bool check();
   bool upgradable();

   bool upgrade();
   bool distUpgrade();

   bool cleanPackageCache(bool forceClean = false);

   bool updateCache(pkgAcquireStatus *status);
   bool commitChanges(pkgAcquireStatus *status, RInstallProgress *iprog);

   inline void setProgressMeter(OpProgress *progMeter) {
      _progMeter = progMeter;
   };

   inline void setUserDialog(RUserDialog *dialog) {
      _userDialog = dialog;
   };

   // policy stuff                             
   vector<string> getPolicyArchives() {
      if (_cacheValid)
         return _cache->getPolicyArchives();
      else
         return vector<string>();
   };

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

   bool readSelections(istream &in);
   bool writeSelections(ostream &out, bool fullState);

   RPackageLister();
   ~RPackageLister();
};


#endif

// vim:sts=3:sw=3
