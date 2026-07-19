// include/beman/task/detail/sub_visit.hpp                            -*-C++-*-
// ----------------------------------------------------------------------------
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// ----------------------------------------------------------------------------

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_SUB_VISIT
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_SUB_VISIT

#include <utility>
#include <variant>
#include <cstddef>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
/*
 * \brief Helper function creatig thunks for a variant visit.
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 * \internal
 */
template <std::size_t Start, typename Fun, typename Var, std::size_t... I>
void sub_visit_thunks(Fun& fun, Var& var, std::index_sequence<I...>) {
    using thunk_t = void (*)(Fun&, Var&);
    static constexpr thunk_t thunks[]{(+[](Fun& f, Var& v) { f(std::get<Start + I>(v)); })...};
    thunks[var.index() - Start](fun, var);
}

/*
 * \brief Helper function visiting a suffix of variant options
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 * \internal
 */
template <std::size_t Start, typename... T>
void sub_visit(auto&& fun, std::variant<T...>& v) {
    if (v.index() < Start)
        return;
    sub_visit_thunks<Start>(fun, v, std::make_index_sequence<sizeof...(T) - Start>{});
}

} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
