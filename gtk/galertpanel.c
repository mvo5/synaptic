/* galerpanel.c
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

#include "galertpanel.h"




static void
buttonCallback(GtkWidget *widget, void *data)
{
    *(int*)data = 1;
    
    gtk_main_quit();
}

int gtk_run_alert_panel(GtkWidget *parent, const char *title,
			const char *message,
			const char *defaultButton,
			const char *alternateButton,
			const char *otherButton)
{
    PangoFontDescription *font_desc;
    GtkWidget *win;
    GtkWidget *label;
    GtkWidget *button;
    int defClicked = 0;
    int altClicked = 0;
    int othClicked = 0;
    
    win = gtk_dialog_new();
    gtk_widget_set_usize(win, 420, 160);
    gtk_window_set_title(GTK_WINDOW(win), title);
    
    label = gtk_label_new(title);
    font_desc = pango_font_description_from_string ("helvetica bold 24");
    gtk_widget_modify_font (label, font_desc);
    pango_font_description_free (font_desc);
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(win)->vbox), label);
    label = gtk_label_new(message);
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(win)->vbox), label);
    
    if (defaultButton) {
	button = gtk_button_new_with_label(defaultButton);
	gtk_widget_show(button);
	
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(win)->action_area),
			  button);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(buttonCallback), &defClicked);
    }

    if (alternateButton) {
	button = gtk_button_new_with_label(alternateButton);
	gtk_widget_show(button);
	
	gtk_container_add(GTK_CONTAINER (GTK_DIALOG(win)->action_area), 
			  button);
	
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(buttonCallback), &altClicked);
    }

    if (otherButton) {
	button = gtk_button_new_with_label(otherButton);
	gtk_widget_show(button);
	
	gtk_container_add(GTK_CONTAINER (GTK_DIALOG(win)->action_area),
			   button);
	
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(buttonCallback), &othClicked);
    }

    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    if (parent) {
	    gtk_window_set_transient_for(GTK_WINDOW(win), 
					 GTK_WINDOW(parent));
    }

    gtk_widget_show_all(win);
    gtk_main(); 
    
    gtk_widget_hide(win); 
    gtk_widget_destroy(win); 
    
    
    if (othClicked) 
      return GTK_ALERT_OTHER; 
    
    if (altClicked) 
      return GTK_ALERT_ALTERNATE; 
    
    if (defClicked) 
      return GTK_ALERT_DEFAULT; 
    
    return GTK_ALERT_ERROR; 
}


int gtk_run_errors_panel(GtkWidget *parent, const char *title,
			 const char *message,
			 const char *defaultButton,
			 const char *alternateButton,
			 const char *otherButton)
{
    PangoFontDescription *font_desc;
    GtkWidget *win;
    GtkWidget *vbox;
    GtkWidget *topbox;
    GtkWidget *botbox;
    GtkWidget *label;
    GtkWidget *sep;
    GtkWidget *button;
    GtkWidget *sview;
    GtkWidget *text;
    GtkTextBuffer *buffer;
    int defClicked = 0;
    int altClicked = 0;
    int othClicked = 0;
    
    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_usize(win, 320, 240);
    gtk_window_set_title(GTK_WINDOW(win), title);
    
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(win), vbox);
    
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    
    label = gtk_label_new(title);
    gtk_widget_set_usize(label, -1, 64);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    font_desc = pango_font_description_from_string ("helvetica bold 24");
    gtk_widget_modify_font (label, font_desc);
    pango_font_description_free (font_desc);

    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
    
    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, TRUE, 0);
    
    sview = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(sview);
    gtk_box_pack_start(GTK_BOX(vbox), sview, TRUE, TRUE, 0);
    
    text = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    gtk_text_buffer_set_text (buffer, message, -1);

    gtk_widget_show(text);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sview), text);
    
    botbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_show(botbox);
    gtk_container_set_border_width(GTK_CONTAINER(botbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), botbox, FALSE, TRUE, 0);
    
    /* empty space filler */
    label = gtk_label_new(NULL);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(botbox), label, TRUE, TRUE, 0);
    
    if (defaultButton) {
	button = gtk_button_new_with_label(defaultButton);
	gtk_widget_show(button);
	
	gtk_box_pack_start(GTK_BOX(botbox), button, TRUE, TRUE, 0);
	
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			    GTK_SIGNAL_FUNC(buttonCallback), &defClicked);
    }

    if (alternateButton) {
	button = gtk_button_new_with_label(alternateButton);
	gtk_widget_show(button);
	
	gtk_box_pack_start(GTK_BOX(botbox), button, TRUE, TRUE, 0);
	
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(buttonCallback), &altClicked);
    }

    if (otherButton) {
	button = gtk_button_new_with_label(otherButton);
	gtk_widget_show(button);
	
	gtk_box_pack_start(GTK_BOX(botbox), button, TRUE, TRUE, 0);
	
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(buttonCallback), &othClicked);
    }

    

    gtk_window_set_modal(GTK_WINDOW(win), TRUE);

    gtk_widget_show(win);
    
    gtk_main();
    
    gtk_widget_hide(win);
    gtk_widget_destroy(win);


    if (othClicked)
	return GTK_ALERT_OTHER;
    
    if (altClicked)
	return GTK_ALERT_ALTERNATE;
    
    if (defClicked)
	return GTK_ALERT_DEFAULT;
    
    return GTK_ALERT_ERROR;
}



#if 0
int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    
    printf("%i\n",
	   gtk_run_alert_panel(NULL, "Test",
			       "Hello, click something",
			       "Ok", "Cancel", NULL));

    gtk_main();
}
#endif
