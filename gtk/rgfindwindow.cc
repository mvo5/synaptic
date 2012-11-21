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
#include <cstring>

#include "rgfindwindow.h"
#include "rgutils.h"
#include "gtk3compat.h"
#include "i18n.h"


gchar* RGFindWindow::getFindString()
{
   return gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(_comboFind));
}

void RGFindWindow::selectText()
{
   /* no need to do something hugely special here, on focus the last text is
    * selected automatically
    */
   gtk_widget_grab_focus(_comboFind);
}

int RGFindWindow::getSearchType()
{
   int searchType;
   searchType = gtk_combo_box_get_active(GTK_COMBO_BOX(_comboSearchType));

   return searchType;
}


void RGFindWindow::doFind(GtkWindow *widget, void *data)
{
   //cout << "RGFindWindow::doFind()"<<endl;
   RGFindWindow *me = (RGFindWindow *) data;
   GtkListStore *prevSearchStore = GTK_LIST_STORE( gtk_combo_box_get_model(
                      GTK_COMBO_BOX(me->_comboFind)));
   GtkTreeIter searchIter;

   const gchar *str = me->getFindString();
   if(strlen(str) == 0)
      return;

   me->_prevSearches = g_list_prepend(me->_prevSearches, g_strdup(str));
   gtk_list_store_prepend(prevSearchStore, &searchIter);
   gtk_list_store_set(prevSearchStore, &searchIter, 0, str, -1);

   doClose(widget, data);
   gtk_dialog_response(GTK_DIALOG(((RGFindWindow *) data)->window()),
                       GTK_RESPONSE_OK);
}

void RGFindWindow::doClose(GtkWindow *widget, void *data)
{
   //cout << "void RGFindWindow::doClose()" << endl;  
   RGFindWindow *me = (RGFindWindow *) data;

   _config->Set("Synaptic::LastSearchType",
		gtk_combo_box_get_active(GTK_COMBO_BOX(me->_comboSearchType)));

   me->hide();
}

void RGFindWindow::cbEntryChanged(GtkWindow *widget, void *data)
{
   RGFindWindow *me = (RGFindWindow *) data;

   if ( strlen(me->getFindString()) > 0 )
      gtk_widget_set_sensitive(me->_findB, TRUE);
   else
      gtk_widget_set_sensitive(me->_findB, FALSE);
}


RGFindWindow::RGFindWindow(RGWindow *win)
: RGGtkBuilderWindow(win, "find"), _prevSearches(0)
{
   GtkListStore *comboStore;
   GtkTreeIter comboIter;
   GtkCellRenderer *crt;
   //cout << " RGFindWindow::RGFindWindow(RGWindow *win) "<< endl;

   g_signal_connect(gtk_builder_get_object(_builder,
                                           "button_close"),
                               "clicked",
                               G_CALLBACK(doClose), this);

   _comboFind = GTK_WIDGET(gtk_builder_get_object(_builder, "comboentry_search"));
   assert(_comboFind);
   g_signal_connect(G_OBJECT(_comboFind), "changed", 
		   G_CALLBACK(cbEntryChanged), this);
   crt = gtk_cell_renderer_text_new();
   gtk_cell_layout_clear(GTK_CELL_LAYOUT(_comboFind));
   gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_comboFind), crt, TRUE);
   gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_comboFind),
                                          crt, "text", 0);

   _comboSearchType = GTK_WIDGET(gtk_builder_get_object(_builder, "combo_lookin"));
   assert(_comboSearchType);
   comboStore = GTK_LIST_STORE(
                   gtk_combo_box_get_model(
                      GTK_COMBO_BOX(_comboSearchType)));
   for (int i = 0; SearchTypes[i] != NULL; i++) {
      gtk_list_store_append(comboStore, &comboIter);
      gtk_list_store_set(comboStore, &comboIter, 0, _(SearchTypes[i]), -1);
   }
   crt = gtk_cell_renderer_text_new();
   gtk_cell_layout_clear(GTK_CELL_LAYOUT(_comboSearchType));
   gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_comboSearchType), crt, TRUE);
   gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_comboSearchType),
                                          crt, "text", 0);
   gtk_combo_box_set_active(GTK_COMBO_BOX(_comboSearchType),
			       _config->FindI("Synaptic::LastSearchType",1));

   _findB = GTK_WIDGET(gtk_builder_get_object(_builder, "button_find"));
   assert(_findB);
   g_signal_connect(G_OBJECT(_findB),
                    "clicked",
                    G_CALLBACK(doFind), this);
   gtk_widget_set_sensitive(_findB, FALSE);

   setTitle(_("Find"));
   skipTaskbar(true);
}
