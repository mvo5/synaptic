// tests/beman/task/allocator_of.test.cpp                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/allocator_of.hpp>
#include <memory_resource>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

// ----------------------------------------------------------------------------

namespace {
struct no_allocator {};
struct defines_allocator {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
};
struct defines_wrong_allocator {
    using allocator_type = std::pmr::polymorphic_allocator<char>;
};
struct non_allocator {};
struct defines_non_allocator {
    using allocator_type = non_allocator;
};
} // namespace

int main() {
    static_assert(std::same_as<std::allocator<std::byte>, beman::task::detail::allocator_of_t<no_allocator>>);
    static_assert(std::same_as<std::pmr::polymorphic_allocator<std::byte>,
                               beman::task::detail::allocator_of_t<defines_allocator>>);
    // using type = beman::task::detail::allocator_of_t<defines_wrong_allocator>;
    // using type = beman::task::detail::allocator_of_t<defines_non_allocator>;
}
