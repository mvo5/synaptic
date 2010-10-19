/* rgtaskswin.cc
 *
 * Copyright (c) 2004 Michael Vogt
 *
 * Author: Michael Vogt <mvo@debian.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <gtk/gtk.h>
#include <cassert>
#include <list>
#include "config.h"
#include "rgtaskswin.h"
#include "rgmainwindow.h"
#include "rguserdialog.h"
#include "i18n.h"

enum {
   TASK_CHECKBOX_COLUMN,
   TASK_SENSITIVE_COLUMN,
   TASK_NAME_COLUMN,
   TASK_DESCR_COLUMN,
   TASK_N_COLUMNS
};

void RGTasksWin::cbButtonOkClicked(GtkWidget *self, void *data)
{
   //cout << "cbButtonOkClicked(GtkWidget *self, void *data)"<<endl;
   RGTasksWin *me = (RGTasksWin*)data;
   
   GtkTreeIter iter;
   GtkTreeModel *model = GTK_TREE_MODEL(me->_store);
   if(!gtk_tree_model_get_iter_first(model, &iter)) {
      me->hide();
      return;
   }

   me->setBusyCursor(true);

   // get selected tasks
   bool act=false;
   gboolean marked = FALSE;
   gboolean activatable = FALSE;
   gchar *taskname = NULL;
   string cmd = _config->Find("Synaptic::taskHelperProg", "/usr/bin/tasksel");
   do {
      gtk_tree_model_get(model, &iter, 
			 TASK_CHECKBOX_COLUMN, &marked, 
			 TASK_SENSITIVE_COLUMN, &activatable, 
			 TASK_NAME_COLUMN, &taskname,
			 -1);
      // only install if the state has changed
      if(marked && activatable) {
	 //cout << "selected: " << taskname << endl;
	 cmd += " --task-packages " + string(taskname);
	 act=true;
      }
   } while(gtk_tree_model_iter_next(model, &iter));

   //   cout << "cmd: " << cmd << endl;

   vector<string> packages;
   // only act if at least one task was selected
   if(act) {
      char buf[255];
      FILE *f = popen(cmd.c_str(), "r");
      while(fgets(buf, 254, f) != NULL) {
	 packages.push_back(string(g_strstrip(buf)));
      }
      pclose(f);

#if 0 // some debug code
      cout << "got: " << endl;
      for(int i=0;i<packages.size();i++) {
	 cout << packages[i] << endl;
      }
#endif
   }

   me->setBusyCursor(false);
   me->hide();

   me->_mainWin->selectToInstall(packages);

}

void RGTasksWin::cbButtonCancelClicked(GtkWidget *self, void *data)
{
   //cout << "cbButtonCancelClicked(GtkWidget *self, void *data)"<<endl;
   RGTasksWin *me = (RGTasksWin*)data;
   me->hide();
}

void RGTasksWin::cbButtonDetailsClicked(GtkWidget *self, void *data)
{
   //cout << "cbButtonDetailsClicked(GtkWidget *self, void *data)"<<endl;
   RGTasksWin *me = (RGTasksWin*)data;

   me->setBusyCursor(true);

   // get selected task-name
   GtkTreeIter iter;
   GtkTreeSelection* selection;
   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_taskView));
   if(!gtk_tree_selection_get_selected (selection,
					(GtkTreeModel**)(&me->_store), 
					&iter)) {
      return;
   }
   gchar *str;
   gtk_tree_model_get(GTK_TREE_MODEL(me->_store), &iter,
		      TASK_NAME_COLUMN, &str, -1);

   // ask tasksel about the selected task
   string cmd = _config->Find("Synaptic::taskHelperProg", "/usr/bin/tasksel") + " --task-desc " + string(str);
   string taskDescr;
   char buf[255];
   FILE *f=popen(cmd.c_str(), "r");
   while(fgets(buf, 254, f) != NULL) {
      taskDescr += string(buf);
   }

   // display the result in a nice dialog
   RGGtkBuilderUserDialog dia(me, "task_descr");
   
   //TRANSLATORS: Title of the task window - %s is the task (e.g. "desktop" or "mail server")
   gchar *title = g_strdup_printf(_("Description %s"), str);
   dia.setTitle(title);
   
   GtkWidget *tv = GTK_WIDGET(gtk_builder_get_object(dia.getGtkBuilder(),
                                                     "textview"));
   GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
   gtk_text_buffer_set_text(tb, utf8(taskDescr.c_str()), -1);
   
   dia.run();

   g_free(str);
   g_free(title);
   me->setBusyCursor(false);
}


void RGTasksWin::cell_toggled_callback (GtkCellRendererToggle *cell,
					gchar *path_string,
					gpointer user_data)
{
   GtkTreeIter iter;
   gboolean res;

   RGTasksWin *me = (RGTasksWin *)user_data;

   static int selected = 0;

   GtkTreeModel *model = GTK_TREE_MODEL(me->_store);
   GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
   gtk_tree_model_get_iter(model, &iter, path);
   gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
		      TASK_CHECKBOX_COLUMN, &res, -1);
   gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		      TASK_CHECKBOX_COLUMN, !res,
		      -1);

   if(!res)
      selected++;
   else
      selected--;

   if(selected > 0)
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(me->_builder,
                                                                 "button_ok")),
			       true);
   else 
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(me->_builder,
                                                                 "button_ok")),
			       false);
}

void RGTasksWin::selection_changed_callback(GtkTreeSelection *selection,
					    gpointer data)
{
   RGTasksWin *me = (RGTasksWin*)data;
   
   GtkTreeIter iter;
   GtkTreeModel *model;
   bool sensitiv = false;

   if (gtk_tree_selection_get_selected (selection, &model, &iter))  
      sensitiv = true;

   gtk_widget_set_sensitive(me->_detailsButton, sensitiv);
}


RGTasksWin::RGTasksWin(RGWindow *parent) 
   : RGGtkBuilderWindow(parent, "tasks")
{
   _mainWin = (RGMainWindow *)parent;
   _detailsButton = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                      "button_details"));
   assert(_detailsButton);

   GtkListStore *store= _store = gtk_list_store_new (TASK_N_COLUMNS, 
						     G_TYPE_BOOLEAN, 
						     G_TYPE_BOOLEAN, 
						     G_TYPE_STRING, 
						     G_TYPE_STRING);
   
   // fiel in tasks
   char buf[255];
   gchar **strArray;
   string cmd = _config->Find("Synaptic::taskHelperProg", "/usr/bin/tasksel") + " --list-tasks";
   FILE *f = popen(cmd.c_str(),"r");

   while(fgets(buf,254,f) != NULL) {
      bool installed = false;

      strArray = g_strsplit(buf, "\t", 2);
      g_strstrip(strArray[1]);
      if(strArray[0][0] == 'i')
	 installed = true;

      char *name = strArray[0];
      name+=2;

      GtkTreeIter iter;
      gtk_list_store_append (store, &iter);
      // you can't uninstall a task for now from synaptic, we make
      // tasks that are already installed insensitive
      gtk_list_store_set (store, &iter,
			  TASK_CHECKBOX_COLUMN, installed,
			  TASK_SENSITIVE_COLUMN, !installed,
			  TASK_NAME_COLUMN, name,
			  TASK_DESCR_COLUMN, strArray[1],
			  -1);
      g_strfreev(strArray);
   }
   pclose(f);
   GtkWidget *tree;
   GtkTreeSelection * select;

   tree = _taskView = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                        "treeview_tasks"));
   select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
   gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
   g_signal_connect (G_OBJECT (select), "changed",
		     G_CALLBACK (selection_changed_callback),
		     this);
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;
   renderer = gtk_cell_renderer_toggle_new ();
   g_object_set(renderer, "activatable", TRUE, NULL);
   g_signal_connect(renderer, "toggled", 
		    (GCallback) cell_toggled_callback, this);
   column = gtk_tree_view_column_new_with_attributes ("Install",
                                                      renderer,
                                                      "active", TASK_CHECKBOX_COLUMN,
						      "activatable", TASK_SENSITIVE_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
   renderer = gtk_cell_renderer_text_new ();
#if 0
   column = gtk_tree_view_column_new_with_attributes ("Taskname",
						      renderer,
						      "text", TASK_NAME_COLUMN,
						      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
#endif
   column = gtk_tree_view_column_new_with_attributes ("Description",
						      renderer,
						      "text", TASK_DESCR_COLUMN,
						      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
   

   gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
   
   g_signal_connect(gtk_builder_get_object(_builder, "button_ok"),
                    "clicked",
                    (GCallback) cbButtonOkClicked, this);
   g_signal_connect(gtk_builder_get_object(_builder, "button_cancel"),
                    "clicked",
                    (GCallback) cbButtonCancelClicked, this);
   g_signal_connect(gtk_builder_get_object(_builder, "button_details"),
                    "clicked",
                    (GCallback) cbButtonDetailsClicked, this);

};


