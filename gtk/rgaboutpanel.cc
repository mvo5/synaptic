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

#include <cstring>
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
RGGtkBuilderWindow(parent, "about", "credits")
{
   GtkWidget *closebutton;
   closebutton = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                   "credits_closebutton"));
   g_signal_connect(G_OBJECT(closebutton),
                    "clicked",
                    G_CALLBACK(closeWindow), this);

   //   skipTaskbar(true);

   // hide translators credits if it is not found in the po file
   GtkWidget *credits;
   credits = GTK_WIDGET(gtk_builder_get_object(_builder,"label_translator_credits"));

   assert(credits);
   const char* s = gtk_label_get_text(GTK_LABEL(credits));
   if(strcmp(s, "translators-credits") == 0)
     gtk_widget_hide(credits);
};


void RGAboutPanel::creditsClicked(GtkWidget *self, void *data)
{
   RGAboutPanel *me = (RGAboutPanel *) data;

   if (me->credits == NULL) {
      me->credits = new RGCreditsPanel(me);
   }
   me->credits->setTitle(_("Credits"));

   me->credits->show();
}


RGAboutPanel::RGAboutPanel(RGWindow *parent)
: RGGtkBuilderWindow(parent, "about"), credits(NULL)
{
   GtkWidget *okbutton, *button_credits;
   okbutton = GTK_WIDGET(gtk_builder_get_object(_builder, "about_okbutton"));
   button_credits = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                      "button_credits"));
   g_signal_connect(G_OBJECT(okbutton),
                      "clicked",
                      G_CALLBACK(closeWindow), this);
   g_signal_connect(G_OBJECT(button_credits),
                      "clicked",
                      G_CALLBACK(creditsClicked), this);

   //skipTaskbar(true);

   setTitle(_("About Synaptic"));
   GtkWidget *w = GTK_WIDGET(gtk_builder_get_object(_builder, "label_version"));
   assert(w);

   gchar *s =
      g_strdup_printf("<span size=\"xx-large\" weight=\"bold\">%s</span>",
                      PACKAGE " " VERSION);
   gtk_label_set_markup(GTK_LABEL(w), s);

   g_free(s);
}
