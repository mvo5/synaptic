/* rpackageview.cc - Package sectioning system
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

#include <rpackage.h>
#include <rpackageview.h>

#include <map>
#include <vector>

using namespace std;

bool RPackageView::setSelected(string name)
{
   map<string, vector<RPackage *> >::iterator I = _view.find(name);
   if (I != _view.end()) {
      _hasSelection = true;
      _selectedName = name;
      _selectedView = (*I).second;
   } else {
      clearSelection();
   }
   return _hasSelection;;
}

vector<string> RPackageView::getSubViews()
{
   vector<string> subViews;
   for (map<string, vector<RPackage *> >::iterator I = _view.begin();
        I != _view.end(); I++)
      subViews.push_back((*I).first);
   return subViews;
}

void RPackageView::clear()
{
   clearSelection();
   _view.clear();
}

void RPackageView::clearSelection()
{
   _hasSelection = false;
   _selectedName.clear();
   _selectedView.clear();
}

void RPackageViewStatus::addPackage(RPackage *pkg)
{
   string str;
   int flags = pkg->getFlags();

   if (flags & RPackage::FNew)
      str = _("New in repository");
   else if ((flags & RPackage::FNotInstallable) &&
	    !(flags & RPackage::FResidualConfig))
      str = _("Installed and obsolete");
   else if (flags & RPackage::FInstalled) {
      if (flags & RPackage::FOutdated)
	 str = _("Installed and upgradable");
      else
	 str = _("Installed");
   } else {
      if (flags & RPackage::FResidualConfig)
	 str = _("Not installed (residual config)");
      else
	 str = _("Not installed");
   }

   _view[str].push_back(pkg);
}

// vim:sts=3:sw=3
