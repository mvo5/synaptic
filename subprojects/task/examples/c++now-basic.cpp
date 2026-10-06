// examples/c++now-basic.cpp                                          -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <coroutine>
#include <iostream>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
ex::task<> basic() { co_await std::suspend_never{}; }

ex::task<> await_sender() {
    co_await ex::just();
    [[maybe_unused]] int n       = co_await ex::just(1);
    [[maybe_unused]] auto [m, b] = co_await ex::just(1, true);
    try {
        co_await ex::just_error(1);
    } catch (int e) {
        std::cout << "caught " << e << '\n';
    }
    co_await ex::just_stopped();
}

} // namespace

int main() {
    ex::sync_wait(std::suspend_never{});
    ex::sync_wait(basic());
    ex::sync_wait(await_sender());
}
