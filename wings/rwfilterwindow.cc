/* rwfilterwindow.cc
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

#include "rwfilterwindow.h"
#include "rwfiltereditor.h"

#include <WINGs/wtabledelegates.h>



enum {
    CNExclude,
	CNPattern,
	CNWhere
};


void RWFilterWindow::windowCloseAction(WMWidget *win, void *data)
{
    RWFilterWindow *rwin = (RWFilterWindow*)data;

    rwin->_closeAction(rwin->_closeObject, rwin);
}


void RWFilterWindow::clearFilterAction(WMWidget *self, void *data)
{
    RWFilterWindow *rwin = (RWFilterWindow*)data;
    
    rwin->_editor->resetFilter();
}


void RWFilterWindow::saveFilterAction(WMWidget *self, void *data)
{
    RWFilterWindow *rwin = (RWFilterWindow*)data;

    rwin->_editor->applyChanges();
    
    rwin->_saveAction(rwin->_saveObject, rwin);
}
								  


RWFilterWindow::RWFilterWindow(RWWindow *win, RPackageLister *lister)
    : RWWindow(win, "filter"), _lister(lister)
{
    WMBox *hbox;
    WMButton *button;


    WMSetWindowCloseAction(_win, windowCloseAction, this);
    
    WMSetWindowMinSize(_win, 320, 240);

    WMResizeWidget(_win, 350, 240);

    
    _editor = new RWFilterEditor(_topBox);

    {
	vector<string> tmp;
	tmp = _lister->getSections();
	_editor->setPackageSections(tmp);
    }
    WMMapWidget(_editor->widget());
    WMAddBoxSubview(_topBox, WMWidgetView(_editor->widget()), True, True,
		    200, 0, 5);

    // bottom buttons
    hbox = WMCreateBox(_topBox);
    WMSetBoxHorizontal(hbox, True);
    WMAddBoxSubview(_topBox, WMWidgetView(hbox), False, True, 22, 0, 0);

    button = WMCreateCommandButton(hbox);
    WMSetButtonText(button, _("Cancel"));
    WMAddBoxSubview(hbox, WMWidgetView(button), True, True, 80, 0, 5);
    WMSetButtonAction(button, windowCloseAction, this);

    button = WMCreateCommandButton(hbox);
    WMSetButtonText(button, _("Clear"));
    WMSetButtonAction(button, clearFilterAction, this);
    WMAddBoxSubview(hbox, WMWidgetView(button), True, True, 80, 0, 5);
    
    button = WMCreateCommandButton(hbox);
    WMSetButtonText(button, _("Set"));
    WMSetButtonAction(button, saveFilterAction, this);
    WMSetButtonImagePosition(button, WIPRight);
    WMSetButtonImage(button, WMGetSystemPixmap(WMWidgetScreen(button),
					    WSIReturnArrow));
    WMSetButtonAltImage(button, WMGetSystemPixmap(WMWidgetScreen(button),
					       WSIHighlightedReturnArrow));
    WMAddBoxSubview(hbox, WMWidgetView(button), True, True, 80, 0, 0);
    
    WMMapSubwidgets(hbox);
    WMMapWidget(hbox);    
    
    WMMapWidget(_topBox);
    
    WMRealizeWidget(_win);
}


RWFilterWindow::~RWFilterWindow()
{
    delete _editor;
}



void RWFilterWindow::editFilter(RFilter *filter)
{
    setTitle(_("Package Listing Filter  -  ") + filter->getName());
   
    _editor->editFilter(filter);

    show();
}


void RWFilterWindow::setCloseCallback(RWFilterWindowCloseAction *action, 
				      void *data)
{
    _closeAction = action;
    _closeObject = data;
}


void RWFilterWindow::setSaveCallback(RWFilterWindowSaveAction *action, 
				     void *data)
{
    _saveAction = action;
    _saveObject = data;
}


