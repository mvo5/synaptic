// examples/task_sender.hpp                                           -*-C++-*-
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

void* operator new(std::size_t n) {
    auto p = std::malloc(n);
    std::cout << "global new(" << n << ")->" << p << "\n";
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
                             [&task = this->task, ... a = std::forward<Arg>(arg)](auto alloc) {
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

struct env {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
};

auto lambda{[](int i, auto&&...) -> ex::task<void, env> {
    auto alloc = co_await ex::read_env(ex::get_allocator);
    alloc.deallocate(alloc.allocate(1), 1);
    std::cout << "lambda(" << i << ")\n";
    co_return;
}};

class example {
    ex::task<void, env> member_(std::allocator_arg_t, std::pmr::polymorphic_allocator<std::byte>, int);
    ex::task<void, env> const_member_(std::allocator_arg_t, std::pmr::polymorphic_allocator<std::byte>, int) const;

  public:
    auto member(int i) { return defer_frame(&example::member_, this)(i); }
    auto const_member(int i) { return defer_frame(&example::const_member_, this)(i); }
};

inline ex::task<void, env> example::member_(std::allocator_arg_t, std::pmr::polymorphic_allocator<std::byte>, int i) {
    std::cout << "example::member(" << i << ")\n";
    co_return;
}
inline ex::task<void, env>
example::const_member_(std::allocator_arg_t, std::pmr::polymorphic_allocator<std::byte>, int i) const {
    std::cout << "example::const member(" << i << ")\n";
    co_return;
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

int main() {
    resource                                   res{};
    std::pmr::polymorphic_allocator<std::byte> alloc(&res);
    std::cout << "direct allocator use:\n";
    alloc.deallocate(alloc.allocate(1), 1);
    std::cout << "write_env/just/then use:\n";
    ex::sync_wait(ex::write_env(ex::just(alloc) | ex::then([](auto a) { a.deallocate(a.allocate(1), 1); }),
                                ex::env{ex::prop{ex::get_allocator, alloc}}));
    std::cout << "write_env/read_env/then use:\n";
    ex::sync_wait(
        ex::write_env(ex::read_env(ex::get_allocator) | ex::then([](auto a) { a.deallocate(a.allocate(1), 1); }),
                      ex::env{ex::prop{ex::get_allocator, alloc}}));
    std::cout << "write_env/let_value/then use:\n";
    ex::sync_wait(ex::write_env(ex::just() | ex::let_value([] {
                                    return ex::read_env(ex::get_allocator) |
                                           ex::then([](auto a) { a.deallocate(a.allocate(1), 1); });
                                }),
                                ex::env{ex::prop{ex::get_allocator, alloc}}));
    std::cout << "write_env/task<>:\n";
    ex::sync_wait(ex::write_env(
        []() -> ex::task<> {
            auto a = co_await ex::read_env(ex::get_allocator);
            a.deallocate(a.allocate(1), 1);
        }(),
        ex::env{ex::prop{ex::get_allocator, alloc}}));
    std::cout << "write_env/task<void, env>:\n";
    ex::sync_wait(ex::write_env(
        [](auto&&...) -> ex::task<void, env> {
            auto a = co_await ex::read_env(ex::get_allocator);
            a.deallocate(a.allocate(1), 1);
        }(std::allocator_arg, alloc),
        ex::env{ex::prop{ex::get_allocator, alloc}}));
    std::cout << "write_env/defer_frame<task<void, env>>:\n";
    static constexpr defer_frame t0([](auto, auto, int i) -> ex::task<void, env> {
        std::cout << "  i=" << i << "\n";
        auto a = co_await ex::read_env(ex::get_allocator);
        a.deallocate(a.allocate(1), 1);
    });
    ex::sync_wait(ex::write_env(t0(17), ex::env{ex::prop{ex::get_allocator, alloc}}));
    std::cout << "write_env/temporary defer_frame<task<void, env>>:\n";
    ex::sync_wait(ex::write_env(defer_frame([](auto, auto, auto i) -> ex::task<void, env> {
                                    std::cout << "  i=" << i << "\n";
                                    co_await std::suspend_never{};
                                    auto a = co_await ex::read_env(ex::get_allocator);
                                    a.deallocate(a.allocate(1), 1);
                                })(42),
                                ex::env{ex::prop{ex::get_allocator, alloc}}));
    std::cout << "done\n";
}
