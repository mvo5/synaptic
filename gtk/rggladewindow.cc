/* rggladewindow.cc
 *
 * Copyright (c) 2003 Michael Vogt
 *
 * Author: Michael Vogt <mvo@debian.irg>
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


#include <apt-pkg/fileutl.h>
#include <assert.h>
#include "config.h"
#include "i18n.h"
#include "rggladewindow.h"


/*  the convention is that the top window is named:
   "window_$windowname" in your glade-source 
*/
RGGladeWindow::RGGladeWindow(RGWindow *parent, string name)
{
    std::cout << "RGGladeWindow::RGGladeWindow(parent,name)" << endl;

    // for development
    gchar *filename = NULL;
    gchar *main_widget = NULL;
    
    filename = g_strdup_printf("window_%s.glade",name.c_str());
    main_widget = g_strdup_printf("window_%s", name.c_str());
    if(FileExists(filename)) {
	_gladeXML = glade_xml_new(filename, main_widget, NULL);
    } else {
	g_free(filename);
	filename = g_strdup_printf(SYNAPTIC_GLADEDIR "window_%s.glade",name.c_str());
	_gladeXML = glade_xml_new(filename, main_widget,	NULL);
    }
    assert(_gladeXML);
    _win = glade_xml_get_widget(_gladeXML, main_widget);

    
    assert(_win);
    g_free(filename);
    g_free(main_widget);
   
    gtk_window_set_title(GTK_WINDOW(_win), (char*)name.c_str());

    gtk_object_set_data(GTK_OBJECT(_win), "me", this);
    gtk_signal_connect(GTK_OBJECT(_win), "delete-event",
		       (GtkSignalFunc)windowCloseCallback, this);
    _topBox = NULL;
    gtk_widget_realize(_win);

    if(parent!=NULL)
	gtk_window_set_transient_for(GTK_WINDOW(_win), 
				     GTK_WINDOW(parent->window()));

}


