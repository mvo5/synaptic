/* gtkpkgtree.h
 * Copyright (C) 2003  Michael Vogt 
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

#ifndef __GTK_PKG_TREE_H__
#define __GTK_PKG_TREE_H__

#include <gtk/gtk.h>
#include "rpackagelister.h"
#include "conversion.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_TYPE_PKG_TREE			(gtk_pkg_tree_get_type ())
#define GTK_PKG_TREE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_PKG_TREE, GtkPkgTree))
#define GTK_PKG_TREE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_PKG_TREE, GtkPkgTreeClass))
#define GTK_IS_PKG_TREE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_PKG_TREE))
#define GTK_IS_PKG_TREE_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_PKG_TREE))
#define GTK_PKG_TREE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_PKG_TREE, GtkPkgTreeClass))

typedef struct _GtkPkgTree       GtkPkgTree;
typedef struct _GtkPkgTreeClass  GtkPkgTreeClass;

  enum {
    DUMMY_COLUMN,
    PIXMAP_COLUMN,
    NAME_COLUMN,
    INSTALLED_VERSION_COLUMN,
    AVAILABLE_VERSION_COLUMN,
    DESCR_COLUMN,
    COLOR_COLUMN,
    PKG_COLUMN,
    N_COLUMNS
  };


struct _GtkPkgTree
{
  GObject parent;

  RPackageLister *_lister;
  gint n_columns;
  GType column_headers[N_COLUMNS];
};

struct _GtkPkgTreeClass
{
  GObjectClass parent_class;

};


GType       gtk_pkg_tree_get_type         ();
GtkPkgTree *gtk_pkg_tree_new              (RPackageLister *lister);

/* is this really needed */
void        gtk_pkg_tree_refresh          (GtkPkgTree *);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_TREE_STORE_H__ */
