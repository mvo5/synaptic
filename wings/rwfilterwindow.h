/* rwfilterwindow.h
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

#ifndef _RWFILTERWINDOW_H_
#define _RWFILTERWINDOW_H_

#include <vector>
#include <string>

#include <WINGs/WINGs.h>
#include <WINGs/wtableview.h>

#include "rwwindow.h"

#include "rpackagefilter.h"

struct RFilter;
class RPackageLister;
class RWFilterEditor;

class RWFilterWindow;

typedef void RWFilterWindowCloseAction(void *self, RWFilterWindow *win);

typedef void RWFilterWindowSaveAction(void *self, RWFilterWindow *win);

class RWFilterWindow : public RWWindow
{
private:

   RPackageLister *_lister;
   RWFilterEditor *_editor;
   
   static void saveFilterAction(WMWidget *win, void *data);
   static void clearFilterAction(WMWidget *self, void *data);
   static void windowCloseAction(WMWidget *win, void *data);

   RWFilterWindowCloseAction *_closeAction;
   void *_closeObject;
   RWFilterWindowSaveAction *_saveAction;
   void *_saveObject;
   
public:
   RWFilterWindow(RWWindow *win, RPackageLister *lister);
   virtual ~RWFilterWindow();

   void editFilter(RFilter *filter);
   
   void setCloseCallback(RWFilterWindowCloseAction *action, void *data);
   void setSaveCallback(RWFilterWindowSaveAction *action, void *data);
};


#endif
