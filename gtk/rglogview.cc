/* rglogview.cc
 *
 * Copyright (c) 2004 Michael Vogt <mvo@debian.org>
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

#include <cassert>
#include <cstring>
#include <map>
#include <apt-pkg/fileutl.h>

#include "config.h"
#include "rglogview.h"
#include "rgutils.h"
#include "rconfiguration.h"

#include "i18n.h"

enum { COLUMN_LOG_DAY, 
       COLUMN_LOG_FILENAME, 
       COLUMN_LOG_TYPE, 
       N_LOG_COLUMNS };

enum { LOG_TYPE_TOPLEVEL, LOG_TYPE_FILE };

void RGLogView::readLogs()
{
   map<int, GtkTreeIter> history_map;
   int history_key;

   GtkTreeStore *store = gtk_tree_store_new(N_LOG_COLUMNS, 
					    G_TYPE_STRING,
					    G_TYPE_STRING,
					    G_TYPE_INT);
   
   GtkTreeIter month_iter;  /* Parent iter */
   GtkTreeIter date_iter;  /* Child iter  */

   unsigned int year, month, day, hour, min, sec;
   char str[128];
   const gchar *logfile;
   GDir *dir = g_dir_open(RLogDir().c_str(), 0, NULL);
   while((logfile=g_dir_read_name(dir)) != NULL) {
      if(sscanf(logfile, "%4u-%2u-%2u.%2u%2u%2u.log", 
		&year, &month, &day, &hour, &min, &sec) != 6)
	 continue;

      struct tm t;
      t.tm_year = year-1900;
      t.tm_mon = month-1;
      t.tm_mday = day;
      t.tm_hour = hour;
      t.tm_min = min;
      t.tm_sec = sec;
      GDate *date = g_date_new_dmy(day, (GDateMonth)month, year);
      // need to convert here:
      // glib: 1=Monday to 7=Sunday 
      // libc: 0=Sunday to 6=Saturday
      t.tm_wday = g_date_get_weekday(date); 
      t.tm_wday %= 7;

      history_key = year*100+month;
      if(history_map.count(history_key) == 0) {
	 gtk_tree_store_append(store, &month_iter, NULL); 
	 strftime(str, 128, "%B %Y", &t);
	 gchar *sort_key = g_strdup_printf("%i", history_key);
	 gtk_tree_store_set (store, &month_iter,
			     COLUMN_LOG_DAY, utf8(str),
			     COLUMN_LOG_FILENAME, sort_key, 
			     COLUMN_LOG_TYPE, LOG_TYPE_TOPLEVEL, 
			     -1);
	 g_free(sort_key);
	 history_map.insert(make_pair<int,GtkTreeIter>(history_key,month_iter));
      } else {
	 month_iter = history_map[history_key];
      }

      strftime(str, 512, "%x %R", &t);
      gtk_tree_store_append (store, &date_iter, &month_iter);
      gtk_tree_store_set (store, &date_iter,
			  COLUMN_LOG_DAY, utf8(str),
			  COLUMN_LOG_FILENAME, logfile, 
			  COLUMN_LOG_TYPE, LOG_TYPE_FILE, 
			  -1);
      g_free(date);
   }
   g_dir_close(dir);

   GtkTreeModel *sort_model;
   /* Create the first tree */
   sort_model = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(store));

   gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort_model),
					 COLUMN_LOG_FILENAME, 
					 GTK_SORT_DESCENDING);
   gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), 
			   GTK_TREE_MODEL(sort_model));
   _realModel = sort_model;
}

void RGLogView::cbCloseClicked(GtkWidget *self, void *data)
{
   RGLogView *me = (RGLogView*)data;
   me->close();
}

void RGLogView::cbTreeSelectionChanged(GtkTreeSelection *selection, 
				      gpointer data)
{
   //cout << "cbTreeSelectionChanged()" << endl;
   RGLogView *me = (RGLogView*)data;
   
   GtkTreeIter iter;
   GtkTreeModel *model;
   GtkTextIter start, end;
   gchar *file = NULL;

   if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
	 GtkTextBuffer *buffer;
	 GtkTextIter start,end;

	 gtk_tree_model_get (model, &iter, COLUMN_LOG_FILENAME, &file, -1);
	 // the months do not have a valid file 
	 if(!FileExists(RLogDir()+string(file)))
	    return;

	 buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(me->_textView));
	 gtk_text_buffer_get_start_iter (buffer, &start);
	 gtk_text_buffer_get_end_iter(buffer,&end);
	 gtk_text_buffer_delete(buffer,&start,&end);
   
	 string logfile = RLogDir() + string(file);
	 ifstream in(logfile.c_str());
	 string s;
	 while(getline(in, s)) {
	    // no need to free str later, it is allocated in a static buffer
	    const char *str = utf8(s.c_str());
	    if(str!=NULL) {
	       gtk_text_buffer_get_end_iter(buffer, &end);
	       int line = gtk_text_iter_get_line(&end);
	       gtk_text_buffer_insert_at_cursor(buffer, str, -1);
	       if(me->findStr) {
		  char *off = g_strstr_len(str, strlen(str), me->findStr);
		  if(off) {
		     gtk_text_buffer_get_iter_at_line_index(buffer, &start, 
							    line, off-str);
		     gtk_text_buffer_get_iter_at_line_index(buffer, &end, 
							    line, off-str+strlen(me->findStr));
		     gtk_text_buffer_apply_tag (buffer, me->_markTag, 
						&start, &end);
		  }
	       } 

	    }
	    gtk_text_buffer_insert_at_cursor(buffer, "\n", -1);
	 }
	 g_free(file);
   }
}

gboolean RGLogView::filter_func(GtkTreeModel *model, GtkTreeIter *iter,
				gpointer data)
{
   RGLogView *me = (RGLogView*)data;
   gchar *file;
   int type;
   string s;

   gtk_tree_model_get(model, iter, 
		      COLUMN_LOG_FILENAME, &file, 
		      COLUMN_LOG_TYPE, &type,
		      -1);

   if(type == LOG_TYPE_TOPLEVEL)
      return TRUE;


   string logfile = RLogDir() + string(file);

   ifstream in(logfile.c_str());
   if(!in) {
      g_warning("can't open logfile: %s",logfile.c_str());
      return FALSE;
   }
   while(getline(in, s)) {
      if(s.find(me->findStr) != string::npos) {
	 return TRUE;
      }
   }

   return FALSE;
}

gboolean empty_row_filter_func(GtkTreeModel *model, GtkTreeIter *iter, 
			       gpointer data)
{
   int type;

   gtk_tree_model_get(model, iter, COLUMN_LOG_TYPE, &type, -1);
   // top-level expander
   if(type == LOG_TYPE_TOPLEVEL) {
      return gtk_tree_model_iter_has_child(model, iter);
   }

   return TRUE;
}

void RGLogView::clearLogBuf()
{
   GtkTextBuffer *buffer;
   GtkTextIter start,end;

   buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(_textView));
   gtk_text_buffer_get_start_iter (buffer, &start);
   gtk_text_buffer_get_end_iter(buffer,&end);
   gtk_text_buffer_delete(buffer,&start,&end);

}

void RGLogView::appendLogBuf(string text)
{
   GtkTextBuffer *buffer;
   buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(_textView));
   gtk_text_buffer_insert_at_cursor(buffer, text.c_str(), -1);
}

void RGLogView::cbButtonFind(GtkWidget *self, void *data)
{
   //cout << "RGLogView::cbButtonFind()" << endl;
   RGLogView *me = (RGLogView*)data;
   GtkTreeModel *filter_model, *empty_row_filter;
   GtkTreeIter iter; 
   gchar *file;

   GtkTreeModel *model = me->_realModel;
   if(model == NULL) {
      g_warning("model==NULL in cbButtonFind");
      return;
   }

   me->clearLogBuf();

   me->findStr = gtk_entry_get_text(GTK_ENTRY(me->_entryFind));
   // reset to old model
   if(strlen(me->findStr) == 0) {
      me->findStr = NULL;
      gtk_tree_view_set_model(GTK_TREE_VIEW(me->_treeView), me->_realModel);
      return;
   } 
     
   // filter for the search string
   filter_model=(GtkTreeModel*)gtk_tree_model_filter_new(model, NULL);
   gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter_model), 
					  me->filter_func, me, NULL);

   // filter out empty nodes
   empty_row_filter=(GtkTreeModel*)gtk_tree_model_filter_new(filter_model, NULL);
   gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(empty_row_filter), 
					  empty_row_filter_func, me, NULL);
   gtk_tree_view_set_model(GTK_TREE_VIEW(me->_treeView), empty_row_filter);


   int toplevel_children = gtk_tree_model_iter_n_children(empty_row_filter, NULL);
   if(toplevel_children == 0) {
      me->appendLogBuf(_("Not found"));
   } else {
      me->appendLogBuf(_("Expression was found, please see the list "
			 "on the left for matching entries."));
   }

   // expand to show what we found
   gtk_tree_view_expand_all(GTK_TREE_VIEW(me->_treeView));
}

void RGLogView::show()
{
   clearLogBuf();
   gtk_entry_set_text(GTK_ENTRY(_entryFind), "");
   RGWindow::show();
}

RGLogView::RGLogView(RGWindow *parent)
   : RGGtkBuilderWindow(parent, "logview"), findStr(NULL)
{
   GtkWidget *vbox = GTK_WIDGET(gtk_builder_get_object(_builder, "vbox_main"));
   assert(vbox);

   _entryFind = GTK_WIDGET(gtk_builder_get_object(_builder, "entry_find"));
   assert(_entryFind);

   _treeView = GTK_WIDGET(gtk_builder_get_object(_builder, "treeview_dates"));
   assert(_treeView);
   
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Date",
						      renderer,
						      "markup", COLUMN_LOG_DAY,
						      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW(_treeView), column);

   // find button
   g_signal_connect(gtk_builder_get_object(_builder, "button_find"),
                    "clicked",
                    G_CALLBACK(cbButtonFind), this);
   // close
   g_signal_connect(gtk_builder_get_object(_builder, "button_close"),
                    "clicked",
                    G_CALLBACK(cbCloseClicked), this);

 
   /* Setup the selection handler */
   GtkTreeSelection *select;
   select = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
   gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
   g_signal_connect(G_OBJECT(select), "changed",
		    G_CALLBACK (cbTreeSelectionChanged),
		    this);
   _textView = GTK_WIDGET(gtk_builder_get_object(_builder, "textview_log"));
   assert(_textView);

   g_signal_connect(gtk_builder_get_object(_builder, "entry_find"),
                    "activate",
                    G_CALLBACK(cbButtonFind), this);

   GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(_textView));
   _markTag = gtk_text_buffer_create_tag (buf, "mark",
					  "background", "yellow", NULL); 

}
