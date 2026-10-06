// examples/dangling-references.cpp                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/task/task.hpp>
#include <utility>
#include <functional>
#include <string>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
ex::task<int, ex::env<>>  do_work(std::string) { /* work */ co_return 0; };
ex::task<void, ex::env<>> execute_all() {
    co_await ex::when_all(do_work("arguments 1"), do_work("arguments 2"));
    co_return;
}

struct error_env {
    using error_types = ex::completion_signatures<ex::set_error_t(int)>;
};
} // namespace

int main() {
    ex::sync_wait([]() -> ex::task<ex::with_error<int>, error_env> { co_return ex::with_error<int>{42}; }());

    ex::sync_wait(execute_all());
    ex::sync_wait([]() -> ex::task<void, ex::env<>> {
        auto t = [](const int /* this would be added: &*/ v) -> ex::task<int, ex::env<>> { co_return v; }(42);
        [[maybe_unused]] auto v = co_await std::move(t);
    }());
}
