/* rgaboutpanel.h
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


#ifndef _RGABOUTPANEL_H_
#define _RGABOUTPANEL_H_

#include "rggtkbuilderwindow.h"

class RGCreditsPanel:public RGGtkBuilderWindow {
 public:
   RGCreditsPanel(RGWindow *parent);
   virtual ~RGCreditsPanel() {
   };
};

class RGAboutPanel:public RGGtkBuilderWindow {
   static void creditsClicked(GtkWidget *self, void *data);
   RGCreditsPanel *credits;
 public:
   RGAboutPanel(RGWindow *parent);
   virtual ~ RGAboutPanel() {
   };
};


#endif
