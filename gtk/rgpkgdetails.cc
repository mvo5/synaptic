/* rgpkgdetails.cc - show details of a pkg
 * 
 * Copyright (c) 2004 Michael Vogt
 *
 * Author: Michael Vogt <mvo@debian.org>
 *         Gustavo Niemeyer <niemeyer@conectiva.com>
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

#include "rgwindow.h"
#include "rgmainwindow.h"
#include "rgpkgdetails.h"
#include "rggladewindow.h"
#include "rpackage.h"

RGPkgDetailsWindow::RGPkgDetailsWindow(RGWindow *parent, RPackage *pkg)
   : RGGladeWindow(parent, "details")
{

   glade_xml_signal_connect_data(_gladeXML,
				 "on_button_close_clicked",
				 G_CALLBACK(cbCloseClicked),
				 this); 

   fillInValues(pkg);

}

void RGPkgDetailsWindow::cbCloseClicked(GtkWidget *self, void *data)
{
   RGPkgDetailsWindow *me = static_cast<RGPkgDetailsWindow*>(data);

   gtk_widget_destroy(me->_win); 

   // XXX destroy class itself
}

bool RGPkgDetailsWindow::setLabel(const char *widget_name, const char *value)
{
   GtkWidget *widget = glade_xml_get_widget(_gladeXML, widget_name);
   if (widget == NULL)
      cout << "widget == NULL with: " << widget_name << endl;

   if (!value)
      value = _("N/A");
   gtk_label_set_label(GTK_LABEL(widget), utf8(value));
}

bool RGPkgDetailsWindow::setLabel(const char *widget_name, const long value)
{
   string strVal;
   GtkWidget *widget = glade_xml_get_widget(_gladeXML, widget_name);
   if (widget == NULL)
      cout << "widget == NULL with: " << widget_name << endl;

   // we can never have values of zero or less
   if (value <= 0)
      strVal = _("N/A");
   else
      strVal = SizeToStr(value);
   gtk_label_set_label(GTK_LABEL(widget), utf8(strVal.c_str()));
}

bool RGPkgDetailsWindow::setTextView(const char *widget_name, 
				     const char* value, 
				     bool use_headline)
{
   GtkTextIter start,end;

   GtkWidget *view = glade_xml_get_widget(_gladeXML, widget_name);
   if (view == NULL)
      cout << "textview == NULL with: " << widget_name << endl;

   GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
   gtk_text_buffer_set_text (buffer, utf8(value), -1);

   if(use_headline) {
      GtkTextTag *bold_tag = gtk_text_buffer_create_tag(buffer, "bold", 
					       "weight", PANGO_WEIGHT_BOLD,
					       "scale", 1.1, 
					       NULL);
      gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);
      
      gtk_text_buffer_get_iter_at_offset(buffer, &end, 0);
      gtk_text_iter_forward_line(&end);

      gtk_text_buffer_apply_tag(buffer, bold_tag, &start, &end);
   }

   gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);
   gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(view), &start,0,FALSE,0,0);
}

bool RGPkgDetailsWindow::setPixmap(const char *widget_name, GdkPixbuf *value)
{
   GtkWidget *pix = glade_xml_get_widget(_gladeXML, widget_name);
   if (pix == NULL)
      cout << "textview == NULL with: " << widget_name << endl;
   
   gtk_image_set_from_pixbuf(GTK_IMAGE(pix), value);

}

void RGPkgDetailsWindow::fillInValues(RPackage *pkg)
{
   char *pkg_summary = g_strdup_printf("%s\n%s",
					     pkg->name(), pkg->summary());
   setTextView("textview_pkgcommon", pkg_summary, true);
   g_free(pkg_summary);

   setLabel("label_maintainer", pkg->maintainer());
   setPixmap("image_state", RGPackageStatus::pkgStatus.getPixbuf(pkg));
   setLabel("label_state", RGPackageStatus::pkgStatus.getLongStatusString(pkg));
   setLabel("label_priority", pkg->priority());
   setLabel("label_section", pkg->section());

   setLabel("label_installed_version", pkg->installedVersion());
   setLabel("label_installed_size", pkg->installedSize());

   setLabel("label_latest_version", pkg->availableVersion());
   setLabel("label_latest_size", pkg->availableInstalledSize());
   setLabel("label_latest_download_size", pkg->availablePackageSize());
	       
   setTextView("text_descr", pkg->description());

}


RGPkgDetailsWindow::~RGPkgDetailsWindow()
{

}
