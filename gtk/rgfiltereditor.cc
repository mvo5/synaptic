/* rgfiltereditor.cc
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

#include "config.h"
#include "i18n.h"


#include "rpackagelister.h"

#include "rgfiltereditor.h"



enum {
  CNExclude,
  CNPattern,
  CNWhere
};


static char *ActOptions[] = {
    _("Include"),
    _("Exclude"),
    NULL
};


static char *DepOptions[] = {
    _("are Named"),
    _("in Version"),
    _("in Description"),
    _("Depends on"), // depends, predepends etc
    _("Provides"), // provides and name
    _("Conflicts with"), // conflicts
    _("Replaces"), // replaces/obsoletes
    _("Suggests or Recommends"), // suggests/recommends
    _("Reverse Depends"), // Reverse Depends
    NULL
};



static bool bla = false;

RGFilterEditor::RGFilterEditor(GtkWidget *parent)
{
    if (!bla) {
	for (int i = 0; ActOptions[i]; i++) {
	    ActOptions[i] = _(ActOptions[i]);
	}
	for (int i = 0; DepOptions[i]; i++) {
	    DepOptions[i] = _(DepOptions[i]);
	}
	bla = true;
    }
    
    _tabview = gtk_notebook_new();
    gtk_widget_show(_tabview);

    makeSectionFilterPanel(_tabview);
    
    makeStatusFilterPanel(_tabview);
    
    makePatternFilterPanel(_tabview);
}


void RGFilterEditor::resetFilter()
{
    gtk_clist_unselect_all(GTK_CLIST(_groupL));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_exclGB), TRUE);

    for (int i = 0; i < NrOfStatusBits; i++) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_statusB[i]), TRUE);
    }
}


void RGFilterEditor::editFilter(RFilter *filter)
{
    resetFilter();

    _filter = filter;
    
    setSectionFilter(filter->section);

    setStatusFilter(filter->status);
    
    setPatternFilter(filter->pattern);
}


void RGFilterEditor::applyChanges()
{
    getSectionFilter(_filter->section);
    getStatusFilter(_filter->status);
    getPatternFilter(_filter->pattern);
}


// mvo: helper function as gtk_clist_find_row_from_data will
//      only compare pointers not the data ???
int clist_find_row_from_text(GtkCList *clist, char *text)
{
  int i=0;
  char *s;

  while(gtk_clist_get_text(GTK_CLIST(clist), i, 0, &s))  {
    if(strcmp(s,text) == 0) {
      return i;
    }
    i++;
  }
  return -1;
}


void RGFilterEditor::setSectionFilter(RSectionPackageFilter &f)
{    
    string section;
    int row;
    
    gtk_clist_unselect_all(GTK_CLIST(_groupL));
    
    for (int i = f.count()-1; i >= 0; i--) {
	section = f.section(i);
	row = clist_find_row_from_text(GTK_CLIST(_groupL), 
				       (char*)section.c_str());
	if (row >= 0) 
	  gtk_clist_select_row(GTK_CLIST(_groupL), row, 0);
    }

    if (f.inclusive()) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_inclGB), TRUE);
    } else {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_exclGB), TRUE);
    }
}


void RGFilterEditor::getSectionFilter(RSectionPackageFilter &f)
{        
    GList *list;
    
    int inclusive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_inclGB));
    f.setInclusive(inclusive == TRUE);

    f.clear();
    
    list = GTK_CLIST(_groupL)->selection;

    while (list) {
	int row = GPOINTER_TO_INT(list->data);
	char *text;
	
	gtk_clist_get_text(GTK_CLIST(_groupL), row, 0, &text);

	f.addSection(string(text));
	
	list = g_list_next(list);
    }
}


void RGFilterEditor::setStatusFilter(RStatusPackageFilter &f)
{    
    int i;
    int type = f.status();

    for (i = 0; i < NrOfStatusBits; i++) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_statusB[i]),
				     (type & StatusMasks[i]) ? TRUE : FALSE);
    }
}


void RGFilterEditor::getStatusFilter(RStatusPackageFilter &f)
{
    int i;
    int type = 0;

    for (i = 0; i < NrOfStatusBits; i++) {
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_statusB[i])))
	    type |= StatusMasks[i];
    }
    
    f.setStatus(type);
}


bool RGFilterEditor::getPatternRow(int row, 
				   bool &exclude,
				   RPatternPackageFilter::DepType &type,
				   string &pattern)
{
    char *str;
    
    if (!gtk_clist_get_text(GTK_CLIST(_patT), row, 0, &str))
	return false;

    if (strcmp(str, ActOptions[0]) == 0)
	exclude = false;
    else
	exclude = true;
    
    gtk_clist_get_text(GTK_CLIST(_patT), row, 1, &str);
    for (int j = 0; DepOptions[j]; j++) {
	if (strcmp(str, DepOptions[j]) == 0) {
	    type = (RPatternPackageFilter::DepType)j;
	    break;
	}
    }
    
    gtk_clist_get_text(GTK_CLIST(_patT), row, 2, &str);
    pattern = string(str);
    
    return true;
}


bool RGFilterEditor::setPatternRow(int row, 
				   bool exclude,
				   RPatternPackageFilter::DepType type,
				   string pattern)
{
    char *array[3];
    
    array[0] = ActOptions[exclude ? 1 : 0];
    array[1] = DepOptions[(int)type];
    array[2] = (char*)pattern.c_str();
    
    if (row < 0)
	gtk_clist_append(GTK_CLIST(_patT), array);
    else {
	for (int i = 0; i < 3; i++)
	    gtk_clist_set_text(GTK_CLIST(_patT), row, i, array[i]);
    }
    
    return true;
}



void RGFilterEditor::setPatternFilter(RPatternPackageFilter &f)
{    
    int i;

    gtk_clist_clear(GTK_CLIST(_patT));

    for (i = 0; i < f.count(); i++) {
	RPatternPackageFilter::DepType type;
	string pattern;
	bool exclude;
	f.pattern(i, type, pattern, exclude);
	setPatternRow(-1, exclude, type, pattern);
    }
}



void RGFilterEditor::getPatternFilter(RPatternPackageFilter &f)
{
    f.reset();

    for (int i = 0; i < GTK_CLIST(_patT)->rows; i++) {
	bool exclude;
	string pattern;
	RPatternPackageFilter::DepType type;
	
	getPatternRow(i, exclude, type, pattern);
	
	f.addPattern(type, pattern, exclude);
    }
}



void RGFilterEditor::makeSectionFilterPanel(GtkWidget *tabview)
{
    GtkWidget *box = gtk_vbox_new(FALSE, 0);

    gtk_container_set_border_width(GTK_CONTAINER(box), 10);

    GtkWidget *sview;
    
    sview = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(box), sview, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sview),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    
    _groupL = gtk_clist_new(1);
    gtk_clist_set_selection_mode(GTK_CLIST(_groupL), GTK_SELECTION_MULTIPLE);
    gtk_container_add(GTK_CONTAINER(sview), _groupL);    
    gtk_object_set_data(GTK_OBJECT(_groupL), "me", this);

    _inclGB = gtk_radio_button_new_with_label(NULL, 
	      _("Include Only Selected Sections"));
    gtk_box_pack_start(GTK_BOX(box), _inclGB, FALSE, TRUE, 0);

    _exclGB = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(_inclGB),
	     _("Exclude Selected Sections"));
    gtk_box_pack_start(GTK_BOX(box), _exclGB, FALSE, TRUE, 0);

    gtk_widget_show_all(box);
    gtk_notebook_append_page(GTK_NOTEBOOK(_tabview), box, 
			     gtk_label_new(_("by Section")));
}



void RGFilterEditor::makeStatusFilterPanel(GtkWidget *tabview)
{
    GtkWidget *box = gtk_vbox_new(FALSE, 0);
    GtkWidget *but;
    GtkWidget *label;
    int i = 0;

    gtk_container_set_border_width(GTK_CONTAINER(box), 10);

    label = gtk_label_new(_("Include packages that are..."));
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

    but = gtk_check_button_new_with_label(_("Not Installed"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;
    
    but = gtk_check_button_new_with_label(_("Installed and Upgradable"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;
    
    but = gtk_check_button_new_with_label(_("Installed"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;

    // sperator for now, latter a seperate page(?)
    but = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);


    but = gtk_check_button_new_with_label(_("Marked to Keep"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;

    but = gtk_check_button_new_with_label(_("Marked to Install/Upgrade"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;

    but = gtk_check_button_new_with_label(_("Marked to Remove"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;
    
    but = gtk_check_button_new_with_label(_("Broken"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;

    // sperator for now, latter a seperate page
    but = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);

    but = gtk_check_button_new_with_label(_("New"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;

    but = gtk_check_button_new_with_label(_("Pinned"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;

#ifndef HAVE_RPM /* mvo: only interessting for debian right now */
    but = gtk_check_button_new_with_label(_("Orphaned"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;
#endif   

    but = gtk_check_button_new_with_label(_("Residual Config"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;

#ifndef HAVE_RPM /* mvo: only interessting for debian right now */
    but = gtk_check_button_new_with_label(_("Debconf"));
    gtk_box_pack_start(GTK_BOX(box), but, FALSE, TRUE, 0);
    _statusB[i++] = but;
#endif   

    gtk_widget_show_all(box);
    gtk_notebook_append_page(GTK_NOTEBOOK(_tabview), box, 
			     gtk_label_new(_("by Status")));
}


void RGFilterEditor::setPackageSections(vector<string> &sections)
{
    for (vector<string>::const_iterator iter = sections.begin();
	 iter != sections.end();
	 iter++) {
	char *array[1];
	
	array[0] = (char*)iter->c_str();
	gtk_clist_append(GTK_CLIST(_groupL), array);
    }
    
    gtk_clist_sort(GTK_CLIST(_groupL));
}

void RGFilterEditor::changedAction(GtkWidget *w, void *data)
{
  RPatternPackageFilter::DepType *type;
  string str;
  bool exclude;
  GtkWidget *item, *menu;

  RGFilterEditor *fwin = (RGFilterEditor*)data;

  gtk_signal_handler_block_by_func (GTK_OBJECT (fwin->_patternT),
				    GTK_SIGNAL_FUNC (changedAction),
				    data);
  
  menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(fwin->_actionP));
  item = gtk_menu_get_active(GTK_MENU(menu));
  int *j = (int*)gtk_object_get_user_data(GTK_OBJECT(item));
  if(*j==0)
    exclude=false;
  else
    exclude=true;

  menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(fwin->_whatP));
  item = gtk_menu_get_active(GTK_MENU(menu));
  type = (RPatternPackageFilter::DepType*)
    gtk_object_get_user_data(GTK_OBJECT(item));

  str = gtk_entry_get_text(GTK_ENTRY(fwin->_patternT));
  fwin->setPatternRow(fwin->selected_row, exclude, *type, str);

  gtk_signal_handler_unblock_by_func (GTK_OBJECT (fwin->_patternT),
				    GTK_SIGNAL_FUNC (changedAction),
				    data);
}

void RGFilterEditor::newPatternAction(GtkWidget *w, void *data)
{
    RGFilterEditor *fwin = (RGFilterEditor*)data;

    fwin->setPatternRow(-1, false, (RPatternPackageFilter::DepType)0, "");

    gtk_clist_moveto(GTK_CLIST(fwin->_patT), 
		     GTK_CLIST(fwin->_patT)->rows, 0, 0.5, 0.0);
}


void RGFilterEditor::removePatternAction(GtkWidget *w, void *data)
{
    RGFilterEditor *fwin = (RGFilterEditor*)data;
    int row;
    
    assert(GTK_CLIST(fwin->_patT)->selection);

    row = GPOINTER_TO_INT(GTK_CLIST(fwin->_patT)->selection->data);

    assert(row >= 0);
    gtk_clist_remove(GTK_CLIST(fwin->_patT), row);
}


void RGFilterEditor::selectedPatternRow(GtkWidget *w, int row, int col,
					GdkEvent *event)
{
    RGFilterEditor *fwin = (RGFilterEditor*)gtk_object_get_data(GTK_OBJECT(w),
								"me");

    fwin->selected_row = row;

    gtk_widget_set_sensitive(fwin->_removePB, TRUE);
    gtk_widget_set_sensitive(fwin->_actionP, TRUE);
    gtk_widget_set_sensitive(fwin->_whatP, TRUE);
    gtk_widget_set_sensitive(fwin->_patternT, TRUE);
    
    
    string str;
    RPatternPackageFilter::DepType type;
    bool exclude;
    
    fwin->getPatternRow(row, exclude, type, str);
    
    gtk_option_menu_set_history(GTK_OPTION_MENU(fwin->_actionP), 
				exclude ? 1 : 0);
    gtk_option_menu_set_history(GTK_OPTION_MENU(fwin->_whatP),
				(int)type);
    gtk_entry_set_text(GTK_ENTRY(fwin->_patternT), str.c_str());
}


void RGFilterEditor::unselectedPatternRow(GtkWidget *w, int row, int col,
					GdkEvent *event)
{
    RGFilterEditor *fwin = (RGFilterEditor*)gtk_object_get_data(GTK_OBJECT(w),
								"me");

    fwin->selected_row = -1;
    gtk_widget_set_sensitive(fwin->_removePB, FALSE);
    gtk_widget_set_sensitive(fwin->_actionP, FALSE);
    gtk_widget_set_sensitive(fwin->_whatP, FALSE);
    gtk_widget_set_sensitive(fwin->_patternT, FALSE);
}


void RGFilterEditor::makePatternFilterPanel(GtkWidget *tabview)
{
    GtkWidget *box = gtk_vbox_new(FALSE, 5);
    GtkWidget *hbox;
    GtkWidget *button;

    
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);

    
    GtkWidget *sview;
    
    sview = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sview),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_widget_show(sview);
    gtk_box_pack_start(GTK_BOX(box), sview, TRUE, TRUE, 0);
    
    char *titles[3];

    titles[0] = _("Do");
    titles[1] = _("Packages that");
    titles[2] = _("Pattern");

    _patT = gtk_clist_new_with_titles(3, titles);
    gtk_widget_show(_patT);
    gtk_object_set_data(GTK_OBJECT(_patT), "me", this);
    gtk_container_add(GTK_CONTAINER(sview), _patT);
    gtk_signal_connect(GTK_OBJECT(_patT), "select-row",
		       (GtkSignalFunc)selectedPatternRow, NULL);
    gtk_signal_connect(GTK_OBJECT(_patT), "unselect-row",
		       (GtkSignalFunc)unselectedPatternRow, NULL);
    
    gtk_clist_set_column_width(GTK_CLIST(_patT), 0, 50);
    gtk_clist_set_column_width(GTK_CLIST(_patT), 1, 100);
    gtk_clist_set_column_width(GTK_CLIST(_patT), 2, 200);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

    _actionP = gtk_option_menu_new();
    gtk_widget_set_sensitive(_actionP, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), _actionP, FALSE, TRUE, 0);
    //gtk_widget_set_usize(_actionP, 80, -1);
    
    GtkWidget *menu, *item;
    menu = gtk_menu_new();
    
    item = gtk_menu_item_new_with_label(_("Include"));
    gtk_widget_show(item);
    int *i;
    i = new(int);
    *i=0;
    gtk_object_set_user_data(GTK_OBJECT(item), i);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
		       GTK_SIGNAL_FUNC(changedAction), this);
    gtk_menu_append(GTK_MENU(menu), item);

    item = gtk_menu_item_new_with_label(_("Exclude"));
    gtk_widget_show(item);
    i = new(int);
    *i=1;
    gtk_object_set_user_data(GTK_OBJECT(item), i);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
		       GTK_SIGNAL_FUNC(changedAction), this);
    gtk_menu_append(GTK_MENU(menu), item);
    
    gtk_option_menu_set_menu(GTK_OPTION_MENU(_actionP), menu);
    
    _whatP = gtk_option_menu_new();
    gtk_widget_set_sensitive(_whatP, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), _whatP, FALSE, TRUE, 2);
    //gtk_widget_set_usize(_whatP, 120, -1);
    
    menu = gtk_menu_new();
    int *j;
    for (int i = 0; DepOptions[i]!=NULL; i++) {
	item = gtk_menu_item_new_with_label(DepOptions[i]);
	j = new(int); *j=i;
	gtk_object_set_user_data(GTK_OBJECT(item), j);
        gtk_signal_connect(GTK_OBJECT(item), "activate", 
			   GTK_SIGNAL_FUNC(changedAction), this);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu), item);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(_whatP), menu);
    
    _patternT = gtk_entry_new();
    gtk_widget_set_sensitive(_patternT, FALSE);
    gtk_signal_connect(GTK_OBJECT(_patternT), "changed", 
		       GTK_SIGNAL_FUNC(changedAction), this);
    gtk_box_pack_start(GTK_BOX(hbox), _patternT, TRUE, TRUE, 0);
    
    
    gtk_widget_show_all(hbox);
    
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

    button = gtk_button_new_with_label(_("New"));
    //gtk_widget_set_usize(button, 80, -1);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)newPatternAction, this);
    
    _removePB = button = gtk_button_new_with_label(_("Remove"));
    //gtk_widget_set_usize(button, 80, -1);    
    gtk_widget_set_sensitive(button, FALSE);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)removePatternAction, this);

    gtk_widget_show_all(hbox);
    gtk_widget_show(hbox);
    gtk_widget_show(box);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(_tabview), box,
			     gtk_label_new(_("by Package")));
}

