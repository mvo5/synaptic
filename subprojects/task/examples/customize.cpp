// examples/customize.cpp                                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/task/task.hpp>
#include <iostream>
#include <utility>
#include <cassert>

namespace ex = beman::execution;

namespace {
struct empty {};
} // namespace

int main() {
#if 0
    auto task = []()->ex::task<void, empty> { co_return; }();
    static_assert(ex::sender<decltype(task)>);
    auto env = ex::get_env(task);
    static_assert(std::same_as<decltype(task)::env, decltype(env)>);
    auto dom = ex::get_domain(env);
    static_assert(std::same_as<decltype(task)::domain, decltype(dom)>);
    auto edom = ex::detail::get_domain_early(task);
    static_assert(std::same_as<decltype(task)::domain, decltype(edom)>);
    std::cout << "---\n";
    transform_sender(edom, ex::detail::make_sender(ex::affine_on, ex::inline_scheduler{}, std::move(task)));
    std::cout << "---\n";
    auto affine = ex::affine_on(std::move(task), ex::inline_scheduler{});
#endif
}
