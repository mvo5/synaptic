// examples/stop.cpp                                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/execution/stop_token.hpp>
#include <beman/task/task.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <cinttypes>

namespace ex = beman::execution;

namespace {
void unreachable([[maybe_unused]] const char* msg) { assert(nullptr == msg); }
} // namespace

int main() {
    try {
        ex::stop_source source;
        std::thread     t([&source] {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(100ms);
            source.request_stop();
        });

        struct context {
            struct xstop_source_type { // remove the x to disable the stop token
                static constexpr bool                               stop_possible() { return false; }
                static constexpr void                               request_stop() {}
                static constexpr beman::execution::never_stop_token get_token() { return {}; }
            };
        };

        auto [result] = ex::sync_wait(ex::detail::write_env(
                                          []() -> ex::task<std::uint64_t, context> {
                                              auto          token(co_await ex::read_env(ex::get_stop_token));
                                              std::uint64_t count{};
                                              while (!token.stop_requested() && count != 200'000'000u) {
                                                  ++count;
                                              }
                                              co_return count;
                                          }(),
                                          ex::detail::make_env(ex::get_stop_token, source.get_token())))
                            .value_or(std::uint64_t());
        std::cout << "result=" << result << "\n";

        t.join();
    } catch (...) {
        unreachable("an unexpected exception escaped to main");
    }
}
