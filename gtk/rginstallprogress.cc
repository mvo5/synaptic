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
#include "gsynaptic.h"

#include "rginstallprogress.h"

#include <apt-pkg/configuration.h>

#include <unistd.h>
#include <stdio.h>


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
		gtk_label_set_text(GTK_LABEL(_label), utf8(line));
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
	usleep(5000);
        if (_startCounting == false) {
	    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbar));
	    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbar_total));
        }
    }
}

class GeometryParser
{
    protected:
    
    int _width;
    int _height;
    int _xpos;
    int _ypos;

    bool ParseSize(char **size);
    bool ParsePosition(char **pos);
    
    public:
    
    int Width() {return _width;};
    int Height() {return _height;};
    int XPos() {return _xpos;};
    int YPos() {return _ypos;};

    bool HasSize() {return _width != -1 && _height != -1;};
    bool HasPosition() {return _xpos != -1 && _ypos != -1;};

    bool Parse(string Geo);

    GeometryParser(string Geo="")
	: _width(-1), _height(-1), _xpos(-1), _ypos(-1)
	{Parse(Geo);};
};

RGInstallProgress::RGInstallProgress(RGMainWindow *main)
    : RInstallProgress(), RGWindow(main, "install_progress", true, false)
{
    setTitle(_("Performing Changes"));

    _donePackages = 0;
    _startCounting = false;

    string GeoStr = _config->Find("Synaptic::Geometry::InstProg", "");
    GeometryParser Geo(GeoStr);
    if (Geo.HasSize())
	gtk_widget_set_usize(GTK_WIDGET(_win), Geo.Width(), Geo.Height());
    else
	gtk_widget_set_usize(GTK_WIDGET(_win), 320, 120);
    if (Geo.HasPosition())
	gtk_widget_set_uposition(GTK_WIDGET(_win), Geo.XPos(), Geo.YPos());

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

bool GeometryParser::ParseSize(char **size)
{
    char *buf = *size;
    char *p = buf;
    while (*p >= '0' && *p <= '9') p++;
    if (*p != 'x')
	return false;
    *p++ = 0;
    _width = atoi(buf);
    buf = p;
    while (*p >= '0' && *p <= '9') p++;
    if (*p != '+' && *p != '-' && *p != 0)
	return false;
    char tmp = *p;
    *p = 0;
    _height = atoi(buf);
    *p = tmp;
    *size = p;
    return true;
}

bool GeometryParser::ParsePosition(char **pos)
{
    char *buf = *pos;
    char *p = buf+1;
    if (*p < '0' || *p > '9')
	return false;
    while (*p >= '0' && *p <= '9') p++;
    if (*p != '+' && *p != '-')
	return false;
    char tmp = *p;
    *p = 0;
    _xpos = atoi(buf);
    *p = tmp;
    buf = p++;
    if (*p < '0' || *p > '9')
	return false;
    while (*p >= '0' && *p <= '9') p++;
    if (*p != 0)
	return false;
    _ypos = atoi(buf);
    *pos = p;
    return true;
}

bool GeometryParser::Parse(string Geo)
{
    if (Geo.empty() == true)
	return false;
    
    char *buf = strdup(Geo.c_str());
    char *p = buf;
    bool ret = false;
    if (*p == '+' || *p == '-') {
	ret = ParsePosition(&p);
    } else {
	if (*p == '=')
	    p++;
	ret = ParseSize(&p);
	if (ret == true && (*p == '+' || *p == '-'))
	    ret = ParsePosition(&p);
    }
    free(buf);
    
    if (ret == false) {
	_width = -1;
	_height = -1;
	_xpos = -1;
	_ypos = -1;
    }

    return ret;
}

// vim:sts=4:sw=4
