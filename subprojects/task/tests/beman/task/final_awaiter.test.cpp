// tests/beman/task/final_awaiter.test.cpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/final_awaiter.hpp>
#include <beman/task/detail/handle.hpp>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

// ----------------------------------------------------------------------------

namespace {

struct tester {
    template <typename P>
    using handle = beman::task::detail::handle<P>;

    struct promise_type {
        bool& called;
        explicit promise_type(bool& c, const auto&...) : called(c) {}
        explicit promise_type(auto&&, bool& c, const auto&...) : called(c) {}
        std::suspend_always                final_suspend() noexcept { return {}; }
        beman::task::detail::final_awaiter initial_suspend() noexcept { return {}; }
        void                               unhandled_exception() {}
        tester                             get_return_object() { return {handle<promise_type>(this)}; }
        void                               return_void() {}
        std::coroutine_handle<>            notify_complete() {
            called = true;
            return std::coroutine_handle<promise_type>::from_promise(*this);
        }
    };

    handle<promise_type> h;
};

} // namespace

int main() {
#ifndef _MSC_VER
    bool called{false};
    [](bool&) -> tester { co_return; }(called);
    assert(called);
#endif
}
