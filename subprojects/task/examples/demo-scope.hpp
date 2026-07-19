// examples/demo_scope.hpp                                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_EXAMPLES_DEMO_SCOPE
#define INCLUDED_EXAMPLES_DEMO_SCOPE

#include <beman/net/net.hpp>
#include <atomic>
#include <iostream>
#include <utility>

// ----------------------------------------------------------------------------

namespace demo {
namespace ex = ::beman::net::detail::ex;

class scope {
  private:
    static constexpr bool log_completions{false};
    struct env {
        scope* self;

        auto query(ex::get_stop_token_t) const noexcept { return this->self->source.get_token(); }
    };

    struct job_base {
        virtual ~job_base() = default;
    };

    struct receiver {
        using receiver_concept = ex::receiver_tag;
        scope*    self;
        job_base* state{};

        auto set_error(auto&&) noexcept -> void {
            ::std::cerr << "ERROR: demo::scope::job in scope completed with error!\n";
            this->complete();
        }
        auto set_value() && noexcept -> void {
            if (log_completions)
                std::cout << "demo::scope::set_value()\n";
            this->complete();
        }
        auto set_stopped() && noexcept -> void {
            if (log_completions)
                std::cout << "demo::scope::set_stopped()\n";
            this->complete();
        }
        auto complete() -> void {
            scope* slf{this->self};
            delete this->state;
            if (0u == --slf->count) {
                slf->complete();
            }
        }
        auto get_env() const noexcept -> env { return {this->self}; }
    };

    template <typename Sender>
    struct job : job_base {
        using state_t = decltype(ex::connect(std::declval<Sender&&>(), std::declval<receiver>()));
        state_t state;
        template <typename S>
        job(scope* self, S&& sender) : state(ex::connect(::std::forward<S>(sender), receiver{self, this})) {
            ex::start(this->state);
        }
    };

    std::atomic<std::size_t> count{};
    ex::inplace_stop_source  source;

    auto complete() -> void {}

  public:
    ~scope() {
        if (0u < this->count)
            std::cerr << "ERROR: scope destroyed with live jobs: " << this->count << "\n";
    }
    template <ex::sender Sender>
    auto spawn(Sender&& sender) {
        ++this->count;
        new job<Sender>(this, std::forward<Sender>(sender));
    }
    auto stop() -> void { this->source.request_stop(); }
    auto empty() const -> bool { return 0u == this->count; }
};
} // namespace demo

// ----------------------------------------------------------------------------

#endif
