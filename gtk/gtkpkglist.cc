/* gtkpkglist.cc
 * Copyright (C) 2002,2003  Michael Vogt
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <algorithm>
#include <gtk/gtk.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/strutl.h>
#include <cassert>
#include "gtkpkglist.h"
#include "rgutils.h"
#include "rgpackagestatus.h"
#include "rpackagelister.h"

using namespace std;

// #define DEBUG_LIST
// #define DEBUG_LIST_FULL

static void gtk_pkg_list_init(GtkPkgList *pkg_tree);
static void gtk_pkg_list_class_init(GtkPkgListClass *klass);
static void gtk_pkg_list_tree_model_init(GtkTreeModelIface *iface);
static void gtk_pkg_list_sortable_init(GtkTreeSortableIface *iface);
static void gtk_pkg_list_finalize(GObject *object);
static GtkTreeModelFlags gtk_pkg_list_get_flags(GtkTreeModel *tree_model);
static gint gtk_pkg_list_get_n_columns(GtkTreeModel *tree_model);
static GType gtk_pkg_list_get_column_type(GtkTreeModel *tree_model,
                                          gint index);
static gboolean gtk_pkg_list_get_iter(GtkTreeModel *tree_model,
                                      GtkTreeIter *iter, GtkTreePath *path);
static GtkTreePath *gtk_pkg_list_get_path(GtkTreeModel *tree_model,
                                          GtkTreeIter *iter);
static void gtk_pkg_list_get_value(GtkTreeModel *tree_model,
                                   GtkTreeIter *iter,
                                   gint column, GValue * value);
static gboolean gtk_pkg_list_iter_next(GtkTreeModel *tree_model,
                                       GtkTreeIter *iter);
static gboolean gtk_pkg_list_iter_children(GtkTreeModel *tree_model,
                                           GtkTreeIter *iter,
                                           GtkTreeIter *parent);
static gboolean gtk_pkg_list_iter_has_child(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter);
static gint gtk_pkg_list_iter_n_children(GtkTreeModel *tree_model,
                                         GtkTreeIter *iter);
static gboolean gtk_pkg_list_iter_nth_child(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter,
                                            GtkTreeIter *parent, gint n);
static gboolean gtk_pkg_list_iter_parent(GtkTreeModel *tree_model,
                                         GtkTreeIter *iter,
                                         GtkTreeIter *child);

/* sortable */
static void gtk_pkg_list_sort(GtkPkgList *pkg_list);
static gboolean gtk_pkg_list_get_sort_column_id(GtkTreeSortable *sortable,
                                                gint *sort_column_id,
                                                GtkSortType *order);
static void gtk_pkg_list_set_sort_column_id(GtkTreeSortable *sortable,
                                            gint sort_column_id,
                                            GtkSortType order);
static gboolean gtk_pkg_list_has_default_sort_func(GtkTreeSortable *sortable);


static GObjectClass *parent_class = NULL;

extern GdkPixbuf *StatusPixbuf[12];
extern GdkRGBA *StatusColors[12];

void RCacheActorPkgList::run(vector<RPackage *> &List, int Action)
{
   static GtkTreeIter iter;
   int j;

   for (unsigned int i = 0; i < List.size(); i++) {
      RPackage *pkg = List[i];

      j = _lister->getViewPackageIndex(pkg);
      if (j == -1)
         continue;

      iter.user_data = pkg;
      iter.user_data2 = GINT_TO_POINTER(j);

      // fill in treepath
      GtkTreePath *path = gtk_tree_path_new();
      gtk_tree_path_append_index(path, j);
      gtk_tree_model_row_changed(GTK_TREE_MODEL(_pkgList), path, &iter);
      gtk_tree_path_free(path);
   }

}

void RPackageListActorPkgList::run(vector<RPackage *> &List, int listEvent)
{
   cout <<
      "RPackageListActorPkgList::run(vector<RPackage*> &List, int pkgEvent)" <<
      endl;

   cout << "action: " << listEvent << endl;
   cout << "listsize: " << List.size() << endl;

   //_pkgList = gtk_pkg_list_new(_lister);
   //gtk_tree_view_set_model(GTK_TREE_VIEW(_pkgView),
   //                        GTK_TREE_MODEL(_pkgList));
   //return;

   //mvo: FIXME those nested loops suck as they are slow
   if (listEvent == PKG_REMOVED) {
      for (int i = (int)List.size() - 1; i >= 0; i--) {
         int j;
         for (j = 0; j < (int)_lastDisplayList.size(); j++)
            if (_lastDisplayList[j] == List[i])
               break;
         cout << "Removed at pos: " << j << endl;
         GtkTreePath *path = gtk_tree_path_new();
         gtk_tree_path_append_index(path, j);
         gtk_tree_model_row_deleted(GTK_TREE_MODEL(_pkgList), path);
         gtk_tree_path_free(path);
      }
   } else if (listEvent == PKG_ADDED) {
      static GtkTreeIter iter;
      for (int i = 0; i < (int)List.size(); i++) {
         int j = _lister->getViewPackageIndex(List[i]);
         cout << "Inserted " << List[i]->name() << " at pos: " << j << endl;
         GtkTreePath *path = gtk_tree_path_new();
         gtk_tree_path_append_index(path, j);
         iter.user_data = List[i];
         iter.user_data2 = GINT_TO_POINTER(j);
         iter.stamp = 140677;
         gtk_tree_model_row_inserted(GTK_TREE_MODEL(_pkgList), path, &iter);
         gtk_tree_path_free(path);
      }
   }
}

GType gtk_pkg_list_get_type(void)
{
   static GType pkg_list_type = 0;

   if (!pkg_list_type) {
      static const GTypeInfo pkg_list_info = {
         sizeof(GtkPkgListClass),
         NULL,                  /* base_init */
         NULL,                  /* base_finalize */
         (GClassInitFunc) gtk_pkg_list_class_init,
         NULL,                  /* class finalize */
         NULL,                  /* class_data */
         sizeof(GtkPkgList),
         0,                     /* n_preallocs */
         (GInstanceInitFunc) gtk_pkg_list_init
      };

      static const GInterfaceInfo tree_model_info = {
         (GInterfaceInitFunc) gtk_pkg_list_tree_model_init,
         NULL,
         NULL
      };

      static const GInterfaceInfo sortable_info = {
         (GInterfaceInitFunc) gtk_pkg_list_sortable_init,
         NULL,
         NULL
      };

      pkg_list_type = g_type_register_static(G_TYPE_OBJECT, "GtkPkgList",
                                             &pkg_list_info, (GTypeFlags) 0);

      g_type_add_interface_static(pkg_list_type,
                                  GTK_TYPE_TREE_MODEL, &tree_model_info);
      g_type_add_interface_static(pkg_list_type,
                                  GTK_TYPE_TREE_SORTABLE, &sortable_info);
   }

   return pkg_list_type;
}

static void gtk_pkg_list_class_init(GtkPkgListClass *klass)
{
   GObjectClass *object_class;

   parent_class = (GObjectClass *) g_type_class_peek_parent(klass);
   object_class = (GObjectClass *) klass;

   object_class->finalize = gtk_pkg_list_finalize;
}

static void gtk_pkg_list_tree_model_init(GtkTreeModelIface *iface)
{
   iface->get_flags = gtk_pkg_list_get_flags;
   iface->get_n_columns = gtk_pkg_list_get_n_columns;
   iface->get_column_type = gtk_pkg_list_get_column_type;
   iface->get_iter = gtk_pkg_list_get_iter;
   iface->get_path = gtk_pkg_list_get_path;
   iface->get_value = gtk_pkg_list_get_value;
   iface->iter_next = gtk_pkg_list_iter_next;
   iface->iter_children = gtk_pkg_list_iter_children;
   iface->iter_has_child = gtk_pkg_list_iter_has_child;
   iface->iter_n_children = gtk_pkg_list_iter_n_children;
   iface->iter_nth_child = gtk_pkg_list_iter_nth_child;
   iface->iter_parent = gtk_pkg_list_iter_parent;
}

void gtk_pkg_list_sortable_init(GtkTreeSortableIface *iface)
{
   iface->get_sort_column_id = gtk_pkg_list_get_sort_column_id;
   iface->set_sort_column_id = gtk_pkg_list_set_sort_column_id;
   iface->set_sort_func = NULL;
   iface->set_default_sort_func = NULL;
   iface->has_default_sort_func = gtk_pkg_list_has_default_sort_func;
}

static void gtk_pkg_list_init(GtkPkgList *pkg_list)
{
   //cout << "list_init()" << endl;
   pkg_list->n_columns = N_COLUMNS;
   pkg_list->column_headers[0] = GDK_TYPE_PIXBUF;
   pkg_list->column_headers[1] = GDK_TYPE_PIXBUF;
   pkg_list->column_headers[2] = G_TYPE_STRING;
   pkg_list->column_headers[3] = G_TYPE_STRING;
   pkg_list->column_headers[4] = G_TYPE_STRING;
   pkg_list->column_headers[5] = G_TYPE_STRING;
   pkg_list->column_headers[6] = G_TYPE_STRING;
   pkg_list->column_headers[7] = G_TYPE_STRING;
   pkg_list->column_headers[8] = G_TYPE_STRING;
   pkg_list->column_headers[9] = G_TYPE_STRING;
   pkg_list->column_headers[10] = GDK_TYPE_RGBA;
   pkg_list->column_headers[11] = G_TYPE_POINTER;
}

/**
 * gtk_pkg_list_new()
 **/
GtkPkgList *gtk_pkg_list_new(RPackageLister *lister)
{
   GtkPkgList *retval = NULL;

   //cout << "pkg_list_new()\n";

   retval = (GtkPkgList *) g_object_new(GTK_TYPE_PKG_LIST, NULL);
   assert(retval);
   retval->_lister = lister;

   return retval;
}


static void gtk_pkg_list_finalize(GObject *object)
{
   //  GtkPkgList *pkg_list = GTK_PKG_LIST (object);


   /* give back all memory */


   /* must chain up */
   (*parent_class->finalize) (object);
}


static GtkTreeModelFlags gtk_pkg_list_get_flags(GtkTreeModel *tree_model)
{
   g_return_val_if_fail(GTK_IS_PKG_LIST(tree_model), (GtkTreeModelFlags) 0);

   return GTK_TREE_MODEL_LIST_ONLY;
}


static gint gtk_pkg_list_get_n_columns(GtkTreeModel *tree_model)
{
   GtkPkgList *pkg_list = (GtkPkgList *) tree_model;

   g_return_val_if_fail(GTK_IS_PKG_LIST(tree_model), 0);

   return pkg_list->n_columns;
}


static GType
gtk_pkg_list_get_column_type(GtkTreeModel *tree_model, gint index)
{
   GtkPkgList *tree_store = (GtkPkgList *) tree_model;

   g_return_val_if_fail(GTK_IS_PKG_LIST(tree_model), G_TYPE_INVALID);
   g_return_val_if_fail(index < GTK_PKG_LIST(tree_model)->n_columns &&
                        index >= 0, G_TYPE_INVALID);

   return tree_store->column_headers[index];
}


static gboolean
gtk_pkg_list_get_iter(GtkTreeModel *tree_model,
                      GtkTreeIter *iter, GtkTreePath *path)
{   
   g_return_val_if_fail(GTK_IS_PKG_LIST(tree_model), FALSE);

   GtkPkgList *pkg_list = (GtkPkgList *) tree_model;
   gint *indices;
   gint depth;
   RPackage *pkg = NULL;

   assert(GTK_IS_PKG_LIST(pkg_list));
   assert(path != NULL);

   indices = gtk_tree_path_get_indices(path);
   depth = gtk_tree_path_get_depth(path);

   // we do not support more 
   assert(depth <= 1);

   int element = indices[0];

#ifdef DEBUG_LIST
   cout << "get_iter: index " << element << "  path: " << path << endl;
#endif

   if (element >= pkg_list->_lister->viewPackagesSize()) {
#ifdef DEBUG_LIST
      cout << "indices[0] > pkg_list->_lister->count()" << endl;
      cout << indices[0] << " >= " << pkg_list->_lister->viewPackagesSize() << endl;
#endif
      //gtk_tree_model_row_deleted(GTK_TREE_MODEL(pkg_list),path);
      return FALSE;
   }

   pkg = pkg_list->_lister->getViewPackage(element);
   assert(pkg);
   iter->stamp = 140677;
   iter->user_data = pkg;
   iter->user_data2 = GINT_TO_POINTER(element);
   return TRUE;
}

static GtkTreePath *gtk_pkg_list_get_path(GtkTreeModel *tree_model,
                                          GtkTreeIter *iter)
{
   GtkTreePath *retval;
   //GtkPkgList *pkg_list = GTK_PKG_LIST(tree_model);

   g_return_val_if_fail(GTK_IS_PKG_LIST(tree_model), NULL);
   g_return_val_if_fail(iter != NULL, NULL);
   //g_return_val_if_fail(iter->user_data != NULL, NULL);

   retval = gtk_tree_path_new();
   int i = GPOINTER_TO_INT(iter->user_data2);
   gtk_tree_path_append_index(retval, i);

#ifdef DEBUG_LIST
   RPackage *pkg = (RPackage *) iter->user_data;

   cout << "get_path() ";
   if (pkg != NULL)
      cout << ": " << pkg->name() << endl;
   else
      cout << "(pkg == NULL) " << endl;

   cout << "complete path: " << gtk_tree_path_to_string(retval) << endl;
#endif

   return retval;
}

static void
gtk_pkg_list_get_value(GtkTreeModel *tree_model,
                       GtkTreeIter *iter, gint column, GValue * value)
{
   g_return_if_fail(GTK_IS_PKG_LIST(tree_model));
   g_return_if_fail(iter != NULL);
   g_return_if_fail(column < GTK_PKG_LIST(tree_model)->n_columns);

   g_value_init(value, GTK_PKG_LIST(tree_model)->column_headers[column]);

   RPackage *pkg = (RPackage *) iter->user_data;

#ifdef DEBUG_LIST_FULL
   if (column == NAME_COLUMN) {
      cout << "get_value: column: " << column;
      if (pkg != NULL)
         cout << " name: " << pkg->name() << endl;
   }
#endif

   GtkPkgList *pkg_list = GTK_PKG_LIST(tree_model);
   int element = GPOINTER_TO_INT(iter->user_data2);
   if (element >= pkg_list->_lister->viewPackagesSize())
      return;

   if (pkg == NULL) {
      cout << "get_value: pkg == NULL" << endl;
      return;
   }

   const gchar *str;
   switch (column) {
      case NAME_COLUMN:
         str = utf8(pkg->name());
         g_value_set_string(value, str);
         break;
      case PKG_SIZE_COLUMN:
         if (pkg->installedVersion()) {
            str = SizeToStr(pkg->installedSize()).c_str();
            g_value_set_string(value, str);
         }
         break;
      case PKG_DOWNLOAD_SIZE_COLUMN:
	 str = SizeToStr(pkg->availablePackageSize()).c_str();
	 g_value_set_string(value, str);
         break;
      case SECTION_COLUMN:
	 str = pkg->section();
	 if(str != NULL)
	    g_value_set_string(value, str);
	 break;
      case COMPONENT_COLUMN:
	 str = pkg->component().c_str();
	 if(str)
	    g_value_set_string(value, str);
	 break;
      case INSTALLED_VERSION_COLUMN:
         str = pkg->installedVersion();
         g_value_set_string(value, str);
         break;
      case AVAILABLE_VERSION_COLUMN:
         str = pkg->availableVersion();
         g_value_set_string(value, str);
         break;
      case DESCR_COLUMN:
         str = utf8(pkg->summary());
         g_value_set_string(value, str);
         break;
      case PKG_COLUMN:
         g_value_set_pointer(value, pkg);
         break;
      case COLOR_COLUMN:
       {
	  if(_config->FindB("Synaptic::UseStatusColors", TRUE) == FALSE) 
	     return;
          GdkRGBA *bg;
          bg = RGPackageStatus::pkgStatus.getBgColor(pkg);
          g_value_set_boxed(value, bg);
          break;
       }
      case SUPPORTED_COLUMN:
       {
          if (pkg == NULL)
             return;
          GdkPixbuf *pix;
          pix = RGPackageStatus::pkgStatus.getSupportedPix(pkg);
          g_value_set_object(value, pix);
          break;
       }
      case PIXMAP_COLUMN:
       {
          GdkPixbuf *pix;
          pix = RGPackageStatus::pkgStatus.getPixbuf(pkg);
          g_value_set_object(value, pix);
          break;
       }
   }
}

static gboolean
gtk_pkg_list_iter_next(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
   g_return_val_if_fail(GTK_IS_PKG_LIST(tree_model), FALSE);

   if (iter == NULL)
      return FALSE;
   if (iter->user_data == NULL)
      return FALSE;

   GtkPkgList *pkg_list = GTK_PKG_LIST(tree_model);
   int i, old;

   old = GPOINTER_TO_INT(iter->user_data2);
   i = old + 1;
   if (i >= pkg_list->_lister->viewPackagesSize()) {
      GtkTreePath *path = gtk_tree_path_new_from_indices(i, -1);
      gtk_tree_model_row_deleted(tree_model, path);
      return FALSE;
   }

   RPackage *pkg = pkg_list->_lister->getViewPackage(i);

#ifdef DEBUG_LIST_FULL
   RPackage *oldpkg = (RPackage *) iter->user_data;
   cout << "iter_next()  " << endl;
   cout << "old: " << oldpkg->name() << " [" << old << "] " << endl;
   cout << "new: " << pkg->name() << " [" << i << "] " << endl;
   cout << "viewPackagesSize: " << pkg_list->_lister->viewPackagesSize() << endl;
#endif

   iter->stamp = 140677;
   iter->user_data = pkg;
   iter->user_data2 = GINT_TO_POINTER(i);
   return TRUE;
}

static gboolean
gtk_pkg_list_iter_children(GtkTreeModel *tree_model,
                           GtkTreeIter *iter, GtkTreeIter *parent)
{
   g_return_val_if_fail(parent == NULL || parent->user_data != NULL, FALSE);
   g_return_val_if_fail(GTK_IS_PKG_LIST(tree_model), FALSE);

#ifdef DEBUG_LIST
   cout << "iter_children: " << endl;
#endif

   // this is a list, nodes have no children 
   if (parent)
      return FALSE;

   GtkPkgList *pkg_list = (GtkPkgList *) tree_model;

   // should never happen, but does apparently with atk turned on
   if (pkg_list->_lister->getViewPackages().size() == 0)
      return FALSE;

   RPackage *pkg = pkg_list->_lister->getViewPackage(0);

   iter->stamp = 140677;
   iter->user_data = pkg;
   iter->user_data2 = 0;
   return TRUE;
}

static gboolean
gtk_pkg_list_iter_has_child(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
   return FALSE;
}

static gint
gtk_pkg_list_iter_n_children(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
   g_return_val_if_fail(GTK_IS_PKG_LIST(tree_model), -1);
   g_return_val_if_fail(iter == NULL || iter->user_data != NULL, FALSE);

   GtkPkgList *pkg_list = GTK_PKG_LIST(tree_model);

#ifdef DEBUG_LIST
   cout << "iter_n_children" << endl;
#endif

   if (iter == NULL)
      return pkg_list->_lister->viewPackagesSize();

   return 0;
}

static gboolean
gtk_pkg_list_iter_nth_child(GtkTreeModel *tree_model,
                            GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
   g_return_val_if_fail(GTK_IS_PKG_LIST(tree_model), FALSE);
   GtkPkgList *pkg_list = (GtkPkgList *) tree_model;

   if (parent) {
      cout << "parent != NULL" << endl;
      return FALSE;
   }

   if (n >= (gint) pkg_list->_lister->viewPackagesSize())
      return FALSE;

   RPackage *pkg = pkg_list->_lister->getViewPackage(n);
   assert(pkg);

#ifdef DEBUG_LIST
   cout << "iter_nth_child(): " << n << " is: " << pkg->name() << endl;
#endif

   iter->stamp = 140677;
   iter->user_data = pkg;
   iter->user_data2 = GINT_TO_POINTER(n);
   return TRUE;
}

static gboolean
gtk_pkg_list_iter_parent(GtkTreeModel *tree_model,
                         GtkTreeIter *iter, GtkTreeIter *child)
{
   return FALSE;
}



// sortable
static gboolean gtk_pkg_list_get_sort_column_id(GtkTreeSortable *sortable,
                                                gint *sort_column_id,
                                                GtkSortType *order)
{
   GtkPkgList *pkg_list = (GtkPkgList *) sortable;
   g_return_val_if_fail(GTK_IS_PKG_LIST(sortable), FALSE);

   //cout << " gtk_pkg_list_get_sort_column_id()" << endl;

   if (sort_column_id)
      *sort_column_id = pkg_list->sort_column_id;
   if (order)
      *order = pkg_list->order;
   return TRUE;
}


static void gtk_pkg_list_set_sort_column_id(GtkTreeSortable *sortable,
                                            gint sort_column_id,
                                            GtkSortType order)
{
   GtkPkgList *pkg_list = (GtkPkgList *) sortable;
   g_return_if_fail(GTK_IS_PKG_LIST(sortable));

   //cout << "gtk_pkg_list_set_sort_column_id(): " << sort_column_id << endl;

   if ((pkg_list->sort_column_id == sort_column_id) &&
       (pkg_list->order == order))
      return;

   pkg_list->sort_column_id = sort_column_id;
   pkg_list->order = order;

   gtk_tree_sortable_sort_column_changed(sortable);
   gtk_pkg_list_sort(pkg_list);
}

static gboolean gtk_pkg_list_has_default_sort_func(GtkTreeSortable *sortable)
{
   g_return_val_if_fail(GTK_IS_PKG_LIST(sortable), FALSE);
   return TRUE;
}

static void gtk_pkg_list_sort(GtkPkgList *pkg_list)
{
   //cout << "gtk_pkg_list_sort(GtkPkgList *pkg_list)" << endl;
   g_return_if_fail(GTK_IS_PKG_LIST(pkg_list));

   switch (pkg_list->sort_column_id) {
   case COLOR_COLUMN:
      if(pkg_list->order)
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_STATUS_ASC);
      else
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_STATUS_DES);
      break;
   case SUPPORTED_COLUMN:
      if(pkg_list->order)
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_SUPPORTED_ASC);
      else
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_SUPPORTED_DES);
      break;
   case NAME_COLUMN:
      if(pkg_list->order)
     pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_NAME_DES);
      else
     pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_NAME_ASC);
      break;
   case PKG_SIZE_COLUMN:
      if(pkg_list->order)
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_SIZE_DES);
      else
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_SIZE_ASC);
      break;
   case COMPONENT_COLUMN:
      if(pkg_list->order)
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_COMPONENT_DES);
      else
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_COMPONENT_ASC);
      break;
   case SECTION_COLUMN:
      if(pkg_list->order)
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_SECTION_DES);
      else
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_SECTION_ASC);
      break;
   case PKG_DOWNLOAD_SIZE_COLUMN:
      if(pkg_list->order)
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_DLSIZE_DES);
      else
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_DLSIZE_ASC);
      break;
   case AVAILABLE_VERSION_COLUMN:
      if(pkg_list->order)
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_VERSION_ASC);
      else
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_VERSION_DES);
      break;
   case INSTALLED_VERSION_COLUMN:
      if(pkg_list->order)
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_INST_VERSION_ASC);
      else
	 pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_INST_VERSION_DES);
      break;
   default:
      //cerr << "unknown sort column: " << pkg_list->sort_column_id << endl;
      pkg_list->_lister->sortPackages(RPackageLister::LIST_SORT_DEFAULT);
   }
}

// vim:ts=3:sw=3:et
