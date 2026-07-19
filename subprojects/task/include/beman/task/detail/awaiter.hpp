// include/beman/task/detail/awaiter.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_AWAITER
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_AWAITER

#include <beman/task/detail/handle.hpp>
#include <beman/task/detail/state_base.hpp>
#include <beman/task/detail/state_rep.hpp>
#include <coroutine>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
template <typename Awaiter>
struct awaiter_scheduler_receiver {
    using receiver_concept = ::beman::execution::receiver_tag;
    Awaiter* aw;
    auto     set_value(auto&&...) noexcept { this->aw->actual_complete().resume(); }
    auto     set_error(auto&&) noexcept { this->aw->actual_complete().resume(); }
    auto     set_stopped() noexcept { this->aw->actual_complete().resume(); }
};

template <typename Awaiter,
          typename ParentPromise,
          bool =
              requires(const ParentPromise& p) { ::beman::execution::get_scheduler(::beman::execution::get_env(p)); }>
struct awaiter_op_t {
    using state_type =
        decltype(::beman::execution::connect(::beman::execution::schedule(::beman::execution::get_scheduler(
                                                 ::beman::execution::get_env(::std::declval<const ParentPromise&>()))),
                                             ::std::declval<awaiter_scheduler_receiver<Awaiter>>()));

    awaiter_op_t(const ParentPromise& p, Awaiter* aw)
        : state(::beman::execution::connect(
              ::beman::execution::schedule(beman::execution::get_scheduler(::beman::execution::get_env(p))),
              awaiter_scheduler_receiver<Awaiter>{aw})) {}
    state_type state;
    auto       start() noexcept -> void { ::beman::execution::start(this->state); }
};
template <typename Awaiter, typename ParentPromise>
struct awaiter_op_t<Awaiter, ParentPromise, false> {
    awaiter_op_t(const ParentPromise&, Awaiter*) noexcept {}
    auto start() noexcept -> void {}
};

template <typename Value, typename Env, typename OwnPromise, typename ParentPromise>
class awaiter : public ::beman::task::detail::state_base<Value, Env> {
  public:
    using allocator_type  = typename ::beman::task::detail::state_base<Value, Env>::allocator_type;
    using stop_token_type = typename ::beman::task::detail::state_base<Value, Env>::stop_token_type;
    using scheduler_type  = typename ::beman::task::detail::state_base<Value, Env>::scheduler_type;

    explicit awaiter(::beman::task::detail::handle<OwnPromise> h) : handle(std::move(h)) {}
    constexpr auto await_ready() const noexcept -> bool { return false; }
    struct env_receiver {
        ParentPromise* parent;
        auto           get_env() const noexcept { return parent->get_env(); }
    };
    auto await_suspend(::std::coroutine_handle<ParentPromise> parent) noexcept {
        this->state_rep.emplace(env_receiver{&parent.promise()});
        this->scheduler.emplace(
            this->template from_env<scheduler_type>(::beman::execution::get_env(parent.promise())));
        this->parent = ::std::move(parent);
        return this->handle.start(this);
    }
    auto await_resume() { return this->result_resume(); }

  private:
    friend struct awaiter_scheduler_receiver<awaiter>;
    auto do_complete() -> std::coroutine_handle<> override {
        assert(this->parent);
        assert(this->scheduler);
        if constexpr (requires {
                          *this->scheduler !=
                              ::beman::execution::get_scheduler(::beman::execution::get_env(this->parent.promise()));
                      }) {
            if (*this->scheduler !=
                ::beman::execution::get_scheduler(::beman::execution::get_env(this->parent.promise()))) {
                this->reschedule.emplace(this->parent.promise(), this);
                this->reschedule->start();
                return ::std::noop_coroutine();
            }
        }
        return this->actual_complete();
    }
    auto actual_complete() -> std::coroutine_handle<> {
        return this->no_completion_set() ? this->parent.promise().unhandled_stopped() : ::std::move(this->parent);
    }
    auto do_get_allocator() -> allocator_type override {
        if constexpr (requires {
                          ::beman::execution::get_allocator(::beman::execution::get_env(this->parent.promise()));
                      })
            return ::beman::execution::get_allocator(::beman::execution::get_env(this->parent.promise()));
        else
            return allocator_type{};
    }
    auto do_get_scheduler() -> scheduler_type override { return *this->scheduler; }
    auto do_set_scheduler(scheduler_type other) -> scheduler_type override {
        return ::std::exchange(*this->scheduler, other);
    }
    auto do_get_stop_token() -> stop_token_type override { return {}; }
    auto do_get_environment() -> Env& override { return this->state_rep->context; }

    ::beman::task::detail::handle<OwnPromise>                            handle;
    ::std::optional<::beman::task::detail::state_rep<Env, env_receiver>> state_rep;
    ::std::optional<scheduler_type>                                      scheduler;
    ::std::coroutine_handle<ParentPromise>                               parent{};
    ::std::optional<awaiter_op_t<awaiter, ParentPromise>>                reschedule{};
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
