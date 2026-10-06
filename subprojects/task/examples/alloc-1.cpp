// examples/alloc-1.cpp                                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <new>
#include <cstdlib>
#include <utility>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------
// --- defer_frame turns yields a sender calling a coroutine upon start()   ---

template <typename Mem, typename Self = void>
struct defer_frame {
    Mem  mem;
    Self self;
    template <typename... Arg>
    auto operator()(Arg&&... arg) const {
        return ex::let_value(ex::read_env(ex::get_allocator),
                             [mem = this->mem, self = this->self, ... a = std::forward<Arg>(arg)](auto alloc) {
                                 return std::invoke(mem, self, std::allocator_arg, alloc, std::move(a)...);
                             });
    }
    template <typename Alloc, typename... Arg>
    auto operator()(::std::allocator_arg_t, Alloc alloc, Arg&&... arg) const {
        return ex::let_value(ex::just(alloc),
                             [&mem = this->mem, self = this->self, ... a = std::forward<Arg>(arg)](auto alloc) {
                                 return std::invoke(mem, self, std::allocator_arg, alloc, std::move(a)...);
                             });
    }
    auto operator()(::std::allocator_arg_t) const = delete;
};

template <typename Task>
struct defer_frame<Task, void> {
    Task task;
    template <typename... Arg>
    auto operator()(Arg&&... arg) const {
        return ex::let_value(ex::read_env(ex::get_allocator),
                             [task = this->task, ... a = std::forward<Arg>(arg)](auto alloc) {
                                 return std::invoke(task, std::allocator_arg, alloc, std::move(a)...);
                             });
    }
    template <typename Alloc, typename... Arg>
    auto operator()(::std::allocator_arg_t, Alloc alloc, Arg&&... arg) const {
        return ex::let_value(ex::just(alloc), [&task = this->task, ... a = std::forward<Arg>(arg)](auto alloc) {
            return std::invoke(task, std::allocator_arg, alloc, std::move(a)...);
        });
    }
    auto operator()(::std::allocator_arg_t) const = delete;
};

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

using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
struct alloc_env {
    using allocator_type = ::allocator_type;
};
template <typename T = void>
using a_task = ex::task<T, alloc_env>;

a_task<int> hidden_async_fun(std::allocator_arg_t, ::allocator_type, int value) { co_return value; }
auto        async_fun(int value) { return defer_frame(&hidden_async_fun)(value); }

int main() {
    std::cout << std::unitbuf;
    resource       res{};
    allocator_type alloc(&res);

    std::cout << "not setting up an allocator:\n";
    ex::sync_wait([]() -> a_task<> {
        auto result{co_await async_fun(17)};
        std::cout << "    result=" << result << "\n";
    }());

    std::cout << "setting up an allocator:\n";
    ex::sync_wait(ex::write_env(
        []() -> a_task<> {
            auto result{co_await async_fun(17)};
            std::cout << "    result=" << result << "\n";
        }(),
        ex::env{ex::prop{ex::get_allocator, alloc}}));

    std::cout << "setting up an allocator and using defer_frame:\n";
    ex::sync_wait(ex::write_env(defer_frame([](auto, auto) -> a_task<> {
                                    auto result{co_await async_fun(17)};
                                    std::cout << "    result=" << result << "\n";
                                })(),
                                ex::env{ex::prop{ex::get_allocator, alloc}}));
}
