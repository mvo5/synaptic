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

#include "config.h"

#include <apt-pkg/fileutl.h>

#include "rgwindow.h"
#include "rgutils.h"

gboolean RGWindow::windowCloseCallback(GtkWidget *window, gpointer user_data)
{
   //cout << "windowCloseCallback" << endl;
   RGWindow *rwin = (RGWindow *) user_data;
   start_task([rwin]() -> task<void> {
      bool _close = co_await rwin->close();
   });
   return true;
}

RGWindow::RGWindow(string name, bool makeBox)
{
   //std::cout << "RGWindow::RGWindow(string name, bool makeBox)" << endl;
   _win = gtk_window_new();
   gtk_window_set_title(GTK_WINDOW(_win), (char *)name.c_str());
   gtk_window_set_icon_name(GTK_WINDOW(_win), "synaptic");

   g_object_set_data(G_OBJECT(_win), "me", this);
   g_signal_connect(G_OBJECT(_win), "close-request",
                    G_CALLBACK(windowCloseCallback), this);

   if (makeBox) {
      _topBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      gtk_window_set_child(GTK_WINDOW(_win), _topBox);
      gtk_widget_set_margin_top(_topBox, 5);
      gtk_widget_set_margin_bottom(_topBox, 5);
      gtk_widget_set_margin_start(_topBox, 5);
      gtk_widget_set_margin_end(_topBox, 5);
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
   _win = gtk_window_new();
   gtk_window_set_title(GTK_WINDOW(_win), (char *)name.c_str());

   g_object_set_data(G_OBJECT(_win), "me", this);

   g_signal_connect(G_OBJECT(_win), "close-request",
                    G_CALLBACK(windowCloseCallback), this);

   if (makeBox) {
      _topBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      gtk_window_set_child(GTK_WINDOW(_win), _topBox);
      gtk_widget_set_margin_top(_topBox, 5);
      gtk_widget_set_margin_bottom(_topBox, 5);
      gtk_widget_set_margin_start(_topBox, 5);
      gtk_widget_set_margin_end(_topBox, 5);
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
   gtk_window_destroy(GTK_WINDOW(_win));
}


void RGWindow::setTitle(string title)
{
   gtk_window_set_title(GTK_WINDOW(_win), (char *)title.c_str());
}

task<bool> RGWindow::close()
{
   //cout << "RGWindow::close()" << endl;
   hide();
   co_return true;
}

// vim:ts=3:sw=3:et
