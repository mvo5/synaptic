/* rwinstallprogress.cc
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

#include "rwmainwindow.h"

#include "rwinstallprogress.h"

#include <unistd.h>
#include <stdio.h>

extern void RWFlushInterface();


void RWInstallProgress::startUpdate()
{
    show();
    RWFlushInterface();
}


void RWInstallProgress::finishUpdate()
{
    WMSetProgressIndicatorValue(_pbar, 1000);
    
    RWFlushInterface();
    
    hide();
}


void RWInstallProgress::updateInterface()
{
    char buf[2];
    static char line[128] = "";

    while (1) {
	int len = read(_childin, buf, 1);
	if (len < 1)
	    break;
	if (buf[0] == '\n') {
	    if (line[0] != '%') {
		WMSetLabelText(_label, line);
		WMSetProgressIndicatorValue(_pbar, 0);
	    } else {
		float val;

		sscanf(line + 3, "%f", &val);
		WMSetProgressIndicatorValue(_pbar, (int)(val * 10.0));
	    }
	    line[0] = 0;
	} else {
	    buf[1] = 0;
	    strcat(line, buf);
	}
    }

    if (XPending(WMScreenDisplay(_scr)) > 0) {
	XEvent ev;

	WMNextEvent(WMScreenDisplay(_scr), &ev);
	WMHandleEvent(&ev);
    } else {
	wusleep(1000);
    }
}


RWInstallProgress::RWInstallProgress(RWMainWindow *main)
    : RInstallProgress(), RWWindow(main, "installProgress", true, false)
{
    setTitle(_("Performing Changes"));
    _scr = WMWidgetScreen(_win);


    WMResizeWidget(_win, 320, 80);

    WMSetBoxBorderWidth(_topBox, 10);
    
    WMSetBoxHorizontal(_topBox, False);
    
    _label = WMCreateLabel(_topBox);
    WMMapWidget(_label);
    WMAddBoxSubview(_topBox, WMWidgetView(_label), True, True, 20, 0, 10);

    _pbar = WMCreateProgressIndicator(_topBox);
    WMSetProgressIndicatorMaxValue(_pbar, 1000);
    WMMapWidget(_pbar);
    WMAddBoxSubview(_topBox, WMWidgetView(_pbar), False, True, 25, 0, 0);

    WMRealizeWidget(_win);
}


