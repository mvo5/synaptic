/* $Id: rgsrcwindow.cc,v 1.15 2003/01/23 13:29:49 mvogt Exp $ */
#include <apt-pkg/error.h>
#include "rgsrcwindow.h"
#include "config.h"
#include "i18n.h"

#if HAVE_RPM
enum  {ITEM_TYPE_RPM, 
       ITEM_TYPE_RPMSRC, 
       ITEM_TYPE_DEB , 
       ITEM_TYPE_DEBSRC};
#else
enum  {ITEM_TYPE_DEB, 
       ITEM_TYPE_DEBSRC, 
       ITEM_TYPE_RPM , 
       ITEM_TYPE_RPMSRC};
#endif

RGSrcEditor::RGSrcEditor(RGWindow *parent)
  : RGWindow(parent, "sources")
{
	selectedrow = -1;
	dialog = CreateWidget();
	_userDialog = new RGUserDialog(dialog);
	Applied = false;
}

RGSrcEditor::~RGSrcEditor()
{
	gtk_widget_destroy(dialog);
	delete _userDialog;
}

GtkWidget *RGSrcEditor::CreateWidget()
{
	GtkWidget *dlgSrcList;
	GtkWidget *dialog_vbox1;
	GtkWidget *frmMain;
	GtkWidget *vbox1;
	GtkWidget *scrolledwindow1;
	GtkWidget *btnClear;
	GtkWidget *btnAddNew;
	GtkWidget *btnRemove;
	GtkWidget *table1;
	GtkWidget *hbox2;
	//GtkWidget *lblStatus;
	//GtkWidget *lblType;
	GtkWidget *btnEditVendors;
	GtkWidget *lblURI;
	GtkWidget *item;
	GtkWidget *lblDist;
	GtkWidget *lblSection;
	GtkWidget *dialog_action_area1;
	GtkWidget *hbox1;
	GtkWidget *btnOK;
	GtkWidget *btnCancel;

	dlgSrcList = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW (dlgSrcList), _("Setup Repositories"));
	//GTK_WINDOW (dlgSrcList)->type = GTK_WINDOW_DIALOG;
	gtk_window_set_modal (GTK_WINDOW (dlgSrcList), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dlgSrcList), -1, 400);
	gtk_window_set_policy (GTK_WINDOW (dlgSrcList), TRUE, TRUE, FALSE);

	dialog_vbox1 = GTK_DIALOG (dlgSrcList)->vbox;
	gtk_widget_show (dialog_vbox1);

	frmMain = gtk_frame_new (NULL);
	gtk_widget_show(frmMain);
	gtk_box_pack_start(GTK_BOX (dialog_vbox1), frmMain, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER (frmMain), 5);

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox1);
	gtk_container_add(GTK_CONTAINER (frmMain), vbox1);
	gtk_container_set_border_width(GTK_CONTAINER (vbox1), 5);

 	scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
 	gtk_widget_show(scrolledwindow1);
 	gtk_box_pack_start(GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 0);

	char *titles[] = {_("Status"),
			  _("Type"),
			  _("Vendor"),
			  _("URI"),
			  _("Distribution"),
			  _("Section(s)")};

	lstSources = gtk_clist_new_with_titles(6, titles);
	gtk_widget_show(lstSources);
	gtk_container_add(GTK_CONTAINER (scrolledwindow1), lstSources);
 	gtk_clist_set_column_width(GTK_CLIST (lstSources), 0, 61);
 	gtk_clist_set_column_width(GTK_CLIST (lstSources), 1, 71);
 	gtk_clist_set_column_width(GTK_CLIST (lstSources), 2, 52);
 	gtk_clist_set_column_width(GTK_CLIST (lstSources), 3, 300);
 	gtk_clist_set_column_width(GTK_CLIST (lstSources), 4, 80);
 	gtk_clist_set_column_width(GTK_CLIST (lstSources), 5, 260);
	gtk_clist_set_column_auto_resize(GTK_CLIST(lstSources), 3, TRUE);
	gtk_clist_column_titles_show(GTK_CLIST (lstSources));

	table1 = gtk_table_new (2, 3, FALSE);
	gtk_widget_show (table1);
	gtk_box_pack_start (GTK_BOX (vbox1), table1, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (table1), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 10);


	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox2);
	gtk_container_set_border_width (GTK_CONTAINER (hbox2), 10);
	gtk_table_attach (GTK_TABLE (table1), hbox2, 1, 2, 1, 2,
		    (GtkAttachOptions) (GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);

	cbDisabled = gtk_check_button_new_with_label (_("Disabled"));
	gtk_widget_show (cbDisabled);
	gtk_box_pack_start(GTK_BOX(hbox2), cbDisabled, TRUE, TRUE, 10);

	optType = gtk_option_menu_new();
	optTypeMenu = gtk_menu_new();

#if HAVE_RPM
	item = gtk_menu_item_new_with_label("rpm");
	gtk_menu_append(GTK_MENU(optTypeMenu), item);
	gtk_widget_show(item);
	gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_RPM);

	item = gtk_menu_item_new_with_label("rpm-src");
	gtk_menu_append(GTK_MENU(optTypeMenu), item);
	gtk_widget_show(item);
	gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_RPMSRC);

	item = gtk_menu_item_new_with_label("deb");
	gtk_menu_append(GTK_MENU(optTypeMenu), item);
	gtk_widget_show(item);
	gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_DEB);

	item = gtk_menu_item_new_with_label("deb-src");
	gtk_menu_append(GTK_MENU(optTypeMenu), item);
	gtk_widget_show(item);
	gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_DEBSRC);
#else
	item = gtk_menu_item_new_with_label("deb");
	gtk_menu_append(GTK_MENU(optTypeMenu), item);
	gtk_widget_show(item);
	gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_DEB);

	item = gtk_menu_item_new_with_label("deb-src");
	gtk_menu_append(GTK_MENU(optTypeMenu), item);
	gtk_widget_show(item);
	gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_DEBSRC);

	item = gtk_menu_item_new_with_label("rpm");
	gtk_menu_append(GTK_MENU(optTypeMenu), item);
	gtk_widget_show(item);
	gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_RPM);

	item = gtk_menu_item_new_with_label("rpm-src");
	gtk_menu_append(GTK_MENU(optTypeMenu), item);
	gtk_widget_show(item);
	gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_RPMSRC);
#endif

	gtk_option_menu_set_menu(GTK_OPTION_MENU(optType), optTypeMenu);

	gtk_box_pack_start(GTK_BOX(hbox2), optType, TRUE, TRUE, 10);

	optVendor = gtk_option_menu_new();
	optVendorMenu = gtk_menu_new();
	item = gtk_menu_item_new_with_label(_("(no vendor)"));
	gtk_menu_append(GTK_MENU(optVendorMenu), item);
	gtk_widget_show(item);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(optVendor), optVendorMenu);
	gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)"");
	gtk_box_pack_start(GTK_BOX(hbox2), optVendor, TRUE, TRUE, 10);

	btnEditVendors = gtk_button_new_with_label (_("Edit"));
	//gtk_widget_set_usize (btnEditVendors, 50, -1);
	gtk_widget_show (btnEditVendors);
	gtk_box_pack_start(GTK_BOX(hbox2), btnEditVendors, TRUE, TRUE, 10);

	lblURI = gtk_label_new (_("URI"));
	gtk_misc_set_alignment(GTK_MISC(lblURI), 1.0, 0.5);
	gtk_widget_show (lblURI);
	gtk_table_attach (GTK_TABLE (table1), lblURI, 0, 1, 2, 3,
		    (GtkAttachOptions) (GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);

	entryURI = gtk_entry_new ();
	gtk_widget_show (entryURI);
	gtk_table_attach (GTK_TABLE (table1), entryURI, 1, 8, 2, 3,
		    (GtkAttachOptions) (GTK_EXPAND|GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);


	lblDist = gtk_label_new (_("Distribution"));
	gtk_misc_set_alignment(GTK_MISC(lblDist), 1.0, 0.5);
	gtk_widget_show (lblDist);
	gtk_table_attach (GTK_TABLE (table1), lblDist, 0, 1, 3, 4,
		    (GtkAttachOptions) (GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);

	entryDist = gtk_entry_new ();
	gtk_widget_show (entryDist);
	gtk_table_attach (GTK_TABLE (table1), entryDist, 1, 8, 3, 4,
		    (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
		    (GtkAttachOptions) (0), 0, 0);

	lblSection = gtk_label_new (_("Section(s)"));
	gtk_misc_set_alignment(GTK_MISC(lblSection), 1.0, 0.5);
	gtk_widget_show (lblSection);
	gtk_table_attach (GTK_TABLE (table1), lblSection, 0, 1, 4, 5,
		    (GtkAttachOptions) (GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);

	entrySect = gtk_entry_new ();
	gtk_widget_show (entrySect);
	gtk_table_attach (GTK_TABLE (table1), entrySect, 1, 8, 4, 5,
		    (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
		    (GtkAttachOptions) (0), 0, 0);

	sourceFile = gtk_entry_new();

	dialog_action_area1 = GTK_DIALOG (dlgSrcList)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area1), 10);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (dialog_action_area1), hbox1, TRUE, TRUE, 8);

	btnOK = gtk_button_new_with_label (_("OK"));
	gtk_widget_show (btnOK);
	gtk_box_pack_start (GTK_BOX (hbox1), btnOK, TRUE, TRUE, 10);

	btnCancel = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_show (btnCancel);
	gtk_box_pack_end (GTK_BOX (hbox1), btnCancel, TRUE, TRUE, 10);

	btnClear = gtk_button_new_with_label (_("Clear"));
	gtk_widget_show (btnClear);
	gtk_box_pack_start (GTK_BOX (hbox1), btnClear, TRUE, TRUE, 10);

	btnAddNew = gtk_button_new_with_label (_("Add"));
	gtk_widget_show (btnAddNew);
	gtk_box_pack_start (GTK_BOX (hbox1), btnAddNew, TRUE, TRUE, 10);

	btnRemove = gtk_button_new_with_label (_("Remove"));
	gtk_widget_show (btnRemove);
	gtk_box_pack_start (GTK_BOX (hbox1), btnRemove, TRUE, TRUE, 10);

	gtk_signal_connect(GTK_OBJECT(btnEditVendors), "clicked",
		GTK_SIGNAL_FUNC(VendorsWindow), (gpointer)this);

	gtk_signal_connect(GTK_OBJECT(lstSources), "select-row",
		GTK_SIGNAL_FUNC(UpdateDisplay), (gpointer)this);
	gtk_signal_connect(GTK_OBJECT(lstSources), "unselect-row",
		GTK_SIGNAL_FUNC(UnselectRow), (gpointer)this);

	gtk_signal_connect(GTK_OBJECT(btnClear), "clicked",
			   GTK_SIGNAL_FUNC(DoClear), (gpointer)this);
	gtk_signal_connect(GTK_OBJECT(btnAddNew), "clicked",
			   GTK_SIGNAL_FUNC(DoAdd), (gpointer)this);

	gtk_signal_connect(GTK_OBJECT(btnRemove), "clicked",
			   GTK_SIGNAL_FUNC(DoRemove), (gpointer)this);

	gtk_signal_connect(GTK_OBJECT(btnOK), "clicked",
		GTK_SIGNAL_FUNC(DoOK), (gpointer)this);
	gtk_signal_connect(GTK_OBJECT(btnCancel), "clicked",
		GTK_SIGNAL_FUNC(DoCancel), (gpointer)this);

	return dlgSrcList;
}

bool RGSrcEditor::Run()
{
  if (lst.ReadSources() == false) {
    _error->Error(_("Cannot read sources.list file"));
    _userDialog->showErrors();
    return false;
  }
  if (lst.ReadVendors() == false) {
    _error->Error(_("Cannot read vendors.list file"));
    _userDialog->showErrors();
    return false;
  }

  GdkColormap *cmap = gdk_colormap_get_system();
  GdkColor gray;
  gray.red = gray.green = gray.blue = 0xAA00;
  gdk_color_alloc(cmap, &gray);
		
  gtk_clist_freeze(GTK_CLIST(lstSources));
  gtk_clist_clear(GTK_CLIST(lstSources));

  for(SourcesListIter it =  lst.SourceRecords.begin();
      it != lst.SourceRecords.end(); it++) {

    if ((*it)->Type & SourcesList::Comment) continue;
    string Sections;
    for (unsigned int J = 0; J < (*it)->NumSections; J++) {
      Sections += (*it)->Sections[J];
      Sections += " ";
    }

    const gchar *rowtxt[] = {
      (((*it)->Type & SourcesList::Disabled) ? "Disabled" : "Enabled"),
      (*it)->GetType().c_str(), (*it)->VendorID.c_str(), (*it)->URI.c_str(),
      (*it)->Dist.c_str(), Sections.c_str()
    };
    gint row = gtk_clist_append(GTK_CLIST(lstSources), (gchar **)rowtxt);
    gtk_clist_set_row_data(GTK_CLIST(lstSources), row, (gpointer)(*it));
    if ((*it)->Type & SourcesList::Disabled) 
      gtk_clist_set_foreground(GTK_CLIST(lstSources), row, &gray);
  }

  gtk_clist_thaw(GTK_CLIST(lstSources));

  UpdateVendorMenu();

  gtk_widget_show_all(dialog);
  gtk_main();

  return Applied;
}

void RGSrcEditor::UpdateVendorMenu()
{
	gtk_option_menu_remove_menu(GTK_OPTION_MENU(optVendor));
	optVendorMenu = gtk_menu_new();
	GtkWidget *item = gtk_menu_item_new_with_label(_("(no vendor)"));
	gtk_menu_append(GTK_MENU(optVendorMenu), item);
	gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)"");
	gtk_widget_show(item);
	for (VendorsListIter it = lst.VendorRecords.begin();
	     it != lst.VendorRecords.end(); it++) {
		item = gtk_menu_item_new_with_label((*it)->VendorID.c_str());
		gtk_menu_append(GTK_MENU(optVendorMenu), item);
		gtk_widget_show(item);
		gtk_object_set_data(GTK_OBJECT(item), "id",
				    (gpointer)(*it)->VendorID.c_str());
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU(optVendor), optVendorMenu);
}

int RGSrcEditor::VendorMenuIndex(string VendorID)
{
	int index = 0;
	for (VendorsListIter it = lst.VendorRecords.begin();
	     it != lst.VendorRecords.end(); it++) {
		index += 1;
		if ((*it)->VendorID == VendorID)
			return index;
	}
	return 0;
}

void RGSrcEditor::DoClear(GtkWidget *, gpointer data)
{
        RGSrcEditor *me = (RGSrcEditor*)data;
	me->UnselectRow(NULL, 0, 0, NULL, data);
}

void RGSrcEditor::DoAdd(GtkWidget *, gpointer data)
{
        RGSrcEditor *me = (RGSrcEditor*)data;
	unsigned char Type = 0;
	const char *Section;
	string VendorID;
	string URI;
	string Dist;
	string Sections[5];
	unsigned short count = 0;
	// XXX user added entries go to sources.list for now..
	string SourceFile = "/etc/apt/sources.list";

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->cbDisabled)))
		Type |= SourcesList::Disabled;

	GtkWidget *menuitem = gtk_menu_get_active(GTK_MENU(me->optTypeMenu));
	switch ((int)(gtk_object_get_data(GTK_OBJECT(menuitem), "id"))) {
		case ITEM_TYPE_DEB:
			Type |= SourcesList::Deb; break;
		case ITEM_TYPE_DEBSRC:
			Type |= SourcesList::DebSrc; break;
		case ITEM_TYPE_RPM:
			Type |= SourcesList::Rpm; break;
		case ITEM_TYPE_RPMSRC:
			Type |= SourcesList::RpmSrc; break;
		default:
		  me->_userDialog->error(_("Unknown source type"));
		  return;
	}

	menuitem = gtk_menu_get_active(GTK_MENU(me->optVendorMenu));
	VendorID = (char*)gtk_object_get_data(GTK_OBJECT(menuitem), "id");

	URI = gtk_entry_get_text(GTK_ENTRY(me->entryURI));
	Dist = gtk_entry_get_text(GTK_ENTRY(me->entryDist));
	Section = gtk_entry_get_text(GTK_ENTRY(me->entrySect));
    SourceFile = gtk_entry_get_text(GTK_ENTRY(me->sourceFile));

	if (Section != 0 && Section[0] != 0)
		Sections[count++] = Section;

	SourcesList::SourceRecord *rec = me->lst.AddSource((SourcesList::RecType)Type, VendorID, URI, Dist, Sections, count, SourceFile);

	if(rec == NULL) {
	  me->_userDialog->error(_("Invalid URL"));
	  return;
	}

	string Sects;
	for (unsigned short J = 0; J < rec->NumSections; J++) {
		Sects += rec->Sections[J];
		Sects += " ";
	}
	
	const gchar *rowtxt[] = {
		((rec->Type & SourcesList::Disabled) ? "Disabled" : "Enabled"),
		rec->GetType().c_str(), rec->VendorID.c_str(), rec->URI.c_str(),
		rec->Dist.c_str(), Sects.c_str()
	};
	gint row = gtk_clist_append(GTK_CLIST(me->lstSources), (gchar **)rowtxt);
	if(!gtk_clist_row_is_visible(GTK_CLIST(me->lstSources), row))
		gtk_clist_moveto(GTK_CLIST(me->lstSources), row, 0, 1.0,  0.0);
	gtk_clist_set_row_data(GTK_CLIST(me->lstSources), row, (gpointer)rec);
	gtk_clist_select_row(GTK_CLIST(me->lstSources), row, 0);

	if (rec->Type & SourcesList::Disabled) {
		GdkColormap *cmap = gdk_colormap_get_system();
		GdkColor gray;
		gray.red = gray.green = gray.blue = 0xAA00;
		gdk_color_alloc(cmap, &gray);
		gtk_clist_set_foreground(GTK_CLIST(me->lstSources),
			row, &gray);
	} else {
		gtk_clist_set_foreground(GTK_CLIST(me->lstSources),
			row, NULL);
	}
}

void RGSrcEditor::DoEdit(GtkWidget *, gpointer data)
{
        RGSrcEditor *me = (RGSrcEditor*)data;

	if (me->selectedrow < 0) return; /* no row selected */

	SourcesList::SourceRecord &rec = *((SourcesList::SourceRecord *)gtk_clist_get_row_data(GTK_CLIST(me->lstSources), me->selectedrow));

	rec.Type = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->cbDisabled)))
		rec.Type |= SourcesList::Disabled;
		
	GtkWidget *menuitem = gtk_menu_get_active(GTK_MENU(me->optTypeMenu));
	switch ((long)(gtk_object_get_data(GTK_OBJECT(menuitem), "id"))) {
		case ITEM_TYPE_DEB:
			rec.Type |= SourcesList::Deb; break;
		case ITEM_TYPE_DEBSRC:
			rec.Type |= SourcesList::DebSrc; break;
		case ITEM_TYPE_RPM:
			rec.Type |= SourcesList::Rpm; break;
		case ITEM_TYPE_RPMSRC:
			rec.Type |= SourcesList::RpmSrc; break;
		default:
		  me->_userDialog->error(_("Unknown source type"));
		  return;
	}

	menuitem = gtk_menu_get_active(GTK_MENU(me->optVendorMenu));
	rec.VendorID = (char*)gtk_object_get_data(GTK_OBJECT(menuitem), "id");

	rec.URI = gtk_entry_get_text(GTK_ENTRY(me->entryURI));
	rec.Dist = gtk_entry_get_text(GTK_ENTRY(me->entryDist));
	rec.SourceFile = gtk_entry_get_text(GTK_ENTRY(me->sourceFile));
	
	delete [] rec.Sections;
	rec.NumSections = 0;

	const char *Section = gtk_entry_get_text(GTK_ENTRY(me->entrySect));
	if (Section != 0 && Section[0] != 0)
		rec.NumSections++;

	rec.Sections = new string[rec.NumSections];
	rec.NumSections = 0;
	Section = gtk_entry_get_text(GTK_ENTRY(me->entrySect));

	if (Section != 0 && Section[0] != 0)
		rec.Sections[rec.NumSections++] = Section;

	string Sect;
	for (unsigned int I = 0; I < rec.NumSections; I++) {
		Sect += rec.Sections[I];
		Sect += " ";
	}

	/* repaint screen */
	gtk_clist_set_text(GTK_CLIST(me->lstSources), me->selectedrow,
		0, (rec.Type & SourcesList::Disabled ? "Disabled" : "Enabled"));
	gtk_clist_set_text(GTK_CLIST(me->lstSources), me->selectedrow,
		1, rec.GetType().c_str());
	gtk_clist_set_text(GTK_CLIST(me->lstSources), me->selectedrow,
		2, rec.VendorID.c_str());
	gtk_clist_set_text(GTK_CLIST(me->lstSources), me->selectedrow,
		3, rec.URI.c_str());
	gtk_clist_set_text(GTK_CLIST(me->lstSources), me->selectedrow,
		4, rec.Dist.c_str());
	gtk_clist_set_text(GTK_CLIST(me->lstSources), me->selectedrow,
		5, Sect.c_str());

	if (rec.Type & SourcesList::Disabled) {
		GdkColormap *cmap = gdk_colormap_get_system();
		GdkColor gray;
		gray.red = gray.green = gray.blue = 0xAA00;
		gdk_color_alloc(cmap, &gray);
		gtk_clist_set_foreground(GTK_CLIST(me->lstSources),
			me->selectedrow, &gray);
	} else {
		gtk_clist_set_foreground(GTK_CLIST(me->lstSources),
			me->selectedrow, NULL);
	}
}

void RGSrcEditor::DoRemove(GtkWidget *, gpointer data)
{
	RGSrcEditor *me = (RGSrcEditor*)data;	
	gint row = me->selectedrow;
	if (row < 0) return;
	me->selectedrow = -1;

	SourcesList::SourceRecord *rec = (SourcesList::SourceRecord *)gtk_clist_get_row_data(GTK_CLIST(me->lstSources), row);
	assert(rec);

	me->lst.RemoveSource(rec);
	gtk_clist_remove(GTK_CLIST(me->lstSources), row);
}

void RGSrcEditor::DoOK(GtkWidget *, gpointer data)
{
	DoEdit(NULL, data);
	RGSrcEditor *me = (RGSrcEditor*)data;	
	me->lst.UpdateSources();
	gtk_main_quit();
	me->Applied = true;
}

void RGSrcEditor::DoCancel(GtkWidget *, gpointer data)
{
	//GtkUI::SrcEditor *This = (GtkUI::SrcEditor *)data;
	gtk_main_quit();
}

void RGSrcEditor::UnselectRow(GtkCList *clist, gint row, gint col,
	GdkEventButton *event, gpointer data)
{
	DoEdit(NULL, data);
        RGSrcEditor *me = (RGSrcEditor*)data;
	me->selectedrow = -1;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(me->cbDisabled), FALSE);
	gtk_option_menu_set_history(GTK_OPTION_MENU(me->optType), 0);
	gtk_option_menu_set_history(GTK_OPTION_MENU(me->optVendor), 0);
	gtk_entry_set_text(GTK_ENTRY(me->entryURI), "");
	gtk_entry_set_text(GTK_ENTRY(me->entryDist), "");
	gtk_entry_set_text(GTK_ENTRY(me->entrySect), "");
	gtk_entry_set_text(GTK_ENTRY(me->sourceFile), "");
}

void RGSrcEditor::UpdateDisplay(GtkCList *clist, gint row, gint col,
	GdkEventButton *event, gpointer data)
{
	DoEdit(NULL, data);
        RGSrcEditor *me = (RGSrcEditor*)data;
	me->selectedrow = row;

	const SourcesList::SourceRecord &rec = *((SourcesList::SourceRecord *)gtk_clist_get_row_data(clist, row));

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(me->cbDisabled), (rec.Type & SourcesList::Disabled));
	int id = ITEM_TYPE_DEB;
	if (rec.Type & SourcesList::DebSrc)
		id = ITEM_TYPE_DEBSRC;
	else if (rec.Type & SourcesList::Rpm)
		id = ITEM_TYPE_RPM;
	else if (rec.Type & SourcesList::RpmSrc)
		id = ITEM_TYPE_RPMSRC;
	gtk_option_menu_set_history(GTK_OPTION_MENU(me->optType), id);

	gtk_option_menu_set_history(GTK_OPTION_MENU(me->optVendor),
				    me->VendorMenuIndex(rec.VendorID));

	gtk_entry_set_text(GTK_ENTRY(me->entryURI), rec.URI.c_str());
	gtk_entry_set_text(GTK_ENTRY(me->entryDist), rec.Dist.c_str());
	gtk_entry_set_text(GTK_ENTRY(me->sourceFile), rec.SourceFile.c_str());
	gtk_entry_set_text(GTK_ENTRY(me->entrySect), "");

	for (unsigned int I = 0; I < rec.NumSections; I++) {
		gtk_entry_append_text(GTK_ENTRY(me->entrySect), rec.Sections[I].c_str());
		gtk_entry_append_text(GTK_ENTRY(me->entrySect), " ");
	}
}

void RGSrcEditor::VendorsWindow(GtkWidget *, void *data)
{
	RGSrcEditor *me = (RGSrcEditor*)data;
	RGVendorsEditor w(me, me->lst);
	w.Run();
	GtkWidget *menuitem = gtk_menu_get_active(GTK_MENU(me->optVendorMenu));
	string VendorID = (char*)gtk_object_get_data(GTK_OBJECT(menuitem), "id");
	me->UpdateVendorMenu();
	gtk_option_menu_set_history(GTK_OPTION_MENU(me->optVendor),
		    		    me->VendorMenuIndex(VendorID));
}
