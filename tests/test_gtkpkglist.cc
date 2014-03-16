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
   
   _config->Set("Debug::Synaptic::View", true);
   
   RPackageLister *lister = new RPackageLister();
   lister->openCache();
   lister->setView(0);
   lister->setSubView("");

   std::cerr << "lister all packages size: " 
             << lister->packagesSize()
             << std::endl;

   std::cerr << "lister visible size: " 
             << lister->viewPackagesSize()
             << std::endl;

   GtkTreeModel *pkglist = GTK_TREE_MODEL(gtk_pkg_list_new(lister));
   GtkWidget *treeview = gtk_tree_view_new();
   setupTreeView(treeview);
   gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), pkglist);

   std::cerr << "size model: " 
             << gtk_tree_model_iter_n_children(pkglist, NULL) 
             << std::endl;

   GtkWidget *scroll =  gtk_scrolled_window_new(NULL, NULL);
   gtk_container_add(GTK_CONTAINER(scroll), treeview);

   GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_container_add(GTK_CONTAINER(win), scroll);
   gtk_window_set_default_size(GTK_WINDOW(win), 600, 400);
   gtk_widget_show_all(win);

   gtk_main();
}
