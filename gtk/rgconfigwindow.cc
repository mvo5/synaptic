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
 
  newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[2]));
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
    : RGWindow(win, "options", false, true, true)
{
    GtkWidget *button;
    GtkWidget *box;

    _optionB[0] = glade_xml_get_widget(_gladeXML, "check_regexp");
    _optionB[1] = glade_xml_get_widget(_gladeXML, "check_text_only");
    _optionB[2] = glade_xml_get_widget(_gladeXML, "check_use_colors");

    _cacheB[0] = glade_xml_get_widget(_gladeXML, "radio_cache_leave");
    _cacheB[1] = glade_xml_get_widget(_gladeXML, "radio_cache_del_after");
    _cacheB[2] = glade_xml_get_widget(_gladeXML, "radio_cache_del_obsolete");
    
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
