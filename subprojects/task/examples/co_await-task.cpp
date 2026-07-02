// examples/co_await-task.cpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

auto inner() -> ex::task<> { co_return; }

auto outer() -> ex::task<> {
    for (int i{}; i < 10; ++i) {
        co_await inner();
    }
    co_return;
}

auto main() -> int {
    std::cout << std::unitbuf;
    ex::sync_wait(outer());
}
