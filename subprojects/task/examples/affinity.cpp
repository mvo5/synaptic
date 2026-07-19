// examples/affinity.cpp                                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/task/task.hpp>
#include "demo-thread_loop.hpp"
#include <iostream>
#include <cassert>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
struct test_receiver {
    using receiver_concept = ex::receiver_tag;

    auto set_value(auto&&...) && noexcept {}
    auto set_error(auto&&) && noexcept {}
    auto set_stopped() && noexcept {}
};
static_assert(ex::receiver<test_receiver>);

std::ostream& fmt_id(std::ostream& out) { return out << std::this_thread::get_id(); }

struct non_affine {
    using scheduler_type = ex::inline_scheduler;
};
} // namespace

int main() {
    std::cout << std::unitbuf;
    demo::thread_loop loop;
    ex::sync_wait(ex::just() | ex::then([]() noexcept { std::cout << "main:" << fmt_id << "\n"; }));
    ex::sync_wait(ex::schedule(loop.get_scheduler()) |
                  ex::then([]() noexcept { std::cout << "loop:" << fmt_id << "\n"; }));
    ex::sync_wait(ex::schedule(ex::task_scheduler(loop.get_scheduler())) |
                  ex::then([]() noexcept { std::cout << "any: " << fmt_id << "\n"; }));
    ex::sync_wait([]() -> ex::task<void> {
        std::cout << "coro:" << fmt_id << "\n";
        co_return;
    }());
    std::cout << "scheduler affine:\n";
    ex::sync_wait([](auto& pl) -> ex::task<void> {
        std::cout << "cor1:" << fmt_id << "\n";
        co_await (ex::schedule(pl.get_scheduler()) | ex::then([] { std::cout << "then:" << fmt_id << "\n"; }));
        std::cout << "cor2:" << fmt_id << "\n";
    }(loop));

    std::cout << "not scheduler affine:\n";
    ex::sync_wait([](auto& pl) -> ex::task<void, non_affine> {
        std::cout << "cor1:" << fmt_id << "\n";
        co_await (ex::schedule(pl.get_scheduler()) | ex::then([] { std::cout << "then:" << fmt_id << "\n"; }));
        std::cout << "cor2:" << fmt_id << "\n";
    }(loop));

    std::cout << "use inline_scheduler:\n";
    ex::sync_wait(ex::starts_on(ex::inline_scheduler{}, [](auto& pl) -> ex::task<void> {
        std::cout << "cor1:" << fmt_id << "\n";
        co_await (ex::schedule(pl.get_scheduler()) | ex::then([] { std::cout << "then:" << fmt_id << "\n"; }));
        std::cout << "cor2:" << fmt_id << "\n";
    }(loop)));
}
