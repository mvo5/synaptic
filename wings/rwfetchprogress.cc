/* rwfetchprogress.cc
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

#include <apt-pkg/acquire-item.h>
#include <apt-pkg/acquire-worker.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/error.h>

#include "rwfetchprogress.h"

#include <WINGs/wtableview.h>
#include <WINGs/wtabledelegates.h>

#include "stop.xpm"

#include <stdio.h>

extern void RWFlushInterface();



enum {
    DLDone = -1,
	DLQueued = -2,
	DLFailed = -3,
	DLHit = -4
};
    



static WMTableColumnDelegate *WTCreateDLProgressDelegate(WMTableView *parent);
static WMTableColumnDelegate *WTCreateRStringDelegate(WMTableView *parent);


RWFetchProgress::RWFetchProgress(RWWindow *wwin) 
    : RWWindow(wwin, "fetchProgress")
{
    WMTableColumn *column;
    WMTableColumnDelegate *cdeleg;
    static WMTableViewDelegate delegate = {
	NULL,
	    numberOfRows,
	    valueForCell,
	    NULL
    };
    
    setTitle(_("Fetching Files"));

    WMResizeWidget(_win, 550, 250);
    
    _table = WMCreateTableView(_topBox);
    WMMapWidget(_table);
    WMAddBoxSubview(_topBox, WMWidgetView(_table), True, True, 20, 0, 5);
    
    WMSetTableViewDataSource(_table, this);
    WMSetTableViewHeaderHeight(_table, 20);
    WMSetTableViewDelegate(_table, &delegate);


    cdeleg = WTCreateRStringDelegate(_table);

    column = WMCreateTableColumn(_("URL"));
    WMSetTableColumnWidth(column, 365);
    WMAddTableViewColumn(_table, column);
    WMSetTableColumnDelegate(column, cdeleg);
    WMSetTableColumnId(column, (void*)0);

    column = WMCreateTableColumn(_("Size"));
    WMSetTableColumnWidth(column, 60);
    WMAddTableViewColumn(_table, column);	
    WMSetTableColumnDelegate(column, cdeleg);
    WMSetTableColumnId(column, (void*)1);


    cdeleg = WTCreateDLProgressDelegate(_table);

    column = WMCreateTableColumn(_("Status"));
    WMSetTableColumnWidth(column, 90);
    WMAddTableViewColumn(_table, column);
    WMSetTableColumnDelegate(column, cdeleg);
    WMSetTableColumnId(column, (void*)2);
    WMSetTableViewGridColor(_table, WMBlackColor(WMWidgetScreen(_win)));

    WMBox *hbox = WMCreateBox(_topBox);
    WMSetBoxHorizontal(hbox, True);
    WMAddBoxSubview(_topBox, WMWidgetView(hbox), False, True, 35, 0, 0);
    
    _statusL = WMCreateLabel(hbox);
    
    WMSetLabelRelief(_statusL, WRSunken);
    WMAddBoxSubview(hbox, WMWidgetView(_statusL), True, True, 15, 0, 5);

    WMButton *btn;
    btn = WMCreateCommandButton(hbox);
//    WMSetButtonText(btn, _("Stop"));
    WMSetButtonImagePosition(btn, WIPImageOnly);
    WMSetButtonImage(btn, WMCreatePixmapFromXPMData(WMWidgetScreen(_win),
						    stop_xpm));
    WMSetButtonAction(btn, stopDownload, this);
    WMAddBoxSubview(hbox, WMWidgetView(btn), False, True, 35, 0, 0);

    WMMapSubwidgets(hbox);
    
    WMMapSubwidgets(_topBox);
    WMMapWidget(_topBox);
    WMRealizeWidget(_win);
}

   
bool RWFetchProgress::MediaChange(string Media,string Drive)
{
    string msg;
    int res;
    
    msg = _("Please insert the disk labeled:\n")+Media+_("\nin drive ")+Drive;

    res = WMRunAlertPanel(WMWidgetScreen(_win), _win,
			  _("Disk Change"), (char*)msg.c_str(),
			  _("Continue"), _("Cancel"), NULL);

    Update = true;

    RWFlushInterface();

    if (res != WAPRDefault)
       _cancelled = true;
   
    return (res == WAPRDefault) ? true : false;
}


void RWFetchProgress::updateStatus(pkgAcquire::ItemDesc &Itm,
				   int status)
{
    bool reload = false;

    if (Itm.Owner->ID == 0) {
	Item item;
	item.uri = Itm.Description;
	item.size = string(SizeToStr(Itm.Owner->FileSize));
	_items.push_back(item);
	Itm.Owner->ID = _items.size();
	reload = true;
    }
    _items[Itm.Owner->ID-1].status = status;

    if (reload) {
	WMReloadTableView(_table);
	WMScrollTableViewRowToVisible(_table, _items.size()-1);
    } else {
	WMRedisplayWidget(_table);
    }
}


void RWFetchProgress::IMSHit(pkgAcquire::ItemDesc &Itm)
{
    updateStatus(Itm, DLHit);

    RWFlushInterface();
}


void RWFetchProgress::Fetch(pkgAcquire::ItemDesc &Itm)
{
    updateStatus(Itm, DLQueued);

    RWFlushInterface();
}


void RWFetchProgress::Done(pkgAcquire::ItemDesc &Itm)
{
    updateStatus(Itm, DLDone);

    RWFlushInterface();
}


void RWFetchProgress::Fail(pkgAcquire::ItemDesc &Itm)
{
    updateStatus(Itm, DLFailed);

    RWFlushInterface();
}


bool RWFetchProgress::Pulse(pkgAcquire *Owner)
{
    string str;
    pkgAcquireStatus::Pulse(Owner);

    if (CurrentCPS != 0) {
	char buf[128];
	unsigned long ETA = (unsigned long)((TotalBytes - CurrentBytes)/CurrentCPS);
	long i;
	i = CurrentItems < TotalItems ? CurrentItems+1 : CurrentItems;
	snprintf(buf, sizeof(buf), _("%-3li/%-3li files    %4sB/s   ETA %6s\n"),
		 i, TotalItems,
		 SizeToStr(CurrentCPS).c_str(),
		 TimeToStr(ETA).c_str());

	str = string(buf);
    } else {
	str = _("(stalled)\n");
    }
    
    
    for (pkgAcquire::Worker *I = Owner->WorkersBegin(); I != 0;
	 I = Owner->WorkerStep(I)) {

#undef Status // damn Xlib
	if (I->CurrentItem == 0) {
	    if (!I->Status.empty()) {
		str = str + '[' + I->Status.c_str() + "] ";
	    } else {
		str = str + _("[Working] ");
	    }
	    continue;
	}
	
	str = str + _("[Receiving] ");

	if (I->TotalSize > 0)
	    updateStatus(*I->CurrentItem, I->CurrentSize*100 / I->TotalSize);
	else
	    updateStatus(*I->CurrentItem, 100);
    }

    WMSetLabelText(_statusL, (char*)str.c_str());
    WMReloadTableView(_table);
    

    RWFlushInterface();

    return !_cancelled;
}


void RWFetchProgress::Start()
{
    pkgAcquireStatus::Start();

    _cancelled = false;
    
    show();
    
    RWFlushInterface();
}


void RWFetchProgress::Stop()
{
    hide();
    
    RWFlushInterface();

    pkgAcquireStatus::Stop();
}



void RWFetchProgress::stopDownload(WMWidget *self, void *data)
{
    RWFetchProgress *me = (RWFetchProgress*)data;
    
    me->_cancelled = true;
}


int RWFetchProgress::numberOfRows(WMTableViewDelegate *self, 
				  WMTableView *table)
{
    RWFetchProgress *me = (RWFetchProgress*)WMGetTableViewDataSource(table);

    return me->_items.size();
}


void *RWFetchProgress::valueForCell(WMTableViewDelegate *self,
				    WMTableColumn *column, int row)
{
    WMTableView *table = (WMTableView*)WMGetTableColumnTableView(column);
    RWFetchProgress *me = (RWFetchProgress*)WMGetTableViewDataSource(table);
    int c = (int)WMGetTableColumnId(column);

    switch (c) {
     case 0:
	return (void*)me->_items[row].uri.c_str();
     case 1:
	return (void*)me->_items[row].size.c_str();
     case 2:
	return (void*)me->_items[row].status;
    }
    
    return NULL;
}




/* ---------------------------------------------------------------------- */


typedef struct {
    WMTableView *table;
    WMFont *font;
    GC gc;
    GC textGc;
} StringData;


typedef struct {
    WMTableView *table;
    WMFont *font;
    GC gc;
    GC textGc;
    GC barGc;
} DLProgressData;



//static char *SelectionColor = "#bbbbcc";

static void progressDraw(WMScreen *scr, Drawable d, GC gc, GC barGc, GC textGc,
		       WMFont *font, void *data,
		       WMRect rect)
{
    int x, y;
    XRectangle rects[1];
    Display *dpy = WMScreenDisplay(scr);
    char *str;
    int status = (int)data;

    rects[0].x = rect.pos.x+1;
    rects[0].y = rect.pos.y+1;
    rects[0].width = rect.size.width-1;
    rects[0].height = rect.size.height-1;
    XSetClipRectangles(dpy, gc, 0, 0, rects, 1, YXSorted);

    str = "";

    int px, pw;
	
    px = rects[0].x;
    pw = status * rects[0].width / 100;    
    
    if (status < 0) {
	XFillRectangle(dpy, d, status == DLDone ? barGc : gc, 
		       px, rects[0].y, pw, rects[0].height);

	switch (status) {
	 case DLQueued:
	    str = _("Queued");
	    break;
	 case DLDone:
	    str = _("Done");
	    break;
	 case DLHit:
	    str = _("Hit");
	    break;
	 case DLFailed:
	    str = _("Failed");
	    break;
	}
    } else {
	static char buf[16];

	XFillRectangle(dpy, d, barGc, px, rects[0].y, pw, rects[0].height);
	XFillRectangle(dpy, d, gc,  px+pw, rects[0].y, 
		       rects[0].width - pw - 2, rects[0].height);
	
	snprintf(buf, sizeof(buf), "%d%%", status);
	str = buf;
    }

    x = rect.pos.x + (rect.size.width - WMWidthOfString(font, str, strlen(str))) / 2;
    y = rect.pos.y + (rect.size.height - WMFontHeight(font))/2;

    WMDrawString(scr, d, textGc, font, x, y, str, strlen(str));

    XSetClipMask(dpy, gc, None);
}




static void DLCellPainter(WMTableColumnDelegate *self,
			WMTableColumn *column, int row, Drawable d)
{
    DLProgressData *strdata = (DLProgressData*)self->data;
    WMTableView *table = WMGetTableColumnTableView(column);
    
    progressDraw(WMWidgetScreen(table), d,
		 strdata->gc,  strdata->barGc, strdata->textGc, strdata->font,
		 WMTableViewDataForCell(table, column, row),
		 WMTableViewRectForCell(table, column, row));
}



static WMTableColumnDelegate *WTCreateDLProgressDelegate(WMTableView *parent)
{
    WMTableColumnDelegate *delegate;
    WMScreen *scr = WMWidgetScreen(parent);
    DLProgressData *data = (DLProgressData*)wmalloc(sizeof(DLProgressData));

    delegate = (WMTableColumnDelegate*)wmalloc(sizeof(WMTableColumnDelegate));

    data->table = parent;
    data->font = WMSystemFontOfSize(scr, 12);
    data->gc = WMColorGC(WMGrayColor(scr));
    data->textGc = WMColorGC(WMBlackColor(scr));    
    data->barGc = WMColorGC(WMWhiteColor(scr));

    delegate->data = data;
    delegate->drawCell = DLCellPainter;
    delegate->drawSelectedCell = DLCellPainter;
    delegate->beginCellEdit = NULL;
    delegate->endCellEdit = NULL;

    return delegate;
}








static void stringDraw(WMScreen *scr, Drawable d, GC gc,
		       GC stgc, WMFont *font, char *data,
		       WMRect rect, Bool selected)
{
    int x, y;
    XRectangle rects[1];
    Display *dpy = WMScreenDisplay(scr);

    x = rect.pos.x + 5;
    y = rect.pos.y + (rect.size.height - WMFontHeight(font))/2;

    if ((unsigned)WMWidthOfString(font, data, strlen(data)) > rect.size.width - 10)
	x -= (WMWidthOfString(font, data, strlen(data)) - rect.size.width + 10);
    
    rects[0].x = rect.pos.x+1;
    rects[0].y = rect.pos.y+1;
    rects[0].width = rect.size.width-1;
    rects[0].height = rect.size.height-1;
    XSetClipRectangles(dpy, gc, 0, 0, rects, 1, YXSorted);
    
    XFillRectangles(dpy, d, gc, rects, 1);
    
    WMDrawString(scr, d, stgc, font, x, y,
		 data, strlen(data));	
    
    XSetClipMask(dpy, gc, None);
}



static void SCellPainter(WMTableColumnDelegate *self,
			WMTableColumn *column, int row, Drawable d)
{
    StringData *strdata = (StringData*)self->data;
    WMTableView *table = WMGetTableColumnTableView(column);
    
    stringDraw(WMWidgetScreen(table), d,
	       strdata->gc, strdata->textGc, strdata->font,
	       (char*)WMTableViewDataForCell(table, column, row),
	       WMTableViewRectForCell(table, column, row),
	       False);
}


static WMTableColumnDelegate *WTCreateRStringDelegate(WMTableView *parent)
{
    WMTableColumnDelegate *delegate = (WMTableColumnDelegate*)wmalloc(sizeof(WMTableColumnDelegate));
    WMScreen *scr = WMWidgetScreen(parent);
    StringData *data = (StringData*)wmalloc(sizeof(StringData));
    
    data->table = parent;
    data->font = WMSystemFontOfSize(scr, 12);
    data->textGc = WMColorGC(WMBlackColor(scr));
    data->gc = WMColorGC(WMGrayColor(scr));
    
    delegate->data = data;
    delegate->drawCell = SCellPainter;
    delegate->drawSelectedCell = SCellPainter;
    delegate->beginCellEdit = NULL;
    delegate->endCellEdit = NULL;

    return delegate;
}
