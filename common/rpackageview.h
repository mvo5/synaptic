/* rpackageview.h - Package sectioning system
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


#ifndef RPACKAGEVIEW_H
#define RPACKAGEVIEW_H

#include <string>
#include <map>

#include "rpackage.h"

#include "i18n.h"

using namespace std;

class RPackageView {
 protected:

   map<string, vector<RPackage *> > _view;

   bool _hasSelection;
   string _selectedName;
   vector<RPackage *> _selectedView;

 public:
   RPackageView() {
   };
   virtual ~ RPackageView() {
   };

   bool hasSelection() {
      return _hasSelection;
   };
   string getSelected() {
      return _selectedName;
   };
   bool setSelected(string name);

   vector<string> getSubViews();

   virtual string getName() = 0;
   virtual void addPackage(RPackage *package) = 0;

   typedef vector<RPackage *>::iterator iterator;

   iterator begin() {
      return _selectedView.begin();
   };
   iterator end() {
      return _selectedView.end();
   };

   virtual void clear();
   virtual void clearSelection();
};

class RPackageViewSections:public RPackageView {
 public:

   string getName() {
      return _("Sections");
   };

   void addPackage(RPackage *package) {
      _view[package->section()].push_back(package);
   };
};

class RPackageViewAlphabetic : public RPackageView {
 public:

   string getName() {
      return _("Alphabetic");
   };

   void addPackage(RPackage *package) {
      char letter[2] = { ' ', '\0' };
      letter[0] = toupper(package->name()[0]);
      _view[letter].push_back(package);
   };
};

class RPackageViewAll:public RPackageView {
 public:

   string getName() {
      return _("All Packages");
   };

   void addPackage(RPackage *package) {
      _selectedView.push_back(package);
   };

   RPackageViewAll() {
      _hasSelection = true;
   };

   void clear() {
      _selectedView.clear();
   };
   void clearSelection() {};
};

#endif

// vim:sts=3:sw=3
