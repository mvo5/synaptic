/* rwindow.h
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

#ifndef _RWWINDOW_H_
#define _RWWINDOW_H_


#include <WINGs/WINGs.h>

#include <string>

using namespace std;

class RWWindow {   
protected:
   WMWindow *_win;
   WMBox *_topBox;
   
   static void windowCloseAction(WMWidget *win, void *data);
   virtual void close();
   
public:
   inline virtual WMWindow *window() { return _win; };
   
   virtual void setTitle(string title);
   
   inline virtual void hide() { WMCloseWindow(_win); };
   inline virtual void show() { WMMapWidget(_win); };

   RWWindow(WMScreen *scr, string name, bool makeBox = true);
   RWWindow(RWWindow *parent, string name, bool makeBox = true,
	    bool closable = true);
   virtual ~RWWindow();
};

#endif
    
