// tests/beman/task/allocator_support.test.cpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/allocator_support.hpp>
#include <memory_resource>
#include <new>
#include <cstddef>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#ifdef _MSC_VER
#pragma warning(disable : 4291)
#endif

// ----------------------------------------------------------------------------

namespace {
struct test_resource : std::pmr::memory_resource {
    std::size_t outstanding{};

    void* do_allocate(std::size_t size, std::size_t) override {
        this->outstanding += size;
        return ::operator new(size);
    }
    void do_deallocate(void* ptr, std::size_t size, std::size_t) override {
        ::operator delete(ptr);

        this->outstanding -= size;
    }
    bool do_is_equal(const memory_resource& other) const noexcept override {
        auto* tr{dynamic_cast<const test_resource*>(&other)};
        return tr == this;
    }
};

struct some_data {
    double data{};
};

template <typename Allocator>
struct allocator_aware : some_data, beman::task::detail::allocator_support<Allocator> {
    allocator_aware() : some_data() {}
};
} // namespace

int main() {
    using type = allocator_aware<std::pmr::polymorphic_allocator<std::byte>>;
    [[maybe_unused]] std::unique_ptr<type> unused(new type{});

    test_resource resource{};
    assert(resource.outstanding == 0u);
    type* ptr{new (std::allocator_arg, &resource) type{}};
    assert(resource.outstanding != 0u);
    ptr->~type();
    assert(resource.outstanding != 0u);
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif
    type::operator delete(ptr, sizeof(type), std::allocator_arg, &resource);
    assert(resource.outstanding == 0u);
}
