// examples/odd-return.cpp                                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/execution/task.hpp>
#include <cassert>
#include <exception>
#include <variant>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

int main() {
    auto mono = ex::sync_wait([]() -> ex::task<std::monostate> { co_return std::monostate{}; }());
    assert(mono);

    auto exp = ex::sync_wait([]() -> ex::task<std::exception_ptr> { co_return std::make_exception_ptr(17); }());
    assert(exp);
}
