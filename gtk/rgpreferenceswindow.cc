/* rgpreferenceswindow.cc
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

#include "config.h"
#include "i18n.h"

#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <gtk/gtk.h>
#include "rconfiguration.h"
#include "rgpreferenceswindow.h"
#include "rguserdialog.h"
#include "gsynaptic.h"

extern GdkColor *StatusColors[];

char * RGPreferencesWindow::color_buttons[] = {
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


void RGPreferencesWindow::saveAction(GtkWidget *self, void *data)
{
    RGPreferencesWindow *me = (RGPreferencesWindow*)data;
    bool newval;

    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_cacheB[1]));
    _config->Set("Synaptic::CleanCache", newval ? "true" : "false");
    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_cacheB[2]));
    _config->Set("Synaptic::AutoCleanCache", newval ? "true" : "false");

    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[0])) ;
    _config->Set("Synaptic::UseRegexp", newval ? "true" : "false");
 
    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[2]));
    _config->Set("Synaptic::AskRelated",  newval ? "true" : "false");
    
    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[3]));
    _config->Set("Synaptic::UseTerminal",  newval ? "true" : "false");

    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[4]));
    _config->Set("Synaptic::UseRecommends",  newval ? "true" : "false");

    newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_optionB[5]));
    _config->Set("Synaptic::AskQuitOnProceed",  newval ? "true" : "false");


    int maxUndo = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(me->_maxUndoE));
    _config->Set("Synaptic::undoStackSize",  maxUndo);
    
    int delAction = gtk_option_menu_get_history(GTK_OPTION_MENU(me->_optionmenuDel));
    // ugly :( but we need this +2 because RGPkgAction starts with 
    //         "keep","install"
    delAction += 2;
    _config->Set("Synaptic::delAction", delAction);

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


    // rebuild the treeview
    me->saveTreeViewValues();
    me->_mainWin->rebuildTreeView();


    if (!RWriteConfigFile(*_config)) {
	_error->Error(_("An error occurred while saving configurations."));
	RGUserDialog userDialog(me);
	userDialog.showErrors();
    }
}


void RGPreferencesWindow::closeAction(GtkWidget *self, void *data)
{
    RGPreferencesWindow *me = (RGPreferencesWindow*)data;
    me->close();
}

void RGPreferencesWindow::doneAction(GtkWidget *self, void *data)
{
    RGPreferencesWindow *me = (RGPreferencesWindow*)data;
    me->saveAction(self, data);
    me->closeAction(self, data);
}

void RGPreferencesWindow::clearCacheAction(GtkWidget *self, void *data)
{
    RGPreferencesWindow *me = (RGPreferencesWindow*)data;

    me->_lister->cleanPackageCache(true);
}


void RGPreferencesWindow::show()
{
    bool postClean, postAutoClean;    
    string str;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_cacheB[1]),
				 _config->FindB("Synaptic::CleanCache", false));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_cacheB[2]),
				 _config->FindB("Synaptic::AutoCleanCache", false));


    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[0]),
				 _config->FindB("Synaptic::UseRegexp", false));
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[1]),
				 _config->FindB("Synaptic::UseStatusColors", 
						true));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[2]),
				 _config->FindB("Synaptic::AskRelated", 
						true));
#ifdef HAVE_RPM
    int UndoStackSize = 3;
#else
    int UndoStackSize = 20;
#endif
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(_maxUndoE), 
			      _config->FindI("Synaptic::undoStackSize",
				      	     UndoStackSize));

    bool UseTerminal = false;
#ifndef HAVE_ZVT
    gtk_widget_set_sensitive(GTK_WIDGET(_optionB[3]), false);
    _config->Set("Synaptic::UseTerminal", false);
#else
#ifndef HAVE_RPM
    UseTerminal = true;
#endif
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[3]),
				 _config->FindB("Synaptic::UseTerminal", 
						UseTerminal));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[4]),
				 _config->FindB("Synaptic::UseRecommends",
					 	true));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionB[5]),
				 _config->FindB("Synaptic::AskQuitOnProceed", 
						false));

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

void RGPreferencesWindow::readTreeViewValues()
{
    GtkWidget *b;
    int pos;

    b = glade_xml_get_widget(_gladeXML, "spinbutton_status");
    assert(b);
    pos = _config->FindI("Synaptic::statusColumnPos",0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(b),pos);

    b = glade_xml_get_widget(_gladeXML, "spinbutton_name");
    assert(b);
    pos = _config->FindI("Synaptic::nameColumnPos",1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(b),pos);

    b = glade_xml_get_widget(_gladeXML, "spinbutton_instver");
    assert(b);
    pos = _config->FindI("Synaptic::instVerColumnPos",2);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(b),pos);
    
    b = glade_xml_get_widget(_gladeXML, "spinbutton_availver");
    assert(b);
    pos = _config->FindI("Synaptic::availVerColumnPos",3);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(b),pos);

    b = glade_xml_get_widget(_gladeXML, "spinbutton_instsize");
    assert(b);
    pos = _config->FindI("Synaptic::instSizeColumnPos",4);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(b),pos);

    b = glade_xml_get_widget(_gladeXML, "spinbutton_descr");
    assert(b);
    pos = _config->FindI("Synaptic::descrColumnPos",5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(b),pos);

}

void RGPreferencesWindow::saveTreeViewValues() 
{
    GtkWidget *b;
    int pos;

    b = glade_xml_get_widget(_gladeXML, "spinbutton_status");
    assert(b);
    pos = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(b));
    _config->Set("Synaptic::statusColumnPos",pos);

    b = glade_xml_get_widget(_gladeXML, "spinbutton_name");
    assert(b);
    pos = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(b));
    _config->Set("Synaptic::nameColumnPos",pos);

    b = glade_xml_get_widget(_gladeXML, "spinbutton_instver");
    assert(b);
    pos = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(b));
    _config->Set("Synaptic::instVerColumnPos",pos);

    b = glade_xml_get_widget(_gladeXML, "spinbutton_availver");
    assert(b);
    pos = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(b));
    _config->Set("Synaptic::availVerColumnPos",pos);

    b = glade_xml_get_widget(_gladeXML, "spinbutton_instsize");
    assert(b);
    pos = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(b));
    _config->Set("Synaptic::instSizeColumnPos",pos);


    b = glade_xml_get_widget(_gladeXML, "spinbutton_descr");
    assert(b);
    pos = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(b));
    _config->Set("Synaptic::descrColumnPos",pos);
  
}



void RGPreferencesWindow::readColors()
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

void RGPreferencesWindow::saveColor(GtkWidget *self, void *data) 
{
    GdkColor color;
    GtkColorSelectionDialog *color_selector;

    RGPreferencesWindow *me = (RGPreferencesWindow*)g_object_get_data(G_OBJECT(self), "me");
    color_selector = (GtkColorSelectionDialog*)g_object_get_data(G_OBJECT(self), "color_selector");
    

    gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(color_selector->colorsel), &color);

    StatusColors[GPOINTER_TO_INT(data)] = gdk_color_copy(&color);
    me->readColors();
}

void RGPreferencesWindow::colorClicked(GtkWidget *self, void *data) 
{
    GtkWidget *color_dialog;
    GtkWidget *color_selection;
    GtkWidget *ok_button, *cancel_button;
    RGPreferencesWindow *me;

    me = (RGPreferencesWindow*)g_object_get_data(G_OBJECT(self), "me");

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

RGPreferencesWindow::RGPreferencesWindow(RGWindow *win, RPackageLister *_lister) 
    : RGGladeWindow(win, "preferences")
{
    GtkWidget *button;
    
    _optionB[0] = glade_xml_get_widget(_gladeXML, "check_regexp");
    _optionB[1] = glade_xml_get_widget(_gladeXML, "check_use_colors");
    _optionB[2] = glade_xml_get_widget(_gladeXML, "check_ask_related");
    _optionB[3] = glade_xml_get_widget(_gladeXML, "check_terminal");
    _optionB[4] = glade_xml_get_widget(_gladeXML, "check_recommends");
    _optionB[5] = glade_xml_get_widget(_gladeXML, "check_ask_quit");

    _cacheB[0] = glade_xml_get_widget(_gladeXML, "radio_cache_leave");
    _cacheB[1] = glade_xml_get_widget(_gladeXML, "radio_cache_del_after");
    _cacheB[2] = glade_xml_get_widget(_gladeXML, "radio_cache_del_obsolete");
    _mainWin = (RGMainWindow*)win;

    _maxUndoE = glade_xml_get_widget(_gladeXML, "spinbutton_max_undos");
    assert(_maxUndoE);

    _optionmenuDel = glade_xml_get_widget(_gladeXML, "optionmenu_delbutton_action");
    assert(_optionmenuDel);
    int delAction = _config->FindI("Synaptic::delAction", PKG_DELETE);
    // ugly :( but we need this -2 because RGPkgAction starts with 
    //         "keep","install"
    gtk_option_menu_set_history(GTK_OPTION_MENU(_optionmenuDel),delAction-2);


    readColors();
    readTreeViewValues();

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

    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_clean_cache_clicked",
				  G_CALLBACK(clearCacheAction),
				  this); 


    for(int i=0; color_buttons[i] != NULL; i++) {
	button = glade_xml_get_widget(_gladeXML, color_buttons[i]);
	g_object_set_data(G_OBJECT(button), "me", this);
	assert(button);
	g_signal_connect(G_OBJECT(button), "clicked", 
			 G_CALLBACK(colorClicked), GINT_TO_POINTER(i));
    }

  setTitle(_("Set preferences"));
}
