/* rgfetchprogress.cc
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

#include "config.h"

#include "i18n.h"

#include <apt-pkg/acquire-item.h>
#include <apt-pkg/acquire-worker.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/error.h>

#include "rgfetchprogress.h"
#include "rguserdialog.h"
#include "gsynaptic.h"

#include <stdio.h>
#include <pango/pango.h>
#include <gtk/gtk.h>

#include "stop.xpm"


enum {
    DLDone = -1,
    DLQueued = -2,
    DLFailed = -3,
    DLHit = -4
};
    


#define TABLE_ROW_HEIGHT 16


RGFetchProgress::RGFetchProgress(RGWindow *win) 
    : RGWindow(win, "fetchProgress")
{    
    setTitle(_("Fetching Files"));
    
    gint dummy;
    gdk_window_get_geometry(_win->window, &dummy, &dummy, &dummy, &dummy,
			    &_depth);
    
    gtk_widget_set_usize(_win, 620, 250);
    
    char *headers[] = {
	_("Status"),
	_("Size"),
	_("URL")
    };

    GtkWidget *sview;
    sview = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(_topBox), sview, TRUE, TRUE, 5);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sview),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);


    _table = gtk_clist_new_with_titles(3, headers);
    gtk_widget_show(_table);
    gtk_container_add(GTK_CONTAINER(sview), _table);
    
    gtk_clist_set_column_justification(GTK_CLIST(_table), 1, 
				       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_row_height(GTK_CLIST(_table), TABLE_ROW_HEIGHT);
    
    gtk_clist_set_column_width(GTK_CLIST(_table), 0, 100);
    gtk_clist_set_column_width(GTK_CLIST(_table), 1, 60);

    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_set_usize(hbox, -1, 35);
    gtk_box_pack_start(GTK_BOX(_topBox), hbox, FALSE, TRUE, 0);
    
    _statusL = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(_statusL), 0.0f, 0.0f);
    gtk_label_set_justify(GTK_LABEL(_statusL), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(hbox), _statusL, TRUE, TRUE, 5);

    GtkWidget *btn;
    btn = gtk_button_new();

    GtkWidget *pix;
    {
	GdkPixmap *xpm;
	GdkBitmap *mask;
	
	xpm = gdk_pixmap_create_from_xpm_d(_win->window, &mask, NULL, stop_xpm);
	pix = gtk_pixmap_new(xpm, mask);
    }
    gtk_container_add(GTK_CONTAINER(btn), pix);
    gtk_signal_connect(GTK_OBJECT(btn), "clicked",
		       (GtkSignalFunc)stopDownload, this);
    gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, TRUE, 0);

    gtk_widget_show_all(hbox);
    
    gtk_widget_show_all(_topBox);
    gtk_widget_show(_topBox);

    PangoContext *context = gdk_pango_context_get();
    _layout = pango_layout_new(context);

    GtkStyle *style = gtk_widget_get_style(_win);

    _font = style->font_desc;
    _gc = style->white_gc;
    _barGC = style->bg_gc[0];
    _textGC = style->black_gc;
}

   
bool RGFetchProgress::MediaChange(string Media,string Drive)
{
    string msg;
    
    msg = _("Please insert the disk labeled:\n")+Media+_("\nin drive ")+Drive;

    RGUserDialog userDialog(this);
    _cancelled = !userDialog.proceed(msg.c_str());

    Update = true;

    RGFlushInterface();

    return !_cancelled;
}


void RGFetchProgress::updateStatus(pkgAcquire::ItemDesc &Itm,
				   int status)
{
    if (Itm.Owner->ID == 0) {
	Item item;
	item.uri = Itm.Description;
	item.size = SizeToStr(Itm.Owner->FileSize);
	item.status = status;
	_items.push_back(item);
	Itm.Owner->ID = _items.size();
	appendTable(Itm.Owner->ID-1);
    } else if (_items[Itm.Owner->ID-1].status != status) {
        _items[Itm.Owner->ID-1].status = status;
	refreshTable(Itm.Owner->ID-1);
    }
}

void RGFetchProgress::IMSHit(pkgAcquire::ItemDesc &Itm)
{
    gtk_clist_freeze(GTK_CLIST(_table));
    updateStatus(Itm, DLHit);
    gtk_clist_thaw(GTK_CLIST(_table));
    RGFlushInterface();
}

void RGFetchProgress::Fetch(pkgAcquire::ItemDesc &Itm)
{
    gtk_clist_freeze(GTK_CLIST(_table));
    updateStatus(Itm, DLQueued);
    gtk_clist_thaw(GTK_CLIST(_table));
    RGFlushInterface();
}

void RGFetchProgress::Done(pkgAcquire::ItemDesc &Itm)
{
    gtk_clist_freeze(GTK_CLIST(_table));
    updateStatus(Itm, DLDone);
    gtk_clist_thaw(GTK_CLIST(_table));
    RGFlushInterface();
}

void RGFetchProgress::Fail(pkgAcquire::ItemDesc &Itm)
{
    if (Itm.Owner->Status == pkgAcquire::Item::StatIdle)
        return;
    gtk_clist_freeze(GTK_CLIST(_table));
    updateStatus(Itm, DLFailed);
    gtk_clist_thaw(GTK_CLIST(_table));
    RGFlushInterface();
}

bool RGFetchProgress::Pulse(pkgAcquire *Owner)
{
    string str;
    pkgAcquireStatus::Pulse(Owner);

    gtk_clist_freeze(GTK_CLIST(_table));

    if (CurrentCPS != 0) {
	char buf[128];
	long i;
	unsigned long ETA = (unsigned long)((TotalBytes - CurrentBytes)/CurrentCPS);
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

    gtk_label_set_text(GTK_LABEL(_statusL), (char*)str.c_str());
    gtk_clist_thaw(GTK_CLIST(_table));    

    RGFlushInterface();

    return !_cancelled;
}

void RGFetchProgress::Start()
{
    pkgAcquireStatus::Start();
    _cancelled = false;
    show();
    RGFlushInterface();
}

void RGFetchProgress::Stop()
{
    RGFlushInterface();
    hide();
    pkgAcquireStatus::Stop();

    //FIXME: this needs to be handled in a better way (gtk-2 maybe?)
    sleep(1); // this sucks, but if ommited, the window will not always
              // closed (e.g. when a package is only deleted)
    RGFlushInterface();
}

void RGFetchProgress::stopDownload(GtkWidget *self, void *data)
{
    RGFetchProgress *me = (RGFetchProgress*)data;
    
    me->_cancelled = true;
}

GdkPixmap *RGFetchProgress::statusDraw(int width, int height, int status)
{
    int x, y;
    char *str = "";
    GdkPixmap *pix;
    int px, pw;

    pix = gdk_pixmap_new(_win->window, width, height, _depth);
    	
    px = 0;
    pw = status * width / 100;    
    
    if (status < 0) {
	gdk_draw_rectangle(pix, status == DLDone ? _barGC : _gc, 
			   TRUE, px, 0, pw, height);

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

	gdk_draw_rectangle(pix, _barGC, TRUE, px, 0, pw, height);
	gdk_draw_rectangle(pix, _gc,  TRUE, px+pw, 0, 
			   width - pw - 2, height);
	
	snprintf(buf, sizeof(buf), "%d%%", status);
	str = buf;
    }

    pango_layout_set_font_description(_layout, _font);
    pango_layout_set_text(_layout, str, -1);
    pango_layout_get_pixel_size(_layout, &x,&y);
    x = (width - x)/2;
    gdk_draw_layout(pix, _textGC, x,0, _layout);

    return pix;
}


void RGFetchProgress::refreshTable(int row)
{
    int w = GTK_CLIST(_table)->column[0].button->allocation.width;
    GdkPixmap *pix = statusDraw(w, TABLE_ROW_HEIGHT, _items[row].status);
    gtk_clist_set_pixmap(GTK_CLIST(_table), row, 0, pix, NULL);
    gdk_pixmap_unref(pix);

    gtk_clist_set_text(GTK_CLIST(_table), row, 1, _items[row].size.c_str());
    gtk_clist_set_text(GTK_CLIST(_table), row, 2, _items[row].uri.c_str());
}

void RGFetchProgress::appendTable(int row)
{
    char *s[] = {"","",""};
    gtk_clist_insert(GTK_CLIST(_table), row, s);
    refreshTable(row);
    if (gtk_clist_row_is_visible(GTK_CLIST(_table), row-2))
	gtk_clist_moveto(GTK_CLIST(_table), row, 0, 0.0, 0.0);
}

void RGFetchProgress::reloadTable()
{
    for (unsigned int i = 0; i < _items.size(); i++)
	refreshTable(i);
}

// vim:sts=4:sw=4
