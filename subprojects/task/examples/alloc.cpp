// examples/alloc.cpp                                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <array>
#include <iostream>
#include <memory_resource>
#include <cassert>
#include <cstdlib>
#include <memory_resource>
#include <beman/execution/execution.hpp>
#include <beman/task/task.hpp>

namespace ex = beman::execution;

#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define TSAN_ENABLED
#endif
#endif
#if !defined(TSAN_ENABLED)
void* operator new(std::size_t size) { // NOLINT(misc-no-recursion)
    void* pointer(std::malloc(size));  // NOLINT(hicpp-no-malloc)
    std::cout << "global new(" << size << ")->" << pointer << "\n";
    return pointer;
}
void operator delete(void* pointer) noexcept {
    std::cout << "global delete(" << pointer << ")\n";
    return std::free(pointer); // NOLINT(hicpp-no-malloc)
}
void operator delete(void* pointer, std::size_t size) noexcept {
    std::cout << "global delete(" << pointer << ", " << size << ")\n";
    return std::free(pointer); // NOLINT(hicpp-no-malloc)
}
#endif

template <std::size_t Size>
class fixed_resource : public std::pmr::memory_resource {
    std::array<std::byte, Size> buffer{};
    std::byte*                  free{this->buffer.data()};

    void* do_allocate(std::size_t size, std::size_t) override {
        if (size <= std::size_t(buffer.data() + Size - free)) {
            auto ptr{this->free};
            this->free += size;
            std::cout << "resource alloc(" << size << ")->" << ptr << "\n";
            return ptr;
        } else {
            return nullptr;
        }
    }
    void do_deallocate(void* ptr, std::size_t size, std::size_t) override {
        std::cout << "resource dealloc(" << size << "+" << sizeof(std::pmr::polymorphic_allocator<std::byte>) << ")->"
                  << ptr << "\n";
    }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return &other == this; }
};

struct alloc_aware {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
};

struct test_base {
    template <typename... Args>
    void* operator new(std::size_t size, Args&&...) {
        void* ptr{::operator new(size)};
        std::cout << "custom alloc(" << size << ") -> " << ptr << "\n";
        return ptr;
    }
    void operator delete(void* ptr, std::size_t size) {
        std::cout << "custom dealloc(" << ptr << ", " << size << ")\n";
        ::operator delete(ptr, size);
    }
};
struct test : test_base {};

int main() {
    std::cout << "running just\n";
    ex::sync_wait(ex::just(0));
    std::cout << "just done\n\n";

    std::cout << "running ex::task<void, alloc_aware>\n";
    ex::sync_wait([]() -> ex::task<void, alloc_aware> {
        co_await ex::just(0);
        co_await ex::just(0);
    }());
    std::cout << "ex::task<void, alloc_aware> done\n\n";

    fixed_resource<2048> resource;
    if constexpr (true) {
        std::cout << "running ex::task<void, alloc_aware> with alloc\n";
        ex::sync_wait([](auto&&...) -> ex::task<void, alloc_aware> {
            co_await ex::just(0);
            co_await ex::just(0);
        }(std::allocator_arg, &resource));
        std::cout << "ex::task<void, alloc_aware> with alloc done\n\n";
    }

    if constexpr (false) {
        std::cout << "running ex::task<void> with alloc\n";
        ex::sync_wait([](auto&&...) -> ex::task<void> {
            co_await ex::just(0);
            co_await ex::just(0);
        }(std::allocator_arg, &resource));
        std::cout << "ex::task<void> with alloc done\n\n";
    }

    if constexpr (false) {
        std::cout << "running ex::task<void, alloc_aware> extracting alloc\n";
        ex::sync_wait([](auto&&, [[maybe_unused]] auto* res) -> ex::task<void, alloc_aware> {
            auto alloc = co_await ex::read_env(ex::get_allocator);
            static_assert(std::same_as<std::pmr::polymorphic_allocator<std::byte>, decltype(alloc)>);
            assert(alloc == std::pmr::polymorphic_allocator<std::byte>(res));
        }(std::allocator_arg, &resource));
        std::cout << "ex::task<void, alloc_aware> extracting alloc done\n\n";
    }

    delete new test{};
}
