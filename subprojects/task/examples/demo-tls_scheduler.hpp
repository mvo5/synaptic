// examples/demo-tls_scheduler.hpp                                    -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_EXAMPLES_DEMO_TLS_SCHEDULER
#define INCLUDED_EXAMPLES_DEMO_TLS_SCHEDULER

#include <beman/execution/execution.hpp>
#include <beman/execution/task.hpp>
#include <utility>

// ----------------------------------------------------------------------------

namespace demo {
struct tls_domain {
    template <::beman::execution::receiver Rcvr, typename Data>
    struct affine_state_base {
        std::remove_cvref_t<Rcvr> rcvr;
        Data                      data{};

        auto complete() {
            std::cout << "affine_state::complete\n";
            this->data.restore();
            ::beman::execution::set_value(::std::move(this->rcvr));
        }
    };

    template <::beman::execution::receiver Rcvr, typename Data>
    struct affine_receiver {
        using receiver_concept = ::beman::execution::receiver_tag;
        struct env_t {
            affine_state_base<Rcvr, Data>* st;
            template <typename Q, typename... A>
                requires requires(affine_state_base<Rcvr, Data>* self, Q&& q, A&&... a) {
                    std::forward<Q>(q)(::beman::execution::get_env(self->st->rcvr), ::std::forward<A>(a)...);
                }
            auto query(Q&& q, A&&... a) const noexcept {
                std::forward<Q>(q)(::beman::execution::get_env(this->st->rcvr, ::std::forward<A>(a)...));
            }
        };
        affine_state_base<Rcvr, Data>* st;

        template <typename E>
        auto set_error(E&& e) && noexcept -> void {
            ::beman::execution::set_error(::std::move(this->st->rcvr), ::std::forward<E>(e));
        }
        auto set_stopped() && noexcept -> void { ::beman::execution::set_stopped(::std::move(this->st->rcvr)); }
        auto set_value() && noexcept -> void { this->st->complete(); }

        auto get_env() const noexcept -> env_t { return {this->st}; }
    };

    template <::beman::execution::sender Sndr, ::beman::execution::scheduler Sch, ::beman::execution::receiver Rcvr>
    struct affine_state : affine_state_base<Rcvr, typename Sch::type> {
        using operation_state_concept = ::beman::execution::operation_state_tag;
        using env_t                   = decltype(::beman::execution::get_env(std::declval<Rcvr>()));
        using base_t                  = affine_state_base<Rcvr, typename Sch::type>;
        using data_t                  = typename Sch::type;
        using state_t                 = decltype(::beman::execution::connect(
            ::beman::execution::affine_on(::std::declval<Sndr>(), ::std::declval<Sch>()),
            ::std::declval<affine_receiver<Rcvr, data_t>>()));

        state_t state;

        template <::beman::execution::sender S, ::beman::execution::scheduler SC, ::beman::execution::receiver R>
        affine_state(S&& s, SC&& sc, R&& r)
            : base_t{::std::forward<R>(r)},
              state(::beman::execution::connect(
                  ::beman::execution::affine_on(::std::forward<S>(s), ::std::forward<SC>(sc)),
                  affine_receiver<Rcvr, data_t>{this})) {}
        auto start() & noexcept -> void {
            std::cout << "affine_state::start\n";
            this->data.save();
            ::beman::execution::start(this->state);
        }
    };
    template <::beman::execution::sender Sndr, ::beman::execution::scheduler Sch>
    struct affine_sender {
        using sender_concept = ::beman::execution::sender_tag;

        std::remove_cvref_t<Sndr> sndr;
        std::remove_cvref_t<Sch>  sch;

        template <typename Env>
        auto get_completion_signatures(const Env& env) const noexcept {
            return ::beman::execution::get_completion_signatures(this->sndr, env);
        }
        template <::beman::execution::receiver Rcvr>
        auto connect(Rcvr&& rcvr) const& {
            return affine_state<Sndr, Sch, Rcvr>(sndr, sch, std::forward<Rcvr>(rcvr));
        }
        template <::beman::execution::receiver Rcvr>
        auto connect(Rcvr&& rcvr) && {
            return affine_state<Sndr, Sch, Rcvr>(std::move(sndr), std::move(sch), std::forward<Rcvr>(rcvr));
        }
    };
    template <::beman::execution::sender Sndr, typename... Env>
        requires std::same_as<::beman::execution::tag_of_t<::std::remove_cvref_t<Sndr>>,
                              ::beman::execution::affine_on_t>
    auto transform_sender(Sndr&& s, const Env&...) const noexcept {
        auto [tag, sch, sndr] = s;
        return affine_sender<decltype(sndr), decltype(sch)>{::beman::execution::detail::forward_like<Sndr>(sndr),
                                                            ::beman::execution::detail::forward_like<Sndr>(sch)};
    }
};

template <typename Data, ::beman::execution::scheduler Scheduler>
struct tls_scheduler {
    using scheduler_concept = ::beman::execution::scheduler_tag;
    using type              = Data;
    struct env {
        Scheduler sched;
        template <typename Tag>
        auto query(const ::beman::execution::get_completion_scheduler_t<Tag>&) const noexcept -> tls_scheduler {
            return {this->sched};
        }
    };
    template <::beman::execution::receiver Receiver>
    struct state {
        using operation_state_concept = ::beman::execution::operation_state_tag;
        using upstream_state_t        = decltype(::beman::execution::connect(
            ::beman::execution::schedule(std::declval<Scheduler>()), std::declval<std::remove_cvref_t<Receiver>>()));

        upstream_state_t upstream;

        template <::beman::execution::sender Sndr, ::beman::execution::receiver Rcvr>
        state(Sndr&& sndr, Rcvr&& rcvr)
            : upstream(::beman::execution::connect(std::forward<Sndr>(sndr), std::forward<Rcvr>(rcvr))) {}
        auto start() & noexcept -> void { ::beman::execution::start(this->upstream); }
    };
    struct tls_sender {
        using sender_concept = ::beman::execution::sender_tag;
        using sender_type    = decltype(::beman::execution::schedule(std::declval<Scheduler>()));

        Scheduler sched;

        template <typename... Env>
        auto get_completion_signatures(const Env&... env) const noexcept {
            return ::beman::execution::get_completion_signatures(::beman::execution::schedule(Scheduler(this->sched)),
                                                                 env...);
        }
        auto get_env() const noexcept -> env { return {this->sched}; }

        template <::beman::execution::receiver Receiver>
        auto connect(Receiver&& rcvr) -> state<Receiver> {
            return state<Receiver>(::beman::execution::schedule(this->sched), std::forward<Receiver>(rcvr));
        }
    };

    Scheduler sched;
    template <typename Sch>
        requires(!std::same_as<tls_scheduler, std::remove_cvref_t<Sch>>)
    tls_scheduler(Sch&& sch) : sched(sch) {}
    tls_scheduler(const tls_scheduler&)            = default;
    tls_scheduler(tls_scheduler&&)                 = default;
    tls_scheduler& operator=(const tls_scheduler&) = default;
    tls_scheduler& operator=(tls_scheduler&&)      = default;

    auto schedule() -> tls_sender { return {this->sched}; }

    auto query(const ::beman::execution::get_domain_t&) const noexcept -> tls_domain { return {}; }

    auto operator==(const tls_scheduler&) const -> bool = default;
};
static_assert(::beman::execution::scheduler<
              tls_scheduler<int, decltype(std::declval<::beman::execution::run_loop>().get_scheduler())>>);

} // namespace demo

// ----------------------------------------------------------------------------

#endif
