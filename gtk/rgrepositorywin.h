/* $Id: rgsrcwindow.h,v 1.6 2002/11/10 14:03:32 mvogt Exp $ */
#ifndef _rgsrcwindow_H
#define _rgsrcwindow_H

#include<gtk/gtk.h>
#include "rsources.h"
#include "rggladewindow.h"
#include "rgvendorswindow.h"
#include "rguserdialog.h"

typedef list<SourcesList::SourceRecord*>::iterator SourcesListIter;
typedef list<SourcesList::VendorRecord*>::iterator VendorsListIter;

class RGRepositoryEditor : RGGladeWindow {
    SourcesList _lst;
    int _selectedRow;

    // the gtktreeview
    GtkWidget *_sourcesListView;
    GtkListStore *_sourcesListStore;
    GtkTreeIter *_lastIter;

    GtkWidget *_optVendor;
    GtkWidget *_optVendorMenu;
    GtkWidget *_entryURI;
    GtkWidget *_entrySect;
    GtkWidget *_optType;
    GtkWidget *_optTypeMenu;
    GtkWidget *_entryDist;
    //GtkWidget *_cbEnabled;

    RGUserDialog *_userDialog;

    bool _applied;
    GdkColor _gray;

    void UpdateVendorMenu();
    int VendorMenuIndex(string VendorID);

    // static event handlers
    static void DoClear(GtkWidget *, gpointer);
    static void DoAdd(GtkWidget *, gpointer);
    static void DoRemove(GtkWidget *, gpointer);
    static void DoOK(GtkWidget *, gpointer);
    static void DoCancel(GtkWidget *, gpointer);
    static void VendorsWindow(GtkWidget *, gpointer);
    static void SelectionChanged(GtkTreeSelection *selection, gpointer data);

    // get values
    void doEdit();


 public:
    RGRepositoryEditor(RGWindow *parent);
    ~RGRepositoryEditor();
    
    bool Run();
};

#endif
