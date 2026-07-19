// examples/aggregate-return.cpp                                      -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/task/task.hpp>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

int main() {
    struct aggregate {
        int value = 0;
    };

    ex::sync_wait([]() -> ex::task<aggregate> { co_return aggregate{42}; }());
    ex::sync_wait([]() -> ex::task<aggregate> { co_return aggregate{}; }());
    ex::sync_wait([]() -> ex::task<aggregate> { co_return {42}; }());
    ex::sync_wait([]() -> ex::task<aggregate> { co_return {}; }());
}
