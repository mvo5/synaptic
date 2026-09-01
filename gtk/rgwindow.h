/* rgwindow.h
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2002 Michael Vogt <mvo@debian.org>
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <mvo@debian.org>
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

#pragma once

#include "config.h" // IWYU pragma: associated

#include <gtk/gtk.h>
#include <string>

class RGWindow
{
 protected:
   GtkWidget *_win;

   static gboolean windowCloseCallback(GtkWidget *widget,
                                       GdkEvent *event,
                                       gpointer data);
   virtual void close();

   void init();

   RGWindow() : _win(nullptr)
   {}

 public:
   inline virtual GtkWidget *window()
   {
      return _win;
   }

   virtual void setTitle(std::string title);

   inline virtual void hide()
   {
      gtk_widget_hide(_win);
   }

   inline virtual void show()
   {
      gtk_widget_show(_win);
   }

   explicit RGWindow(RGWindow *parent, std::string name);
   virtual ~RGWindow();
};
