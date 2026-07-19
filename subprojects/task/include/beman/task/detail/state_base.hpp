// include/beman/task/detail/state_base.hpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_STATE_BASE
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_STATE_BASE

#include <beman/task/detail/stop_source.hpp>
#include <beman/task/detail/scheduler_of.hpp>
#include <beman/task/detail/allocator_of.hpp>
#include <beman/task/detail/error_types_of.hpp>
#include <beman/task/detail/result_type.hpp>
#include <coroutine>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
template <typename Value, typename Environment>
class state_base : public ::beman::task::detail::result_type<::beman::task::detail::stoppable::yes,
                                                             Value,
                                                             ::beman::task::detail::error_types_of_t<Environment>> {
  public:
    using allocator_type   = ::beman::task::detail::allocator_of_t<Environment>;
    using stop_source_type = ::beman::task::detail::stop_source_of_t<Environment>;
    using stop_token_type  = decltype(std::declval<stop_source_type>().get_token());
    using scheduler_type   = ::beman::task::detail::scheduler_of_t<Environment>;

    auto complete() -> std::coroutine_handle<> { return this->do_complete(); }
    auto get_allocator() -> allocator_type { return this->do_get_allocator(); }
    auto get_stop_token() -> stop_token_type { return this->do_get_stop_token(); }
    auto get_environment() -> Environment& {
        assert(this);
        return this->do_get_environment();
    }
    auto get_scheduler() -> scheduler_type { return this->do_get_scheduler(); }
    auto set_scheduler(scheduler_type other) -> scheduler_type { return this->do_set_scheduler(other); }

  protected:
    template <::beman::execution::scheduler Scheduler, typename Env>
    static auto from_env(const Env& env) {
        if constexpr (requires { Scheduler(::beman::execution::get_scheduler(env)); }) {
            return Scheduler(::beman::execution::get_scheduler(env));
        } else {
            return Scheduler();
        }
    }

    // NOLINTBEGIN(portability-template-virtual-member-function)
    virtual auto do_complete() -> std::coroutine_handle<>                 = 0;
    virtual auto do_get_allocator() -> allocator_type                     = 0;
    virtual auto do_get_stop_token() -> stop_token_type                   = 0;
    virtual auto do_get_environment() -> Environment&                     = 0;
    virtual auto do_get_scheduler() -> scheduler_type                     = 0;
    virtual auto do_set_scheduler(scheduler_type other) -> scheduler_type = 0;
    // NOLINTEND(portability-template-virtual-member-function)

    virtual ~state_base() = default;
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
