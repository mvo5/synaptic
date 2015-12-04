/* rgzvtinstallprogress.h
 *
 * Copyright (c) 2002 Michael Vogt
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


#ifndef _RGTERMNSTALLPROGRESS_H_
#define _RGTERMINSTALLPROGRESS_H_

#ifdef HAVE_TERMINAL

#include "rgmainwindow.h"
#include "rinstallprogress.h"
#include "rgwindow.h"

#include <vte/vte.h>

class RGTermInstallProgress : public RInstallProgress, public RGGtkBuilderWindow {
  GtkWidget *_term;
  GtkWidget *_scrollbar;
  GtkWidget *_statusL;
  GtkWidget *_closeB;
  GtkWidget *_closeOnF;
  GtkWidget *_sock;

  pkgPackageManager::OrderResult res;
  static gboolean zvtFocus (GtkWidget *widget, GdkEventButton *event, gpointer user_data);

protected:
  bool child_has_exited;
  static void child_exited(VteTerminal *vteterminal, gint ret,
			   gpointer data);
  virtual void startUpdate();
  virtual void updateInterface();
  virtual void finishUpdate();
  static void stopShell(GtkWidget *self, void* data);
  virtual bool close();

  pid_t _child_id;

public:
   RGTermInstallProgress(RGMainWindow *main);
   ~RGTermInstallProgress() {};

   virtual pkgPackageManager::OrderResult start(pkgPackageManager *pm,
		   				int numPackages = 0,
						int totalPackages = 0);

};

#endif /* HAVT_TERMINAL */

#endif
