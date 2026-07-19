// include/beman/task/detail/task_scheduler.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_TASK_DETAIL_task_scheduler
#define INCLUDED_BEMAN_TASK_DETAIL_task_scheduler

#include <beman/execution/execution.hpp>
#include <beman/task/detail/infallible_scheduler.hpp>
#include <beman/task/detail/poly.hpp>
#include <new>
#include <optional>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::task::detail {

/*!
 * \brief Type-erasing scheduler
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 *
 * The class `task_scheduler` is used to type-erase any scheduler class.
 * Any error produced by the underlying scheduler except `std::error_code` is turned into
 * an `std::exception_ptr`. `std::error_code` is forwarded as is. The `task_scheduler`
 * forwards stop requests reported by the stop token obtained from the `connect`ed
 * receiver to the sender used by the underlying scheduler.
 *
 * Completion signatures:
 *
 * - `ex::set_value_t()`
 * - `ex::set_stopped()`
 *
 * Usage:
 *
 *     task_scheduler sched(other_scheduler);
 *     auto sender{ex::schedule(sched) | some_sender};
 */
class task_scheduler {
    struct state_base {
        virtual ~state_base()         = default;
        virtual void complete_value() = 0;
    };

    struct inner_state {
        struct receiver;
        struct receiver {
            using receiver_concept = ::beman::execution::receiver_tag;
            state_base* state;
            void        set_value() && noexcept { this->state->complete_value(); }
        };
        static_assert(::beman::execution::receiver<receiver>);

        struct base {
            virtual ~base()      = default;
            virtual void start() = 0;
        };
        template <::beman::execution::sender Sender>
        struct concrete : base {
            using state_t = decltype(::beman::execution::connect(std::declval<Sender>(), std::declval<receiver>()));
            state_t state;
            template <::beman::execution::sender S>
            concrete(S&& s, state_base* b) : state(::beman::execution::connect(std::forward<S>(s), receiver{b})) {}
            void start() override { ::beman::execution::start(state); }
        };
        ::beman::task::detail::poly<base, 16u * sizeof(void*)> state;
        template <::beman::execution::sender S>
        inner_state(S&& s, state_base* b) : state(static_cast<concrete<S>*>(nullptr), std::forward<S>(s), b) {}
        void start() { this->state->start(); }
    };

    template <::beman::execution::receiver Receiver>
    struct state : state_base {
        using operation_state_concept = ::beman::execution::operation_state_tag;
        std::remove_cvref_t<Receiver> receiver;
        inner_state                   s;

        template <::beman::execution::receiver R, typename PS>
        state(R&& r, PS& ps) : receiver(std::forward<R>(r)), s(ps->connect(this)) {}
        void start() & noexcept { this->s.start(); }
        void complete_value() override { ::beman::execution::set_value(std::move(this->receiver)); }
    };

    class sender;
    class env {
        friend class sender;

      private:
        const sender* sndr;
        env(const sender* s) : sndr(s) {}

      public:
        task_scheduler
        query(const ::beman::execution::get_completion_scheduler_t<::beman::execution::set_value_t>&) const noexcept {
            return this->sndr->inner_sender->get_completion_scheduler();
        }
    };

    // sender implementation
    class sender {
        friend class env;

      private:
        struct base {
            virtual ~base()                                         = default;
            virtual base*          move(void*)                      = 0;
            virtual base*          clone(void*) const               = 0;
            virtual inner_state    connect(state_base*)             = 0;
            virtual task_scheduler get_completion_scheduler() const = 0;
        };
        template <::beman::execution::scheduler Scheduler>
        struct concrete : base {
            using sender_tag = decltype(::beman::execution::schedule(std::declval<Scheduler>()));
            sender_tag sender;

            template <::beman::execution::scheduler S>
            concrete(S&& s) : sender(::beman::execution::schedule(std::forward<S>(s))) {}
            base*          move(void* buffer) override { return new (buffer) concrete(std::move(*this)); }
            base*          clone(void* buffer) const override { return new (buffer) concrete(*this); }
            inner_state    connect(state_base* b) override { return inner_state(::std::move(sender), b); }
            task_scheduler get_completion_scheduler() const override {
                return task_scheduler(::beman::execution::get_completion_scheduler<::beman::execution::set_value_t>(
                    ::beman::execution::get_env(this->sender)));
            }
        };
        poly<base, 4 * sizeof(void*)> inner_sender;

      public:
        using sender_concept        = ::beman::execution::sender_tag;
        using completion_signatures = ::beman::execution::completion_signatures<::beman::execution::set_value_t()>;
        template <typename...>
        static consteval auto get_completion_signatures() noexcept -> completion_signatures {
            return {};
        }

        template <::beman::execution::scheduler S>
        explicit sender(S&& s) : inner_sender(static_cast<concrete<S>*>(nullptr), std::forward<S>(s)) {}
        sender(sender&&)      = default;
        sender(const sender&) = default;

        template <::beman::execution::receiver R>
        state<R> connect(R&& r) {
            return state<R>(std::forward<R>(r), this->inner_sender);
        }

        env get_env() const noexcept { return env(this); }
    };

    // scheduler implementation
    struct base {
        virtual ~base()                          = default;
        virtual sender schedule()                = 0;
        virtual base*  move(void* buffer)        = 0;
        virtual base*  clone(void*) const        = 0;
        virtual bool   equals(const base*) const = 0;
    };
    template <::beman::execution::scheduler Scheduler>
    struct concrete : base {
        Scheduler scheduler;
        template <typename S>
            requires ::beman::execution::scheduler<::std::remove_cvref_t<S>>
        explicit concrete(S&& s) : scheduler(std::forward<S>(s)) {}
        sender schedule() override { return sender(this->scheduler); }
        base*  move(void* buffer) override { return new (buffer) concrete(std::move(*this)); }
        base*  clone(void* buffer) const override { return new (buffer) concrete(*this); }
        bool   equals(const base* o) const override {
            auto other{dynamic_cast<const concrete*>(o)};
            return other ? this->scheduler == other->scheduler : false;
        }
    };

    poly<base, 4 * sizeof(void*)> scheduler;

  public:
    using scheduler_concept = ::beman::execution::scheduler_tag;

    template <typename S, typename Allocator = ::std::allocator<void>>
        requires(not std::same_as<task_scheduler, std::remove_cvref_t<S>>) &&
                ::beman::execution::scheduler<::std::remove_cvref_t<S>> &&
                ::beman::task::detail::infallible_scheduler<::std::remove_cvref_t<S>, ::beman::execution::env<>>
    explicit task_scheduler(S&& s, Allocator = {})
        : scheduler(static_cast<concrete<std::decay_t<S>>*>(nullptr), std::forward<S>(s)) {}
    task_scheduler(task_scheduler&&)      = default;
    task_scheduler(const task_scheduler&) = default;
    template <typename Allocator>
    task_scheduler(const task_scheduler& other, Allocator) : scheduler(other.scheduler) {}
    task_scheduler& operator=(const task_scheduler&) = default;
    ~task_scheduler()                                = default;

    sender schedule() { return this->scheduler->schedule(); }
    bool   operator==(const task_scheduler&) const = default;
    template <typename Sched>
        requires(not ::std::same_as<task_scheduler, Sched>) && ::beman::execution::scheduler<Sched>
    bool operator==(const Sched& other [[maybe_unused]]) const {
        return *this == task_scheduler(other);
    }
};
static_assert(::beman::execution::scheduler<task_scheduler>);

} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
