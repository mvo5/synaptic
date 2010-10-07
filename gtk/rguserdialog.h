/* rguserdialog.h
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2003 Michael Vogt
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

#ifndef RGUSERDIALOG_H
#define RGUSERDIALOG_H

#include "ruserdialog.h"
#include "rgwindow.h"

class RGUserDialog : public RUserDialog
{
protected:

    GtkWidget *_parentWindow;

public:

    RGUserDialog() : _parentWindow(0) {};
    RGUserDialog(RGWindow *parent) : _parentWindow(parent->window()) {};
    RGUserDialog(GtkWidget *parent) : _parentWindow(parent) {};
    
    virtual bool showErrors();

    virtual bool message(const char *msg,
	    RUserDialog::DialogType dialog=RUserDialog::DialogInfo,
	    RUserDialog::ButtonsType buttons=RUserDialog::ButtonsOk,
	    bool defres=true);

};


/*
 * A alternative interface for the ruserdialog, here is how it works:
 * the gtkbuilder file must called "dialog_$NAME.ui"
 * the buttons must have valid RESPONSE_IDs attached
 * see gtkbuilder/dialog_quit.ui as an example
 * Example:
  	RGGtkBuilderUserDialog dia(this);
	// return true is user clicked on a button with GTK_RESPONSE_OK
	if(dia->run("quit")) {
	    do_response_ok_stuff();
	} else {
            do_not_ok_stuff();
        }
 *
 * 
 * if you need more complex interaction, use the getGtkBuilder() call to ask
 * for specific widgets in the dialog (see gtkbuilder/dialog_upgrade.ui as
 * example.
*/
class RGGtkBuilderUserDialog : public RGUserDialog
{
 protected:
    GtkWidget *_dialog;
    GtkResponseType res;
    GtkBuilder *builder;
    bool init(const char *name);

 public:
    RGGtkBuilderUserDialog(RGWindow* parent) : builder(0) {};
    RGGtkBuilderUserDialog(RGWindow* parent, const char *name);
    virtual ~RGGtkBuilderUserDialog()  { gtk_widget_destroy(_dialog); };

    void setTitle(string title) { 
       gtk_window_set_title(GTK_WINDOW(_dialog),title.c_str());
    }

    int run(const char *name=NULL, bool return_gtk_response=false);
    GtkBuilder *getGtkBuilder() { return builder; };
};
#endif

// vim:sts=4:sw=4
