/* rgchangelogdialog.cc - main window of application
 * 
 * Copyright (c) 2010 Michael Vogt <mvo@ubuntu.com>
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

#include "rgchangelogdialog.h"

void ShowChangelogDialog(RGWindow *me, RPackage *pkg)
{
   RGFetchProgress *status = new RGFetchProgress(me);;
   status->setDescription(_("Downloading Changelog"),
			  _("The changelog contains information about the"
             " changes and closed bugs in each version of"
             " the package."));
   pkgAcquire fetcher(status);
   string filename = pkg->getChangelogFile(&fetcher);
   
   RGGtkBuilderUserDialog dia(me,"changelog");

   // set title
   GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(dia.getGtkBuilder(), 
					              "dialog_changelog"));
   assert(win);
   // TRANSLATORS: Title of the changelog dialog - %s is the name of the package
   gchar *str = g_strdup_printf(_("%s Changelog"), pkg->name());
   gtk_window_set_title(GTK_WINDOW(win), str);
   g_free(str);


   // set changelog data
   GtkWidget *textview = GTK_WIDGET(gtk_builder_get_object
                                    (dia.getGtkBuilder(),
                                     "textview_changelog"));
   assert(textview);
   GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
   GtkTextIter start,end;
   gtk_text_buffer_get_start_iter (buffer, &start);
   gtk_text_buffer_get_end_iter(buffer,&end);
   gtk_text_buffer_delete(buffer,&start,&end);
   
   ifstream in(filename.c_str());
   string s;
   while(getline(in, s)) {
      // no need to free str later, it is allocated in a static buffer
      const char *str = utf8(s.c_str());
      if(str!=NULL)
	 gtk_text_buffer_insert_at_cursor(buffer, str, -1);
      gtk_text_buffer_insert_at_cursor(buffer, "\n", -1);
   }
   
   dia.run();

   // clean up
   delete status;
   unlink(filename.c_str());
}
