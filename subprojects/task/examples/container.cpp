// examples/container.cpp                                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>
#include <vector>
#include <cassert>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
void unreachable([[maybe_unused]] const char* msg) { assert(nullptr != msg); }
} // namespace

int main() {
    try {
        std::vector<ex::task<>> cont;
        cont.emplace_back([]() -> ex::task<> { co_return; }());
        cont.push_back([]() -> ex::task<> { co_return; }());
    } catch (...) {
        unreachable("no exception should escape to main");
    }
}
