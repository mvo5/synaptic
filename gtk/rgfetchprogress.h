/* rgfetchprogress.h
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

#include "rpackagelister.h"
#include "rggtkbuilderwindow.h"
#include "coroutines.h"
#include "racquireasync.h"

#include <apt-pkg/acquire.h>
#include <gtk/gtk.h>
#include <set>
#include <string>
#include <vector>

class RGWindow;

class RPkgAcquireStatusImpl : public pkgAcquireStatus
{
   friend class RGFetchProgress;

 protected:
   virtual bool MediaChange(std::string Media, std::string Drive) override
   {
      return true;
   }
};

class RGFetchProgress : public RPkgAcquireStatusAsync, public RGGtkBuilderWindow
{
   RPkgAcquireStatusImpl _status;

   struct Item
   {
      std::string descr;
      std::string uri;
      std::string size;
      int status;
   };

   std::vector<Item> _items;

   GtkWidget *_table;
   GtkListStore *_tableListStore;
   std::set<int> _tableRows;

   GtkWidget *_mainProgressBar; // GtkProgressBar

   GtkTreeViewColumn *_statusColumn;
   GtkCellRenderer *_statusRenderer;
   bool _cancelled;

   void updateStatus(pkgAcquire::ItemDesc &Itm, int status);
   static void stopDownload(GtkWidget *self, void *data);

   static void cursorChanged(GtkTreeView *treeview, gpointer user_data);
   static void expanderActivate(GObject *object,
                                GParamSpec *param_spec,
                                gpointer user_data);
   bool _cursorDirty;

   char *getStatusStr(int status);
   int getStatusPercent(int status);
   void refreshTable(int row, bool append = false);
   // GdkPixmap *statusDraw(int width, int height, int status);

 public:
   [[nodiscard]] virtual task<bool> MediaChange(std::string Media,
                                                std::string Drive) override;
   [[nodiscard]] virtual task<void> IMSHit(pkgAcquire::ItemDesc &Itm) override;
   [[nodiscard]] virtual task<void> Fetch(pkgAcquire::ItemDesc &Itm) override;
   [[nodiscard]] virtual task<void> Done(pkgAcquire::ItemDesc &Itm) override;
   [[nodiscard]] virtual task<void> Fail(pkgAcquire::ItemDesc &Itm) override;
   [[nodiscard]] virtual task<void> Start() override;
   [[nodiscard]] virtual task<void> Stop() override;

   virtual void close() override;

   bool Pulse(pkgAcquire *Owner);

   // set description of the current task (main and additonal explaination)
   void setDescription(std::string mainText, std::string secondText = "");


   RGFetchProgress(RGWindow *win);
};
