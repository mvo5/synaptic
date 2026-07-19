// examples/issue-frame-allocator.cpp                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>
#include <functional>
#include <memory_resource>

namespace ex = beman::execution;

template <typename Env>
ex::task<int, Env> test(int i, auto&&...) {
    co_return co_await ex::just(i);
}
struct default_env {};
struct allocator_env {
    using allocator_type = std::pmr::polymorphic_allocator<>;
};

int main() {
    ex::sync_wait(test<default_env>(17));                                            // OK: no allocator
    ex::sync_wait(test<default_env>(17, std::allocator_arg, std::allocator<int>())); // OK: allocator is convertible
    // ex::sync_wait(test<default_env>(17, std::allocator_arg, std::pmr::polymorphic_allocator<>())); // error
    ex::sync_wait(test<allocator_env>(17, std::allocator_arg, std::pmr::polymorphic_allocator<>())); // OK but unusual
}
