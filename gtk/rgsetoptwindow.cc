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
   RGSetOptWindow *me = (RGSetOptWindow *) data;

   GtkWidget *entry_name = GTK_WIDGET(gtk_builder_get_object(me->_builder, "entry_name"));
   GtkWidget *entry_value = GTK_WIDGET(gtk_builder_get_object(me->_builder, "entry_value"));

   const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
   const gchar *value = gtk_entry_get_text(GTK_ENTRY(entry_value));

   _config->Set(name, value);
}

void RGSetOptWindow::DoClose(GtkWindow *widget, void *data)
{
   RGSetOptWindow *me = (RGSetOptWindow *) data;

   me->hide();
}

RGSetOptWindow::RGSetOptWindow(RGWindow *win)
: RGGtkBuilderWindow(win, "setopt")
{
   g_signal_connect(GTK_WIDGET(gtk_builder_get_object
                               (_builder, "button_apply")),
                    "clicked",
                    G_CALLBACK(DoApply), this);

   g_signal_connect(GTK_WIDGET(gtk_builder_get_object
                               (_builder, "button_close")),
                    "clicked",
                    G_CALLBACK(DoClose), this);

   setTitle("");
   skipTaskbar(true);
}
