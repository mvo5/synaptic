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

#include <vector>
#include <functional>
#include <coroutine>
#include <exception>
#include <optional>
#include <utility>
#include <type_traits>

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

void RGFlushInterface();

bool is_binary_in_path(const char *program);

char *gtk_get_string_from_color(GdkRGBA * colp);
void gtk_get_color_from_string(const char *cpp, GdkRGBA ** colp);

const char *utf8_to_locale(const char *str);
const char *utf8(const char *str);

GtkWidget *get_gtk_image(const char *name, int size=48);
GdkPixbuf *get_gdk_pixbuf(const gchar *name, int size=48);

std::string SizeToStr(double Bytes);
bool RunAsSudoUserCommand(std::vector<const gchar *> cmd);

std::string MarkupEscapeString(std::string str);
std::string MarkupUnescapeString(std::string str);

struct nothing {};

void g_main_context_schedule(std::coroutine_handle<> h);

template <typename F>
void start_task(F&& f)
{
   f().start_detached();
}

template <typename T>
struct task_awaitable;

template <typename T>
struct task_awaitable_owned;

template <typename T>
class task {
public:
   struct promise_type {
      std::optional<T> value;
      std::exception_ptr exception;
      std::coroutine_handle<> continuation;
      bool detached = false;

      task get_return_object() {
         return task {
            std::coroutine_handle<promise_type>::from_promise(*this)
         };
      }

      std::suspend_always initial_suspend() noexcept { return {}; }
      auto final_suspend() noexcept {
         struct final_awaiter {
            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<promise_type> h) const noexcept
            {
               auto &promise = h.promise();
               if (promise.continuation) {
                  promise.continuation.resume();
               }
               if (promise.detached) {
                  h.destroy();
               }
            }

            void await_resume() const noexcept {}
         };

         return final_awaiter {};
      }

      void return_value(T v) { value = std::move(v); }
      void unhandled_exception() { exception = std::current_exception(); }
   };

   using handle_t = std::coroutine_handle<promise_type>;

   explicit task(handle_t h) : h_(h) {}
   task(task&& o) noexcept : h_(o.h_) { o.h_ = {}; }
   ~task() { if (h_) h_.destroy(); }

   T result() {
      if (h_.promise().exception)
         std::rethrow_exception(h_.promise().exception);
      if (!h_.promise().value) {
         if constexpr (std::is_same_v<T, nothing>) {
            return nothing {};
         }
      }
      return std::move(*h_.promise().value);
   }

   handle_t handle() const { return h_; }

   void start_detached()
   {
      if (!h_)
         return;

      h_.promise().detached = true;
      g_main_context_schedule(h_);
      h_ = {};
   }

   auto operator co_await() & { return task_awaitable<T>{*this}; }
   auto operator co_await() && { return task_awaitable_owned<T>{std::move(*this)}; }

private:
   handle_t h_;
};

struct sleep_ms {
   int ms;

   bool await_ready() const noexcept { return false; }

   void await_suspend(std::coroutine_handle<> h)
   {
      auto* source = g_timeout_source_new(ms);
      g_source_set_callback(
         source,
         [](gpointer data) -> gboolean {
            auto h = std::coroutine_handle<>::from_address(data);
            h.resume();
            return G_SOURCE_REMOVE;
         },
         h.address(),
         nullptr
      );
      g_source_attach(source, g_main_context_default());
      g_source_unref(source);
   }

   void await_resume() const noexcept {}
};

template <typename T>
struct task_awaitable {
   task<T>& t;

   bool await_ready() const noexcept { return false; }

   void await_suspend(std::coroutine_handle<> h)
   {
      t.handle().promise().continuation = h;
      t.handle().resume();
   }

   T await_resume()
   {
      return t.result();
   }
};

template <typename T>
struct task_awaitable_owned {
   task<T> t;

   bool await_ready() const noexcept { return false; }

   void await_suspend(std::coroutine_handle<> h)
   {
      t.handle().promise().continuation = h;
      t.handle().resume();
   }

   T await_resume()
   {
      return t.result();
   }
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

#endif
