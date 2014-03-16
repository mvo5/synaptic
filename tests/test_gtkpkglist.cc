#include <gtk/gtk.h>

#include <apt-pkg/init.h>
#include <apt-pkg/configuration.h>
#include <iostream>

#include "config.h"
#include "rpackagelister.h"
#include "rpackageview.h"
#include "rpackage.h"

#include "gtk/gtkpkglist.h"
#include "gtk/rgmainwindow.h"
#include "gtk/rgpkgtreeview.h"

using namespace std;

int main(int argc, char **argv)
{
   gtk_init(&argc, &argv);

   pkgInitConfig(*_config);
   pkgInitSystem(*_config, _system);
   
   RPackageLister *lister = new RPackageLister();
   lister->openCache();

   GtkTreeModel *pkglist = GTK_TREE_MODEL(gtk_pkg_list_new(lister));
   GtkWidget *treeview = gtk_tree_view_new();
   setupTreeView(treeview);
   gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), pkglist);

   GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_container_add(GTK_CONTAINER(win), treeview);
   gtk_window_set_default_size(GTK_WINDOW(win), 600, 400);
   gtk_widget_show_all(win);

   gtk_main();
}
