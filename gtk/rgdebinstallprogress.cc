/* rgdebinstallprogress.cc
 *
 * Copyright (c) 2004 Canonical
 *
 * Author: Michael Vogt <mvo@debian.org>
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
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include "rgmainwindow.h"
#include "gsynaptic.h"

#include "rgdebinstallprogress.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <gtk/gtk.h>

#include <unistd.h>
#include <stdio.h>

#include "i18n.h"


void RGDebInstallProgress::startUpdate()
{
   show();
   RGFlushInterface();
}

RGDebInstallProgress::RGDebInstallProgress(RGMainWindow *main,
					   RPackageLister *lister)
   : RInstallProgress(), RGGladeWindow(main, "rgdebinstall_progress"),
     _numRemovals(0)
{
   prepare(lister);
   setTitle(_("Applying Changes"));

   // make sure that debconf is not run in the terminal
   setenv("DEBIAN_FRONTEND","gnome",1);

   _startCounting = false;
   _label_status = glade_xml_get_widget(_gladeXML, "label_status");
   _labelSummary = glade_xml_get_widget(_gladeXML, "label_summary");
   _pbar = glade_xml_get_widget(_gladeXML, "progress_package");
   _pbarTotal = glade_xml_get_widget(_gladeXML, "progress_total");
   //_image = glade_xml_get_widget(_gladeXML, "image");

   gtk_label_set_text(GTK_LABEL(_labelSummary), "");
   gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbar));
   gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(_pbar), 0.01);

   skipTaskbar(true);
}


void RGDebInstallProgress::updateInterface()
{
   char buf[2];
   static char line[1024] = "";

   int i=0;
   while (1) {
      usleep(1000); // sleep a bit so that we don't have 100% cpu

      // This algorithm should be improved.
      int len = read(_childin, buf, 1);
      if(len == 0) { // happens when the stream is closed
	 break;
      }

#if 0 // this is only used for the old pulse style listbox
      gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbar));
#endif

      while (gtk_events_pending())
	 gtk_main_iteration();

      if (len < 1) { // no data
	 continue;
      }

      // got complete line
      if( buf[0] == '\n') {
	 //cout << line << endl;
	 
	 _startCounting = true;
	 gchar **split = g_strsplit(line, ":",4);

	 gchar *s=NULL;
	 gchar *pkg = split[1];
	 gchar *status = split[2];
	 // major problem here, we got unexpected input. should _never_ happen
	 if(!(pkg && status))
	    continue;

	 // stages
	 //install packages:
	 // "unpack","half-configured","half-installed", "installed"
	 //removed packages:
	 // "half-configured","half-installed", "config-files", "not-installed"
	 if(strstr(status,"unpacked") != NULL) {
	    s = g_strdup_printf(_("Unpacking %s"), split[1]);
	    if(_unpackSeen.find(string(pkg)) == _unpackSeen.end()) {
	       _unpackSeen.insert(string(pkg));
	       _donePackages += 1;
	    }
	 } else if(strstr(split[2], "half-configured") != NULL) {
	    s = g_strdup_printf(_("Configuring %s"), split[1]);
	    if(_configuredSeen.find(string(pkg)) == _configuredSeen.end()) {
	       _configuredSeen.insert(string(pkg));
	       _donePackages += 1;
	    }
	 } else if(strstr(split[2], "half-installed") != NULL) {
	    s = g_strdup_printf(_("Installing %s"), split[1]);
	    if(_installedSeen.find(string(pkg)) == _installedSeen.end()) {
	       _installedSeen.insert(string(pkg));
	       _donePackages += 1;
	    } 
	 }else if(strstr(split[2], "installed") != NULL) {
	    //s = g_strdup_printf(_("Installed %s"), split[1]);
	 } else if(strstr(split[2], "config-files") != NULL) {
	    if(_removeSeen.find(string(pkg)) == _removeSeen.end()) {
	       _removeSeen.insert(string(pkg));
	       _donePackages++;
	    }
	    s = g_strdup_printf(_("Removing %s"), split[1]);
	 } else if(strstr(split[2], "not-installed") != NULL) { 
	    s = g_strdup_printf(_("Removing with config %s"), split[1]);
	    //_donePackages += 1;
	 } else if(strstr(split[2], "error") != NULL) { 
	    // error from dpkg
	    s = g_strdup_printf(_("Error in package %s"), split[1]);
	    string err = split[1] + string(": ") + split[3];
	    _error->Error(err.c_str());
	 } else if(strstr(split[2], "conffile-prompt") != NULL) {
	    // conffile-request
	    //cout << split[2] << " " << split[3] << endl;
	    GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(window()),
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_QUESTION,
							GTK_BUTTONS_YES_NO,
							"Conffile changed '%s'\n"
							"Install new?",
							split[1]);
	    int res = gtk_dialog_run (GTK_DIALOG (dialog));
	    gtk_widget_destroy (dialog);
	    
	    // get a lot of broken pipes here
	    if(res ==  GTK_RESPONSE_YES)
	       write(_child_control,"y\n",2);
	    else
	       write(_child_control,"n\n",2);
	 }

	 // each package goes through three stages
	 float val = ((float)_donePackages)/((float)(_numPackagesTotal+_numRemovals)*3.0);
	 //cout << _donePackages << "/" << (_numPackagesTotal+_numRemovals)*3.0
	 //<< " = " << val << endl;
	 gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbarTotal), val);
	 if(s!=NULL)
	    gtk_label_set(GTK_LABEL(_label_status),s);
	 
	 // clean-up
	 g_strfreev(split);
	 g_free(s);
	 line[0] = 0;
      } else {
	 buf[1] = 0;
	 strcat(line, buf);
      }      
   }

   gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbarTotal),1.0);
   while (gtk_events_pending())
      gtk_main_iteration();
   
}



void RGDebInstallProgress::finishUpdate()
{
   if (_startCounting) {
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbar), 1.0);
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbarTotal), 1.0);
   }
   RGFlushInterface();

   hide();
}

void RGDebInstallProgress::prepare(RPackageLister *lister)
{
   //cout << "prepeare called" << endl;

   for (unsigned int row = 0; row < lister->packagesSize(); row++) {
      RPackage *pkg = lister->getPackage(row);

      int flags = pkg->getFlags();
      if((flags & RPackage::FRemove) || (flags & RPackage::FPurge)) {
	 _numRemovals++;
      }
   }
}



// vim:ts=3:sw=3:et
