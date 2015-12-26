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
#include <apt-pkg/acquire.h>
#include <apt-pkg/progress.h>

#ifdef WITH_EPT
#include <ept/axi/axi.h>
#endif

#include "rpackagecache.h"
#include "rpackage.h"
#include "rpackagestatus.h"
#include "rpackageview.h"
#include "ruserdialog.h"
#include "config.h"

using namespace std;

class OpProgress;
class RPackageCache;
class RPackageFilter;
class RCacheActor;
class RPackageViewFilter;
class RPackageViewSearch;
class pkgRecords;
class pkgAcquireStatus;
class pkgPackageManager;


struct RFilter;
class RPackageView;

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

// base sort class
// for a example use see sortPackages()
template<class T>
class sortFunc {
 protected:
   bool _ascent;
   T cmp;
 public:
   sortFunc(bool ascent) : _ascent(ascent) {};
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

#ifdef WITH_EPT
   Xapian::Database *_xapianDatabase;
#endif


   // Other members.
   vector<RPackage *> _packages;
   vector<int> _packagesIndex;

   vector<RPackage *> _viewPackages;
   vector<int> _viewPackagesIndex;

   // this is what we feed to the views as "all packages" to avoid
   // to show all the multiarch versions by default, the user can
   // turn that off with a config option
   vector<RPackage *> _nativeArchPackages;

   // It shouldn't be needed to control this inside this class. -- niemeyer
   bool _updating;

   // all known packages (needed identifing "new" pkgs)
   set<string> packageNames; 

   bool _cacheValid;            // is the cache valid?

   int _installedCount;         // # of installed packages

   vector<RCacheActor *> _actors;

   RPackageViewFilter *_filterView; // the package view that does the filtering
   RPackageViewSearch *_searchView; // the package view that does the (simple) search

   // helper for the limitBySearch() code
   bool xapianSearch(string searchString);

   public:

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
   typedef vector<int> pkgState;
#endif

   private:

   vector<RPackageView *> _views;
   RPackageView *_selectedView;
   RPackageStatus _pkgStatus;
   
   void applyInitialSelection();

   bool lockPackageCache(FileFd &lock);

   void sortPackages(vector<RPackage *> &packages,listSortMode mode);

   struct {
      char *pattern;
      regex_t regex;
      bool isRegex;
      int last;
   } _searchData;

   vector<RPackageObserver *> _packageObservers;
   vector<RCacheObserver *> _cacheObservers;

   RUserDialog *_userDialog;

   void makeCommitLog();
   void writeCommitLog();
   string _logEntry;
   time_t _logTime;

   // undo/redo stuff
   list<pkgState> undoStack;
   list<pkgState> redoStack;

   public:
   // limit what the current view displays
   bool limitBySearch(string searchString);

   // clean files older than "Synaptic::delHistory"
   void cleanCommitLog();

   void sortPackages(listSortMode mode) {
      sortPackages(_viewPackages, mode);
   };

   void setView(unsigned int index);
   vector<string> getViews();
   vector<string> getSubViews();

   // set subView (if newView is empty, set to all packages)
   bool setSubView(string newView="");

   // this needs a different name, something like refresh
   void reapplyFilter();
   
   // refresh view 
   void refreshView();

   // is is exposed for the stuff like filter manager window
   RPackageViewFilter *filterView() { return _filterView; };
   RPackageViewSearch *searchView() { return _searchView; };

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

   void getStats(int &installed, int &broken, int &toInstall,
		 int &toRemove, double &sizeChange);

   void getSummary(int &held, int &kept, int &essential,
                   int &toInstall, int &toReInstall, int &toUpgrade, 
		   int &toRemove,  int &toDowngrade, 
		   int &unAuthenticated,  double &sizeChange);


   void getDetailedSummary(vector<RPackage *> &held,
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
			vector<RPackage *> &notAuthenticated,
                        const vector<RPackage *> &exclude, 
                        bool sorted = true);

   // open (lock if run as root)
   bool openCache();
   bool fixBroken();
   bool check();
   bool upgradable();
   bool upgrade();
   bool distUpgrade();
   bool cleanPackageCache(bool forceClean = false);
   bool updateCache(pkgAcquireStatus *status, string &error);
   bool commitChanges(pkgAcquireStatus *status, RInstallProgress *iprog);

   // some information
   bool getDownloadUris(vector<string> &uris);
   bool addArchiveToCache(string archiveDir, string &pkgname);

   void setProgressMeter(OpProgress *progMeter) {
      if(_progMeter != NULL)
	 delete _progMeter;
      _progMeter = progMeter;
   };

   void setUserDialog(RUserDialog *dialog) {
      _userDialog = dialog;
   };

   // policy stuff                             
   vector<string> getPolicyArchives(bool filenames_only=false) {
      if (_cacheValid)
         return _cache->getPolicyArchives(filenames_only);
      else
         return vector<string>();
   };

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

   bool readSelections(istream &in);
   bool writeSelections(ostream &out, bool fullState);

   RPackageCache* getCache() { return _cache; };
#ifdef WITH_EPT
   Xapian::Database* xapiandatabase() { return _xapianDatabase; }
   bool xapianIndexNeedsUpdate();
   bool openXapianIndex();
#endif

   RPackageLister();
   ~RPackageLister();
};


#endif

// vim:ts=3:sw=3:et
