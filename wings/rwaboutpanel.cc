/* rwaboutpanel.cc
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

#include "rwaboutpanel.h"


#include "cnc.xpm"
#include "deb.xpm"
#include "logo.xpm"


static void closeWindow(WMWidget *self, void *data)
{
    RWAboutPanel *about = (RWAboutPanel*)data;
    
    about->hide();
}


RWAboutPanel::RWAboutPanel(RWWindow *parent) 
    : RWWindow(parent, "about")
{
    WMBox *vbox, *hbox;
    WMScreen *scr = WMWidgetScreen(parent->window());
    WMLabel *label;
    WMLabel *icon;
    WMPixmap *pix;
    WMColor *white = WMWhiteColor(scr);


    WMSetWindowMinSize(_win, 360, 240);
    WMSetWindowMaxSize(_win, 360, 240);
    
    vbox = _topBox;
    WMSetBoxBorderWidth(vbox, 10);
    WMSetWidgetBackgroundColor(vbox, white);
    WMMapWidget(vbox);
        
    hbox = WMCreateBox(vbox);
    WMSetBoxHorizontal(hbox, True);
    WMSetWidgetBackgroundColor(hbox, white);
    WMAddBoxSubview(vbox, WMWidgetView(hbox), False, True, 60, 0, 5);
    
    icon = WMCreateLabel(hbox);
    WMSetWidgetBackgroundColor(icon, white);
    pix = WMCreatePixmapFromXPMData(scr, cnc_xpm);
    WMSetLabelImagePosition(icon, WIPImageOnly);
    WMSetLabelImage(icon, pix);
    WMReleasePixmap(pix);
    WMAddBoxSubview(hbox, WMWidgetView(icon), False, True, 100, 0, 5);

    icon = WMCreateLabel(hbox);
    WMSetWidgetBackgroundColor(icon, white);
    pix = WMCreatePixmapFromXPMData(scr, logo_xpm);
    WMSetLabelImagePosition(icon, WIPImageOnly);
    WMSetLabelImage(icon, pix);
    WMReleasePixmap(pix);
    WMAddBoxSubview(hbox, WMWidgetView(icon), True, True, 100, 0, 5);

    icon = WMCreateLabel(hbox);
    WMSetWidgetBackgroundColor(icon, white);
    pix = WMCreatePixmapFromXPMData(scr, deb_xpm);
    WMSetLabelImagePosition(icon, WIPImageOnly);
    WMSetLabelImage(icon, pix);
    WMReleasePixmap(pix);
    WMAddBoxSubview(hbox, WMWidgetView(icon), False, True, 50, 0, 5);

    label = WMCreateLabel(vbox);
    WMSetWidgetBackgroundColor(label, WMWhiteColor(scr));
//			       WMCreateNamedColor(scr, "#ddddee", False));
    WMSetLabelRelief(label, WRFlat);
    WMSetLabelText(label, _("Copyright (c) 2001 Conectiva S/A\n\n"
		   "Author: Alfredo K. Kojima <kojima@conectiva.com.br>\n"
		   "Icons shamelessly ripped from KDE\n"
		   "APT backend: Jason Gunthorpe <jgg@debian.org>\n\n"
		   "This software is licensed under the terms of the\n"
		   "GNU General Public License, Version 2"));
	
    WMAddBoxSubview(vbox, WMWidgetView(label), True, True, 125, 0, 5);

    WMButton *button = WMCreateCommandButton(vbox);
    WMMapWidget(button);
    WMSetButtonText(button, _("Dismiss"));
    WMAddBoxSubview(vbox, WMWidgetView(button), False, True, 24, 0, 0);
    WMSetButtonAction(button, closeWindow, this);
    
    WMMapSubwidgets(hbox);
    WMMapSubwidgets(vbox);
    
    setTitle(PACKAGE" version "VERSION);
    
    WMRealizeWidget(_win);
}
