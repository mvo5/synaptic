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
#include "rguserdialog.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <gtk/gtk.h>

#include <unistd.h>
#include <stdio.h>

#include "i18n.h"

char* RGDebInstallProgress::remove_stages[NR_REMOVE_STAGES] = {
   "half-configured",
   "half-installed", 
   "config-files"};
char *RGDebInstallProgress::purge_stages[NR_PURGE_STAGES] = { 
   "half-configured",
   "half-installed", 
   "config-files", 
   "not-installed"};
char *RGDebInstallProgress::purge_only_stages[NR_PURGE_ONLY_STAGES] = { 
   "config-files", 
   "not-installed"};
char *RGDebInstallProgress::install_stages[NR_INSTALL_STAGES] = { 
   "half-installed",
   "unpacked",
   "half-configured",
   "installed"};
char *RGDebInstallProgress::update_stages[NR_UPDATE_STAGES] = { 
   "unpack",
   "half-installed", 
   "unpacked",
   "half-configured",
   "installed"};
char *RGDebInstallProgress::reinstall_stages[NR_REINSTALL_STAGES] = { 
   "half-configured",
   "unpacked",
   "half-installed", 
   "unpacked",
   "half-configured",
   "installed" };

void RGDebInstallProgress::conffile(gchar *conffile, gchar *status)
{
   string primary, secondary;
   gchar *m,*s,*p;
   GtkWidget *w;
   RGGladeUserDialog dia(this, "conffile");
   GladeXML *xml = dia.getGladeXML();

   p = g_strdup_printf(_("Install new configuration file\n'%s'?"),conffile);
   s = g_strdup_printf(_("The configuration file %s was modified (by "
			 "you or by a script). A updated version is shipped "
			 "in this package. If you want to keep your current "
			 "version say 'No'. Do you want to install the "
			 "new package maintainers version? "),conffile);

   // setup dialog
   w = glade_xml_get_widget(xml, "label_message");
   m = g_strdup_printf("<span weight=\"bold\" size=\"larger\">%s </span> "
		       "\n\n%s", p, s);
   gtk_label_set_markup(GTK_LABEL(w), m);
   g_free(p);
   g_free(s);
   g_free(m);

   // diff stuff
   bool quot=false;
   int i=0;
   string orig_file, new_file;

   // FIXME: add some sanity checks here

   // go to first ' and read until the end
   for(;status[i] != '\'';i++) /*nothing*/;
   i++;
   for(;status[i] != '\'';i++) 
      orig_file.append(1,status[i]);
   i++;

   // same for second ' and read until the end
   for(;status[i] != '\'';i++) /*nothing*/;
   i++;
   for(;status[i] != '\'';i++) 
      new_file.append(1,status[i]);
   i++;
   //cout << "got:" << orig_file << new_file << endl;

   // read diff
   string diff;
   char buf[512];
   char *cmd = g_strdup_printf("diff -u %s %s", orig_file.c_str(), new_file.c_str());
   FILE *f = popen(cmd,"r");
   while(fgets(buf,512,f) != NULL) {
      diff += utf8(buf);
   }
   pclose(f);
   g_free(cmd);

   // set into buffer
   GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(glade_xml_get_widget(xml,"textview_diff")));
   gtk_text_buffer_set_text(text_buffer,diff.c_str(),-1);

   int res = dia.run(NULL,true);
   if(res ==  GTK_RESPONSE_YES)
      write(_child_control,"y\n",2);
   else
      write(_child_control,"n\n",2);

}

void RGDebInstallProgress::startUpdate()
{
   show();
   RGFlushInterface();
}

RGDebInstallProgress::RGDebInstallProgress(RGMainWindow *main,
					   RPackageLister *lister)
   : RInstallProgress(), RGGladeWindow(main, "rgdebinstall_progress"),
     _totalActions(0), _progress(0)

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

   // translate the dpkg status into human a readable status
   _transDpkgStates.insert(make_pair("error",N_("Error in %s")));
   _transDpkgStates.insert(make_pair("conffile-prompt",N_("Config-File %s")));
   _transDpkgStates.insert(make_pair("unpacked",N_("Unpacking %s")));
   _transDpkgStates.insert(make_pair("half-configured",N_("Configuring %s")));
   _transDpkgStates.insert(make_pair("half-installed",N_("Installing %s")));
   _transDpkgStates.insert(make_pair("installed", N_("Installed %s")));
   _transDpkgStates.insert(make_pair("config-files",N_("Removing %s")));
   _transDpkgStates.insert(make_pair("not-installed", N_("Removing with config %s")));

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

      while (gtk_events_pending())
	 gtk_main_iteration();

      if (len < 1) { // no data
	 continue;
      }

      // got complete line
      if( buf[0] == '\n') {
	 cout << line << endl;
	 
	 _startCounting = true;
	 gchar **split = g_strsplit(line, ":",4);

	 gchar *s=NULL;
	 gchar *pkg = g_strstrip(split[1]);
	 gchar *status = g_strstrip(split[2]);
	 // major problem here, we got unexpected input. should _never_ happen
	 if(!(pkg && status))
	    continue;

	 // first check for errors and conf-file prompts
	 if(strstr(status, "error") != NULL) { 
	    // error from dpkg
	    s = g_strdup_printf(_("Error in package %s"), split[1]);
	    string err = split[1] + string(": ") + split[3];
	    _error->Error(err.c_str());
	 } else if(strstr(status, "conffile-prompt") != NULL) {
	    // conffile-request
	    //cout << split[2] << " " << split[3] << endl;
	    conffile(pkg, split[3]);
	 } else {

	    // then go on with the package stuff
	    char *next_stage_str = NULL;
	    int next_stage = _stagesMap[pkg];
	    // is a element is not found in the map, NULL is returned
	    // (this happens when dpkg does some work left from a previous
	    //  session (rare but happens))
	    char **states = _actionsMap[pkg]; 
	    if(states) {
	       next_stage_str = states[next_stage];
	       cout << "waiting for: " << next_stage_str << endl;
	       if(next_stage_str && (strstr(status, next_stage_str) != NULL)) {
		  next_stage++;
		  _stagesMap[pkg] = next_stage;
		  _progress++;
	       }
	    }
	 }
	 
	 if(_transDpkgStates.find(status) != _transDpkgStates.end())
	    s = g_strdup_printf(_(_transDpkgStates[status].c_str()), split[1]);

	 // each package goes through three stages
	 float val = ((float)_progress)/((float)_totalActions);
	 cout << _progress << "/" << _totalActions << " = " << val << endl;
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
      string name = pkg->name();

      if((flags & RPackage::FPurge)&&
	 ((flags & RPackage::FInstalled)||(flags&RPackage::FOutdated))){
	 _actionsMap.insert(pair<string,char**>(name, purge_stages));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_PURGE_STAGES;
      } else if((flags & RPackage::FPurge)&& 
		(!(flags & RPackage::FInstalled)||(flags&RPackage::FOutdated))){
	 _actionsMap.insert(pair<string,char**>(name, purge_only_stages));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_PURGE_ONLY_STAGES;
      } else if(flags & RPackage::FRemove) {
	 _actionsMap.insert(pair<string,char**>(name, remove_stages));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_REMOVE_STAGES;
      } else if(flags & RPackage::FNewInstall) {
	 _actionsMap.insert(pair<string,char**>(name, install_stages));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_INSTALL_STAGES;
      } else if(flags & RPackage::FReInstall) {
	 _actionsMap.insert(pair<string,char**>(name, reinstall_stages));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_REINSTALL_STAGES;
      } else if((flags & RPackage::FUpgrade)||(flags & RPackage::FDowngrade)) {
	 _actionsMap.insert(pair<string,char**>(name, update_stages));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_UPDATE_STAGES;
      }
   }
}



// vim:ts=3:sw=3:et
