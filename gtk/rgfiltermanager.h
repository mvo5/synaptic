/* rgfiltermanager.h
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

#ifndef _RWFILTERMANAGER_H_
#define _RWFILTERMANAGER_H_

#include <gtk/gtk.h>

#include "rgwindow.h"



class RPackageLister;

class RGFilterEditor;

class RGFilterManagerWindow;


typedef void RGFilterEditorCloseAction(void *self, RGFilterManagerWindow *win);


class RGFilterManagerWindow : public RGWindow 
{
   static void newFilterAction(GtkWidget *self, void *data);
   static void deleteFilterAction(GtkWidget *self, void *data);
   static void closeAction(GtkWidget *self, void *data);
   static gint deleteEventAction(GtkWidget *widget, GdkEvent  *event,
				 gpointer   data );
   static void applyFilterAction(GtkWidget *self, void *data);
   static void clearFilterAction(GtkWidget *self, void *data);
   //static void editFilterAction(GtkWidget *self, void *data);   
   static void editFilterAction(GtkWidget *self, void *data);   
   static void selectAction(GtkWidget *self,gint row, gint column, 
 			    GdkEventButton *event, gpointer user_data); 


   RGFilterEditorCloseAction *_closeAction;
   void *_closeData;

   GtkWidget *_newB;    /* GtkButton */
   GtkWidget *_filterL; /* GtkCList */
   int selected_row;    /* the currently selected row in GtkCList */
   GtkWidget *_nameT;   /* GtkEntry */

   RPackageLister *_lister;
   RGFilterEditor *_editor;
   
public:
   RGFilterManagerWindow(RGWindow *win, RPackageLister *lister);
   virtual ~RGFilterManagerWindow();

   void setCloseCallback(RGFilterEditorCloseAction *action, void *data);
   virtual void close();
   virtual void show();
  
};


#endif

