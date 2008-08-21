/* gtktagtree.h
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

#ifndef __GTK_TAG_TREE_H__
#define __GTK_TAG_TREE_H__

#include "config.h"

#ifdef HAVE_DEBTAGS

#include <iostream>
#include <gtk/gtk.h>

#include <TagcollParser.h>
#include <StdioParserInput.h>
#include <SmartHierarchy.h>
#include <TagcollBuilder.h>
#include <HandleMaker.h>
#include <TagCollection.h>

#include "gsynaptic.h"
#include "rpackagelister.h"
#include "rcacheactor.h"
#include "rpackagelistactor.h"
#include "rtagcollbuilder.h"
#include "rgutils.h"

using namespace std;

#define GTK_TYPE_TAG_TREE			(gtk_tag_tree_get_type ())
#define GTK_TAG_TREE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_TAG_TREE, GtkTagTree))
#define GTK_TAG_TREE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_TAG_TREE, GtkTagTreeClass))
#define GTK_IS_TAG_TREE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_TAG_TREE))
#define GTK_IS_TAG_TREE_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_TAG_TREE))
#define GTK_TAG_TREE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_TAG_TREE, GtkTagTreeClass))

typedef struct _GtkTagTree GtkTagTree;
typedef struct _GtkTagTreeClass GtkTagTreeClass;

struct _GtkTagTree {
   GObject parent;

   RPackageLister *_lister;
   gint n_columns;
   GType column_headers[N_COLUMNS];

     HierarchyNode<int> *root;
     HandleMaker<RPackage *> *hmaker;
};

struct _GtkTagTreeClass {
   GObjectClass parent_class;

};

GType gtk_tag_tree_get_type();
GtkTagTree *gtk_tag_tree_new(RPackageLister *_lister,
                             HierarchyNode<int> *root,
                             HandleMaker<RPackage *> *hmaker);

void test_tag_tree(GtkTagTree *);

#endif // HAVE_DEBTAGS

#endif /* __GTK_TREE_STORE_H__ */
