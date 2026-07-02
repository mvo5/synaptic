// tests/beman/task/handle.test.cpp                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
        bool& destroyed;
        explicit promise_type(bool& d, const auto&...) : destroyed(d) {}
        promise_type(auto&&, bool& d, const auto&...) : destroyed(d) {}
        promise_type(const promise_type&) = delete;
        promise_type(promise_type&&)      = delete;
        ~promise_type() { this->destroyed = true; }
        promise_type& operator=(const promise_type&) = delete;
        promise_type& operator=(promise_type&&)      = delete;

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void                unhandled_exception() {}
        tester              get_return_object() { return {handle<promise_type>(this)}; }
        void                return_void() {}
        void                start(bool& b) {
            b = true;
            std::coroutine_handle<promise_type>::from_promise(*this).resume();
        }
    };

    handle<promise_type> h;
};

} // namespace

int main() {
    {
        bool destroyed{false};
        int  started{0};
        [](bool&, int& s) -> tester {
            s = 17;
            co_return;
        }(destroyed, started);
        assert(destroyed);
        assert(started == 0);
    }
    {
        bool destroyed{false};
        int  started{0};
        bool called{false};

        auto t{[](bool&, int& s) -> tester {
            s = 17;
            co_await ::std::suspend_always{};
        }(destroyed, started)};
        assert(started == false);

        t.h.start(called);
        assert(started == 17);
        assert(called);
        assert(destroyed == false);

        t.h.reset();

        assert(destroyed);
    }
    {
        bool destroyed{false};
        {
            int  started{0};
            bool called{false};

            auto t{[](bool&, int& s) -> tester {
                s = 17;
                co_return;
            }(destroyed, started)};
            assert(started == false);

            t.h.start(called);
            assert(started == 17);
            assert(called);
            assert(destroyed == false);
        }
        assert(destroyed);
    }
}
