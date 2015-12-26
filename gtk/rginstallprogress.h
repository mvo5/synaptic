/* rginstallprogress.h
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


#ifndef _RGINSTALLPROGRESS_H_
#define _RGINSTALLPROGRESS_H_


#include "rinstallprogress.h"
#include "rggtkbuilderwindow.h"
#include "rgslideshow.h"

class RGMainWindow;

class RGInstallProgressMsgs:public RGGtkBuilderWindow {

   GtkTextBuffer *_textBuffer;
   static void onCloseClicked(GtkWidget *self, void *data);

   const char *_currentPackage;
   bool _hasHeader;

   GtkCssProvider *_cssProvider;

 protected:
   virtual void addText(const char *text, bool bold = false);

 public:
   virtual void newPackage(const char *name);
   virtual void addLine(const char *line);

   virtual bool empty();
   virtual void run();
   virtual bool close();

   RGInstallProgressMsgs(RGWindow *win);
   ~RGInstallProgressMsgs();
};

class RGInstallProgress:public RInstallProgress, public RGGtkBuilderWindow {

   GtkWidget *_label;
   GtkWidget *_labelSummary;

   GtkWidget *_pbar;
   GtkWidget *_pbarTotal;

   GtkWidget *_image;

   bool _startCounting;

   map<string, string> _summaryMap;

   RGInstallProgressMsgs _msgs;

   RGSlideShow *_ss;

   GtkCssProvider *_cssProvider;
   GtkCssProvider *_cssProviderBold;

 protected:
   virtual void startUpdate();
   virtual void updateInterface();
   virtual void finishUpdate();

   virtual void prepare(RPackageLister *lister);

 public:
   RGInstallProgress(RGMainWindow *main, RPackageLister *lister);
   ~RGInstallProgress();
};

#endif
