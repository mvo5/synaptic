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
    gtk_widget_destroy(me->_summaryL);
    gtk_widget_destroy(me->_middleF);

    gtk_box_set_child_packing(GTK_BOX(me->_topBox), 
			      me->_topF, 
			      TRUE, TRUE, 5, GTK_PACK_START);

    gtk_window_set_default_size(GTK_WINDOW(me->_win), 400, 440);

    gtk_frame_set_shadow_type(GTK_FRAME(me->_topF), GTK_SHADOW_IN);
    view = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(view), 
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    
    //TODO: make this a gtk_clist instead of a label
    info = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(info), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(info), 0.0f, 0.0f);
    gtk_widget_show(info);
    
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(view), info);
    gtk_container_add(GTK_CONTAINER(me->_topF), view);

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
    : RGWindow(wwin, "summary", true, false)
{
    GtkWidget *hbox;
    GtkWidget *button;
    GtkWidget *scrolled_win;
    
    _potentialBreak = false;
    _lister = lister;
    
    setTitle(_("Summary"));
    gtk_window_set_default_size(GTK_WINDOW(_win), 400, 250);
        
    _topF = gtk_frame_new(NULL);
    gtk_widget_show(_topF);
    gtk_box_pack_start(GTK_BOX(_topBox), _topF, FALSE, FALSE, 5);

    _summaryL = gtk_label_new(NULL);
    gtk_label_set_selectable(GTK_LABEL(_summaryL), true);
    gtk_widget_show(_summaryL);
    gtk_container_add(GTK_CONTAINER(_topF), _summaryL);

    
    _middleF = gtk_frame_new(NULL);
    scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_AUTOMATIC);
    gtk_widget_set_usize (scrolled_win, 150, 200);
    gtk_container_add (GTK_CONTAINER(_middleF), scrolled_win);
    gtk_widget_show (scrolled_win);

    // new tree store
    _treeStore = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING);
    buildTree(this);
    _tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrolled_win),
					   _tree);
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    //GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Summary",renderer, PKG_COLUMN, "pkg", NULL);
    GtkTreeViewColumn *column;
    column = gtk_tree_view_column_new_with_attributes (_("Package changes"), 
		    					renderer,
						       "text", PKG_COLUMN,
						       NULL);
   /* Add the column to the view. */
   gtk_tree_view_append_column (GTK_TREE_VIEW (_tree), column);
   gtk_widget_show (_tree);


    gtk_widget_show(_middleF);
    gtk_box_pack_start(GTK_BOX(_topBox), _middleF, TRUE, TRUE, 5);

    _dlonlyB = gtk_check_button_new_with_label(_("Perform package download only."));
    gtk_widget_show(_dlonlyB);
    gtk_box_pack_start(GTK_BOX(_topBox), _dlonlyB, FALSE, TRUE, 10);

    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(_topBox), hbox, FALSE, TRUE, 0);

    button = gtk_button_new_with_label(_("Show Details"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)clickedDetails, this);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 20);

    _defBtn = button = gtk_button_new_with_label(_("Proceed"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)clickedOk, this);

    gtk_box_pack_end(GTK_BOX(hbox), button, TRUE, TRUE, 0);

    button = gtk_button_new_with_label(_("Cancel"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       (GtkSignalFunc)clickedCancel, this);
    gtk_box_pack_end(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    gtk_widget_show_all(hbox);
    gtk_widget_show(hbox);

    gtk_widget_show(_topBox);


    int toInstall, toRemove, toUpgrade, toDowngrade;
    int held, kept, essential;
    double sizeChange, dlSize;
    int dlCount;
    GString *msg = g_string_new("");
	
    lister->getSummary(held, kept, essential,
		       toInstall, toUpgrade, toRemove, toDowngrade,
		       sizeChange);
    lister->getDownloadSummary(dlCount, dlSize);

    if (held)
	g_string_append_printf(msg,_("%d %s were held;\n"), 
			       held, held > 1 ? _("packages") : _("package"));
    if (kept)
	g_string_append_printf(msg,
			       _("%d %s were kept back and not upgraded;\n"), 
			       kept, kept > 1 ? _("packages") : _("package"));

    if (toInstall)
	g_string_append_printf(msg,
			       _("%d new %s will be installed;\n"), 
			       toInstall, 
			       toInstall > 1 ? _("packages") : _("package"));
    
    if (toUpgrade)
	g_string_append_printf(msg,_("%d %s will be upgraded;\n"), toUpgrade,
			       toUpgrade > 1 ? _("packages") : _("package"));
    
    if (toRemove)
	g_string_append_printf(msg,_("%d %s will be removed;\n"), toRemove,
			       toRemove > 1 ? _("packages") : _("package"));
    
    if (toDowngrade)
	g_string_append_printf(msg,_("%d %s will be DOWNGRADED;\n"), 
			       toDowngrade, 
			       toDowngrade > 1 ? _("packages") : _("package"));
			   
    if (essential) {
	g_string_append_printf(msg,
			      _("WARNING: %d essential %s will be removed!\n"),
			       essential, 
			       essential > 1 ? _("packages") : _("package"));
	_potentialBreak = true;
    }

    if (sizeChange > 0) {
	g_string_append_printf(msg, _("\n%s B will be used after finished.\n"),
			       SizeToStr(sizeChange).c_str());
    } else if (sizeChange < 0) {
	g_string_append_printf(msg, _("\n%s B will be freed after finished.\n"),
			       SizeToStr(sizeChange).c_str());
	sizeChange = -sizeChange;
    }
    
    if (dlSize > 0) {
	g_string_append_printf(msg, _("\n%s B need to be downloaded."),
			       SizeToStr(dlSize).c_str());
    }
    
    gtk_label_set_text(GTK_LABEL(_summaryL), msg->str);
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
			  	"That can render your system unusable!!!\n")
			      , false) == false)
	return false;
    
    _config->Set("Synaptic::Download-Only",
		 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_dlonlyB)) ? "true" : "false");
    
    return _confirmed;
}

