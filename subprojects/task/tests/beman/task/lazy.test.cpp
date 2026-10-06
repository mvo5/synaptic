// tests/beman/task/lazy.test.cpp                                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/lazy/lazy.hpp>
#include <beman/execution/execution.hpp>

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

// ----------------------------------------------------------------------------

int main() {
#ifndef _MSC_VER
    beman::execution::sync_wait([]() -> beman::lazy::lazy<> { co_return; }());
    beman::execution::sync_wait([]() -> beman::execution::lazy<> { co_return; }());
#endif
}
