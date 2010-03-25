/* rguserdialog.cc
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2003 Michael Vogt
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

#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>

#include <assert.h>
#include <string>
#include "i18n.h"
#include "rguserdialog.h"
#include "rgutils.h"

static void actionResponse(GtkDialog *dialog, gint id, gpointer user_data)
{
   GtkResponseType *res = (GtkResponseType *) user_data;
   *res = (GtkResponseType) id;
}

bool RGUserDialog::showErrors()
{
   GtkWidget *dia;
   std::string err,warn;
   
   if (_error->empty())
      return false;

   while (!_error->empty()) {
      string message;
      bool iserror = _error->PopMessage(message);

      // Ignore some stupid error messages.
      if (message == "Tried to dequeue a fetching object")
         continue;

      if (!message.empty()) {
         if (iserror)
            err = err + "E: " + message+ "\n";
         else
            warn = warn + "W: " + message+ "\n";
      }
   }
   string msg;
   GtkMessageType msg_type = GTK_MESSAGE_ERROR;
   if(err.empty()) {
      msg_type = GTK_MESSAGE_WARNING;
      msg = warn;
   } else
      msg = err + "\n"+ warn;
   dia = gtk_message_dialog_new(GTK_WINDOW(_parentWindow),
                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                msg_type, GTK_BUTTONS_CLOSE,
				NULL);
   gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(dia),
				  g_strdup_printf("<b><big>%s</big></b>\n\n%s",
				                  _("An error occurred"), 
				                  _("The following details "
						    "are provided:")));
   gtk_widget_set_size_request(dia, 500, 300);
   gtk_window_set_resizable(GTK_WINDOW(dia), TRUE);
   gtk_container_set_border_width(GTK_CONTAINER(dia), 6);
   GtkWidget *scroll = gtk_scrolled_window_new(NULL,NULL);
   GtkWidget *textview = gtk_text_view_new();
   GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
   gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer),utf8(msg.c_str()), -1);
   gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
   gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), 3);
   gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview), FALSE);
   gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), 
                                       GTK_SHADOW_IN);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
   gtk_container_set_border_width(GTK_CONTAINER(scroll), 6);
   gtk_container_add(GTK_CONTAINER(scroll), textview);
   gtk_widget_show_all(scroll);
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dia)->vbox), scroll);

   // honor foreign parent windows (to make embedding easy)
   int id = _config->FindI("Volatile::ParentWindowId", -1);
   if (id > 0) {
      GdkWindow *win = gdk_window_foreign_new(id);
      if(win) {
	 gtk_widget_realize(dia);
	 gdk_window_set_transient_for(GDK_WINDOW(dia->window), win);
      }
   }

   gtk_dialog_run(GTK_DIALOG(dia));
   gtk_widget_destroy(dia);

   return true;
}


bool RGUserDialog::message(const char *msg,
                           RUserDialog::DialogType dialog,
                           RUserDialog::ButtonsType buttons, bool defres)
{
   GtkWidget *dia;
   GtkResponseType res;
   GtkMessageType gtkmessage;
   GtkButtonsType gtkbuttons;

   switch (dialog) {
      case RUserDialog::DialogInfo:
         gtkmessage = GTK_MESSAGE_INFO;
         gtkbuttons = GTK_BUTTONS_CLOSE;
         break;
      case RUserDialog::DialogWarning:
         gtkmessage = GTK_MESSAGE_WARNING;
         gtkbuttons = GTK_BUTTONS_CLOSE;
         break;
      case RUserDialog::DialogError:
         gtkmessage = GTK_MESSAGE_ERROR;
         gtkbuttons = GTK_BUTTONS_CLOSE;
         break;
      case RUserDialog::DialogQuestion:
         gtkmessage = GTK_MESSAGE_QUESTION;
         gtkbuttons = GTK_BUTTONS_YES_NO;
         break;
   }

   switch (buttons) {
      case RUserDialog::ButtonsDefault:
         break;
      case RUserDialog::ButtonsOk:
         gtkbuttons = GTK_BUTTONS_CLOSE;
         break;
      case RUserDialog::ButtonsOkCancel:
         gtkbuttons = GTK_BUTTONS_OK_CANCEL;
         break;
      case RUserDialog::ButtonsYesNo:
         gtkbuttons = GTK_BUTTONS_YES_NO;
         break;
   }

   dia = gtk_message_dialog_new (GTK_WINDOW(_parentWindow),
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 gtkmessage, gtkbuttons, "%s", 
			         NULL);
   
   gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(dia), utf8(msg));
   gtk_container_set_border_width(GTK_CONTAINER(dia), 6);

   if (defres) {
      switch (buttons) {
         case RUserDialog::ButtonsOkCancel:
            gtk_dialog_set_default_response(GTK_DIALOG(dia), GTK_RESPONSE_OK);
            break;
         case RUserDialog::ButtonsYesNo:
            gtk_dialog_set_default_response(GTK_DIALOG(dia), GTK_RESPONSE_YES);
            break;
      }
   } else {
      switch (buttons) {
         case RUserDialog::ButtonsOkCancel:
            gtk_dialog_set_default_response(GTK_DIALOG(dia),
                                            GTK_RESPONSE_CANCEL);
            break;
         case RUserDialog::ButtonsYesNo:
            gtk_dialog_set_default_response(GTK_DIALOG(dia), GTK_RESPONSE_NO);
            break;
      }
   }

   g_signal_connect(GTK_OBJECT(dia), "response",
                    G_CALLBACK(actionResponse), (gpointer) & res);

   // honor foreign parent windows (to make embedding easy)
   int id = _config->FindI("Volatile::ParentWindowId", -1);
   if (id > 0) {
      GdkWindow *win = gdk_window_foreign_new(id);
      if(win) {
	 gtk_widget_realize(dia);
	 gdk_window_set_transient_for(GDK_WINDOW(dia->window), win);
      }
   }

   gtk_dialog_run(GTK_DIALOG(dia));
   gtk_widget_destroy(dia);
   return (res == GTK_RESPONSE_OK) || (res == GTK_RESPONSE_YES) || (res == GTK_RESPONSE_CLOSE);
}

// RGGladeUserDialog
RGGladeUserDialog::RGGladeUserDialog(RGWindow *parent, const char *name)
{
   _parentWindow = parent->window();
   init(name);
}

bool RGGladeUserDialog::init(const char *name)
{
   gchar *filename = NULL;
   gchar *main_widget = NULL;

   //cout << "RGGladeUserDialog::RGGladeUserDialog()" << endl;

   filename = g_strdup_printf("dialog_%s.glade", name);
   main_widget = g_strdup_printf("dialog_%s", name);
   if (FileExists(filename)) {
      gladeXML = glade_xml_new(filename, main_widget, NULL);
   } else {
      g_free(filename);
      filename = g_strdup_printf(SYNAPTIC_GLADEDIR "dialog_%s.glade", name);
      gladeXML = glade_xml_new(filename, main_widget, NULL);
   }
   assert(gladeXML);
   _dialog = glade_xml_get_widget(gladeXML, main_widget);
   assert(_dialog);

   gtk_window_set_position(GTK_WINDOW(_dialog),
			   GTK_WIN_POS_CENTER_ON_PARENT);
   if(gtk_window_get_modal(GTK_WINDOW(_dialog)))
      gtk_window_set_skip_taskbar_hint(GTK_WINDOW(_dialog), TRUE);
   gtk_window_set_transient_for(GTK_WINDOW(_dialog), 
				GTK_WINDOW(_parentWindow));

   // honor foreign parent windows (to make embedding easy)
   int id = _config->FindI("Volatile::ParentWindowId", -1);
   if (id > 0 && _parentWindow == NULL) {
      GdkWindow *win = gdk_window_foreign_new(id);
      if(win) {
	 gtk_widget_realize(_dialog);
	 gdk_window_set_transient_for(GDK_WINDOW(_dialog->window), win);
      }
   }

   g_free(filename);
   g_free(main_widget);
}

int RGGladeUserDialog::run(const char *name,bool return_gtk_response)
{
   if(name != NULL)
      init(name);

   res = (GtkResponseType) gtk_dialog_run(GTK_DIALOG(_dialog));
   gtk_widget_hide(_dialog);

   if(return_gtk_response)
      return res;
   else
      return (res == GTK_RESPONSE_OK) || (res == GTK_RESPONSE_YES) || (res == GTK_RESPONSE_CLOSE);
}


// vim:sts=4:sw=4
