/* rwcacheprogress.cc
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
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

#include "rwcacheprogress.h"

extern void RWFlushInterface();


RWCacheProgress::RWCacheProgress(WMBox *parent, WMLabel *label) 
    : _parent(parent), _label(label)
{
    _prog = WMCreateProgressIndicator(parent);

    WMSetLabelText(label, (char*)Op.c_str());

    _mapped = false;
}


RWCacheProgress::~RWCacheProgress()
{
    WMDestroyWidget(_prog);
}


void RWCacheProgress::Update()
{
    if (!CheckChange()) {
	RWFlushInterface();
	return;
    }
    
    if (!_mapped) {
	WMMapWidget(_prog);
	WMAddBoxSubview(_parent, WMWidgetView(_prog), True, True, 100, 0, 0);
	_mapped = true;
    }

    if (MajorChange)
	WMSetLabelText(_label, (char*)Op.c_str());

    WMSetProgressIndicatorValue(_prog, (int)Percent);

    RWFlushInterface();
    
}


void RWCacheProgress::Done()
{
    WMSetProgressIndicatorValue(_prog, 100);
    RWFlushInterface();

    WMUnmapWidget(_prog);
    WMRemoveBoxSubview(_parent, WMWidgetView(_prog));

    _mapped = false;
}
