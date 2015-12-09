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
#include <apt-pkg/configuration.h>
#include <apt-pkg/fileutl.h>

#include <gdk/gdkx.h>

#include <cassert>
#include "config.h"
#include "i18n.h"
#include "rggtkbuilderwindow.h"


/*  the convention is that the top window is named:
   "window_$windowname" in your gtkbuilder-source 
*/
RGGtkBuilderWindow::RGGtkBuilderWindow(RGWindow *parent, string name, string mainName)
{
   _busyCursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_WATCH);
   _builder = gtk_builder_new ();

   // for development
   gchar *filename = NULL;
   gchar *local_filename = NULL;
   gchar *main_widget = NULL;
   GError* error = NULL;

   filename = g_strdup_printf("window_%s.ui", name.c_str());
   local_filename = g_strdup_printf("gtkbuilder/%s", filename);
   if (mainName.empty())
      main_widget = g_strdup_printf("window_%s", name.c_str());
   else
      main_widget = g_strdup_printf("window_%s", mainName.c_str());
   if (FileExists(local_filename)) {
      if (!gtk_builder_add_from_file (_builder, local_filename, &error)) {
         g_warning ("Couldn't load builder file: %s", error->message);
         g_error_free (error);
      }
   } else {
      g_free(filename);
      filename =
         g_strdup_printf(SYNAPTIC_GTKBUILDERDIR "window_%s.ui", name.c_str());
      if (!gtk_builder_add_from_file (_builder, filename, &error)) {
         g_warning ("Couldn't load builder file: %s", error->message);
         g_error_free (error);
      }
   }
   _win = GTK_WIDGET (gtk_builder_get_object (_builder, main_widget));
   assert(_win);

   if(parent != NULL) 
      gtk_window_set_transient_for(GTK_WINDOW(_win), 
				   GTK_WINDOW(parent->window()));

   gtk_window_set_position(GTK_WINDOW(_win),
			   GTK_WIN_POS_CENTER_ON_PARENT);
   GdkPixbuf *icon = get_gdk_pixbuf( "synaptic" );
   gtk_window_set_icon(GTK_WINDOW(_win), icon);

   g_free(filename);
   g_free(local_filename);
   g_free(main_widget);

   //gtk_window_set_title(GTK_WINDOW(_win), (char *)name.c_str());

   g_object_set_data(G_OBJECT(_win), "me", this);
   g_signal_connect(G_OBJECT(_win), "delete-event",
                    G_CALLBACK(windowCloseCallback), this);
   _topBox = NULL;

   // honor foreign parent windows (to make embedding easy)
   int id = _config->FindI("Volatile::ParentWindowId", -1);
   if (id > 0) {
      GdkWindow *win = gdk_x11_window_foreign_new_for_display(
         gdk_display_get_default(), id);
      if(win) {
	 gtk_widget_realize(_win);
	 gdk_window_set_transient_for(GDK_WINDOW(gtk_widget_get_window(_win)), win);
      }
   }
   // if we have no parent, don't skip the taskbar hint
   if(_config->FindB("Volatile::HideMainwindow",false) && id < 0)
   {
      gtk_window_set_skip_taskbar_hint(GTK_WINDOW(_win), FALSE);
      gtk_window_set_urgency_hint(GTK_WINDOW(_win), TRUE);
   }

}

bool RGGtkBuilderWindow::setLabel(const char *widget_name, const char *value)
{
   GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (_builder, widget_name));
   if (widget == NULL) {
      cout << "widget == NULL with: " << widget_name << endl;
      return false;
   }

   const gchar *utf8value=utf8(value);

   if (!utf8value)
      utf8value = _("N/A");
   gtk_label_set_label(GTK_LABEL(widget), utf8value);
   return true;
}

bool RGGtkBuilderWindow::setMarkup(const char *widget_name, const char *value)
{
   GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (_builder, widget_name));
   if (widget == NULL) {
      cout << "widget == NULL with: " << widget_name << endl;
      return false;
   }

   const gchar *utf8value=utf8(value);

   if (!utf8value)
      utf8value = _("N/A");
   gtk_label_set_markup(GTK_LABEL(widget), utf8value);
   return true;
}

bool RGGtkBuilderWindow::setLabel(const char *widget_name, const long value)
{
   string strVal;
   GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (_builder, widget_name));
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

bool RGGtkBuilderWindow::setTreeList(const char *widget_name, vector<string> values,
				bool use_markup)
{
   const char *type;
   string strVal;
   GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (_builder, widget_name));
   if (widget == NULL) {
      cout << "widget == NULL with: " << widget_name << endl;
      return false;
   }
   // create column (if needed)
   if(gtk_tree_view_get_column(GTK_TREE_VIEW(widget), 0) == NULL) {

      // cell renderer
      GtkCellRenderer *renderer;
      renderer = gtk_cell_renderer_text_new();

      if(use_markup)
	 type = "markup";
      else
	 type = "text";
      GtkTreeViewColumn *column;
      column = gtk_tree_view_column_new_with_attributes("SubView",
							renderer,
							type, 0, 
							NULL);
      gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);
   }

   // store stuff
   GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
   GtkTreeIter iter;
   for(unsigned int i=0;i<values.size();i++) {
      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter, 0, utf8(values[i].c_str()), -1);
   }
   gtk_tree_view_set_model(GTK_TREE_VIEW(widget), GTK_TREE_MODEL(store));
   return true;
}


bool RGGtkBuilderWindow::setTextView(const char *widget_name, 
				     const char* value, 
				     bool use_headline)
{
   GtkTextIter start,end;

   GtkWidget *view = GTK_WIDGET (gtk_builder_get_object (_builder, widget_name));
   if (view == NULL) {
      cout << "textview == NULL with: " << widget_name << endl;
      return false;
   }

   GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
   const gchar *utf8value = utf8(value);
   if(!utf8value)
      utf8value = _("N/A");
   gtk_text_buffer_set_text (buffer, utf8value, -1);

   if(use_headline) {
      GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);
      if(gtk_text_tag_table_lookup(tag_table, "bold") == NULL) {
	 gtk_text_buffer_create_tag(buffer, "bold", 
				    "weight", PANGO_WEIGHT_BOLD,
				    "scale", 1.1, 
				    NULL);
      }
      gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);
      
      gtk_text_buffer_get_iter_at_offset(buffer, &end, 0);
      gtk_text_iter_forward_line(&end);

      gtk_text_buffer_apply_tag_by_name(buffer, "bold", &start, &end);
   }

   gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);
   gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(view), &start,0,FALSE,0,0);
   return true;
}

bool RGGtkBuilderWindow::setPixmap(const char *widget_name, GdkPixbuf *value)
{
   GtkWidget *pix = GTK_WIDGET (gtk_builder_get_object (_builder, widget_name));
   if (pix == NULL) {
      cout << "textview == NULL with: " << widget_name << endl;
      return false;
   }
   gtk_image_set_from_pixbuf(GTK_IMAGE(pix), value);
   
   return true;
}

void RGGtkBuilderWindow::setBusyCursor(bool flag) 
{
   if(flag) {
      if(gtk_widget_get_visible(_win))
	 gdk_window_set_cursor(gtk_widget_get_window(window()), _busyCursor);
#if GTK_CHECK_VERSION(2,4,0)
      // if we don't iterate here, the busy cursor is not shown
      while (gtk_events_pending())
	 gtk_main_iteration();
#endif
   } else {
      if(gtk_widget_get_visible(_win))
	 gdk_window_set_cursor(gtk_widget_get_window(window()), NULL);
   }
}
