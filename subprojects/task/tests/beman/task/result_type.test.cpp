// tests/beman/task/result_type.test.cpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/result_type.hpp>
#include <beman/execution/execution.hpp>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

namespace ex = ::beman::execution;

// ----------------------------------------------------------------------------

namespace {

void unexpected_call_assert(const char* msg) { assert(nullptr == msg); }

struct stopped_receiver {
    using receiver_concept = ::beman::execution::receiver_tag;
    bool& flag;
    void  set_value(auto&&...) && noexcept { unexpected_call_assert("set_value unexpectedly called"); }
    void  set_error(auto&&) && noexcept { unexpected_call_assert("set_error unexpectedly called"); }
    void  set_stopped() && noexcept { flag = true; }
};
static_assert(::beman::execution::receiver<stopped_receiver>);

struct value_receiver {
    using receiver_concept = ::beman::execution::receiver_tag;
    int& value;
    void set_value(int v) && noexcept { value = v; }
    void set_value(auto&&...) && noexcept { unexpected_call_assert("unexpected set_value called"); }
    void set_error(auto&&) && noexcept { unexpected_call_assert("set_error unexpectedly called"); }
    void set_stopped() && noexcept { unexpected_call_assert("set_stopped unexpectedly called"); }
};
static_assert(::beman::execution::receiver<value_receiver>);

struct void_receiver {
    using receiver_concept = ::beman::execution::receiver_tag;
    bool& flag;
    void  set_value() && noexcept { flag = true; }
    void  set_value(auto&&...) && noexcept { unexpected_call_assert("unexpected set_value called"); }
    void  set_error(auto&&) && noexcept { unexpected_call_assert("set_error unexpectedly called"); }
    void  set_stopped() && noexcept { unexpected_call_assert("set_stopped unexpectedly called"); }
};
static_assert(::beman::execution::receiver<value_receiver>);

struct error_receiver {
    using receiver_concept = ::beman::execution::receiver_tag;
    int& error;
    void set_value(auto&&...) && noexcept { unexpected_call_assert("unexpected set_value called"); }
    void set_error(auto&&) && noexcept { unexpected_call_assert("unexpected set_error called"); }
    void set_error(int e) && noexcept { error = e; }
    void set_stopped() && noexcept { unexpected_call_assert("set_stopped unexpectedly called"); }
};
static_assert(::beman::execution::receiver<error_receiver>);

void test_stopped() {
    beman::task::detail::
        result_type<beman::task::detail::stoppable::yes, void, ex::completion_signatures<ex::set_error_t(int)>>
            result{};

    bool flag{false};
    result.result_complete(stopped_receiver{flag});
    assert(flag == true);
}

void test_value() {
    {
        beman::task::detail::
            result_type<beman::task::detail::stoppable::yes, int, ex::completion_signatures<ex::set_error_t(int)>>
                result{};
        result.set_value(' ');

        int value{};
        result.result_complete(value_receiver{value});
        assert(value == ' ');
    }
    {
        beman::task::detail::
            result_type<beman::task::detail::stoppable::yes, void, ex::completion_signatures<ex::set_error_t(int)>>
                result{};
        result.set_value(::beman::task::detail::void_type());
        bool flag{false};
        result.result_complete(void_receiver{flag});
        assert(flag == true);
    }
}

void test_error() {
    struct type {};
    beman::task::detail::result_type<beman::task::detail::stoppable::yes,
                                     int,
                                     ex::completion_signatures<ex::set_error_t(type), ex::set_error_t(int)>>
        result{};
    result.set_error(17);

    int error{};
    result.result_complete(error_receiver{error});
    assert(error == 17);
}

} // namespace

int main() {
    test_stopped();
    test_value();
    test_error();
}
