/* rgchangeswindow.h
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

#pragma once

#include "config.h"  // IWYU pragma: associated

#include "rggtkbuilderwindow.h"

#include <gtk/gtk.h>
#include <vector>

class RGWindow;
class RPackage;
class RPackageLister;

class RGChangesWindow:public RGGtkBuilderWindow {
   enum {
      PKG_COLUMN,
      N_COLUMNS
   };

   GtkWidget *_tree;
   GtkTreeStore *_treeStore;

 public:
   RGChangesWindow(RGWindow *win);

   void confirm(RPackageLister *lister,
		std::vector<RPackage *> &kept,
		std::vector<RPackage *> &toInstall,
		std::vector<RPackage *> &toReInstall,
		std::vector<RPackage *> &toUpgrade,
		std::vector<RPackage *> &toRemove,
		std::vector<RPackage *> &toDowngrade,
		std::vector<RPackage *> &notAuthenticated);
};
