// tests/beman/task/state_base.test.cpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/task/detail/state_base.hpp>
#include <beman/task/detail/allocator_of.hpp>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

namespace ex = beman::execution;
namespace bt = beman::task::detail;

// ----------------------------------------------------------------------------

namespace {
struct environment {};

struct state : beman::task::detail::state_base<int, environment> {
    using allocator_type = ::beman::task::detail::allocator_of_t<environment>;
    stop_source_type source;
    environment      env;
    bool             completed{};
    bool             token{};
    bool             got_environment{};

    ::std::coroutine_handle<> do_complete() override {
        this->completed = true;
        return std::noop_coroutine();
    }
    allocator_type  do_get_allocator() override { return allocator_type{}; }
    stop_token_type do_get_stop_token() override {
        this->token = true;
        return this->source.get_token();
    }
    environment& do_get_environment() override {
        this->got_environment = true;
        return this->env;
    }
    auto do_get_scheduler() -> scheduler_type override { return scheduler_type(ex::inline_scheduler()); }
    auto do_set_scheduler(scheduler_type) -> scheduler_type override { return scheduler_type(ex::inline_scheduler()); }
};
} // namespace

int main() {
    state s;
    assert(s.completed == false);
    s.complete();
    assert(s.completed == true);

    assert(s.token == false);
    s.get_stop_token();
    assert(s.token == true);

    assert(s.got_environment == false);
    s.get_environment();
    assert(s.got_environment == true);
}
