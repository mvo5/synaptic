/* rguserdialog.cc
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

#include <rquserdialog.h>
#include <qmessagebox.h>
#include <qstring.h>

#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>

bool RQUserDialog::message(const char *msg,
                           RUserDialog::DialogType dialog,
                           RUserDialog::ButtonsType buttons, bool defres)
{
   int res = 0;
   int button0 = 0;
   int button1 = 0;

   if (buttons == RUserDialog::ButtonsOkCancel) {
      button0 = QMessageBox::Ok;
      button1 = QMessageBox::Cancel;
   } else {
      button0 = QMessageBox::Yes;
      button1 = QMessageBox::No;
   }
   if (defres) {
      button0 |= QMessageBox::Default;
      button1 |= QMessageBox::Escape;
   } else {
      button0 |= QMessageBox::Escape;
      button1 |= QMessageBox::Default;
   }

   switch (dialog) {
      case RUserDialog::DialogInfo:
         res = QMessageBox::information(_parent, "Synaptic", msg,
                                        QMessageBox::Ok);
         break;
      case RUserDialog::DialogWarning:
         res = QMessageBox::warning(_parent, "Synaptic", msg,
                                    QMessageBox::Ok, 0);
         break;
      case RUserDialog::DialogError:
         res = QMessageBox::critical(_parent, "Synaptic", msg,
                                     QMessageBox::Ok, 0);
         break;
      case RUserDialog::DialogQuestion:
         res = QMessageBox::question(_parent, "Synaptic", msg,
                                     button0, button1);
         break;
   }

   return (res == 0);
}

// vim:ts=3:sw=3:et
