/* rwfiltereditor.cc
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

#include <assert.h>

#include "config.h"
#include "i18n.h"


#include "rpackagelister.h"

#include "rwfiltereditor.h"

#include <WINGs/wtabledelegates.h>



enum {
    CNExclude,
	CNPattern,
	CNWhere
};


// must be in the same order as of the check buttons
static RStatusPackageFilter::Types StatusMasks[] =  {
	RStatusPackageFilter::NotInstalled,
	RStatusPackageFilter::Upgradable,
	RStatusPackageFilter::Installed,
	RStatusPackageFilter::MarkKeep,
	RStatusPackageFilter::MarkInstall,
	RStatusPackageFilter::MarkRemove,
	RStatusPackageFilter::Broken
};




int RWFilterEditor::numberOfRows(WMTableViewDelegate *self,
				 WMTableView *table)
{
    RWFilterEditor *fwin = (RWFilterEditor*)self->data;

    return fwin->_patternL.size();
}


void *RWFilterEditor::valueForCell(WMTableViewDelegate *self, 
				   WMTableColumn *column, int row)
{
    RWFilterEditor *fwin = (RWFilterEditor*)self->data;
    
    switch ((int)WMGetTableColumnId(column)) {
     case CNExclude:
	return (void*)fwin->_patternL[row].exclude;
	
     case CNPattern:
	return (void*)fwin->_patternL[row].pattern.c_str();

     case CNWhere:
	return (void*)fwin->_patternL[row].type;

     default:
	return (void*)"???";
    }
}


void RWFilterEditor::setValueForCell(WMTableViewDelegate *self, 
				     WMTableColumn *column, int row, 
				     void *data)
{
    RWFilterEditor *fwin = (RWFilterEditor*)self->data;
    
    switch ((int)WMGetTableColumnId(column)) {
     case CNExclude:
	fwin->_patternL[row].exclude = data != NULL;
	break;
	
     case CNPattern:
	if (data)
	    fwin->_patternL[row].pattern = string((char*)data);
	else
	    fwin->_patternL[row].pattern = "";
	break;
	
     case CNWhere:
	fwin->_patternL[row].type = (int)data;
	break;
    }
}


RWFilterEditor::RWFilterEditor(WMWidget *parent)
{
    _tabview = WMCreateTabView(parent);
    WMMapWidget(_tabview);

    makeSectionFilterPanel(_tabview);
    
    makeStatusFilterPanel(_tabview);
    
    makePatternFilterPanel(_tabview);
}


RWFilterEditor::~RWFilterEditor()
{
    delete _tableDelegate;
}


void RWFilterEditor::resetFilter()
{
    WMUnselectAllListItems(_groupL);

    WMPerformButtonClick(_exclGB);

    for (int i = 0; i < 7; i++) {
	WMSetButtonSelected(_statusB[i], True);
    }
}


void RWFilterEditor::editFilter(RFilter *filter)
{
    resetFilter();

    _filter = filter;
    
    setSectionFilter(filter->section);

    setStatusFilter(filter->status);
    
    setPatternFilter(filter->pattern);
}


void RWFilterEditor::applyChanges()
{
    getSectionFilter(_filter->section);

    getStatusFilter(_filter->status);
    
    getPatternFilter(_filter->pattern);
}



void RWFilterEditor::setSectionFilter(RSectionPackageFilter &f)
{    
    string section;
    int row;
    
    WMUnselectAllListItems(_groupL);
    
    for (int i = f.count()-1; i >= 0; i--) {
	section = f.section(i);
	
	row = WMFindRowOfListItemWithTitle(_groupL, (char*)section.c_str());
	if (row >= 0) 
	    WMSelectListItem(_groupL, row);
    }

    if (f.inclusive()) {
	WMPerformButtonClick(_inclGB);
    } else {
	WMPerformButtonClick(_exclGB);
    }
}


void RWFilterEditor::getSectionFilter(RSectionPackageFilter &f)
{        
    WMArray *list;
    
    f.setInclusive(WMGetButtonSelected(_inclGB) == True);

    f.clear();
    
    list = WMGetListSelectedItems(_groupL);

    for (int i = 0; i < WMGetArrayItemCount(list); i++) {
	WMListItem *item = (WMListItem*)WMGetFromArray(list, i);
	f.addSection(string(item->text));
    }
}


void RWFilterEditor::setStatusFilter(RStatusPackageFilter &f)
{    
    int i;
    int type = f.status();

    for (i = 0; i < 7; i++) {
	WMSetButtonSelected(_statusB[i], 
			    (type & StatusMasks[i]) ? True : False);
    }
}


void RWFilterEditor::getStatusFilter(RStatusPackageFilter &f)
{
    int i;
    int type = 0;

    for (i = 0; i < 7; i++) {
	if (WMGetButtonSelected(_statusB[i]))
	    type |= StatusMasks[i];
    }
    
    f.setStatus(type);
}



void RWFilterEditor::setPatternFilter(RPatternPackageFilter &f)
{    
    int i;

    _patternL.erase(_patternL.begin(), _patternL.end());
		    
    for (i = 0; i < f.count(); i++) {
	PatternFilterItem item;
	RPatternPackageFilter::DepType type;
	
	f.getPattern(i, type, item.pattern, item.exclude);
	item.type = (int)type;
	
	_patternL.push_back(item);
    }
    WMNoteTableViewNumberOfRowsChanged(_patT);
}


void RWFilterEditor::getPatternFilter(RPatternPackageFilter &f)
{
    WMEditTableViewRow(_patT, -1);
    
    f.reset();

    for (vector<PatternFilterItem>::const_iterator iter = _patternL.begin();
	 iter != _patternL.end(); iter++) {

	f.addPattern((RPatternPackageFilter::DepType)iter->type,
		     iter->pattern, iter->exclude);
    }
}



void RWFilterEditor::makeSectionFilterPanel(WMTabView *tabview)
{
    WMBox *box = WMCreateBox(_tabview);

    WMSetBoxBorderWidth(box, 5);

    _groupL = WMCreateList(box);
    WMSetListAllowMultipleSelection(_groupL, True);
    WMSetListAllowEmptySelection(_groupL, True);
    WMAddBoxSubview(box, WMWidgetView(_groupL), True, True, 40, 0, 5);

    _inclGB = WMCreateRadioButton(box);
    WMSetButtonText(_inclGB, _("Include Only Selected Sections"));
    WMAddBoxSubview(box, WMWidgetView(_inclGB), False, True, 16, 0, 5);

    _exclGB = WMCreateRadioButton(box);
    WMSetButtonText(_exclGB, _("Exclude Selected Sections"));
    WMAddBoxSubview(box, WMWidgetView(_exclGB), False, True, 16, 0, 0);
    WMGroupButtons(_inclGB, _exclGB);

    WMMapSubwidgets(box);
    WMAddTabViewItemWithView(_tabview, WMWidgetView(box), 0, _("by Section"));
}



void RWFilterEditor::makeStatusFilterPanel(WMTabView *tabview)
{
    WMBox *box = WMCreateBox(_tabview);
    WMButton *but;
    WMLabel *label;
    int i = 0;

    WMSetBoxBorderWidth(box, 10);

    label = WMCreateLabel(box);
    WMSetLabelText(label, _("Include packages that are..."));
    WMAddBoxSubview(box, WMWidgetView(label), False, True, 15, 0, 5);

    but = WMCreateSwitchButton(box);
    WMSetButtonText(but, _("Not Installed"));
    WMAddBoxSubview(box, WMWidgetView(but), False, True, 16, 0, 5);
    _statusB[i++] = but;
    
    but = WMCreateSwitchButton(box);
    WMSetButtonText(but, _("Installed and Upgradable"));
    WMAddBoxSubview(box, WMWidgetView(but), False, True, 16, 0, 5);
    _statusB[i++] = but;
    
    but = WMCreateSwitchButton(box);
    WMSetButtonText(but, _("Installed"));
    WMAddBoxSubview(box, WMWidgetView(but), False, True, 16, 0, 5);
    _statusB[i++] = but;

    but = WMCreateSwitchButton(box);
    WMSetButtonText(but, _("Marked to Keep"));
    WMAddBoxSubview(box, WMWidgetView(but), False, True, 16, 0, 5);
    _statusB[i++] = but;

    but = WMCreateSwitchButton(box);
    WMSetButtonText(but, _("Marked to Install/Upgrade"));
    WMAddBoxSubview(box, WMWidgetView(but), False, True, 16, 0, 5);
    _statusB[i++] = but;

    but = WMCreateSwitchButton(box);
    WMSetButtonText(but, _("Marked to Remove"));
    WMAddBoxSubview(box, WMWidgetView(but), False, True, 16, 0, 5);
    _statusB[i++] = but;
    
    but = WMCreateSwitchButton(box);
    WMSetButtonText(but, _("Broken"));
    WMAddBoxSubview(box, WMWidgetView(but), False, True, 16, 0, 0);
    _statusB[i++] = but;
    
    
    WMMapSubwidgets(box);
    WMAddTabViewItemWithView(_tabview, WMWidgetView(box), 0, _("by Status"));
}


void RWFilterEditor::setPackageSections(vector<string> &sections)
{
    for (vector<string>::const_iterator iter = sections.begin();
	 iter != sections.end();
	 iter++) {
	WMAddListItem(_groupL, (char*)iter->c_str());
    }
    
    WMSortListItems(_groupL);
}



void RWFilterEditor::newPatternAction(WMWidget *w, void *data)
{
    RWFilterEditor *fwin = (RWFilterEditor*)data;
    PatternFilterItem item;

    item.type = 0;
    item.pattern = "";
    item.exclude = false;

    fwin->_patternL.push_back(item);

    WMNoteTableViewNumberOfRowsChanged(fwin->_patT);
    
    WMScrollTableViewRowToVisible(fwin->_patT, fwin->_patternL.size()-1);
}


void RWFilterEditor::removePatternAction(WMWidget *w, void *data)
{
    RWFilterEditor *fwin = (RWFilterEditor*)data;
    int row = WMGetTableViewClickedRow((WMTableView*)w);

    assert(row >= 0);
    fwin->_patternL.erase(fwin->_patternL.begin() + row);

    WMNoteTableViewNumberOfRowsChanged(fwin->_patT);
}


void RWFilterEditor::selectedPatternRow(WMWidget *w, void *data)
{
    RWFilterEditor *fwin = (RWFilterEditor*)data;
    int row = WMGetTableViewClickedRow((WMTableView*)w);

    if (row >= 0) {
	WMEditTableViewRow((WMTableView*)w, row);
	WMSetButtonEnabled(fwin->_removePB, True);
    } else {
	WMSetButtonEnabled(fwin->_removePB, False);
    }
}


void RWFilterEditor::makePatternFilterPanel(WMTabView *tabview)
{
    WMBox *box = WMCreateBox(_tabview);
    WMBox *hbox;
    WMButton *button;
    WMScreen *scr = WMWidgetScreen(tabview);
    WMTableColumn *col;
    static char *options[] = {
	N_("are Named"),
	N_("in Description"),
	N_("Depends on"), // depends, predepends etc
	N_("Provides"), // provides and name
	N_("Conflicts with"), // conflicts
	N_("Replaces"), // replaces/obsoletes
	N_("Suggests") // suggests/recommends
    };

    _tableDelegate = new WMTableViewDelegate;
    
    _tableDelegate->data = this;
    _tableDelegate->numberOfRows = numberOfRows;
    _tableDelegate->valueForCell = valueForCell;
    _tableDelegate->setValueForCell = setValueForCell;

    
    WMSetBoxBorderWidth(box, 5);

    _patT = WMCreateTableView(box);
    WMMapWidget(_patT);
//    WMSetTableViewDataSource(_patT, _lister);
    WMSetTableViewBackgroundColor(_patT, WMWhiteColor(scr));
    WMSetTableViewGridColor(_patT, WMGrayColor(scr));
    WMSetTableViewHeaderHeight(_patT, 18);
    WMSetTableViewRowHeight(_patT, 18);
    WMSetTableViewDelegate(_patT, _tableDelegate);
    WMSetTableViewAction(_patT, selectedPatternRow, this);    
    WMAddBoxSubview(box, WMWidgetView(_patT), True, True, 200, 0, 5);

    WMTableColumnDelegate *colDeleg;


    colDeleg = WTCreateBooleanSwitchDelegate(_patT);
    
    col = WMCreateTableColumn(_("Excl."));
    WMAddTableViewColumn(_patT, col);
    WMSetTableColumnWidth(col, 50);
    WMSetTableColumnDelegate(col, colDeleg);
    WMSetTableColumnId(col, (void*)CNExclude);

    for (unsigned int i = 0; i < sizeof(options)/sizeof(char*); i++) {
	options[i] = _(options[i]);
    }
    colDeleg = WTCreateEnumSelectorDelegate(_patT);
    WTSetEnumSelectorOptions(colDeleg, options,
			     2);//sizeof(options)/sizeof(char*));

    col = WMCreateTableColumn(_("Packages that"));
    WMSetTableColumnWidth(col, 100);
    WMAddTableViewColumn(_patT, col);	
    WMSetTableColumnDelegate(col, colDeleg);
    WMSetTableColumnId(col, (void*)CNWhere);
    
    
    colDeleg = WTCreateStringEditorDelegate(_patT);    
    col = WMCreateTableColumn(_("Pattern"));
    
    WMSetTableColumnWidth(col, 300);
    WMAddTableViewColumn(_patT, col);	
    WMSetTableColumnDelegate(col, colDeleg);
    WMSetTableColumnId(col, (void*)CNPattern);
    
    
    hbox = WMCreateBox(box);
    WMSetBoxHorizontal(hbox, True);
    WMAddBoxSubview(box, WMWidgetView(hbox), False, True, 20, 0, 0);

    button = WMCreateCommandButton(hbox);
    WMSetButtonText(button, _("New"));
    WMAddBoxSubviewAtEnd(hbox, WMWidgetView(button), False, True, 80, 0, 0);
    WMSetButtonAction(button, newPatternAction, this);
    
    _removePB = button = WMCreateCommandButton(hbox);
    WMSetButtonEnabled(_removePB, False);
    WMSetButtonText(button, _("Remove"));
    WMAddBoxSubviewAtEnd(hbox, WMWidgetView(button), False, True, 80, 0, 5);
    WMSetButtonAction(button, removePatternAction, this);
    

    WMMapSubwidgets(hbox);
    WMMapWidget(hbox);
    WMMapWidget(box);
    
    WMAddTabViewItemWithView(_tabview, WMWidgetView(box), 0, _("by Package"));
}

