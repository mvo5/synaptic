/* rgfilterwindow.cc
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

#include "rgfilterwindow.h"
#include "rgfiltereditor.h"



enum {
    CNExclude,
    CNPattern,
    CNWhere
};

void RGFilterWindow::close()
{
  //  cout << "RGFilterWindow::close()" << endl;
  _closeAction(_closeObject, this);
}

void RGFilterWindow::windowCloseAction(GtkWidget *win, void *data)
{
    RGFilterWindow *rwin = (RGFilterWindow*)data;
    //cout << "windowCloseAction(GtkWidget *win, void *data) "<< endl;

    rwin->_closeAction(rwin->_closeObject, rwin);
}


void RGFilterWindow::clearFilterAction(GtkWidget *self, void *data)
{
    RGFilterWindow *rwin = (RGFilterWindow*)data;
    
    rwin->_editor->resetFilter();
}


void RGFilterWindow::saveFilterAction(GtkWidget *self, void *data)
{
    RGFilterWindow *rwin = (RGFilterWindow*)data;

    rwin->_editor->applyChanges();
    
    rwin->_saveAction(rwin->_saveObject, rwin);
}
								  


RGFilterWindow::RGFilterWindow(RGWindow *win, RPackageLister *lister)
    : RGWindow(win, "filter"), _lister(lister)
{
    GtkWidget *hbox;
    GtkWidget *button;

    gtk_signal_connect(GTK_OBJECT(_win), "delete_event",
		       GTK_SIGNAL_FUNC(windowDeleteAction), this);
    
    //gtk_widget_set_usize(_win, 350, 280);
    
    _editor = new RGFilterEditor(_topBox);

    {
	vector<string> tmp;
	_lister->getSections(tmp);
	_editor->setPackageSections(tmp);
    }
    gtk_widget_show(_editor->widget());
    gtk_box_pack_start(GTK_BOX(_topBox), _editor->widget(), TRUE, TRUE, 5);

    // bottom buttons
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(_topBox), hbox, FALSE, TRUE, 0);

    button = gtk_button_new_with_label(_("Set"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)saveFilterAction, this);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
    
    button = gtk_button_new_with_label(_("Clear"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)clearFilterAction, this);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
    
    button = gtk_button_new_with_label(_("Cancel"));
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)windowCloseAction, this);

    gtk_widget_show_all(hbox);
    gtk_widget_show(hbox);    
    
    gtk_widget_show(_topBox);
}

gint RGFilterWindow::windowDeleteAction( GtkWidget *widget,
                   GdkEvent  *event,
                   gpointer   data )
{
  RGFilterWindow *me = (RGFilterWindow*)data;
  me->windowCloseAction(widget, data);

  return(TRUE);
}



RGFilterWindow::~RGFilterWindow()
{
  delete _editor;
}



void RGFilterWindow::editFilter(RFilter *filter)
{
    setTitle(_("Package Listing Filter  -  ") + filter->getName());
   
    _editor->editFilter(filter);

    show();
}


void RGFilterWindow::setCloseCallback(RGFilterWindowCloseAction *action, 
				      void *data)
{
    _closeAction = action;
    _closeObject = data;
}


void RGFilterWindow::setSaveCallback(RGFilterWindowSaveAction *action, 
				     void *data)
{
    _saveAction = action;
    _saveObject = data;
}


