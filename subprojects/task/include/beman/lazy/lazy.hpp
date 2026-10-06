// include/beman/lazy/lazy.hpp                                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_LAZY_LAZY
#define INCLUDED_INCLUDE_BEMAN_LAZY_LAZY

#include <beman/task/task.hpp>

// ----------------------------------------------------------------------------

namespace beman::lazy {
template <typename T = void, typename Context = ::beman::task::detail::default_environment>
using lazy = ::beman::task::detail::task<T, Context>;
}

namespace beman::execution {
template <typename T = void, typename Context = ::beman::task::detail::default_environment>
using lazy [[deprecated("beman::execution::lazy has been renamed to beman::execution::task")]] =
    ::beman::execution::task<T, Context>;
}

// ----------------------------------------------------------------------------

#endif
