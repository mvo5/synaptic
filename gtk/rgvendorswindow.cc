/* $Id: rgvendorswindow.cc,v 1.4 2002/12/06 08:16:40 mvogt Exp $ */
#include <cassert>
#include "rgvendorswindow.h"
#include "rgrepositorywin.h"
#include "config.h"
#include "i18n.h"

RGVendorsEditor::RGVendorsEditor(RGWindow *parent, SourcesList &_lst)
: RGWindow(parent, "vendors"), lst(_lst)
{
   selectedrow = -1;
   dialog = CreateWidget();
}

RGVendorsEditor::~RGVendorsEditor()
{
   gtk_widget_destroy(dialog);
}

GtkWidget *RGVendorsEditor::CreateWidget()
{
#if 0 // PORTME
   GtkWidget *dlgSrcList;
   GtkWidget *dialog_vbox1;
   GtkWidget *frmMain;
   GtkWidget *vbox1;
   GtkWidget *scrolledwindow1;
   GtkWidget *btnAddNew;
   GtkWidget *btnRemove;
   GtkWidget *table1;
   GtkWidget *lblVendor;
   GtkWidget *lblDesc;
   GtkWidget *lblFPrint;
   GtkWidget *dialog_action_area1;
   GtkWidget *hbox1;
   GtkWidget *btnOK;
   GtkWidget *btnCancel;

   dlgSrcList = gtk_dialog_new();
   gtk_window_set_title(GTK_WINDOW(dlgSrcList), _("Setup Vendors"));
   //GTK_WINDOW (dlgSrcList)->type = GTK_WINDOW_DIALOG;
   gtk_window_set_modal(GTK_WINDOW(dlgSrcList), TRUE);
   gtk_window_set_default_size(GTK_WINDOW(dlgSrcList), -1, 400);
   gtk_window_set_policy(GTK_WINDOW(dlgSrcList), TRUE, TRUE, FALSE);

   dialog_vbox1 = GTK_DIALOG(dlgSrcList)->vbox;
   gtk_widget_show(dialog_vbox1);

   frmMain = gtk_frame_new(NULL);
   gtk_widget_show(frmMain);
   gtk_box_pack_start(GTK_BOX(dialog_vbox1), frmMain, TRUE, TRUE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(frmMain), 5);

   vbox1 = gtk_vbox_new(FALSE, 0);
   gtk_widget_show(vbox1);
   gtk_container_add(GTK_CONTAINER(frmMain), vbox1);
   gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);

   scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
   gtk_widget_show(scrolledwindow1);
   gtk_box_pack_start(GTK_BOX(vbox1), scrolledwindow1, TRUE, TRUE, 0);

   char *titles[] = { _("Vendor"), _("Description"), _("FingerPrint") };

   lstVendors = gtk_clist_new_with_titles(3, titles);
   gtk_widget_show(lstVendors);
   gtk_container_add(GTK_CONTAINER(scrolledwindow1), lstVendors);
   gtk_clist_set_column_width(GTK_CLIST(lstVendors), 0, 60);
   gtk_clist_set_column_width(GTK_CLIST(lstVendors), 1, 300);
   gtk_clist_set_column_width(GTK_CLIST(lstVendors), 2, 300);
   gtk_clist_set_column_auto_resize(GTK_CLIST(lstVendors), 1, TRUE);
   gtk_clist_set_column_auto_resize(GTK_CLIST(lstVendors), 2, TRUE);
   gtk_clist_column_titles_show(GTK_CLIST(lstVendors));

   table1 = gtk_table_new(2, 3, FALSE);
   gtk_widget_show(table1);
   gtk_box_pack_start(GTK_BOX(vbox1), table1, FALSE, TRUE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(table1), 5);
   gtk_table_set_col_spacings(GTK_TABLE(table1), 10);

   lblVendor = gtk_label_new(_("Vendor"));
   gtk_misc_set_alignment(GTK_MISC(lblVendor), 1.0, 0.5);
   gtk_widget_show(lblVendor);
   gtk_table_attach(GTK_TABLE(table1), lblVendor, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

   entryVendor = gtk_entry_new();
   gtk_widget_set_usize(entryVendor, 50, -1);
   gtk_widget_show(entryVendor);
   gtk_table_attach(GTK_TABLE(table1), entryVendor, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

   lblDesc = gtk_label_new(_("Description"));
   gtk_widget_show(lblDesc);
   gtk_table_attach(GTK_TABLE(table1), lblDesc, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

   entryDesc = gtk_entry_new();
   gtk_widget_set_usize(entryDesc, 300, -1);
   gtk_widget_show(entryDesc);
   gtk_table_attach(GTK_TABLE(table1), entryDesc, 1, 6, 1, 2,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

   lblFPrint = gtk_label_new(_("FingerPrint"));
   gtk_widget_show(lblFPrint);
   gtk_table_attach(GTK_TABLE(table1), lblFPrint, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

   entryFPrint = gtk_entry_new();
   gtk_widget_set_usize(entryFPrint, 300, -1);
   gtk_widget_show(entryFPrint);
   gtk_table_attach(GTK_TABLE(table1), entryFPrint, 1, 6, 2, 3,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

   dialog_action_area1 = GTK_DIALOG(dlgSrcList)->action_area;
   gtk_widget_show(dialog_action_area1);
   gtk_container_set_border_width(GTK_CONTAINER(dialog_action_area1), 10);

   hbox1 = gtk_hbox_new(FALSE, 0);
   gtk_widget_show(hbox1);
   gtk_box_pack_start(GTK_BOX(dialog_action_area1), hbox1, TRUE, TRUE, 8);

   btnOK = gtk_button_new_with_label(_("OK"));
   gtk_widget_show(btnOK);
   gtk_box_pack_start(GTK_BOX(hbox1), btnOK, TRUE, TRUE, 10);

   btnAddNew = gtk_button_new_with_label(_("Add"));
   gtk_widget_show(btnAddNew);
   gtk_box_pack_start(GTK_BOX(hbox1), btnAddNew, TRUE, TRUE, 10);

   btnRemove = gtk_button_new_with_label(_("Remove"));
   gtk_widget_show(btnRemove);
   gtk_box_pack_start(GTK_BOX(hbox1), btnRemove, TRUE, TRUE, 10);

   btnCancel = gtk_button_new_with_label(_("Cancel"));
   gtk_widget_show(btnCancel);
   gtk_box_pack_start(GTK_BOX(hbox1), btnCancel, TRUE, TRUE, 10);

   gtk_signal_connect(GTK_OBJECT(lstVendors), "select-row",
                      GTK_SIGNAL_FUNC(UpdateDisplay), (gpointer) this);
   gtk_signal_connect(GTK_OBJECT(lstVendors), "unselect-row",
                      GTK_SIGNAL_FUNC(UnselectRow), (gpointer) this);

   gtk_signal_connect(GTK_OBJECT(btnAddNew), "clicked",
                      GTK_SIGNAL_FUNC(DoAdd), (gpointer) this);

   gtk_signal_connect(GTK_OBJECT(btnRemove), "clicked",
                      GTK_SIGNAL_FUNC(DoRemove), (gpointer) this);

   gtk_signal_connect(GTK_OBJECT(btnOK), "clicked",
                      GTK_SIGNAL_FUNC(DoOK), (gpointer) this);
   gtk_signal_connect(GTK_OBJECT(btnCancel), "clicked",
                      GTK_SIGNAL_FUNC(DoCancel), (gpointer) this);

   return dlgSrcList;
#endif
}

void RGVendorsEditor::Run()
{
#if 0 // PORTME
   GdkColormap *cmap = gdk_colormap_get_system();
   GdkColor gray;
   gray.red = gray.green = gray.blue = 0xAA00;
   gdk_color_alloc(cmap, &gray);

   gtk_clist_freeze(GTK_CLIST(lstVendors));
   gtk_clist_clear(GTK_CLIST(lstVendors));

   for (VendorsListIter it = lst.VendorRecords.begin();
        it != lst.VendorRecords.end(); it++) {
      const gchar *rowtxt[] = {
         (*it)->VendorID.c_str(),
         (*it)->Description.c_str(),
         (*it)->FingerPrint.c_str()
      };
      gint row = gtk_clist_append(GTK_CLIST(lstVendors), (gchar **) rowtxt);
      gtk_clist_set_row_data(GTK_CLIST(lstVendors), row, (gpointer) (*it));
   }

   gtk_clist_thaw(GTK_CLIST(lstVendors));

   gtk_widget_show_all(dialog);
   gtk_main();
#endif
}

void RGVendorsEditor::DoAdd(GtkWidget *, gpointer data)
{
#if 0 // PORTME
   RGVendorsEditor *me = (RGVendorsEditor *) data;
   string VendorID;
   string Description;
   string FingerPrint;

   VendorID = gtk_entry_get_text(GTK_ENTRY(me->entryVendor));
   Description = gtk_entry_get_text(GTK_ENTRY(me->entryDesc));
   FingerPrint = gtk_entry_get_text(GTK_ENTRY(me->entryFPrint));

   SourcesList::VendorRecord *rec =
      me->lst.AddVendor(VendorID, FingerPrint, Description);

   const gchar *rowtxt[] = {
      rec->VendorID.c_str(),
      rec->Description.c_str(),
      rec->FingerPrint.c_str()
   };

   gint row = gtk_clist_append(GTK_CLIST(me->lstVendors), (gchar **) rowtxt);
   if (!gtk_clist_row_is_visible(GTK_CLIST(me->lstVendors), row))
      gtk_clist_moveto(GTK_CLIST(me->lstVendors), row, 0, 1.0, 0.0);
   gtk_clist_set_row_data(GTK_CLIST(me->lstVendors), row, (gpointer) rec);
   gtk_clist_select_row(GTK_CLIST(me->lstVendors), row, 0);
#endif
}

void RGVendorsEditor::DoEdit(GtkWidget *, gpointer data)
{
#if 0 // PORTME
   RGVendorsEditor *me = (RGVendorsEditor *) data;

   if (me->selectedrow < 0)
      return;                   /* no row selected */

   SourcesList::VendorRecord &rec =
      *((SourcesList::
         VendorRecord *) gtk_clist_get_row_data(GTK_CLIST(me->lstVendors),
                                                me->selectedrow));

   rec.VendorID = gtk_entry_get_text(GTK_ENTRY(me->entryVendor));
   rec.Description = gtk_entry_get_text(GTK_ENTRY(me->entryDesc));
   rec.FingerPrint = gtk_entry_get_text(GTK_ENTRY(me->entryFPrint));

   /* repaint screen */
   gtk_clist_set_text(GTK_CLIST(me->lstVendors), me->selectedrow,
                      0, rec.VendorID.c_str());
   gtk_clist_set_text(GTK_CLIST(me->lstVendors), me->selectedrow,
                      1, rec.Description.c_str());
   gtk_clist_set_text(GTK_CLIST(me->lstVendors), me->selectedrow,
                      2, rec.FingerPrint.c_str());
#endif
}

void RGVendorsEditor::DoRemove(GtkWidget *, gpointer data)
{
#if 0 // PORTME
   RGVendorsEditor *me = (RGVendorsEditor *) data;
   gint row = me->selectedrow;
   if (row < 0)
      return;
   me->selectedrow = -1;

   SourcesList::VendorRecord *rec =
      (SourcesList::VendorRecord *) gtk_clist_get_row_data(GTK_CLIST(me->lstVendors), row);
   assert(rec);

   me->lst.RemoveVendor(rec);
   gtk_clist_remove(GTK_CLIST(me->lstVendors), row);
#endif
}

void RGVendorsEditor::DoOK(GtkWidget *, gpointer data)
{
   DoEdit(NULL, data);
   RGVendorsEditor *me = (RGVendorsEditor *) data;
   me->lst.UpdateVendors();
   gtk_main_quit();
}

void RGVendorsEditor::DoCancel(GtkWidget *, gpointer data)
{
   //GtkUI::SrcEditor *This = (GtkUI::SrcEditor *)data;
   gtk_main_quit();
}

#if 0 // PORTME
void RGVendorsEditor::UnselectRow(GtkCList * clist, gint row, gint col,
                                  GdkEventButton * event, gpointer data)
{
   DoEdit(NULL, data);
   RGVendorsEditor *me = (RGVendorsEditor *) data;
   me->selectedrow = -1;

   gtk_entry_set_text(GTK_ENTRY(me->entryVendor), "");
   gtk_entry_set_text(GTK_ENTRY(me->entryDesc), "");
   gtk_entry_set_text(GTK_ENTRY(me->entryFPrint), "");
}

void RGVendorsEditor::UpdateDisplay(GtkCList * clist, gint row, gint col,
                                    GdkEventButton * event, gpointer data)
{
   DoEdit(NULL, data);
   RGVendorsEditor *me = (RGVendorsEditor *) data;
   me->selectedrow = row;

   const SourcesList::VendorRecord &rec =
      *((SourcesList::VendorRecord *) gtk_clist_get_row_data(clist, row));

   gtk_entry_set_text(GTK_ENTRY(me->entryVendor), rec.VendorID.c_str());
   gtk_entry_set_text(GTK_ENTRY(me->entryDesc), rec.Description.c_str());
   gtk_entry_set_text(GTK_ENTRY(me->entryFPrint), rec.FingerPrint.c_str());
}
#endif
