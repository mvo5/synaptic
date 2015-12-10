/* rgwindow.cc
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
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


#include <apt-pkg/fileutl.h>
#include "config.h"
#include "i18n.h"
#include "rgwindow.h"
#include "rgutils.h"

bool RGWindow::windowCloseCallback(GtkWidget *window, GdkEvent * event)
{
   //cout << "windowCloseCallback" << endl;
   RGWindow *rwin = (RGWindow *) g_object_get_data(G_OBJECT(window), "me");

   return rwin->close();
}

RGWindow::RGWindow(string name, bool makeBox)
{
   //std::cout << "RGWindow::RGWindow(string name, bool makeBox)" << endl;
   _win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(_win), (char *)name.c_str());
   GdkPixbuf *icon = get_gdk_pixbuf( "synaptic" );
   gtk_window_set_icon(GTK_WINDOW(_win), icon);

   g_object_set_data(G_OBJECT(_win), "me", this);
   g_signal_connect(G_OBJECT(_win), "delete-event",
                    G_CALLBACK(windowCloseCallback), this);

   if (makeBox) {
      _topBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      gtk_container_add(GTK_CONTAINER(_win), _topBox);
      gtk_widget_show(_topBox);
      gtk_container_set_border_width(GTK_CONTAINER(_topBox), 5);
   } else {
      _topBox = NULL;
   }

   //gtk_widget_realize(_win);
   //gtk_widget_show_all(_win);
}


RGWindow::RGWindow(RGWindow *parent, string name, bool makeBox, bool closable)
{
   //std::cout 
   //<< "RGWindow::RGWindow(RGWindow *parent, string name, bool makeBox,  bool closable)"
   //<< endl;
   _win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(_win), (char *)name.c_str());

   g_object_set_data(G_OBJECT(_win), "me", this);

   g_signal_connect(G_OBJECT(_win), "delete-event",
                    G_CALLBACK(windowCloseCallback), this);

   if (makeBox) {
      _topBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      gtk_container_add(GTK_CONTAINER(_win), _topBox);
      gtk_widget_show(_topBox);
      gtk_container_set_border_width(GTK_CONTAINER(_topBox), 5);
   } else {
      _topBox = NULL;
   }

   //gtk_widget_realize(_win);

   gtk_window_set_transient_for(GTK_WINDOW(_win),
                                GTK_WINDOW(parent->window()));
}


RGWindow::~RGWindow()
{
   //cout << "~RGWindow"<<endl;
   gtk_widget_destroy(_win);
}


void RGWindow::setTitle(string title)
{
   gtk_window_set_title(GTK_WINDOW(_win), (char *)title.c_str());
}

bool RGWindow::close()
{
   //cout << "RGWindow::close()" << endl;
   hide();
   return true;
}

// vim:ts=3:sw=3:et
