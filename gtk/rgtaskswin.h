/* rgtaskswin.h
 *
 * Copyright (c) 2004 Michael Vogt
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


#ifndef _RGTASKSWIN_H_
#define _RGTASKSWIN_H_

#include "rggtkbuilderwindow.h"

class RGMainWindow;

class RGTasksWin : public RGGtkBuilderWindow {
 protected:
   RGMainWindow *_mainWin;
   GtkListStore *_store;
   GtkWidget *_taskView;
   GtkWidget *_detailsButton;

   static void cbButtonOkClicked(GtkWidget *self, void *data);
   static void cbButtonCancelClicked(GtkWidget *self, void *data);
   static void cbButtonDetailsClicked(GtkWidget *self, void *data);
   static void cell_toggled_callback (GtkCellRendererToggle *cell,
				      gchar *path_string,
				      gpointer user_data);
   static void selection_changed_callback(GtkTreeSelection *selection,
				    gpointer user_data);



 public:
   RGTasksWin(RGWindow *parent);
   virtual ~ RGTasksWin() {
   };
};



#endif
