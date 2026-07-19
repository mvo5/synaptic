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

#pragma once

#include "config.h" // IWYU pragma: associated

#ifdef HAVE_TERMINAL

#   include "rggtkbuilderwindow.h"
#   include "rinstallprogress.h"

#   include <apt-pkg/packagemanager.h>
#   include <gtk/gtk.h>
#   include <sys/types.h>
#   include <vte/vte.h>

class RGMainWindow;

class RGTermInstallProgress : public RInstallProgress, public RGGtkBuilderWindow
{
   GtkWidget *_term;
   GtkWidget *_scrollbar;
   GtkWidget *_statusL;
   GtkWidget *_closeB;
   GtkWidget *_closeOnF;

   pkgPackageManager::OrderResult res;

 protected:
   bool child_has_exited;
   static void child_exited(VteTerminal *vteterminal, gint ret, gpointer data);
   virtual void startUpdate();
   virtual void updateInterface();
   virtual void finishUpdate();
   static void stopShell(GtkWidget *self, void *data);
   virtual void close() override;

   pid_t _child_id;

 public:
   RGTermInstallProgress(RGMainWindow *main);
   ~RGTermInstallProgress() {};

   virtual pkgPackageManager::OrderResult start(pkgPackageManager *pm,
                                                int numPackages = 0,
                                                int totalPackages = 0);
};

#endif /* HAVT_TERMINAL */
