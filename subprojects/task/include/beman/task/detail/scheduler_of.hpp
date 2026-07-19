// include/beman/task/detail/scheduler_of.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_SCHEDULER_OF
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_SCHEDULER_OF

#include <beman/task/detail/task_scheduler.hpp>
#include <beman/execution/execution.hpp>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
/*!
 * \brief Utility to get a scheduler type from a context
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 * \internal
 */
template <typename>
struct scheduler_of {
    using type = ::beman::task::detail::task_scheduler;
};
template <typename Context>
    requires requires { typename Context::scheduler_type; }
struct scheduler_of<Context> {
    using type = typename Context::scheduler_type;
    static_assert(::beman::execution::scheduler<type>, "The type alias scheduler_type needs to refer to a scheduler");
};
template <typename Context>
using scheduler_of_t = typename scheduler_of<Context>::type;
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
