// examples/c++now-stop_token.cpp                                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <beman/execution/stop_token.hpp>
#include <chrono>
#include <iostream>
#include <thread>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
ex::task<> stopping() {
    auto        token = co_await ex::read_env(ex::get_stop_token);
    std::size_t count{};
    while (!token.stop_requested()) {
        ++count;
    }
    std::cout << "count=" << count << "\n";
}

} // namespace

int main() {
    using namespace std::chrono_literals;

    ex::inplace_stop_source source;
    std::thread             thread([&] {
        ex::sync_wait(ex::detail::write_env(stopping(), ex::detail::make_env(ex::get_stop_token, source.get_token())));
    });

    std::this_thread::sleep_for(100ms);
    source.request_stop();

    thread.join();
}
