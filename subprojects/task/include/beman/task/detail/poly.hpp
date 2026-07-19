// include/beman/task/detail/poly.hpp                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_TASK_DETAIL_POLY
#define INCLUDED_BEMAN_TASK_DETAIL_POLY

#include <array>
#include <concepts>
#include <new>
#include <cstddef>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
/*!
 * \brief Utility providing small object optimization and type erasure.
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 * \internal
 */
template <typename Base, std::size_t Size = 4u * sizeof(void*)>
class alignas(sizeof(double)) poly {
  private:
    std::array<std::byte, Size> buf{};

    Base*       pointer() { return static_cast<Base*>(static_cast<void*>(buf.data())); }
    const Base* pointer() const { return static_cast<const Base*>(static_cast<const void*>(buf.data())); }

  public:
    template <typename T, typename... Args>
        requires(sizeof(T) <= Size)
    poly(T*, Args&&... args) {
        new (this->buf.data()) T(::std::forward<Args>(args)...);
        static_assert(sizeof(T) <= Size);
    }
    poly(poly&& other)
        requires requires(Base* b, void* t) { b->move(t); }
    {
        other.pointer()->move(this->buf.data());
    }
    poly& operator=(poly&& other)
        requires requires(Base* b, void* t) { b->move(t); }
    {
        if (this != &other) {
            this->pointer()->~Base();
            other.pointer()->move(this->buf.data());
        }
        return *this;
    }
    poly& operator=(const poly& other)
        requires requires(Base* b, void* t) { b->clone(t); }
    {
        if (this != &other) {
            this->pointer()->~Base();
            other.pointer()->clone(this->buf.data());
        }
        return *this;
    }
    poly(const poly& other)
        requires requires(Base* b, void* t) { b->clone(t); }
    {
        other.pointer()->clone(this->buf.data());
    }
    ~poly() { this->pointer()->~Base(); }
    bool operator==(const poly& other) const
        requires requires(const Base& b) {
            { b.equals(&b) } -> std::same_as<bool>;
        }
    {
        return other.pointer()->equals(this->pointer());
    }
    Base*       operator->() { return this->pointer(); }
    const Base* operator->() const { return this->pointer(); }
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
