/* rgchangeswindow.cc
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

#include "config.h"

#include <X11/keysym.h>

#include <apt-pkg/configuration.h>
#include <apt-pkg/strutl.h>

#include "rpackagelister.h"

#include <stdio.h>
#include <string>

#include "rgchangeswindow.h"

#include "i18n.h"


RGChangesWindow::RGChangesWindow(RGWindow *wwin)
: RGGladeWindow(wwin, "changes")
{
   setTitle("");
   // new tree store
   _treeStore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING);
   _tree = glade_xml_get_widget(_gladeXML, "tree");
   gtk_tree_view_set_model(GTK_TREE_VIEW(_tree), GTK_TREE_MODEL(_treeStore));

   GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
   GtkTreeViewColumn *column;
   column = gtk_tree_view_column_new_with_attributes(_("Package changes"),
                                                     renderer,
                                                     "text", PKG_COLUMN, NULL);
   /* Add the column to the view. */
   gtk_tree_view_append_column(GTK_TREE_VIEW(_tree), column);
   gtk_widget_show(_tree);
}



void RGChangesWindow::confirm(RPackageLister *lister,
                                     vector<RPackage *> &kept,
                                     vector<RPackage *> &toInstall,
                                     vector<RPackage *> &toReInstall,
                                     vector<RPackage *> &toUpgrade,
                                     vector<RPackage *> &toRemove,
                                     vector<RPackage *> &toDowngrade)
{
   GtkTreeIter iter, iter_child;

   if (toRemove.size() > 0) {
      /* removed */
      gtk_tree_store_append(_treeStore, &iter, NULL);
      gtk_tree_store_set(_treeStore, &iter,
                         PKG_COLUMN, _("To be removed"), -1);
      for (vector<RPackage *>::const_iterator p = toRemove.begin();
           p != toRemove.end(); p++) {
         gtk_tree_store_append(_treeStore, &iter_child, &iter);
         gtk_tree_store_set(_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

   if (toDowngrade.size() > 0) {
      /* downgrade */
      gtk_tree_store_append(_treeStore, &iter, NULL);
      gtk_tree_store_set(_treeStore, &iter,
                         PKG_COLUMN, _("To be downgraded"), -1);
      for (vector<RPackage *>::const_iterator p = toDowngrade.begin();
           p != toDowngrade.end(); p++) {
         gtk_tree_store_append(_treeStore, &iter_child, &iter);
         gtk_tree_store_set(_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }


   if (toUpgrade.size() > 0) {
      gtk_tree_store_append(_treeStore, &iter, NULL);
      gtk_tree_store_set(_treeStore, &iter,
                         PKG_COLUMN, _("To be upgraded"), -1);
      for (vector<RPackage *>::const_iterator p = toUpgrade.begin();
           p != toUpgrade.end(); p++) {
         gtk_tree_store_append(_treeStore, &iter_child, &iter);
         gtk_tree_store_set(_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

   if (toInstall.size() > 0) {
      gtk_tree_store_append(_treeStore, &iter, NULL);
      gtk_tree_store_set(_treeStore, &iter,
                         PKG_COLUMN, _("To be installed"), -1);
      for (vector<RPackage *>::const_iterator p = toInstall.begin();
           p != toInstall.end(); p++) {
         gtk_tree_store_append(_treeStore, &iter_child, &iter);
         gtk_tree_store_set(_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

   if (toReInstall.size() > 0) {
      gtk_tree_store_append(_treeStore, &iter, NULL);
      gtk_tree_store_set(_treeStore, &iter,
                         PKG_COLUMN, _("To be re-installed"), -1);
      for (vector<RPackage *>::const_iterator p = toReInstall.begin();
           p != toInstall.end(); p++) {
         gtk_tree_store_append(_treeStore, &iter_child, &iter);
         gtk_tree_store_set(_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

   if (kept.size() > 0) {
      gtk_tree_store_append(_treeStore, &iter, NULL);
      gtk_tree_store_set(_treeStore, &iter, PKG_COLUMN, _("To be kept"), -1);
      for (vector<RPackage *>::const_iterator p = kept.begin();
           p != kept.end(); p++) {
         gtk_tree_store_append(_treeStore, &iter_child, &iter);
         gtk_tree_store_set(_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

   gtk_tree_view_expand_all(GTK_TREE_VIEW(_tree));
}
