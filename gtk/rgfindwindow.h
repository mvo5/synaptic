/* rgfindwindow.h
 *
 * Copyright (c) 2003 Michael Vogt
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

#ifndef _GTK_RGFINDWINDOW_H
#define _GTK_RGFINDWINDOW_H

#include "config.h"  // IWYU pragma: associated

#include "i18n.h"
#include "rggtkbuilderwindow.h"

#include <cstddef>
#include <glib.h>
#include <gtk/gtk.h>

class RGFindWindow;
class RGWindow;

typedef void RGFindWindowFindAction(void *self, RGFindWindow * win);

static char *SearchTypes[] = {
   _("Name"),
   _("Description and Name"),
   _("Maintainer"),
   _("Version"),
   _("Dependencies"),           // depends, predepends etc
   _("Provided packages"),      // provides and name
   NULL
};

class RGFindWindow:public RGGtkBuilderWindow {

   //RPatternPackageFilter::DepType _searchType;
   GList *_prevSearches;
   GtkWidget *_comboFind, *_findB, *_comboSearchType;

   static void doFind(GtkWindow *widget, void *data);
   static void doClose(GtkWindow *widget, void *data);
   static void cbEntryChanged(GtkWindow *widget, void *data);

 public:
   RGFindWindow(RGWindow *owner);
   virtual ~RGFindWindow() {
   };

   int getSearchType();

   gchar* getFindString();
   void selectText();
};

#endif
