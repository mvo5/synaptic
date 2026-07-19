// include/beman/task/detail/result_type.hpp                          -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_RESULT_TYPE
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_RESULT_TYPE

#include <beman/task/detail/sub_visit.hpp>
#include <beman/execution/execution.hpp>
#include <exception>
#include <utility>
#include <type_traits>
#include <variant>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
/**
 * \brief Helper type used as a placeholder for a void result
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 * \internal
 */
enum void_type : unsigned char {};
/**
 * \brief Helper type indicating whether a stopped result is possible
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 * \internal
 */
enum class stoppable { yes, no };

/**
 * \brief Type to hold the result of a coroutine
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 */
template <::beman::task::detail::stoppable Stop, typename Value, typename Errors>
class result_type;

template <::beman::task::detail::stoppable Stop, typename Value, typename... Error>
class result_type<Stop, Value, ::beman::execution::completion_signatures<::beman::execution::set_error_t(Error)...>> {
  private:
    using value_type = ::std::conditional_t<::std::same_as<void, Value>, void_type, Value>;

    ::std::variant<std::monostate, value_type, Error...> result;

    template <size_t I, typename E, typename Err, typename... Errs>
    static constexpr ::std::size_t find_index() {
        if constexpr (std::same_as<E, Err>)
            return I;
        else {
            static_assert(0u != sizeof...(Errs), "error type not found in result type");
            return find_index<I + 1u, E, Errs...>();
        }
    }

  public:
    /**
     * \brief Set the result for a `set_value` completion.
     *
     * If `T` is `void_type` the completion is set to become `set_value()`.
     * Otherwise, the `value` becomes the argument of `set_value(value)` when `result_complete()`
     * is called.
     */
    template <typename T>
    auto set_value(T&& value) -> void {
        this->result.template emplace<1u>(::std::forward<T>(value));
    }
    /**
     * \brief Set the result for a `set_error` completion.
     */
    template <typename E>
    auto set_error(E&& error) -> void {
        this->result.template emplace<2u + find_index<0u, ::std::remove_cvref_t<E>, Error...>()>(
            ::std::forward<E>(error));
    }

    auto no_completion_set() const noexcept -> bool { return this->result.index() == 0u; }
    /**
     * \brief Call the completion function according to the current result.
     *
     * Depending on the current index of the result `result_complete()` calls the
     * a suitable completion function:
     * - If the index is `0` it calls `set_stopped(std::move(rcvr))`.
     * - If the index is `1` it calls `set_value(std::move(rcvr))` if
     *     the `Value` type is `void_type` and `set_value(std::move(rcvr), std::move(std::get<1>(result)))`
     *     otherwise.
     * - Otherwise it calls `set_error(std::move(rcvr), std::move(std::get<I>(result)))`.
     */
    template <::beman::execution::receiver Receiver>
    auto result_complete(Receiver&& rcvr) -> void {
        switch (this->result.index()) {
        case 0:
            if constexpr (Stop == ::beman::task::detail::stoppable::yes)
                ::beman::execution::set_stopped(::std::move(rcvr));
            else
                ::std::terminate();
            break;
        case 1:
            if constexpr (::std::same_as<::beman::task::detail::void_type, value_type>)
                ::beman::execution::set_value(::std::move(rcvr));
            else
                ::beman::execution::set_value(::std::move(rcvr), ::std::move(::std::get<1u>(this->result)));
            break;
        default:
            if constexpr (0u < sizeof...(Error))
                ::beman::task::detail::sub_visit<2u>(
                    [&rcvr](auto& error) { ::beman::execution::set_error(::std::move(rcvr), ::std::move(error)); },
                    this->result);
            break;
        }
    }
    auto result_resume() {
        switch (this->result.index()) {
        case 0:
            std::terminate(); // should never come here!
            break;
        case 1:
            break;
        default:
            if constexpr (0u < sizeof...(Error))
                ::beman::task::detail::sub_visit<2u>(
                    []<typename E>(E& error) {
                        if constexpr (::std::same_as<::std::remove_cvref_t<E>, ::std::exception_ptr>)
                            std::rethrow_exception(::std::move(error));
                        else
                            throw ::std::move(error);
                    },
                    this->result);
            std::terminate(); // should never come here!
            break;
        }
        if constexpr (::std::same_as<::beman::task::detail::void_type, value_type>)
            return;
        else
            return ::std::move(::std::get<1u>(this->result));
    }
};
template <::beman::task::detail::stoppable Stop, typename Value>
class result_type<Stop, Value, ::beman::execution::completion_signatures<>> {
  private:
    using value_type = ::std::conditional_t<::std::same_as<void, Value>, void_type, Value>;

    ::std::variant<std::monostate, value_type> result;

    template <size_t I, typename E, typename Err, typename... Errs>
    static constexpr auto find_index() -> ::std::size_t {
        if constexpr (std::same_as<E, Err>)
            return I;
        else {
            static_assert(0u != sizeof...(Errs), "error type not found in result type");
            return find_index<I + 1u, E, Errs...>();
        }
    }

  public:
    /**
     * \brief Set the result for a `set_value` completion.
     *
     * If `T` is `void_type` the completion is set to become `set_value()`.
     * Otherwise, the `value` becomes the argument of `set_value(value)` when `result_complete()`
     * is called.
     */
    template <typename T>
    auto set_value(T&& value) -> void {
        this->result.template emplace<1u>(::std::forward<T>(value));
    }
    auto no_completion_set() const noexcept -> bool { return this->result.index() == 0u; }

    /**
     * \brief Call the completion function according to the current result.
     *
     * Depending on the current index of the result `result_complete()` calls the
     * a suitable completion function:
     * - If the index is `0` it calls `set_stopped(std::move(rcvr))`.
     * - If the index is `1` it calls `set_value(std::move(rcvr))` if
     *     the `Value` type is `void_type` and `set_value(std::move(rcvr), std::move(std::get<1>(result)))`
     *     otherwise.
     * - Otherwise it calls `set_error(std::move(rcvr), std::move(std::get<I>(result)))`.
     */
    template <::beman::execution::receiver Receiver>
    auto result_complete(Receiver&& rcvr) -> void {
        switch (this->result.index()) {
        case 0:
            if constexpr (Stop == ::beman::task::detail::stoppable::yes)
                ::beman::execution::set_stopped(::std::move(rcvr));
            else
                ::std::terminate();
            break;
        case 1:
            if constexpr (::std::same_as<::beman::task::detail::void_type, value_type>)
                ::beman::execution::set_value(::std::move(rcvr));
            else
                ::beman::execution::set_value(::std::move(rcvr), ::std::move(::std::get<1u>(this->result)));
            break;
        default:
            std::terminate(); // should never come here!
            break;
        }
    }
    auto result_resume() {
        switch (this->result.index()) {
        case 0:
            std::terminate(); // should never come here!
            break;
        case 1:
            break;
        default:
            std::terminate(); // should never come here!
            break;
        }
        if constexpr (::std::same_as<::beman::task::detail::void_type, value_type>)
            return;
        else
            return ::std::move(::std::get<1u>(this->result));
    }
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
