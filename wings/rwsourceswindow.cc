/* rwsourceswindow.cc
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

#include "rwsourceswindow.h"

#include <WINGs/wtableview.h>

#include "up.xpm"
#include "down.xpm"


static int numberOfRows(WMTableViewDelegate *self, WMTableView *table)
{
    return 0;
}


static void *valueForCell(WMTableViewDelegate *self, WMTableColumn *column, int row)
{

    return (void*)"A";
}




static WMButton *makeButton(WMWidget *parent, char **image)
{
    WMButton *button;
    WMPixmap *pix;
    
    button = WMCreateCommandButton(parent);

    pix = WMCreatePixmapFromXPMData(WMWidgetScreen(parent), image);
    WMSetButtonImagePosition(button, WIPImageOnly);
    WMSetButtonImage(button, pix);
    WMReleasePixmap(pix);

    return button;
}


RWSourcesWindow::RWSourcesWindow(RWWindow *parent)
    : RWWindow(parent, "sources")
{
    WMBox *hbox;
    WMButton *btn;
    WMTableView *table;
    WMScreen *scr = WMWidgetScreen(_win);
    static WMTableViewDelegate delegate = {
	NULL,
	    numberOfRows,
	    valueForCell,
	    NULL
    };
    
    WMResizeWidget(_win, 600, 200);

    WMSetBoxHorizontal(_topBox, False);
     
    
    table = WMCreateTableView(_topBox);
    WMSetTableViewDataSource(table, this);
    WMSetTableViewBackgroundColor(table, WMWhiteColor(scr));
    WMSetTableViewGridColor(table, WMGrayColor(scr));
    WMSetTableViewHeaderHeight(table, 18);
    WMSetTableViewDelegate(table, &delegate);
    WMAddBoxSubview(_topBox, WMWidgetView(table), True, True, 200, 0, 5);

    {
	WMTableColumn *col;
	
    	col = WMCreateTableColumn("");
	WMAddTableViewColumn(table, col);
	WMSetTableColumnWidth(col, 20);
//	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)0);
	
	col = WMCreateTableColumn(_("Type"));
	WMSetTableColumnWidth(col, 50);
	WMAddTableViewColumn(table, col);
//	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)1);	
	
	col = WMCreateTableColumn(_("Vendor"));
	WMSetTableColumnWidth(col, 54);	
	WMAddTableViewColumn(table, col);
//	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)2);
	
	col = WMCreateTableColumn(_("URI"));
	WMSetTableColumnWidth(col, 250);
	WMAddTableViewColumn(table, col);
//	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)4);
	
	col = WMCreateTableColumn(_("Distribution"));
	WMSetTableColumnWidth(col, 80);
	WMAddTableViewColumn(table, col);
//	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)3);
	
	col = WMCreateTableColumn(_("Components"));
	WMSetTableColumnWidth(col, 100);
	WMAddTableViewColumn(table, col);
//	WMSetTableColumnDelegate(col, colDeleg);
	WMSetTableColumnId(col, (void*)4);	
    }

   
    hbox = WMCreateBox(_topBox);
    WMSetBoxHorizontal(hbox, True);
    WMAddBoxSubview(_topBox, WMWidgetView(hbox), False, True, 24, 0, 0);

    btn = makeButton(hbox, up_xpm);
    WMAddBoxSubview(hbox, WMWidgetView(btn), False, True, 24, 0, 3);

    btn = makeButton(hbox, down_xpm);
    WMAddBoxSubview(hbox, WMWidgetView(btn), False, True, 24, 0, 10);

    

    btn = WMCreateCommandButton(hbox);
    WMSetButtonText(btn, _("Add Repository"));
    WMAddBoxSubview(hbox, WMWidgetView(btn), False, True, 120, 0, 5);

    btn = WMCreateCommandButton(hbox);
    WMSetButtonText(btn, _("Add CD-ROM"));
    WMAddBoxSubview(hbox, WMWidgetView(btn), False, True, 120, 0, 5);
    
    btn = WMCreateCommandButton(hbox);
    WMSetButtonText(btn, _("Remove"));
    WMAddBoxSubview(hbox, WMWidgetView(btn), False, True, 100, 0, 5);

    btn = WMCreateCommandButton(hbox);
    WMSetButtonText(btn, _("Close"));
    WMAddBoxSubviewAtEnd(hbox, WMWidgetView(btn), False, True, 100, 0, 0);
    
    WMMapSubwidgets(_topBox);
    WMMapSubwidgets(_win);
    
    setTitle(_("Setup Package Repositories (still not working!)"));
    
    WMRealizeWidget(_win);
}

