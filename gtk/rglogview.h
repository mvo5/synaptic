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

#include "rggtkbuilderwindow.h"


class RGLogView : public RGGtkBuilderWindow {
 protected:
   static void cbCloseClicked(GtkWidget *self, void *data);
   static void cbButtonFind(GtkWidget *self, void *data);
   static void cbTreeSelectionChanged(GtkTreeSelection *selection, 
				      gpointer data);
   static gboolean filter_func(GtkTreeModel *model, GtkTreeIter *iter,
			       gpointer data);

   // some widgets
   GtkWidget *_treeView;
   GtkWidget *_entryFind;
   GtkWidget *_textView;
   GtkTextTag *_markTag;

   // data
   const gchar *findStr;
   GtkTreeModel *_realModel;

   // set new logbuffer text
   void clearLogBuf();
   void appendLogBuf(string text);

 public:
   RGLogView(RGWindow *parent);

   virtual void show();
   void readLogs();

   virtual ~RGLogView() {};
};


#endif
