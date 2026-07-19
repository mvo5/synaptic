// examples/environment.cpp                                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef _MSC_VER
#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <beman/net/net.hpp>
#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <type_traits>
#include "demo-scope.hpp"
#include "demo-thread_loop.hpp"

namespace ex  = beman::execution;
namespace net = beman::net;
using namespace std::chrono_literals;

// ----------------------------------------------------------------------------

template <ex::scheduler Sched, ex::sender Sender>
void spawn(Sched&& sched, demo::scope& scope, Sender&& sender) {
    scope.spawn(ex::detail::write_env(std::forward<Sender>(sender),
                                      ex::detail::make_env(ex::get_scheduler, std::forward<Sched>(sched))));
}

// ----------------------------------------------------------------------------

class environment {
    static thread_local std::string name;

  public:
    static auto get() -> std::string { return name; }
    static auto set(const std::string& n) -> void { name = n; }
};
thread_local std::string environment::name{"<none>"};

struct env_scheduler {
    using scheduler_concept = ex::scheduler_tag;

    std::string        name;
    ex::task_scheduler scheduler;

    template <typename Sched>
    env_scheduler(std::string n, Sched&& sched) : name(std::move(n)), scheduler(std::forward<Sched>(sched)) {}

    template <ex::receiver Rcvr>
    struct receiver {
        using receiver_concept = ex::receiver_tag;

        std::remove_cvref_t<Rcvr> rcvr;
        std::string               name;

        receiver(Rcvr&& r, std::string n) : rcvr(std::forward<Rcvr>(r)), name(std::move(n)) {}
        auto get_env() const noexcept { return ex::get_env(this->rcvr); }
        auto set_value() && noexcept {
            environment::set(std::move(this->name));
            ex::set_value(std::move(this->rcvr));
        }
        template <typename E>
        auto set_error(E&& e) && noexcept {
            environment::set(std::move(this->name));
            ex::set_error(std::move(this->rcvr), std::forward<E>(e));
        }
        auto set_stopped() && noexcept {
            environment::set(std::move(this->name));
            ex::set_stopped(std::move(this->rcvr));
        }
    };

    struct env {
        std::string        name;
        ex::task_scheduler scheduler;
        template <typename Tag>
        auto query(const ex::get_completion_scheduler_t<Tag>&) const noexcept {
            return env_scheduler(this->name, this->scheduler);
        }
    };

    struct sender {
        using sender_concept = ex::sender_tag;
        using task_sender    = decltype(ex::schedule(std::declval<ex::task_scheduler>()));
        template <typename E>
        auto get_completion_signatures(const E& e) const noexcept {
            return ex::get_completion_signatures(this->sender, e);
        }

        std::string name;
        task_sender sender;

        auto get_env() const noexcept -> env {
            return env{this->name, ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(this->sender))};
        }

        template <ex::receiver Rcvr>
        auto connect(Rcvr&& rcvr) && {
            return ex::connect(std::move(this->sender),
                               receiver<Rcvr>(std::forward<Rcvr>(rcvr), std::move(this->name)));
        }
    };

    auto schedule() -> sender { return sender{this->name, ex::schedule(this->scheduler)}; }
    bool operator==(const env_scheduler&) const = default;
};

struct with_env {
    using scheduler_type = env_scheduler;
};

// ----------------------------------------------------------------------------

std::ostream& print_env(std::ostream& out) {
    return out << "tid=" << std::this_thread::get_id() << " "
               << "env=" << environment::get();
}

ex::task<void, with_env> run(auto scheduler, auto duration) {
    std::cout << print_env << " duration=" << duration << " start\n" << std::flush;
    for (int i = 0; i != 4; ++i) {
        co_await net::resume_after(scheduler, duration);
        std::cout << print_env << " duration=" << duration << "\n" << std::flush;
    }
    std::cout << print_env << " duration=" << duration << " done\n" << std::flush;
}

const std::string                  text("####");
[[maybe_unused]] const std::string black("\x1b[30m" + text + "\x1b[0m:");
[[maybe_unused]] const std::string red("\x1b[31m" + text + "\x1b[0m:");
[[maybe_unused]] const std::string green("\x1b[32m" + text + "\x1b[0m:");
[[maybe_unused]] const std::string yellow("\x1b[33m" + text + "\x1b[0m:");
[[maybe_unused]] const std::string blue("\x1b[34m" + text + "\x1b[0m:");
[[maybe_unused]] const std::string magenta("\x1b[35m" + text + "\x1b[0m:");
[[maybe_unused]] const std::string cyan("\x1b[36m" + text + "\x1b[0m:");
[[maybe_unused]] const std::string white("\x1b[37m" + text + "\x1b[0m:");

int main() {
    demo::thread_loop loop1;
    demo::thread_loop loop2;
    net::io_context   context;
    demo::scope       scope;

    environment::set("main");

    ex::sync_wait(ex::schedule(loop1.get_scheduler()) | ex::then([] { environment::set("thread1"); }) |
                  ex::then([] { std::cout << print_env << "\n"; }));
    std::cout << print_env << "\n";

    spawn(env_scheduler(magenta, loop1.get_scheduler()), scope, run(context.get_scheduler(), 100ms));
    spawn(env_scheduler(green, loop1.get_scheduler()), scope, run(context.get_scheduler(), 150ms));
    spawn(env_scheduler(blue, loop1.get_scheduler()), scope, run(context.get_scheduler(), 250ms));

    while (!scope.empty()) {
        context.run();
    }
}
#else
int main() {}
#endif
