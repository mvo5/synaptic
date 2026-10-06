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

#pragma once

#include "config.h" // IWYU pragma: associated

#include "coroutines.h"

class RUserDialog
{
 public:
   enum ButtonsType {
      ButtonsDefault,
      ButtonsOk,
      ButtonsOkCancel,
      ButtonsYesNo
   };

   enum DialogType { DialogInfo, DialogWarning, DialogQuestion, DialogError };

   [[nodiscard]] virtual task<bool> message(
      const char *msg,
      DialogType dialog = DialogInfo,
      ButtonsType buttons = ButtonsDefault,
      bool defaultResponse = true) = 0;

   [[nodiscard]] virtual task<bool> confirm(const char *msg,
                                            bool defaultResponse = true)
   {
      co_return co_await message(
         msg, DialogQuestion, ButtonsYesNo, defaultResponse);
   }

   [[nodiscard]] virtual task<bool> proceed(const char *msg,
                                            bool defaultResponse = true)
   {
      co_return co_await message(
         msg, DialogInfo, ButtonsOkCancel, defaultResponse);
   }

   [[nodiscard]] virtual task<bool> warning(const char *msg,
                                            bool nocancel = true)
   {
      if (nocancel)
         co_return co_await message(msg, DialogWarning);
      else
         co_return co_await message(msg, DialogWarning, ButtonsOkCancel, false);
   }

   [[nodiscard]] virtual task<void> error(const char *msg)
   {
      co_await message(msg, DialogError);
   }

   [[nodiscard]] virtual task<bool> showErrors();
};
