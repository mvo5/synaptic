// tests/beman/task/poly.test.cpp                                     -*-C++-*-
// ----------------------------------------------------------------------------
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// ----------------------------------------------------------------------------

#include <beman/task/detail/poly.hpp>
#include <array>
#include <concepts>
#include <utility>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

namespace ex = beman::task;

// ----------------------------------------------------------------------------

namespace {
// ----------------------------------------------------------------------------
struct value {
    int val{};
    explicit value(int v = {}) : val(v) {}
    value(value&& other) noexcept : val(std::exchange(other.val, 42)) {}
    value(const value& other) : val(other.val) {}
    ~value()                                       = default;
    value& operator=(value&& other)                = delete;
    value& operator=(const value& other)           = delete;
    bool   operator==(const value&) const noexcept = default;
};
// ----------------------------------------------------------------------------
struct immovable_base {
    immovable_base()                                 = default;
    immovable_base(const immovable_base&)            = delete;
    immovable_base(immovable_base&&)                 = delete;
    virtual ~immovable_base()                        = default;
    immovable_base& operator=(const immovable_base&) = delete;
    immovable_base& operator=(immovable_base&&)      = delete;

    virtual int  ivalue() const = 0;
    virtual bool bvalue() const = 0;
};

struct immovable_concrete : immovable_base {
    int  ival{};
    bool bval{};
    explicit immovable_concrete(int v = 0, bool b = false) : ival(v), bval(b) {}
    int  ivalue() const override { return this->ival; }
    bool bvalue() const override { return this->bval; }
};
struct immovable_big : immovable_base {
    std::array<void*, 8> member{};
};
// ----------------------------------------------------------------------------
struct copyable_base {
    copyable_base()                                = default;
    copyable_base(copyable_base&&)                 = default;
    copyable_base(const copyable_base&)            = delete;
    virtual ~copyable_base()                       = default;
    copyable_base& operator=(const copyable_base&) = default;
    copyable_base& operator=(copyable_base&&)      = default;
    virtual void   clone(void*) const              = 0;

    virtual int   ivalue() const = 0;
    virtual bool  bvalue() const = 0;
    virtual value vvalue() const = 0;
};

struct copyable_concrete : copyable_base {
    int   ival{};
    bool  bval{};
    value vval{};
    explicit copyable_concrete(int i = 0, bool b = false, value v = value()) : ival(i), bval(b), vval(std::move(v)) {}
    void clone(void* d) const override { new (d) copyable_concrete(this->ival, this->bval, this->vval); }

    int   ivalue() const override { return this->ival; }
    bool  bvalue() const override { return this->bval; }
    value vvalue() const override { return this->vval; }
};
// ----------------------------------------------------------------------------
struct movable_base {
    movable_base()                               = default;
    movable_base(movable_base&&)                 = default;
    movable_base(const movable_base&)            = delete;
    virtual ~movable_base()                      = default;
    movable_base& operator=(movable_base&&)      = default;
    movable_base& operator=(const movable_base&) = delete;
    virtual void  move(void*)                    = 0;

    virtual int   ivalue() const = 0;
    virtual bool  bvalue() const = 0;
    virtual value vvalue() const = 0;
};

struct movable_concrete : movable_base {
    int   ival{};
    bool  bval{};
    value vval{};
    explicit movable_concrete(int i = 0, bool b = false, value v = value()) : ival(i), bval(b), vval(std::move(v)) {}
    void move(void* d) override { new (d) movable_concrete(this->ival, this->bval, std::move(this->vval)); }

    int   ivalue() const override { return this->ival; }
    bool  bvalue() const override { return this->bval; }
    value vvalue() const override { return this->vval; }
};
// ----------------------------------------------------------------------------
struct both_base {
    both_base()                              = default;
    both_base(both_base&&)                   = default;
    both_base(const both_base&)              = delete;
    virtual ~both_base()                     = default;
    both_base&   operator=(both_base&&)      = default;
    both_base&   operator=(const both_base&) = delete;
    virtual void move(void*)                 = 0;
    virtual void clone(void*) const          = 0;

    virtual int   ivalue() const = 0;
    virtual bool  bvalue() const = 0;
    virtual value vvalue() const = 0;
};

struct both_concrete : both_base {
    int   ival{};
    bool  bval{};
    value vval{};
    explicit both_concrete(int i = 0, bool b = false, value v = value()) : ival(i), bval(b), vval(std::move(v)) {}
    void move(void* d) override { new (d) both_concrete(this->ival, this->bval, std::move(this->vval)); }
    void clone(void* d) const override { new (d) both_concrete(this->ival, this->bval, this->vval); }

    int   ivalue() const override { return this->ival; }
    bool  bvalue() const override { return this->bval; }
    value vvalue() const override { return this->vval; }
};
// ----------------------------------------------------------------------------
struct equals_base {
    equals_base()                              = default;
    equals_base(const equals_base&)            = delete;
    equals_base(equals_base&&)                 = delete;
    virtual ~equals_base()                     = default;
    equals_base& operator=(const equals_base&) = delete;
    equals_base& operator=(equals_base&&)      = delete;

    virtual int  ivalue() const                                = 0;
    virtual bool bvalue() const                                = 0;
    virtual bool equals(const equals_base*) const              = 0;
    bool         operator==(const equals_base&) const noexcept = default;
};

struct equals_concrete : equals_base {
    int  ival{};
    bool bval{};
    explicit equals_concrete(int v = 0, bool b = false) : ival(v), bval(b) {}
    int  ivalue() const override { return this->ival; }
    bool bvalue() const override { return this->bval; }
    bool equals(const equals_base* other) const override {
        const auto* o{dynamic_cast<const equals_concrete*>(other)};
        return o && *this == *o;
    }
    bool operator==(const equals_concrete&) const noexcept = default;
};
// ----------------------------------------------------------------------------
template <bool Expect, typename Base, typename Concrete>
void test_poly_exists() {
    static_assert(Expect == requires { ex::detail::poly<Base>(static_cast<Concrete*>(nullptr)); });
}
// ----------------------------------------------------------------------------
void test_poly_ctor() {
    ex::detail::poly<immovable_base> i0(static_cast<immovable_concrete*>(nullptr));
    assert(i0->ivalue() == 0);
    assert(i0->bvalue() == false);
    ex::detail::poly<immovable_base> i1(static_cast<immovable_concrete*>(nullptr), 17);
    assert(i1->ivalue() == 17);
    assert(i1->bvalue() == false);
    ex::detail::poly<immovable_base> i2(static_cast<immovable_concrete*>(nullptr), 17, true);
    assert(i2->ivalue() == 17);
    assert(i2->bvalue() == true);
}
// ----------------------------------------------------------------------------
template <bool Expect, typename Base>
void test_poly_move_exists() {
    static_assert(Expect == requires(ex::detail::poly<Base> p) { ex::detail::poly<Base>(std::move(p)); });
    static_assert(Expect == requires(ex::detail::poly<Base> p, ex::detail::poly<Base> q) {
        { q = std::move(p) } -> std::same_as<ex::detail::poly<Base>&>;
    });
}
template <typename Base>
void test_poly_move(ex::detail::poly<Base> p, bool, ex::detail::poly<Base> o, const auto& moved_from) {
    auto i = p->ivalue();
    auto b = p->bvalue();
    auto v = p->vvalue();

    ex::detail::poly<Base> q(std::move(p));
    assert(moved_from ==
           p->vvalue()); // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move,hicpp-invalid-access-moved)
    assert(i == q->ivalue());
    assert(b == q->bvalue());
    assert(v == q->vvalue());

    assert(i != o->ivalue());
    assert(b != o->bvalue());
    assert(v != o->vvalue());
    o = std::move(q);
    assert(moved_from ==
           q->vvalue()); // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move,hicpp-invalid-access-moved)
    assert(i == o->ivalue());
    assert(b == o->bvalue());
    assert(v == o->vvalue());
}
// ----------------------------------------------------------------------------
template <bool Expect, typename Base>
void test_poly_copy_exists() {
    static_assert(Expect == requires(ex::detail::poly<Base> p) { ex::detail::poly<Base>(p); });
    static_assert(Expect == requires(ex::detail::poly<Base> p) {
        { p = p } -> std::same_as<ex::detail::poly<Base>&>;
    });
}
template <typename Base>
void test_poly_copy(ex::detail::poly<Base> p, bool, ex::detail::poly<Base> o) {
    auto i = p->ivalue();
    auto b = p->bvalue();
    auto v = p->vvalue();

    ex::detail::poly<Base> q(p); // NOLINT(performance-unnecessary-copy-initialization)
    assert(i == p->ivalue());
    assert(b == p->bvalue());
    assert(v == p->vvalue());
    assert(i == q->ivalue());
    assert(b == q->bvalue());
    assert(v == q->vvalue());

    assert(i != o->ivalue());
    assert(b != o->bvalue());
    assert(v != o->vvalue());
    o = q;
    assert(i == q->ivalue());
    assert(b == q->bvalue());
    assert(v == q->vvalue());
    assert(i == o->ivalue());
    assert(b == o->bvalue());
    assert(v == o->vvalue());
}
// ----------------------------------------------------------------------------
template <bool Expect, typename Base>
void test_poly_equals_exists() {
    static_assert(Expect == requires(const ex::detail::poly<Base> p) { p == p; });
    static_assert(Expect == requires(const ex::detail::poly<Base> p) { p != p; });
}
// ----------------------------------------------------------------------------
} // namespace

int main() {
    test_poly_exists<true, immovable_base, immovable_concrete>();
    test_poly_exists<false, immovable_base, immovable_big>();
    test_poly_exists<true, movable_base, movable_concrete>();
    test_poly_exists<true, copyable_base, copyable_concrete>();
    test_poly_exists<true, both_base, both_concrete>();

    movable_concrete*  mtag(nullptr);
    copyable_concrete* ctag(nullptr);
    both_concrete*     btag(nullptr);

    test_poly_ctor();
    test_poly_move_exists<false, immovable_base>();
    test_poly_move_exists<true, movable_base>();
    test_poly_move_exists<true, copyable_base>();
    test_poly_move_exists<true, both_base>();
    test_poly_move(ex::detail::poly<movable_base>(mtag, 17, true, value(18)),
                   true,
                   ex::detail::poly<movable_base>(mtag, 19, false, value(20)),
                   value(42));
    test_poly_move(ex::detail::poly<copyable_base>(ctag, 17, true, value(18)),
                   true,
                   ex::detail::poly<copyable_base>(ctag, 19, false, value(20)),
                   value(18));
    test_poly_move(ex::detail::poly<both_base>(btag, 17, true, value(18)),
                   true,
                   ex::detail::poly<both_base>(btag, 19, false, value(20)),
                   value(42));

    test_poly_copy_exists<false, immovable_base>();
    test_poly_copy_exists<false, movable_base>();
    test_poly_copy_exists<true, copyable_base>();
    test_poly_copy_exists<true, both_base>();
    test_poly_copy(ex::detail::poly<copyable_base>(ctag, 17, true, value(18)),
                   true,
                   ex::detail::poly<copyable_base>(ctag, 19, false, value(20)));
    test_poly_copy(ex::detail::poly<both_base>(btag, 17, true, value(18)),
                   true,
                   ex::detail::poly<both_base>(btag, 19, false, value(20)));

    test_poly_equals_exists<false, immovable_base>();
    test_poly_equals_exists<false, movable_base>();
    test_poly_equals_exists<false, copyable_base>();
    test_poly_equals_exists<false, both_base>();
    static_assert(true == requires(equals_base& b) {
        { b.equals(&b) } -> std::same_as<bool>;
    });
    test_poly_equals_exists<true, equals_base>();
}
