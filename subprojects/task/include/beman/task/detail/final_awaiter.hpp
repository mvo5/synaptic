// include/beman/task/detail/final_awaiter.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_FINAL_AWAITER
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_FINAL_AWAITER

#include <beman/task/detail/state_base.hpp>
#include <coroutine>

// ----------------------------------------------------------------------------

namespace beman::task::detail {

struct final_awaiter {
    static constexpr auto await_ready() noexcept -> bool { return false; }
    template <typename Promise>
    static auto await_suspend(std::coroutine_handle<Promise> handle) noexcept {
        return handle.promise().notify_complete();
    }
    static constexpr void await_resume() noexcept {}
};

} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
