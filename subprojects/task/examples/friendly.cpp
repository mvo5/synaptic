// examples/friendly.cpp                                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <memory>
#include <iostream>
#include <coroutine>
#include <exception>
#include <system_error>
#include <beman/execution/execution.hpp>
#include <beman/task/task.hpp>

namespace ex = beman::execution;

int main() {
    ex::sync_wait([]() -> ex::task<void> { co_await ex::just(); }());
    ex::sync_wait([]() -> ex::task<void> { co_await std::suspend_never(); }());
}
