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
#include <map>
#include <set>
#include <regex.h>

#include "rpackage.h"
#include "tree.hh"

using namespace std;

class OpProgress;
class RPackageCache;
class RPackageFilter;
class pkgRecords;
class pkgAcquireStatus;
class pkgPackageManager;


struct RFilter;


class RInstallProgress;

class RPackageObserver {
public:
   virtual void notifyChange(RPackage *pkg) = 0;
};


class RUserDialog {
public:
   virtual bool confirm(char *title, char *message) = 0;
};


class RPackageLister
{
private:
   RPackageCache *_cache;
   pkgRecords *_records;
   OpProgress *_progMeter;
   
   RPackage **_packages;
   unsigned _count;
   set<string> allPackages; //all known packages (needed identifing "new" pkgs)
   
   bool _updating; // can't access anything while this is true
   
   int _installedCount; // # of installed packages
   
   vector<RFilter*> _filterL;
   RFilter *_filter; // effective filter, NULL for none
   vector<RPackage*> _displayList; // list of packages after filter

   vector<string> _sectionList; // list of all available package sections

public:
   typedef pair<string,RPackage*> pkgPair;
   typedef tree<pkgPair>::iterator treeIter;
   typedef tree<pkgPair>::sibling_iterator sibTreeIter;
   typedef tree<pkgPair>::tree_node treeNode;
   typedef enum {
     TREE_DISPLAY_SECTIONS,
     TREE_DISPLAY_FLAT,
     TREE_DISPLAY_ALPHABETIC,
     TREE_DISPLAY_STATUS
   } treeDisplayMode;
   treeDisplayMode _displayMode;

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

   vector<RPackageObserver*> _observers;
   
   RUserDialog *_userDialog;
   
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

   bool registerFilter(RFilter *filter);
   void unregisterFilter(RFilter *filter);
   void getFilterNames(vector<string> &filters);
   inline RFilter *findFilter(int index) { return _filterL[index]; };

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

   typedef vector<RPackage::MarkedStatus> pkgState;
   void saveState(pkgState &state);
   void restoreState(pkgState &state);
   bool getStateChanges(pkgState &state,
			vector<RPackage*> &kept,
			vector<RPackage*> &toInstall, 
			vector<RPackage*> &toUpgrade, 
			vector<RPackage*> &toRemove,
			RPackage *exclude = NULL);

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
   void notifyChange(RPackage *pkg);
   void registerObserver(RPackageObserver *observer);
   void unregisterObserver(RPackageObserver *observer);

   bool readSelections(istream &in);
   
   RPackageLister();   
};


#endif
