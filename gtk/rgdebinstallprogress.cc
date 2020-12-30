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
#include <math.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <pty.h>

#include "rgmainwindow.h"
#include "gsynaptic.h"

#include "rgdebinstallprogress.h"
#include "rguserdialog.h"


#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/install-progress.h>
#include <gtk/gtk.h>

#include <unistd.h>
#include <cstdio>
#include <cstdlib>

#include <vte/vte.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>


#include "i18n.h"

void RGDebInstallProgress::child_exited(VteTerminal *vteterminal,
					gint ret,
					gpointer data)
{
   RGDebInstallProgress *me = (RGDebInstallProgress*)data;

   me->res = (pkgPackageManager::OrderResult)WEXITSTATUS(ret);
   me->child_has_exited=true;
}

ssize_t
write_fd(int fd, void *ptr, size_t nbytes, int sendfd)
{
        struct msghdr   msg;
        struct iovec    iov[1];

        union {
          struct cmsghdr        cm;
          char   control[CMSG_SPACE(sizeof(int))];
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
        ssize_t  n;
        int newfd;

        union {
          struct cmsghdr        cm;
          char   control[CMSG_SPACE(sizeof(int))];
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
   memset(&servaddr, 0, sizeof(servaddr));
   servaddr.sun_family = AF_LOCAL;
   strcpy(servaddr.sun_path, UNIXSTR_PATH);

   // wait max 5s (5000 * 1000/1000000) for the server
   for(int i=0;i<5000;i++) {
      if(connect(serverfd, (struct sockaddr *)&servaddr, sizeof(servaddr))==0) 
	 break;
      usleep(1000);
   }
   // send fd to server
   write_fd(serverfd, (void*)"",1,fd);
   close(serverfd);
   return 0;
}

int ipc_recv_fd()
{
   int ret;

   // setup socket
   struct sockaddr_un servaddr,cliaddr;
   char c;
   int connfd=-1,fd;

   int listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);
   fcntl(listenfd, F_SETFL, O_NONBLOCK);

   unlink(UNIXSTR_PATH);
   memset(&servaddr, 0, sizeof(servaddr));
   servaddr.sun_family = AF_LOCAL;
   strcpy(servaddr.sun_path, UNIXSTR_PATH);
   bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
   listen(listenfd, 1);

   // wait for connections
   socklen_t clilen = sizeof(cliaddr);

   // wait max 5s (5000 * 1000/1000000) for the client
   for(int i=0;i<5000 || connfd > 0;i++) {
      connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
      if(connfd > 0)
	 break;
      usleep(1000);
      RGFlushInterface();
   }
   // read_fd 
   read_fd(connfd, &c,1,&fd);

   close(connfd);
   close(listenfd);

   return fd;
}


void RGDebInstallProgress::conffile(gchar *conffile, gchar *status)
{
   string primary, secondary;
   gchar *m,*s,*p;
   GtkWidget *w;
   RGGtkBuilderUserDialog dia(this, "conffile");
   GtkBuilder *dia_builder = dia.getGtkBuilder();

   p = g_strdup_printf(_("Replace configuration file\n'%s'?"),conffile);
   s = g_strdup_printf(_("The configuration file %s was modified (by "
			 "you or by a script). An updated version is shipped "
			 "in this package. If you want to keep your current "
			 "version say 'Keep'. Do you want to replace the "
			 "current file and install the new package "
			 "maintainers version? "),conffile);

   // setup dialog
   w = GTK_WIDGET(gtk_builder_get_object(dia_builder, "label_message"));
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
   for(;status[i] != '\'' || status[i] == 0;i++) 
      /*nothing*/
      ;
   i++;
   for(;status[i] != '\'' || status[i] == 0;i++) 
      orig_file.append(1,status[i]);
   i++;

   // same for second ' and read until the end
   for(;status[i] != '\'' || status[i] == 0;i++) 
      /*nothing*/
      ;
   i++;
   for(;status[i] != '\'' || status[i] == 0;i++) 
      new_file.append(1,status[i]);
   i++;
   //cout << "got:" << orig_file << new_file << endl;

   // read diff
   string diff;
   char buf[512];
   char *cmd = g_strdup_printf("/usr/bin/diff -u %s %s", orig_file.c_str(), new_file.c_str());
   FILE *f = popen(cmd,"r");
   while(fgets(buf,sizeof(buf),f) != NULL) {
      diff += utf8(buf);
   }
   pclose(f);
   g_free(cmd);

   // set into buffer
   GtkWidget *text_view = GTK_WIDGET(gtk_builder_get_object(dia_builder, "textview_diff"));
   GtkStyleContext *styleContext = gtk_widget_get_style_context(text_view);
   gtk_css_provider_load_from_data(_cssProvider, "GtkTextView { font-family: monospace; }", -1, NULL);
   gtk_style_context_add_provider(styleContext, GTK_STYLE_PROVIDER(_cssProvider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
   GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
   gtk_text_buffer_set_text(text_buffer,diff.c_str(),-1);

   int res = dia.run(NULL,true);
   if(res ==  GTK_RESPONSE_YES)
      vte_terminal_feed_child(VTE_TERMINAL(_term), "y\n",2);
   else
      vte_terminal_feed_child(VTE_TERMINAL(_term), "n\n",2);

   // update the "action" clock
   last_term_action = time(NULL);
}

void RGDebInstallProgress::startUpdate()
{
   child_has_exited=false;

   // check if we run embedded
   int id = _config->FindI("Volatile::PlugProgressInto", -1);
   if (id > 0) {
      g_error("gtk_plugin_new not supported with gtk3");
   } else {
      show();
   }
   RGFlushInterface();
}

void RGDebInstallProgress::cbCancel(GtkWidget *self, void *data)
{
   //FIXME: we can't activate this yet, it's way to heavy (sending KILL)
   //cout << "cbCancel: sending SIGKILL to child" << endl;
   RGDebInstallProgress *me = (RGDebInstallProgress*)data;
   //kill(me->_child_id, SIGINT);
   //kill(me->_child_id, SIGQUIT);
   kill(me->_child_id, SIGTERM);
   //kill(me->_child_id, SIGKILL);
   
}


void RGDebInstallProgress::cbClose(GtkWidget *self, void *data)
{
   //cout << "cbCancel: sending SIGKILL to child" << endl;
   RGDebInstallProgress *me = (RGDebInstallProgress*)data;
   me->_updateFinished = true;
}

bool RGDebInstallProgress::close()
{
   if(child_has_exited)
      cbClose(NULL, this);

   return TRUE;
}

RGDebInstallProgress::~RGDebInstallProgress()
{
   delete _userDialog;
   g_object_unref(_cssProvider);
}

RGDebInstallProgress::RGDebInstallProgress(RGMainWindow *main,
					   RPackageLister *lister)

   : RInstallProgress(), RGGtkBuilderWindow(main, "rgdebinstall_progress"),
     _totalActions(0), _progress(0), _sock(0), _userDialog(0)

{
   // timeout in sec until the expander is expanded 
   // (bigger nowdays because of the gconf stuff)
   _terminalTimeout=_config->FindI("Synaptic::TerminalTimeout",120);

   prepare(lister);
   setTitle(_("Applying Changes"));

   // make sure we try to get a graphical debconf
   setenv("DEBIAN_FRONTEND", "gnome", FALSE);
   setenv("APT_LISTCHANGES_FRONTEND", "gtk", FALSE);

   _startCounting = false;
   _label_status = GTK_WIDGET(gtk_builder_get_object(_builder, "label_status"));
   _pbarTotal = GTK_WIDGET(gtk_builder_get_object(_builder, "progress_total"));
   gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(_pbarTotal), 0.025);
   _autoClose = GTK_WIDGET(gtk_builder_get_object(_builder, "checkbutton_auto_close"));
   // we don't save options in non-interactive mode, so there is no 
   // point in showing this here
   if(_config->FindB("Volatile::Non-Interactive", false))
      gtk_widget_hide(_autoClose);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_autoClose), 
				_config->FindB("Synaptic::closeZvt", false));
   //_image = GTK_WIDGET(gtk_builder_get_object(_builder, "image"));

   // work around for kdesudo blocking our SIGCHLD (LP: #156041) 
   sigset_t sset;
   sigemptyset(&sset);
   sigaddset(&sset, SIGCHLD);
   sigprocmask(SIG_UNBLOCK, &sset, NULL);

   _term = vte_terminal_new();
   vte_terminal_set_size(VTE_TERMINAL(_term),80,23);
   GtkWidget *scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL,
                                             gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(VTE_TERMINAL(_term))));
   gtk_widget_set_can_focus (scrollbar, FALSE);
   vte_terminal_set_scrollback_lines(VTE_TERMINAL(_term), 10000);

   const char *s;
   if(_config->FindB("Synaptic::useUserTerminalFont")) {
      s = _config->Find("Synaptic::TerminalFontName").c_str();
   } else {
      s = "monospace 8";
   }
   PangoFontDescription *fontdesc = pango_font_description_from_string(s);
   vte_terminal_set_font(VTE_TERMINAL(_term), fontdesc);
   pango_font_description_free(fontdesc);

   gtk_box_pack_start(GTK_BOX(GTK_WIDGET(gtk_builder_get_object(_builder,"hbox_vte"))), _term, TRUE, TRUE, 0);
   g_signal_connect(G_OBJECT(_term), "key-press-event", G_CALLBACK(key_press_event), this);
   g_signal_connect(G_OBJECT(_term), "button-press-event",
           (GCallback) cbTerminalClicked, this);

   gtk_widget_show(_term);

   gtk_box_pack_end(GTK_BOX(GTK_WIDGET(gtk_builder_get_object(_builder,"hbox_vte"))), scrollbar, FALSE, FALSE, 0);

   // Terminal contextual menu
   GtkWidget *img, *menuitem;
   _popupMenu = gtk_menu_new();
   menuitem = gtk_menu_item_new_with_label(_("Copy"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbMenuitemClicked, (void *)EDIT_COPY);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);
   gtk_widget_show(menuitem);

   menuitem = gtk_menu_item_new_with_label(_("Select All"));
   g_object_set_data(G_OBJECT(menuitem), "me", this);
   g_signal_connect(menuitem, "activate",
                    (GCallback) cbMenuitemClicked, (void *)EDIT_SELECT_ALL);
   gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), menuitem);
   gtk_widget_show(menuitem);

   gtk_widget_show(scrollbar);

   gtk_window_set_default_size(GTK_WINDOW(_win), 500, -1);

   g_signal_connect(_term, "contents-changed",
		    G_CALLBACK(content_changed), this);

   g_signal_connect(gtk_builder_get_object(_builder, "button_cancel"),
                    "clicked",
                    G_CALLBACK(cbCancel), this);
   g_signal_connect(gtk_builder_get_object(_builder, "button_close"),
                    "clicked",
                    G_CALLBACK(cbClose), this);
   g_signal_connect(VTE_TERMINAL(_term), "child-exited",
		    G_CALLBACK(child_exited),
		    this);

   if(_userDialog == NULL)
      _userDialog = new RGUserDialog(this);

   
   gtk_window_set_urgency_hint(GTK_WINDOW(_win), FALSE);

   // init the timer
   last_term_action = time(NULL);

   _cssProvider = gtk_css_provider_new();
}

void RGDebInstallProgress::content_changed(GObject *object, 
					   gpointer data)
{
   //cout << "RGDebInstallProgress::content_changed()" << endl;

   RGDebInstallProgress *me = (RGDebInstallProgress*)data;

   me->last_term_action = time(NULL);
}

gboolean RGDebInstallProgress::key_press_event(GtkWidget *widget, 
                                               GdkEventKey *event, 
                                               gpointer user_data)
{
   RGDebInstallProgress *me = (RGDebInstallProgress *)user_data;

   // user pressed ctrl-c
   if (event->keyval == GDK_c && event->state & GDK_CONTROL_MASK) {
      gchar *summary = _("Ctrl-c pressed");
      char *msg = _("This will abort the operation and may leave the system "
                    "in a broken state. Are you sure you want to do that?");
      GtkWidget *dia = gtk_message_dialog_new (GTK_WINDOW(me->_win),
					       GTK_DIALOG_DESTROY_WITH_PARENT,
					       GTK_MESSAGE_WARNING,
					       GTK_BUTTONS_YES_NO,
					       "%s", summary);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dia), "%s", msg);
      int res = gtk_dialog_run (GTK_DIALOG (dia));
      gtk_widget_destroy (dia);
      switch(res) {
	 case GTK_RESPONSE_YES:
	    return false;
	 case GTK_RESPONSE_NO:
	    return true;
      }
   } else if (event->keyval == GDK_C && 
           event->state & (GDK_CONTROL_MASK|GDK_SHIFT_MASK)) {
      // ctrl+shift+C copy to clipboard to mimic gnome-terminal behavior
      me->terminalAction(me->_term, EDIT_COPY);
	  return true;
   } else if (event->keyval == GDK_a && event->state & GDK_CONTROL_MASK) {
      me->terminalAction(me->_term, EDIT_SELECT_ALL);
	  return true;
   } else if (event->keyval == GDK_A &&
           event->state & (GDK_CONTROL_MASK|GDK_SHIFT_MASK)) {
      me->terminalAction(me->_term, EDIT_SELECT_NONE);
	  return true;
   } 
   return false;
}

gboolean RGDebInstallProgress::cbTerminalClicked(GtkWidget *widget,
        GdkEventButton *event, gpointer user_data)
{
   if (event->button == 3) {
      RGDebInstallProgress *me = (RGDebInstallProgress *)user_data;
      gtk_menu_popup(GTK_MENU(me->_popupMenu), NULL, NULL, NULL, NULL,
              event->button,
              gdk_event_get_time((GdkEvent *) event));
      return true;
   }
   return false;
}

void RGDebInstallProgress::cbMenuitemClicked(GtkMenuItem *menuitem,
        gpointer user_data) {
   RGDebInstallProgress *me = 
       (RGDebInstallProgress *)g_object_get_data(G_OBJECT(menuitem), "me");
   me->terminalAction(me->_term, (TermAction)GPOINTER_TO_INT(user_data));
}

void RGDebInstallProgress::terminalAction(GtkWidget *terminal,
        TermAction action) {
   switch(action) {
      case EDIT_COPY:
          vte_terminal_copy_clipboard(VTE_TERMINAL(terminal));
          break;
      case EDIT_SELECT_ALL:
          vte_terminal_select_all(VTE_TERMINAL(terminal));
          break;
      case EDIT_SELECT_NONE:
          vte_terminal_unselect_all(VTE_TERMINAL(terminal));
          break;
   }
}

void RGDebInstallProgress::updateInterface()
{
   char buf[2];
   static char line[1024] = "";
   int i=0;

   while (1) {

      // This algorithm should be improved (it's the same as the rpm one ;)
      int len = read(_childin, buf, 1);

      // nothing was read
      if(len < 1) 
	 break;

      // update the time we last saw some action
      last_term_action = time(NULL);

      if( buf[0] == '\n') {
	 //cout << "got line: " << line << endl;
	 
	 gchar **split = g_strsplit(line, ":",4);
	 gchar *status = g_strstrip(split[0]);
	 gchar *pkg = g_strstrip(split[1]);
	 gchar *percent = g_strstrip(split[2]);
	 gchar *str = g_strdup(g_strstrip(split[3]));

	 // major problem here, we got unexpected input. should _never_ happen
	 if(!(pkg && status))
	    continue;

	 // first check for errors and conf-file prompts
	 if(strstr(status, "pmerror") != NULL) { 
	    // error from dpkg, needs to be parsed different
	    str = g_strdup_printf(_("Error in package %s"), split[1]);
	    gtk_label_set_text(GTK_LABEL(_label_status), str);
	    string err = split[1] + string(": ") + split[3];
	    _error->Error("%s",utf8(err.c_str()));
	 // first check for errors and conf-file prompts
	 } else if(strstr(status, "pmrecover") != NULL) { 
	    // running dpkg --configure -a
	    str = g_strdup(_("Trying to recover from package failure"));
	    gtk_label_set_text(GTK_LABEL(_label_status), str);
	 } else if(strstr(status, "pmconffile") != NULL) {
	    // conffile-request from dpkg, needs to be parsed different
	    //cout << split[2] << " " << split[3] << endl;
	    conffile(pkg, split[3]);
	 } else {
	    _startCounting = true;
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbarTotal), 0);
	 }

	 // reset the urgency hint, something changed on the terminal
	 if(gtk_window_get_urgency_hint(GTK_WINDOW(_win)))
	    gtk_window_set_urgency_hint(GTK_WINDOW(_win), FALSE);

	 float val = atof(percent)/100.0;
 	 //cout << "progress: " << val << endl;
         if (fabs(val-gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(_pbarTotal))) > 0.1)
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbarTotal), val);

	 if(str!=NULL)
	    gtk_label_set_text(GTK_LABEL(_label_status),utf8(str));
	 
	 // clean-up
	 g_strfreev(split);
	 g_free(str);
	 line[0] = 0;
      } else {
	 buf[1] = 0;
	 strcat(line, buf);
      }      
   }

   time_t now = time(NULL);

   if(!_startCounting) {
      usleep(100000);
      gtk_progress_bar_pulse (GTK_PROGRESS_BAR(_pbarTotal));
      // wait until we get the first message from apt
      last_term_action = now;
   }


   if ((now - last_term_action) > _terminalTimeout) {
      // get some debug info
      const gchar *s = gtk_label_get_text(GTK_LABEL(_label_status));
      g_warning("no statusfd changes/content updates in terminal for %i" 
		" seconds",_terminalTimeout);
      g_warning("TerminalTimeout in step: %s", s);
      // now expand the terminal
      GtkWidget *w;
      w = GTK_WIDGET(gtk_builder_get_object(_builder, "expander_terminal"));
      gtk_expander_set_expanded(GTK_EXPANDER(w), TRUE);
      last_term_action = time(NULL);
      // try to get the attention of the user
      gtk_window_set_urgency_hint(GTK_WINDOW(_win), TRUE);
   } 


   if (gtk_events_pending()) {
      while (gtk_events_pending())
         gtk_main_iteration();
   } else {
      // 25fps
      usleep(1000000/25);
   }
}

pkgPackageManager::OrderResult RGDebInstallProgress::start(pkgPackageManager *pm,
                                                       int numPackages,
                                                       int numPackagesTotal)
{
   void *dummy;
   pkgPackageManager::OrderResult res;
   int ret;

   res = pm->DoInstallPreFork();
   if (res == pkgPackageManager::Failed)
       return res;
   
   int master;
   _child_id = forkpty(&master, NULL, NULL, NULL);
   if(_child_id < 0) {
      cerr << "vte_terminal_forkpty() failed. " << strerror(errno) << endl;
      res = pkgPackageManager::Failed;
      GtkWidget *dialog;
      dialog = gtk_message_dialog_new (GTK_WINDOW(window()),
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE,
                                       NULL);
      gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), 
                                     _("Error failed to fork pty"));
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
      return res;
   } else if (_child_id == 0) {
      int fd[2];
      pipe(fd);
      ipc_send_fd(fd[0]); // send the read part of the pipe to the parent

      APT::Progress::PackageManagerProgressFd progress(fd[1]);
      res = pm->DoInstallPostFork(&progress);

      // dump errors into cerr (pass it to the parent process)	
      _error->DumpErrors();

      // HACK: try to correct the situation
      if(res == pkgPackageManager::Failed) {
	 cerr << _("A package failed to install.  Trying to recover:") << endl;
	 const char *rstatus = "pmrecover:dpkg:0:Trying to recover\n";
	 write(fd[1], rstatus, strlen(rstatus));
	 system("dpkg --configure -a");
      }

      ::close(fd[0]);
      ::close(fd[1]);

      _exit(res);
   }
   // parent: assign pty to the vte terminal
   GError *err = NULL;
   VtePty *pty = vte_pty_new_foreign_sync(master, NULL, &err);
   if (err != NULL) {
      std::cerr << "failed to create new pty: " << err->message << std::endl;
      g_error_free (err);
      return pkgPackageManager::Failed;
   }
   vte_terminal_set_pty(VTE_TERMINAL(_term), pty);
   // FIXME: is there a race here? i.e. what if the child is dead before
   //        we can set it?
   vte_terminal_watch_child(VTE_TERMINAL(_term), _child_id);

   _childin = ipc_recv_fd();
   if(_childin < 0) {
      // something _bad_ happend. so the terminal window and hope for the best
      GtkWidget *w = GTK_WIDGET(gtk_builder_get_object(_builder, "expander_terminal"));
      gtk_expander_set_expanded(GTK_EXPANDER(w), TRUE);
      gtk_widget_hide(_pbarTotal);
   }

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
   ::close(master);

   _config->Clear("APT::Keep-Fds", _childin);

   return res;
}



void RGDebInstallProgress::finishUpdate()
{
   if (_startCounting) {
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_pbarTotal), 1.0);
   }
   RGFlushInterface();

   GtkWidget *_closeB = GTK_WIDGET(gtk_builder_get_object(_builder, "button_close"));
   gtk_widget_set_sensitive(_closeB, TRUE);

   bool autoClose= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_autoClose));
   if(res == 0) {
      gtk_widget_grab_focus(_closeB);
      if(autoClose)
	 _updateFinished = true;
   }

   string s = _config->Find("Volatile::InstallFinishedStr",
			    _("Changes applied"));
   gchar *msg = g_strdup_printf("<big><b>%s</b></big>\n%s", s.c_str(),
				_(getResultStr(res)));
   setTitle(_("Changes applied"));
   GtkWidget *l = GTK_WIDGET(gtk_builder_get_object(_builder, "label_action"));
   gtk_label_set_markup(GTK_LABEL(l), msg);
   g_free(msg);

   // hide progress and label
   gtk_widget_hide(_pbarTotal);
   gtk_widget_hide(_label_status);

   GtkWidget *img = GTK_WIDGET(gtk_builder_get_object(_builder, "image_finished"));
   switch(res) {
   case 0: // success
      gtk_image_set_from_icon_name(GTK_IMAGE(img),
			      "synaptic", GTK_ICON_SIZE_DIALOG);
      break;
   case 1: // error
      gtk_image_set_from_icon_name(GTK_IMAGE(img), "dialog-error",
			       GTK_ICON_SIZE_DIALOG);
      _userDialog->showErrors();
      break;
   case 2: // incomplete
      gtk_image_set_from_icon_name(GTK_IMAGE(img), "dialog-information",
			       GTK_ICON_SIZE_DIALOG);
      break;
   }
   gtk_widget_show(img);
   
   // wait for user action
   while(true) {
      // events
      while (gtk_events_pending())
	 gtk_main_iteration();

      // user clicked "close" button
      if(_updateFinished) 
	 break;

      // user has autoClose set *and* there was no error
      autoClose= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_autoClose));
      if(autoClose && res != 1)
	 break;
      
      // wait a bit
      g_usleep(100000);
   }
      
   // set the value again, it may have changed
   _config->Set("Synaptic::closeZvt", autoClose	? "true" : "false");

   // hide and finish
   if(_sock != NULL) {
      gtk_widget_destroy(_sock);
   } else {
      hide();
   }
}

void RGDebInstallProgress::prepare(RPackageLister *lister)
{
   //cout << "prepeare called" << endl;

   // build a meaningfull dialog
   int installed, broken, toInstall, toRemove;
   double sizeChange;
   const gchar *p = "Should never be displayed, please report";
   string s = _config->Find("Volatile::InstallProgressStr",
			    _("The marked changes are now being applied. "
			      "This can take some time. Please wait."));
   lister->getStats(installed, broken, toInstall, toRemove, sizeChange);
   if(toRemove > 0 && toInstall > 0) 
      p = _("Installing and removing software");
   else if(toRemove > 0)
      p = _("Removing software");
   else if(toInstall > 0)
      p =  _("Installing software");

   gchar *msg = g_strdup_printf("<big><b>%s</b></big>\n\n%s", p, s.c_str());
   GtkWidget *l = GTK_WIDGET(gtk_builder_get_object(_builder, "label_action"));
   gtk_label_set_markup(GTK_LABEL(l), msg);
   g_free(msg);

}

#endif

