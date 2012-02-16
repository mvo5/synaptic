/* rggtkbuilderwindow.h
 *
 * Copyright (c) 2003 Michael Vogt <mvo@debian.org>
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


#ifndef _RGGTKBUILDERWINDOW_H_
#define _RGGTKBUILDERWINDOW_H_

#include "config.h"

#include <gtk/gtk.h>
#include <string>
#include <iostream>
#include <vector>

#include "rgwindow.h"
#include "rgutils.h"

using namespace std;

class RGGtkBuilderWindow:public RGWindow {
 protected:
   GtkBuilder *_builder;
   GdkCursor *_busyCursor;

 public:
   RGGtkBuilderWindow(RGWindow *parent, string name, string main_widget = "");

   void skipTaskbar(bool value) {
      gtk_window_set_skip_taskbar_hint(GTK_WINDOW(_win), value);
   }

   // show busy cursor over main window
   void setBusyCursor(bool flag=true);

   // functions to set various widgets
   bool setLabel(const char *name, const char *value);
   bool setMarkup(const char *widget_name, const char *value);
   bool setLabel(const char *name, const long value);
   bool setTextView(const char *widget_name, const char *value,
		    bool useHeadline=false);
   bool setPixmap(const char *widget_name, GdkPixbuf *value);
   bool setTreeList(const char *widget_name, vector<string> values,
		    bool useMarkup=false);

   GtkBuilder* getGtkBuilder() {return _builder;};
};

#endif
