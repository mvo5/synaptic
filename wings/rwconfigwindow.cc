/* rwconfigwindow.cc
 *
 * Copyright (c) 2000-2003 Conectiva S/A
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
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

#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>

#include "rconfiguration.h"

#include "rwconfigwindow.h"



void RWConfigWindow::saveAction(WMWidget *self, void *data)
{
    RWConfigWindow *me = (RWConfigWindow*)data;

    _config->Set("Synaptic::UseRegexp", 
		 WMGetButtonSelected(me->_optionB[0]) ? "true" : "false");

    bool newval = WMGetButtonSelected(me->_optionB[1]);
    
    _config->Set("Synaptic::TextButtons",
		 newval ? "true" : "false");

    bool postClean, postAutoClean;
    
    postClean = WMGetButtonSelected(me->_cacheB[1]) == True;
    postAutoClean = WMGetButtonSelected(me->_cacheB[2]) == True;

    _config->Set("Synaptic::CleanCache", postClean ? "true" : "false");
    _config->Set("Synaptic::AutoCleanCache", postAutoClean ? "true" : "false");
    
    if (!RWriteConfigFile(*_config)) {
	_error->DumpErrors();
	WMRunAlertPanel(WMWidgetScreen(self), me->_win,
			_("Error"), _("An error occurred while saving configurations."),
			_("OK"), NULL, NULL);
    }
}



void RWConfigWindow::show()
{
    string str;

    WMSetButtonSelected(_optionB[0], 
			_config->FindB("Synaptic::UseRegexp", false));

    WMSetButtonSelected(_optionB[1],
			_config->FindB("Synaptic::TextButtons", false));
    

    bool postClean = _config->FindB("Synaptic::CleanCache", false);
    bool postAutoClean = _config->FindB("Synaptic::AutoCleanCache", false);
    
    if (postClean)
	WMPerformButtonClick(_cacheB[1]);
    else if (postAutoClean)
	WMPerformButtonClick(_cacheB[2]);
    else
	WMPerformButtonClick(_cacheB[0]);

//    str = _config->FindDir("Dir::Cache::Archives");
//    WMSetTextFieldText(_pathT, (char*)str.c_str());

//    str = _config->Find("Synaptic::CacheSize");
//    if (str.empty())
//	str = "50";
    
//    WMSetTextFieldText(_pathT, (char*)str);

    RWWindow::show();
}




RWConfigWindow::RWConfigWindow(RWWindow *win) 
    : RWWindow(win, "options")
{
    WMButton *button;
    WMBox *box;
    int b = -1;

    WMSetBoxHorizontal(_topBox, False);
    WMResizeWidget(_win, 400, 250);

    WMTabView *tab;
    
    tab = WMCreateTabView(_topBox);
    WMMapWidget(tab);
    WMAddBoxSubview(_topBox, WMWidgetView(tab), True, True, 50, 0, 5);
    box = WMCreateBox(tab);
    WMSetBoxHorizontal(box, False);

    
    {
	WMSetBoxBorderWidth(box, 10);

	_optionB[++b] = WMCreateSwitchButton(box);
	WMSetButtonText(_optionB[b],
			_("Use regular expressions during searches or matching"));
	WMAddBoxSubview(box, WMWidgetView(_optionB[b]), False, True, 
			20, 0, 0);
	
	_optionB[++b] = WMCreateSwitchButton(box);
	WMSetButtonText(_optionB[b],
			_("Use text-only buttons in main window\n(requires a restart to take effect)"));
	WMAddBoxSubview(box, WMWidgetView(_optionB[b]), False, True, 
			40, 0, 0);

	WMMapSubwidgets(box);
	WMAddTabViewItemWithView(tab, WMWidgetView(box), 0, _("Options"));
    }

    {
	WMBox *vbox = WMCreateBox(tab);
	WMSetBoxBorderWidth(vbox, 10);
/*
        WMLabel *label;
	label = WMCreateLabel(vbox);
	WMSetLabelText(label, 
		       _("Packages are stored in the cache when downloaded."));
 * 
	WMAddBoxSubview(vbox, WMWidgetView(label), False, True, 40, 0, 5);
 */
	_cacheB[0] = button = WMCreateRadioButton(vbox);
	WMSetButtonText(button, _("Leave downloaded packages in the cache"));
	WMAddBoxSubview(vbox, WMWidgetView(button), False, True, 20, 0, 0);

	_cacheB[1] = button = WMCreateRadioButton(vbox);
	WMSetButtonText(button, _("Delete downloaded packages after installation"));
	WMAddBoxSubview(vbox, WMWidgetView(button), False, True, 20, 0, 0);

	_cacheB[2] = button = WMCreateRadioButton(vbox);
	WMSetButtonText(button, _("Delete obsoleted packages from cache"));
	WMAddBoxSubview(vbox, WMWidgetView(button), False, True, 20, 0, 10);

	WMGroupButtons(_cacheB[0], _cacheB[1]);
	WMGroupButtons(_cacheB[0], _cacheB[2]);
	
/*	box = WMCreateBox(vbox);
	WMSetBoxHorizontal(box, True);	
	WMAddBoxSubviewAtEnd(vbox, WMWidgetView(box), False, True, 20, 0, 0);

	label = WMCreateLabel(box);
	WMSetLabelText(label, _("Max. Cache Size:"));
	WMAddBoxSubview(box, WMWidgetView(label), False, True, 120, 0, 0);
	
	_sizeT = WMCreateTextField(box);
	WMAddBoxSubview(box, WMWidgetView(_sizeT), True, True, 80, 0, 2);
	
	label = WMCreateLabel(box);
	WMSetLabelText(label, _("Mbytes"));
	WMAddBoxSubview(box, WMWidgetView(label), False, True, 50, 0, 5);

	button = WMCreateCommandButton(box);
	WMSetButtonText(button, _("Clear Cache"));
	WMAddBoxSubview(box, WMWidgetView(button), False, True, 100, 0, 0);
	WMMapSubwidgets(box);

	
	box = WMCreateBox(vbox);
	WMSetBoxHorizontal(box, True);	
	WMAddBoxSubviewAtEnd(vbox, WMWidgetView(box), False, True, 20, 0, 5);

	label = WMCreateLabel(box);
	WMSetLabelText(label, _("Cache Directory:"));
	WMAddBoxSubview(box, WMWidgetView(label), False, True, 120, 0, 0);
	
	_pathT = WMCreateTextField(box);
	WMAddBoxSubview(box, WMWidgetView(_pathT), True, True, 80, 0, 5);
	
	button = WMCreateCommandButton(box);
	WMSetButtonText(button, _("Browse..."));
	WMAddBoxSubview(box, WMWidgetView(button), False, True, 70, 0, 0);

  */  
	WMMapSubwidgets(vbox);
	WMAddTabViewItemWithView(tab, WMWidgetView(vbox), 1, _("Cache"));
    }

    
    box = WMCreateBox(_topBox);
    WMAddBoxSubview(_topBox, WMWidgetView(box), False, True, 24, 0, 0);
    WMSetBoxHorizontal(box, True);
    
    button = WMCreateCommandButton(box);
    WMSetButtonText(button, _("Close"));
    WMSetButtonAction(button, windowCloseAction, this);
    WMAddBoxSubviewAtEnd(box, WMWidgetView(button), False, True, 80, 0, 0);

    button = WMCreateCommandButton(box);
    WMSetButtonText(button, _("Save"));
    WMSetButtonAction(button, saveAction, this);
    WMAddBoxSubviewAtEnd(box, WMWidgetView(button), False, True, 80, 0, 5);

    WMMapSubwidgets(box);
    WMMapWidget(box);


    setTitle(_("Preferences"));

    WMRealizeWidget(_win);
}


