/* rgwindow.cc
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


#include<apt-pkg/fileutl.h>
#include "config.h"
#include "i18n.h"
#include "rgwindow.h"


void RGWindow::windowCloseCallback(GtkWidget *window, GdkEvent *event)
{
    RGWindow *rwin = (RGWindow*)gtk_object_get_data(GTK_OBJECT(window), "me");
    
    rwin->close();
}

/* if you use glade, the convention is that the top window is named:
   "window_$windowname" in your glade-source 
*/
RGWindow::RGWindow(string name, bool makeBox, bool useGlade)
{
  if(!useGlade) {
    _win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  } else {
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
  };
   
  gtk_window_set_title(GTK_WINDOW(_win), (char*)name.c_str());

  gtk_object_set_data(GTK_OBJECT(_win), "me", this);
  gtk_signal_connect(GTK_OBJECT(_win), "delete-event",
		     (GtkSignalFunc)windowCloseCallback, this);

  if (makeBox) {
    _topBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(_win), _topBox);
    gtk_widget_show(_topBox);
    gtk_container_set_border_width(GTK_CONTAINER(_topBox), 5);
  } else {
    _topBox = NULL;
  }
  
  gtk_widget_realize(_win);
}


RGWindow::RGWindow(RGWindow *parent, string name, bool makeBox, 
		   bool closable, bool useGlade)
{
  if(!useGlade) {
    _win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  } else {
    // For development
    gchar *filename = NULL;
    // This code is leaking and must be fixed.
    filename = g_strdup_printf("window_%s.glade",name.c_str());
    if(FileExists(filename)) {
       _gladeXML = glade_xml_new(filename,
				 g_strdup_printf("window_%s",name.c_str()),
				 NULL);
    } else {
      g_free(filename);
      filename = g_strdup_printf(SYNAPTIC_GLADEDIR "window_%s.glade",name.c_str());
      _gladeXML = glade_xml_new(filename,
				g_strdup_printf("window_%s",name.c_str()),
				NULL);
    }
    assert(_gladeXML);
    _win = glade_xml_get_widget(_gladeXML, 
				g_strdup_printf ("window_%s",name.c_str()));
    assert(_win);
    g_free(filename);
  };
			  
  gtk_window_set_title(GTK_WINDOW(_win), (char*)name.c_str());

  gtk_object_set_data(GTK_OBJECT(_win), "me", this);

  gtk_signal_connect(GTK_OBJECT(_win), "delete-event",
		     (GtkSignalFunc)windowCloseCallback, this);

  if (makeBox) {
    _topBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(_win), _topBox);
    gtk_widget_show(_topBox);
    gtk_container_set_border_width(GTK_CONTAINER(_topBox), 5);
  } else {
    _topBox = NULL;
  }

  gtk_widget_realize(_win);
  
  gtk_window_set_transient_for(GTK_WINDOW(_win), 
			       GTK_WINDOW(parent->window()));
}


RGWindow::~RGWindow()
{
//    cout << "Desotry wind"<<endl;
    gtk_widget_destroy(_win);
}


void RGWindow::setTitle(string title)
{
    gtk_window_set_title(GTK_WINDOW(_win), (char*)title.c_str());
}

void RGWindow::close()
{
    hide();
}
