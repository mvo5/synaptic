/* gtktagtree.c
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

#include "config.h"

#ifdef HAVE_DEBTAGS

#include <algorithm>
#include <iostream>
#include <gtk/gtk.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/strutl.h>
#include "gtktagtree.h"
#include "gsynaptic.h"


//#define DEBUG_TREE
//#define DEBUG_TREE_FULL

static void gtk_tag_tree_init(GtkTagTree * pkg_tree);
static void gtk_tag_tree_class_init(GtkTagTreeClass * klass);
static void gtk_tag_tree_tree_model_init(GtkTreeModelIface *iface);
static void gtk_tag_tree_finalize(GObject *object);
static GtkTreeModelFlags gtk_tag_tree_get_flags(GtkTreeModel *tree_model);
static gint gtk_tag_tree_get_n_columns(GtkTreeModel *tree_model);
static GType gtk_tag_tree_get_column_type(GtkTreeModel *tree_model,
                                          gint index);
static gboolean gtk_tag_tree_get_iter(GtkTreeModel *tree_model,
                                      GtkTreeIter *iter, GtkTreePath *path);
static GtkTreePath *gtk_tag_tree_get_path(GtkTreeModel *tree_model,
                                          GtkTreeIter *iter);
static void gtk_tag_tree_get_value(GtkTreeModel *tree_model,
                                   GtkTreeIter *iter,
                                   gint column, GValue * value);
static gboolean gtk_tag_tree_iter_next(GtkTreeModel *tree_model,
                                       GtkTreeIter *iter);
static gboolean gtk_tag_tree_iter_children(GtkTreeModel *tree_model,
                                           GtkTreeIter *iter,
                                           GtkTreeIter *parent);
static gboolean gtk_tag_tree_iter_has_child(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter);
static gint gtk_tag_tree_iter_n_children(GtkTreeModel *tree_model,
                                         GtkTreeIter *iter);
static gboolean gtk_tag_tree_iter_nth_child(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter,
                                            GtkTreeIter *parent, gint n);
static gboolean gtk_tag_tree_iter_parent(GtkTreeModel *tree_model,
                                         GtkTreeIter *iter,
                                         GtkTreeIter *child);

static GObjectClass *parent_class = NULL;

extern GdkPixbuf *StatusPixbuf[12];
extern GdkColor *StatusColors[12];



using namespace std;
GType gtk_tag_tree_get_type(void)
{
   static GType pkg_tree_type = 0;

   if (!pkg_tree_type) {
      static const GTypeInfo pkg_tree_info = {
         sizeof(GtkTagTreeClass),
         NULL,                  /* base_init */
         NULL,                  /* base_finalize */
         (GClassInitFunc) gtk_tag_tree_class_init,
         NULL,                  /* class finalize */
         NULL,                  /* class_data */
         sizeof(GtkTagTree),
         0,                     /* n_preallocs */
         (GInstanceInitFunc) gtk_tag_tree_init
      };

      static const GInterfaceInfo tree_model_info = {
         (GInterfaceInitFunc) gtk_tag_tree_tree_model_init,
         NULL,
         NULL
      };

      pkg_tree_type = g_type_register_static(G_TYPE_OBJECT, "GtkTagTree",
                                             &pkg_tree_info, (GTypeFlags) 0);

      g_type_add_interface_static(pkg_tree_type,
                                  GTK_TYPE_TREE_MODEL, &tree_model_info);
   }

   return pkg_tree_type;
}

static void gtk_tag_tree_class_init(GtkTagTreeClass * klass)
{
   GObjectClass *object_class;

   parent_class = (GObjectClass *) g_type_class_peek_parent(klass);
   object_class = (GObjectClass *) klass;

   object_class->finalize = gtk_tag_tree_finalize;
}

static void gtk_tag_tree_tree_model_init(GtkTreeModelIface *iface)
{
   iface->get_flags = gtk_tag_tree_get_flags;
   iface->get_n_columns = gtk_tag_tree_get_n_columns;
   iface->get_column_type = gtk_tag_tree_get_column_type;
   iface->get_iter = gtk_tag_tree_get_iter;
   iface->get_path = gtk_tag_tree_get_path;
   iface->get_value = gtk_tag_tree_get_value;
   iface->iter_next = gtk_tag_tree_iter_next;
   iface->iter_children = gtk_tag_tree_iter_children;
   iface->iter_has_child = gtk_tag_tree_iter_has_child;
   iface->iter_n_children = gtk_tag_tree_iter_n_children;
   iface->iter_nth_child = gtk_tag_tree_iter_nth_child;
   iface->iter_parent = gtk_tag_tree_iter_parent;
}


static void gtk_tag_tree_init(GtkTagTree * pkg_tree)
{
   //cout << "tag_init()" << endl;
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
 * gtk_tag_tree_new()
 **/
GtkTagTree *gtk_tag_tree_new(RPackageLister *lister,
                             HierarchyNode<int> *root,
                             HandleMaker<RPackage *> *hmaker)
{
   static bool expanded = false;
   GtkTagTree *retval;
#ifdef DEBUG_TREE
   g_print("pkg_tag_new()\n");
#endif
   retval = (GtkTagTree *) g_object_new(GTK_TYPE_TAG_TREE, NULL);

   //cout << "tree new" << endl;

   assert(lister);
   retval->_lister = lister;
   retval->hmaker = hmaker;
   retval->root = root;

   if (!expanded)
      root->expand();

   expanded = true;
#ifdef DEBUG_TREE
   cout << "expand done" << endl;
#endif
   return retval;
}


static void gtk_tag_tree_finalize(GObject *object)
{
   //GtkTagTree *pkg_tree = GTK_TAG_TREE (object);


   /* must chain up */
   (*parent_class->finalize) (object);
}


static GtkTreeModelFlags gtk_tag_tree_get_flags(GtkTreeModel *tree_model)
{
   g_return_val_if_fail(GTK_IS_TAG_TREE(tree_model), (GtkTreeModelFlags) 0);

//   RPackageLister::treeDisplayMode mode = GTK_TAG_TREE(tree_model)->_lister->getTreeDisplayMode();

//   if(mode == RPackageLister::TREE_DISPLAY_FLAT)
//     return GTK_TREE_MODEL_LIST_ONLY|GTK_TREE_MODEL_ITERS_PERSIST);
//   else
   return GTK_TREE_MODEL_ITERS_PERSIST;
}


static gint gtk_tag_tree_get_n_columns(GtkTreeModel *tree_model)
{
   GtkTagTree *pkg_tree = (GtkTagTree *) tree_model;

   g_return_val_if_fail(GTK_IS_TAG_TREE(tree_model), 0);

   return pkg_tree->n_columns;
}


static GType
gtk_tag_tree_get_column_type(GtkTreeModel *tree_model, gint index)
{
   GtkTagTree *tree_store = (GtkTagTree *) tree_model;

   g_return_val_if_fail(GTK_IS_TAG_TREE(tree_model), G_TYPE_INVALID);
   g_return_val_if_fail(index < GTK_TAG_TREE(tree_model)->n_columns &&
                        index >= 0, G_TYPE_INVALID);

   return tree_store->column_headers[index];
}

static string get_path(HierarchyNode<int> *cur)
{
   if (cur->parent()) {
      string res = get_path(cur->parent());
      return res + "/" + cur->tag();
   } else
      return cur->tag();
}

static gboolean
gtk_tag_tree_get_iter(GtkTreeModel *tree_model,
                      GtkTreeIter *iter, GtkTreePath *path)
{
#ifdef DEBUG_TREE
   cout << "gtk_tag_tree_get_iter()" << endl;
#endif


   GtkTagTree *pkg_tree = (GtkTagTree *) tree_model;
   g_return_val_if_fail(GTK_IS_TAG_TREE(pkg_tree), FALSE);

   gint *indices;
   gint depth;

   indices = gtk_tree_path_get_indices(path);
   depth = gtk_tree_path_get_depth(path);
#ifdef DEBUG_TREE
   for (int i = 0; i < depth; i++)
      if (i == 0)
         cout << indices[i];
      else
         cout << ":" << indices[i];
   cout << "\n";
#endif

   HierarchyNode<int> *cur = pkg_tree->root;
   gint *levl = indices;
#ifdef DEBUG_TREE
   cout << "cur is: " << cur << endl;
#endif

   // Go down the tree hierarchy, iterating thru nodes, finding the
   // parent node of the node pointed by path
   for (int i = 0; i < depth - 1; i++)
      cur = (*cur)[levl[i]];

   if (levl[depth - 1] < cur->size()) {
      // Iterate over child nodes
      iter->user_data = cur;
      g_assert(iter->user_data != 0);
      iter->user_data2 = GINT_TO_POINTER(levl[depth - 1]);
      iter->user_data3 = GINT_TO_POINTER(-1);
#ifdef DEBUG_TREE
      cout << "Node: " << get_path((*cur)[levl[depth - 1]]) << endl;
#endif
   } else {
      // Iterate over leaf nodes
      iter->user_data = cur;
      g_assert(iter->user_data != 0);
      iter->user_data2 = GINT_TO_POINTER(-1);
      iter->user_data3 = GINT_TO_POINTER(levl[depth - 1] - cur->size());
#ifdef DEBUG_TREE
      cout << "Node: " << get_path(cur) << ":" << (levl[depth - 1] -
                                                   cur->size()) << endl;
#endif

   }

   return TRUE;
}

static GtkTreePath *gtk_tag_tree_get_path(GtkTreeModel *tree_model,
                                          GtkTreeIter *iter)
{
   g_return_val_if_fail(GTK_IS_TAG_TREE(tree_model), NULL);
   g_return_val_if_fail(iter != NULL, NULL);
   g_return_val_if_fail(iter->user_data != NULL, NULL);

#ifdef DEBUG_TREE
   cout << "get_path(): " << iter->user_data << " : "
      << iter->user_data2 << endl;
#endif

   GtkTreePath *retval;

   retval = gtk_tree_path_new();

   HierarchyNode<int> *cur = (HierarchyNode<int> *)iter->user_data;
   int child = GPOINTER_TO_INT(iter->user_data2);
   int item = GPOINTER_TO_INT(iter->user_data3);

   if (item == -1)
      gtk_tree_path_prepend_index(retval, child);
   else
      gtk_tree_path_prepend_index(retval, item + cur->size());

   for (HierarchyNode<int> *p = cur->parent(); p; p = p->parent()) {
      int i = 0;
      for (; i < p->size() && (*p)[i] != cur; i++);
      gtk_tree_path_prepend_index(retval, i);
      cur = p;
   }

#ifdef DEBUG_TREE
   cout << "complete path: " << gtk_tree_path_to_string(retval) << endl;
#endif
   return retval;
}


static void
gtk_tag_tree_get_value(GtkTreeModel *tree_model,
                       GtkTreeIter *iter, gint column, GValue * value)
{
   g_return_if_fail(GTK_IS_TAG_TREE(tree_model));
   g_return_if_fail(iter != NULL);
   g_return_if_fail(column < GTK_TAG_TREE(tree_model)->n_columns);

   g_value_init(value, GTK_TAG_TREE(tree_model)->column_headers[column]);

   GtkTagTree *pkg_tree = (GtkTagTree *) tree_model;

#ifdef DEBUG_TREE_FULL
   cout << "get_value() ";
#endif

   HierarchyNode<int> *cur = (HierarchyNode<int> *)iter->user_data;
   int child = GPOINTER_TO_INT(iter->user_data2);
   int item = GPOINTER_TO_INT(iter->user_data3);

   string name;
   RPackage *pkg = NULL;
   if (item == -1) {
      // iter points to a child node
      if (child >= cur->size())
         cout << "child out of range!";
      HierarchyNode<int> *chld = (*cur)[child];
      if (chld)
         name = chld->tag();
      else {
         cout << "BAAADD!!";
         name = "sigh";
      }
   } else {
      // iter points to a package
      OpSet<int> items = cur->getItems();
      OpSet<int>::const_iterator n = items.begin();
      for (int i = 0; i < item; i++, n++);
      int handle = *n;
      //cout << "trying to get name of item" << endl;
      pkg = pkg_tree->hmaker->getItem(handle);
      if (pkg == NULL)
         return;
      name = pkg->name();
   }

#ifdef DEBUG_TREE_FULL
   cout << "column: " << column << " name: " << name << endl << " name: " +
      name;
#endif

   const gchar *str;

   switch (column) {
      case NAME_COLUMN:
         str = utf8(name.c_str());
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
         g_value_set_pointer(value, pkg);
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
gtk_tag_tree_iter_next(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
   g_return_val_if_fail(iter != NULL, FALSE);

   if (iter->user_data == NULL)
      return FALSE;

#ifdef DEBUG_TREE
   cout << "iter_next(): " << endl;
#endif

   HierarchyNode<int> *cur = (HierarchyNode<int> *)iter->user_data;
   int child = GPOINTER_TO_INT(iter->user_data2);
   int item = GPOINTER_TO_INT(iter->user_data3);

#ifdef DEBUG_TREE
   cout << "Node: " << cur->
      tag() << " child: " << child << " item: " << item << endl;
#endif

   if (item == -1) {
      // iter points to a child node
      if (child < cur->size() - 1)
         iter->user_data2 = GINT_TO_POINTER(child + 1);
      else if (cur->getItems().size() > 0) {
         iter->user_data2 = GINT_TO_POINTER(-1);
         iter->user_data3 = GINT_TO_POINTER(0);
      } else
         return FALSE;
   } else {
      int size = cur->getItems().size() - 1;
      if (item < size)
         iter->user_data3 = GINT_TO_POINTER(item + 1);
      else
         return FALSE;
   }

   return TRUE;
}


static gboolean
gtk_tag_tree_iter_children(GtkTreeModel *tree_model,
                           GtkTreeIter *iter, GtkTreeIter *parent)
{
#ifdef DEBUG_TREE
   cout << "gtk_tag_tree_iter_children()" << endl;
#endif

   HierarchyNode<int> *cur = (HierarchyNode<int> *)parent->user_data;
   int child = GPOINTER_TO_INT(parent->user_data2);
   int item = GPOINTER_TO_INT(parent->user_data3);

   // FIXME: this is a  special case (see treemodel docs)
   if (parent == NULL)
      return FALSE;
   //if(parent->user_data == NULL) return FALSE;

#ifdef DEBUG_TREE
   cout << " cur: " << cur << " child: " << child << " item: " << item << endl;

#endif
   if (cur->size() == 0)
      return FALSE;

   if (item != -1)
      return FALSE;

#ifdef DEBUG_TREE
   cout << "Node (before): " << get_path(cur) << endl;
#endif

   iter->user_data = cur = (*cur)[child];

#ifdef DEBUG_TREE
   cout << "Node (after): " << get_path(cur) << endl;
#endif


   g_assert(iter->user_data != 0);
   if (cur->size() > 0) {
      iter->user_data2 = GINT_TO_POINTER(0);
      iter->user_data3 = GINT_TO_POINTER(-1);
   } else if (cur->getItems().size() > 0) {
      iter->user_data2 = GINT_TO_POINTER(-1);
      iter->user_data3 = GINT_TO_POINTER(0);
   } else {
      cout <<
         "Found a node without child nodes nor items! (it should not be part of the tree)"
         << endl;
      iter->user_data2 = GINT_TO_POINTER(-1);
      iter->user_data3 = GINT_TO_POINTER(-1);
   }

   return TRUE;
}

static gboolean
gtk_tag_tree_iter_has_child(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
   g_return_val_if_fail(GTK_IS_TAG_TREE(tree_model), FALSE);
   g_return_val_if_fail(iter->user_data != NULL, FALSE);

#ifdef DEBUG_TREE
   cout << "iter_has_child(): " << endl;
#endif

   HierarchyNode<int> *cur = (HierarchyNode<int> *)iter->user_data;
   int item = GPOINTER_TO_INT(iter->user_data3);

   g_assert(cur != 0);

   // check if we are actually have children
   if (item != -1)
      return FALSE;
   else
      return TRUE;
}

static gint
gtk_tag_tree_iter_n_children(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
   g_return_val_if_fail(GTK_IS_TAG_TREE(tree_model), 0);
   g_return_val_if_fail(iter == NULL || iter->user_data != NULL, FALSE);

#ifdef DEBUG_TREE
   cout << "gtk_tree_iter_n_children()" << endl;
#endif

   HierarchyNode<int> *cur = (HierarchyNode<int> *)iter->user_data;
   int item = GPOINTER_TO_INT(iter->user_data3);

   if (item != -1)
      return 0;
   else
      return cur->size();
}

static gboolean
gtk_tag_tree_iter_nth_child(GtkTreeModel *tree_model,
                            GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
#ifdef DEBUG_TREE
   cout << "gtk_tag_tree_iter_nth_child(): n=" << n << endl;
   cout << "parent = " << parent << endl;
#endif
   g_return_val_if_fail(GTK_IS_TAG_TREE(tree_model), FALSE);

   // special case (yeah, the gtk_tree_model interface rocks :-/)
   if (parent == NULL) {
      //cout << "special case not there yet" << endl;
      return false;
#if 0
      RPackageLister::treeIter it(pkgTree->begin());
      RPackageLister::treeIter it_child(it);

      for (int i = 0; i < n; i++) {
         it_child.skip_children();
         ++it_child;
      }
      iter->user_data = it_child.node;
      return TRUE;
#endif
   }


   HierarchyNode<int> *cur = (HierarchyNode<int> *)parent->user_data;
   int child = GPOINTER_TO_INT(parent->user_data2);
   int item = GPOINTER_TO_INT(parent->user_data3);

   // ill case
   if (parent->user_data == NULL)
      return FALSE;

   // normal case
   // check if we are actually have children
   if (item != -1) {
      return FALSE;
   }

   /* go to the child pointed by the iterator */
   cur = (*cur)[child];

   /* now move to the n-th child */
   if (n < cur->size()) {
      iter->user_data = (HierarchyNode<int> *)cur;
      g_assert(iter->user_data != 0);
      iter->user_data2 = GINT_TO_POINTER(n);
      iter->user_data3 = GINT_TO_POINTER(-1);
   } else {
      iter->user_data = (HierarchyNode<int> *)cur;
      g_assert(iter->user_data != 0);
      iter->user_data2 = GINT_TO_POINTER(-1);
      iter->user_data3 = GINT_TO_POINTER(n - cur->size());
   }

#ifdef DEBUG_TREE
   cout << "parent name: " << endl;
   cout << "nth-child is " << endl;
#endif

   return TRUE;
}

static gboolean
gtk_tag_tree_iter_parent(GtkTreeModel *tree_model,
                         GtkTreeIter *iter, GtkTreeIter *child)
{
   g_return_val_if_fail(iter != NULL, FALSE);
   g_return_val_if_fail(child != NULL, FALSE);
   g_return_val_if_fail(child->user_data != NULL, FALSE);

   HierarchyNode<int> *cur = (HierarchyNode<int> *)child->user_data;

   HierarchyNode<int> *parent = cur->parent();
   if (!parent)
      return FALSE;

   iter->user_data = (HierarchyNode<int> *)parent;
   g_assert(iter->user_data != 0);
   iter->user_data3 = GINT_TO_POINTER(-1);

   for (int i = 0; i < parent->size(); i++)
      if ((*parent)[i] == cur) {
         iter->user_data2 = GINT_TO_POINTER(i);
         break;
      }

   return TRUE;

}

void test_tag_tree(GtkTagTree * _tagTree)
{
   GtkTreeIter it;
   GtkTreePath *path = gtk_tree_path_new_from_indices(1, -1);
   gtk_tag_tree_get_iter(GTK_TREE_MODEL(_tagTree), &it, path);
   int i = 0;
   do {
      GValue gv = { 0, };
      gtk_tag_tree_get_value(GTK_TREE_MODEL(_tagTree), &it, NAME_COLUMN, &gv);
      const gchar *name = g_value_get_string(&gv);
      cerr << ++i << ": " << name << "\n";
   } while (gtk_tag_tree_iter_next(GTK_TREE_MODEL(_tagTree), &it));
}

#endif /* HAVE_DEBTAGS */
