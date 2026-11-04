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
#include <format>
#include <gtk/gtk.h>
#include <string>
#include <vector>

using namespace std;

// If there is an error locating the resource, then the program will be aborted.
GtkWidget *RGGtkBuilderWindow::getWindow(GtkBuilder *builder, const std::string_view name)
{
   std::string windowName = std::format("window_{}", name);
   GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, windowName.c_str()));
   assert(window != nullptr);
   return window;
}

// If there is an error locating the resource or parsing the description, then the program will be aborted.
GtkBuilder *RGGtkBuilderWindow::getBuilderFromResource(const std::string_view name)
{
   std::string resource_path = std::format("/io/github/mvo5/synaptic/ui/window_{}.ui", name);
   return gtk_builder_new_from_resource(resource_path.c_str());
}

// The corresponding resource (file) is name window_{name}..ui, and the respective window definition
// in the resource is names window_{name}.

RGGtkBuilderWindow::RGGtkBuilderWindow(RGWindow *parent, const string &name)
   : _builder(getBuilderFromResource(name)),
     _busyCursor(gdk_cursor_new_for_display(gdk_display_get_default(), GDK_WATCH)),
     _win(getWindow(_builder, name)),
     onDeleteEventHandlerId(g_signal_connect(G_OBJECT(_win), "delete-event", G_CALLBACK(onDeleteEventCallback), this))
{
   assert(_busyCursor != nullptr);

   if (parent != NULL) {
      gtk_window_set_transient_for(GTK_WINDOW(_win), GTK_WINDOW(parent->window()));
   }

   gtk_window_set_position(GTK_WINDOW(_win), GTK_WIN_POS_CENTER_ON_PARENT);
   gtk_window_set_icon_name(GTK_WINDOW(_win), "synaptic");
}

void RGGtkBuilderWindow::setLabel(const char *widget_name, const char *value)
{
   GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(_builder, widget_name));
   if (widget == NULL) {
      g_error("widget == NULL with: %s", widget_name);
   }

   const gchar *utf8value = utf8(value);
   if (utf8value == nullptr) utf8value = _("N/A");
   gtk_label_set_label(GTK_LABEL(widget), utf8value);
}

void RGGtkBuilderWindow::setLabel(const char *widget_name, const long value)
{
   GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(_builder, widget_name));
   if (widget == NULL) {
      g_error("widget == NULL with: %s", widget_name);
   }

   // TRANSLATORS: "N/A" is an abbreviation for "not applicable" (on forms).
   // It happens when e.g. a package has no installed version (or no downloadable version).
   string strVal = (value <= 0) ? _("N/A") : SizeToStr(value);
   gtk_label_set_label(GTK_LABEL(widget), utf8(strVal.c_str()));
}

void RGGtkBuilderWindow::setMarkup(const char *widget_name, const char *value)
{
   GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(_builder, widget_name));
   if (widget == NULL) {
      g_error("widget == NULL with: %s", widget_name);
   }

   const gchar *utf8value = utf8(value);
   if (utf8value == nullptr) utf8value = _("N/A");
   gtk_label_set_markup(GTK_LABEL(widget), utf8value);
}

void RGGtkBuilderWindow::setTreeList(const char *widget_name, const vector<string> &values, bool use_markup)
{
   GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(_builder, widget_name));
   if (widget == NULL) {
      g_error("widget == NULL with: %s", widget_name);
   }

   // create column (if needed)
   if (gtk_tree_view_get_column(GTK_TREE_VIEW(widget), 0) == NULL) {
      GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
      const char *type = use_markup ? "markup" : "text";
      GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("SubView", renderer, type, 0, NULL);
      gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);
   }

   GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
   for (const string &value : values) {
      gtk_list_store_insert_with_values(store, nullptr, -1, 0, utf8(value.c_str()), -1);
   }
   gtk_tree_view_set_model(GTK_TREE_VIEW(widget), GTK_TREE_MODEL(store));
   g_object_unref(store); // Unreference the list store as it is now managed by the treeview.
}

void RGGtkBuilderWindow::setTextView(const char *widget_name, const char *value, bool use_headline)
{
   GtkTextView *view = GTK_TEXT_VIEW(gtk_builder_get_object(_builder, widget_name));
   if (view == NULL) {
      g_error("textview == NULL with: %s", widget_name);
   }

   GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
   const gchar *utf8value = utf8(value);
   if (utf8value == nullptr) utf8value = _("N/A");
   gtk_text_buffer_set_text(buffer, utf8value, -1);

   GtkTextIter start, end;

   if (use_headline) {
      GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);
      if (gtk_text_tag_table_lookup(tag_table, "bold") == NULL) {
         gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, "scale", 1.1, NULL);
      }
      gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);
      gtk_text_buffer_get_iter_at_offset(buffer, &end, 0);
      gtk_text_iter_forward_line(&end);
      gtk_text_buffer_apply_tag_by_name(buffer, "bold", &start, &end);
   }

   gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);
   gtk_text_view_scroll_to_iter(view, &start, 0, FALSE, 0, 0);
}

void RGGtkBuilderWindow::setPixmap(const char *widget_name, GdkPixbuf *value)
{
   GtkWidget *pix = GTK_WIDGET(gtk_builder_get_object(_builder, widget_name));
   if (pix == NULL) {
      g_error("textview == NULL with: %s", widget_name);
   }

   gtk_image_set_from_pixbuf(GTK_IMAGE(pix), value);
}

void RGGtkBuilderWindow::setBusyCursor(bool isBusy)
{
   gdk_window_set_cursor(gtk_widget_get_window(window()), (isBusy) ? _busyCursor : NULL);
}
