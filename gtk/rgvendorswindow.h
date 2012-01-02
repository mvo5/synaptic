/* $Id: rgvendorswindow.h,v 1.2 2002/11/10 14:03:32 mvogt Exp $ */
#ifndef _rgvendorswindow_H
#define _rgvendorswindow_H

#include "rsources.h"
#include "rgwindow.h"

class RGVendorsEditor:RGWindow {
   SourcesList & lst;

   GtkWidget *dialog;
   GtkWidget *lstVendors;
   GtkWidget *entryVendor;
   GtkWidget *entryDesc;
   GtkWidget *entryFPrint;

   int selectedrow;

   GtkWidget *CreateWidget();

   // static event handlers
   static void DoAdd(GtkWidget *, gpointer);
   static void DoEdit(GtkWidget *, gpointer);
   static void DoRemove(GtkWidget *, gpointer);
   static void DoOK(GtkWidget *, gpointer);
   static void DoCancel(GtkWidget *, gpointer);
#if 0 // PORTME TO GTk3
   static void UpdateDisplay(GtkCList *, gint, gint, GdkEventButton *,
                             gpointer);
   static void UnselectRow(GtkCList *, gint, gint, GdkEventButton *, gpointer);
#endif

 public:
   RGVendorsEditor(RGWindow *parent, SourcesList &lst);
   ~RGVendorsEditor();

   void Run();
};

#endif
