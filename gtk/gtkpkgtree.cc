/* gtkpkgtree.c
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
#include <iostream>
#include <gtk/gtk.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/strutl.h>
#include "i18n.h"
#include "gtkpkgtree.h"
#include "rgmisc.h"

using namespace std;

//#define DEBUG_SORT
//#define DEBUG_TREE
//#define DEBUG_TREE_FULL


#ifdef DEBUG_TREE
#define DEBUG(message) cout << message << endl;
#else
#define DEBUG(message)          /*nothing */
#endif

static void gtk_pkg_tree_init(GtkPkgTree *pkg_tree);
static void gtk_pkg_tree_class_init(GtkPkgTreeClass * klass);
static void gtk_pkg_tree_tree_model_init(GtkTreeModelIface *iface);
static void gtk_pkg_tree_sortable_init(GtkTreeSortableIface *iface);
static void gtk_pkg_tree_finalize(GObject *object);
static GtkTreeModelFlags gtk_pkg_tree_get_flags(GtkTreeModel *tree_model);
static gint gtk_pkg_tree_get_n_columns(GtkTreeModel *tree_model);
static GType gtk_pkg_tree_get_column_type(GtkTreeModel *tree_model,
                                          gint index);
static gboolean gtk_pkg_tree_get_iter(GtkTreeModel *tree_model,
                                      GtkTreeIter *iter, GtkTreePath *path);
static GtkTreePath *gtk_pkg_tree_get_path(GtkTreeModel *tree_model,
                                          GtkTreeIter *iter);
static void gtk_pkg_tree_get_value(GtkTreeModel *tree_model,
                                   GtkTreeIter *iter,
                                   gint column, GValue * value);
static gboolean gtk_pkg_tree_iter_next(GtkTreeModel *tree_model,
                                       GtkTreeIter *iter);
static gboolean gtk_pkg_tree_iter_children(GtkTreeModel *tree_model,
                                           GtkTreeIter *iter,
                                           GtkTreeIter *parent);
static gboolean gtk_pkg_tree_iter_has_child(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter);
static gint gtk_pkg_tree_iter_n_children(GtkTreeModel *tree_model,
                                         GtkTreeIter *iter);
static gboolean gtk_pkg_tree_iter_nth_child(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter,
                                            GtkTreeIter *parent, gint n);
static gboolean gtk_pkg_tree_iter_parent(GtkTreeModel *tree_model,
                                         GtkTreeIter *iter,
                                         GtkTreeIter *child);
/* sortable */
static void gtk_pkg_tree_sort(GtkPkgTree *pkg_tree);
static gboolean gtk_pkg_tree_get_sort_column_id(GtkTreeSortable *sortable,
                                                gint *sort_column_id,
                                                GtkSortType *order);
static void gtk_pkg_tree_set_sort_column_id(GtkTreeSortable *sortable,
                                            gint sort_column_id,
                                            GtkSortType order);
static gboolean gtk_pkg_tree_has_default_sort_func(GtkTreeSortable *sortable);



static GObjectClass *parent_class = NULL;

extern GdkPixbuf *StatusPixbuf[12];
extern GdkColor *StatusColors[12];


void RCacheActorPkgTree::run(vector<RPackage *> &List, int Action)
{
   DEBUG("RCacheActorPkgTree::run()");

   static GtkTreeIter iter;
   tree<RPackageLister::pkgPair> *pkgTree;
   RPackageLister::treeIter it;
   RPackageLister::pkgPair pair;

   for (unsigned int i = 0; i < List.size(); i++) {
      DEBUG("RCacheActorPkgTree::run()/loop");

      pair.first = List[i]->name();
      pair.second = List[i];
      pkgTree = _lister->getTreeOrganizer();

      //FIXME: we have room for optisation here
      it = find(pkgTree->begin(), pkgTree->end(), pair);
      // found
      if (it != pkgTree->end()) {
         // fill in iter
         iter.user_data = it.node;
         // fill in treepath
         GtkTreePath *path = gtk_tree_path_new();
         do {
            int j = pkgTree->index(it);
            gtk_tree_path_prepend_index(path, j);
            it = pkgTree->parent(it);
         } while (it.node != NULL);
         gtk_tree_model_row_changed(GTK_TREE_MODEL(_pkgTree), path, &iter);
         gtk_tree_path_free(path);
      } else {
         DEBUG("not found pkg: " << pair.first);
      }
   }
}

void RPackageListActorPkgTree::run(vector<RPackage *> &List, int pkgEvent)
{
   DEBUG("RPackageListActorPkgTree::run()");

   //FIXME: workaround for now, this should be more intelligent
   //       (see gtkpkglist.cc for a example)
   _pkgTree = gtk_pkg_tree_new(_lister);;
   gtk_tree_view_set_model(GTK_TREE_VIEW(_pkgView), GTK_TREE_MODEL(_pkgTree));
}



GType gtk_pkg_tree_get_type(void)
{
   static GType pkg_tree_type = 0;

   if (!pkg_tree_type) {
      static const GTypeInfo pkg_tree_info = {
         sizeof(GtkPkgTreeClass),
         NULL,                  /* base_init */
         NULL,                  /* base_finalize */
         (GClassInitFunc) gtk_pkg_tree_class_init,
         NULL,                  /* class finalize */
         NULL,                  /* class_data */
         sizeof(GtkPkgTree),
         0,                     /* n_preallocs */
         (GInstanceInitFunc) gtk_pkg_tree_init
      };

      static const GInterfaceInfo tree_model_info = {
         (GInterfaceInitFunc) gtk_pkg_tree_tree_model_init,
         NULL,
         NULL
      };

      static const GInterfaceInfo sortable_info = {
         (GInterfaceInitFunc) gtk_pkg_tree_sortable_init,
         NULL,
         NULL
      };

      pkg_tree_type = g_type_register_static(G_TYPE_OBJECT, "GtkPkgTree",
                                             &pkg_tree_info, (GTypeFlags) 0);

      g_type_add_interface_static(pkg_tree_type,
                                  GTK_TYPE_TREE_MODEL, &tree_model_info);
      g_type_add_interface_static(pkg_tree_type,
                                  GTK_TYPE_TREE_SORTABLE, &sortable_info);
   }

   return pkg_tree_type;
}

static void gtk_pkg_tree_class_init(GtkPkgTreeClass * klass)
{
   GObjectClass *object_class;

   parent_class = (GObjectClass *) g_type_class_peek_parent(klass);
   object_class = (GObjectClass *) klass;

   object_class->finalize = gtk_pkg_tree_finalize;
}

static void gtk_pkg_tree_tree_model_init(GtkTreeModelIface *iface)
{
   iface->get_flags = gtk_pkg_tree_get_flags;
   iface->get_n_columns = gtk_pkg_tree_get_n_columns;
   iface->get_column_type = gtk_pkg_tree_get_column_type;
   iface->get_iter = gtk_pkg_tree_get_iter;
   iface->get_path = gtk_pkg_tree_get_path;
   iface->get_value = gtk_pkg_tree_get_value;
   iface->iter_next = gtk_pkg_tree_iter_next;
   iface->iter_children = gtk_pkg_tree_iter_children;
   iface->iter_has_child = gtk_pkg_tree_iter_has_child;
   iface->iter_n_children = gtk_pkg_tree_iter_n_children;
   iface->iter_nth_child = gtk_pkg_tree_iter_nth_child;
   iface->iter_parent = gtk_pkg_tree_iter_parent;
}

void gtk_pkg_tree_sortable_init(GtkTreeSortableIface *iface)
{
   iface->get_sort_column_id = gtk_pkg_tree_get_sort_column_id;
   iface->set_sort_column_id = gtk_pkg_tree_set_sort_column_id;
   iface->set_sort_func = NULL;
   iface->set_default_sort_func = NULL;
   iface->has_default_sort_func = gtk_pkg_tree_has_default_sort_func;
}


static void gtk_pkg_tree_init(GtkPkgTree *pkg_tree)
{
   //cout << "tree_init()" << endl;
   pkg_tree->n_columns = N_COLUMNS;
   pkg_tree->column_headers[0] = GDK_TYPE_PIXBUF;
   pkg_tree->column_headers[1] = G_TYPE_STRING;
   pkg_tree->column_headers[2] = G_TYPE_STRING;
   pkg_tree->column_headers[3] = G_TYPE_STRING;
   pkg_tree->column_headers[4] = G_TYPE_STRING;
   pkg_tree->column_headers[5] = G_TYPE_STRING;
   pkg_tree->column_headers[6] = GDK_TYPE_COLOR;
   pkg_tree->column_headers[7] = G_TYPE_POINTER;


}

/**
 * gtk_pkg_tree_new()
 **/
GtkPkgTree *gtk_pkg_tree_new(RPackageLister *lister)
{
   GtkPkgTree *retval;

   DEBUG("pkg_tree_new()");
   retval = (GtkPkgTree *) g_object_new(GTK_TYPE_PKG_TREE, NULL);
   retval->_lister = lister;
   return retval;
}


static void gtk_pkg_tree_finalize(GObject *object)
{
   //  GtkPkgTree *pkg_tree = GTK_PKG_TREE (object);


   /* give back all memory */


   /* must chain up */
   (*parent_class->finalize) (object);
}


static GtkTreeModelFlags gtk_pkg_tree_get_flags(GtkTreeModel *tree_model)
{
   g_return_val_if_fail(GTK_IS_PKG_TREE(tree_model), (GtkTreeModelFlags) 0);

//   RPackageLister::treeDisplayMode mode = GTK_PKG_TREE(tree_model)->_lister->getTreeDisplayMode();

//   if(mode == RPackageLister::TREE_DISPLAY_FLAT)
//     return GTK_TREE_MODEL_LIST_ONLY|GTK_TREE_MODEL_ITERS_PERSIST);
//   else
   return GTK_TREE_MODEL_ITERS_PERSIST;
}


static gint gtk_pkg_tree_get_n_columns(GtkTreeModel *tree_model)
{
   GtkPkgTree *pkg_tree = (GtkPkgTree *) tree_model;

   g_return_val_if_fail(GTK_IS_PKG_TREE(tree_model), 0);

   return pkg_tree->n_columns;
}


static GType
gtk_pkg_tree_get_column_type(GtkTreeModel *tree_model, gint index)
{
   GtkPkgTree *tree_store = (GtkPkgTree *) tree_model;

   g_return_val_if_fail(GTK_IS_PKG_TREE(tree_model), G_TYPE_INVALID);
   g_return_val_if_fail(index < GTK_PKG_TREE(tree_model)->n_columns &&
                        index >= 0, G_TYPE_INVALID);

   return tree_store->column_headers[index];
}



static gboolean
gtk_pkg_tree_get_iter(GtkTreeModel *tree_model,
                      GtkTreeIter *iter, GtkTreePath *path)
{
   tree<RPackageLister::pkgPair> *pkgTree;

   GtkPkgTree *pkg_tree = (GtkPkgTree *) tree_model;
   gint *indices;
   gint depth;

   DEBUG("gtk_pkg_tree_get_iter()");

   g_return_val_if_fail(GTK_IS_PKG_TREE(pkg_tree), FALSE);

   indices = gtk_tree_path_get_indices(path);
   depth = gtk_tree_path_get_depth(path);

   DEBUG(" indices: " << indices[0] << ":" << indices[1]);
   DEBUG(" depth = " << depth);

   pkgTree = pkg_tree->_lister->getTreeOrganizer();

   //DEBUG(" size is: " << pkgTree->size());

   // check if we are working on a empty tree
   if (pkgTree->empty())
      return FALSE;

   RPackageLister::sibTreeIter sib(pkgTree->begin());
   RPackageLister::sibTreeIter end = pkgTree->end(sib);

   // jump to right top-level node
   for (int j = 0; j < indices[0]; j++) {
      sib = pkgTree->next_sibling(sib);
      if (sib == end) {
         DEBUG("out of bound for indices[1]" << indices[0]);
         return FALSE;
      }
   }

   // now go for the children 
   if (depth == 2) {
      if (pkgTree->number_of_children(sib) == 0) {
         DEBUG(" depth==2 && number_of_children(sib) == 0");
         return FALSE;
      }
      sib = pkgTree->child(sib, 0);
      end = pkgTree->end(sib);
      for (int j = 0; j < indices[1]; j++) {
         sib = pkgTree->next_sibling(sib);
         if (sib == end) {
            DEBUG("out of bound for indices[1]" << indices[1]);
            return FALSE;
         }
      }
   }


   DEBUG(" get_iter: " << (*sib).first << " depth: " << depth
         << " indices: " << indices[0] << ":" << indices[1]);

   iter->user_data = sib.node;
   return TRUE;
}

static GtkTreePath *gtk_pkg_tree_get_path(GtkTreeModel *tree_model,
                                          GtkTreeIter *iter)
{
   GtkTreePath *retval;
   gint i = 0;

   g_return_val_if_fail(GTK_IS_PKG_TREE(tree_model), NULL);
   g_return_val_if_fail(iter != NULL, NULL);
   g_return_val_if_fail(iter->user_data != NULL, NULL);

   tree<RPackageLister::pkgPair> *pkgTree;
   pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

   retval = gtk_tree_path_new();
   RPackageLister::sibTreeIter it((RPackageLister::treeNode *) iter->
                                  user_data);

   DEBUG("get_path(): " << (*it).first);

   do {
      i = pkgTree->index(it);
      DEBUG(" index: " << i);

      gtk_tree_path_prepend_index(retval, i);
      it = pkgTree->parent(it);
   } while (pkgTree->is_valid(it));

   DEBUG(" complete path: " << gtk_tree_path_to_string(retval));

   return retval;
}


static void
gtk_pkg_tree_get_value(GtkTreeModel *tree_model,
                       GtkTreeIter *iter, gint column, GValue * value)
{
   g_return_if_fail(GTK_IS_PKG_TREE(tree_model));
   g_return_if_fail(iter != NULL);
   g_return_if_fail(column < GTK_PKG_TREE(tree_model)->n_columns);

   g_value_init(value, GTK_PKG_TREE(tree_model)->column_headers[column]);

   tree<RPackageLister::pkgPair> *pkgTree;
   pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

   RPackageLister::treeIter it((RPackageLister::treeNode *) iter->user_data);
#ifdef DEBUG_TREE_FULL
   cout << "get_value() ";
#endif
   if (it == pkgTree->end() || pkgTree->empty())
      return;
   // this is a error, but we don't want to segfault here
   if (it == 0) {
      cerr << "Internal Error: gtk_pkg_tree_get_value() it == 0" << endl;
      return;
   }
#ifdef DEBUG_TREE_FULL
   cout << "column: " << column << " name: " << (*it).first << endl;
#endif

   const gchar *str;
   RPackage *pkg = (RPackage *) (*it).second;
   switch (column) {
      case NAME_COLUMN:
         str = utf8((*it).first.c_str());
         g_value_set_string(value, str);
         break;
      case PKG_SIZE_COLUMN:
         if (pkg == NULL)
            return;
         if (pkg->installedVersion()) {
            str = SizeToStr(pkg->installedSize()).c_str();
            g_value_set_string(value, str);
         }
         break;
      case INSTALLED_VERSION_COLUMN:
         if (pkg == NULL)
            return;
         str = pkg->installedVersion();
         g_value_set_string(value, str);
         break;
      case AVAILABLE_VERSION_COLUMN:
         if (pkg == NULL)
            return;
         str = pkg->availableVersion();
         g_value_set_string(value, str);
         break;
      case DESCR_COLUMN:
         if (pkg == NULL)
            return;
         str = utf8(pkg->summary());
         g_value_set_string(value, str);
         break;
      case PKG_COLUMN:
         if (pkg == NULL)
            return;
         g_value_set_pointer(value, (*it).second);
         break;
      case COLOR_COLUMN:
       {
          if (pkg == NULL)
             return;
          GdkColor *bg;
          bg = RPackageStatus::pkgStatus.getBgColor(pkg);
          g_value_set_boxed(value, bg);
          break;
       }
      case PIXMAP_COLUMN:
       {
          if (pkg == NULL)
             return;
          GdkPixbuf *pix;
          pix = RPackageStatus::pkgStatus.getPixbuf(pkg);
          g_value_set_object(value, pix);
          break;
       }
   }
}

static gboolean
gtk_pkg_tree_iter_next(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
   g_return_val_if_fail(iter != NULL, FALSE);

   if (iter->user_data == NULL) {
      DEBUG("iter_next() called with iter->user_data == NULL");
      return FALSE;
   }

   tree<RPackageLister::pkgPair> *pkgTree;
   pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

   RPackageLister::sibTreeIter it((RPackageLister::treeNode *) iter->
                                  user_data);
   RPackageLister::sibTreeIter end = pkgTree->end(it);

   DEBUG("iter_next()");
   DEBUG(" current node: " << (*it).first);

   it = pkgTree->next_sibling(it);

   if (it == end) {
      DEBUG(" it == end");
      return FALSE;
   } else {
      iter->user_data = it.node;
      DEBUG(" iter is now: " << (*it).first);
      return TRUE;
   }
}

static gboolean
gtk_pkg_tree_iter_children(GtkTreeModel *tree_model,
                           GtkTreeIter *iter, GtkTreeIter *parent)
{
   DEBUG("gtk_pkg_tree_iter_children()");

   tree<RPackageLister::pkgPair> *pkgTree;
   pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

   // FIXME: this is a  special case (see treemodel docs)
   if (parent == NULL)
      return FALSE;
   if (parent->user_data == NULL)
      return FALSE;

   RPackageLister::treeIter it((RPackageLister::treeNode *) parent->user_data);
   DEBUG(" parent: " << (*it).first);

   if (pkgTree->number_of_children(it) == 0)
      return FALSE;

   RPackageLister::treeIter it_child(it);

   /* go to first child */
   ++it_child;

   DEBUG(" child: " << (*it_child).first);

   iter->user_data = it_child.node;

   return TRUE;
}

static gboolean
gtk_pkg_tree_iter_has_child(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
   g_return_val_if_fail(GTK_IS_PKG_TREE(tree_model), FALSE);
   g_return_val_if_fail(iter->user_data != NULL, FALSE);

   tree<RPackageLister::pkgPair> *pkgTree;
   pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

   RPackageLister::treeIter it((RPackageLister::treeNode *) iter->user_data);

   DEBUG("iter_has_child()");
   DEBUG(" child " << (*it).first << " nr: "
         << pkgTree->number_of_children(it));

   if (pkgTree->number_of_children(it) > 0) {
      return TRUE;
   } else {
      return FALSE;
   }
}

static gint
gtk_pkg_tree_iter_n_children(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
   g_return_val_if_fail(GTK_IS_PKG_TREE(tree_model), 0);
   g_return_val_if_fail(iter == NULL || iter->user_data != NULL, FALSE);

   DEBUG("gtk_tree_iter_n_children()");

   tree<RPackageLister::pkgPair> *pkgTree;
   pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();
   RPackageLister::treeIter it((RPackageLister::treeNode *) iter->user_data);

   return pkgTree->number_of_children(it);
}

static gboolean
gtk_pkg_tree_iter_nth_child(GtkTreeModel *tree_model,
                            GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
   DEBUG("gtk_pkg_tree_iter_nth_child(): n=" << n);
   DEBUG(" parent = " << parent);

   g_return_val_if_fail(GTK_IS_PKG_TREE(tree_model), FALSE);

   tree<RPackageLister::pkgPair> *pkgTree;
   pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

   DEBUG(" tree-size: " << pkgTree->size());

   if (pkgTree->empty())
      return FALSE;

   // special case (yeah, the gtk_tree_model interface rocks :-/)
   if (parent == NULL) {
      RPackageLister::sibTreeIter it(pkgTree->begin());
      RPackageLister::sibTreeIter end = pkgTree->end(it);

      for (int i = 0; i < n - 1; i++) {
         ++it;
         if (it == end) {
            DEBUG(" nth_child() out of bounds");
            return FALSE;
         }
      }
      DEBUG(" toplevel iter" << n << " is " << it.node);
      iter->user_data = it.node;
      return TRUE;
   }
   // ill case
   if (parent->user_data == NULL)
      return FALSE;

   // normal case
   RPackageLister::treeIter it((RPackageLister::treeNode *) parent->user_data);

   // check if n is not out of reach
   int children = it.number_of_children() - 1;
   if (n > children) {
      DEBUG(" nth_child() out of bounds");
      return FALSE;
   }

   DEBUG(" parent name: " << (*it).first);
   DEBUG(" number of children: " << it.number_of_children());

   it = pkgTree->child(it, n);

   DEBUG(" nth-child is " << (*it).first);

   iter->user_data = it.node;

   return TRUE;
}

static gboolean
gtk_pkg_tree_iter_parent(GtkTreeModel *tree_model,
                         GtkTreeIter *iter, GtkTreeIter *child)
{
   g_return_val_if_fail(iter != NULL, FALSE);
   g_return_val_if_fail(child != NULL, FALSE);
   g_return_val_if_fail(child->user_data != NULL, FALSE);

   DEBUG("gtk_pkg_tree_iter_parent ()");

   tree<RPackageLister::pkgPair> *pkgTree;
   pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

   RPackageLister::treeIter it((RPackageLister::treeNode *) child->user_data);

   RPackageLister::treeIter it_parent = pkgTree->parent(it);

   iter->user_data = it_parent.node;

   return TRUE;

}

// sortable
static gboolean gtk_pkg_tree_get_sort_column_id(GtkTreeSortable *sortable,
                                                gint *sort_column_id,
                                                GtkSortType *order)
{
   GtkPkgTree *pkg_tree = (GtkPkgTree *) sortable;
   g_return_val_if_fail(GTK_IS_PKG_TREE(sortable), FALSE);
#ifdef DEBUG_SORT
   cout << " gtk_pkg_tree_get_sort_column_id()" << endl;
#endif
   if (sort_column_id)
      *sort_column_id = pkg_tree->sort_column_id;
   if (order)
      *order = pkg_tree->order;
   return TRUE;
}


static void gtk_pkg_tree_set_sort_column_id(GtkTreeSortable *sortable,
                                            gint sort_column_id,
                                            GtkSortType order)
{
   GtkPkgTree *pkg_tree = (GtkPkgTree *) sortable;
   g_return_if_fail(GTK_IS_PKG_TREE(sortable));
#ifdef DEBUG_SORT
   cout << "gtk_pkg_tree_set_sort_column_id(): " << sort_column_id << endl;
#endif
   if ((pkg_tree->sort_column_id == sort_column_id) &&
       (pkg_tree->order == order))
      return;

   pkg_tree->sort_column_id = sort_column_id;
   pkg_tree->order = order;

   gtk_tree_sortable_sort_column_changed(sortable);
   gtk_pkg_tree_sort(pkg_tree);
}

static gboolean gtk_pkg_tree_has_default_sort_func(GtkTreeSortable *sortable)
{
#ifdef DEBUG_SORT
   cout << "gtk_pkg_tree_has_default_sort_func ()" << endl;
#endif
   g_return_val_if_fail(GTK_IS_PKG_TREE(sortable), FALSE);
   return TRUE;
}

static void gtk_pkg_tree_sort(GtkPkgTree *pkg_tree)
{
#ifdef DEBUG_SORT
   cout << "gtk_pkg_tree_sort(): column_id: " << pkg_tree->sort_column_id
      << endl;
#endif

   switch (pkg_tree->sort_column_id) {
      case NAME_COLUMN:
         pkg_tree->_lister->sortPackagesByName();
         break;
      case PKG_SIZE_COLUMN:
         pkg_tree->_lister->sortPackagesByInstSize((int)pkg_tree->order);
         break;
         //default:
         //cerr << "unknown sort column" << endl;
   }


}
