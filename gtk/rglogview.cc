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
#include "config.h"
#include "rglogview.h"
#include "rgmisc.h"

#include "i18n.h"

enum { COLUMN_LOG_DAY, 
       COLUMN_LOG_FILENAME, 
       N_LOG_COLUMNS };

void RGLogView::readLogs()
{
   GtkTreeStore *store = gtk_tree_store_new(N_LOG_COLUMNS, 
					    G_TYPE_STRING,
					    G_TYPE_STRING);
   
   GtkTreeIter month_iter;  /* Parent iter */
   GtkTreeIter date_iter;  /* Child iter  */

   unsigned int year, month, day, hour, min, sec;
   unsigned int last_year, last_month;
   char str[512];
   const gchar *logfile;
   const gchar *logdir = RLogDir().c_str();
   GDir *dir = g_dir_open(logdir, 0, NULL);
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
      t.tm_wday = g_date_get_weekday(date);

      if(year != last_year || month != last_month) {
	 gtk_tree_store_append(store, &month_iter, NULL); 
	 strftime(str, 512, "%B %Y", &t);
	 gtk_tree_store_set (store, &month_iter,
			     COLUMN_LOG_DAY, str, 
			     COLUMN_LOG_FILENAME, NULL, 
			     -1);
	 last_year = year;
	 last_month = month;
      }

      strftime(str, 512, "%c", &t);
      gtk_tree_store_append (store, &date_iter, &month_iter);
      gtk_tree_store_set (store, &date_iter,
			  COLUMN_LOG_DAY, str, 
			  COLUMN_LOG_FILENAME, logfile, 
			  -1);
      g_free(date);
   }
   g_dir_close(dir);

   GtkTreeModel *sort_model;
   /* Create the first tree */
   sort_model = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(store));

   gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort_model),
					 COLUMN_LOG_FILENAME, 
					 GTK_SORT_ASCENDING);
   gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), 
			   GTK_TREE_MODEL(sort_model));
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
   gchar *file = NULL;

   if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
	 GtkTextBuffer *buffer;
	 GtkTextIter start,end;

	 gtk_tree_model_get (model, &iter, COLUMN_LOG_FILENAME, &file, -1);
	 if(file == NULL)
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
	    if(str!=NULL)
	       gtk_text_buffer_insert_at_cursor(buffer, str, -1);
	    gtk_text_buffer_insert_at_cursor(buffer, "\n", -1);
	 }
	 g_free(file);
   }
}

RGLogView::RGLogView(RGWindow *parent)
: RGGladeWindow(parent, "logview")
{
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_button_close_clicked",
                                 G_CALLBACK(cbCloseClicked), this);
   GtkWidget *vbox = glade_xml_get_widget(_gladeXML, "vbox_main");
   assert(vbox);

   _treeView = glade_xml_get_widget(_gladeXML, "treeview_dates");
   assert(_treeView);
   
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Date",
						      renderer,
						      "text", COLUMN_LOG_DAY,
						      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW(_treeView), column);


 
   /* Setup the selection handler */
   GtkTreeSelection *select;
   select = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
   gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
   g_signal_connect(G_OBJECT(select), "changed",
		    G_CALLBACK (cbTreeSelectionChanged),
		    this);
   _textView = glade_xml_get_widget(_gladeXML, "textview_log");
   assert(_textView);
   
}
