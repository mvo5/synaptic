/* gtkpkgtree.c
 * Copyright (C) 2002  Michael Vogt
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
#include "gtkpkgtree.h"

static void         gtk_pkg_tree_init            (GtkPkgTree      *pkg_tree);
static void         gtk_pkg_tree_class_init      (GtkPkgTreeClass *klass);
static void         gtk_pkg_tree_tree_model_init (GtkTreeModelIface *iface);
static void         gtk_pkg_tree_finalize        (GObject           *object);
static GtkTreeModelFlags gtk_pkg_tree_get_flags  (GtkTreeModel      *tree_model);
static gint         gtk_pkg_tree_get_n_columns   (GtkTreeModel      *tree_model);
static GType        gtk_pkg_tree_get_column_type (GtkTreeModel      *tree_model,
						  gint               index);
static gboolean     gtk_pkg_tree_get_iter        (GtkTreeModel      *tree_model,
						  GtkTreeIter       *iter,
						  GtkTreePath       *path);
static GtkTreePath *gtk_pkg_tree_get_path        (GtkTreeModel      *tree_model,
						  GtkTreeIter       *iter);
static void         gtk_pkg_tree_get_value       (GtkTreeModel      *tree_model,
						  GtkTreeIter       *iter,
						  gint               column,
						  GValue            *value);
static gboolean     gtk_pkg_tree_iter_next       (GtkTreeModel      *tree_model,
						  GtkTreeIter       *iter);
static gboolean     gtk_pkg_tree_iter_children   (GtkTreeModel      *tree_model,
						  GtkTreeIter       *iter,
						  GtkTreeIter       *parent);
static gboolean     gtk_pkg_tree_iter_has_child  (GtkTreeModel      *tree_model,
						  GtkTreeIter       *iter);
static gint         gtk_pkg_tree_iter_n_children (GtkTreeModel      *tree_model,
						  GtkTreeIter       *iter);
static gboolean     gtk_pkg_tree_iter_nth_child  (GtkTreeModel      *tree_model,
						  GtkTreeIter       *iter,
						  GtkTreeIter       *parent,
						  gint               n);
static gboolean     gtk_pkg_tree_iter_parent     (GtkTreeModel      *tree_model,
						  GtkTreeIter       *iter,
						  GtkTreeIter       *child);


static void gtk_pkg_tree_set_n_columns   (GtkPkgTree *tree_store,
					  gint          n_columns);
static void gtk_pkg_tree_set_column_type (GtkPkgTree *tree_store,
					  gint          column,
					  GType         type);


static GObjectClass *parent_class = NULL;



GType
gtk_pkg_tree_get_type (void)
{
  static GType pkg_tree_type = 0;

  if (!pkg_tree_type)
    {
      static const GTypeInfo pkg_tree_info =
      {
        sizeof (GtkPkgTreeClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
        (GClassInitFunc) gtk_pkg_tree_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (GtkPkgTree),
	0,              /* n_preallocs */
        (GInstanceInitFunc) gtk_pkg_tree_init
      };

      static const GInterfaceInfo tree_model_info =
      {
	(GInterfaceInitFunc) gtk_pkg_tree_tree_model_init,
	NULL,
	NULL
      };

      pkg_tree_type = g_type_register_static (G_TYPE_OBJECT, "GtkPkgTree",
					      &pkg_tree_info, (GTypeFlags)0);

      g_type_add_interface_static (pkg_tree_type,
				   GTK_TYPE_TREE_MODEL,
				   &tree_model_info);
    }

  return pkg_tree_type;
}

static void
gtk_pkg_tree_class_init (GtkPkgTreeClass *klass)
{
  GObjectClass *object_class;

  parent_class = (GObjectClass*)g_type_class_peek_parent (klass);
  object_class = (GObjectClass *) klass;

  object_class->finalize = gtk_pkg_tree_finalize;
}

static void
gtk_pkg_tree_tree_model_init (GtkTreeModelIface *iface)
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


static void
gtk_pkg_tree_init (GtkPkgTree *pkg_tree)
{
  pkg_tree->n_columns = N_COLUMNS;
  pkg_tree->column_headers[0] = G_TYPE_STRING;
  pkg_tree->column_headers[1] = GDK_TYPE_PIXBUF;
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
GtkPkgTree *
gtk_pkg_tree_new (RPackageLister *lister, bool list_mode)
{
  GtkPkgTree *retval;

  g_print("pkg_tree_new()\n");

  retval = (GtkPkgTree*) g_object_new (GTK_TYPE_PKG_TREE, NULL);
  retval->_lister = lister;
  retval->_list_mode = list_mode;

  return retval;
}


static void
gtk_pkg_tree_finalize (GObject *object)
{
  GtkPkgTree *pkg_tree = GTK_PKG_TREE (object);


  /* give back all memory */
  

  /* must chain up */
  (* parent_class->finalize) (object);
}


static GtkTreeModelFlags
gtk_pkg_tree_get_flags (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), (GtkTreeModelFlags)0);

  return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
gtk_pkg_tree_get_n_columns (GtkTreeModel *tree_model)
{
  GtkPkgTree *pkg_tree = (GtkPkgTree *) tree_model;

  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), 0);

  pkg_tree->columns_dirty = TRUE;


  return pkg_tree->n_columns;
}


static GType
gtk_pkg_tree_get_column_type (GtkTreeModel *tree_model,
			      gint          index)
{
  GtkPkgTree *tree_store = (GtkPkgTree *) tree_model;

  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), G_TYPE_INVALID);
  g_return_val_if_fail (index < GTK_PKG_TREE (tree_model)->n_columns &&
			index >= 0, G_TYPE_INVALID);

  return tree_store->column_headers[index];
}


/* converted to here */

static gboolean
gtk_pkg_tree_get_iter (GtkTreeModel *tree_model,
		       GtkTreeIter  *iter,
		       GtkTreePath  *path)
{
  GtkPkgTree *pkg_tree = (GtkPkgTree *) tree_model;
  GtkTreeIter parent;
  gint *indices;
  gint depth, i;

  //g_print( "gtk_pkg_tree_get_iter()\n");
  g_return_val_if_fail (GTK_IS_PKG_TREE (pkg_tree), FALSE);

  indices = gtk_tree_path_get_indices (path);
  depth = gtk_tree_path_get_depth (path);

//   g_print("indices: %i : %i ; depth: %i \n",  indices[0], indices[1],   depth);
//   g_print("path: %s\n",gtk_tree_path_to_string(path));

  iter->user_data = GINT_TO_POINTER(depth);
  iter->user_data2 = GINT_TO_POINTER(indices[0]);
  iter->user_data3 = GINT_TO_POINTER(indices[1]);
  return TRUE;
}

static GtkTreePath *
gtk_pkg_tree_get_path (GtkTreeModel *tree_model,
			 GtkTreeIter  *iter)
{
  GtkTreePath *retval;
  GNode *tmp_node;
  gint i = 0;

  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), NULL);
  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (iter->user_data != NULL, NULL);

  g_print("gtk_pkg_tree_get_path()\n");

  return NULL;
}


static void
gtk_pkg_tree_get_value (GtkTreeModel *tree_model,
			GtkTreeIter  *iter,
			gint          column,
			GValue       *value)
{
  g_return_if_fail (GTK_IS_PKG_TREE (tree_model));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (column < GTK_PKG_TREE (tree_model)->n_columns);
  
  //g_print("gtk_pkg_tree_get_value()\n");

  int depth = GPOINTER_TO_INT(iter->user_data);
  int index = GPOINTER_TO_INT(iter->user_data2);
  int pkg_index = GPOINTER_TO_INT(iter->user_data3);

  //g_print("column: %i ; index: %i ; depth: %i \n",  column, index, depth);
  
  g_value_init (value, GTK_PKG_TREE(tree_model)->column_headers[column]);


  if(depth == 1) {
    switch(column) {
    case NAME_COLUMN: 
      {
	if(GTK_PKG_TREE(tree_model)->_list_mode) {
	  if(GTK_PKG_TREE(tree_model)->_lister->count() == 0)
	    return;

	  RPackage *pkg = GTK_PKG_TREE(tree_model)->_lister->getElement(index);
	  if(pkg==NULL)
	    return;
	  gchar *str = g_strdup(pkg->name());
	  g_value_set_string(value, str);
 	} else {
	  vector<string> sections;
	  GTK_PKG_TREE(tree_model)->_lister->getSections(sections);
	  if(sections.size() == 0 || index >= sections.size()) 
	    return;

	  gchar *str = g_strdup(sections[index].c_str());
	  g_value_set_string(value, str);
	  break;
	}
      }
    }
  }


  if(depth == 2) {
    RPackage *pkg;
    pkg = GTK_PKG_TREE(tree_model)->_lister->getElementOfSection(index,pkg_index);
    if(pkg==NULL)
      return;

    switch(column) {
    case NAME_COLUMN: 
      {
	gchar *str = g_strdup(pkg->name());
	//g_print("pkg: %i ; index: %i ; depth: %i \n", pkg_index, index, depth);
	g_value_set_string(value, str);
	break;
      }
    case PKG_COLUMN:
      {
	g_value_set_pointer(value, (gpointer)(pkg));
      }
    }
  }

}

static gboolean
gtk_pkg_tree_iter_next (GtkTreeModel  *tree_model,
			GtkTreeIter   *iter)
{
  int count;
  //  g_print("gtk_pkg_tree_iter_next()\n");

  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (iter->user_data != NULL, FALSE);

  int depth = GPOINTER_TO_INT(iter->user_data);
  int index = GPOINTER_TO_INT(iter->user_data2);


  if(GTK_PKG_TREE(tree_model)->_list_mode)
    count = GTK_PKG_TREE(tree_model)->_lister->count();
  else
    count = GTK_PKG_TREE(tree_model)->_lister->nrOfSections();
  
  //g_print("_lister: %x\n",GTK_PKG_TREE(tree_model)->_lister);
  //g_print("index: %i ; depth: %i ; count: %i \n",  index, depth, count);
  if(depth == 1 && index < count)
    {
      index++;
      iter->user_data2 = GINT_TO_POINTER(index);
      //g_print("return TRUE; indices: %i\n",index);
      return TRUE;
    }

  if(depth == 2) {
    int depth = GPOINTER_TO_INT(iter->user_data);
    int sec_index = GPOINTER_TO_INT(iter->user_data2);
    int pkg_index = GPOINTER_TO_INT(iter->user_data3);
    //g_print("sec_index: %i ; pkg_index: %i\n",sec_index,pkg_index);
    iter->user_data3 = GINT_TO_POINTER(++pkg_index);
    count = GTK_PKG_TREE(tree_model)->_lister->getNrElementsOfSection(sec_index);    
    if(pkg_index > count)
      return FALSE;
    return TRUE;
  }

  return FALSE;

}

static gboolean
gtk_pkg_tree_iter_children (GtkTreeModel *tree_model,
			    GtkTreeIter  *iter,
			    GtkTreeIter  *parent)
{
  g_return_val_if_fail (parent == NULL || parent->user_data != NULL, FALSE);

  //g_print( "gtk_pkg_tree_iter_children()\n");

  int depth = GPOINTER_TO_INT(parent->user_data);
  int index = GPOINTER_TO_INT(parent->user_data2);
  iter->user_data = GINT_TO_POINTER(++depth);
  iter->user_data2 = GINT_TO_POINTER(index);
  iter->user_data3 = 0;
  //g_print("after: index: %i ; depth: %i \n",  index, depth);
  if(depth > 2)
    return FALSE;

  return TRUE;

#if 0
  if (parent)
    children = G_NODE (parent->user_data)->children;
  else
    children = G_NODE (GTK_TREE_STORE (tree_model)->root)->children;

  if (children)
    {
      iter->stamp = GTK_TREE_STORE (tree_model)->stamp;
      iter->user_data = children;
      return TRUE;
    }
  else
    return FALSE;
#endif
}

static gboolean
gtk_pkg_tree_iter_has_child (GtkTreeModel *tree_model,
			     GtkTreeIter  *iter)
{
  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), FALSE);
  g_return_val_if_fail (iter->user_data != NULL, FALSE);

  //g_print("gtk_pkg_tree_iter_has_child()\n");
  int depth = GPOINTER_TO_INT(iter->user_data);
  //g_print("depth: %i\n", depth);

  if(GTK_PKG_TREE(tree_model)->_list_mode)
    return FALSE;

  /* toplevel */
  if(depth == 1)
    return TRUE;
  else
    return FALSE;
}

void gtk_pkg_tree_refresh(GtkPkgTree *pkg_tree)
{
  
  
//   gtk_tree_model_row_insert(GTK_TREE_MODEL(tree_model),
// 			    GtkTreePath *path,
// 			    GtkTreeIter *iter);
  
  

}

static gint
gtk_pkg_tree_iter_n_children (GtkTreeModel *tree_model,
				GtkTreeIter  *iter)
{
  GNode *node;
  gint i = 0;

  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), 0);
  g_return_val_if_fail (iter == NULL || iter->user_data != NULL, FALSE);

  //g_print("gtk_tree_store_iter_n_children()\n");
  int depth = GPOINTER_TO_INT(iter->user_data);
  int sec_index = GPOINTER_TO_INT(iter->user_data2);
  //  g_print("depth: %i\n", depth);

  /* toplevel */
  if(depth == 1)
    return GTK_PKG_TREE(tree_model)->_lister->nrOfSections();

  if(depth == 2)
    return GTK_PKG_TREE(tree_model)->_lister->getNrElementsOfSection(sec_index);

}

static gboolean
gtk_pkg_tree_iter_nth_child (GtkTreeModel *tree_model,
			     GtkTreeIter  *iter,
			     GtkTreeIter  *parent,
			     gint          n)
{
  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), FALSE);

  //g_print("gtk_pkg_tree_iter_nth_child ()\n");
  if(parent == NULL)
    return FALSE;

  int depth = GPOINTER_TO_INT(parent->user_data);
  int index = GPOINTER_TO_INT(parent->user_data2);
  //g_print("depth: %i ; index: %i ; n: %i \n",depth,index, n);
  iter->user_data = GINT_TO_POINTER(++depth);
  iter->user_data2 = GINT_TO_POINTER(index);
  iter->user_data3 = GINT_TO_POINTER(n);

  return TRUE;
}

static gboolean
gtk_pkg_tree_iter_parent (GtkTreeModel *tree_model,
			  GtkTreeIter  *iter,
			  GtkTreeIter  *child)
{
  GNode *parent;

  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (child != NULL, FALSE);
  g_return_val_if_fail (child->user_data != NULL, FALSE);

  //g_print("gtk_pkg_tree_iter_parent ()\n");

  int depth = GPOINTER_TO_INT(child->user_data);
  int section = GPOINTER_TO_INT(child->user_data2);
  iter->user_data = GINT_TO_POINTER(section);
  iter->user_data2 = GINT_TO_POINTER(--depth);
  iter->user_data3 = 0;

  return TRUE;

}


