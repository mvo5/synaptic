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

#include <WINGs/WINGs.h>

#include "rpackagelister.h"


class RWUserDialog : public RUserDialog {
   WMScreen *_scr;

public:

   WMWindow *_confirmDialogOwner;

   RWUserDialog(WMScreen *scr) : _scr(scr), _confirmDialogOwner(0) {};

   virtual bool confirm(const char *message) {
       return WMRunAlertPanel(_scr, _confirmDialogOwner, "Confirm", message,
			      "Yes", "No", NULL) == WAPRDefault;
   }

   virtual bool warning(const char *message) {
       return WMRunAlertPanel(_scr, _confirmDialogOwner, "Warning", message,
			      "Ok", NULL) == WAPRDefault;
   }

   virtual bool info(const char *message) {
       return WMRunAlertPanel(_scr, _confirmDialogOwner, "Info", message,
			      "Ok", NULL) == WAPRDefault;
   }

   virtual bool error(const char *message) {
       return WMRunAlertPanel(_scr, _confirmDialogOwner, "Error", message,
			      "Ok", NULL) == WAPRDefault;
   }

};
