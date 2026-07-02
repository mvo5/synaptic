// tests/beman/task/scheduler_of.test.cpp                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/scheduler_of.hpp>
#include <beman/task/detail/task_scheduler.hpp>
#include <concepts>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

// ----------------------------------------------------------------------------

namespace {
struct no_scheduler {};

struct defines_scheduler {
    using scheduler_type = beman::execution::inline_scheduler;
};

struct non_scheduler {};
struct defines_non_scheduler {
    using scheduler_type = non_scheduler;
};
} // namespace

int main() {
    static_assert(
        std::same_as<beman::task::detail::task_scheduler, beman::task::detail::scheduler_of_t<no_scheduler>>);
    static_assert(
        std::same_as<beman::execution::inline_scheduler, beman::task::detail::scheduler_of_t<defines_scheduler>>);
    // using type = beman::task::detail::scheduler_of_t<defines_non_scheduler>;
}
