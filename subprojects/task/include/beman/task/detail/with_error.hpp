// include/beman/task/detail/with_error.hpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_WITH_ERROR
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_WITH_ERROR

#include <coroutine>
#include <utility>

// ----------------------------------------------------------------------------
/*
 * \brief Tag type used to indicate an error is produced.
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 * \internal
 */
namespace beman::task::detail {
template <typename E>
struct with_error {
    using type = ::std::remove_cvref_t<E>;
    type error;
};
template <typename E>
with_error(E&&) -> with_error<E>;

} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
