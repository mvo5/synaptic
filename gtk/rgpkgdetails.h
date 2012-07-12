/* rgpkgdetails.h - show details of a pkg
 * 
 * Copyright (c) 2004 Michael Vogt
 *
 * Author: Michael Vogt <mvo@debian.org>
 *         Gustavo Niemeyer <niemeyer@conectiva.com>
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

#ifndef _RGPKGDETAILS_H
#define _RGPKGDETAILS_H

#include <gtk/gtk.h>
#include "rpackage.h"
#include "rggtkbuilderwindow.h"

class RGPkgDetailsWindow : public RGGtkBuilderWindow {
   
 protected:
   // used for the screenshot parameter passing
   struct screenshot_info {
      GtkTextChildAnchor *anchor;
      GtkWidget *textview;
      RPackage *pkg;
      bool thumb;
   };

   static vector<string> formatDepInformation(vector<DepInformation> deps);
   static void cbDependsMenuChanged(GtkWidget *self, void *data);
   static void cbCloseClicked(GtkWidget *self, void *data);
   static void cbShowScreenshot(GtkWidget *button, void *data);
   static void cbShowBigScreenshot(GtkWidget *button, GdkEventButton *event, void *data);
   static void cbShowChangelog(GtkWidget *button, void *data);
   static gboolean cbOpenHomepage(GtkWidget *button, void *data);
   static gboolean cbOpenLink(GtkWidget *button, gchar *uri, void *data);
   static void doShowBigScreenshot(RPackage *pkg);

 public:
   RGPkgDetailsWindow(RGWindow *parent);
   static void fillInValues(RGGtkBuilderWindow *me, RPackage *pkg, 
			    bool setTitle=false);
   ~RGPkgDetailsWindow();
};
#endif
