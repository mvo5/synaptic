/* rgsummarywindow.cc
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

#include "config.h"
#include "i18n.h"

#include <X11/keysym.h>

#include <apt-pkg/configuration.h>
#include <apt-pkg/strutl.h>

#include "rpackagelister.h"

#include <stdio.h>
#include <string>

#include "rgsummarywindow.h"
#include "rguserdialog.h"



void RGSummaryWindow::clickedOk(GtkWidget *self, void *data)
{
    RGSummaryWindow *me = (RGSummaryWindow*)data;
    
    me->_confirmed = true;
    gtk_main_quit();
}


void RGSummaryWindow::clickedCancel(GtkWidget *self, void *data)
{
    RGSummaryWindow *me = (RGSummaryWindow*)data;

    me->_confirmed = false;
    gtk_main_quit();
}

void RGSummaryWindow::buildTree(RGSummaryWindow *me) 
{
  RPackageLister *lister = me->_lister;
  GtkTreeIter   iter, iter_child;
  
  vector<RPackage*> held;
  vector<RPackage*> kept;
  vector<RPackage*> essential;
  vector<RPackage*> toInstall;
  vector<RPackage*> toUpgrade;
  vector<RPackage*> toRemove;
  vector<RPackage*> toDowngrade;
  double sizeChange;
   
  lister->getDetailedSummary(held, kept, essential, toInstall, toUpgrade, 
			     toRemove, toDowngrade, sizeChange);

  if(essential.size() > 0) {
    /* (Essentail) removed */
    gtk_tree_store_append (me->_treeStore, &iter, NULL);  
    gtk_tree_store_set (me->_treeStore, &iter,
			PKG_COLUMN, _("(ESSENTIAL) to be removed"), -1);

    for (vector<RPackage*>::const_iterator p = essential.begin(); 
	 p != essential.end(); 
	 p++) 
      {
	gtk_tree_store_append (me->_treeStore, &iter_child, &iter);
	gtk_tree_store_set(me->_treeStore, &iter_child,
			   PKG_COLUMN,(*p)->name(), -1);
      }
  }
  if(toDowngrade.size() > 0) {
    /* removed */
    gtk_tree_store_append (me->_treeStore, &iter, NULL);  
    gtk_tree_store_set (me->_treeStore, &iter,
			PKG_COLUMN, _("To be DOWNGRADED"), -1);
    for (vector<RPackage*>::const_iterator p = toDowngrade.begin(); 
	 p != toDowngrade.end(); 
	 p++) 
      {
	gtk_tree_store_append (me->_treeStore, &iter_child, &iter);
	gtk_tree_store_set(me->_treeStore, &iter_child,
			   PKG_COLUMN,(*p)->name(), -1);
      }
  }

  if(toRemove.size() > 0) {
    /* removed */
    gtk_tree_store_append (me->_treeStore, &iter, NULL);  
    gtk_tree_store_set (me->_treeStore, &iter,
			PKG_COLUMN, _("To be removed"),	-1);
    for (vector<RPackage*>::const_iterator p = toRemove.begin(); 
	 p != toRemove.end(); 
	 p++) 
      {
	gtk_tree_store_append (me->_treeStore, &iter_child, &iter);
	gtk_tree_store_set(me->_treeStore, &iter_child,
			   PKG_COLUMN,(*p)->name(), -1);
      }
  }

  if(toUpgrade.size() > 0) {
    gtk_tree_store_append (me->_treeStore, &iter, NULL);  
    gtk_tree_store_set (me->_treeStore, &iter,
			PKG_COLUMN, _("To be upgraded"),-1);
    for (vector<RPackage*>::const_iterator p = toUpgrade.begin(); 
	 p != toUpgrade.end(); 
	 p++) 
      {
	gtk_tree_store_append (me->_treeStore, &iter_child, &iter);
	gtk_tree_store_set(me->_treeStore, &iter_child,
			   PKG_COLUMN,(*p)->name(), -1);
      }
  }

  if(toInstall.size() > 0) {
    gtk_tree_store_append (me->_treeStore, &iter, NULL);  
    gtk_tree_store_set (me->_treeStore, &iter,
			PKG_COLUMN, _("To be installed"),-1);
    for (vector<RPackage*>::const_iterator p = toInstall.begin(); 
	 p != toInstall.end(); 
	 p++) 
      {
	gtk_tree_store_append (me->_treeStore, &iter_child, &iter);
	gtk_tree_store_set(me->_treeStore, &iter_child,
			   PKG_COLUMN,(*p)->name(), -1);
      }
  }

  if(kept.size() > 0) {
    gtk_tree_store_append (me->_treeStore, &iter, NULL);  
    gtk_tree_store_set (me->_treeStore, &iter,
			PKG_COLUMN, _("To be kept back"),-1);
    for (vector<RPackage*>::const_iterator p = kept.begin(); 
	 p != kept.end(); 
	 p++) 
        {
	gtk_tree_store_append (me->_treeStore, &iter_child, &iter);
	gtk_tree_store_set(me->_treeStore, &iter_child,
			   PKG_COLUMN,(*p)->name(), -1);
        }
  }
}

void RGSummaryWindow::clickedDetails(GtkWidget *self, void *data)
{
    RGSummaryWindow *me = (RGSummaryWindow*)data;
    RPackageLister *lister = me->_lister;
    GtkWidget *view;
    GtkWidget *info;

    gtk_widget_set_sensitive(self, FALSE);
    gtk_widget_destroy(me->_tree);
    gtk_widget_hide(self);

    info = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(info), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(info), 0.0f, 0.0f);
    gtk_widget_show(info);
    
    view = glade_xml_get_widget(me->_gladeXML,"scrolledwindow_summary");
    assert(view);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(view), info);

    string text;
    gchar *str;
    
    vector<RPackage*> held;
    vector<RPackage*> kept;
    vector<RPackage*> essential;
    vector<RPackage*> toInstall;
    vector<RPackage*> toUpgrade;
    vector<RPackage*> toRemove;
    vector<RPackage*> toDowngrade;
    double sizeChange;
   
    lister->getDetailedSummary(held, 
			       kept, 
			       essential,
			       toInstall, 
			       toUpgrade, 
			       toRemove,
			       toDowngrade,
			       sizeChange);

    for (vector<RPackage*>::const_iterator p = essential.begin();
	 p != essential.end(); p++) {
	str = g_strdup_printf(_("%s: (ESSENTIAL) will be Removed\n"),(*p)->name());
	text += str;
	g_free(str);
    }
    
    for (vector<RPackage*>::const_iterator p = toDowngrade.begin(); 
	 p != toDowngrade.end(); p++) {
	str = g_strdup_printf(_("%s: will be DOWNGRADED\n"), (*p)->name());
	text += str;
	g_free(str);
    }
    
    for (vector<RPackage*>::const_iterator p = toRemove.begin(); 
	 p != toRemove.end(); p++) {
	str = g_strdup_printf(_("%s: will be Removed\n"), (*p)->name());
	text += str;
	g_free(str);
    }

    for (vector<RPackage*>::const_iterator p = toUpgrade.begin(); 
	 p != toUpgrade.end(); p++) {
	str = g_strdup_printf(_("%s %s: will be Upgraded to %s\n"),(*p)->name(),(*p)->installedVersion(),(*p)->availableVersion());
	text += str;
	g_free(str);
    }

    for (vector<RPackage*>::const_iterator p = toInstall.begin(); 
	 p != toInstall.end(); p++) {
	str = g_strdup_printf(_("%s %s: will be Installed\n"),(*p)->name(), (*p)->availableVersion());
	text += str;
	g_free(str);
    }
    
    gtk_label_set_text(GTK_LABEL(info), text.c_str());
    
    me->_summaryL = info;
}


RGSummaryWindow::RGSummaryWindow(RGWindow *wwin, RPackageLister *lister)
    : RGGladeWindow(wwin, "summary")
{
    GtkWidget *button;

    _potentialBreak = false;
    _lister = lister;
    
    setTitle(_("Summary"));
    //gtk_window_set_default_size(GTK_WINDOW(_win), 400, 250);
        

    _summaryL = glade_xml_get_widget(_gladeXML, "label_summary");
    assert(_summaryL);

    _summarySpaceL = glade_xml_get_widget(_gladeXML, "label_summary_space");
    assert(_summaryL);
    
    // new tree store
    _treeStore = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING);
    buildTree(this);
    _tree = glade_xml_get_widget(_gladeXML, "treeview_summary");
    assert(_tree);
    gtk_tree_view_set_model(GTK_TREE_VIEW(_tree), GTK_TREE_MODEL(_treeStore));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    //GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Summary",renderer, PKG_COLUMN, "pkg", NULL);
    GtkTreeViewColumn *column;
    column = gtk_tree_view_column_new_with_attributes (_("Queued Changes"), 
		    					renderer,
						       "text", PKG_COLUMN,
						       NULL);
   /* Add the column to the view. */
   gtk_tree_view_append_column (GTK_TREE_VIEW (_tree), column);
   gtk_widget_show (_tree);

   _dlonlyB = glade_xml_get_widget(_gladeXML, "checkbutton_download_only");



    button = glade_xml_get_widget(_gladeXML, "togglebutton_details");
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)clickedDetails, this);



    button = _defBtn = glade_xml_get_widget(_gladeXML, "button_execute");
    assert(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)clickedOk, this);

    button =  glade_xml_get_widget(_gladeXML, "button_cancel");
    assert(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       (GtkSignalFunc)clickedCancel, this);

    int toInstall, toRemove, toUpgrade, toDowngrade;
    int held, kept, essential;
    double sizeChange, dlSize;
    int dlCount;
    GString *msg = g_string_new("");
    GString *msg_space = g_string_new("");
	
    lister->getSummary(held, kept, essential,
		       toInstall, toUpgrade, toRemove, toDowngrade,
		       sizeChange);
    lister->getDownloadSummary(dlCount, dlSize);

    if (held)
	g_string_append_printf(msg,_("%d %s are set on hold\n"), 
			       held, held > 1 ? _("packages") : _("package"));
    if (kept)
	g_string_append_printf(msg,
			       _("%d %s are kept back and not upgraded\n"), 
			       kept, kept > 1 ? _("packages") : _("package"));

    if (toInstall)
	g_string_append_printf(msg,
			       _("%d new %s will be installed\n"), 
			       toInstall, 
			       toInstall > 1 ? _("packages") : _("package"));
    
    if (toUpgrade)
	g_string_append_printf(msg,_("%d %s will be upgraded\n"), toUpgrade,
			       toUpgrade > 1 ? _("packages") : _("package"));
    
    if (toRemove)
	g_string_append_printf(msg,_("%d %s will be removed\n"), toRemove,
			       toRemove > 1 ? _("packages") : _("package"));
    
    if (toDowngrade)
	g_string_append_printf(msg,_("%d %s will be DOWNGRADED\n"), 
			       toDowngrade, 
			       toDowngrade > 1 ? _("packages") : _("package"));
			   
    if (essential) {
	g_string_append_printf(msg,
			      _("WARNING: %d essential %s will be removed\n"),
			       essential, 
			       essential > 1 ? _("packages") : _("package"));
	_potentialBreak = true;
    }

    if (sizeChange > 0) {
	g_string_append_printf(msg_space,_("\n%s of extra space will be used"),
			       SizeToStr(sizeChange).c_str());
    } else if (sizeChange < 0) {
	g_string_append_printf(msg_space, _("\n%s of extra space will be freed"),
			       SizeToStr(sizeChange).c_str());
	sizeChange = -sizeChange;
    }
    
    if (dlSize > 0) {
	g_string_append_printf(msg_space, _("\n%s have to be downloaded"),
			       SizeToStr(dlSize).c_str());
    }
    
    gtk_label_set_text(GTK_LABEL(_summaryL), msg->str);
    gtk_label_set_text(GTK_LABEL(_summarySpaceL), msg_space->str);
    g_string_free(msg,TRUE);
}



bool RGSummaryWindow::showAndConfirm()
{    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_dlonlyB),
			_config->FindB("Synaptic::Download-Only", false));
    
    show();
    gtk_window_set_modal(GTK_WINDOW(_win), TRUE);
    gtk_main();

    RGUserDialog userDialog(this);
    if (_confirmed && _potentialBreak
	&& userDialog.warning(_("Essential packages will be removed.\n"
			  	"This may render your system unusable!\n")
			      , false) == false)
	return false;
    
    _config->Set("Synaptic::Download-Only",
		 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_dlonlyB)) ? "true" : "false");
    
    return _confirmed;
}

