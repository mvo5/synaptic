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


#ifndef _RGWINDOW_H_
#define _RGWINDOW_H_

#include <gtk/gtk.h>
#include <string>

using namespace std;

class RGWindow {
 protected:
   GtkWidget *_win;
   GtkWidget *_topBox;

   static bool windowCloseCallback(GtkWidget *widget, GdkEvent * event);
   virtual bool close();

 public:
   inline virtual GtkWidget *window() {
      return _win;
   };

   virtual void setTitle(string title);

   inline virtual void hide() {
      gtk_widget_hide(_win);
   };
   inline virtual void show() {
      gtk_widget_show(_win);
   };

   RGWindow() : _win(0), _topBox(0) {};
   RGWindow(string name, bool makeBox = true);
   RGWindow(RGWindow *parent, string name, bool makeBox = true,
            bool closable = true);
   virtual ~RGWindow();
};

#endif
