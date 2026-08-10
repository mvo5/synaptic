// examples/loop.cpp                                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <iostream>
#include <string>
#include <beman/execution/execution.hpp>
#include <beman/task/task.hpp>
#include <iostream>

namespace ex = beman::execution;

namespace {
struct run_loop_env {
    using scheduler_type = decltype(std::declval<ex::run_loop>().get_scheduler());
};

template <ex::scheduler Scheduler>
struct log_scheduler {
    using scheduler_concept = ex::scheduler_tag;

    std::remove_cvref_t<Scheduler> scheduler;

    log_scheduler(auto&& sched) : scheduler(std::forward<decltype(sched)>(sched)) {}

    struct sender {
        using sender_concept        = ex::sender_tag;
        using completion_signatures = ex::completion_signatures<ex::set_value_t()>;
        using up_sender             = decltype(ex::schedule(std::declval<Scheduler>()));

        up_sender _sender;

        template <ex::receiver Receiver>
        struct state {
            using operation_state_concept = ex::operation_state_tag;
            using up_state_t              = decltype(ex::connect(std::declval<up_sender>(), std::declval<Receiver>()));

            up_state_t _state;

            template <ex::sender S, ex::receiver R>
            state(S&& sender, R&& receiver)
                : _state(ex::connect(std::forward<S>(sender), std::forward<R>(receiver))) {}

            auto start() & noexcept -> void {
                std::cout << "start scheduler: " << &this->_state << "\n";
                ex::start(this->_state);
            }
        };

        struct env {
            log_scheduler _sched;
            auto query(const ex::get_completion_scheduler_t<ex::set_value_t>&) const noexcept -> log_scheduler {
                return this->_sched;
            }
        };
        auto get_env() const noexcept -> env {
            return {ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(this->_sender))};
        }
        template <ex::receiver Receiver>
        auto connect(Receiver&& receiver) noexcept -> state<std::remove_cvref_t<Receiver>> {
            return {this->_sender, std::forward<Receiver>(receiver)};
        }
    };

    auto schedule() noexcept { return sender{ex::schedule(this->scheduler)}; }
    auto operator==(const log_scheduler&) const noexcept -> bool = default;
};
template <ex::scheduler Scheduler>
log_scheduler(Scheduler&&) -> log_scheduler<std::remove_cvref_t<Scheduler>>;

static_assert(ex::scheduler<log_scheduler<ex::inline_scheduler>>);
static_assert(ex::scheduler<log_scheduler<ex::task_scheduler>>);
} // namespace

namespace {
[[maybe_unused]] ex::task<void> loop(auto count) {
    for (decltype(count) i{}; i < count; ++i)
        // co_await ex::just(i);
        co_await [](int x) -> ex::task<> {
            std::cout << "before co_await: " << &x << ": " << x << "\n";
            co_await (ex::just(x) | ex::then([](int v) { std::cout << &v << ": " << v << "\n"; }));
            std::cout << "after co_await: " << &x << ": " << x << "\n";
        }(i);
}
} // namespace

int main(int ac, char* av[]) {
    auto count = 1 < ac && av[1] == std::string_view("run-it") ? 1000000 : 1000;
    ex::sync_wait(loop(count));
    ex::sync_wait([](std::size_t count) -> ex::task<void, run_loop_env> {
        co_await ex::write_env(
            loop(count),
            ex::detail::make_env(ex::get_scheduler, log_scheduler(co_await ex::read_env(ex::get_scheduler))));
    }(count));
#if 0
     ex::sync_wait(ex::detail::write_env(loop(count), ex::detail::make_env(ex::get_scheduler, ex::inline_scheduler{})));
#endif
}
