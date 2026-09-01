// examples/task_scheduler.cpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>
#include <cstdlib>

namespace ex = beman::execution;

int main() {
    ex::inline_scheduler       isched;
    const ex::inline_scheduler cisched;
    ex::task_scheduler         asched(isched);
    const ex::task_scheduler   casched(cisched);

    return asched == isched && isched == asched && asched == cisched && cisched == casched && !(cisched != casched) &&
                   !(asched != isched)
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
