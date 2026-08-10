/* rggtkbuilderwindow.cc
 *
 * Copyright (c) 2003 Michael Vogt
 *
 * Author: Michael Vogt <mvo@debian.org>
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

#include "config.h" // IWYU pragma: associated

#include "rggtkbuilderwindow.h"

#include "i18n.h"
#include "rgutils.h"
#include "rgwindow.h"

#include <apt-pkg/configuration.h>
#include <cassert>
#include <cstddef>
#include <gtk/gtk.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

/*  the convention is that the top window is named:
   "window_$windowname" in your gtkbuilder-source
*/
RGGtkBuilderWindow::RGGtkBuilderWindow(RGWindow *parent,
                                       string name,
                                       string mainName)
{
   _builder = gtk_builder_new();

   gchar *main_widget = NULL;
   GError *error = NULL;

   if (mainName.empty())
      main_widget = g_strdup_printf("window_%s", name.c_str());
   else
      main_widget = g_strdup_printf("window_%s", mainName.c_str());

   std::string resource_path =
      "/io/github/mvo5/synaptic/ui/window_" + name + ".ui";
   if (!gtk_builder_add_from_resource(
          _builder, resource_path.c_str(), &error)) {
      g_warning("Couldn't load builder resource: %s", error->message);
      g_error_free(error);
   }

   _win = GTK_WIDGET(gtk_builder_get_object(_builder, main_widget));
   assert(_win);
   init();

   if (parent != NULL)
      gtk_window_set_transient_for(GTK_WINDOW(_win),
                                   GTK_WINDOW(parent->window()));

   gtk_window_set_icon_name(GTK_WINDOW(_win), "synaptic");

   g_free(main_widget);
}

bool RGGtkBuilderWindow::setLabel(const char *widget_name, const char *value)
{
   GtkWidget *widget =
      GTK_WIDGET(gtk_builder_get_object(_builder, widget_name));
   if (widget == NULL) {
      cout << "widget == NULL with: " << widget_name << endl;
      return false;
   }

   const gchar *utf8value = utf8(value);

   if (!utf8value)
      utf8value = _("N/A");
   gtk_label_set_label(GTK_LABEL(widget), utf8value);
   return true;
}

bool RGGtkBuilderWindow::setMarkup(const char *widget_name, const char *value)
{
   GtkWidget *widget =
      GTK_WIDGET(gtk_builder_get_object(_builder, widget_name));
   if (widget == NULL) {
      cout << "widget == NULL with: " << widget_name << endl;
      return false;
   }

   const gchar *utf8value = utf8(value);

   if (!utf8value)
      utf8value = _("N/A");
   gtk_label_set_markup(GTK_LABEL(widget), utf8value);
   return true;
}

bool RGGtkBuilderWindow::setLabel(const char *widget_name, const long value)
{
   string strVal;
   GtkWidget *widget =
      GTK_WIDGET(gtk_builder_get_object(_builder, widget_name));
   if (widget == NULL) {
      cout << "widget == NULL with: " << widget_name << endl;
      return false;
   }
   // we can never have values of zero or less
   if (value <= 0)
      // TRANSLATORS: this is a abbreviation for "not applicable" (on forms)
      // happens when e.g. a package has no installed version (or no
      // downloadable version)
      strVal = _("N/A");
   else
      strVal = SizeToStr(value);
   gtk_label_set_label(GTK_LABEL(widget), utf8(strVal.c_str()));
   return true;
}

bool RGGtkBuilderWindow::setTreeList(const char *widget_name,
                                     vector<string> values,
                                     bool use_markup)
{
   GtkWidget *widget =
      GTK_WIDGET(gtk_builder_get_object(_builder, widget_name));
   if (widget == NULL) {
      cout << "widget == NULL with: " << widget_name << endl;
      return false;
   }

   // create column (if needed)
   if (gtk_tree_view_get_column(GTK_TREE_VIEW(widget), 0) == NULL) {
      GtkCellRenderer *renderer = gtk_cell_renderer_text_new(); // cell renderer
      const char *type = use_markup ? "markup" : "text";
      GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
         "SubView", renderer, type, 0, NULL);
      gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);
   }

   // store stuff
   GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
   GtkTreeIter iter;
   for (unsigned int i = 0; i < values.size(); i++) {
      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter, 0, utf8(values[i].c_str()), -1);
   }

   gtk_tree_view_set_model(GTK_TREE_VIEW(widget), GTK_TREE_MODEL(store));
   return true;
}

bool RGGtkBuilderWindow::setTextView(const char *widget_name,
                                     const char *value,
                                     bool use_headline)
{
   GtkTextIter start, end;

   GtkWidget *view = GTK_WIDGET(gtk_builder_get_object(_builder, widget_name));
   if (view == NULL) {
      cout << "textview == NULL with: " << widget_name << endl;
      return false;
   }

   GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
   const gchar *utf8value = utf8(value);
   if (!utf8value)
      utf8value = _("N/A");
   gtk_text_buffer_set_text(buffer, utf8value, -1);

   if (use_headline) {
      GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);
      if (gtk_text_tag_table_lookup(tag_table, "bold") == NULL) {
         gtk_text_buffer_create_tag(
            buffer, "bold", "weight", PANGO_WEIGHT_BOLD, "scale", 1.1, NULL);
      }
      gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);

      gtk_text_buffer_get_iter_at_offset(buffer, &end, 0);
      gtk_text_iter_forward_line(&end);

      gtk_text_buffer_apply_tag_by_name(buffer, "bold", &start, &end);
   }

   gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);
   gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(view), &start, 0, FALSE, 0, 0);
   return true;
}

bool RGGtkBuilderWindow::setPixmap(const char *widget_name, const char *value)
{
   GtkWidget *pix = GTK_WIDGET(gtk_builder_get_object(_builder, widget_name));
   if (pix == NULL) {
      cout << "textview == NULL with: " << widget_name << endl;
      return false;
   }
   gtk_image_set_from_icon_name(GTK_IMAGE(pix), value);

   return true;
}

void RGGtkBuilderWindow::setBusyCursor(bool flag)
{
   if (flag) {
      if (gtk_widget_get_visible(_win))
         gtk_widget_set_cursor_from_name(window(), "watch");
   } else {
      if (gtk_widget_get_visible(_win))
         gtk_widget_set_cursor(window(), NULL);
   }
}
