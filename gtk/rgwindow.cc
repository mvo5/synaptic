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

#include "config.h" // IWYU pragma: associated

#include "rgwindow.h"

#include "rgutils.h"

#include <gtk/gtk.h>
#include <string>

gboolean RGWindow::windowCloseCallback(GtkWidget *window,
                                       GdkEvent *event,
                                       gpointer data)
{
   ((RGWindow *)data)->close();
   return TRUE;
}

void RGWindow::init()
{
   g_object_set_data(G_OBJECT(_win), "me", this);

   g_signal_connect(
      G_OBJECT(_win), "delete-event", G_CALLBACK(windowCloseCallback), this);
}


RGWindow::RGWindow(RGWindow *parent, std::string name)
{
   _win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(_win), (char *)name.c_str());

   init();

   gtk_window_set_transient_for(GTK_WINDOW(_win), GTK_WINDOW(parent->window()));
}

RGWindow::~RGWindow()
{
   gtk_widget_destroy(_win);
}

void RGWindow::setTitle(std::string title)
{
   gtk_window_set_title(GTK_WINDOW(_win), (char *)title.c_str());
}

void RGWindow::close()
{
   hide();
}
