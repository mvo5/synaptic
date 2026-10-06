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

#pragma once

#include "config.h" // IWYU pragma: associated

#ifdef HAVE_APTPKG_CDROM

#   include "rgwindow.h"
#   include "coroutines.h"

#   include <apt-pkg/cdrom.h>
#   include <gtk/gtk.h>
#   include <string>

class RGMainWindow;
class RUserDialog;

class RGCDScanner : public pkgCdromStatus, public RGWindow
{
 protected:
   GtkWidget *_label;
   GtkWidget *_pbar;

   RUserDialog *_userDialog;

 public:
   RGCDScanner(RGMainWindow *main, RUserDialog *userDialog);

   bool ChangeCdrom();
   bool AskCdromName(std::string &defaultName);
   void Update(std::string text, int current);

   [[nodiscard]] task<bool> run();
};

#endif
