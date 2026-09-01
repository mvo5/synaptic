// examples/c++now-with_error.cpp                                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>
#include <cassert>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
void unreachable([[maybe_unused]] const char* msg) { assert(nullptr == msg); }

struct E {
    int value;
};

struct context {
    using error_types = ex::completion_signatures<ex::set_error_t(E)>;
};

// ----------------------------------------------------------------------------

ex::task<int, context> error_return(int arg) noexcept {
    try {
        if (arg == 1)
            co_yield ex::with_error{E{arg}};
        co_return arg * 17;
    } catch (...) {
        unreachable("no error should escape co_yield with_error{E{0}}");
        std::terminate();
    }
}

struct ctxt {
    using error_types = ex::completion_signatures<ex::set_error_t(int)>;
};

ex::task<int, ctxt> call(int v) {
    if (v == 1)
        co_yield ex::with_error{-1};
    co_return 2 * v;
}
} // namespace

int main(int ac, char*[]) {
    for (int i{}; i != 3; ++i) {
        auto [r] = ex::sync_wait(error_return(i) | ex::upon_error([](auto x) {
                                     std::cout << "error: " << x.value << "\n";
                                     return -1;
                                 }))
                       .value_or(std::tuple(0));
        std::cout << i << ": r=" << r << "\n";
    }

    [[maybe_unused]] auto [n] = ex::sync_wait(call(ac) | ex::upon_error([](int e) {
                                                  std::cout << "error(" << e << "\n";
                                                  return -1;
                                              }))
                                    .value_or(std::tuple(0));
}
