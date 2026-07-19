#include <coroutine>

struct task {
    struct final_awaiter {
        static constexpr bool await_ready() noexcept { return false; }
        static auto           await_suspend(std::coroutine_handle<> h) noexcept {
            h.destroy();
            return std::noop_coroutine();
        }
        static constexpr void await_resume() noexcept {}
    };
    struct promise_type {
        constexpr std::suspend_always initial_suspend() noexcept { return {}; }
        constexpr final_awaiter       final_suspend() noexcept { return {}; }
        void                          unhandled_exception() {}
        task get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
        void return_void() {}
    };
    std::coroutine_handle<> h;
};

int main() {
    []() -> task { co_return; }().h.resume();
}
