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
#include <gtk/gtk.h>
#include <apt-pkg/configuration.h>
#include "gtkpkgtree.h"
#include "gsynaptic.h"

using namespace std;

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

static GObjectClass *parent_class = NULL;

extern GdkPixbuf *StatusPixbuf[12];
extern GdkColor *StatusColors[12];


GType
gtk_pkg_tree_get_type (void)
{
  static GType pkg_tree_type = 0;

  if (!pkg_tree_type)
    {
      static const GTypeInfo pkg_tree_info =
      {
        sizeof (GtkPkgTreeClass),
	NULL,		     /* base_init */
	NULL,		     /* base_finalize */
        (GClassInitFunc)     gtk_pkg_tree_class_init,
	NULL,                /* class finalize */
	NULL,		     /* class_data */
        sizeof (GtkPkgTree),
	0,                   /* n_preallocs */
        (GInstanceInitFunc)  gtk_pkg_tree_init
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
  //cout << "tree_init()" << endl;
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
gtk_pkg_tree_new (RPackageLister *lister)
{
  GtkPkgTree *retval;

  //g_print("pkg_tree_new()\n");

  retval = (GtkPkgTree*) g_object_new (GTK_TYPE_PKG_TREE, NULL);
  retval->_lister = lister;

  return retval;
}


static void
gtk_pkg_tree_finalize (GObject *object)
{
    //  GtkPkgTree *pkg_tree = GTK_PKG_TREE (object);


    /* give back all memory */

    
    /* must chain up */
    (* parent_class->finalize) (object);
}


static GtkTreeModelFlags
gtk_pkg_tree_get_flags (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), (GtkTreeModelFlags)0);

//   RPackageLister::treeDisplayMode mode = GTK_PKG_TREE(tree_model)->_lister->getTreeDisplayMode();

//   if(mode == RPackageLister::TREE_DISPLAY_FLAT)
//     return GTK_TREE_MODEL_LIST_ONLY|GTK_TREE_MODEL_ITERS_PERSIST);
//   else
  return GTK_TREE_MODEL_ITERS_PERSIST;
}


static gint
gtk_pkg_tree_get_n_columns (GtkTreeModel *tree_model)
{
  GtkPkgTree *pkg_tree = (GtkPkgTree *) tree_model;

  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), 0);

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



static gboolean
gtk_pkg_tree_get_iter (GtkTreeModel *tree_model,
		       GtkTreeIter  *iter,
		       GtkTreePath  *path)
{
  tree<RPackageLister::pkgPair> *pkgTree;

  GtkPkgTree *pkg_tree = (GtkPkgTree *) tree_model;
  gint *indices;
  gint depth;

  //  g_print( "gtk_pkg_tree_get_iter()\n");
  g_return_val_if_fail (GTK_IS_PKG_TREE (pkg_tree), FALSE);

  indices = gtk_tree_path_get_indices (path);
  depth = gtk_tree_path_get_depth (path);

  pkgTree = pkg_tree->_lister->getTreeOrganizer();

//   // tree is empty
  if(pkgTree->size() == 0)
    return FALSE;

  RPackageLister::treeIter it(pkgTree->begin());

  // we do not support more 
  assert(depth <= 2);
    
  // jump to right spot 
  for(int j=0;j<indices[0];j++) {
    it.skip_children();
    ++it;
  }

  // now go for the children 
  if(depth == 2) {
    for(int j=-1;j<indices[1];j++) {
      ++it;
    }
  }

//      cout << "get_iter: index " << indices[0] << ":"<<indices[1] << " is: " 
//           << (*it).first << " depth: " << depth
//           << endl;

  iter->user_data = it.node;
  return TRUE;
}

static GtkTreePath *
gtk_pkg_tree_get_path (GtkTreeModel *tree_model,
		       GtkTreeIter  *iter)
{
  GtkTreePath *retval;
  gint i = 0;

  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), NULL);
  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (iter->user_data != NULL, NULL);

  tree<RPackageLister::pkgPair> *pkgTree;
  pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

  retval = gtk_tree_path_new ();
  RPackageLister::sibTreeIter it((RPackageLister::treeNode*)iter->user_data);

  //   cout << "get_path(): " << (*it).first;

  do {
    i = pkgTree->index(it);
    //cout << " index: " << i << endl;
    gtk_tree_path_prepend_index(retval, i);
    it = pkgTree->parent(it);
  } while(pkgTree->is_valid(it) );
  
  //cout << "complete path: " << gtk_tree_path_to_string(retval) << endl;
  return retval;
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
  
  g_value_init (value, GTK_PKG_TREE(tree_model)->column_headers[column]);

  tree<RPackageLister::pkgPair> *pkgTree;
  pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

  RPackageLister::treeIter it((RPackageLister::treeNode*)iter->user_data);
  //  cout << "get_value()" << column << " name: " << (*it).first << endl;

  if(it == pkgTree->end())
    return;

  const gchar *str;
  RPackage *pkg = (RPackage*)(*it).second;
  switch(column) {
  case NAME_COLUMN: 
      str = utf8((*it).first.c_str());
      g_value_set_string(value, str);
      break;
  case INSTALLED_VERSION_COLUMN:
      if(pkg == NULL) return;
      str = pkg->installedVersion();
      g_value_set_string(value, str);
      break;
  case AVAILABLE_VERSION_COLUMN:
      if(pkg == NULL) return;      
      str = pkg->availableDownloadableVersion();
      g_value_set_string(value, str);
      break;
  case DESCR_COLUMN:
      if(pkg == NULL) return;
      str = utf8(pkg->summary());
      g_value_set_string(value, str);
      break;
  case PKG_COLUMN:
      if(pkg == NULL) return;
      g_value_set_pointer(value, (*it).second);
      break;
  case COLOR_COLUMN:
    {
      if(pkg == NULL) return;
      GdkColor *bg;
      RPackage::MarkedStatus s = pkg->getMarkedStatus();
      int other = pkg->getOtherStatus();
      
      if (pkg->wouldBreak()) {
	bg =  StatusColors[(int)RPackage::MBroken];
      } else if (other & RPackage::OPinned) {
	bg = StatusColors[(int)RPackage::MPinned];
      } else if (s == RPackage::MKeep 
		 && pkg->getStatus() == RPackage::SInstalledOutdated) {
	bg = StatusColors[RPackage::MHeld];
      } else if ((other & RPackage::ONew) && 
		 !pkg->getMarkedStatus() == RPackage::MInstall) {
	bg = StatusColors[(int)RPackage::MNew];
      } else {
	bg = StatusColors[(int)s];
      }
      g_value_set_boxed(value, bg);
      break;
    }
  case PIXMAP_COLUMN:
    {
      if(pkg == NULL) return;
      GdkPixbuf *pix;

      RPackage::MarkedStatus s = pkg->getMarkedStatus();
      int other = pkg->getOtherStatus();
      
      if (pkg->wouldBreak()) {
	pix = StatusPixbuf[(int)RPackage::MBroken];
      } else if (other & RPackage::OPinned) {
	pix = StatusPixbuf[(int)RPackage::MPinned];
      } else if (s == RPackage::MKeep 
		 && pkg->getStatus() == RPackage::SInstalledOutdated) {
	pix = StatusPixbuf[RPackage::MHeld];
      } else if ((other & RPackage::ONew) && 
		 !pkg->getMarkedStatus() == RPackage::MInstall) {
	pix = StatusPixbuf[(int)RPackage::MNew];
      } else {
	pix = StatusPixbuf[(int)s];
      }
      g_value_set_object(value, pix);
      break;
    }
  }
}

static gboolean
gtk_pkg_tree_iter_next (GtkTreeModel  *tree_model,
			GtkTreeIter   *iter)
{
  g_return_val_if_fail (iter != NULL, FALSE);
  
  if(iter->user_data == NULL)
    return FALSE;

  tree<RPackageLister::pkgPair> *pkgTree;
  pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

  RPackageLister::sibTreeIter it((RPackageLister::treeNode*)iter->user_data);

  //cout << "iter_next(): " << (*it).first << endl;
  int depth = pkgTree->depth(it);
  //cout << "before: " << depth << endl;
  
  if(depth == 0)
    it.skip_children();

  ++it;

  //  cout << "after: " << pkgTree->depth(*it) << endl;
  if( it == pkgTree->end() ||  !pkgTree->is_valid(it) || depth != pkgTree->depth(it) ) 
    {
      //g_print("end of children \n");
      return FALSE;
    } else {
      iter->user_data = it.node;
      return TRUE;
    }
}

static gboolean
gtk_pkg_tree_iter_children (GtkTreeModel *tree_model,
			    GtkTreeIter  *iter,
			    GtkTreeIter  *parent)
{
  g_return_val_if_fail (parent == NULL || parent->user_data != NULL, FALSE);

  tree<RPackageLister::pkgPair> *pkgTree;
  pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();
  RPackageLister::treeIter it((RPackageLister::treeNode*)parent->user_data);
  //cout << "iter_children: parent: " << (*it).first << endl;  

  if(pkgTree->number_of_children(it) == 0)
    return FALSE;

  RPackageLister::treeIter it_child(it);

  /* go to first child */
  ++it_child;
  
  //cout     << " child: " << (*it_child)->first << endl;

  iter->user_data = it_child.node;

  return TRUE;
}

static gboolean
gtk_pkg_tree_iter_has_child (GtkTreeModel *tree_model,
			     GtkTreeIter  *iter)
{
  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), FALSE);
  g_return_val_if_fail (iter->user_data != NULL, FALSE);

  tree<RPackageLister::pkgPair> *pkgTree;
  pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

  RPackageLister::treeIter it((RPackageLister::treeNode*)iter->user_data);

//      cout <<" iter_has_child(): " << (*it).first << " nr: " 
//           << pkgTree->number_of_children(it) << endl;
    
  if( pkgTree->number_of_children(it) > 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

static gint
gtk_pkg_tree_iter_n_children (GtkTreeModel *tree_model,
			      GtkTreeIter  *iter)
{
  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), 0);
  g_return_val_if_fail (iter == NULL || iter->user_data != NULL, FALSE);

  //g_print("gtk_tree_iter_n_children()\n");
  
  tree<RPackageLister::pkgPair> *pkgTree;
  pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();
  RPackageLister::treeIter it((RPackageLister::treeNode*)iter->user_data);  

  return pkgTree->number_of_children(it);
}

static gboolean
gtk_pkg_tree_iter_nth_child (GtkTreeModel *tree_model,
			     GtkTreeIter  *iter,
			     GtkTreeIter  *parent,
			     gint          n)
{
  g_return_val_if_fail (GTK_IS_PKG_TREE (tree_model), FALSE);

  tree<RPackageLister::pkgPair> *pkgTree;
  pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();

  // special case (yeah, the gtk_tree_model interface rocks :-/)
  if(parent == NULL) {
    RPackageLister::treeIter it(pkgTree->begin());
    RPackageLister::treeIter it_child(it);

    for(int i=0;i<n;i++) {
      it_child.skip_children();
      ++it_child;
    }
    iter->user_data = it_child.node;
    return TRUE;
  }

  // ill case
  if(parent->user_data == NULL) 
    return FALSE;

  // normal case
  RPackageLister::treeIter it((RPackageLister::treeNode*)parent->user_data);
  RPackageLister::treeIter it_child(it);

  for(int i=0;i<n;i++)
    ++it_child;
  
  //   cout << "nth-child is " << (*it_child).first << endl;
  iter->user_data = it_child.node;

  return TRUE;
}

static gboolean
gtk_pkg_tree_iter_parent (GtkTreeModel *tree_model,
			  GtkTreeIter  *iter,
			  GtkTreeIter  *child)
{
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (child != NULL, FALSE);
  g_return_val_if_fail (child->user_data != NULL, FALSE);

  //g_print("gtk_pkg_tree_iter_parent ()\n");
  tree<RPackageLister::pkgPair> *pkgTree;
  pkgTree = GTK_PKG_TREE(tree_model)->_lister->getTreeOrganizer();
  RPackageLister::treeIter it((RPackageLister::treeNode*)child->user_data);
  
  RPackageLister::treeIter it_parent = pkgTree->parent(it);

  iter->user_data = it_parent.node;
  
  return TRUE;

}



void gtk_pkg_tree_refresh(GtkPkgTree *pkg_tree)
{

//   tree<RPackageLister::pkgPair> *pkgTree;
//   pkgTree = pkg_tree->_lister->getTreeOrganizer();
  
  cout << "void gtk_pkg_tree_refresh(GtkPkgTree *pkg_tree)"<<endl;

//   gtk_tree_model_foreach(GTK_TREE_MODEL(pkg_tree), 
// 			 gtk_pkg_tree_changed, 
// 			 NULL);


}
