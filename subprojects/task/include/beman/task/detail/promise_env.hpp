// include/beman/task/detail/promise_env.hpp                          -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_PROMISE_ENV
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_PROMISE_ENV

#include <beman/execution/execution.hpp>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
template <typename Promise>
struct promise_env {
    const Promise* promise;

    auto query(const ::beman::execution::get_scheduler_t&) const noexcept -> typename Promise::scheduler_type {
        return this->promise->get_scheduler();
    }
    auto query(const ::beman::execution::get_allocator_t&) const noexcept -> typename Promise::allocator_type {
        return this->promise->get_allocator();
    }
    auto query(const ::beman::execution::get_stop_token_t&) const noexcept -> typename Promise::stop_token_type {
        return this->promise->get_stop_token();
    }

    template <typename Q, typename... A>
        requires requires(const Promise* p, Q q, A&&... a) {
            ::beman::execution::forwarding_query(q);
            q(p->get_environment(), std::forward<A>(a)...);
        }
    auto query(Q q, A&&... a) const noexcept {
        return q(promise->get_environment(), std::forward<A>(a)...);
    }
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
