/* rgaboutpanel.cc
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

#include <cassert>
#include "config.h"
#include "rgaboutpanel.h"
#include "i18n.h"

static void closeWindow(GtkWidget *self, void *data)
{
   RGAboutPanel *me = (RGAboutPanel *) data;

   me->hide();
}

RGCreditsPanel::RGCreditsPanel(RGWindow *parent):
RGGladeWindow(parent, "about", "credits")
{
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_closebutton_clicked",
                                 G_CALLBACK(closeWindow), this);
};


void RGAboutPanel::creditsClicked(GtkWidget *self, void *data)
{
   RGAboutPanel *me = (RGAboutPanel *) data;

   if (me->credits == NULL) {
      me->credits = new RGCreditsPanel::RGCreditsPanel(me);
   }
   me->credits->setTitle(_("Credits"));
   me->credits->show();

}


RGAboutPanel::RGAboutPanel(RGWindow *parent)
: RGGladeWindow(parent, "about"), credits(NULL)
{
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_okbutton_clicked",
                                 G_CALLBACK(closeWindow), this);
   glade_xml_signal_connect_data(_gladeXML,
                                 "on_button_credits_clicked",
                                 G_CALLBACK(creditsClicked), this);

   setTitle(_("About Synaptic"));
   GtkWidget *w = glade_xml_get_widget(_gladeXML, "label_version");
   assert(w);
   gchar *s =
      g_strdup_printf("<span size=\"xx-large\" weight=\"bold\">%s</span>",
                      PACKAGE " " VERSION);
   gtk_label_set_markup(GTK_LABEL(w), s);
#ifndef HAVE_DEBTAGS
   gtk_widget_hide(glade_xml_get_widget(_gladeXML, "label_debtags"));
#endif
   g_free(s);
}
