
#include <algorithm>
#include <vector>
#include <utility>

#include <apt-pkg/configuration.h>

#include "rgpkgtreeview.h"
#include "rgutils.h"

#include "i18n.h"

// needed for the buildTreeView function
struct mysort {
   bool operator() (const std::pair<int, GtkTreeViewColumn *> &x,
                    const std::pair<int, GtkTreeViewColumn *> &y) {
      return x.first < y.first;
}};

void setupTreeView(GtkWidget *treeview)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column, *name_column = NULL;
  GtkTreeSelection *selection;
  std::vector<std::pair<int, GtkTreeViewColumn *> > all_columns;
  int pos = 0;
  bool visible;
 
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview),TRUE);

   gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview), NAME_COLUMN);
   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
   gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

   /* Status(pixmap) column */
   pos = _config->FindI("Synaptic::statusColumnPos", 0);
   visible = _config->FindI("Synaptic::statusColumnVisible", true);
   if(visible) {
      renderer = gtk_cell_renderer_pixbuf_new();
      // TRANSLATORS: Column header for the column "Status" in the package list
      column = gtk_tree_view_column_new_with_attributes(_("S"), renderer,
                                                        "pixbuf",
                                                        PIXMAP_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 20);
      //gtk_tree_view_insert_column(GTK_TREE_VIEW(treeview), column, pos);
      gtk_tree_view_column_set_sort_column_id(column, COLOR_COLUMN);
      all_columns.push_back(std::pair<int, GtkTreeViewColumn *>(pos, column));
   }

   /* supported(pixmap) column */
   pos = _config->FindI("Synaptic::supportedColumnPos", 1);
   visible = _config->FindI("Synaptic::supportedColumnVisible", true);
   if(visible) {
      renderer = gtk_cell_renderer_pixbuf_new();
      column = gtk_tree_view_column_new_with_attributes(" ", renderer,
                                                        "pixbuf",
                                                        SUPPORTED_COLUMN, 
							NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 20);
      //gtk_tree_view_insert_column(GTK_TREE_VIEW(treeview), column, pos);
      gtk_tree_view_column_set_sort_column_id(column, SUPPORTED_COLUMN);
      all_columns.push_back(std::pair<int, GtkTreeViewColumn *>(pos, column));
   }


   /* Package name */
   pos = _config->FindI("Synaptic::nameColumnPos", 2);
   visible = _config->FindI("Synaptic::nameColumnVisible", true);
   if (visible) {
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      name_column = column =
         gtk_tree_view_column_new_with_attributes(_("Package"), renderer,
                                                  "markup", NAME_COLUMN,
                                                  //"text", NAME_COLUMN,
                                                  "background-gdk",
                                                  COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 200);

      //gtk_tree_view_insert_column(GTK_TREE_VIEW(treeview), column, pos);
      all_columns.push_back(std::pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_resizable(column, TRUE);
      gtk_tree_view_column_set_sort_column_id(column, NAME_COLUMN);
   }

   // section 
   pos = _config->FindI("Synaptic::sectionColumnPos", 2);
   visible = _config->FindI("Synaptic::sectionColumnVisible", false);
   if (visible) {
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      column =
         gtk_tree_view_column_new_with_attributes(_("Section"),
                                                  renderer, "text",
                                                  SECTION_COLUMN,
                                                  "background-gdk",
                                                  COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 130);
      //gtk_tree_view_insert_column (GTK_TREE_VIEW(treeview), column, pos);
      all_columns.push_back(std::pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_sort_column_id(column, SECTION_COLUMN);
      gtk_tree_view_column_set_resizable(column, TRUE);
   }

   // component
   pos = _config->FindI("Synaptic::componentColumnPos", 3);
   visible = _config->FindI("Synaptic::componentColumnVisible", false);
   if (visible) {
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      column =
         gtk_tree_view_column_new_with_attributes(_("Component"),
                                                  renderer, "text",
                                                  COMPONENT_COLUMN,
                                                  "background-gdk",
                                                  COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 130);
      //gtk_tree_view_insert_column (GTK_TREE_VIEW(treeview), column, pos);
      all_columns.push_back(std::pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_sort_column_id(column, COMPONENT_COLUMN);
      gtk_tree_view_column_set_resizable(column, TRUE);
   }


   /* Installed Version */
   pos = _config->FindI("Synaptic::instVerColumnPos", 4);
   visible = _config->FindI("Synaptic::instVerColumnVisible", true);
   if (visible) {
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      column =
         gtk_tree_view_column_new_with_attributes(_("Installed Version"),
                                                  renderer, "text",
                                                  INSTALLED_VERSION_COLUMN,
                                                  "background-gdk",
                                                  COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 130);
      //gtk_tree_view_insert_column (GTK_TREE_VIEW(treeview), column, pos);
      all_columns.push_back(std::pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_sort_column_id(column, INSTALLED_VERSION_COLUMN);
      gtk_tree_view_column_set_resizable(column, TRUE);
   }

   /* Available Version */
   pos = _config->FindI("Synaptic::availVerColumnPos", 5);
   visible = _config->FindI("Synaptic::availVerColumnVisible", true);
   if (visible) {
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      column =
         gtk_tree_view_column_new_with_attributes(_("Latest Version"),
                                                  renderer, "text",
                                                  AVAILABLE_VERSION_COLUMN,
                                                  "background-gdk",
                                                  COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 130);
      //gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), column, pos);
      all_columns.push_back(std::pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_sort_column_id(column, AVAILABLE_VERSION_COLUMN);
      gtk_tree_view_column_set_resizable(column, TRUE);
   }
   // installed size
   pos = _config->FindI("Synaptic::instSizeColumnPos", 6);
   visible = _config->FindI("Synaptic::instSizeColumnVisible", false);
   if (visible) {
      /* Installed size */
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      g_object_set(G_OBJECT(renderer), "xalign",1.0, "xpad", 10, NULL);
      column = gtk_tree_view_column_new_with_attributes(_("Size"), renderer,
                                                        "text",
							PKG_SIZE_COLUMN,
                                                        "background-gdk",
                                                        COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 80);
      //gtk_tree_view_insert_column (GTK_TREE_VIEW(treeview), column, pos);
      all_columns.push_back(std::pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_resizable(column, TRUE);
      gtk_tree_view_column_set_sort_column_id(column, PKG_SIZE_COLUMN);
   }

   pos = _config->FindI("Synaptic::downloadSizeColumnPos", 7);
   visible = _config->FindI("Synaptic::downloadSizeColumnVisible", false);
   if (visible) {
      /* Download size */
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      g_object_set(G_OBJECT(renderer), "xalign",1.0, "xpad", 10, NULL);
      column = gtk_tree_view_column_new_with_attributes(_("Download"), 
							renderer,"text",
                                                        PKG_DOWNLOAD_SIZE_COLUMN,
                                                        "background-gdk",
                                                        COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 80);
      //gtk_tree_view_insert_column (GTK_TREE_VIEW(treeview), column, pos);
      all_columns.push_back(std::pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_resizable(column, TRUE);
      gtk_tree_view_column_set_sort_column_id(column, PKG_DOWNLOAD_SIZE_COLUMN);
   }


   /* Description */
   pos = _config->FindI("Synaptic::descrColumnPos", 8);
   visible = _config->FindI("Synaptic::descrColumnVisible", true);
   if (visible) {
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
                                                        (renderer), 1);
      column =
         gtk_tree_view_column_new_with_attributes(_("Description"), renderer,
                                                  "text", DESCR_COLUMN,
                                                  "background-gdk",
                                                  COLOR_COLUMN, NULL);
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(column, 500);
      //gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), column, pos);
      all_columns.push_back(std::pair<int, GtkTreeViewColumn *>(pos, column));
      gtk_tree_view_column_set_resizable(column, TRUE);
   }
   // now sort and insert in order
   sort(all_columns.begin(), all_columns.end(), mysort());
   for (unsigned int i = 0; i < all_columns.size(); i++) {
      gtk_tree_view_append_column(GTK_TREE_VIEW(treeview),
                                  GTK_TREE_VIEW_COLUMN(all_columns[i].second));
   }
   // now set name column to expander column
   if (name_column)
      gtk_tree_view_set_expander_column(GTK_TREE_VIEW(treeview), name_column);

   g_object_set(G_OBJECT(treeview), 
		"fixed_height_mode", TRUE,
		NULL);

   // we need to set a empty model first so that gtklistview
   // can do its cleanup, if we do not do that, then the cleanup
   // code in gtktreeview gets confused and throws
   // Gtk-CRITICAL **: gtk_tree_view_unref_tree_helper: assertion `node != NULL' failed
   // at us, see LP: #38397 for more information
   gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), NULL);
   gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview), NAME_COLUMN);
}
