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
#include "gsynaptic.h"

extern GdkColor *StatusColors[];

char * RGConfigWindow::color_buttons[] = {
      "button_keep_color",
      "button_install_color",
      "button_upgrade_color",
      "button_downgrade_color",
      "button_remove_color",
      "button_held_color",
      "button_broken_color",
      "button_pin_color",
      "button_new_color",
      NULL
  };


void RGConfigWindow::saveAction(GtkWidget *self, void *data)
{
    RGConfigWindow *me = (RGConfigWindow*)data;
    bool newval, postClean, postAutoClean;

    postClean = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_cacheB[1]));
    postAutoClean = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_cacheB[2]));

    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[0])) ;
    _config->Set("Synaptic::UseRegexp", newval ? "true" : "false");
 
    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[2]));
    _config->Set("Synaptic::AskRelated",  newval ? "true" : "false");
    

    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[3]));
    _config->Set("Synaptic::UseTerminal",  newval ? "true" : "false");

    int maxUndo = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(me->_maxUndoE));
    _config->Set("Synaptic::undoStackSize",  maxUndo);
    

    /* color stuff */
    char *colstr;
    colstr = gtk_get_string_from_color(StatusColors[(int)RPackage::MKeep]);
    _config->Set("Synaptic::MKeepColor", colstr);
    
    colstr = gtk_get_string_from_color(StatusColors[(int)RPackage::MInstall]);
    _config->Set("Synaptic::MInstallColor", colstr);
    
    colstr = gtk_get_string_from_color(StatusColors[(int)RPackage::MUpgrade]);
    _config->Set("Synaptic::MUpgradeColor", colstr);
    
    colstr =gtk_get_string_from_color(StatusColors[(int)RPackage::MDowngrade]);
    _config->Set("Synaptic::MDowngradeColor", colstr);
    
    colstr = gtk_get_string_from_color(StatusColors[(int)RPackage::MRemove]);
    _config->Set("Synaptic::MRemoveColor", colstr);
    
    colstr = gtk_get_string_from_color(StatusColors[(int)RPackage::MHeld]);
    _config->Set("Synaptic::MHeldColor", colstr);
    
    colstr = gtk_get_string_from_color(StatusColors[(int)RPackage::MBroken]);
    _config->Set("Synaptic::MBrokenColor", colstr);
    
    colstr = gtk_get_string_from_color(StatusColors[(int)RPackage::MPinned]);
    _config->Set("Synaptic::MPinColor", colstr);
    
    colstr = gtk_get_string_from_color(StatusColors[(int)RPackage::MNew]);
    _config->Set("Synaptic::MNewColor", colstr);

    /* now reread the colors */
    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[1]));
    _config->Set("Synaptic::UseStatusColors",  newval ? "true" : "false");
    me->_mainWin->setColors(newval);


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
    bool postClean, postAutoClean;    
    string str;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[0]),
				 _config->FindB("Synaptic::UseRegexp", false));
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[1]),
				 _config->FindB("Synaptic::UseStatusColors", 
						true));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[2]),
				 _config->FindB("Synaptic::AskRelated", 
						true));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[3]),
				 _config->FindB("Synaptic::AskRelated", 
						true));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(_maxUndoE), 
			      _config->FindI("Synaptic::undoStackSize", 20));

    bool UseTerminal = false;
#ifndef HAVE_ZVT
    gtk_widget_set_sensitive(GTK_WIDGET(_optionB[4]), false);
    _config->Set("Synaptic::UseTerminal", false);
#else
#ifndef HAVE_RPM
    UseTerminal = true;
#endif
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[3]),
				 _config->FindB("Synaptic::UseTerminal", 
						UseTerminal));

    postClean = _config->FindB("Synaptic::CleanCache", false);
    postAutoClean = _config->FindB("Synaptic::AutoCleanCache", false);
    
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

    for(int i=0; color_buttons[i] != NULL; i++) {
	button = glade_xml_get_widget(_gladeXML, color_buttons[i]);
	assert(button);
	if(StatusColors[i] != NULL) {
	    color = *StatusColors[i]; 
	    gtk_widget_modify_bg(button, GTK_STATE_PRELIGHT, &color);
	    gtk_widget_modify_bg(button, GTK_STATE_NORMAL, &color);
	}
    }
}

void RGConfigWindow::saveColor(GtkWidget *self, void *data) 
{
    GdkColor color;
    GtkColorSelectionDialog *color_selector;

    RGConfigWindow *me = (RGConfigWindow*)g_object_get_data(G_OBJECT(self), "me");
    color_selector = (GtkColorSelectionDialog*)g_object_get_data(G_OBJECT(self), "color_selector");
    

    gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(color_selector->colorsel), &color);

    StatusColors[GPOINTER_TO_INT(data)] = gdk_color_copy(&color);
    me->readColors();
}

void RGConfigWindow::colorClicked(GtkWidget *self, void *data) 
{
    GtkWidget *color_dialog;
    GtkWidget *color_selection;
    GtkWidget *ok_button, *cancel_button;
    RGConfigWindow *me;

    me = (RGConfigWindow*)g_object_get_data(G_OBJECT(self), "me");

    color_dialog = gtk_color_selection_dialog_new(_("Color selection"));
    ok_button = GTK_COLOR_SELECTION_DIALOG(color_dialog)->ok_button;
    cancel_button = GTK_COLOR_SELECTION_DIALOG(color_dialog)->cancel_button;
    color_selection = GTK_COLOR_SELECTION_DIALOG(color_dialog)->colorsel;
    if(StatusColors[GPOINTER_TO_INT(data)] != NULL)
	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(color_selection),
					      StatusColors[GPOINTER_TO_INT(data)]);

    g_object_set_data(G_OBJECT(ok_button), "color_selector", color_dialog);
    g_object_set_data(G_OBJECT(ok_button), "me", me);

    g_signal_connect(GTK_OBJECT(ok_button), "clicked", 
		     G_CALLBACK(saveColor), data);

    g_signal_connect_swapped(GTK_OBJECT(ok_button), "clicked",
                             G_CALLBACK(gtk_widget_destroy), 
                             (gpointer)color_dialog); 

    g_signal_connect_swapped(GTK_OBJECT(cancel_button),
                             "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             (gpointer)color_dialog); 
    
    gtk_widget_show(color_dialog);
}

RGConfigWindow::RGConfigWindow(RGWindow *win) 
    : RGWindow(win, "options", false, true, true)
{
    GtkWidget *button;
    
    _optionB[0] = glade_xml_get_widget(_gladeXML, "check_regexp");
    _optionB[1] = glade_xml_get_widget(_gladeXML, "check_use_colors");
    _optionB[2] = glade_xml_get_widget(_gladeXML, "check_ask_related");
    _optionB[3] = glade_xml_get_widget(_gladeXML, "check_terminal");

    _cacheB[0] = glade_xml_get_widget(_gladeXML, "radio_cache_leave");
    _cacheB[1] = glade_xml_get_widget(_gladeXML, "radio_cache_del_after");
    _cacheB[2] = glade_xml_get_widget(_gladeXML, "radio_cache_del_obsolete");
    _mainWin = (RGMainWindow*)win;

    _maxUndoE = glade_xml_get_widget(_gladeXML, "spinbutton_max_undos");
    assert(_maxUndoE);

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

    for(int i=0; color_buttons[i] != NULL; i++) {
	button = glade_xml_get_widget(_gladeXML, color_buttons[i]);
	g_object_set_data(G_OBJECT(button), "me", this);
	assert(button);
	g_signal_connect(G_OBJECT(button), "clicked", 
			 G_CALLBACK(colorClicked), GINT_TO_POINTER(i));
    }


}
