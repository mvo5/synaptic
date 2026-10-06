// tests/beman/task/task.test.cpp                                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <cassert>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
auto test_co_return() {
    auto rc = ex::sync_wait([]() -> ex::task<int> { co_return 17; }());
    assert(rc);
    [[maybe_unused]] auto [value] = rc.value_or(std::tuple{0});
    assert(value == 17);
}

auto test_cancel() {
    ex::sync_wait([]() -> ex::task<> {
        bool stopped{};
        co_await ([]() -> ex::task<void> { co_await ex::just_stopped(); }() |
                              ex::upon_stopped([&stopped]() { stopped = true; }));
        assert(stopped);
    }());
}

auto test_indirect_cancel() {
    // This approach uses symmetric transfer
    ex::sync_wait([]() -> ex::task<> {
        bool stopped{};
        co_await ([]() -> ex::task<void> { co_await []() -> ex::task<void> { co_await ex::just_stopped(); }(); }() |
                              ex::upon_stopped([&stopped]() { stopped = true; }));
        assert(stopped);
    }());
}

auto test_affinity() {
    std::cout << "test_affinity\n";
    ex::sync_wait([]() -> ex::task<> {
        co_await []() -> ex::task<> {
            ex::inline_scheduler sched{};
            std::cout << "comparing schedulers=" << std::boolalpha
                      << (sched == co_await ex::read_env(ex::get_scheduler)) << "\n";
            std::cout << "changing scheduler\n";
            co_await ex::change_coroutine_scheduler(sched);
            std::cout << "changed scheduler\n";
            co_return;
        }();
    }());
}
} // namespace

auto main() -> int {
    test_co_return();
    test_cancel();
    test_indirect_cancel();
    test_affinity();
}
