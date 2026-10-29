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
 private:

   GtkWidget *const _win;

 protected:

   static gboolean windowCloseCallback(GtkWidget *window, GdkEvent *event, RGWindow *self);

   inline virtual void close()
   {
      hide();
   }

   // Constructor called by classes which create their own window object.
   RGWindow() : _win(nullptr)
   {}

 public:

   explicit RGWindow(RGWindow *parent, const std::string &name);

   virtual ~RGWindow()
   {
      if (_win != nullptr) gtk_widget_destroy(_win);
   }

   inline virtual GtkWidget *window() const
   {
      return _win;
   }

   inline virtual void setTitle(const std::string &title)
   {
      gtk_window_set_title(GTK_WINDOW(window()), title.c_str());
   }

   inline virtual void hide()
   {
      gtk_widget_hide(window());
   }

   inline virtual void show()
   {
      gtk_widget_show(window());
   }
};
