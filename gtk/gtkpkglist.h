/* gtkpkglist.h
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

#ifndef GTKPKGLIST_H
#define GTKPKGLIST_H

#include <gtk/gtk.h>
#include "rpackagelister.h"
#include "rcacheactor.h"
#include "rpackagelistactor.h"
#include "rgutils.h"


#define GTK_TYPE_PKG_LIST			(gtk_pkg_list_get_type ())
#define GTK_PKG_LIST(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_PKG_LIST, GtkPkgList))
#define GTK_PKG_LIST_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_PKG_LIST, GtkPkgListClass))
#define GTK_IS_PKG_LIST(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_PKG_LIST))
#define GTK_IS_PKG_LIST_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_PKG_LIST))
#define GTK_PKG_LIST_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_PKG_LIST, GtkPkgListClass))

typedef struct _GtkPkgList GtkPkgList;
typedef struct _GtkPkgListClass GtkPkgListClass;


struct _GtkPkgList {
   GObject parent;

   RPackageLister *_lister;

   gint n_columns;
   GType column_headers[N_COLUMNS];
   // sortable
   gint sort_column_id;
   GtkSortType order;
};

struct _GtkPkgListClass {
   GObjectClass parent_class;
};


GType gtk_pkg_list_get_type();
GtkPkgList *gtk_pkg_list_new(RPackageLister *lister);

class RCacheActorPkgList : public RCacheActor {

   protected:

   GtkPkgList *_pkgList;
   GtkTreeView *_pkgView;

   public:

   virtual void run(vector<RPackage *> &List, int Action);

   RCacheActorPkgList(RPackageLister *lister,
                      GtkPkgList *pkgList,
                      GtkTreeView *pkgView)
      : RCacheActor(lister), _pkgList(pkgList), _pkgView(pkgView) {};
};


class RPackageListActorPkgList:public RPackageListActor {

   protected:

   GtkPkgList *_pkgList;
   GtkTreeView *_pkgView;

   public:

   virtual void run(vector<RPackage *> &List, int listEvent);

   RPackageListActorPkgList(RPackageLister *lister,
                            GtkPkgList *pkgList,
                            GtkTreeView *pkgView)
      : RPackageListActor(lister), _pkgList(pkgList), _pkgView(pkgView) {};
};

#endif /* GTKPKGLIST_H */

// vim:ts=3:sw=3:et
