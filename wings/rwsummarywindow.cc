/* rwsummarywindow.cc
 *
 * Copyright (c) 2000-2003 Conectiva S/A
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

#include <X11/keysym.h>

#include <apt-pkg/configuration.h>
#include <apt-pkg/strutl.h>

#include "rpackagelister.h"

#include <stdio.h>
#include <WINGs/WINGs.h>
#include "rwsummarywindow.h"



void RWSummaryWindow::handleKeyPress(XEvent *event, void *clientData)
{
    RWSummaryWindow *me = (RWSummaryWindow*)clientData;
    KeySym ksym;

    XLookupString(&event->xkey, NULL, 0, &ksym, NULL);
    
    if (ksym == XK_Return) {
	WMPerformButtonClick(me->_defBtn);
    }
}


void RWSummaryWindow::clickedOk(WMWidget *self, void *data)
{
    RWSummaryWindow *me = (RWSummaryWindow*)data;
    
    me->_confirmed = true;
    WMBreakModalLoop(WMWidgetScreen(self));
}


void RWSummaryWindow::clickedCancel(WMWidget *self, void *data)
{
    RWSummaryWindow *me = (RWSummaryWindow*)data;

    me->_confirmed = false;
    WMBreakModalLoop(WMWidgetScreen(self));
}


void RWSummaryWindow::clickedDetails(WMWidget *self, void *data)
{
    RWSummaryWindow *me = (RWSummaryWindow*)data;
    RPackageLister *lister = me->_lister;
    WMScrollView *view;
    WMLabel *info;
    int lines;

    WMSetButtonEnabled((WMButton*)self, False);
    
    WMDestroyWidget(me->_summaryL);
    
    WMResizeWidget(me->_win, 360, 400);

    WMSetFrameRelief(me->_topF, WRSunken);
    view = WMCreateScrollView(me->_topF);
    WMSetViewExpandsToParent(WMWidgetView(view), 1, 1, 2, 2);
    WMMapWidget(view);
    WMSetScrollViewHasVerticalScroller(view, True);
    
    info = WMCreateLabel(view);
    WMMapWidget(info);
    WMSetScrollViewContentView(view, WMWidgetView(info));
    
    
    lines = 1;
    
    string text;
    
    vector<RPackage*> held;
    vector<RPackage*> kept;
    vector<RPackage*> essential;
    vector<RPackage*> toInstall;
    vector<RPackage*> toReInstall;
    vector<RPackage*> toUpgrade;
    vector<RPackage*> toRemove;
    vector<RPackage*> toDowngrade;
    double sizeChange;

    
    lister->getDetailedSummary(held, 
			       kept, 
			       essential,
			       toInstall,
			       toReInstall,
			       toUpgrade, 
			       toRemove,
			       toDowngrade,
			       sizeChange);

    for (vector<RPackage*>::const_iterator p = essential.begin();
	 p != essential.end(); p++) {
	text += string((*p)->name()) + _(": (ESSENTIAL) will be Removed\n");
	lines++;
    }

    for (vector<RPackage*>::const_iterator p = toDowngrade.begin(); 
	 p != toDowngrade.end(); p++) {
	text += string((*p)->name()) + _(": will be Downgraded\n");
	lines++;
    }
    
    for (vector<RPackage*>::const_iterator p = toRemove.begin(); 
	 p != toRemove.end(); p++) {
	text += string((*p)->name()) + _(": will be Removed\n");
	lines++;
    }

    for (vector<RPackage*>::const_iterator p = toUpgrade.begin(); 
	 p != toUpgrade.end(); p++) {
	text += string((*p)->name()) + " " +(*p)->installedVersion()
	    + _(": will be Upgraded to ")+(*p)->availableVersion()+"\n";
	lines++;
    }

    for (vector<RPackage*>::const_iterator p = toInstall.begin(); 
	 p != toInstall.end(); p++) {
	text += string((*p)->name()) + " " + (*p)->availableVersion() + _(": will be Installed\n");
	lines++;
    }
    
    for (vector<RPackage*>::const_iterator p = toReInstall.begin(); 
	 p != toReInstall.end(); p++) {
	text += string((*p)->name()) + " " + (*p)->availableVersion() + _(": will be re-installed\n");
	lines++;
    }

    WMResizeWidget(info, 500, lines*14);

    WMSetLabelText(info, (char*)text.c_str());
    
    WMRealizeWidget(view);
}


RWSummaryWindow::RWSummaryWindow(RWWindow *wwin, RPackageLister *lister)
    : RWWindow(wwin, "summary", true, false)
{
    WMBox *hbox;
    WMButton *button;
    int lines = 0;

    _potentialBreak = false;

    _lister = lister;
    
    setTitle(_("Operation Summary"));
    
    WMResizeWidget(_win, 360, 150);
    
    WMSetBoxHorizontal(_topBox, False);
    WMSetBoxBorderWidth(_topBox, 10);
    
    WMCreateEventHandler(WMWidgetView(_win), KeyPressMask,
			 handleKeyPress, this);
    
    _topF = WMCreateFrame(_topBox);
    WMMapWidget(_topF);
    WMAddBoxSubview(_topBox, WMWidgetView(_topF), True, True, 10, 0, 5);
    
    _summaryL = WMCreateLabel(_topF);
    WMMapWidget(_summaryL);
    WMSetViewExpandsToParent(WMWidgetView(_summaryL), 5, 5, 5, 5);
    
    _dlonlyB = WMCreateSwitchButton(_topBox);
    WMSetButtonText(_dlonlyB, _("Perform package download only."));
    WMMapWidget(_dlonlyB);
    WMAddBoxSubview(_topBox, WMWidgetView(_dlonlyB), False, True, 20, 0, 10);

    
    hbox = WMCreateBox(_topBox);
    WMSetBoxHorizontal(hbox, True);
    WMAddBoxSubviewAtEnd(_topBox, WMWidgetView(hbox), False, True, 24, 0, 0);

    button = WMCreateCommandButton(hbox);
    WMSetButtonText(button, _("Show Details"));
    WMSetButtonAction(button, clickedDetails, this);
    WMAddBoxSubview(hbox, WMWidgetView(button), True, True, 80, 0, 20);

    _defBtn = button = WMCreateCommandButton(hbox);
    WMSetButtonText(button, _("Proceed"));
    WMSetButtonAction(button, clickedOk, this);
    WMSetButtonImagePosition(button, WIPRight);
    WMSetButtonImage(button, WMGetSystemPixmap(WMWidgetScreen(button),
					       WSIReturnArrow));
    WMSetButtonAltImage(button, WMGetSystemPixmap(WMWidgetScreen(button),
						  WSIHighlightedReturnArrow));

    WMAddBoxSubviewAtEnd(hbox, WMWidgetView(button), False, True, 100, 0, 0);

    button = WMCreateCommandButton(hbox);
    WMSetButtonText(button, _("Cancel"));
    WMSetButtonAction(button, clickedCancel, this);
    WMAddBoxSubviewAtEnd(hbox, WMWidgetView(button), False, True, 80, 0, 5);

    WMMapSubwidgets(hbox);
    WMMapWidget(hbox);

    WMMapWidget(_topBox);


    {
	char buffer[1024];
	int left = sizeof(buffer);
	int used = 0;
	int toInstall, toReInstall, toRemove, toUpgrade;
	int held, kept, essential, toDowngrade;
	double sizeChange, dlSize;
	int dlCount;
	
	lister->getSummary(held, kept, essential,
			   toInstall, toReInstall, toUpgrade, toRemove, toDowngrade,
			   sizeChange);

	lister->getDownloadSummary(dlCount, dlSize);
	
#define APPEND_TXT(descr, number)\
    used += snprintf(buffer+used, left, descr, number, \
			 number > 1 ? _("packages") : _("package")),\
    left -= used, lines++
	
	if (held)
	    APPEND_TXT(_("%d %s were held;\n"), held);
	
	if (kept)
	    APPEND_TXT(_("%d %s were kept back and not upgraded;\n"), kept);
	
	if (toInstall)
	    APPEND_TXT(_("%d new %s will be installed;\n"), toInstall);
	
	if (toUpgrade)
	    APPEND_TXT(_("%d %s will be upgraded;\n"), toUpgrade);
	
	if (toRemove)
	    APPEND_TXT(_("%d %s will be removed;\n"), toRemove);
			   
	if (essential) {
	    APPEND_TXT(_("WARNING: %d essential %s will be removed!\n"),
		       essential);
	    _potentialBreak = true;
	}

	char *templ = NULL;
	if (sizeChange > 0) {
	    templ = _("\n%s B will be used after finished.\n");
	} else if (sizeChange < 0) {
	    templ = _("\n%s B will be freed after finished.\n");
	    sizeChange = -sizeChange;
	}

	if (templ) {
	    used += snprintf(buffer+used, left, templ,
			     SizeToStr(sizeChange).c_str());
	    left -= used;
	    lines++;
	}

	if (dlSize > 0) {
	    used += snprintf(buffer+used, left, 
			     _("\n%s B need to be downloaded."),
			     SizeToStr(dlSize).c_str());
	    left -= used;
	    lines++;
	}
	if (left <= 0) {
	    cout << _("BUFFER OVERFLOW DETECTED, ABORTING") << endl;
	    abort();
	}
	
#undef APPEND_TXT

	WMSetLabelText(_summaryL, buffer);
    }
    
    WMResizeWidget(_win, 360, lines * 12 + 10 + 100 + 20);
    
    
    WMRealizeWidget(_win);
}



bool RWSummaryWindow::showAndConfirm()
{    
    WMSetButtonSelected(_dlonlyB, 
			_config->FindB("Synaptic::Download-Only", false));
    
    show();
    WMRunModalLoop(WMWidgetScreen(_win), WMWidgetView(_win));

    if (_confirmed && _potentialBreak
	&& WMRunAlertPanel(WMWidgetScreen(_win), _win, _("WARNING!!!"),
			   _("Essential packages will be removed.\n"
			     "That can render your system unusable!!!\n"),
			   _("Cancel"), _("Proceed System Breakage"), NULL) == WAPRDefault)
	return false;
    
    _config->Set("Synaptic::Download-Only",
		 WMGetButtonSelected(_dlonlyB) ? "true" : "false");
    
    return _confirmed;
}

