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
   : RInstallProgress(), RGGladeWindow(main, "rgdebinstall_progress")
{
   prepare(lister);
   setTitle(_("Applying Changes"));

   _startCounting = false;

   _label = glade_xml_get_widget(_gladeXML, "label_name");
   _label_status = glade_xml_get_widget(_gladeXML, "label_status");
   _labelSummary = glade_xml_get_widget(_gladeXML, "label_summary");
   _pbar = glade_xml_get_widget(_gladeXML, "progress_package");
   _pbarTotal = glade_xml_get_widget(_gladeXML, "progress_total");
   //_image = glade_xml_get_widget(_gladeXML, "image");

   gtk_label_set_text(GTK_LABEL(_label), "");
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
      usleep(1000);

      // This algorithm should be improved.
      int len = read(_childin, buf, 1);
      if(len == 0) {
	 cout << "len == 0" << endl;
	 break;
      }
      if (len < 1) {
	 continue;
      }

      gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbar));
      if (gtk_events_pending()) 
	 while (gtk_events_pending())
	    gtk_main_iteration();

      // got complete line
      if( buf[0] == '\n') {
	 //cout << line << endl;
	 _startCounting = true;
	 //gtk_widget_show(vbox_total);
	 gchar **split = g_strsplit(line, ":",4);

	 gchar *s=NULL;
	 gchar *pkg = split[1];
	 gchar *status = split[2];
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
	 } else if(strstr(split[2], "installed") != NULL) {
	    s = g_strdup_printf(_("Installed %s"), split[1]);
	    if(_installedSeen.find(string(pkg)) == _installedSeen.end()) {
	       _installedSeen.insert(string(pkg));
	       _donePackages += 1;
	    }
	 // FIXME: need a special case for removals and pkg (RemoveSet?)
	 // the problem is that removed package go through the 
	 // "half-configured", "half-installed" stages as well
	 } else if(strstr(split[2], "config-files") != NULL) {
	       s = g_strdup_printf(_("Removed %s"), split[1]);
	 } else if(strstr(split[2], "not-installed") != NULL) { 
	       s = g_strdup_printf(_("Removed with config %s"), split[1]);
	 } else if(strstr(split[2], "error") != NULL) { 
	       s = g_strdup_printf(_("Error in package %s"), split[1]);
	       cout << endl << split[1] <<"print: " 
		    << split[3] << endl << endl;
	       string err = split[1] + string(": ") + split[3];
	       _error->Error(err.c_str());
	 } else if(strstr(split[2], "conffile-prompt") != NULL) {
	    cout << split[2] << " " << split[3] << endl;
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

	 float val = ((float)_donePackages) / ((float)(_numPackages*3.0));
	 cout << _donePackages << "/" << _numPackages*3.0
	      << " = " << val << endl;
	 gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbarTotal),
				       val);
	 if(s!=NULL)
	    gtk_label_set(GTK_LABEL(_label_status),s);
	 
	 // clean-up
	 g_strfreev(split);
	 g_free(s);
	 line[0] = 0;
      }  else {
         buf[1] = 0;
         strcat(line, buf);
      }      
   }

   line[i++] = 0;
   cout << line << endl;
   i=0;
   
   
   if (gtk_events_pending()) {
      while (gtk_events_pending())
	 gtk_main_iteration();
   } else {
      usleep(5000);
      if (_startCounting == false) 
	 gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_pbar));
   }
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
   cout << "prepeare called" << endl;
#if 0
   for (unsigned int row = 0; row < lister->packagesSize(); row++) {
      RPackage *elem = lister->getPackage(row);

      // Is it going to be seen?
      if (!(elem->getFlags() & RPackage::FInstall))
         continue;

      const char *name = elem->name();
      const char *ver = elem->availableVersion();
      const char *pos = strchr(ver, ':');
      if (pos)
         ver = pos + 1;
      string namever = string(name) + "-" + string(ver);
      _summaryMap[namever] = elem->summary();
   }
#endif
}



// vim:ts=3:sw=3:et
