/* rgpkgcdrom.h - make use of the new apt-pkg/cdrom.h interface
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2004 Canonical
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <michael.vogt@ubuntu.com>
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

#ifndef RGPKGCDROM_H
#define RGPKGCDROM_H

#include <config.h>
#ifdef HAVE_APTPKG_CDROM

#include "rggtkbuilderwindow.h"
#include <apt-pkg/cdrom.h>

class RGMainWindow;

class RGCDScanner:public pkgCdromStatus, public RGWindow {
 protected:

   GtkWidget *_label;
   GtkWidget *_pbar;

   RUserDialog *_userDialog;

 public:
   RGCDScanner(RGMainWindow *main, RUserDialog *userDialog);

   bool ChangeCdrom();
   bool AskCdromName(string &defaultName);
   void Update(string text, int current);

   bool run();

};

#endif
#endif
