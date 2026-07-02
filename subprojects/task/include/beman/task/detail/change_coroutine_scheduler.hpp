// include/beman/task/detail/change_coroutine_scheduler.hpp           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_CHANGE_COROUTINE_SCHEDULER
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_CHANGE_COROUTINE_SCHEDULER

#include <beman/execution/execution.hpp>
#include <beman/task/detail/task_scheduler.hpp>
#include <coroutine>
#include <type_traits>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
template <::beman::execution::scheduler Scheduler>
struct change_coroutine_scheduler {
    using type = Scheduler;
    type scheduler;

    template <::beman::execution::scheduler S>
    change_coroutine_scheduler(S&& s) : scheduler(::std::forward<S>(s)) {}

    constexpr bool await_ready() const { return false; }
    template <typename P>
    bool await_suspend(::std::coroutine_handle<P> h) {
        this->scheduler = h.promise().change_scheduler(std::move(this->scheduler));
        return false;
    }
    type await_resume() { return this->scheduler; }
};
template <::beman::execution::scheduler S>
change_coroutine_scheduler(S&&) -> change_coroutine_scheduler<::beman::task::detail::task_scheduler>;
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
