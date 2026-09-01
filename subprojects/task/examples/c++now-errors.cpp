// examples/c++now-errors.cpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <expected>
#include <iostream>
#include <concepts>
#include <type_traits>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
struct none_t {};
std::ostream& operator<<(std::ostream& out, none_t) { return out << "<none>"; }

template <typename...>
struct identity_or_none;
template <>
struct identity_or_none<> {
    using type = none_t;
};
template <>
struct identity_or_none<void> {
    using type = none_t;
};
template <typename T>
struct identity_or_none<T> {
    using type = T;
};
template <typename... T>
using identity_or_none_t = typename identity_or_none<T...>::type;

#if 202202 <= __cpp_lib_expected
template <ex::sender Sender>
auto as_expected(Sender&& sndr) {
    using value_type  = ex::value_types_of_t<Sender, ex::env<>, std::tuple, identity_or_none_t>;
    using error_type  = ex::error_types_of_t<Sender, ex::env<>, identity_or_none_t>;
    using result_type = std::expected<value_type, error_type>;

    return std::forward<Sender>(sndr) |
           ex::then([]<typename T>(T&& x) noexcept { return result_type(std::forward<T>(x)); }) |
           ex::upon_error([]<typename T>(T&& x) noexcept { return result_type(std::unexpected(std::forward<T>(x))); });
}
#endif

void print_expected(const char* msg, const auto& e) {
    if (e) {
        if constexpr (std::same_as<none_t, std::remove_cvref_t<decltype(e.value())>>)
            std::cout << msg << " (value)<none>\n";
        else
            std::cout << msg << " (value)" << std::get<0>(e.value()) << "\n";
    } else
        std::cout << msg << " (error)" << e.error() << "\n";
}

// ----------------------------------------------------------------------------

ex::task<> error_result() {
    try {
        co_await ex::just_error(17);
    } catch (int n) {
        std::cout << "Error: " << n << "\n";
    }
}

#if 202202 <= __cpp_lib_expected
ex::task<> expected() {
    auto e = co_await as_expected(ex::just(17));
    print_expected("expected with value=", e);
    auto u = co_await as_expected(ex::just_error(17));
    print_expected("expected without value=", u);
}
#endif
} // namespace

int main() {
    ex::sync_wait(error_result());
#if 202202 <= __cpp_lib_expected
    ex::sync_wait(expected());
#endif
}
