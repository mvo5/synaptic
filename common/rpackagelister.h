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

#include "rpackage.h"
#include "ruserdialog.h"
#include "tree.hh"

#include "config.h"

#include <apt-pkg/depcache.h>

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

class RPackageLister
{
private:
   RPackageCache *_cache;
   pkgRecords *_records;
   OpProgress *_progMeter;
   
   RPackage **_packages;
   int *_packageindex;
   unsigned _count;
   set<string> allPackages; //all known packages (needed identifing "new" pkgs)
   
   bool _updating; // can't access anything while this is true
   
   int _installedCount; // # of installed packages
   
   vector<RFilter*> _filterL;
   RFilter *_filter; // effective filter, NULL for none
   vector<RPackage*> _displayList; // list of packages after filter

   vector<string> _sectionList; // list of all available package sections

   vector<RCacheActor*> _actors;
#if 0
   tree<string> virtualPackages; // all virtual packages
#endif

public:
   typedef pair<string,RPackage*> pkgPair;
   typedef tree<pkgPair>::iterator treeIter;
   typedef tree<pkgPair>::sibling_iterator sibTreeIter;
   typedef tree<pkgPair>::tree_node treeNode;
   typedef enum {
     TREE_DISPLAY_SECTIONS,
     TREE_DISPLAY_ALPHABETIC,
     TREE_DISPLAY_STATUS,
     TREE_DISPLAY_FLAT
   } treeDisplayMode;
   treeDisplayMode _displayMode;

#ifdef HAVE_RPM
   typedef pkgDepCache::State pkgState;
#else
   typedef vector<RPackage::MarkedStatus> pkgState;
#endif

private:
   tree<pkgPair> _treeOrganizer;

   pkgPackageManager *_packMan;

   void applyInitialSelection();
   
   void makePresetFilters();
   
   bool applyFilters(RPackage *package);

   bool lockPackageCache(FileFd &lock);

   void getFilteredPackages(vector<RPackage*> &packages);
   void addFilteredPackageToTree(tree<pkgPair>& tree,
				 map<string,tree<pkgPair>::iterator>& itermap,
				 RPackage *pkg);

   void sortPackagesByName(vector<RPackage*> &packages);
   
   struct {
       char *pattern;       
       regex_t regex;
       bool isRegex;
       int last;
   } _searchData;

   vector<RPackageObserver*> _packageObservers;
   vector<RCacheObserver*> _cacheObservers;
   
   RUserDialog *_userDialog;

   // undo/redo stuff
   list<pkgState> undoStack;
   list<pkgState> redoStack;

  
public:
   inline void getSections(vector<string> &sections) {sections=_sectionList; };

   inline int nrOfSections() { return _sectionList.size(); };

   inline tree<pkgPair>* RPackageLister::getTreeOrganizer() { 
     return &_treeOrganizer; 
   };
   inline void setTreeDisplayMode(treeDisplayMode mode) {
     //cout << "setTreeDisplayMode() " << mode << endl;
     _displayMode = mode;
   };
   inline treeDisplayMode getTreeDisplayMode() {
     return _displayMode;
   };
   
   void storeFilters();
   void restoreFilters();

   // filter management
   void setFilter(int index=-1);
   // note that setFilter will only set it if the filter is already registered
   void setFilter(RFilter *filter);
   inline RFilter *getFilter() { return _filter; };
   int getFilterIndex(RFilter *filter=NULL);
   unsigned int nrOfFilters() { return _filterL.size(); };

   bool registerFilter(RFilter *filter);
   void unregisterFilter(RFilter *filter);
   void getFilterNames(vector<string> &filters);
   inline RFilter *findFilter(unsigned int index) { 
       if(index > _filterL.size()) return NULL; else return _filterL[index]; 
   };

   void reapplyFilter();
   
   // find 
   int findPackage(const char *pattern);
   int findNextPackage();

   inline unsigned count() { return _updating ? 0 : _displayList.size(); };
   inline RPackage *getElement(int row) { 
       if (!_updating && row < (int)_displayList.size()) 
	   return _displayList[row];
       else
	   return NULL;
   };
   int getElementIndex(RPackage *pkg);
   RPackage *getElement(pkgCache::PkgIterator &pkg);
   RPackage *getElement(string Name);

   void getStats(int &installed, int &broken, int &toinstall, int &toremove,
		 double &sizeChange);
   
   void getSummary(int &held, int &kept, int &essential,
		   int &toInstall, int &toUpgrade, int &toRemove,
		   double &sizeChange);
   

   void getDetailedSummary(vector<RPackage*> &held, 
			   vector<RPackage*> &kept, 
			   vector<RPackage*> &essential,
			   vector<RPackage*> &toInstall, 
			   vector<RPackage*> &toUpgrade, 
			   vector<RPackage*> &toRemove,
			   double &sizeChange);   

   void getDownloadSummary(int &dlCount, double &dlSize);

   void saveUndoState(pkgState &state);
   void saveUndoState();
   void undo();
   void redo();
   void saveState(pkgState &state);
   void restoreState(pkgState &state);
   bool getStateChanges(pkgState &state,
			vector<RPackage*> &kept,
			vector<RPackage*> &toInstall, 
			vector<RPackage*> &toUpgrade, 
			vector<RPackage*> &toRemove,
			vector<RPackage*> &exclude,
			bool sorted=true);

   bool openCache(bool reset);

   bool fixBroken();
   
   bool check();
   bool upgradable();
   
   bool upgrade();
   bool distUpgrade();
   
   bool cleanPackageCache();

   bool updateCache(pkgAcquireStatus *status);
   bool commitChanges(pkgAcquireStatus *status, RInstallProgress *iprog);
   
   inline void setProgressMeter(OpProgress *progMeter) { _progMeter = progMeter; };

   inline void setUserDialog(RUserDialog *dialog) { _userDialog = dialog; };

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

// vim:sts=4:sw=4
