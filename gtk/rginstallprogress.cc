/* rginstallprogress.cc
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

#include "rgmainwindow.h"

#include "rginstallprogress.h"

#include <unistd.h>
#include <stdio.h>

extern void RGFlushInterface();


void RGInstallProgress::startUpdate()
{
    show();
    RGFlushInterface();
}


void RGInstallProgress::finishUpdate()
{
    gtk_progress_bar_update(GTK_PROGRESS_BAR(_pbar), 1.0);
    
    RGFlushInterface();
    
    hide();
}


void RGInstallProgress::updateInterface()
{
    char buf[2];
    static char line[128] = "";

    while (1) {
	int len = read(_childin, buf, 1);
	if (len < 1)
	    break;
	if (buf[0] == '\n') {
	    if (line[0] != '%') {
		gtk_label_set_text(GTK_LABEL(_label), line);
		gtk_progress_bar_update(GTK_PROGRESS_BAR(_pbar), 0);
	    } else {
		float val;

		sscanf(line + 3, "%f", &val);
		gtk_progress_bar_update(GTK_PROGRESS_BAR(_pbar), val);
	    }
	    line[0] = 0;
	} else {
	    buf[1] = 0;
	    strcat(line, buf);
	}
    }

    if (gtk_events_pending()) {
	
	while (gtk_events_pending()) gtk_main_iteration();
    } else {
	usleep(1000);
    }
}


RGInstallProgress::RGInstallProgress(RGMainWindow *main)
    : RInstallProgress(), RGWindow(main, "installProgress", true, false)
{
    setTitle(_("Performing Changes"));


    gtk_widget_set_usize(_win, 320, 80);

    gtk_container_set_border_width(GTK_CONTAINER(_topBox), 10);
    
    _label = gtk_label_new("");
    gtk_widget_show(_label);
    gtk_box_pack_start(GTK_BOX(_topBox), _label, TRUE, TRUE, 10);

    _pbar = gtk_progress_bar_new();
    gtk_widget_show(_pbar);
    gtk_widget_set_usize(_pbar, -1, 25);
    gtk_box_pack_start(GTK_BOX(_topBox), _pbar, FALSE, TRUE, 0);
}


