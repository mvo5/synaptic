// examples/c++now-result-types.cpp                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <tuple>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
ex::task<> result_type() {
    co_await ex::just();
    [[maybe_unused]] int        n = co_await ex::just(1);
    [[maybe_unused]] std::tuple p = co_await ex::just(1, true);
}
} // namespace

int main() { ex::sync_wait(result_type()); }
