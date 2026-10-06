// tests/beman/task/promise_base.test.cpp                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/promise_base.hpp>
#include <beman/task/detail/allocator_of.hpp>
#include <beman/task/detail/state_base.hpp>
#include <beman/execution/execution.hpp>
#include <beman/execution/stop_token.hpp>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

namespace ex = beman::execution;
namespace bt = beman::task::detail;

// ----------------------------------------------------------------------------

namespace {

void unexpected_call_assert(const char* message) { assert(nullptr == message); }

struct void_receiver {
    using receiver_concept = ::beman::execution::receiver_tag;

    bool& flag;
    void  set_value() && noexcept { flag = true; }
    void  set_error(auto&&) && noexcept { unexpected_call_assert("unexpected call to set_error"); }
    void  set_stopped() && noexcept { unexpected_call_assert("unexpected call to set_stopped"); }
};
static_assert(::beman::execution::receiver<void_receiver>);

struct int_receiver {
    using receiver_concept = ::beman::execution::receiver_tag;

    int& value;
    void set_value(int v) && noexcept { this->value = v; }
    void set_error(auto&&) && noexcept { unexpected_call_assert("unexpected call to set_error"); }
    void set_stopped() && noexcept { unexpected_call_assert("unexpected call to set_stopped"); }
};
static_assert(::beman::execution::receiver<int_receiver>);

template <typename... E>
struct env {
    using error_types = ex::completion_signatures<ex::set_error_t(E)...>;
};

template <typename T, typename... E>
struct state : bt::state_base<T, env<E...>> {
    using Environment      = env<E...>;
    using allocator_type   = ::beman::task::detail::allocator_of_t<Environment>;
    using stop_source_type = ::beman::task::detail::stop_source_of_t<Environment>;
    using stop_token_type  = decltype(std::declval<stop_source_type>().get_token());
    using scheduler_type   = ::beman::task::detail::scheduler_of_t<Environment>;

    stop_source_type source;
    Environment      ev;
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
    Environment& do_get_environment() override {
        this->got_environment = true;
        return this->ev;
    }
    auto do_get_scheduler() -> scheduler_type override { return scheduler_type(ex::inline_scheduler()); }
    auto do_set_scheduler(scheduler_type) -> scheduler_type override { return scheduler_type(ex::inline_scheduler()); }
};

template <typename T>
using result_type = beman::task::detail::promise_base<beman::task::detail::stoppable::no, T, env<int>>;

} // namespace

int main() {
    {
        state<void, int>  s{};
        result_type<void> result{};
        result.set_state(&s);
        result.return_void();
        bool flag{false};
        s.result_complete(void_receiver{flag});
        assert(flag == true);
    }
    {
        state<int, int>  s{};
        result_type<int> result{};
        result.set_state(&s);
        result.return_value(17);
        int value{};
        s.result_complete(int_receiver{value});
        assert(value == 17);
    }
}
