/* rwfiltermanager.h
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

#ifndef _RWFILTERMANAGER_H_
#define _RWFILTERMANAGER_H_

#include <WINGs/WINGs.h>

#include "rwwindow.h"



class RPackageLister;

class RWFilterEditor;

class RWFilterManagerWindow;


typedef void RWFilterEditorCloseAction(void *self, RWFilterManagerWindow *win);


class RWFilterManagerWindow : public RWWindow 
{
   static void newFilterAction(WMWidget *self, void *data);
   static void editFilterAction(WMWidget *self, void *data);   
   static void deleteFilterAction(WMWidget *self, void *data);
   static void closeAction(WMWidget *self, void *data);
   static void applyFilterAction(WMWidget *self, void *data);
   static void clearFilterAction(WMWidget *self, void *data);

   RWFilterEditorCloseAction *_closeAction;
   void *_closeData;

   WMButton *_newB;
   
   WMList *_filterL;
   WMTextField *_nameT;

   RPackageLister *_lister;
   
   RWFilterEditor *_editor;
   
public:
   RWFilterManagerWindow(RWWindow *win, RPackageLister *lister);
   virtual ~RWFilterManagerWindow();

   void setCloseCallback(RWFilterEditorCloseAction *action, void *data);
   
   virtual void show();
   
};


#endif

