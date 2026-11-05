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

#pragma once

#include "config.h" // IWYU pragma: associated

#include "rggtkbuilderwindow.h"
#include "rinstallprogress.h"

#include <gtk/gtk.h>
#include <map>
#include <string>

class RGMainWindow;
class RGSlideShow;
class RGWindow;
class RPackageLister;

class RGInstallProgressMsgs final : public RGGtkBuilderWindow
{
   GtkTextBuffer *const _textBuffer;
   GtkCssProvider *const _cssProvider;

   const char *_currentPackage;
   bool _hasHeader;

   static void onCloseClicked(GtkWidget *self, void *data);
   static GtkTextBuffer *createTextBuffer(GtkBuilder *builder);

 protected:

   void addText(const char *text, bool bold = false);

 public:

   inline void newPackage(const char *name)
   {
      _currentPackage = name;
      _hasHeader = false;
   }

   void addLine(const char *line);

   inline bool empty()
   {
      return gtk_text_buffer_get_char_count(_textBuffer) == 0;
   }

   void run();

   inline void close() override
   {
      gtk_main_quit();
   }

   RGInstallProgressMsgs(RGWindow *win);

   ~RGInstallProgressMsgs() override
   {
      g_object_unref(_cssProvider);
   }
};

class RGInstallProgress final : public RInstallProgress, public RGGtkBuilderWindow
{
   GtkWidget *const _label;
   GtkWidget *const _labelSummary;

   GtkWidget *const _pbar;
   GtkWidget *const _pbarTotal;

   GtkCssProvider *const _cssProvider;
   GtkCssProvider *const _cssProviderBold;

   bool _startCounting;

   std::map<std::string, std::string> _summaryMap;

   RGInstallProgressMsgs _msgs;

 protected:

   void prepare(RPackageLister *lister);

 public:

   RGInstallProgress(RGMainWindow *main, RPackageLister *lister);

   ~RGInstallProgress() override
   {
      g_object_unref(_cssProvider);
      g_object_unref(_cssProviderBold);
   }

   void startUpdate() override;
   void updateInterface() override;
   void finishUpdate() override;
};
