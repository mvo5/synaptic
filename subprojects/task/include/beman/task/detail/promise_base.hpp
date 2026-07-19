// include/beman/task/detail/promise_base.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_PROMISE_BASE
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_PROMISE_BASE

#include <beman/task/detail/state_base.hpp>
#include <beman/execution/execution.hpp>
#include <cstddef>
#include <concepts>
#include <type_traits>
#include <variant>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
/*
 * \brief Helper base class dealing with void vs. value results.
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 * \internal
 */
template <::beman::task::detail::stoppable Stop, typename Value, typename Environment>
class promise_base {
  public:
    /*
     * \brief Set the value result.
     * \internal
     */
    template <typename T = Value>
    void return_value(T&& value) {
        this->get_state()->set_value(::std::forward<T>(value));
    }

  public:
    auto set_state(::beman::task::detail::state_base<Value, Environment>* s) noexcept -> void { this->state_ = s; }
    auto get_state() const noexcept -> ::beman::task::detail::state_base<Value, Environment>* { return this->state_; }

  private:
    ::beman::task::detail::state_base<Value, Environment>* state_{};
};

template <typename ::beman::task::detail::stoppable Stop, typename Environment>
class promise_base<Stop, void, Environment> {
  public:
    /*
     * \brief Set the value result although without any value.
     */
    void return_void() { this->get_state()->set_value(void_type{}); }

  public:
    auto set_state(::beman::task::detail::state_base<void, Environment>* s) noexcept -> void { this->state_ = s; }
    auto get_state() const noexcept -> ::beman::task::detail::state_base<void, Environment>* { return this->state_; }

  private:
    ::beman::task::detail::state_base<void, Environment>* state_{};
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
