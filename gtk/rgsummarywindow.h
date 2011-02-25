/* rgsummarywindow.h
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2002 Michael Vogt <mvo@debian.org>
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <mvo@debian.org>
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


#ifndef _RGSUMMARYWINDOW_H_
#define _RGSUMMARYWINDOW_H_

#include "rggtkbuilderwindow.h"


class RPackageLister;


class RGSummaryWindow:public RGGtkBuilderWindow {
   GtkWidget *_topF;
   GtkWidget *_middleF;
   GtkWidget *_summaryL;
   GtkWidget *_summarySpaceL;
   bool _potentialBreak;
   RPackageLister *_lister;
   GtkWidget *_dlonlyB;
   GtkWidget *_checkSigsB;

   enum {
      PKG_COLUMN,
      N_COLUMNS
   };
   GtkWidget *_tree;
   GtkTreeStore *_treeStore;

   static void buildTree(RGSummaryWindow *me);
   static void buildLabel(RGSummaryWindow *me);
   static void clickedDetails(GtkWidget *w, void *data);

 public:
   RGSummaryWindow(RGWindow *win, RPackageLister *lister);

   bool showAndConfirm();
};

#endif
