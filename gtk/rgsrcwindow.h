/* $Id: rgsrcwindow.h,v 1.6 2002/11/10 14:03:32 mvogt Exp $ */
#ifndef _rgsrcwindow_H
#define _rgsrcwindow_H

#include "rsources.h"
#include "rgwindow.h"
#include "rgvendorswindow.h"
#include "rguserdialog.h"

typedef list<SourcesList::SourceRecord*>::iterator SourcesListIter;
typedef list<SourcesList::VendorRecord*>::iterator VendorsListIter;

class RGSrcEditor : RGWindow {
	SourcesList lst;

	GtkWidget *dialog;
	GtkWidget *lstSources;
	//GtkWidget *entryVendor;
	GtkWidget *optVendor;
	GtkWidget *optVendorMenu;
	GtkWidget *entryURI;
	GtkWidget *entrySect;
	GtkWidget *optType;
	GtkWidget *optTypeMenu;
	GtkWidget *entryDist;
	GtkWidget *cbDisabled;

	int selectedrow;

	RGUserDialog *_userDialog;

	bool Applied;

	GtkWidget *CreateWidget();

	void UpdateVendorMenu();
	int VendorMenuIndex(string VendorID);

	// static event handlers
	static void DoClear(GtkWidget *, gpointer);
	static void DoAdd(GtkWidget *, gpointer);
	static void DoEdit(GtkWidget *, gpointer);
	static void DoRemove(GtkWidget *, gpointer);
	static void DoOK(GtkWidget *, gpointer);
	static void DoCancel(GtkWidget *, gpointer);
	static void UpdateDisplay(GtkCList *, gint, gint, GdkEventButton *, gpointer);
	static void UnselectRow(GtkCList *, gint, gint, GdkEventButton *, gpointer);
	static void VendorsWindow(GtkWidget *, gpointer);

public:
	RGSrcEditor(RGWindow *parent);
	~RGSrcEditor();

	bool Run();
};

#endif
