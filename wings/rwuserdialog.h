/* rwuserdialog.h
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

#ifndef RWUSERDIALOG_H
#define RWUSERDIALOG_H

#include <WINGs/WINGs.h>

#include "ruserdialog.h"
#include "rwwindow.h"

class RWUserDialog : public RUserDialog
{

protected:

    WMScreen *_scr;
    WMWindow *_parentWindow;

public:

    RWUserDialog(WMScreen *scr) : _scr(scr), _parentWindow(0) {};

    virtual bool message(const char *msg,
	    RUserDialog::DialogType dialog=RUserDialog::DialogInfo,
	    RUserDialog::ButtonsType buttons=RUserDialog::ButtonsOk,
	    bool defres=true);
};

#endif

// vim:sts=4:sw=4
