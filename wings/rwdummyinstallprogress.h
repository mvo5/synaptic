/* rwdummyinstallprogress.h
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


#ifndef _RWDUMMYINSTALLPROGRESS_H_
#define _RWDUMMYINSTALLPROGRESS_H_


#include "rinstallprogress.h"
#include "rwwindow.h"



class RWDummyInstallProgress : public RInstallProgress {
   WMScreen *_scr;

protected:
   virtual void startUpdate();
   virtual void updateInterface();
   virtual void finishUpdate();

public:
   RWDummyInstallProgress(WMScreen *scr) : RInstallProgress(), _scr(scr) {};
};

#endif
