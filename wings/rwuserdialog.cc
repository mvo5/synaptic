/* rwuserdialog.cc
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
#include "rwuserdialog.h"


bool RWUserDialog::message(const char *msg,
		     RUserDialog::DialogType dialog,
		     RUserDialog::ButtonsType buttons,
		     bool defres)
{
    char *button1 = NULL;
    char *button2 = NULL;
    char *tmpbutton;
    char *title;
    int res;

    switch(dialog) {
	case DialogInfo:
	    title = _("Info");
	    button1 = _("Ok");
	    break;
	case DialogWarning:
	    title = _("Warning");
	    button1 = _("Ok");
	    break;
	case DialogError:
	    title = _("Error");
	    button1 = _("Ok");
	    break;
	case DialogQuestion:
	    title = _("Question");
	    button1 = _("Yes");
	    button2 = _("No");
	    break;
    }

    switch(buttons) {
	case ButtonsDefault:
	    break;
	case ButtonsOk:
	    button1 = _("Ok");
	    button2 = NULL;
	    break;
	case ButtonsOkCancel:
	    button1 = _("Ok");
	    button2 = _("Cancel");
	    break;
	case ButtonsYesNo:
	    button1 = _("Yes");
	    button2 = _("No");
	    break;
    }

    if (defres == false) {
	tmpbutton = button1;
	button1 = button2;
	button2 = button1;
    }

    res = WMRunAlertPanel(_scr, _parentWindow,
			  title, (char*) msg, button1, button2, NULL);
    return (defres  && res == WAPRDefault) ||
	   (!defres && res != WAPRDefault);
}

// vim:sts=4:sw=4
