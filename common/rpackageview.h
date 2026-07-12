/* rpackageview.h - Package sectioning system
 *
 * Copyright (c) 2004 Conectiva S/A
 *               2004 Michael Vogt <mvo@debian.org>
 *
 * Author: Gustavo Niemeyer
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

#pragma once

#include "config.h" // IWYU pragma: associated

#include "i18n.h"
#include "rpackage.h"
#include "sections_trans.h"

#include <cctype>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

class OpProgress;
struct RFilter;

enum {
   PACKAGE_VIEW_SECTION,
   PACKAGE_VIEW_STATUS,
   PACKAGE_VIEW_ORIGIN,
   PACKAGE_VIEW_CUSTOM,
   PACKAGE_VIEW_SEARCH,
   PACKAGE_VIEW_ARCHITECTURE,
   N_PACKAGE_VIEWS
};

class RPackageView
{
 protected:
   std::map<std::string, std::vector<RPackage *>> _view;

   bool _hasSelection;
   std::string _selectedName;

   // packages in selected
   std::vector<RPackage *> _selectedView;

   // all packages in current global filter
   std::vector<RPackage *> &_all;

 public:
   RPackageView(std::vector<RPackage *> &allPackages) : _all(allPackages)
   {}
   virtual ~RPackageView()
   {}

   bool hasSelection()
   {
      return _hasSelection;
   }
   std::string getSelected()
   {
      return _selectedName;
   }
   bool hasPackage(RPackage *pkg);
   virtual bool setSelected(std::string name);

   void showAll()
   {
      _selectedView = _all;
      _hasSelection = false;
      _selectedName.clear();
   }

   virtual std::vector<std::string> getSubViews() const;

   virtual std::string getName() const = 0;
   virtual void addPackage(RPackage *package) = 0;

   typedef std::vector<RPackage *>::iterator iterator;

   virtual iterator begin()
   {
      return _selectedView.begin();
   }
   virtual iterator end()
   {
      return _selectedView.end();
   }

   virtual void clear();
   virtual void clearSelection();

   virtual void refresh();
};

class RPackageViewSections : public RPackageView
{
 public:
   RPackageViewSections(std::vector<RPackage *> &allPkgs)
      : RPackageView(allPkgs)
   {}

   std::string getName() const
   {
      return _("Sections");
   };

   inline void addPackage(RPackage *package)
   {
      std::string section = trans_section(package->section());
      section[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(section[0])));
      _view[section].push_back(package);
   }
};

class RPackageViewAlphabetic : public RPackageView
{
 public:
   RPackageViewAlphabetic(std::vector<RPackage *> &allPkgs)
      : RPackageView(allPkgs)
   {}
   std::string getName() const
   {
      return _("Alphabetic");
   }

   void addPackage(RPackage *package)
   {
      char letter[2] = {' ', '\0'};
      letter[0] = toupper(package->name()[0]);
      _view[letter].push_back(package);
   }
};

class RPackageViewArchitecture : public RPackageView
{
 public:
   RPackageViewArchitecture(std::vector<RPackage *> &allPkgs)
      : RPackageView(allPkgs)
   {}
   std::string getName() const
   {
      return _("Architecture");
   }

   void addPackage(RPackage *package);
};

class RPackageViewOrigin : public RPackageView
{
 public:
   RPackageViewOrigin(std::vector<RPackage *> &allPkgs) : RPackageView(allPkgs)
   {}
   std::string getName() const
   {
      return _("Origin");
   }

   void addPackage(RPackage *package);
};

class RPackageViewStatus : public RPackageView
{
 protected:
   // mark the software as unsupported in status view
   bool markUnsupported;
   std::vector<std::string> supportedComponents;

 public:
   RPackageViewStatus(std::vector<RPackage *> &allPkgs);

   std::string getName() const
   {
      return _("Status");
   }

   void addPackage(RPackage *package);
};

class RPackageViewSearch : public RPackageView
{
   struct searchItem
   {
      std::vector<std::string> searchStrings;
      std::string searchName;
      int searchType;
   };
   // the search history
   std::map<std::string, searchItem> searchHistory;
   searchItem _currentSearchItem;
   int found; // nr of found pkgs for the last search

   bool xapianSearch();

 public:
   RPackageViewSearch(std::vector<RPackage *> &allPkgs)
      : RPackageView(allPkgs), found(0)
   {}

   int setSearch(std::string searchName,
                 int type,
                 std::string searchString,
                 OpProgress &searchProgress);

   std::string getName() const
   {
      return _("Search History");
   }

   // return search history here
   virtual std::vector<std::string> getSubViews() const;
   virtual bool setSelected(std::string name);

   void addPackage(RPackage *package);

   // no-op
   virtual void refresh()
   {}
};


class RPackageViewFilter : public RPackageView
{
 protected:
   std::vector<RFilter *> _filterL;
   std::set<std::string> _sectionList; // list of all available package sections

 public:
   void storeFilters();
   void restoreFilters();
   // called after the filtereditor was run
   void refreshFilters();
   void refresh();

   bool registerFilter(RFilter *filter);
   void unregisterFilter(RFilter *filter);

   void makePresetFilters();

   RFilter *findFilter(std::string name);
   unsigned int nrOfFilters()
   {
      return _filterL.size();
   }
   RFilter *findFilter(unsigned int index)
   {
      if (index > _filterL.size())
         return NULL;
      else
         return _filterL[index];
   }

   // used by kynaptic
   int getFilterIndex(RFilter *filter);

   std::vector<std::string> getFilterNames();
   const std::set<std::string> &getSections();

   RPackageViewFilter(std::vector<RPackage *> &allPkgs);

   // build packages list on "demand"
   virtual iterator begin();

   // we never need to clear because we build the view "on-demand"
   virtual void clear()
   {
      clearSelection();
   }

   std::string getName() const
   {
      return _("Custom");
   }

   void addPackage(RPackage *package);
};
