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

RGInstallProgressMsgs::RGInstallProgressMsgs(RGWindow *win)
    : RGGladeWindow(win, "rginstall_progress_msgs"),
      _currentPackage(0), _hasHeader(false)
{
    setTitle(_("Package Manager output"));
    GtkWidget *textView = glade_xml_get_widget(_gladeXML, "textview");
    _textBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    glade_xml_signal_connect_data(_gladeXML, "on_close_clicked",
				  G_CALLBACK(onCloseClicked), this); 
    PangoFontDescription *font;
    font = pango_font_description_from_string ("helvetica 10");
    gtk_widget_modify_font(textView, font);
    font = pango_font_description_from_string ("helvetica bold 10");
    gtk_text_buffer_create_tag(_textBuffer, "bold", "font-desc", font, NULL);
}

void RGInstallProgressMsgs::onCloseClicked(GtkWidget *self, void *data)
{
    //RGInstallProgressMsgs *me = (RGInstallProgressMsgs*)data;
    gtk_main_quit();
}

bool RGInstallProgressMsgs::close()
{
    gtk_main_quit();
}

void RGInstallProgressMsgs::addText(const char *text, bool bold)
{
    GtkTextIter enditer;
    gtk_text_buffer_get_end_iter(_textBuffer, &enditer);
    if (bold == true)
	gtk_text_buffer_insert_with_tags_by_name(_textBuffer, &enditer,
						 text, -1, "bold", NULL);
    else
	gtk_text_buffer_insert(_textBuffer, &enditer, text, -1);
}

void RGInstallProgressMsgs::addLine(const char *text)
{
    if (!_hasHeader) {
	char *header;
	if (_currentPackage != NULL)
	    header = g_strdup_printf(
		_("\nWhile installing package %s:\n\n"),
		_currentPackage);
	else
	    header = g_strdup_printf(
		_("\nWhile preparing for installation:\n\n"));
	addText(header, true);
	_hasHeader = true;
    }
    addText(text);
    addText("\n");
}

void RGInstallProgressMsgs::newPackage(const char *name)
{
    _currentPackage = name;
    _hasHeader = false;
}

bool RGInstallProgressMsgs::empty()
{
    return gtk_text_buffer_get_char_count(_textBuffer) == 0;
}

void RGInstallProgressMsgs::run()
{
    show();
    gtk_main();
    hide();
}

void RGInstallProgress::startUpdate()
{
    show();
    RGFlushInterface();
}

void RGInstallProgress::finishUpdate()
{
    if (_startCounting) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbar), 1.0);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbarTotal), 1.0);
    }

    if (_msgs.empty() == false &&
	_config->FindB("Synaptic::IgnorePMOutput", false) == false)
	_msgs.run();
    
    RGFlushInterface();
    
    hide();
}

void RGInstallProgress::prepare(RPackageLister *lister)
{
    for (unsigned int row=0; row < lister->count(); row++) {
	RPackage *elem = lister->getElement(row);

	// Is it going to be seen?
	RPackage::MarkedStatus mark = elem->getMarkedStatus();
	if (!(mark == RPackage::MInstall ||
	      mark == RPackage::MUpgrade ||
	      mark == RPackage::MDowngrade))
	    continue;

	const char *name = elem->name();
	const char *ver = elem->availableVersion();
	const char *pos = strchr(ver, ':');
	if (pos)
	    ver = pos+1;
	string namever = string(name) + "-" + string(ver);
	_summaryMap[namever] = elem->summary();
    }
}

void RGInstallProgress::updateInterface()
{
    char buf[2];
    static char line[1024] = "";

    while (1) {
	// This algorithm should be improved.
	int len = read(_childin, buf, 1);
	if (len < 1)
	    break;
	if (buf[0] == '\n') {
	    float val;
	    if (line[0] != '%') {
		map<string,string>::const_iterator I =
		    _summaryMap.find(line);
		if (I == _summaryMap.end()) {
		    if (_startCounting == false) {
			gtk_label_set_label(GTK_LABEL(_label), utf8(line));
			gtk_progress_bar_set_fraction(
				GTK_PROGRESS_BAR(_pbar), 0);
		    } else {
			_msgs.addLine(utf8(line));
		    }
		} else {
		    // Get from the map, so that _msgs doesn't have
		    // to keep an internal copy.
		    _msgs.newPackage(I->first.c_str());
		    gtk_label_set_label(GTK_LABEL(_label), utf8(line));
		    gtk_label_set_label(GTK_LABEL(_labelSummary),
					utf8(I->second.c_str()));
		    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbar), 0);
		    _donePackages += 1;
		    val = ((float)_donePackages)/_numPackages;
		    gtk_progress_bar_set_fraction(
				    GTK_PROGRESS_BAR(_pbarTotal), val);
		}
	    } else {
		sscanf(line + 3, "%f", &val);
		val = val*0.01;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbar), val);
		if (_startCounting == false) {
		    // This will happen when the "Preparing..." progress
		    // is shown and its progress percentage starts.
		    _startCounting = true;
		    // Stop pulsing
		    gtk_progress_bar_set_fraction(
				    GTK_PROGRESS_BAR(_pbarTotal), 0);
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
	    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbarTotal));
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

RGInstallProgress::RGInstallProgress(RGMainWindow *main,
				     RPackageLister *lister)
    : RInstallProgress(),
      RGGladeWindow(main, "rginstall_progress"), _msgs(main)
{
    prepare(lister);
    setTitle(_("Performing Changes"));

    _startCounting = false;

    string GeoStr = _config->Find("Synaptic::Geometry::InstProg", "");
    GeometryParser Geo(GeoStr);
    if (Geo.HasSize())
	gtk_widget_set_usize(GTK_WIDGET(_win), Geo.Width(), Geo.Height());
    if (Geo.HasPosition())
	gtk_widget_set_uposition(GTK_WIDGET(_win), Geo.XPos(), Geo.YPos());

    _label = glade_xml_get_widget(_gladeXML, "label_name");
    _labelSummary = glade_xml_get_widget(_gladeXML, "label_summary");
    _pbar = glade_xml_get_widget(_gladeXML, "progress_package");
    _pbarTotal = glade_xml_get_widget(_gladeXML, "progress_total");

    PangoFontDescription *bfont;
    PangoFontDescription *font;
    bfont = pango_font_description_from_string ("helvetica bold 10");    
    font = pango_font_description_from_string ("helvetica 10");

    gtk_widget_modify_font(_label, bfont);
    gtk_widget_modify_font(_labelSummary, font);

    gtk_label_set_text(GTK_LABEL(_label), "");
    gtk_label_set_text(GTK_LABEL(_labelSummary), "");
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbar));
    gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(_pbar), 0.01);
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbarTotal));
    gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(_pbarTotal), 0.01);
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
