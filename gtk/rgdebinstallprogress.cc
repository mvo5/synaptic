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

#ifdef WITH_DPKG_STATUSFD

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "rgmainwindow.h"
#include "gsynaptic.h"

#include "rgdebinstallprogress.h"
#include "rguserdialog.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <gtk/gtk.h>

#include <unistd.h>
#include <stdio.h>

#include <vte/vte.h>


#include "i18n.h"

// removing
char* RGDebInstallProgress::remove_stages[NR_REMOVE_STAGES] = {
   "half-configured", 
   "half-installed", 
   "config-files"};
char* RGDebInstallProgress::remove_stages_translations[NR_REMOVE_STAGES] = {
   N_("Preparing for removal %s"),
   N_("Removing %s"),
   N_("Removed %s")};

// purging
char *RGDebInstallProgress::purge_stages[NR_PURGE_STAGES] = { 
   "half-configured",
   "half-installed", 
   "config-files", 
   "not-installed"};
char *RGDebInstallProgress::purge_stages_translations[NR_PURGE_STAGES] = { 
   N_("Preparing for removal %s"),
   N_("Removing with config %s"), 
   N_("Removed %s"), 
   N_("Removed with config %s")};

// purge only (for packages that are alreay removed)
char *RGDebInstallProgress::purge_only_stages[NR_PURGE_ONLY_STAGES] = { 
   "config-files", 
   "not-installed"};
char *RGDebInstallProgress::purge_only_stages_translations[NR_PURGE_ONLY_STAGES] = { 
   N_("Removing with config %s"), 
   N_("Removed with config %s")};

// install 
char *RGDebInstallProgress::install_stages[NR_INSTALL_STAGES] = { 
   "half-installed",
   "unpacked",
   "half-configured",
   "installed"};
char *RGDebInstallProgress::install_stages_translations[NR_INSTALL_STAGES] = { 
   N_("Preparing %s"),
   N_("Unpacking %s"),
   N_("Configuring %s"),
   N_("Installed %s")};

// update
char *RGDebInstallProgress::update_stages[NR_UPDATE_STAGES] = { 
   "unpack",
   "half-installed", 
   "unpacked",
   "half-configured",
   "installed"};
char *RGDebInstallProgress::update_stages_translations[NR_UPDATE_STAGES] = { 
   N_("Preparing %s"),
   N_("Installing %s"), 
   N_("Unpacking %s"),
   N_("Configuring %s"),
   N_("Installed %s")};

//reinstall
char *RGDebInstallProgress::reinstall_stages[NR_REINSTALL_STAGES] = { 
   "half-configured",
   "unpacked",
   "half-installed", 
   "unpacked",
   "half-configured",
   "installed" };
char *RGDebInstallProgress::reinstall_stages_translations[NR_REINSTALL_STAGES] = { 
   N_("Preparing %s"),
   N_("Unpacking %s"),
   N_("Installing %s"), 
   N_("Unpacking %s"),
   N_("Configuring %s"),
   N_("Installed %s") };


void RGDebInstallProgress::child_exited(VteReaper *vtereaper,
					gint child_pid, gint ret, 
					gpointer data)
{
   RGDebInstallProgress *me = (RGDebInstallProgress*)data;
   if(child_pid == me->_child_id) {
//       cout << "child exited" << endl;
//       cout << "waitpid returned: " << WEXITSTATUS(ret) << endl;
      me->res = (pkgPackageManager::OrderResult)WEXITSTATUS(ret);
      me->child_has_exited=true;
   }
}


ssize_t
write_fd(int fd, void *ptr, size_t nbytes, int sendfd)
{
        struct msghdr   msg;
        struct iovec    iov[1];

        union {
          struct cmsghdr        cm;
          char                          control[CMSG_SPACE(sizeof(int))];
        } control_un;
        struct cmsghdr  *cmptr;

        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);

        cmptr = CMSG_FIRSTHDR(&msg);
        cmptr->cmsg_len = CMSG_LEN(sizeof(int));
        cmptr->cmsg_level = SOL_SOCKET;
        cmptr->cmsg_type = SCM_RIGHTS;
        *((int *) CMSG_DATA(cmptr)) = sendfd;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;

        iov[0].iov_base = ptr;
        iov[0].iov_len = nbytes;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

        return(sendmsg(fd, &msg, 0));
}



ssize_t
read_fd(int fd, void *ptr, size_t nbytes, int *recvfd)
{
        struct msghdr   msg;
        struct iovec    iov[1];
        ssize_t                 n;
        int                             newfd;

        union {
          struct cmsghdr        cm;
          char                          control[CMSG_SPACE(sizeof(int))];
        } control_un;
        struct cmsghdr  *cmptr;

        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);

        msg.msg_name = NULL;
        msg.msg_namelen = 0;

        iov[0].iov_base = ptr;
        iov[0].iov_len = nbytes;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

        if ( (n = recvmsg(fd, &msg, MSG_WAITALL)) <= 0)
                return(n);
        if ( (cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
            cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
	   if (cmptr->cmsg_level != SOL_SOCKET) {
	      perror("control level != SOL_SOCKET");
	      exit(1);
	   }
	   if (cmptr->cmsg_type != SCM_RIGHTS) {
	      perror("control type != SCM_RIGHTS");
	      exit(1);
	   }
                *recvfd = *((int *) CMSG_DATA(cmptr));
        } else
                *recvfd = -1;           /* descriptor was not passed */
        return(n);
}
/* end read_fd */

#define UNIXSTR_PATH "/var/run/synaptic.socket"

int ipc_send_fd(int fd)
{
   // open connection to server
   struct sockaddr_un servaddr;
   int serverfd = socket(AF_LOCAL, SOCK_STREAM, 0);
   bzero(&servaddr, sizeof(servaddr));
   servaddr.sun_family = AF_LOCAL;
   strcpy(servaddr.sun_path, UNIXSTR_PATH);

   // wait for the socket to come up
   while(connect(serverfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0
	 && errno == ECONNREFUSED) 
    usleep(10000);

   // send fd to server
   write_fd(serverfd, (void*)"",1,fd);
   return 0;
}

int ipc_recv_fd()
{
   int ret;

   // setup socket
   struct sockaddr_un servaddr,cliaddr;
   char c;
   int connfd,fd;
   
   int listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);
   unlink(UNIXSTR_PATH);
   bzero(&servaddr, sizeof(servaddr));
   servaddr.sun_family = AF_LOCAL;
   strcpy(servaddr.sun_path, UNIXSTR_PATH);
   bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
   listen(listenfd, 1);

   // wait for connections
   socklen_t clilen = sizeof(cliaddr);
   connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
   
   // read_fd 
   read_fd(connfd, &c,1,&fd);

   close(connfd);

   return fd;
}



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
   for(;status[i] != '\'';i++) 
      /*nothing*/
      ;
   i++;
   for(;status[i] != '\'';i++) 
      orig_file.append(1,status[i]);
   i++;

   // same for second ' and read until the end
   for(;status[i] != '\'';i++) 
      /*nothing*/
      ;
   i++;
   for(;status[i] != '\'';i++) 
      new_file.append(1,status[i]);
   i++;
   //cout << "got:" << orig_file << new_file << endl;

   // read diff
   string diff;
   char buf[512];
   char *cmd = g_strdup_printf("/usr/bin/diff -u %s %s", orig_file.c_str(), new_file.c_str());
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
      vte_terminal_feed_child(VTE_TERMINAL(_term), "y\n",2);
   else
      vte_terminal_feed_child(VTE_TERMINAL(_term), "n\n",2);
}

void RGDebInstallProgress::startUpdate()
{
   child_has_exited=false;
   VteReaper* reaper = vte_reaper_get();
   g_signal_connect(G_OBJECT(reaper), "child-exited",
		    G_CALLBACK(child_exited),
		    this);
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

   _startCounting = false;
   _label_status = glade_xml_get_widget(_gladeXML, "label_status");
   _labelSummary = glade_xml_get_widget(_gladeXML, "label_summary");
   _pbarTotal = glade_xml_get_widget(_gladeXML, "progress_total");
   //_image = glade_xml_get_widget(_gladeXML, "image");

   gtk_label_set_text(GTK_LABEL(_labelSummary), "");

   _term = vte_terminal_new();
   vte_terminal_set_size(VTE_TERMINAL(_term),80,23);
   GtkWidget *scrollbar = 
      gtk_vscrollbar_new (GTK_ADJUSTMENT (VTE_TERMINAL(_term)->adjustment));
   GTK_WIDGET_UNSET_FLAGS (scrollbar, GTK_CAN_FOCUS);
   vte_terminal_set_scrollback_lines(VTE_TERMINAL(_term), 10000);
   if(_config->FindB("Synaptic::useUserTerminalFont")) {
      char *s =(char*)_config->Find("Synaptic::TerminalFontName").c_str();
      vte_terminal_set_font_from_string(VTE_TERMINAL(_term), s);
   } else {
      vte_terminal_set_font_from_string(VTE_TERMINAL(_term), "monospace 10");
   }
   gtk_box_pack_start(GTK_BOX(glade_xml_get_widget(_gladeXML,"hbox_vte")), _term, TRUE, TRUE, 0);
   gtk_widget_show(_term);
   gtk_box_pack_end(GTK_BOX(glade_xml_get_widget(_gladeXML,"hbox_vte")), scrollbar, FALSE, FALSE, 0);
   gtk_widget_show(scrollbar);

   skipTaskbar(true);
}


void RGDebInstallProgress::updateInterface()
{
   char buf[2];
   static char line[1024] = "";

   int i=0;
   while (1) {
      usleep(1000); // sleep a bit so that we don't have 100% cpu

#if 0
      fd_set wfds;
      struct timeval tv;
      int retval;
      FD_ZERO(&wfds);
      FD_SET(_child_control, &wfds);
      tv.tv_sec = 0;
      tv.tv_usec = 1000;
      retval = select(_child_control+1, NULL, &wfds, NULL, &tv);
      if (retval) {
	 printf("Data is available now.\n");
      }
#endif

      // This algorithm should be improved.
      int len = read(_childin, buf, 1);
      if(len == 0) { // happens when the stream is closed
	 child_has_exited=true;
	 break;
      }

      while (gtk_events_pending())
	 gtk_main_iteration();

      if (len < 1 ) { // no data
	 if(errno == 0 || errno == EAGAIN) { // harmless, try again
	    continue;
	 } else {
	    child_has_exited=true;
	    break;
	 }
      }

      // got complete line
      if( buf[0] == '\n') {
// 	 cout << line << endl;
	 
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
	 } else if(_actionsMap.count(pkg) == 0) {
	    // no known dpkg state (happens e.g if apt reports:
	    // /bin/sh: apt-listchanges: command-not-found
	    continue;
	 } else {
	    // then go on with the package stuff
	    char *next_stage_str = NULL;
	    int next_stage = _stagesMap[pkg];
	    // is a element is not found in the map, NULL is returned
	    // (this happens when dpkg does some work left from a previous
	    //  session (rare but happens))
	    
	    char **states = _actionsMap[pkg]; 
	    char **translations = _translationsMap[pkg]; 
	    if(states && translations) {
	       next_stage_str = states[next_stage];
// 	       cout << "waiting for: " << next_stage_str << endl;
	       if(next_stage_str && (strstr(status, next_stage_str) != NULL)) {
		  s = g_strdup_printf(_(translations[next_stage]), split[1]);
		  next_stage++;
		  _stagesMap[pkg] = next_stage;
		  _progress++;
	       }
	    }
	 }

	 // each package goes through three stages
	 float val = ((float)_progress)/((float)_totalActions);
// 	 cout << _progress << "/" << _totalActions << " = " << val << endl;
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

pkgPackageManager::OrderResult RGDebInstallProgress::start(RPackageManager *pm,
                                                       int numPackages,
                                                       int numPackagesTotal)
{
   void *dummy;
   pkgPackageManager::OrderResult res;
   int ret;
   pid_t _child_id;

//    cout << "RInstallProgress::start()" << endl;

   res = pm->DoInstallPreFork();
   if (res == pkgPackageManager::Failed)
       return res;

   /*
    * This will make a pipe from where we can read child's output
    */
   _child_id = vte_terminal_forkpty(VTE_TERMINAL(_term),NULL,NULL,false,false,false);
   if (_child_id == 0) {
      int fd[2];
      pipe(fd);
      ipc_send_fd(fd[0]); // send the read part of the pipe to the parent

#ifdef WITH_DPKG_STATUSFD
      res = pm->DoInstallPostFork(fd[1]);
#else
      res = pm->DoInstallPostFork();
#endif

      // dump errors into cerr (pass it to the parent process)	
      _error->DumpErrors();

      ::close(fd[0]);
      ::close(fd[1]);

      _exit(res);
   }
   _childin = ipc_recv_fd();

   // make it nonblocking
   fcntl(_childin, F_SETFL, O_NONBLOCK);

   _donePackages = 0;
   _numPackages = numPackages;
   _numPackagesTotal = numPackagesTotal;

   startUpdate();
   while(!child_has_exited)
      updateInterface();

   finishUpdate();

   ::close(_childin);
   ::close(_child_control);

   return res;
}



void RGDebInstallProgress::finishUpdate()
{
   if (_startCounting) {
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
	 _translationsMap.insert(pair<string,char**>(name, purge_stages_translations));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_PURGE_STAGES;
      } else if((flags & RPackage::FPurge)&& 
		(!(flags & RPackage::FInstalled)||(flags&RPackage::FOutdated))){
	 _actionsMap.insert(pair<string,char**>(name, purge_only_stages));
	 _translationsMap.insert(pair<string,char**>(name, purge_only_stages_translations));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_PURGE_ONLY_STAGES;
      } else if(flags & RPackage::FRemove) {
	 _actionsMap.insert(pair<string,char**>(name, remove_stages));
	 _translationsMap.insert(pair<string,char**>(name, remove_stages_translations));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_REMOVE_STAGES;
      } else if(flags & RPackage::FNewInstall) {
	 _actionsMap.insert(pair<string,char**>(name, install_stages));
	 _translationsMap.insert(pair<string,char**>(name, install_stages_translations));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_INSTALL_STAGES;
      } else if(flags & RPackage::FReInstall) {
	 _actionsMap.insert(pair<string,char**>(name, reinstall_stages));
	 _translationsMap.insert(pair<string,char**>(name, reinstall_stages_translations));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_REINSTALL_STAGES;
      } else if((flags & RPackage::FUpgrade)||(flags & RPackage::FDowngrade)) {
	 _actionsMap.insert(pair<string,char**>(name, update_stages));
	 _translationsMap.insert(pair<string,char**>(name, update_stages_translations));
	 _stagesMap.insert(pair<string,int>(name, 0));
	 _totalActions += NR_UPDATE_STAGES;
      }
   }
}

#endif


// vim:ts=3:sw=3:et
