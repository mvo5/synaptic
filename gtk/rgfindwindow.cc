/* rgfindwindow.cc
 *
 * Copyright (c) 2003 Michael Vogt
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

#include "config.h"

#include <cassert>

#include "rgfindwindow.h"
#include "rgmisc.h"

#include "i18n.h"

void RGFindWindow::show()
{
   //cout << "void RGFindWindow::show()" << endl;

   gdk_window_set_cursor(_win->window, NULL);
   RGWindow::show();
}

string RGFindWindow::getFindString()
{
   GtkWidget *entry;
   string s;

   entry = glade_xml_get_widget(_gladeXML, "entry_find");
   const char *text = gtk_entry_get_text(GTK_ENTRY(entry));

   return text;
}

int RGFindWindow::getSearchType()
{
   int searchType;
   GtkWidget *omenu = glade_xml_get_widget(_gladeXML, "optionmenu_where");
   assert(omenu);
   searchType = gtk_option_menu_get_history(GTK_OPTION_MENU(omenu));

   return searchType;
}


void RGFindWindow::doFind(GtkWindow *widget, void *data)
{
   //cout << "RGFindWindow::doFind()"<<endl;
   RGFindWindow *me = (RGFindWindow *) data;

   GtkWidget *combo = glade_xml_get_widget(me->_gladeXML, "combo_find");
   const gchar *str = gtk_entry_get_text(GTK_ENTRY(me->_entry));

   if(strlen(str) == 0)
      return;

   me->_prevSearches = g_list_prepend(me->_prevSearches, g_strdup(str));
   gtk_combo_set_popdown_strings(GTK_COMBO(combo), me->_prevSearches);

   doClose(widget, data);
   gtk_dialog_response(GTK_DIALOG(((RGFindWindow *) data)->window()),
                       GTK_RESPONSE_OK);
}

void RGFindWindow::doClose(GtkWindow *widget, void *data)
{
   //cout << "void RGFindWindow::doClose()" << endl;  
   RGFindWindow *w = (RGFindWindow *) data;

   w->hide();
}

void RGFindWindow::cbEntryChanged(GtkWindow *widget, void *data)
{
   RGFindWindow *me = (RGFindWindow *) data;

   if( strlen(gtk_entry_get_text(GTK_ENTRY(me->_entry))) >0 )
      gtk_widget_set_sensitive(me->_findB, TRUE);
   else
      gtk_widget_set_sensitive(me->_findB, FALSE);
}


RGFindWindow::RGFindWindow(RGWindow *win)
: RGGladeWindow(win, "find"), _prevSearches(0)
{
   //cout << " RGFindWindow::RGFindWindow(RGWindow *win) "<< endl;

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_button_find_clicked",
                                 G_CALLBACK(doFind), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_entry_find_activate",
                                 G_CALLBACK(doFind), this);

   glade_xml_signal_connect_data(_gladeXML,
                                 "on_button_close_clicked",
                                 G_CALLBACK(doClose), this);

   GtkWidget *combo = glade_xml_get_widget(_gladeXML, "combo_find");
   gtk_combo_set_value_in_list(GTK_COMBO(combo), FALSE, FALSE);

   _entry = glade_xml_get_widget(_gladeXML, "entry_find");
   assert(_entry);
   g_signal_connect(G_OBJECT(_entry), "changed", 
		   G_CALLBACK(cbEntryChanged), this);
   _findB = glade_xml_get_widget(_gladeXML, "button_find");
   assert(_findB);
   gtk_widget_set_sensitive(_findB, FALSE);

   setTitle(_("Find"));
   skipTaskbar(true);
}
