/* rwwindow.cc 
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


#include "config.h"

#include "i18n.h"

#include "rwwindow.h"



void RWWindow::windowCloseAction(WMWidget *win, void *data)
{
    RWWindow *rwin = (RWWindow*)data;
    
    rwin->close();
}


RWWindow::RWWindow(WMScreen *scr, string name, bool makeBox)
{
    _win = WMCreateWindow(scr, (char*)name.c_str());

    WMSetWindowCloseAction(_win, windowCloseAction, this);
    
    if (makeBox) {
	_topBox = WMCreateBox(_win);
	WMMapWidget(_topBox);
	WMSetBoxBorderWidth(_topBox, 5);
	WMSetViewExpandsToParent(WMWidgetView(_topBox), 0, 0, 0, 0);
    } else {
	_topBox = NULL;
    }
}


RWWindow::RWWindow(RWWindow *parent, string name, bool makeBox, bool closable)
{
    int style = WMTitledWindowMask | WMResizableWindowMask;

    if (closable)
	style |= WMClosableWindowMask;

    _win = WMCreatePanelWithStyleForWindow(parent->window(),
					   (char*)name.c_str(),
					   style);
    
    WMSetWindowCloseAction(_win, windowCloseAction, this);
    
    if (makeBox) {
	_topBox = WMCreateBox(_win);
	WMMapWidget(_topBox);
	WMSetBoxBorderWidth(_topBox, 5);
	WMSetViewExpandsToParent(WMWidgetView(_topBox), 0, 0, 0, 0);
    } else {
	_topBox = NULL;
    }
}


RWWindow::~RWWindow()
{
//    cout << "Desotry wind"<<endl;
    WMDestroyWidget(_win);
}


void RWWindow::setTitle(string title)
{
    WMSetWindowTitle(_win, (char*)title.c_str());
}

void RWWindow::close()
{
    hide();
}
