/* rgconfigwindow.cc
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

#include "config.h"
#include "i18n.h"

#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>

#include "rconfiguration.h"
#include "rgconfigwindow.h"

#include "galertpanel.h"


void RGConfigWindow::saveAction(GtkWidget *self, void *data)
{
  RGConfigWindow *me = (RGConfigWindow*)data;
  bool newval;

  newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[0])) ;
  _config->Set("Synaptic::UseRegexp", newval ? "true" : "false");
 
  newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[1]));
  _config->Set("Synaptic::UseStatusColors",  newval ? "true" : "false");

  bool postClean, postAutoClean;
    
  postClean = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_cacheB[1]));
  postAutoClean = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_cacheB[2]));

  _config->Set("Synaptic::CleanCache", postClean ? "true" : "false");
  _config->Set("Synaptic::AutoCleanCache", postAutoClean ? "true" : "false");
    

  if (!RWriteConfigFile(*_config)) {
    _error->DumpErrors();
    gtk_run_alert_panel(me->_win,
			_("Error"), _("An error occurred while saving configurations."),
			_("OK"), NULL, NULL);
  }
}


void RGConfigWindow::closeAction(GtkWidget *self, void *data)
{
  RGConfigWindow *me = (RGConfigWindow*)data;
    
  me->close();
}



void RGConfigWindow::show()
{
  string str;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[0]),
			       _config->FindB("Synaptic::UseRegexp", false));

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[1]),
			       _config->FindB("Synaptic::UseStatusColors", 
					      true));

  bool postClean = _config->FindB("Synaptic::CleanCache", false);
  bool postAutoClean = _config->FindB("Synaptic::AutoCleanCache", false);
    
  if (postClean)
    gtk_button_clicked(GTK_BUTTON(_cacheB[1]));
  else if (postAutoClean)
    gtk_button_clicked(GTK_BUTTON(_cacheB[2]));
  else
    gtk_button_clicked(GTK_BUTTON(_cacheB[0]));


  RGWindow::show();
}




RGConfigWindow::RGConfigWindow(RGWindow *win) 
    : RGWindow(win, "options")
{
    GtkWidget *button;
    GtkWidget *box;
    int b = -1;

    //gtk_widget_set_usize(_win, 400, 250);

    GtkWidget *tab;
    
    tab = gtk_notebook_new();
    gtk_widget_show(tab);
    gtk_box_pack_start(GTK_BOX(_topBox), tab, TRUE, TRUE, 5);
    box = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    
    {
	_optionB[++b] = gtk_check_button_new_with_label(
			_("Use regular expressions during searches or matching"));
	gtk_box_pack_start(GTK_BOX(box), _optionB[b], FALSE, TRUE, 0);
	
	_optionB[++b] = gtk_check_button_new_with_label(
			_("Use colors in main package list\n(requires a restart to take effect)"));
	gtk_box_pack_start(GTK_BOX(box), _optionB[b], FALSE, TRUE, 0);


	gtk_widget_show_all(box);
	gtk_notebook_append_page(GTK_NOTEBOOK(tab), box, 
				 gtk_label_new(_("Options")));
    }

    {
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
/*
        GtkWidget *label;
	label = WMCreateLabel(vbox);
	WMSetLabelText(label, 
		       "Packages are stored in the cache when downloaded."
		       "");
 * 
	WMAddBoxSubview(vbox, GtkWidgetView(label), False, True, 40, 0, 5);
 */
	_cacheB[0] = button = gtk_radio_button_new_with_label(NULL,
				_("Leave downloaded packages in the cache"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);

	_cacheB[1] = button = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(button)),
				_("Delete downloaded packages after installation"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);

	_cacheB[2] = button = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(button)),
			       _("Delete obsoleted packages from cache"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);
	
/*	box = WMCreateBox(vbox);
	WMSetBoxHorizontal(box, True);	
	WMAddBoxSubviewAtEnd(vbox, GtkWidgetView(box), False, True, 20, 0, 0);

	label = WMCreateLabel(box);
	WMSetLabelText(label, "Max. Cache Size:");
	WMAddBoxSubview(box, GtkWidgetView(label), False, True, 120, 0, 0);
	
	_sizeT = WMCreateTextField(box);
	WMAddBoxSubview(box, GtkWidgetView(_sizeT), True, True, 80, 0, 2);
	
	label = WMCreateLabel(box);
	WMSetLabelText(label, "Mbytes");
	WMAddBoxSubview(box, GtkWidgetView(label), False, True, 50, 0, 5);

	button = WMCreateCommandButton(box);
	WMSetButtonText(button, "Clear Cache");
	WMAddBoxSubview(box, GtkWidgetView(button), False, True, 100, 0, 0);
	WMMapSubwidgets(box);

	
	box = WMCreateBox(vbox);
	WMSetBoxHorizontal(box, True);	
	WMAddBoxSubviewAtEnd(vbox, GtkWidgetView(box), False, True, 20, 0, 5);

	label = WMCreateLabel(box);
	WMSetLabelText(label, "Cache Directory:");
	WMAddBoxSubview(box, GtkWidgetView(label), False, True, 120, 0, 0);
	
	_pathT = WMCreateTextField(box);
	WMAddBoxSubview(box, GtkWidgetView(_pathT), True, True, 80, 0, 5);
	
	button = WMCreateCommandButton(box);
	WMSetButtonText(button, "Browse...");
	WMAddBoxSubview(box, GtkWidgetView(button), False, True, 70, 0, 0);

  */  
	gtk_widget_show_all(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(tab), vbox,
				 gtk_label_new(_("Cache")));
    }

    
    box = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(_topBox), box, FALSE, TRUE, 0);

    
    button = gtk_button_new_with_label(_("Close"));
    gtk_widget_set_usize(button, 80, -1);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       (GtkSignalFunc)closeAction, this);
    gtk_box_pack_end(GTK_BOX(box), button, FALSE, TRUE, 0);

    button = gtk_button_new_with_label(_("Save"));
    gtk_widget_set_usize(button, 80, -1) ;   
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       (GtkSignalFunc)saveAction, this);
    gtk_box_pack_end(GTK_BOX(box), button, FALSE, TRUE, 5);

    gtk_widget_show_all(box);
    gtk_widget_show(box);


    setTitle(_("Preferences"));
}


