// include/beman/task/detail/state.hpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_STATE
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_STATE

#include <beman/task/detail/promise_type.hpp>
#include <beman/task/detail/state_base.hpp>
#include <beman/task/detail/state_rep.hpp>
#include <beman/task/detail/stop_source.hpp>
#include <type_traits>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::task::detail {

template <typename Task, typename T, typename C, typename Receiver>
struct state : ::beman::task::detail::state_base<T, C>, ::beman::task::detail::state_rep<C, Receiver> {
    using operation_state_concept = ::beman::execution::operation_state_tag;
    using promise_type            = ::beman::task::detail::promise_type<Task, T, C>;
    using scheduler_type          = typename ::beman::task::detail::state_base<T, C>::scheduler_type;
    using allocator_type          = typename ::beman::task::detail::state_base<T, C>::allocator_type;
    using stop_source_type        = ::beman::task::detail::stop_source_of_t<C>;
    using stop_token_type         = decltype(std::declval<stop_source_type>().get_token());
    using stop_token_t =
        decltype(::beman::execution::get_stop_token(::beman::execution::get_env(std::declval<Receiver>())));
    struct stop_link {
        stop_source_type& source;
        void              operator()() const noexcept { source.request_stop(); }
    };
    using stop_callback_t = ::beman::execution::stop_callback_for_t<stop_token_t, stop_link>;
    template <typename R, typename H>
    state(R&& r, H h) noexcept //-dk:TODO break down to various members
        : state_rep<C, Receiver>(std::forward<R>(r)),
          handle(std::move(h)),
          scheduler(this->template from_env<scheduler_type>(::beman::execution::get_env(this->receiver))) {}

    ::beman::task::detail::handle<promise_type> handle;
    stop_source_type                            source;
    std::optional<stop_callback_t>              stop_callback;
    scheduler_type                              scheduler;

    auto                    start() & noexcept -> void { this->handle.start(this).resume(); }
    std::coroutine_handle<> do_complete() override {
        this->handle.reset();
        this->result_complete(::std::move(this->receiver));
        return std::noop_coroutine();
    }
    auto do_get_allocator() -> allocator_type override {
        if constexpr (requires {
                          allocator_type(
                              ::beman::execution::get_allocator(::beman::execution::get_env(this->receiver)));
                      })
            return allocator_type(::beman::execution::get_allocator(::beman::execution::get_env(this->receiver)));
        else
            return allocator_type{};
    }
    auto do_get_scheduler() -> scheduler_type override { return this->scheduler; }
    auto do_set_scheduler(scheduler_type other) -> scheduler_type override {
        return ::std::exchange(this->scheduler, other);
    }
    stop_token_type do_get_stop_token() override {
        if (this->source.stop_possible() && not this->stop_callback) {
            this->stop_callback.emplace(
                ::beman::execution::get_stop_token(::beman::execution::get_env(this->receiver)),
                stop_link{this->source});
        }
        return this->source.get_token();
    }
    C& do_get_environment() override { return this->context; }
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
