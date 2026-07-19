// tests/beman/task/find_allocator.test.cpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/find_allocator.hpp>
#include <concepts>
#include <memory_resource>
#include <utility>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

// ----------------------------------------------------------------------------

namespace {
template <typename Expect, typename... Args>
void test_find(Expect expect, Args... args) {
    auto alloc{beman::task::detail::find_allocator<Expect>(std::forward<Args>(args)...)};
    static_assert(std::same_as<Expect, decltype(alloc)>);
    assert(expect == alloc);
}
} // namespace

int main() {
    using std_alloc  = std::allocator<char>;
    using poly_alloc = std::pmr::polymorphic_allocator<char>;

    test_find(std_alloc{});
    test_find(std_alloc{}, 0);
    test_find(std_alloc{}, 0, 1);
    // test_find(std_alloc{}, std::allocator_arg);
    // test_find(std_alloc{}, 0, std::allocator_arg, 1);
    test_find(std_alloc{}, std::allocator_arg, std_alloc{}, 0, 1);
    test_find(std_alloc{}, 0, std::allocator_arg, std_alloc{}, 1);
    test_find(std_alloc{}, 0, 1, std::allocator_arg, std_alloc{});

    test_find(poly_alloc{});
    test_find(poly_alloc{}, std::allocator_arg, poly_alloc{}, 0, 1);
    test_find(poly_alloc{}, std::allocator_arg, std::pmr::new_delete_resource(), 0, 1);
    test_find(
        poly_alloc{std::pmr::null_memory_resource()}, std::allocator_arg, std::pmr::null_memory_resource(), 0, 1);
    test_find(poly_alloc{}, 0, std::allocator_arg, poly_alloc{}, 0, 1);
    test_find(poly_alloc{}, 0, std::allocator_arg, std::pmr::new_delete_resource(), 0, 1);
    test_find(
        poly_alloc{std::pmr::null_memory_resource()}, 0, std::allocator_arg, std::pmr::null_memory_resource(), 0, 1);
}
