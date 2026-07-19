/* rgcacheprogress.cc
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

#include "config.h" // IWYU pragma: associated

#include "rgcacheprogress.h"

#include "rgutils.h"

#include <cmath>
#include <gtk/gtk.h>
#include <string>

RGCacheProgress::RGCacheProgress(GtkWidget *parent, GtkWidget *label)
   : _parent(parent), _label(label)
{
   //     _prog = gtk_progress_bar_new();

   //     gtk_label_set_text(GTK_LABEL(_label), utf8(Op.c_str()));

   //     gtk_box_pack_start(GTK_BOX(_parent), _prog, FALSE, TRUE, 0);

   _prog = parent;
   gtk_label_set_text(GTK_LABEL(_label), utf8(Op.c_str()));

   _mapped = false;
}

RGCacheProgress::~RGCacheProgress()
{
   // gtk_widget_destroy(_prog);
}

void RGCacheProgress::Update()
{
   if (!CheckChange()) {
      // co_await RGFlushInterface();
      return;
   }

   start_task([this]() -> task<void> {
      if (!_mapped) {
         gtk_widget_show(_prog);
         co_await RGFlushInterface();
         _mapped = true;
      }

      if (MajorChange)
         gtk_label_set_text(GTK_LABEL(_label), utf8(Op.c_str()));

      // only call set_fraction when the changes are noticable (0.1%)
      if (fabs(Percent -
               gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(_prog)) * 100.0) >
          0.1)
         gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_prog),
                                       (float)Percent / 100.0);

      co_await RGFlushInterface();
   });
}

void RGCacheProgress::Done()
{
   start_task([this]() -> task<void> {
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_prog), 1.0);
      co_await RGFlushInterface();

      gtk_widget_hide(_prog);

      _mapped = false;
   });
}
