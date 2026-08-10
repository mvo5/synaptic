// include/beman/task/detail/promise_type.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_PROMISE_TYPE
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_PROMISE_TYPE

#include <beman/task/detail/awaiter.hpp>
#include <beman/task/detail/allocator_of.hpp>
#include <beman/task/detail/allocator_support.hpp>
#include <beman/task/detail/change_coroutine_scheduler.hpp>
#include <beman/task/detail/error_types_of.hpp>
#include <beman/task/detail/final_awaiter.hpp>
#include <beman/task/detail/find_allocator.hpp>
#include <beman/task/detail/handle.hpp>
#include <beman/task/detail/promise_base.hpp>
#include <beman/task/detail/result_type.hpp>
#include <beman/task/detail/scheduler_of.hpp>
#include <beman/task/detail/state_base.hpp>
#include <beman/task/detail/with_error.hpp>
#include <beman/execution/execution.hpp>
#include <beman/execution/detail/meta_contains.hpp>
#include <beman/task/detail/promise_env.hpp>
#include <coroutine>
#include <optional>
#include <type_traits>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
namespace meta {
template <typename, typename>
struct list_contains;
template <template <typename...> class L, typename... E, typename T>
struct list_contains<L<E...>, T> {
    static constexpr bool value = (std::same_as<E, T> || ...);
};
template <typename L, typename T>
inline constexpr bool list_contains_v{list_contains<L, T>::value};
} // namespace meta

template <typename Coroutine, typename Value, typename Environment>
class promise_type
    : public ::beman::task::detail::
          promise_base<::beman::task::detail::stoppable::yes, ::std::remove_cvref_t<Value>, Environment>,
      public ::beman::task::detail::allocator_support<::beman::task::detail::allocator_of_t<Environment>> {
  public:
    using allocator_type   = ::beman::task::detail::allocator_of_t<Environment>;
    using scheduler_type   = ::beman::task::detail::scheduler_of_t<Environment>;
    using stop_source_type = ::beman::task::detail::stop_source_of_t<Environment>;
    using stop_token_type  = decltype(std::declval<stop_source_type>().get_token());

    constexpr auto initial_suspend() noexcept -> ::std::suspend_always { return {}; }
    constexpr auto final_suspend() noexcept -> ::beman::task::detail::final_awaiter { return {}; }

    auto unhandled_exception() noexcept -> void {
        using error_types = ::beman::task::detail::error_types_of_t<Environment>;
        if constexpr (::beman::task::detail::meta::
                          list_contains_v<error_types, ::beman::execution::set_error_t(::std::exception_ptr)>) {
            this->get_state()->set_error(::std::current_exception());
        } else {
            std::terminate();
        }
    }
    std::coroutine_handle<> unhandled_stopped() { return this->get_state()->complete(); }

    auto get_return_object() noexcept { return Coroutine(::beman::task::detail::handle<promise_type>(this)); }

    template <::beman::execution::sender Sender>
    auto await_transform(Sender&& sender) {
        if constexpr (requires {
                          ::std::forward<Sender>(sender).as_awaitable(*this);
                          // typename ::std::remove_cvref_t<Sender>::task_concept;
                      }) {
            return ::std::forward<Sender>(sender).as_awaitable(*this);
        } else if constexpr (::std::same_as<::beman::execution::tag_of_t<::std::remove_cvref_t<Sender>>,
                                            ::beman::execution::read_env_t>) {
            return ::beman::execution::as_awaitable(::std::forward<Sender>(sender), *this);
        } else {
            return ::beman::execution::as_awaitable(::beman::execution::affine_on(::std::forward<Sender>(sender)),
                                                    *this);
        }
    }
    auto await_transform(::beman::task::detail::change_coroutine_scheduler<scheduler_type> c) {
        return ::std::move(c);
    }

    template <typename E>
    auto yield_value(with_error<E> with) noexcept -> ::beman::task::detail::final_awaiter {
        this->get_state()->set_error(::std::move(with.error));
        return {};
    }

    auto get_env() const noexcept -> ::beman::task::detail::promise_env<promise_type> { return {this}; }

    auto start(::beman::task::detail::state_base<Value, Environment>* state) -> ::std::coroutine_handle<> {
        this->set_state(state);
        return ::std::coroutine_handle<promise_type>::from_promise(*this);
    }
    auto           notify_complete() -> ::std::coroutine_handle<> { return this->get_state()->complete(); }
    scheduler_type change_scheduler(scheduler_type other) {
        return this->get_state()->set_scheduler(::std::move(other));
    }

    auto get_scheduler() const noexcept -> scheduler_type { return this->get_state()->get_scheduler(); }
    auto get_allocator() const noexcept -> allocator_type { return this->get_state()->get_allocator(); }
    auto get_stop_token() const noexcept -> stop_token_type { return this->get_state()->get_stop_token(); }
    auto get_environment() const noexcept -> const Environment& {
        assert(this);
        assert(this->get_state());
        return this->get_state()->get_environment();
    }

  private:
    using env_t = ::beman::task::detail::promise_env<promise_type>;

    ::std::optional<scheduler_type> scheduler{};
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
