// examples/rvalue-task.cpp                                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/task/task.hpp>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
struct receiver {
    using receiver_concept = ex::receiver_tag;
    void set_value() && noexcept { std::cout << "set_value called\n"; }
    void set_error(std::exception_ptr) && noexcept { std::cout << "set_error called\n"; }
    void set_stopped() && noexcept { std::cout << "set_stopped called\n"; }

    struct env {
        auto query(const ex::get_scheduler_t&) const noexcept -> ex::inline_scheduler {
            return ex::inline_scheduler{};
        }
    };
    env get_env() const noexcept { return {}; }
};

template <typename T>
void test(T&& task) {
    if (requires { ex::connect(std::forward<T>(task), receiver{}); }) {
        //[[maybe_unused]] auto op_state = ex::connect(std::forward<T>(task), std::move(receiver{}));
    }
}
} // namespace

int main() {
    auto task = []() -> ex::task<void, ex::env<>> { co_return; }();
    test(std::move(task));
    // test(task);
}
