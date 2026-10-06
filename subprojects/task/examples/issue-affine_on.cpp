// examples/issue-affine_on.cpp                                       -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>

namespace ex = beman::execution;

template <ex::sender Sender>
ex::task<> test(Sender&& sender) {
    co_await std::move(sender);
}

int main() {
    ex::sync_wait(test(ex::just()));
    ex::sync_wait(test(ex::read_env(ex::get_scheduler)));
    ex::sync_wait(test(ex::read_env(ex::get_scheduler) | ex::let_value([](auto) noexcept { return ex::just(); })));
    ex::sync_wait(test(ex::read_env(ex::get_scheduler) |
                       ex::let_value([](auto sched) { return ex::starts_on(sched, ex::just()); })));
}
