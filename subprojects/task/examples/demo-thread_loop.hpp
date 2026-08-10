// examples/demo-thread_loop.hpp                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_EXAMPLES_DEMO_THREAD_LOOP
#define INCLUDED_EXAMPLES_DEMO_THREAD_LOOP

// ----------------------------------------------------------------------------

#include <beman/execution/execution.hpp>
#include <functional>
#include <thread>

namespace demo {

class thread_loop : public ::beman::execution::run_loop {
  private:
    std::thread thread{std::bind(&thread_loop::run, this)};

  public:
    ~thread_loop() {
        this->finish();
        this->thread.join();
    }
};

} // namespace demo

// ----------------------------------------------------------------------------

#endif
