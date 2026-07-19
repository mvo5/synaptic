// examples/c++now-return.cpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {

struct E {};
struct F {};

struct context {
    using error_types = ex::completion_signatures<ex::set_error_t(E), ex::set_error_t(F)>;
};

struct empty_errors_context {
    using error_types = ex::completion_signatures<>;
};

// ----------------------------------------------------------------------------

ex::task<> default_return() { co_return; }

ex::task<void> void_return() { co_return; }

ex::task<int> int_return() { co_return 17; }

ex::task<int, context> error_return() { co_return 17; }

ex::task<int, empty_errors_context> no_error_return() { co_return 17; }
} // namespace

int main() {
    ex::sync_wait(default_return());
    ex::sync_wait(void_return());
    [[maybe_unused]] auto [n] = ex::sync_wait(int_return()).value_or(std::tuple(0));
    std::cout << "n=" << n << "\n";
    [[maybe_unused]] auto [e] = ex::sync_wait(error_return()).value_or(std::tuple(0));
    [[maybe_unused]] auto [x] = ex::sync_wait(no_error_return()).value_or(std::tuple(0));
}
