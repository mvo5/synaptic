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
    if (_startCounting) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbar), 1.0);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbar_total), 1.0);
    }
    
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
	    float val;
	    if (line[0] != '%') {
		gtk_label_set_text(GTK_LABEL(_label), line);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbar), 0);
		if (_startCounting) {
		    _donePackages += 1;
		    val = ((float)_donePackages)/_numPackages;
		    gtk_progress_bar_set_fraction(
				    GTK_PROGRESS_BAR(_pbar_total), val);
		}
	    } else {
		sscanf(line + 3, "%f", &val);
		val = val*0.01;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbar), val);
		if (_startCounting == false) {
		    _startCounting = true;
		    // Stop pulsing
		    gtk_progress_bar_set_fraction(
				    GTK_PROGRESS_BAR(_pbar_total), 0);
		}
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
	usleep(5000); // Half a second
        if (_startCounting == false) {
	    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbar));
	    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbar_total));
        }
    }
}


RGInstallProgress::RGInstallProgress(RGMainWindow *main)
    : RInstallProgress(), RGWindow(main, "install_progress", true, false)
{
    setTitle(_("Performing Changes"));

    _donePackages = 0;
    _startCounting = false;

    gtk_widget_set_usize(_win, 320, 120);

    gtk_container_set_border_width(GTK_CONTAINER(_topBox), 10);
    
    _label = gtk_label_new("");
    gtk_widget_show(_label);
    gtk_box_pack_start(GTK_BOX(_topBox), _label, TRUE, TRUE, 10);

    _pbar = gtk_progress_bar_new();
    gtk_widget_show(_pbar);
    gtk_widget_set_usize(_pbar, -1, 25);
    gtk_box_pack_start(GTK_BOX(_topBox), _pbar, FALSE, TRUE, 0);
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbar));
    gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(_pbar), 0.01);

    _pbar_total = gtk_progress_bar_new();
    gtk_widget_show(_pbar_total);
    gtk_widget_set_usize(_pbar_total, -1, 25);
    gtk_box_pack_start(GTK_BOX(_topBox), _pbar_total, FALSE, TRUE, 0);
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbar_total));
    gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(_pbar_total), 0.01);
}


