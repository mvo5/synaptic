// include/beman/task/detail/infallible_scheduler.hpp                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_INFALLIBLE_SCHEDULER
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_INFALLIBLE_SCHEDULER

#include <beman/execution/execution.hpp>
#include <concepts>
#include <type_traits>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
template <class Sch, class Env, class... Comp>
concept completes_with =
    ::std::same_as<::beman::execution::completion_signatures<Comp...>,
                   ::beman::execution::
                       completion_signatures_of_t<decltype(::beman::execution::schedule(::std::declval<Sch>())), Env>>;

template <class Sch, class Env>
concept infallible_scheduler =
    (::beman::execution::scheduler<Sch>) &&
    (::beman::task::detail::completes_with<Sch, Env, ::beman::execution::set_value_t()> ||
     (!::beman::execution::unstoppable_token<::beman::execution::stop_token_of_t<Env>> &&
      (::beman::task::detail::
           completes_with<Sch, Env, ::beman::execution::set_value_t(), ::beman::execution::set_stopped_t()> ||
       ::beman::task::detail::
           completes_with<Sch, Env, ::beman::execution::set_stopped_t(), ::beman::execution::set_value_t()>)));
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
