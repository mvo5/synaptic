/* rgsetoptwindow.h
 *
 * Copyright (c) 2003 Michael Vogt
 *
 * Author: Gustavo Niemeyer <niemeyer@conectiva.com>
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

#include "rggtkbuilderwindow.h"

class RGSetOptWindow:public RGGtkBuilderWindow {

   static void DoApply(GtkWindow *widget, void *data);
   static void DoClose(GtkWindow *widget, void *data);

 public:

   RGSetOptWindow(RGWindow *owner);
   virtual ~ RGSetOptWindow() {
   };

};
