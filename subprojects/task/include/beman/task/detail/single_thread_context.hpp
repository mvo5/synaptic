// include/beman/task/detail/single_thread_context.hpp                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_SINGLE_THREAD_CONTEXT
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_SINGLE_THREAD_CONTEXT

#include <beman/execution/execution.hpp>
#include <functional>
#include <thread>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
class single_thread_context {
  private:
    ::beman::execution::run_loop loop;
    ::std::thread                thread{&single_thread_context::run, this};

    static auto run(single_thread_context* self) -> void { self->loop.run(); }

  public:
    single_thread_context() = default;
    ~single_thread_context() {
        this->finish();
        this->thread.join();
    }
    auto get_scheduler() { return this->loop.get_scheduler(); }
    void finish() { this->loop.finish(); }
};
} // namespace beman::task::detail
// ----------------------------------------------------------------------------

#endif
