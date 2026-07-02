/* rgmisc.h
 * 
 * Copyright (c) 2003 Michael Vogt
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

#ifndef _RGMISC_H_
#define _RGMISC_H_

#include "config.h"

#include <vector>
#include <functional>
#include <coroutine>
#include <exception>
#include <optional>
#include <utility>
#include <type_traits>
#include "coroutines.h"

enum {
   PIXMAP_COLUMN,
   SUPPORTED_COLUMN,
   NAME_COLUMN,
   COMPONENT_COLUMN,
   SECTION_COLUMN,
   PKG_SIZE_COLUMN,
   PKG_DOWNLOAD_SIZE_COLUMN,
   INSTALLED_VERSION_COLUMN,
   AVAILABLE_VERSION_COLUMN,
   DESCR_COLUMN,
   COLOR_COLUMN,
   PKG_COLUMN,
   N_COLUMNS
};

task<void> RGFlushInterface();

bool is_binary_in_path(const char *program);

char *gtk_get_string_from_color(GdkRGBA * colp);
void gtk_get_color_from_string(const char *cpp, GdkRGBA ** colp);

const char *utf8_to_locale(const char *str);
const char *utf8(const char *str);

std::string SizeToStr(double Bytes);
bool RunAsSudoUserCommand(std::vector<const gchar *> cmd);

std::string MarkupEscapeString(std::string str);
std::string MarkupUnescapeString(std::string str);

struct sleep_ms {
   int ms;

   bool await_ready() const noexcept { return false; }

   void await_suspend(std::coroutine_handle<> h)
   {
      g_timeout_add_once(
         ms,
         [](gpointer data) {
            auto h = std::coroutine_handle<>::from_address(data);
            h.resume();
         },
         h.address()
      );
   }

   void await_resume() const noexcept {}
};

struct glib_idle {
   bool await_ready() const noexcept { return false; }

   void await_suspend(std::coroutine_handle<> h)
   {
      g_idle_add_once(
         [](gpointer data) {
            auto h = std::coroutine_handle<>::from_address(data);
            h.resume();
         },
         h.address()
      );
   }

   void await_resume() const noexcept {}
};

template<typename T>
struct Awaiter {
   using F = std::function<void(std::function<void(T)>)>;

   F func;
   std::coroutine_handle<> handle;
   std::optional<T> result;
   std::exception_ptr exception;

   explicit Awaiter(F&& f) : func(std::forward<F>(f)) {}

   bool await_ready() const { return false; }

   void await_suspend(std::coroutine_handle<> h) {
      handle = h;
      try {
         func([this](T value) {
            result = std::move(value);
            if (handle) {
               handle.resume();
            }
         });
      } catch (...) {
         exception = std::current_exception();
         handle.resume();
      }
   }

   T await_resume() {
      if (exception) {
         std::rethrow_exception(exception);
      }
      return std::move(result.value());
   }
};

Awaiter<int> co_run_dialog(GtkDialog *dialog);
Awaiter<nothing> co_run_window(GtkWindow *window);

struct glib_scheduler {
   bool await_ready() const noexcept { return false; }

   void await_suspend(std::coroutine_handle<> h) {
      g_main_context_invoke(
         g_main_context_default(),
         [](gpointer data) -> gboolean {
            auto h = std::coroutine_handle<>::from_address(data);
            h.resume();
            return G_SOURCE_REMOVE;
         },
         h.address()
      );
   }

   void await_resume() const noexcept {}
};

template<class T>
void destroy_later(T *&obj) noexcept {
   if (!obj)
      return;
   T *ptr = obj;
   obj = nullptr;
   g_idle_add_once(
      [](gpointer data) {
         delete static_cast<T *>(data);
      },
      ptr);
}

struct DetachedTaskOperationBase {
   virtual ~DetachedTaskOperationBase() = default;
};

struct DetachedTaskReceiver {
   using receiver_concept = beman::execution::receiver_tag;

   DetachedTaskOperationBase *operation = nullptr;

   struct Env {
      auto query(beman::execution::get_scheduler_t) const noexcept {
         return beman::execution::inline_scheduler {};
      }
   };

   auto get_env() const noexcept {
      return Env {};
   }

   void set_value() && noexcept {
      destroy_later(operation);
   }

   template <typename Error>
   void set_error(Error&&) && noexcept {
      destroy_later(operation);
   }

   void set_stopped() && noexcept {
      destroy_later(operation);
   }
};

template <typename Operation>
struct DetachedTaskOperation : DetachedTaskOperationBase {
   std::move_only_function<task<void>()> *func;
   Operation operation;

   explicit DetachedTaskOperation(std::move_only_function<task<void>()> *func)
      : func(func), operation(beman::execution::connect((*func)(), DetachedTaskReceiver { this })) {}

   ~DetachedTaskOperation() {
      delete func;
   }

   void start() noexcept {
      beman::execution::start(operation);
   }
};

template <typename F>
void start_task(F&& f)
{
   auto *ff = new std::move_only_function<task<void>()> {
      [fn = std::forward<F>(f)]() -> task<void> {
         co_await glib_scheduler {};
         co_await fn();
      }
   };

   using Operation = decltype(beman::execution::connect(std::move((*ff)()), DetachedTaskReceiver {}));
   auto *operation = new DetachedTaskOperation<Operation>(ff);
   operation->start();
}

#endif
