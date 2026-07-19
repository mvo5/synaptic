// examples/http-server.cpp                                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/execution/task.hpp>
#include <beman/net/net.hpp>

namespace ex  = beman::execution;
namespace net = beman::net;

// ----------------------------------------------------------------------------

namespace {
auto sched_spawn(auto&& scheduler, auto&& sender, auto&& token) {
    return ex::spawn(
        ex::write_env(std::forward<decltype(sender)>(sender) |
                          ex::upon_error([](auto&&) noexcept { std::cout << "ERROR!\n"; }),
                      ex::detail::make_env(ex::get_scheduler, std::forward<decltype(scheduler)>(scheduler))),
        std::forward<decltype(token)>(token));
}
} // namespace

int main() {
    net::io_context    context{};
    ex::counting_scope scope{};

    sched_spawn(
        context.get_scheduler(), ex::just() | ex::then([]() noexcept { std::cout << "hello\n"; }), scope.get_token());
    sched_spawn(
        context.get_scheduler(),
        []() -> ex::task<> {
            std::cout << "hello\n";
            co_return;
        }(),
        scope.get_token());
    ex::sync_wait(scope.join());
}
