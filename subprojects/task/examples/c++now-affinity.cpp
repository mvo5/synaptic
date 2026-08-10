// examples/c++now-affinity.cpp                                       -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <beman/execution/stop_token.hpp>
#include "demo-thread_loop.hpp"
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

namespace ex = beman::execution;

namespace {

ex::task<> work1(auto sched) {
    std::cout << "before id=" << std::this_thread::get_id() << "\n";
    co_await (ex::schedule(sched) | ex::then([] { std::cout << "then id  =" << std::this_thread::get_id() << "\n"; }));
    std::cout << "after id =" << std::this_thread::get_id() << "\n\n";
}

ex::task<> work2(auto sched) {
    std::cout << "before id=" << std::this_thread::get_id() << "\n";
    auto s = co_await ex::change_coroutine_scheduler(ex::inline_scheduler());
    co_await (ex::schedule(sched) | ex::then([] { std::cout << "then id  =" << std::this_thread::get_id() << "\n"; }));
    std::cout << "after1 id=" << std::this_thread::get_id() << "\n";

    co_await ex::change_coroutine_scheduler(s);
    co_await (ex::schedule(sched) | ex::then([] { std::cout << "then id  =" << std::this_thread::get_id() << "\n"; }));
    std::cout << "after2 id=" << std::this_thread::get_id() << "\n\n";
}

struct inline_context {
    using scheduler_type = ex::inline_scheduler;
};

ex::task<void, inline_context> work3(auto sched) {
    std::cout << "before id=" << std::this_thread::get_id() << "\n";
    co_await (ex::schedule(sched) | ex::then([] { std::cout << "then id  =" << std::this_thread::get_id() << "\n"; }));
    std::cout << "after id =" << std::this_thread::get_id() << "\n\n";
}
} // namespace

int main() {
    try {
        demo::thread_loop loop;
        ex::sync_wait(work1(loop.get_scheduler()));
        ex::sync_wait(work2(loop.get_scheduler()));
        ex::sync_wait(work3(loop.get_scheduler()));
    } catch (...) {
        std::cout << "ERROR: unexpected exception\n";
    }
}
