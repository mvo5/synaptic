// examples/hello.cpp                                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>

namespace ex = beman::execution;

int main() {
    return std::get<0>(ex::sync_wait([]() -> ex::task<int> {
                           std::cout << "Hello, world!\n";
                           co_return co_await ex::just(0);
                       }())
                           .value_or(std::tuple(-1)));
    ex::sync_wait([](int value) -> ex::task<int> {
        switch (value) {
        default:
            co_return value;
        case -1:
            co_yield ex::with_error(std::make_exception_ptr(value));
        case 2:
            throw value;
        case 0:
            co_await ex::just_stopped();
        }
    }(0));
}
