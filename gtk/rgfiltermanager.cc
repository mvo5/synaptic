/* rgfiltermanager.cc
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
#include "rgfiltermanager.h"


RGFilterManagerWindow::RGFilterManagerWindow(RGWindow *win, 
					     RPackageLister *lister)
    : RGWindow(win, "filterManager"), _lister(lister)
{
    GtkWidget *box;
    GtkWidget *vbox;
    GtkWidget *hbox;

    setTitle(_("Package Filters"));

    box = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(box);
    gtk_box_pack_start(GTK_BOX(_topBox), box, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox);
    gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 0);

    _nameT = gtk_entry_new();
    gtk_widget_show(_nameT);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(_nameT), FALSE, FALSE, 0);

    GtkWidget *sview;
    sview = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), sview, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sview),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_widget_show(sview);

    _filterL = gtk_clist_new(1);
    // mvo: will handle editFilterAction as well
    gtk_signal_connect(GTK_OBJECT(_filterL), "select-row",
		       (GtkSignalFunc)selectAction, this);

    gtk_widget_show(_filterL);
    gtk_container_add(GTK_CONTAINER(sview), GTK_WIDGET(_filterL));
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(_topBox), hbox, FALSE, FALSE, 0);
    

    for (int i = 0; i < 5; i++) {
	GtkWidget *button;
	switch (i) {
	 case 0:
	    button = gtk_button_new_with_label(_("New"));
	    _newB = button;
	    gtk_box_pack_start(GTK_BOX(hbox), 
			       GTK_WIDGET(button), TRUE, TRUE, 0);
	    gtk_widget_show(button);
	    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
			       (GtkSignalFunc)newFilterAction, this);
	    break;
	 case 1:
 	   button = gtk_button_new_with_label(_("Delete"));
	   gtk_box_pack_start(GTK_BOX(hbox), 
			      GTK_WIDGET(button),  TRUE,  TRUE, 0);
	   gtk_widget_show(button);
	   gtk_signal_connect(GTK_OBJECT(button), "clicked", 
			      (GtkSignalFunc)deleteFilterAction, this);
	   break;
	 case 2:
 	   button = gtk_button_new_with_label(_("Clear"));
	   gtk_box_pack_start(GTK_BOX(hbox), 
			      GTK_WIDGET(button), TRUE, TRUE, 0);
	   gtk_widget_show(button);
	   gtk_signal_connect(GTK_OBJECT(button), "clicked", 
			      (GtkSignalFunc)clearFilterAction, this);
	   break;
	 case 3:
 	   button = gtk_button_new_with_label(_("Save"));
	   gtk_box_pack_start(GTK_BOX(hbox), 
			      GTK_WIDGET(button), TRUE, TRUE, 0);
	   gtk_widget_show(button);
	   gtk_signal_connect(GTK_OBJECT(button), "clicked", 
			      (GtkSignalFunc)applyFilterAction, this);
	   break;
	 case 4:
 	   button = gtk_button_new_with_label(_("Close"));
	   gtk_box_pack_start(GTK_BOX(hbox), 
			      GTK_WIDGET(button), TRUE, TRUE, 0);
	   gtk_widget_show(button);
	   gtk_signal_connect(GTK_OBJECT(button), "clicked", 
			      (GtkSignalFunc)closeAction, this);
	    break;
	}
    }
    gtk_widget_show(hbox);
    gtk_signal_connect(GTK_OBJECT(_win), "delete_event",
		       GTK_SIGNAL_FUNC(deleteEventAction), this);

    _editor = new RGFilterEditor(box);
    gtk_widget_show(_editor->widget());
    gtk_box_pack_start(GTK_BOX(box), 
		       GTK_WIDGET(_editor->widget()), TRUE, TRUE, 0);
    
    {
      vector<string> tmp;
      _lister->getSections(tmp);
      _editor->setPackageSections(tmp);
    }
    
    gtk_widget_show(_topBox);
    gtk_widget_show(_win);
}


RGFilterManagerWindow::~RGFilterManagerWindow()
{
    delete _editor;
}

gint RGFilterManagerWindow::deleteEventAction( GtkWidget *widget,
                   GdkEvent  *event,
                   gpointer   data )
{
  RGFilterManagerWindow *me = (RGFilterManagerWindow*)data;
  me->closeAction(widget, data);

  return(TRUE);
}


void RGFilterManagerWindow::show()
{
  gtk_clist_clear(GTK_CLIST(_filterL));
  
  vector<string> filters;
  
  _lister->getFilterNames(filters);

  int i = 1;
  for (vector<string>::const_iterator iter = filters.begin()+1;
       iter != filters.end();
       iter++, i++) {
    RFilter *filter = _lister->findFilter(i);
	
    char *s[] = {""};
    int row = gtk_clist_append(GTK_CLIST(_filterL), s);
    gtk_clist_set_text(GTK_CLIST(_filterL), row, 0, (char*)(*iter).c_str());
    gtk_clist_set_row_data(GTK_CLIST(_filterL), row, filter);
  }
    
  gtk_clist_moveto(GTK_CLIST(_filterL), 0,0, 0.0, 0.0);
  gtk_clist_select_row(GTK_CLIST(_filterL), selected_row, 0);
  editFilterAction(_filterL, this);
    
  RGWindow::show();
}


void RGFilterManagerWindow::selectAction(GtkWidget *self, gint row,
 					 gint column, GdkEventButton *event,
 					 gpointer data)
{
  RGFilterManagerWindow *me = (RGFilterManagerWindow*)data;
  me->selected_row = row;
  
  me->editFilterAction((GtkWidget*)me, (void*)me);

}

void RGFilterManagerWindow::editFilterAction(GtkWidget *self, void *data)
{
    RGFilterManagerWindow *me = (RGFilterManagerWindow*)data;
    RFilter *filter;

    filter = (RFilter*)gtk_clist_get_row_data(GTK_CLIST(me->_filterL), 
					      me->selected_row);
    if(filter) {
      gtk_entry_set_text(GTK_ENTRY(me->_nameT), (char*)filter->getName().c_str());
      me->_editor->editFilter(filter);
    }
}




void RGFilterManagerWindow::newFilterAction(GtkWidget *self, void *data)
{
    RGFilterManagerWindow *me = (RGFilterManagerWindow*)data;
    RFilter *filter;
    
    filter = new RFilter(me->_lister);
    
    filter->setName(_("A New Filter"));
    me->_lister->registerFilter(filter);

    char *s[] = {""};
    int row = gtk_clist_append(GTK_CLIST(me->_filterL), s);
    gtk_clist_set_text(GTK_CLIST(me->_filterL), row, 0, 
		       (char*)filter->getName().c_str());
    gtk_clist_set_row_data(GTK_CLIST(me->_filterL), row, filter);
    gtk_clist_moveto(GTK_CLIST(me->_filterL), row, 0,0,0);
    gtk_clist_select_row(GTK_CLIST(me->_filterL), row, 0);
    editFilterAction(NULL, me);
}



void RGFilterManagerWindow::applyFilterAction(GtkWidget *self, void *data)
{
    RGFilterManagerWindow *me = (RGFilterManagerWindow*)data;
    RFilter *filter;

    filter = (RFilter*)gtk_clist_get_row_data(GTK_CLIST(me->_filterL), 
					      me->selected_row);
    if(filter == NULL)
      return;

    filter->setName(string(gtk_entry_get_text(GTK_ENTRY(me->_nameT))));
    me->_editor->applyChanges();
    gtk_clist_set_text(GTK_CLIST(me->_filterL), me->selected_row, 0, 
		       (char*)filter->getName().c_str());
}



void RGFilterManagerWindow::deleteFilterAction(GtkWidget *self, void *data)
{

    RGFilterManagerWindow *me = (RGFilterManagerWindow*)data;
    RFilter *filter;

    filter = (RFilter*)gtk_clist_get_row_data(GTK_CLIST(me->_filterL), 
					      me->selected_row);
    if(filter) {
	me->_lister->unregisterFilter(filter);
	delete filter;
	gtk_clist_remove(GTK_CLIST(me->_filterL), me->selected_row);
	// needed, otherwise it will segfault on "save"
	gtk_clist_select_row(GTK_CLIST(me->_filterL), me->selected_row-1, 0);
    }    
}

void RGFilterManagerWindow::close()
{
  if(_closeAction)
    _closeAction(_closeData, this);

  RGWindow::close();
}

void RGFilterManagerWindow::closeAction(GtkWidget *self, void *data)
{
  RGFilterManagerWindow *me = (RGFilterManagerWindow*)data;

  me->close();
}


void RGFilterManagerWindow::clearFilterAction(GtkWidget *self, void *data)
{
    RGFilterManagerWindow *me = (RGFilterManagerWindow*)data;
    
    me->_editor->resetFilter();
}



void RGFilterManagerWindow::setCloseCallback(RGFilterEditorCloseAction *action,
					     void *data)
{
    _closeAction = action;
    _closeData = data;
}


