/* rgcdscanner.h - copy of the apt-cdrom.cc stuff for apt's without a library 
 *                 interface
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
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

#ifndef RGCDSCANNER_H
#define RGCDSCANNER_H

#include <config.h>
#ifndef HAVE_APTPKG_CDROM

#include "rcdscanner.h"
#include "rggtkbuilderwindow.h"

class RGMainWindow;

class RGCDScanner:public RCDScanProgress, public RGWindow {
 protected:

   GtkWidget *_label;
   GtkWidget *_pbar;

   RUserDialog *_userDialog;

 public:

   RGCDScanner(RGMainWindow *main, RUserDialog *userDialog);
   void update(string text, int current);
   bool run();

};

#endif
#endif
