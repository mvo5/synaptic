/* ruserdialog.h
 * 
 * Copyright (c) 2003 Conectiva S/A 
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

#ifndef RUSERDIALOG_H
#define RUSERDIALOG_H

class RUserDialog {
 public:
   enum ButtonsType {
      ButtonsDefault,
      ButtonsOk,
      ButtonsOkCancel,
      ButtonsYesNo
   };

   enum DialogType {
      DialogInfo,
      DialogWarning,
      DialogQuestion,
      DialogError
   };

   virtual bool message(const char *msg,
                        DialogType dialog = DialogInfo,
                        ButtonsType buttons = ButtonsDefault,
                        bool defres = true) = 0;

   virtual bool confirm(const char *msg, bool defres = true) {
      return message(msg, DialogQuestion, ButtonsYesNo, defres);
   };

   virtual bool proceed(const char *msg, bool defres = true) {
      return message(msg, DialogInfo, ButtonsOkCancel, defres);
   };

   virtual bool warning(const char *msg, bool nocancel = true) {
      return nocancel ? message(msg, DialogWarning)
         : message(msg, DialogWarning, ButtonsOkCancel, false);
   };

   virtual void error(const char *msg) {
      message(msg, DialogError);
   };

   virtual bool showErrors();

};

#endif

// vim:sts=4:sw=4
