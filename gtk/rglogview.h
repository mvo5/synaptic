/* rglogview.h
 *
 * Copyright (c) 2004 Michael Vogt
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


#ifndef _RGLOGVIEW_H_
#define _RGLOGVIEW_H_

#include "rggladewindow.h"


class RGLogView : public RGGladeWindow {
 protected:
   static void cbCloseClicked(GtkWidget *self, void *data);
   static void cbTreeSelectionChanged(GtkTreeSelection *selection, 
				      gpointer data);

   GtkWidget *_treeView;
   GtkWidget *_textView;

 public:
   RGLogView(RGWindow *parent);

   void readLogs();

   virtual ~RGLogView() {};
};


#endif
