/* ruserdialog.cc
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

#include <apt-pkg/error.h>

#include <ruserdialog.h>

bool RUserDialog::showErrors()
{
   if (_error->empty())
      return false;

   bool iserror = false;
   if (_error->PendingError())
      iserror = true;

   string message = "";
   while (!_error->empty()) {
      string tmp;

      _error->PopMessage(tmp);

      // Ignore some stupid error messages.
      if (tmp == "Tried to dequeue a fetching object")
         continue;

      if (message.empty())
         message = tmp;
      else
         message = message + "\n\n" + tmp;
   }

   if (iserror)
      error(message.c_str());
   else
      warning(message.c_str());

   return true;
}
