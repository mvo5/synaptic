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
#include "i18n.h"

#include "rgfindwindow.h"

void RGFindWindow::show()
{
  //cout << "void RGFindWindow::show()" << endl;

  RGWindow::show();
}

string RGFindWindow::getFindString()
{
  GtkWidget *entry;
  string s;
  
  entry = glade_xml_get_widget(_gladeXML, "entry_find");
  s = gtk_entry_get_text(GTK_ENTRY(entry));
  
  return s;
}

RPatternPackageFilter::DepType RGFindWindow::getSearchType()
{
  return _searchType;
}

void RGFindWindow::setSearchType(RPatternPackageFilter::DepType type) 
{
  _searchType = type;

  switch(type) {
  case RPatternPackageFilter::Name:
    setTitle(_("Find Name"));
    break;
  case RPatternPackageFilter::Description:
    setTitle(_("Find Description"));
    break;
  }
}

void RGFindWindow::doFind(GtkWindow *widget, void *data)
{
  //cout << "RGFindWindow::doFind()"<<endl;
  RGFindWindow *me = (RGFindWindow*)data;

  GtkWidget *combo = glade_xml_get_widget(me->_gladeXML, "combo_find");
  GtkWidget *entry = glade_xml_get_widget(me->_gladeXML, "entry_find");
  const gchar *str = gtk_entry_get_text(GTK_ENTRY(entry));
  
  me->_prevSearches = g_list_prepend(me->_prevSearches,g_strdup(str));
  gtk_combo_set_popdown_strings (GTK_COMBO(combo), me->_prevSearches);
  
  me->_findAction(me->_findObject, me);
}

void RGFindWindow::setFindCallback(RGFindWindowFindAction *action, 
				   void *data)
{
    _findAction = action;
    _findObject = data;
}


void RGFindWindow::doClose(GtkWindow *widget, void *data)
{
  //cout << "void RGFindWindow::doClose()" << endl;  
  RGFindWindow *w = (RGFindWindow*)data;

  w->hide();
}

RGFindWindow::RGFindWindow(RGWindow *win) 
    : RGGladeWindow(win, "find")
{
  //cout << " RGFindWindow::RGFindWindow(RGWindow *win) "<< endl;
 
  _prevSearches = NULL;

  glade_xml_signal_connect_data(_gladeXML,
				"on_button_find_clicked",
				G_CALLBACK(doFind),
				this); 

  glade_xml_signal_connect_data(_gladeXML,
				"on_entry_find_activate",
				G_CALLBACK(doFind),
				this); 

  glade_xml_signal_connect_data(_gladeXML,
				"on_button_close_clicked",
				G_CALLBACK(doClose),
				this); 

  GtkWidget *combo = glade_xml_get_widget(_gladeXML, "combo_find");
  gtk_combo_set_value_in_list(GTK_COMBO(combo),FALSE, FALSE);
  
  setTitle(_("Find tool"));
}


