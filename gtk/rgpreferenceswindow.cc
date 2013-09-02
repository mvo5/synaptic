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

#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/strutl.h>
#include <gtk/gtk.h>
#include <cassert>
#include <cstring>
#include <cstdlib>

#include "rconfiguration.h"
#include "rgpreferenceswindow.h"
#include "rguserdialog.h"
#include "gsynaptic.h"
#include "rgpackagestatus.h"
#include "gtk3compat.h"

#include "i18n.h"

enum { FONT_DEFAULT, FONT_TERMINAL };

const char * RGPreferencesWindow::column_names[] = 
   {"status", "supported", "name", "section", "component", "instVer", 
    "availVer", "instSize", "downloadSize", "descr", NULL };

const char *RGPreferencesWindow::column_visible_names[] = 
   {_("Status"), _("Supported"), _("Package Name"), _("Section"),
    _("Component"), _("Installed Version"), _("Latest Version"), 
    _("Size"), _("Download Size"),_("Description"), NULL };

const gboolean RGPreferencesWindow::column_visible_defaults[] = 
   { TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, TRUE }; 

const char *RGPreferencesWindow::removal_actions[] =
   { N_("Keep Configuration"), N_("Completely"), NULL };

const char *RGPreferencesWindow::update_ask[] =
   { N_("Always Ask"), N_("Ignore"), N_("Automatically"), NULL };

const char *RGPreferencesWindow::upgrade_method[] =
   { N_("Always Ask"), N_("Default Upgrade"), N_("Smart Upgrade"), NULL };


void RGPreferencesWindow::cbHttpProxyEntryChanged(GtkWidget *self, void *data)
{
   // this function strips http:// from a entred proxy url
   RGPreferencesWindow *me = (RGPreferencesWindow *) data;
   const gchar *text = gtk_entry_get_text(GTK_ENTRY(self));
   gchar *new_text = NULL;
   if(g_str_has_prefix(text, "http://")) {
      new_text = g_strdup_printf("%s", &text[strlen("http://")]);
      gtk_entry_set_text(GTK_ENTRY(self), new_text);
   }
   if(new_text != NULL)
      g_free(new_text);
}

void RGPreferencesWindow::cbArchiveSelection(GtkWidget *self, void *data)
{
   RGPreferencesWindow *me = (RGPreferencesWindow *) data;
   //cout << "void RGPreferencesWindow::onArchiveSelection()" << endl;
   //cout << "data is: " << (char*)data << endl;

   if(me->_blockAction)
      return;

   gchar *s=gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(self));
   if(s!=NULL) {
      me->_defaultDistro = s;
      //cout << "new default distro: " << me->_defaultDistro << endl;
      me->distroChanged = true;
   }
}

void RGPreferencesWindow::cbRadioDistributionChanged(GtkWidget *self, 
						     void *data)
{
   //cout << "void RGPreferencesWindow::cbRadioDistributionChanged()" << endl;
   RGPreferencesWindow *me = (RGPreferencesWindow *) data;
   
   // we are only interested in the active one
   if(me->_blockAction || !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self))) 
      return;
   
   gchar *defaultDistro = (gchar*)g_object_get_data(G_OBJECT(self),"defaultDistro");
   if(strcmp(defaultDistro, "distro") == 0) {
      gtk_widget_set_sensitive(GTK_WIDGET(me->_comboDefaultDistro),TRUE);
      gchar *s = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(me->_comboDefaultDistro));
      if(s!=NULL)
	 me->_defaultDistro = s;
   } else {
      gtk_widget_set_sensitive(GTK_WIDGET(me->_comboDefaultDistro),FALSE);
      me->_defaultDistro = defaultDistro;
   }
   //cout << "new default distro: " << me->_defaultDistro << endl;
   me->distroChanged = true;
}

void RGPreferencesWindow::applyProxySettings()
{
   string http, ftp, noProxy;
   string httpUser, httpPass;
   gchar *s;
   int httpPort, ftpPort;

   // save apts settings here
   static bool firstRun = true;
   static string aptHttpProxy, aptFtpProxy;

   if(firstRun) {
      aptHttpProxy = _config->Find("Acquire::http::Proxy");
      aptFtpProxy = _config->Find("Acquire::ftp::Proxy");
      
      firstRun = false;
   }

   bool useProxy = _config->FindB("Synaptic::useProxy", false);
   // now set the stuff for apt
   if (useProxy) {
      http = _config->Find("Synaptic::httpProxy", "");
      httpPort = _config->FindI("Synaptic::httpProxyPort", 3128);
      ftp = _config->Find("Synaptic::ftpProxy", "");
      ftpPort = _config->FindI("Synaptic::ftpProxyPort", 3128);
      noProxy = _config->Find("Synaptic::noProxy", "");
      httpUser = _config->Find("Synaptic::httpProxyUser", "");
      httpPass = _config->Find("Synaptic::httpProxyPass", "");

      if(!http.empty()) {
	 unsetenv("http_proxy");
	 s = g_strdup_printf("http://%s:%i/", http.c_str(), httpPort);
	 _config->Set("Acquire::http::Proxy", s);
	 g_free(s);
      }
      // setup the proxy 
      if(!httpUser.empty() && !httpPass.empty()) { 
          s = g_strdup_printf("http://%s:%s@%s:%i/",
                  QuoteString(httpUser, "@%:/").c_str(),
                  QuoteString(httpPass, "@%:/").c_str(),
                  http.c_str(), httpPort);
	 _config->Set("Acquire::http::Proxy", s);
	 g_free(s);
      }
      if(!ftp.empty()) {
	 unsetenv("ftp_proxy");
	 s = g_strdup_printf("http://%s:%i", ftp.c_str(), ftpPort);
	 _config->Set("Acquire::ftp::Proxy", s);
	 g_free(s);
      }
      // set the no-proxies
      unsetenv("no_proxy");
      gchar **noProxyArray = g_strsplit(noProxy.c_str(), ",", 0);
      for (int j = 0; noProxyArray[j] != NULL; j++) {
         g_strstrip(noProxyArray[j]);
         s = g_strdup_printf("Acquire::http::Proxy::%s", noProxyArray[j]);
         _config->Set(s, "DIRECT");
         g_free(s);
         s = g_strdup_printf("Acquire::ftp::Proxy::%s", noProxyArray[j]);
         _config->Set(s, "DIRECT");
         g_free(s);
      }
      g_strfreev(noProxyArray);
   } else {
      //FIXME: we can't just clean here as apt may have it's own proxy 
      // settings!
      _config->Set("Acquire::http::Proxy",aptHttpProxy);
      _config->Set("Acquire::ftp::Proxy", aptFtpProxy);
   }
}

void RGPreferencesWindow::saveGeneral()
{
   bool newval;
   int i;

   // show package properties in main window
   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_optionShowAllPkgInfoInMain));
   _config->Set("Synaptic::ShowAllPkgInfoInMain", newval ? "true" : "false");
   // apply the changes
   GtkWidget *notebook = GTK_WIDGET(gtk_builder_get_object
                                    (_mainWin->getGtkBuilder(),
                                     "notebook_pkginfo"));
   gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), newval);
   GtkWidget *box = GTK_WIDGET(gtk_builder_get_object
                               (_mainWin->getGtkBuilder(),
                                "vbox_pkgdescr"));
   if(newval) {
      gtk_container_set_border_width(GTK_CONTAINER(box), 6);
   } else {
      gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
      gtk_container_set_border_width(GTK_CONTAINER(box), 0);
   }

   // Ask to confirm changes also affecting other packages
   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_optionAskRelated));
   _config->Set("Synaptic::AskRelated", newval ? "true" : "false");

   // Consider recommended packages as dependencies
   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_optionCheckRecom));
   _config->Set("APT::Install-Recommends", newval ? "true" : "false");

   // Clicking on the status icon marks the most likely action
   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_optionOneClick));
   _config->Set("Synaptic::OneClickOnStatusActions", newval ? "true" : "false");

   // Removal of packages: 
   int delAction = gtk_combo_box_get_active(GTK_COMBO_BOX(_comboRemovalAction));
   // ugly :( but we need this +2 because RGPkgAction starts with 
   //         "keep","install"
   delAction += 2;
   _config->Set("Synaptic::delAction", delAction);

   // System upgrade:
   // upgrade type, (ask=-1,normal=0,dist-upgrade=1)
   i = gtk_combo_box_get_active(GTK_COMBO_BOX(_comboUpgradeMethod));
   _config->Set("Synaptic::upgradeType", i - 1);

   // package list update date check
   i = gtk_combo_box_get_active(GTK_COMBO_BOX(_comboUpdateAsk));
   _config->Set("Synaptic::update::type", i);
   

   // Number of undo operations:
   int maxUndo = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(_maxUndoE));
   _config->Set("Synaptic::undoStackSize", maxUndo);

   // Apply changes in a terminal window
   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_optionUseTerminal));
   _config->Set("Synaptic::UseTerminal", newval ? "true" : "false");

   // Ask to quit after the changes have been applied successfully
   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_optionAskQuit));
   _config->Set("Synaptic::AskQuitOnProceed", newval ? "true" : "false");

}

void RGPreferencesWindow::saveColumnsAndFonts() 
{
   bool newval;

   // Use custom application font
   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(_builder, "checkbutton_user_font")));
   _config->Set("Synaptic::useUserFont", newval);

   GValue value = { 0, };
   g_value_init(&value, G_TYPE_STRING);
   if (newval) {
      g_value_set_string(&value, _config->Find("Synaptic::FontName").c_str());
      g_object_set_property(G_OBJECT(gtk_settings_get_default()),
                            "gtk-font-name", &value);
   } else {
      g_value_set_string(&value,
                         _config->Find("Volatile::orginalFontName",
                                       "Sans 10").c_str());
      g_object_set_property(G_OBJECT(gtk_settings_get_default()),
                            "gtk-font-name", &value);
   }
   g_value_unset(&value);

   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(_builder, "checkbutton_user_terminal_font")));
   _config->Set("Synaptic::useUserTerminalFont", newval);
   

   // treeviewstuff 
   // get from GtkListStore
   GtkListStore *store = _listColumns;
   GtkTreeIter iter;
   int i=0;
   char *column_name, *config_name;
   gboolean visible;
   gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
   do {
      gtk_tree_model_get (GTK_TREE_MODEL(store), &iter,
			  TREE_CHECKBOX_COLUMN, &visible,
			  TREE_NAME_COLUMN, &column_name,
			  -1);

      // pos
      config_name = g_strdup_printf("Synaptic::%sColumnPos",column_name);
      _config->Set(config_name, i);
      //cout << column_name << " : " << i << endl;
      g_free(config_name);
      
      // visible
      config_name = g_strdup_printf("Synaptic::%sColumnVisible",column_name);
      _config->Set(config_name, visible);
      g_free(config_name);

      i++;
   } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));

   // rebuild the treeview
   _mainWin->rebuildTreeView();
}

void RGPreferencesWindow::saveColors()
{
   bool newval;

   // save the colors
   RGPackageStatus::pkgStatus.saveColors();
   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_optionUseStatusColors));
   _config->Set("Synaptic::UseStatusColors", newval ? "true" : "false");
}

void RGPreferencesWindow::saveFiles()
{
   bool newval;

   // cache
   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_cacheClean));
   _config->Set("Synaptic::CleanCache", newval ? "true" : "false");
   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_cacheAutoClean));
   _config->Set("Synaptic::AutoCleanCache", newval ? "true" : "false");

   // history
   newval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_delHistory));
   if(!newval) {
      _config->Set("Synaptic::delHistory", -1);
      return;
   }
   int delHistory= gtk_spin_button_get_value(GTK_SPIN_BUTTON(_spinDelHistory));
   _config->Set("Synaptic::delHistory", delHistory);
   _lister->cleanCommitLog();
}

void RGPreferencesWindow::saveNetwork()
{
   // proxy stuff
   bool useProxy;
   const gchar *http, *ftp, *noProxy;
   int httpPort, ftpPort;

   useProxy = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_useProxy));
   _config->Set("Synaptic::useProxy", useProxy);
   // http
   http = gtk_entry_get_text(
      GTK_ENTRY(gtk_builder_get_object(_builder, "entry_http_proxy")));
   _config->Set("Synaptic::httpProxy", http);
   httpPort = (int) gtk_spin_button_get_value(
      GTK_SPIN_BUTTON(gtk_builder_get_object(_builder,"spinbutton_http_port")));
   _config->Set("Synaptic::httpProxyPort", httpPort);
   // ftp
   ftp = gtk_entry_get_text(
      GTK_ENTRY(gtk_builder_get_object(_builder, "entry_ftp_proxy")));
   _config->Set("Synaptic::ftpProxy", ftp);
   ftpPort = (int) gtk_spin_button_get_value(
      GTK_SPIN_BUTTON(gtk_builder_get_object(_builder,"spinbutton_ftp_port")));
   _config->Set("Synaptic::ftpProxyPort", ftpPort);
   noProxy = gtk_entry_get_text(
      GTK_ENTRY(gtk_builder_get_object(_builder, "entry_no_proxy")));
   _config->Set("Synaptic::noProxy", noProxy);

   applyProxySettings();
}
 
void RGPreferencesWindow::saveDistribution()
{
   if (_defaultDistro.empty()) {
      _config->Clear("APT::Default-Release");
      _config->Clear("Synaptic::DefaultDistro");
   } else {
      _config->Set("APT::Default-Release", _defaultDistro);
      _config->Set("Synaptic::DefaultDistro", _defaultDistro);
   }
}


void RGPreferencesWindow::saveAction(GtkWidget *self, void *data)
{
   RGPreferencesWindow *me = (RGPreferencesWindow *) data;

   me->saveGeneral();
   me->saveColumnsAndFonts();
   me->saveColors();
   me->saveFiles();
   me->saveNetwork();
   me->saveDistribution();

   if (!RWriteConfigFile(*_config)) {
      _error->Error(_("An error occurred while saving configurations."));
      RGUserDialog userDialog(me);
      userDialog.showErrors();
   }
}


void RGPreferencesWindow::closeAction(GtkWidget *self, void *data)
{
   RGPreferencesWindow *me = (RGPreferencesWindow *) data;
   me->close();
}

void RGPreferencesWindow::doneAction(GtkWidget *self, void *data)
{
   RGPreferencesWindow *me = (RGPreferencesWindow *) data;
   me->saveAction(self, data);
   if (me->distroChanged) {
      me->hide();
      me->_lister->unregisterObserver(me->_mainWin);
      me->_mainWin->setTreeLocked(TRUE);
      if (!me->_lister->openCache()) {
	 me->_mainWin->showErrors();
	 exit(1);
      }
      me->_mainWin->setTreeLocked(FALSE);
      me->_lister->registerObserver(me->_mainWin);
      me->_mainWin->refreshTable();
   }
   me->closeAction(self, data);
}

void RGPreferencesWindow::changeFontAction(GtkWidget *self, void *data)
{
   const char *fontName, *propName;
   
   switch (GPOINTER_TO_INT(data)) {
      case FONT_DEFAULT:
         propName = "Synaptic::FontName";
	      fontName = "sans 10";
         break;
      case FONT_TERMINAL:
         propName = "Synaptic::TerminalFontName";
	      fontName = "monospace 10";
         break;
      default:
         cerr << "changeFontAction called with unknown argument" << endl;
         return;
   }

   GtkWidget *fontsel = gtk_font_selection_dialog_new(_("Choose font"));

   gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(fontsel),
                                           _config->Find(propName,
                                                         fontName).c_str());

   gint result = gtk_dialog_run(GTK_DIALOG(fontsel));
   if (result != GTK_RESPONSE_OK) {
      gtk_widget_destroy(fontsel);
      return;
   }

   fontName =
      gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG
                                              (fontsel));

   //cout << "fontname: " << fontName << endl;

   _config->Set(propName, fontName);

   gtk_widget_destroy(fontsel);
}

void RGPreferencesWindow::clearCacheAction(GtkWidget *self, void *data)
{
   RGPreferencesWindow *me = (RGPreferencesWindow *) data;

   me->_lister->cleanPackageCache(true);
}

void RGPreferencesWindow::readGeneral()
{
   // Allow regular expressions in searches and filters
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionShowAllPkgInfoInMain),
                                _config->FindB("Synaptic::ShowAllPkgInfoInMain", false));

   // Ask to confirm changes also affecting other packages
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionAskRelated),
                                _config->FindB("Synaptic::AskRelated", true));

   // Consider recommended packages as dependencies
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionCheckRecom),
                                _config->FindB("APT::Install-Recommends",
                                               false));

   // Clicking on the status icon marks the most likely action
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionOneClick),
                                _config->FindB("Synaptic::OneClickOnStatusActions",
                                               false));

   // Removal of packages: 
   int delAction = _config->FindI("Synaptic::delAction", PKG_DELETE);
   // now set the combobox
   // ugly :( but we need this -2 because RGPkgAction starts with 
   //         "keep","install"
   gtk_combo_box_set_active(GTK_COMBO_BOX(_comboRemovalAction), delAction - 2);


   // System upgrade:
   // upgradeType (ask=-1,normal=0,dist-upgrade=1)
   int i = _config->FindI("Synaptic::upgradeType", 1);
   gtk_combo_box_set_active(GTK_COMBO_BOX(_comboUpgradeMethod), i + 1);

   i = _config->FindI("Synaptic::update::type", 0);
   gtk_combo_box_set_active(GTK_COMBO_BOX(_comboUpdateAsk), i);


   // Number of undo operations:

#ifdef HAVE_RPM
   int UndoStackSize = 3;
#else
   int UndoStackSize = 20;
#endif
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(_maxUndoE),
                             _config->FindI("Synaptic::undoStackSize",
                                            UndoStackSize));

   
   // Apply changes in a terminal window
   bool UseTerminal = false;
#ifndef HAVE_TERMINAL
   gtk_widget_set_sensitive(GTK_WIDGET(_optionUseTerminal), false);
   _config->Set("Synaptic::UseTerminal", false);
#else
#ifndef HAVE_RPM 
#ifndef WITH_DPKG_STATUSFD
   UseTerminal = true;
#endif
#endif
#endif
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionUseTerminal),
                                _config->FindB("Synaptic::UseTerminal",
                                               UseTerminal));

   // Ask to quit after the changes have been applied successfully
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionAskQuit),
                                _config->FindB("Synaptic::AskQuitOnProceed",
                                               false));
}

void RGPreferencesWindow::readColumnsAndFonts()
{
   // font stuff
   bool b = _config->FindB("Synaptic::useUserFont", false);
   gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(gtk_builder_get_object(_builder,
                                             "checkbutton_user_font")), b);
   b = _config->FindB("Synaptic::useUserTerminalFont", false);
   gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(gtk_builder_get_object(_builder,
                                             "checkbutton_user_terminal_font")), b);

   readTreeViewValues();

}

void RGPreferencesWindow::readColors()
{
   GdkColor *color;
   gchar *color_button = NULL;
   GtkWidget *button = NULL;

   // Color packages by their status
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_optionUseStatusColors),
                                _config->FindB("Synaptic::UseStatusColors",
                                               true));

   // color buttons
   GString *custom_css = g_string_new("");
   for (int i = 0; i < RGPackageStatus::N_STATUS_COUNT; i++) {
      color_button =
         g_strdup_printf("button_%s_color",
                         RGPackageStatus::pkgStatus.
                         getShortStatusString(RGPackageStatus::PkgStatus(i)));
      button = GTK_WIDGET(gtk_builder_get_object(_builder, color_button));
      assert(button);
      gtk_widget_set_name(button, color_button);
      if (RGPackageStatus::pkgStatus.getColor(i) != NULL) {
         color = RGPackageStatus::pkgStatus.getColor(i);
         // I whish I could just use gtk_widget_change_background_color
         // but see gtk bug https://bugzilla.gnome.org/show_bug.cgi?id=656461
         g_string_append_printf(custom_css,
            "GtkButton#%s { "
            " background:none; "
            " background-color:#%02x%02x%02x; "
            "} ", 
            color_button, 
            color->red / 256, 
            color->green / 256, 
            color->blue / 256);
      }
      gtk_css_provider_load_from_data(_css_provider, custom_css->str, -1, NULL);
      g_free(color_button);
   }
   g_string_free(custom_css, TRUE);
}

void RGPreferencesWindow::readFiles()
{
   // <b>Temporary Files</b>
   bool postClean = _config->FindB("Synaptic::CleanCache", false);
   bool postAutoClean = _config->FindB("Synaptic::AutoCleanCache", true);

   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_cacheClean), postClean);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_cacheAutoClean), 
				postAutoClean);

   if (postClean)
      gtk_button_clicked(GTK_BUTTON(_cacheClean));
   else if (postAutoClean)
      gtk_button_clicked(GTK_BUTTON(_cacheAutoClean));
   else
      gtk_button_clicked(GTK_BUTTON(_cacheLeave));

   // history
   int delHistory = _config->FindI("Synaptic::delHistory", -1);
   if(delHistory < 0) 
      gtk_button_clicked(GTK_BUTTON(_keepHistory));
   else {
      gtk_button_clicked(GTK_BUTTON(_delHistory));      
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(_spinDelHistory), 
				delHistory);      
   }
}

void RGPreferencesWindow::readNetwork()
{
   // proxy stuff
   bool useProxy = _config->FindB("Synaptic::useProxy", false);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object
                                                  (_builder,"radio_no_proxy")),
                                                  !useProxy);
   gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object
                            (_builder, "table_proxy")),
                            useProxy);
   string str = _config->Find("Synaptic::httpProxy", "");
   gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object
                                (_builder, "entry_http_proxy")), str.c_str());
   int i = _config->FindI("Synaptic::httpProxyPort", 3128);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON
                             (gtk_builder_get_object
                              (_builder, "spinbutton_http_port")), i);

   str = _config->Find("Synaptic::ftpProxy", "");
   gtk_entry_set_text(
      GTK_ENTRY(gtk_builder_get_object(_builder, "entry_ftp_proxy")), str.c_str());
   i = _config->FindI("Synaptic::ftpProxyPort", 3128);
   gtk_spin_button_set_value(
      GTK_SPIN_BUTTON(gtk_builder_get_object(_builder, "spinbutton_ftp_port")), i);
   str = _config->Find("Synaptic::noProxy", "");
   gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(_builder,
                                                       "entry_no_proxy")),
                      str.c_str());

}

void RGPreferencesWindow::readDistribution()
{
   // distro selection, block actions here because the combobox changes
   // and a signal is emited
   _blockAction = true;

   int distroMatch = 0;
   string defaultDistro = _config->Find("Synaptic::DefaultDistro", "");
   vector<string> archives = _lister->getPolicyArchives();
  
   // setup the toggle buttons
   GtkWidget *button, *ignore,*now,*distro;
   ignore = GTK_WIDGET(gtk_builder_get_object(_builder, "radiobutton_ignore"));
   g_object_set_data(G_OBJECT(ignore),"defaultDistro",(void*)"");
   g_signal_connect(G_OBJECT(ignore),
                    "toggled",
                    G_CALLBACK(cbRadioDistributionChanged), this);
   now = GTK_WIDGET(gtk_builder_get_object(_builder, "radiobutton_now"));
   g_object_set_data(G_OBJECT(now),"defaultDistro",(void*)"now");
   g_signal_connect(G_OBJECT(now),
                    "toggled",
                    G_CALLBACK(cbRadioDistributionChanged), this);
   distro = GTK_WIDGET(gtk_builder_get_object(_builder, "radiobutton_distro"));
   g_object_set_data(G_OBJECT(distro),"defaultDistro",(void*)"distro");
   g_signal_connect(G_OBJECT(distro),
                    "toggled",
                    G_CALLBACK(cbRadioDistributionChanged), this);

   // clear the combo box
   GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(_comboDefaultDistro));
   int num = gtk_tree_model_iter_n_children(model, NULL);
   for(;num >= 0;num--)
      gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(_comboDefaultDistro), num);

   if(defaultDistro == "") {
      button = ignore;
      gtk_widget_set_sensitive(GTK_WIDGET(_comboDefaultDistro),FALSE);
   } else if(defaultDistro == "now") {
      button = now;
      gtk_widget_set_sensitive(GTK_WIDGET(_comboDefaultDistro),FALSE);
   } else {
      button = distro;
      gtk_widget_set_sensitive(GTK_WIDGET(_comboDefaultDistro),TRUE);
   }
   assert(button);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);


   // set up combo-box
   g_signal_connect(G_OBJECT(_comboDefaultDistro), "changed",
		    G_CALLBACK(cbArchiveSelection), this);

   g_signal_connect(gtk_builder_get_object(_builder, "entry_http_proxy"),
				 "changed",
				 G_CALLBACK(cbHttpProxyEntryChanged), this);

   GtkTreeIter distroIter;
   GtkListStore *distroStore
      = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(_comboDefaultDistro)));
   for (unsigned int i = 0; i < archives.size(); i++) {
      // ignore "now", it's a toggle button item now
      if(archives[i] == "now")
	 continue;
      gtk_list_store_append(distroStore, &distroIter);
      gtk_list_store_set(distroStore, &distroIter, 0, archives[i].c_str(), -1);
      if (defaultDistro == archives[i]) {
         //cout << "match for: " << archives[i] << " (" << i << ")" << endl;
	 // we ignored the "now" archive, so we have to subtract by one
         distroMatch=i-1;
      }
   }
   GtkCellRenderer *crt;
   crt = gtk_cell_renderer_text_new();
   gtk_cell_layout_clear(GTK_CELL_LAYOUT(_comboDefaultDistro));
   gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_comboDefaultDistro), crt, TRUE);
   gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_comboDefaultDistro),
                                 crt, "text", 0);
   gtk_combo_box_set_active(GTK_COMBO_BOX(_comboDefaultDistro), distroMatch);

   _blockAction = false;
}

void RGPreferencesWindow::show()
{
   readGeneral();
   readColumnsAndFonts();
   readColors();
   readFiles();
   readNetwork();
   readDistribution();

   RGWindow::show();
}


void RGPreferencesWindow::readTreeViewValues()
{

   // number of entries in columns is (sizeof(columns)/sizeof(char*))-1 
   int number_of_columns = sizeof(column_names)/sizeof(char*)-1;

   // the position in this vector is the position of the column
   vector<column_struct> columns(number_of_columns);

   // make sure that we fail gracefully
   bool corrupt=false;

   // put into right place
   gchar *name;
   int pos;
   column_struct c;
   for(int i=0;column_names[i] != NULL; i++) {
      // pos
      name = g_strdup_printf("Synaptic::%sColumnPos",column_names[i]);
      pos = _config->FindI(name, i);
      g_free(name);
      
      // visible
      name = g_strdup_printf("Synaptic::%sColumnVisible",column_names[i]);
      c.visible = _config->FindB(name, column_visible_defaults[i]);
      c.name = column_names[i];
      c.visible_name = column_visible_names[i];
      g_free(name);

      if(pos > number_of_columns || pos < 0 || columns[pos].name != NULL) {
	 //cerr << "invalid column config found, reseting"<<endl;
	 corrupt=true;
	 continue;
      }
      columns[pos] = c;
   }

   // if corrupt for some reason, repair
   if(corrupt) {
      cerr << "seting column order to default" << endl;
      for(int i=0;column_names[i] != NULL; i++) {
	 name = g_strdup_printf("Synaptic::%sColumnPos",column_names[i]);
	 _config->Set(name, i);
	 c.visible = column_visible_defaults[i];
	 c.name = column_names[i];
	 c.visible_name = column_visible_names[i];
	 columns[i] = c;
      }
   }


   // put into GtkListStore
   GtkListStore *store = _listColumns = gtk_list_store_new(3, 
							   G_TYPE_BOOLEAN, 
							   G_TYPE_STRING, 
							   G_TYPE_STRING);
   GtkTreeIter iter;
   for(unsigned int i=0;i<columns.size();i++) {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  TREE_CHECKBOX_COLUMN, columns[i].visible,
			  TREE_NAME_COLUMN, columns[i].name,
			  TREE_VISIBLE_NAME_COLUMN, _(columns[i].visible_name),
			  -1);
      
   }

   // now set the model
   gtk_tree_view_set_model(GTK_TREE_VIEW(_treeView), 
			   GTK_TREE_MODEL(_listColumns));

}

void RGPreferencesWindow::cbMoveColumnUp(GtkWidget *self, void *data)
{
   //cout << "void RGPReferencesWindow::cbMoveColumnUp()" << endl;
   RGPreferencesWindow *me = (RGPreferencesWindow*)data;

   GtkTreeIter iter, next;
   GtkTreePath *first = gtk_tree_path_new_first();

   GtkTreeSelection* selection;
   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_treeView));
   if(!gtk_tree_selection_get_selected (selection, NULL,
					&iter)) {
      return;
   }
   next = iter;
   GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(me->_listColumns), &iter);
   gtk_tree_path_prev(path);
   gtk_tree_model_get_iter(GTK_TREE_MODEL(me->_listColumns), &next, path);
   
   if(gtk_tree_path_compare(path, first) == 0)
      gtk_list_store_move_after(me->_listColumns, &iter, NULL);
   else
      gtk_list_store_move_before(me->_listColumns, &iter, &next);
   
}

void RGPreferencesWindow::cbMoveColumnDown(GtkWidget *self, void *data)
{
   //cout << "void RGPReferencesW2indow::cbMoveColumnDown()" << endl;
   RGPreferencesWindow *me = (RGPreferencesWindow*)data;

   GtkTreeIter iter, next;

   GtkTreeSelection* selection;
   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_treeView));
   if(!gtk_tree_selection_get_selected (selection, NULL,
					&iter)) {
      return;
   }
   next = iter;
   if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(me->_listColumns), &next))
      return;
   gtk_list_store_move_after(me->_listColumns, &iter, &next);
}

void RGPreferencesWindow::cbToggleColumn(GtkWidget *self, char*path_string,
					 void *data)
{
   //cout << "void RGPReferencesWindow::cbToggle()" << endl;
   RGPreferencesWindow *me = (RGPreferencesWindow*)data;
   GtkTreeIter iter;
   gboolean res;

   GtkTreeModel *model = (GtkTreeModel*)me->_listColumns;
   GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
   gtk_tree_model_get_iter(model, &iter, path);
   gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
		      TREE_CHECKBOX_COLUMN, &res, -1);
   gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		      TREE_CHECKBOX_COLUMN, !res,
		      -1);

}


void RGPreferencesWindow::colorClicked(GtkWidget *self, void *data)
{
   GtkWidget *color_dialog;
   GtkWidget *color_selection;
   RGPreferencesWindow *me;
   me = (RGPreferencesWindow *) g_object_get_data(G_OBJECT(self), "me");

   color_dialog = gtk_color_selection_dialog_new(_("Color selection"));
   color_selection =
	gtk_color_selection_dialog_get_color_selection(
		GTK_COLOR_SELECTION_DIALOG(color_dialog));

   GdkColor *color = NULL;
   color = RGPackageStatus::pkgStatus.getColor(GPOINTER_TO_INT(data));
   if (color != NULL)
      gtk_color_selection_set_current_color(GTK_COLOR_SELECTION
                                            (color_selection), color);

   if (gtk_dialog_run(GTK_DIALOG(color_dialog)) == GTK_RESPONSE_OK) {
      GdkColor current_color;
      gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(color_selection), &current_color);
      RGPackageStatus::pkgStatus.setColor(GPOINTER_TO_INT(data),
					  gdk_color_copy(&current_color));
      me->readColors();
   }
   gtk_widget_destroy(color_dialog);
}

void RGPreferencesWindow::useProxyToggled(GtkWidget *self, void *data)
{
   //cout << "void RGPreferencesWindow::useProxyToggled() " << endl;
   bool useProxy;

   RGPreferencesWindow *me = (RGPreferencesWindow *) data;
   useProxy = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_useProxy));
   gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(me->_builder,
                                                              "table_proxy")),
                            useProxy);
}

void RGPreferencesWindow::checkbuttonUserFontToggled(GtkWidget *self,
                                                     void *data)
{
   RGPreferencesWindow *me = (RGPreferencesWindow *) data;

   GtkWidget *button =
      GTK_WIDGET(gtk_builder_get_object(me->_builder, "button_default_font"));
   GtkWidget *check =
      GTK_WIDGET(gtk_builder_get_object(me->_builder, "checkbutton_user_font"));
   gtk_widget_set_sensitive(button,
                            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                         (check)));
}

void RGPreferencesWindow::checkbuttonUserTerminalFontToggled(GtkWidget *self,
                                                             void *data)
{
   RGPreferencesWindow *me = (RGPreferencesWindow *) data;

   GtkWidget *button =
      GTK_WIDGET(gtk_builder_get_object(me->_builder, "button_terminal_font"));
   GtkWidget *check =
      GTK_WIDGET(gtk_builder_get_object(me->_builder,
                                        "checkbutton_user_terminal_font"));
   gtk_widget_set_sensitive(button,
                            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                         (check)));
}



RGPreferencesWindow::RGPreferencesWindow(RGWindow *win,
                                         RPackageLister *lister)
   : RGGtkBuilderWindow(win, "preferences"), distroChanged(false)
{
   GtkWidget *button;
   GtkCellRenderer *crt;
   GtkListStore *comboStore;
   GtkTreeIter comboIter;

   _css_provider = gtk_css_provider_new ();   
   _optionShowAllPkgInfoInMain = GTK_WIDGET(gtk_builder_get_object
                                            (_builder,
                                             "check_show_all_pkg_info"));
   _optionUseStatusColors =
      GTK_WIDGET(gtk_builder_get_object(_builder, "check_use_colors"));

   _optionAskRelated =
      GTK_WIDGET(gtk_builder_get_object(_builder, "check_ask_related"));
   _optionUseTerminal =
      GTK_WIDGET(gtk_builder_get_object(_builder, "check_terminal"));
   _optionCheckRecom =
      GTK_WIDGET(gtk_builder_get_object(_builder, "check_recommends"));
   _optionAskQuit =
      GTK_WIDGET(gtk_builder_get_object(_builder, "check_ask_quit"));
   _optionOneClick =
      GTK_WIDGET(gtk_builder_get_object(_builder, "check_oneclick"));

   // cache
   _cacheLeave = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                   "radio_cache_leave"));
   _cacheClean = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                   "radio_cache_del_after"));
   // history
   _delHistory = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                   "radio_delete_history"));
   _keepHistory = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                    "radio_keep_history"));
   _spinDelHistory = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                       "spin_del_history"));
   _cacheAutoClean =
      GTK_WIDGET(gtk_builder_get_object(_builder, "radio_cache_del_obsolete"));
   _useProxy = GTK_WIDGET(gtk_builder_get_object(_builder, "radio_use_proxy"));
   _mainWin = (RGMainWindow *) win;

   _maxUndoE = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                 "spinbutton_max_undos"));
   assert(_maxUndoE);

   _comboRemovalAction =
      GTK_WIDGET(gtk_builder_get_object(_builder,
                                        "combo_removal_action"));
   
   comboStore = GTK_LIST_STORE(
                   gtk_combo_box_get_model(
                      GTK_COMBO_BOX(_comboRemovalAction)));
   for (int i = 0; removal_actions[i] != NULL; i++) {
      gtk_list_store_append(comboStore, &comboIter);
      gtk_list_store_set(comboStore, &comboIter, 0, _(removal_actions[i]), -1);
   }
   crt = gtk_cell_renderer_text_new();
   gtk_cell_layout_clear(GTK_CELL_LAYOUT(_comboRemovalAction));
   gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_comboRemovalAction), crt, TRUE);
   gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_comboRemovalAction),
                                          crt, "text", 0);
   gtk_combo_box_set_active(GTK_COMBO_BOX(_comboRemovalAction), 0);
   assert(_comboRemovalAction);

   _comboUpdateAsk = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                       "combo_update_ask"));
   comboStore = GTK_LIST_STORE(
                   gtk_combo_box_get_model(
                      GTK_COMBO_BOX(_comboUpdateAsk)));
   for (int i = 0; update_ask[i] != NULL; i++) {
      gtk_list_store_append(comboStore, &comboIter);
      gtk_list_store_set(comboStore, &comboIter, 0, _(update_ask[i]), -1);
   }
   crt = gtk_cell_renderer_text_new();
   gtk_cell_layout_clear(GTK_CELL_LAYOUT(_comboUpdateAsk));
   gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_comboUpdateAsk), crt, TRUE);
   gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_comboUpdateAsk),
                                          crt, "text", 0);
   gtk_combo_box_set_active(GTK_COMBO_BOX(_comboUpdateAsk), 0);
   assert(_comboUpdateAsk);
   
   _comboUpgradeMethod = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                       "combo_upgrade_method"));
   comboStore = GTK_LIST_STORE(
                   gtk_combo_box_get_model(
                      GTK_COMBO_BOX(_comboUpgradeMethod)));
   for (int i = 0; upgrade_method[i] != NULL; i++) {
      gtk_list_store_append(comboStore, &comboIter);
      gtk_list_store_set(comboStore, &comboIter, 0, _(upgrade_method[i]), -1);
   }
   crt = gtk_cell_renderer_text_new();
   gtk_cell_layout_clear(GTK_CELL_LAYOUT(_comboUpgradeMethod));
   gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_comboUpgradeMethod), crt, TRUE);
   gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_comboUpgradeMethod),
                                          crt, "text", 0);
   gtk_combo_box_set_active(GTK_COMBO_BOX(_comboUpgradeMethod), 0);
   assert(_comboUpgradeMethod);

   _comboDefaultDistro =
      GTK_WIDGET(gtk_builder_get_object(_builder, "combobox_default_distro"));
   assert(_comboDefaultDistro);
   gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object
                                   (_builder, "radiobutton_distro")),
		       _("Prefer package versions from the selected "
		       "distribution when upgrading packages. If you "
		       "manually force a version from a different "
		       "distribution, the package version will follow "
		       "that distribution until it enters the default "
		       "distribution."));
   gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object	
                                   (_builder, "radiobutton_now")),
		       _("Never upgrade to a new version automatically. "
			    "Be _very_ careful with this option as you will "
			    "not get security updates automatically! "
		       "If you manually force a version "
		       "the package version will follow "
		       "the chosen distribution."));
   gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object
                                   (_builder,"radiobutton_ignore")),
			_("Let synaptic pick the best version for you. "
			"If unsure use this option. "));


   // hide the "remove with configuration" from rpm users
#ifdef HAVE_RPM
   GtkWidget *w = GTK_WIDGET(gtk_builder_get_object(_builder,
                                                    "menuitem_purge"));
   gtk_widget_hide(w);
   int delAction = _config->FindI("Synaptic::delAction", PKG_DELETE);
   // purge not available 
   if (delAction == PKG_PURGE)
      delAction = PKG_DELETE;
   gtk_widget_hide(GTK_WIDGET(_comboRemovalAction));
   gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(_builder,"label_removal"));
#endif

   // set data for the checkbutton
   g_object_set_data(gtk_builder_get_object
                      (_builder, "checkbutton_user_font"), "me", this);
   g_object_set_data(gtk_builder_get_object
                      (_builder, "checkbutton_user_terminal_font"), "me",
                     this);

   // save the lister
   _lister = lister;

   // treeview stuff
   _treeView = GTK_WIDGET(gtk_builder_get_object(_builder, "treeview_columns"));
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;
   renderer = gtk_cell_renderer_toggle_new ();
   g_object_set(renderer, "activatable", TRUE, NULL);
   g_signal_connect(renderer, "toggled", 
 		    (GCallback) cbToggleColumn, this);
   column = gtk_tree_view_column_new_with_attributes(_("Visible"),
                                                      renderer,
                                                      "active", TREE_CHECKBOX_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW(_treeView), column);
   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes (_("Name"),
						      renderer,
						      "text", TREE_VISIBLE_NAME_COLUMN,
						      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW(_treeView), column);


   // lots of signals :)
   g_signal_connect(gtk_builder_get_object(_builder, "button_column_up"),
                    "clicked",
                    G_CALLBACK(cbMoveColumnUp), this);
   g_signal_connect(gtk_builder_get_object(_builder, "button_column_down"),
                    "clicked",
                    G_CALLBACK(cbMoveColumnDown), this);

   g_signal_connect(gtk_builder_get_object(_builder, "close"),
                    "clicked",
                    G_CALLBACK(closeAction), this);
   g_signal_connect(gtk_builder_get_object(_builder, "apply"),
                    "clicked",
                    G_CALLBACK(saveAction), this);
   g_signal_connect(gtk_builder_get_object(_builder, "ok"),
                    "clicked",
                    G_CALLBACK(doneAction), this);

   g_signal_connect(gtk_builder_get_object(_builder, "button_clean_cache"),
                    "clicked",
                    G_CALLBACK(clearCacheAction), this);

   g_signal_connect(gtk_builder_get_object(_builder, "radio_use_proxy"),
                    "toggled",
                    G_CALLBACK(useProxyToggled), this);

   g_signal_connect(gtk_builder_get_object(_builder, "button_authentication"),
                    "clicked",
                    G_CALLBACK(buttonAuthenticationClicked), this);

   g_signal_connect(gtk_builder_get_object(_builder, "button_default_font"),
                    "clicked",
                    G_CALLBACK(changeFontAction),GINT_TO_POINTER(FONT_DEFAULT));

   g_signal_connect(gtk_builder_get_object(_builder, "checkbutton_user_terminal_font"),
                    "toggled",
                    G_CALLBACK (checkbuttonUserTerminalFontToggled), this);
   g_signal_connect(gtk_builder_get_object(_builder, "checkbutton_user_font"),
                    "toggled",
                    G_CALLBACK(checkbuttonUserFontToggled), this);

   g_signal_connect(gtk_builder_get_object(_builder, "button_terminal_font"),
                                 "clicked",
                                 G_CALLBACK(changeFontAction),
                                 GINT_TO_POINTER(FONT_TERMINAL));

   checkbuttonUserTerminalFontToggled(NULL, this);
   checkbuttonUserFontToggled(NULL, this);

   // color stuff
   char *color_button = NULL;
   for (int i = 0; i < RGPackageStatus::N_STATUS_COUNT; i++) {
      color_button =
         g_strdup_printf("button_%s_color",
                         RGPackageStatus::pkgStatus.
                         getShortStatusString(RGPackageStatus::PkgStatus(i)));
      button = GTK_WIDGET(gtk_builder_get_object(_builder, color_button));
      assert(button);
      gtk_style_context_add_provider(
         gtk_widget_get_style_context (button),
         GTK_STYLE_PROVIDER (_css_provider), 
         GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      g_object_set_data(G_OBJECT(button), "me", this);
      g_signal_connect(G_OBJECT(button), "clicked",
                       G_CALLBACK(colorClicked), GINT_TO_POINTER(i));
      g_free(color_button);
   }

   skipTaskbar(true);
   setTitle(_("Preferences"));
}

RGPreferencesWindow::~RGPreferencesWindow()
{
   g_object_unref(_css_provider);
}

void 
RGPreferencesWindow::buttonAuthenticationClicked(GtkWidget *self, void *data)
{
   RGPreferencesWindow *me = (RGPreferencesWindow *)data;

   RGGtkBuilderUserDialog dia(me, "authentication");
   GtkBuilder *dia_xml = dia.getGtkBuilder();
   GtkWidget *entry_user = GTK_WIDGET(gtk_builder_get_object(dia_xml,"entry_username"));
   GtkWidget *entry_pass = GTK_WIDGET(gtk_builder_get_object(dia_xml,"entry_password"));

   // now set the values
   string now_user =  _config->Find("Synaptic::httpProxyUser","");
   gtk_entry_set_text(GTK_ENTRY(entry_user), now_user.c_str());
   string now_pass =   _config->Find("Synaptic::httpProxyPass","");
   gtk_entry_set_text(GTK_ENTRY(entry_pass), now_pass.c_str());

   int res = dia.run();

   if(!res) 
      return;

   // get the entered data
   const gchar *user = gtk_entry_get_text(GTK_ENTRY(entry_user));
   const gchar *pass = gtk_entry_get_text(GTK_ENTRY(entry_pass));
   
   // write out the configuration   
   _config->Set("Synaptic::httpProxyUser",user);
   _config->Set("Synaptic::httpProxyPass",pass);
}

// vim:ts=3:sw=3:et
