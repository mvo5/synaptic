// examples/hello.cpp                                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "demo-tls_scheduler.hpp"
#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>
#include <string>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

class tls_data {
    static thread_local std::string value;

  public:
    static std::string get() { return tls_data::value; }
    static void        set(std::string_view v) { tls_data::value = v; }
};

thread_local std::string tls_data::value{};

struct tls_save {
    std::optional<std::string> value;
    void                       save() { this->value = tls_data::get(); }
    void                       restore() { tls_data::set(*this->value); }
};

struct tls_env {
    using scheduler_type = demo::tls_scheduler<tls_save, ::beman::execution::task_scheduler>;
    // using scheduler_type = ex::inline_scheduler;
};

ex::task<void, tls_env> run_timer(std::string name) {
    tls_data::set(name);

    for (int i = 0; i != 3; ++i) {
        co_await ex::just();
        std::cout << "data=" << tls_data::get() << "\n";
    }
}

int main() {
    ex::counting_scope scope;
    ex::sync_wait([](auto& scope) -> ex::task<void, tls_env> {
        auto scheduler = co_await ex::read_env(ex::get_scheduler);

        ex::spawn(ex::write_env(run_timer("timer 1") | ex::upon_error([](auto&&) noexcept {}) |
                                    ex::then([]() noexcept { std::cout << "loop done\n"; }),
                                ex::detail::make_env(ex::get_scheduler, scheduler)),
                  scope.get_token());
        ex::spawn(ex::write_env(run_timer("timer 2") | ex::upon_error([](auto&&) noexcept {}) |
                                    ex::then([]() noexcept { std::cout << "loop done\n"; }),
                                ex::detail::make_env(ex::get_scheduler, scheduler)),
                  scope.get_token());
        co_return;
    }(scope));
    ex::sync_wait(scope.join());
}
