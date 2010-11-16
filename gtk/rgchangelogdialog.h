/* rgchangelogdialog.h - main window of application
 * 
 * Copyright (c) 2010 Michael Vogt <mvo@ubuntu.com>
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

#include <rgwindow.h>
#include <rpackage.h>
#include <rgfetchprogress.h>
#include <rguserdialog.h>
#include <cassert>

void ShowChangelogDialog(RGWindow *me, RPackage *pkg);
