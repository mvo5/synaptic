/* rgsetoptwindow.cc
 *
 * Copyright (c) 2003 Michael Vogt
 *
 * Author: Gustavo Niemeyer <niemeyer@conectiva.com>
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

#include <apt-pkg/configuration.h>

#include "config.h"
#include "i18n.h"

#include "rgsetoptwindow.h"

void RGSetOptWindow::DoApply(GtkWindow *widget, void *data)
{
  RGSetOptWindow *me = (RGSetOptWindow*)data;

  GtkWidget *entry_name = glade_xml_get_widget(me->_gladeXML, "entry_name");
  GtkWidget *entry_value = glade_xml_get_widget(me->_gladeXML, "entry_value");

  const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
  const gchar *value = gtk_entry_get_text(GTK_ENTRY(entry_value));

  _config->Set(name, value);
}

void RGSetOptWindow::DoClose(GtkWindow *widget, void *data)
{
  RGSetOptWindow *me = (RGSetOptWindow*)data;

  me->hide();
}

RGSetOptWindow::RGSetOptWindow(RGWindow *win) 
    : RGWindow(win, "setopt", false, true, true)
{
  glade_xml_signal_connect_data(_gladeXML,
				"on_button_apply_clicked",
				G_CALLBACK(DoApply),
				this); 

  glade_xml_signal_connect_data(_gladeXML,
				"on_button_close_clicked",
				G_CALLBACK(DoClose),
				this); 

  setTitle(_("Set generic option"));
}

