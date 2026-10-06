// examples/escaped-exception.cpp                                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>
#include <exception>

namespace ex = beman::execution;

struct my_exception : std::exception {
    const char* what() const noexcept override { return "my_exception"; }
};

int main() {
    try {
        ex::sync_wait([]() -> ex::task<int> {
#ifndef __clang__
            //-dk:TODO determine what's up with clang
            throw my_exception{};
#endif
            co_return 0;
        }());
        std::cout << "not reached!\n";
    } catch (const std::exception& ex) {
        std::cout << "ERROR: " << ex.what() << "\n";
    } catch (...) {
        std::cout << "ERROR: unknown exception\n";
    }
}
