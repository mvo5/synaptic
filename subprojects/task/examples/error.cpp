// examples/error.cpp                                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <iostream>
#include <system_error>
#include <beman/execution/execution.hpp>
#include <beman/task/task.hpp>

namespace ex = beman::execution;

namespace {
struct error_context {
    using error_types =
        ex::completion_signatures<ex::set_error_t(std::exception_ptr), ex::set_error_t(std::error_code)>;
};

ex::task<int, error_context> fun(int i) {
    using on_exit = std::unique_ptr<const char, decltype([](auto msg) { std::cout << msg; })>;
    on_exit msg("> ");
    std::cout << "<";

    switch (i) {
    default:
        break;
    case 0:
        // successful execution: a later `co_return 0;` completes
        co_await ex::just(0);
        break;
    case 1:
        // the co_awaited work fails and an exception is thrown
        co_await ex::just_error(0);
        break;
    case 2:
        // the co_awaited work is stopped and the coroutine is stopped itself
        co_await ex::just_stopped();
        break;
    case 3:
        // successful return of a value
        co_return 17;
    case 4:
        co_yield ex::with_error{std::make_error_code(std::errc::io_error)};
        break;
    }

    co_return 0;
}

template <typename E>
void print(const char* what, const E& e) {
    if constexpr (std::same_as<std::exception_ptr, E>)
        std::cout << what << "(exception_ptr)\n";
    else
        std::cout << what << "(" << e << ")\n";
}
} // namespace

int main() {
    for (int i{}; i != 5; ++i) {
        std::cout << "i=" << i << " ";
        ex::sync_wait(fun(i) | ex::then([](auto e) { print("then", e); }) |
                      ex::upon_error([](const auto& e) { print("upon_error", e); }) |
                      ex::upon_stopped([] { std::cout << "stopped\n"; }));
    }
}
