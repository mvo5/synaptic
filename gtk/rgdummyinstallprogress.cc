/* rgdummyinstallprogress.cc
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

#include "rgdummyinstallprogress.h"
#include "rgutils.h"

#include <unistd.h>
#include <stdio.h>

#include "i18n.h"

void RGDummyInstallProgress::startUpdate()
{
   RGFlushInterface();
}


void RGDummyInstallProgress::finishUpdate()
{
   RGFlushInterface();
}

void RGDummyInstallProgress::updateInterface()
{
   if (gtk_events_pending()) {
      while (gtk_events_pending())
         gtk_main_iteration();
   } else {
      usleep(1000000);
   }
}
