// tests/beman/task/sub_visit.test.cpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/sub_visit.hpp>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

// ----------------------------------------------------------------------------

namespace {
template <typename>
struct type {
    int value;
};
} // namespace

int main() {
    std::variant<bool, int, type<bool>, type<int>> var{};

    int value{};
    beman::task::detail::sub_visit<2u>([&value]<typename T>(type<T>& v) { value = v.value; }, var);
    assert(value == 0);

    var.emplace<1>(17);
    beman::task::detail::sub_visit<2u>([&value]<typename T>(type<T>& v) { value = v.value; }, var);
    assert(value == 0);

    var.emplace<2>(type<bool>{42});
    beman::task::detail::sub_visit<2u>([&value]<typename T>(type<T>& v) { value = v.value; }, var);
    assert(value == 42);

    var.emplace<3>(type<int>{17});
    beman::task::detail::sub_visit<2u>([&value]<typename T>(type<T>& v) { value = v.value; }, var);
    assert(value == 17);
}
