/* rguserdialog.cc
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


#include "rguserdialog.h"


bool RGUserDialog::confirm(const char *message)
{
      GtkWidget *dialog;
      GtkResponseType res;
      dialog = gtk_message_dialog_new(GTK_WINDOW(_parentWindow),
		      		      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_QUESTION,
				      GTK_BUTTONS_YES_NO,
				      "%s", message);
      g_signal_connect(GTK_OBJECT (dialog), "response",
		       G_CALLBACK (this->actionResponse),
		       (gpointer) &res);
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
      return (res == GTK_RESPONSE_YES);
}

void RGUserDialog::warning(const char *message)
{
      GtkWidget *dialog;
      GtkResponseType res;
      dialog = gtk_message_dialog_new(GTK_WINDOW(_parentWindow),
		      		      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_WARNING,
				      GTK_BUTTONS_OK,
				      "%s", message);
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
}

void RGUserDialog::info(const char *message)
{
      GtkWidget *dialog;
      GtkResponseType res;
      dialog = gtk_message_dialog_new(GTK_WINDOW(_parentWindow),
		      		      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_INFO,
				      GTK_BUTTONS_OK,
				      "%s", message);
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
}

void RGUserDialog::error(const char *message)
{
      GtkWidget *dialog;
      GtkResponseType res;
      dialog = gtk_message_dialog_new(GTK_WINDOW(_parentWindow),
		      		      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_ERROR,
				      GTK_BUTTONS_OK,
				      "%s", message);
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
}
