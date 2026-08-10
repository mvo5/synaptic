// examples/c++now-alloctor.cpp                                       -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <memory>
#include <memory_resource>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {

struct with_allocator {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
};

ex::task<void, with_allocator> coro(int value, auto&&...) {
    co_await ex::just(value);
    [[maybe_unused]] auto alloc = co_await ex::read_env(ex::get_allocator);
}
} // namespace

int main() {
    ex::sync_wait(coro(0));
    ex::sync_wait(coro(1, std::allocator_arg, std::pmr::new_delete_resource()));
}
