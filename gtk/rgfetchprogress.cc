/* rgfetchprogress.cc
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2002, 2003 Michael Vogt <mvo@debian.org>
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

enum {
    DLDone = -1,
    DLQueued = -2,
    DLFailed = -3,
    DLHit = -4
};

enum {
    FETCH_PIXMAP_COLUMN,
    SIZE_COLUMN,
    URL_COLUMN,
    N_FETCH_COLUMNS
  };
    

RGFetchProgress::RGFetchProgress(RGWindow *win) 
    : RGGladeWindow(win, "fetch")
{   
    GtkCellRenderer *renderer; 
    GtkTreeViewColumn *column; 
 
    setTitle(_("Fetching Files"));
    gtk_widget_set_usize(_win, 620, 250);

    gint dummy;
    gdk_window_get_geometry(_win->window, &dummy, &dummy, &dummy, &dummy,
                            &_depth);

    _table = glade_xml_get_widget(_gladeXML, "treeview_fetch");
    _tableListStore = gtk_list_store_new(3, 
					 GDK_TYPE_PIXBUF,
					 G_TYPE_STRING,
					 G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(_table), 
			    GTK_TREE_MODEL(_tableListStore));

    /* percent column */
    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes(_("Status"), renderer,
						      "pixbuf", FETCH_PIXMAP_COLUMN,
						       NULL);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(column, 100);
    _statusColumn = column;
    _statusRenderer = renderer;
    gtk_tree_view_append_column(GTK_TREE_VIEW(_table), column);

    /* size */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes(_("Size"), renderer,
						      "text", SIZE_COLUMN,
						       NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_table), column);

    /* url */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes(_("URL"), renderer,
						      "text", URL_COLUMN,
						       NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_table), column);


    _statusL = glade_xml_get_widget(_gladeXML, "label_status");
    gtk_misc_set_alignment(GTK_MISC(_statusL), 0.0f, 0.0f);
    gtk_label_set_justify(GTK_LABEL(_statusL), GTK_JUSTIFY_LEFT);

    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_cancel_clicked",
				  G_CALLBACK(stopDownload),
				  this); 
    
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
    //cout << "void RGFetchProgress::updateStatus()" << endl;

    if (Itm.Owner->ID == 0) {
	Item item;
	item.uri = Itm.Description;
	item.size = string(SizeToStr(Itm.Owner->FileSize));
        item.status = status;
	_items.push_back(item);
	Itm.Owner->ID = _items.size();
        refreshTable(Itm.Owner->ID-1, true);
    } else if (_items[Itm.Owner->ID-1].status != status) {
        _items[Itm.Owner->ID-1].status = status;
        refreshTable(Itm.Owner->ID-1, false);
    }
}


void RGFetchProgress::IMSHit(pkgAcquire::ItemDesc &Itm)
{
    //cout << "void RGFetchProgress::IMSHit(pkgAcquire::ItemDesc &Itm)" << endl;
    updateStatus(Itm, DLHit);

    RGFlushInterface();
}


void RGFetchProgress::Fetch(pkgAcquire::ItemDesc &Itm)
{
    updateStatus(Itm, DLQueued);

    RGFlushInterface();
}


void RGFetchProgress::Done(pkgAcquire::ItemDesc &Itm)
{
    updateStatus(Itm, DLDone);

    RGFlushInterface();
}


void RGFetchProgress::Fail(pkgAcquire::ItemDesc &Itm)
{
    updateStatus(Itm, DLFailed);

    RGFlushInterface();
}


bool RGFetchProgress::Pulse(pkgAcquire *Owner)
{
    //cout << "RGFetchProgress::Pulse(pkgAcquire *Owner)" << endl;

    string str;
    pkgAcquireStatus::Pulse(Owner);

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
    RGFlushInterface();

    return !_cancelled;
}


void RGFetchProgress::Start()
{
  //cout << "RGFetchProgress::Start()" << endl;
    pkgAcquireStatus::Start();
    _cancelled = false;
    show();
    RGFlushInterface();
}


void RGFetchProgress::Stop()
{
  //cout << "RGFetchProgress::Stop()" << endl;
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


void RGFetchProgress::refreshTable(int row, bool append)
{
    //cout << "RGFetchProgress::refreshTable() " << row << endl;
    GtkTreeIter iter;
    static GdkPixmap *pix=NULL;
    static GdkPixbuf *buf=NULL;
    int w,h;

    // unref pix first (they start with a usage count of 1
    // why not unref'ing it after adding in the table? -- niemeyer
    if(pix != NULL) 
	gdk_pixmap_unref(pix);
    if(buf != NULL) 
	gdk_pixbuf_unref(buf);

    w = gtk_tree_view_column_get_width(_statusColumn);
    h = 18; // FIXME: height -> get it from somewhere

    pix = statusDraw(w, h, _items[row].status);
    buf = gdk_pixbuf_get_from_drawable(NULL, pix, NULL,
				 0, 0, 0, 0, w, h );
    GtkTreePath *path;
    if (append == true) {
	gtk_list_store_insert(_tableListStore, &iter, row);
        path = gtk_tree_path_new_from_indices(row, -1);
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(_table), path, NULL, false);
        gtk_tree_path_free(path);
        // can't we use the iterator here?
    } else {
	gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(_tableListStore),
				      &iter, NULL, row);
    }
    gtk_list_store_set(_tableListStore, &iter,
		       FETCH_PIXMAP_COLUMN, buf,
		       SIZE_COLUMN, _items[row].size.c_str(),
		       URL_COLUMN, _items[row].uri.c_str(),
		       -1);
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(_tableListStore),
		    		   &iter);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(_table),
				 path, NULL, TRUE, 0.0, 0.0);
    gtk_tree_path_free(path);
}

// vim:sts=4:sw=4
