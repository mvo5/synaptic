// tests/beman/task/detail/single_thread_context.test.cpp             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/single_thread_context.hpp>
#include <beman/execution/execution.hpp>
#include <thread>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

int main() {
    ::beman::task::detail::single_thread_context context;

    auto main_id = std::this_thread::get_id();
    auto [thread_id] =
        ex::sync_wait(ex::schedule(context.get_scheduler()) | ex::then([] { return std::this_thread::get_id(); }))
            .value_or(std::tuple(std::thread::id{}));

    assert(main_id != thread_id);
}
