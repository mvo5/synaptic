/* rwfiltermanager.cc
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

#include "rwfiltereditor.h"
#include "rwfiltermanager.h"



RWFilterManagerWindow::RWFilterManagerWindow(RWWindow *win, 
					     RPackageLister *lister)
    : RWWindow(win, "filterManager"), _lister(lister)
{
    WMBox *box;
    WMBox *vbox;
    WMBox *hbox;


    WMSetWindowCloseAction(_win, closeAction, this);

    setTitle(_("Package Filters"));

    WMSetWindowMinSize(_win, 400, 240);
    WMResizeWidget(_win, 500, 250);

    WMSetBoxHorizontal(_topBox, False);
    WMSetBoxBorderWidth(_topBox, 5);
    
    box = WMCreateBox(_topBox);
    WMMapWidget(box);
    WMSetBoxHorizontal(box, True);
    WMAddBoxSubview(_topBox, WMWidgetView(box), True, True, 80, 0, 5);
    
    vbox = WMCreateBox(box);
    WMMapWidget(vbox);
    WMSetBoxHorizontal(vbox, False);
    WMAddBoxSubview(box, WMWidgetView(vbox), False, True, 160, 0, 10);


    _nameT = WMCreateTextField(vbox);
    WMMapWidget(_nameT);
    WMAddBoxSubview(vbox, WMWidgetView(_nameT), False, True, 20, 0, 5);
    
    _filterL = WMCreateList(vbox);
    WMMapWidget(_filterL);
    WMSetListAction(_filterL, editFilterAction, this);
    WMAddBoxSubview(vbox, WMWidgetView(_filterL), True, True, 120, 0, 0);
    
    
    hbox = WMCreateBox(_topBox);
    WMMapWidget(hbox);
    WMSetBoxHorizontal(hbox, True);
    WMAddBoxSubview(_topBox, WMWidgetView(hbox), False, True, 20, 0, 0);

    for (int i = 0; i < 5; i++) {
	WMButton *button;
	
	button = WMCreateCommandButton(hbox);
	switch (i) {
	 case 0:
	    _newB = button;
	    WMSetButtonText(button, _("New"));
	    WMSetButtonAction(button, newFilterAction, this);
	    break;
	 case 1:
	    WMSetButtonText(button, _("Delete"));
	    WMSetButtonAction(button, deleteFilterAction, this);
	    break;
	 case 2:
	    WMSetButtonText(button, _("Clear"));
	    WMSetButtonAction(button, clearFilterAction, this);
	    break;
	 case 3:
	    WMSetButtonText(button, _("Save"));
	    WMSetButtonAction(button, applyFilterAction, this);
	    break;
	 case 4:
	    WMSetButtonText(button, _("Close"));
	    WMSetButtonAction(button, closeAction, this);
	    break;
	}
	if (i < 4)
	    WMAddBoxSubview(hbox, WMWidgetView(button), False, True, 80, 0, 5);
	else
	    WMAddBoxSubviewAtEnd(hbox, WMWidgetView(button), False, True, 80, 0, 5);
    }
    WMMapSubwidgets(hbox);
    
    _editor = new RWFilterEditor(box);
    WMMapWidget(_editor->widget());
    WMAddBoxSubview(box, WMWidgetView(_editor->widget()), True, True,
		    100, 0, 0);
    
    {
        vector<string> tmp;
	tmp = _lister->getSections();
	_editor->setPackageSections(tmp);
    }
    
    WMMapWidget(_topBox);
    
    WMRealizeWidget(_win);
}


RWFilterManagerWindow::~RWFilterManagerWindow()
{
    delete _editor;
}


void RWFilterManagerWindow::show()
{
    WMClearList(_filterL);
    
    vector<string> filters;
    
    filters = _lister->getFilterNames();
    
    int i = 1;
    for (vector<string>::const_iterator iter = filters.begin()+1;
	 iter != filters.end();
	 iter++, i++) {
	WMListItem *item;
	RFilter *filter = _lister->findFilter(i);
	
	item = WMAddListItem(_filterL, (char*)(*iter).c_str());
	
	item->clientData = filter;
    }
    
    WMSelectListItem(_filterL, 0);
    editFilterAction(_filterL, this);
    
    RWWindow::show();
}






void RWFilterManagerWindow::newFilterAction(WMWidget *self, void *data)
{
    RWFilterManagerWindow *me = (RWFilterManagerWindow*)data;
    RFilter *filter;
    
    filter = new RFilter(me->_lister);
    
    filter->setName(_("A New Filter"));
    me->_lister->registerFilter(filter);

    WMListItem *item = WMAddListItem(me->_filterL,
				     (char*)filter->getName().c_str());
    item->clientData = filter;
    WMSelectListItem(me->_filterL, WMGetListNumberOfRows(me->_filterL)-1);
    editFilterAction(NULL, me);
}


void RWFilterManagerWindow::editFilterAction(WMWidget *self, void *data)
{
    RWFilterManagerWindow *me = (RWFilterManagerWindow*)data;
    //RFilter *filter;

    WMListItem *item = WMGetListSelectedItem(me->_filterL);
    if (item) {
	RFilter *filter = (RFilter*)item->clientData;

	WMSetTextFieldText(me->_nameT, (char*)filter->getName().c_str());

	me->_editor->editFilter(filter);
    }
}


void RWFilterManagerWindow::applyFilterAction(WMWidget *self, void *data)
{
    RWFilterManagerWindow *me = (RWFilterManagerWindow*)data;
    //RFilter *filter;

    WMListItem *item = WMGetListSelectedItem(me->_filterL);
    if (item) {
	RFilter *filter = (RFilter*)item->clientData;

	filter->setName(string(WMGetTextFieldText(me->_nameT)));

	me->_editor->applyChanges();

	wfree(item->text);
	item->text = wstrdup((char*)filter->getName().c_str());
	WMRedisplayWidget(me->_filterL);
    }
}



void RWFilterManagerWindow::deleteFilterAction(WMWidget *self, void *data)
{
    RWFilterManagerWindow *me = (RWFilterManagerWindow*)data;
    //RFilter *filter;

    WMListItem *item = WMGetListSelectedItem(me->_filterL);
    if (item) {
	RFilter *filter = (RFilter*)item->clientData;
	int row;

	me->_lister->unregisterFilter(filter);
	
	delete filter;
	
	row = WMGetListSelectedItemRow(me->_filterL);
	
	WMRemoveListItem(me->_filterL, row);
	
	if (row >= WMGetListNumberOfRows(me->_filterL))
	    row--;
	
	if (row < 0) {
	    WMSetTextFieldText(me->_nameT, "");
	    clearFilterAction(NULL, me);
	} else {
	    WMSelectListItem(me->_filterL, row);
	    me->editFilterAction(me->_filterL, me);
	}
    }
}


void RWFilterManagerWindow::closeAction(WMWidget *self, void *data)
{
    RWFilterManagerWindow *me = (RWFilterManagerWindow*)data;

    me->_closeAction(me->_closeData, me);
    
    me->close();
}


void RWFilterManagerWindow::clearFilterAction(WMWidget *self, void *data)
{
    RWFilterManagerWindow *me = (RWFilterManagerWindow*)data;
    
    me->_editor->resetFilter();
}



void RWFilterManagerWindow::setCloseCallback(RWFilterEditorCloseAction *action,
					     void *data)
{
    _closeAction = action;
    _closeData = data;
}

