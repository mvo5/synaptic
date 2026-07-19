// examples/into_optional.cpp                                          -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <iostream>
#include <optional>
#include <tuple>
#include <system_error>
#include <beman/execution/execution.hpp>
#include <beman/task/task.hpp>

namespace ex = beman::execution;

namespace {
template <typename... S>
struct multi_sender {
    using sender_concept        = ex::sender_tag;
    using completion_signatures = ex::completion_signatures<S...>;

    template <typename Receiver>
    auto connect(Receiver&& receiver) const {
        return ex::connect(ex::just(), std::forward<Receiver>(receiver));
    }
};

template <typename T>
struct queue {
    multi_sender<ex::set_value_t(), ex::set_value_t(T, T)> async_pop() { return {}; }
};

template <typename... T, typename V>
auto to_optional(V& v) {
    using result_type = decltype([] {
        if constexpr (sizeof...(T) == 1u)
            return std::optional<T...>{};
        else
            return std::optional<std::tuple<T...>>{};
    }());
    return std::visit(
        []<typename... X>(std::tuple<X...>& a) {
            if constexpr (sizeof...(X) == 0u)
                return result_type{};
            else if constexpr (sizeof...(X) == 1u)
                return result_type(std::move(std::get<0>(a)));
            else
                return result_type(std::move(a));
        },
        v);
}

template <ex::sender S>
auto my_into_optional(S&& s) {
    return ex::into_variant(s) | ex::then([](auto&&...) { return std::optional<int>(); })
#if 0
     //-dk:TODO see if this code is needed or can be removed
          | ex::then([]<typename...A, typename... B>(
             std::variant<std::tuple<A...>, std::tuple<B...>> r) {
                 if constexpr (sizeof...(A) != 0u) {
                     return to_optional<A...>(r);
                 }
                 else {
                     return to_optional<B...>(r);
                 }
         })
#endif
        ;
}
} // namespace

int main() {
    queue<double> que;
    ex::sync_wait([](auto& q) -> ex::task<> {
        // auto x = co_await ex::just(true) | into_optional;
        [[maybe_unused]] std::optional x = co_await (q.async_pop() | ex::into_optional);
        [[maybe_unused]] std::optional y = co_await ex::into_optional(q.async_pop());
    }(que));
}
