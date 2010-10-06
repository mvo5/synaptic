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
#include <cassert>

#include "rgchangeswindow.h"

#include "i18n.h"


RGChangesWindow::RGChangesWindow(RGWindow *wwin)
: RGGtkBuilderWindow(wwin, "changes")
{
   // new tree store
   _treeStore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING);
   _tree = GTK_WIDGET(gtk_builder_get_object(_builder, "tree"));
   gtk_tree_view_set_model(GTK_TREE_VIEW(_tree), GTK_TREE_MODEL(_treeStore));

   GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
   GtkTreeViewColumn *column;
   column = gtk_tree_view_column_new_with_attributes(_("Package changes"),
                                                     renderer,
                                                     "markup", PKG_COLUMN, NULL);
   /* Add the column to the view. */
   gtk_tree_view_append_column(GTK_TREE_VIEW(_tree), column);
   gtk_widget_show(_tree);
   
   gtk_dialog_set_default_response (GTK_DIALOG(_win),
				    GTK_RESPONSE_OK);
}



void RGChangesWindow::confirm(RPackageLister *lister,
			      vector<RPackage *> &kept,
			      vector<RPackage *> &toInstall,
			      vector<RPackage *> &toReInstall,
			      vector<RPackage *> &toUpgrade,
			      vector<RPackage *> &toRemove,
			      vector<RPackage *> &toDowngrade,
			      vector<RPackage *> &notAuthenticated)
{
   GtkTreeIter iter, iter_child;
   GtkWidget *label;

   if(!(toInstall.size() || toReInstall.size() || toUpgrade.size() ||
	toRemove.size() || toDowngrade.size())) {
      // we have no changes other than authentication warnings
      label = GTK_WIDGET(gtk_builder_get_object(_builder, "label_changes_header"));
      gtk_widget_hide(label);
      label = GTK_WIDGET(gtk_builder_get_object(_builder, "label_secondary"));
      gtk_widget_hide(label);
   }

  if (notAuthenticated.size() > 0) {
      label = GTK_WIDGET(gtk_builder_get_object(_builder, "label_auth_warning"));
      assert(label);
      // FIXME: make this a message from a trust class (and remeber to
      // change the text in rgsummarywindow then too)
      gchar *msg = g_strdup_printf("<span weight=\"bold\" size=\"larger\">%s"
				   "</span>\n\n%s",
				   _("Warning"), 
				   _("You are about to install software that "
				     "<b>can't be authenticated</b>! Doing "
				     "this could allow a malicious individual "
				     "to damage or take control of your "
				     "system."));
      gtk_label_set_markup(GTK_LABEL(label), msg);
      gtk_widget_show(label);
      g_free(msg);

      gtk_tree_store_append(_treeStore, &iter, NULL);
      gtk_tree_store_set(_treeStore, &iter,
                         PKG_COLUMN, _("NOT AUTHENTICATED"), -1);
      for (vector<RPackage *>::const_iterator p = notAuthenticated.begin();
           p != notAuthenticated.end(); p++) {
         gtk_tree_store_append(_treeStore, &iter_child, &iter);
         gtk_tree_store_set(_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

   if (toRemove.size() > 0) {
      /* removed */
      gchar *str = g_strdup_printf("<b>%s</b>", _("To be removed"));
      gtk_tree_store_append(_treeStore, &iter, NULL);
      gtk_tree_store_set(_treeStore, &iter,
                         PKG_COLUMN, str, -1);
      for (vector<RPackage *>::const_iterator p = toRemove.begin();
           p != toRemove.end(); p++) {
         gtk_tree_store_append(_treeStore, &iter_child, &iter);
         gtk_tree_store_set(_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
      g_free(str);
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

   if (toReInstall.size() > 0) {
      gtk_tree_store_append(_treeStore, &iter, NULL);
      gtk_tree_store_set(_treeStore, &iter,
                         PKG_COLUMN, _("To be re-installed"), -1);
      for (vector<RPackage *>::const_iterator p = toReInstall.begin();
           p != toReInstall.end(); p++) {
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
