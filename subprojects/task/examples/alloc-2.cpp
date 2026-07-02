// examples/alloc-1.cpp                                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <new>
#include <cstdlib>
#include <utility>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

struct tls_allocator : std::pmr::polymorphic_allocator<std::byte> {
    thread_local static std::pmr::memory_resource* alloc;

    tls_allocator() : std::pmr::polymorphic_allocator<std::byte>(alloc) {}

    static void set(std::pmr::memory_resource* a) { alloc = a; }
};
thread_local std::pmr::memory_resource* tls_allocator::alloc{std::pmr::new_delete_resource()};

// ----------------------------------------------------------------------------

void* operator new(std::size_t n) {
    auto p = std::malloc(n);
    std::cout << "    global new(" << n << ")->" << p << "\n";
    return p;
}
void operator delete(void* ptr) noexcept {
    std::cout << "    global operator delete(" << ptr << ")\n";
    std::free(ptr);
}
void operator delete(void* ptr, std::size_t size) noexcept {
    std::cout << "    global operator delete(" << ptr << ", " << size << ")\n";
    std::free(ptr);
}

struct resource : std::pmr::memory_resource {
    void* do_allocate(std::size_t n, std::size_t) override {
        auto p{std::malloc(n)};
        std::cout << "    resource::allocate(" << n << ")->" << p << "\n";
        return p;
    }
    void do_deallocate(void* p, std::size_t n, std::size_t) override {
        std::cout << "    resource::deallocate(" << p << ", " << n << ")\n";
        std::free(p);
    }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
};

// ----------------------------------------------------------------------------

using allocator_type = tls_allocator;
struct alloc_env {
    using allocator_type = ::allocator_type;
};
template <typename T = void>
using a_task = ex::task<T, alloc_env>;

a_task<int> async_fun(int value) { co_return value; }

int main(int ac, char*[]) {
    resource res{};

    std::cout << "not setting up an allocator:\n";
    ex::sync_wait([ac]() -> a_task<> {
        auto result{co_await async_fun(ac)};
        std::cout << "    result=" << result << "\n";
    }());

    std::cout << "setting up an allocator:\n";
    tls_allocator::set(&res);
    ex::sync_wait([ac]() -> a_task<> {
        auto result{co_await async_fun(ac)};
        std::cout << "    result=" << result << "\n";
    }());
}
