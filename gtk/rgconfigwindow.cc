/* rgconfigwindow.cc
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
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

#include "config.h"
#include "i18n.h"

#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <gtk/gtk.h>
#include "rconfiguration.h"
#include "rgconfigwindow.h"
#include "rguserdialog.h"

extern GdkColor *StatusColors[];

void RGConfigWindow::saveAction(GtkWidget *self, void *data)
{
    RGConfigWindow *me = (RGConfigWindow*)data;
    bool newval;

    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[0])) ;
    _config->Set("Synaptic::UseRegexp", newval ? "true" : "false");
 
    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[2]));
    _config->Set("Synaptic::UseStatusColors",  newval ? "true" : "false");
    me->_mainWin->setColors(newval);

    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[3]));
    _config->Set("Synaptic::AskRelated",  newval ? "true" : "false");
    
    bool postClean, postAutoClean;
    
    postClean = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_cacheB[1]));
    postAutoClean = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_cacheB[2]));

  newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[4]));
  _config->Set("Synaptic::UseTerminal",  newval ? "true" : "false");

  postClean = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_cacheB[1]));
  postAutoClean = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_cacheB[2]));


    if (!RWriteConfigFile(*_config)) {
	_error->Error(_("An error occurred while saving configurations."));
	RGUserDialog userDialog(me);
	userDialog.showErrors();
    }
}


void RGConfigWindow::closeAction(GtkWidget *self, void *data)
{
    RGConfigWindow *me = (RGConfigWindow*)data;
    me->close();
}

void RGConfigWindow::doneAction(GtkWidget *self, void *data)
{
    RGConfigWindow *me = (RGConfigWindow*)data;
    me->saveAction(self, data);
    me->closeAction(self, data);
}

void RGConfigWindow::show()
{
    string str;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[0]),
				 _config->FindB("Synaptic::UseRegexp", false));
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[2]),
				 _config->FindB("Synaptic::UseStatusColors", 
						true));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[3]),
				 _config->FindB("Synaptic::AskRelated", 
						true));

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[2]),
			       _config->FindB("Synaptic::UseStatusColors", 
					      true));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[3]),
			       _config->FindB("Synaptic::AskRelated", 
					      true));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[4]),
			       _config->FindB("Synaptic::AskRelated", 
					      true));

  bool UseTerminal = false;
#ifndef HAVE_ZVT
  gtk_widget_set_sensitive(GTK_WIDGET(_optionB[4]), false);
  _config->Set("Synaptic::UseTerminal", false);
#else
#ifndef HAVE_RPM
  UseTerminal = true;
#endif
#endif
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[4]),
			       _config->FindB("Synaptic::UseTerminal", 
					      UseTerminal));

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


void RGConfigWindow::readColors()
{
    GdkColor color;
    GtkWidget *button=NULL;

    char *buttons[] = {
	"button_keep_color",
	"button_install_color",
	"button_upgrade_color",
	"button_downgrade_color",
	"button_remove_color",
	"button_held_color",
	"button_broken_color",
	"button_pin_color",
	"button_new_color"
    };

    for(int i=0; i < 9; i++) {
	button = glade_xml_get_widget(_gladeXML, buttons[i]);
	cout << "button: " << buttons[i] << endl;
	assert(button);
	if(StatusColors[i] != NULL) {
	    color = *StatusColors[i]; 
	    gtk_widget_modify_bg(button, GTK_STATE_PRELIGHT, &color);
	    gtk_widget_modify_bg(button, GTK_STATE_NORMAL, &color);
	}
    }
}

RGConfigWindow::RGConfigWindow(RGWindow *win) 
    : RGWindow(win, "options", false, true, true)
{
    GtkWidget *button;
    GtkWidget *box;
    
    _optionB[0] = glade_xml_get_widget(_gladeXML, "check_regexp");
    _optionB[1] = glade_xml_get_widget(_gladeXML, "check_text_only");
    _optionB[2] = glade_xml_get_widget(_gladeXML, "check_use_colors");
    _optionB[3] = glade_xml_get_widget(_gladeXML, "check_ask_related");
    _optionB[4] = glade_xml_get_widget(_gladeXML, "check_terminal");

    _cacheB[0] = glade_xml_get_widget(_gladeXML, "radio_cache_leave");
    _cacheB[1] = glade_xml_get_widget(_gladeXML, "radio_cache_del_after");
    _cacheB[2] = glade_xml_get_widget(_gladeXML, "radio_cache_del_obsolete");
    _mainWin = (RGMainWindow*)win;

    readColors();
    
    glade_xml_signal_connect_data(_gladeXML,
				  "on_close_clicked",
				  G_CALLBACK(closeAction),
				  this); 
    glade_xml_signal_connect_data(_gladeXML,
				  "on_apply_clicked",
				  G_CALLBACK(saveAction),
				  this); 
    glade_xml_signal_connect_data(_gladeXML,
				  "on_ok_clicked",
				  G_CALLBACK(doneAction),
				  this); 
}
