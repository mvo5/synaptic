/* rgfilterwindow.h
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

#ifndef _RGFILTERWINDOW_H_
#define _RGFILTERWINDOW_H_

#include <vector>
#include <string>

#include "rgwindow.h"

#include "rpackagefilter.h"

struct RFilter;
class RPackageLister;
class RGFilterEditor;

class RGFilterWindow;

typedef void RGFilterWindowCloseAction(void *self, RGFilterWindow *win);

typedef void RGFilterWindowSaveAction(void *self, RGFilterWindow *win);

class RGFilterWindow : public RGWindow
{
private:

   RPackageLister *_lister;
   RGFilterEditor *_editor;
   
   static void saveFilterAction(GtkWidget *win, void *data);
   static void clearFilterAction(GtkWidget *self, void *data);
   static void windowCloseAction(GtkWidget *win, void *data);
   static gint windowDeleteAction(GtkWidget *win, GdkEvent  *event,
				  gpointer   data );

   RGFilterWindowCloseAction *_closeAction;
   void *_closeObject;
   RGFilterWindowSaveAction *_saveAction;
   void *_saveObject;
   
public:
   RGFilterWindow(RGWindow *win, RPackageLister *lister);
   virtual ~RGFilterWindow();

   void editFilter(RFilter *filter);

   void setCloseCallback(RGFilterWindowCloseAction *action, void *data);
   void setSaveCallback(RGFilterWindowSaveAction *action, void *data);

   virtual void close();
};


#endif
