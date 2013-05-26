/* gtk3compat.h
 *
 * Copyright (c) 2011 Michael Vogt
 *
 * Author: Michael Vogt <mvo@debian.org>
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
#include <gtk/gtk.h>

#include <gdk/gdkkeysyms-compat.h>

#if !GTK_CHECK_VERSION(2,6,0)

gchar *
gtk_combo_box_get_active_text (GtkComboBox *combo_box)
{
  GtkTreeIter iter;
  gchar *text = NULL;

  GtkTreeModel* model = gtk_combo_box_get_model(combo_box);

  if (gtk_combo_box_get_active_iter (combo_box, &iter))
    gtk_tree_model_get (model, &iter, 0, &text, -1);

  return text;
}
#endif

#if !GTK_CHECK_VERSION(3,0,0)
 #define GTK_COMBO_BOX_TEXT GTK_COMBO_BOX
 #define gtk_combo_box_text_remove gtk_combo_box_remove_text
 #define gtk_combo_box_text_append_text gtk_combo_box_append_text
 #define gtk_combo_box_text_get_active_text gtk_combo_box_get_active_text

#endif
