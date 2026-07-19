// examples/issue-start-reschedules.cpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include "demo-thread_loop.hpp"
#include <iostream>
#include <thread>

namespace ex = beman::execution;

ex::task<> test(auto sched) {
    std::cout << "init =" << std::this_thread::get_id() << "\n";
    co_await ex::starts_on(sched, ex::just());
    // static_assert(std::same_as<void, decltype(ex::get_completion_signatures(ex::starts_on(sched, ex::just()),
    // ex::env<>{}))>);
    co_await ex::just();
    std::cout << "final=" << std::this_thread::get_id() << "\n";
}

int main() {
    demo::thread_loop loop1;
    demo::thread_loop loop2;
    std::cout << "main =" << std::this_thread::get_id() << "\n";
    ex::sync_wait(ex::schedule(loop1.get_scheduler()) |
                  ex::then([] { std::cout << "loop1=" << std::this_thread::get_id() << "\n"; }));
    ex::sync_wait(ex::schedule(loop2.get_scheduler()) |
                  ex::then([] { std::cout << "loop2=" << std::this_thread::get_id() << "\n"; }));
    try {
        std::cout << "--- use 1 ---\n";
        ex::sync_wait(test(loop2.get_scheduler()));
        std::cout << "--- use 2 ---\n";
        // ex::sync_wait(ex::starts_on(loop1.get_scheduler(), test(loop2.get_scheduler())));
    } catch (...) {
    }
}
