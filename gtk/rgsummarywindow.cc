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

#include <X11/keysym.h>

#include <apt-pkg/configuration.h>
#include <apt-pkg/strutl.h>

#include "rpackagelister.h"

#include <stdio.h>
#include <string>
#include <cassert>

#include "rgsummarywindow.h"
#include "rguserdialog.h"

#include "i18n.h"


void RGSummaryWindow::buildTree(RGSummaryWindow *me)
{
   RPackageLister *lister = me->_lister;
   GtkTreeIter iter, iter_child;

   vector<RPackage *> held;
   vector<RPackage *> kept;
   vector<RPackage *> essential;
   vector<RPackage *> toInstall;
   vector<RPackage *> toReInstall;
   vector<RPackage *> toUpgrade;
   vector<RPackage *> toRemove;
   vector<RPackage *> toPurge;
   vector<RPackage *> toDowngrade;
#ifdef WITH_APT_AUTH
   vector<string> notAuthenticated;
#endif
   double sizeChange;

   lister->getDetailedSummary(held, kept, essential, toInstall, toReInstall,
			      toUpgrade,  toRemove, toPurge, toDowngrade, 
#ifdef WITH_APT_AUTH
			      notAuthenticated,
#endif
			      sizeChange);

#ifdef WITH_APT_AUTH
   if(notAuthenticated.size() > 0) {
      GtkWidget *label;
      label = GTK_WIDGET(gtk_builder_get_object(me->_builder, "label_auth_warning"));
      assert(label);
      // FIXME: make this a message from a trust class (and remeber to
      // change the text in rgchangeswindow then too)
      gchar *msg = g_strdup_printf("<span weight=\"bold\" size=\"larger\">%s"
				   "</span>\n\n%s",
				   _("Warning"), 
				   _("You are about to install software that "
				     "<b>can't be authenticated</b>! Doing "
				     "this could allow a malicious individual "
				     "to damage or take control of your "
				     "system."));
      gtk_label_set_markup(GTK_LABEL(label), msg);
      gtk_widget_show(label);
      g_free(msg);

      /* not authenticated */
      gtk_tree_store_append(me->_treeStore, &iter, NULL);
      gtk_tree_store_set(me->_treeStore, &iter,
                         PKG_COLUMN, _("NOT AUTHENTICATED"), -1);

      for (vector<string>::const_iterator p = notAuthenticated.begin();
           p != notAuthenticated.end(); p++) {
         gtk_tree_store_append(me->_treeStore, &iter_child, &iter);
         gtk_tree_store_set(me->_treeStore, &iter_child,
                            PKG_COLUMN, (*p).c_str(), -1);
      }
   }
#endif


   if (essential.size() > 0) {
      /* (Essentail) removed */
      gtk_tree_store_append(me->_treeStore, &iter, NULL);
      gtk_tree_store_set(me->_treeStore, &iter,
                         PKG_COLUMN, _("<b>(ESSENTIAL) to be removed</b>"), 
			 -1);

      for (vector<RPackage *>::const_iterator p = essential.begin();
           p != essential.end(); p++) {
         gtk_tree_store_append(me->_treeStore, &iter_child, &iter);
         gtk_tree_store_set(me->_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }
   if (toDowngrade.size() > 0) {
      /* removed */
      gtk_tree_store_append(me->_treeStore, &iter, NULL);
      gtk_tree_store_set(me->_treeStore, &iter,
                         PKG_COLUMN, _("<b>To be DOWNGRADED</b>"), -1);
      for (vector<RPackage *>::const_iterator p = toDowngrade.begin();
           p != toDowngrade.end(); p++) {
         gtk_tree_store_append(me->_treeStore, &iter_child, &iter);
         gtk_tree_store_set(me->_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

   if (toRemove.size() > 0) {
      /* removed */
      gtk_tree_store_append(me->_treeStore, &iter, NULL);
      gtk_tree_store_set(me->_treeStore, &iter,
                         PKG_COLUMN, _("<b>To be removed</b>"), -1);
      for (vector<RPackage *>::const_iterator p = toRemove.begin();
           p != toRemove.end(); p++) {
         gtk_tree_store_append(me->_treeStore, &iter_child, &iter);
         gtk_tree_store_set(me->_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

   if (toPurge.size() > 0) {
      /* removed */
      gtk_tree_store_append(me->_treeStore, &iter, NULL);
      gtk_tree_store_set(me->_treeStore, &iter,
                         PKG_COLUMN, _("<b>To be completely removed (including configuration files)</b>"), 
			 -1);
      for (vector<RPackage *>::const_iterator p = toPurge.begin();
           p != toPurge.end(); p++) {
         gtk_tree_store_append(me->_treeStore, &iter_child, &iter);
         gtk_tree_store_set(me->_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

   if (toUpgrade.size() > 0) {
      gtk_tree_store_append(me->_treeStore, &iter, NULL);
      gtk_tree_store_set(me->_treeStore, &iter,
                         PKG_COLUMN, _("To be upgraded"), -1);
      for (vector<RPackage *>::const_iterator p = toUpgrade.begin();
           p != toUpgrade.end(); p++) {
         gtk_tree_store_append(me->_treeStore, &iter_child, &iter);
         gtk_tree_store_set(me->_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

   if (toInstall.size() > 0) {
      gtk_tree_store_append(me->_treeStore, &iter, NULL);
      gtk_tree_store_set(me->_treeStore, &iter,
                         PKG_COLUMN, _("To be installed"), -1);
      for (vector<RPackage *>::const_iterator p = toInstall.begin();
           p != toInstall.end(); p++) {
         gtk_tree_store_append(me->_treeStore, &iter_child, &iter);
         gtk_tree_store_set(me->_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

  if(toReInstall.size() > 0) {
    gtk_tree_store_append (me->_treeStore, &iter, NULL);  
    gtk_tree_store_set (me->_treeStore, &iter,
			PKG_COLUMN, _("To be re-installed"),-1);
    for (vector<RPackage*>::const_iterator p = toReInstall.begin(); 
	 p != toReInstall.end(); 
	 p++) 
      {
	gtk_tree_store_append (me->_treeStore, &iter_child, &iter);
	gtk_tree_store_set(me->_treeStore, &iter_child,
			   PKG_COLUMN,(*p)->name(), -1);
      }
  }


   if (held.size() > 0) {
      gtk_tree_store_append(me->_treeStore, &iter, NULL);
      gtk_tree_store_set(me->_treeStore, &iter,
                         PKG_COLUMN, _("Unchanged"), -1);
      for (vector<RPackage *>::const_iterator p = held.begin();
           p != held.end(); p++) {
         gtk_tree_store_append(me->_treeStore, &iter_child, &iter);
         gtk_tree_store_set(me->_treeStore, &iter_child,
                            PKG_COLUMN, (*p)->name(), -1);
      }
   }

   

}


void RGSummaryWindow::buildLabel(RGSummaryWindow *me)
{
   GtkWidget *info;
   string text;
   gchar *str;

   info = GTK_WIDGET(gtk_builder_get_object(me->_builder, "label_details"));

   vector<RPackage *> held;
   vector<RPackage *> kept;
   vector<RPackage *> essential;
   vector<RPackage *> toInstall;
   vector<RPackage *> toReInstall;
   vector<RPackage *> toUpgrade;
   vector<RPackage *> toRemove;
   vector<RPackage *> toPurge;
   vector<RPackage *> toDowngrade;
   vector<string> notAuthenticated;
   double sizeChange;

   me->_lister->getDetailedSummary(held, kept, essential,
				   toInstall,toReInstall,toUpgrade, 
				   toRemove, toPurge, toDowngrade, 
#ifdef WITH_APT_AUTH
				   notAuthenticated,
#endif
				   sizeChange);

   for (vector<RPackage *>::const_iterator p = essential.begin();
        p != essential.end(); p++) {
      str =
         g_strdup_printf(_("<b>%s</b> (<b>essential</b>) will be removed\n"),
                         (*p)->name());
      text += str;
      g_free(str);
   }

   for (vector<RPackage *>::const_iterator p = toDowngrade.begin();
        p != toDowngrade.end(); p++) {
      str =
         g_strdup_printf(_("<b>%s</b> will be <b>downgraded</b>\n"),
                         (*p)->name());
      text += str;
      g_free(str);
   }

   for (vector<RPackage *>::const_iterator p = toPurge.begin();
        p != toPurge.end(); p++) {
      str = g_strdup_printf(_("<b>%s</b> will be removed with configuration\n"), 
			    (*p)->name());
      text += str;
      g_free(str);
   }

   for (vector<RPackage *>::const_iterator p = toRemove.begin();
        p != toRemove.end(); p++) {
      str = g_strdup_printf(_("<b>%s</b> will be removed\n"), (*p)->name());
      text += str;
      g_free(str);
   }

   for (vector<RPackage *>::const_iterator p = toUpgrade.begin();
        p != toUpgrade.end(); p++) {
      str =
         g_strdup_printf(_
                         ("<b>%s</b> (version <i>%s</i>) will be upgraded to version <i>%s</i>\n"),
                         (*p)->name(), (*p)->installedVersion(),
                         (*p)->availableVersion());
      text += str;
      g_free(str);
   }

   for (vector<RPackage *>::const_iterator p = toInstall.begin();
        p != toInstall.end(); p++) {
      str =
         g_strdup_printf(_
                         ("<b>%s</b> (version <i>%s</i>) will be installed\n"),
                         (*p)->name(), (*p)->availableVersion());
      text += str;
      g_free(str);
   }

   for (vector<RPackage*>::const_iterator p = toReInstall.begin(); 
	p != toReInstall.end(); p++) {
       str = g_strdup_printf(_("<b>%s</b> (version <i>%s</i>) will be re-installed\n"),(*p)->name(), (*p)->installedVersion());
       text += str;
       g_free(str);
   }

   gtk_label_set_markup(GTK_LABEL(info), text.c_str());
}

void RGSummaryWindow::clickedDetails(GtkWidget *self, void *data)
{
   RGSummaryWindow *me = (RGSummaryWindow *) data;
   GtkWidget *s,*d;
   GtkWidget *info;

   s = GTK_WIDGET(gtk_builder_get_object(me->_builder,
                                         "scrolledwindow_summary"));
   d = GTK_WIDGET(gtk_builder_get_object(me->_builder,
                                         "scrolledwindow_details"));

   if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self))) {
      gtk_widget_hide(s);
      gtk_widget_show(d);
      gtk_button_set_label(GTK_BUTTON(self),_("_Hide Details"));
   } else {
      gtk_widget_show(s);
      gtk_widget_hide(d);
      gtk_button_set_label(GTK_BUTTON(self),_("_Show Details"));
   }
}


RGSummaryWindow::RGSummaryWindow(RGWindow *wwin, RPackageLister *lister)
: RGGtkBuilderWindow(wwin, "summary")
{
   GtkWidget *button;

   _potentialBreak = false;
   _lister = lister;

   setTitle(_("Summary"));
   //gtk_window_set_default_size(GTK_WINDOW(_win), 400, 250);

   _summaryL = GTK_WIDGET(gtk_builder_get_object(_builder, "label_summary"));
   assert(_summaryL);

   _summarySpaceL = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                      "label_summary_space"));
   assert(_summarySpaceL);

   // details label
   buildLabel(this);

   // new tree store
   _treeStore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING);
   buildTree(this);
   _tree = GTK_WIDGET(gtk_builder_get_object(_builder, "treeview_summary"));
   assert(_tree);

   GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
   //GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Summary",renderer, PKG_COLUMN, "pkg", NULL);
   GtkTreeViewColumn *column;
   column = gtk_tree_view_column_new_with_attributes(_("Marked Changes"),
                                                     renderer,
                                                     "markup", PKG_COLUMN, NULL);
   /* Add the column to the view. */
   gtk_tree_view_append_column(GTK_TREE_VIEW(_tree), column);
   gtk_widget_show(_tree);
   /* set model last */
   gtk_tree_view_set_model(GTK_TREE_VIEW(_tree), GTK_TREE_MODEL(_treeStore));

   _dlonlyB = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                "checkbutton_download_only"));

   _checkSigsB = GTK_WIDGET(gtk_builder_get_object
                            (_builder, "checkbutton_check_signatures"));
   assert(_checkSigsB);
#ifdef HAVE_RPM
   bool check = _config->FindB("RPM::GPG-Check", true);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_checkSigsB), check);
   gtk_widget_show(_checkSigsB);
#endif

   button = GTK_WIDGET(gtk_builder_get_object(_builder,"togglebutton_details"));
   g_signal_connect(G_OBJECT(button), "clicked",
                    G_CALLBACK(clickedDetails), this);

   int toInstall, toReInstall, toRemove, toUpgrade, toDowngrade;
   int held, kept, essential, unAuthenticated;
   double sizeChange, dlSize;
   int dlCount;
   GString *msg = g_string_new("");
   GString *msg_space = g_string_new("");

   lister->getSummary(held, kept, essential,
                      toInstall, toReInstall, toUpgrade, toRemove, 
		      toDowngrade, unAuthenticated,sizeChange);
   lister->getDownloadSummary(dlCount, dlSize);

#if 0
   if (held) {
      char *str = ngettext("%d package is locked\n",
                           "%d packages are locked\n", held);
      g_string_append_printf(msg, str, held);
   }
#endif

   if (held) {
      char *str = ngettext("%d package will be held back and not upgraded\n",
                           "%d packages will be held back and not upgraded\n",
                           held);
      g_string_append_printf(msg, str, held);
   }

   if (toInstall) {
      char *str = ngettext("%d new package will be installed\n",
                           "%d new packages will be installed\n",
                           toInstall);
      g_string_append_printf(msg, str, toInstall);
   }

   if (toReInstall) {
       char *str = ngettext("%d new package will be re-installed\n",
			    "%d new packages will be re-installed\n",
			    toReInstall);
       g_string_append_printf(msg, str, toReInstall);
   }

   if (toUpgrade) {
      char *str = ngettext("%d package will be upgraded\n",
                           "%d packages will be upgraded\n",
                           toUpgrade);
      g_string_append_printf(msg, str, toUpgrade);
   }

   if (toRemove) {
      char *str = ngettext("%d package will be removed\n",
                           "%d packages will be removed\n",
                           toRemove);
      g_string_append_printf(msg, str, toRemove);
   }

   if (toDowngrade) {
      char *str = ngettext("%d package will be <b>downgraded</b>\n",
                           "%d packages will be <b>downgraded</b>\n",
                           toDowngrade);
      g_string_append_printf(msg, str, toDowngrade);
   }

   if (essential) {
      char *str =
         ngettext("<b>Warning:</b> %d essential package will be removed\n",
                  "<b>Warning:</b> %d essential packages will be removed\n",
                  essential);
      g_string_append_printf(msg, str, essential);
      _potentialBreak = true;
   }
   // remove the trailing newline of msg
   if (msg->str[msg->len - 1] == '\n')
      msg = g_string_truncate(msg, msg->len - 1);

   /* this stuff goes to the msg_space string */
   if (sizeChange > 0) {
      g_string_append_printf(msg_space, _("%s of extra space will be used"),
                             SizeToStr(sizeChange).c_str());
   } else if (sizeChange < 0) {
      g_string_append_printf(msg_space, _("%s of extra space will be freed"),
                             SizeToStr(sizeChange).c_str());
      sizeChange = -sizeChange;
   }

   g_string_append_printf(msg_space, _("\n%s have to be downloaded"),
                             SizeToStr(dlSize).c_str());
   
   gtk_label_set_markup(GTK_LABEL(_summaryL), msg->str);
   gtk_label_set_markup(GTK_LABEL(_summarySpaceL), msg_space->str);
   g_string_free(msg, TRUE);
   g_string_free(msg_space, TRUE);
   if(!_config->FindB("Volatile::HideMainwindow", false))
      skipTaskbar(true);
   else
      skipTaskbar(false);
}



bool RGSummaryWindow::showAndConfirm()
{
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_dlonlyB),
                                _config->FindB("Volatile::Download-Only",
                                               false));
   int res = gtk_dialog_run(GTK_DIALOG(_win));
   bool confirmed = (res == GTK_RESPONSE_APPLY);

   RGUserDialog userDialog(this);
   if (confirmed && _potentialBreak
       && userDialog.warning(_("Essential packages will be removed.\n"
                               "This may render your system unusable!\n")
                             , false) == false)
      return false;

   _config->Set("Volatile::Download-Only",
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_dlonlyB)) ?
                "true" : "false");
#ifdef HAVE_RPM
   _config->Set("RPM::GPG-Check",
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_checkSigsB)) ?
                "true" : "false");
#endif

   return confirmed;
}
