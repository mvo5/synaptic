---
title: Task's Allocator Use
document: P3980R1
date: 2026-03-24
audience:
    - Library Evolution Working Group (LEWG)
    - Library Working Group (LWG)
author:
    - name: Dietmar Kühl (Bloomberg)
      email: <dkuhl@bloomberg.net>
source:
    - https://github.com/bemanproject/task/doc/P3980-allocation.md
toc: true
---

<p>
There are different uses of allocators for `task`. The obvious use
is that the coroutine frame needs to be allocated and using an
allocator control where this coroutine frame gets allocated. In
addition, the environment used when `connect`ing a sender can provide
access to an allocator via the `get_allocator` query. The current
specification uses the same allocator for coroutine frame and the
child environments. At the Kona meeting the room preferred if these
were separated and the allocator for the environment were taken
from the environment of the receiver it gets `connect`ed to. In
doing so, the allocator for the coroutine frame becomes more flexible
and it should be brought more in line with `generator`'s allocator
use. This paper addresses
[US 254-385](https://github.com/cplusplus/nbballot/issues/960),
[US 253-386](https://github.com/cplusplus/nbballot/issues/961),
[US 255-384](https://github.com/cplusplus/nbballot/issues/959),
and [US 261-391](https://github.com/cplusplus/nbballot/issues/966).
</p>

# Change History

## R0 Initial Revision

# Overview of Changes

<p>
There are a few NB comments about `task`'s use of allocators:
</p>
<ul>
<li>[US 254-385](https://github.com/cplusplus/nbballot/issues/960): Constraint `allocator_arg` argument to be the first argument</li>
<li>[US 253-386](https://github.com/cplusplus/nbballot/issues/961): Allow use of arbitrary allocators for coroutine frame</li>
<li>[US 255-384](https://github.com/cplusplus/nbballot/issues/959): Use allocator from receiver's environment </li>
<li>[US 261-391](https://github.com/cplusplus/nbballot/issues/966): Bad specification of parameter type</li>
</ul>
<p>
The first issue
([US 254-385](https://github.com/cplusplus/nbballot/issues/960)) is
about where an allocator argument for the coroutine frame may go
on the coroutine function.  The options are a fixed location (which
would fit first for consistency with existing use) and anywhere. The
status quo is anywhere, and the request is to require that it goes
first. However, to support optionally passing an allocator, having
it go anywhere is easier to do.
</p>
<p>
The allocator constraints for allocating the coroutine frame are
due to the use of the same allocator for the environment of child
senders. If the allocator for the environment of child senders uses
the allocator from the receiver's environment, these constraints
can be relaxed. Instead, there may be requirements on the result
of the `get_allocator` query from the receiver's environment. The
discussion in Kona favored this direction. This change can address
the second ([US 253-386](https://github.com/cplusplus/nbballot/issues/961))
and the third
([US 255-384](https://github.com/cplusplus/nbballot/issues/959)) issues.
</p>
<p>
The fourth issue ([US
261-391](https://github.com/cplusplus/nbballot/issues/966)) is
primarily a wording issue. However, some of the problematic paragraphs
will need some modifications to address the other issues, i.e., fixing
these wording issues in isolation isn't reasonable.
</p>

# Allocator Argument Position

<p>
The combination of using `allocator_arg` followed by an allocator
object when invoking a function or a constructor is used in various
places throughout the standard C++ library. The `allocator_arg`
argument normally needs to be the first argument when present. The
definition of `task` makes the position of the `allocator_arg` more
flexible to allow easier support for optionally passing an allocator.
</p>
<p>
For coroutines, the arguments to the coroutine [factory] function
show up in three separate places:
</p>
<ol>
<li>The parameters to the coroutine [factory] functions.</li>
<li>The constructor of the `promise_type` if there is a suitable matching overload.</li>
<li>the `operator new()` of the `promise_type` if there is a suitable matching overload.</li>
</ol>
<p>
This added flexibility doesn't introduce any constraints on how the coroutine
function is defined. It rather allows passing an `allocator_arg`/allocator
pair without requiring a specific location. The main benefit is that support
of an optional allocator can be supported by having a trailing `, auto&&...`
on the parameter list. Note that the allocator used for the coroutine frame
is normally not used in the body of the allocator. If it is needed, it can
in all cases be put into the first location.
</p>
::: cmptable

### First
```
task<> none(int x)
{ ...  }
```

### Flexible
```
task<> none(int x)
{ ...  }
```

---

```
task<> comes_first(allocator_arg_t, auto a, int x)
{ ...  }
```

```
task<> comes_first(allocator_arg_t, auto a, int x)
{ ...  }
```

---

```
task<> optional(allocator_arg_t, auto, int x)
{ ... }
task<> optional(int x)
{ return optional(allocator_arg, allocator<char>(), x); }
```

```
task<> optional(int x, auto&&...)
{ ... }
```

:::

<p>
The comparison table above shows three separate cases the author of
a coroutine function may want to support:
</p>
<ol>
  <li>No allocator support (`none`): the use identical and just doesn't mention any allocator.</li>
  <li>Mandate that the allocator is the first argument (`comes_first`): the use is identical.</li>
  <li>
    Support optionally passing an `allocator_arg`/allocator pair
    (`optional`): the use can be identical but it can also be simplified
    taking advantage of the flexible location.
  </li>
</ol>

Below are three variations of the wording changes, only one can be picked:

<ol type="A">
<li>Only support `allocator_arg` as the first argument and use the receiver's allocator for the environment.</li>
<li>Flexible position of the allocator arg and use allocator for the environment.</li>
<li>Flexible position of the allocator arg and use the receiver's allocator for the environment.</li>
</ol>

At the LEWG meeting on 2026-02-03 the first approach (putting the `allocator_arg` first, Wording Change A)
was preferred ([notes](https://wiki.isocpp.org/2026-02-03_LEWG_Telecon)). It was identified that the original wording change did not support member functions returning a `task` (the wording was fixed accordingly).

## Wording Change A: `allocator_arg` must be first argument

::: ednote
Change the synopsis of `promise` type in [task.promise], modifying
the overloads of `operator new`:
:::

```
namespace std::execution {
  template<class T, class Environment>
  class task<T, Environment>::promise_type {
  public:
    ...
    unspecified get_env() const noexcept;

    @[void*]{.add}@ @[operator]{.add}@ @[new(size_t]{.add}@ @[size);]{.add}@
    template<@[class Alloc,]{.add}@ class... Args>
      void* operator new(size_t size, @[allocator_arg_t,]{.add}@ @[Alloc]{.add}@ @[alloc, ]{.add}@Args&&... @[args]{.rm}@);
    @[template<class]{.add}@ @[This,]{.add}@ @[class]{.add}@ @[Alloc,]{.add}@ @[class ...]{.add}@ @[Args>]{.add}@
      @[void*]{.add}@ @[operator]{.add}@ @[new(size_t]{.add}@ @[size,]{.add}@ @[const]{.add}@ @[This&,]{.add}@ @[allocator_arg_t,]{.add}@ @[Alloc]{.add}@ @[alloc,]{.add}@ @[Args&&...);]{.add}@

    void operator delete(void* pointer, size_t size) noexcept;

  private:
    ...
  };
}
```

::: ednote
Change [task.promise] paragraphs 17 and 18:
:::

::: add

```
void* operator new(size_t size);
```

[??]{.pnum} _Effects_: Equivalent to `return operator new(size, allocator_arg, allocator_type());`

:::

```
template<@[class Alloc,]{.add}@ class... Args>
  void* operator new(size_t size, @[allocator_arg_t,]{.add}@ @[Alloc]{.add}@ @[alloc, ]{.add}@Args&&... @[args]{.rm}@);
@[template<class]{.add}@ @[This,]{.add}@ @[class]{.add}@ @[Alloc,]{.add}@ @[class ...]{.add}@ @[Args>]{.add}@
  @[void*]{.add}@ @[operator]{.add}@ @[new(size_t]{.add}@ @[size,]{.add}@ @[const]{.add}@ @[This&,]{.add}@ @[allocator_arg_t,]{.add}@ @[Alloc]{.add}@ @[alloc,]{.add}@ @[Args&&...);]{.add}@
```

[17]{.pnum} [If there is no parameter with type `allocator_arg_t`
then let `alloc` be `allocator_type()`. Otherwise, let `arg_next`
be the parameter following the first `allocator_arg_t` parameter,
and let `alloc` be `allocator_type(arg_next)`]{.rm}. Let `PAlloc`
be `allocator_traits<@[allocator_type]{.rm}@@[Alloc]{.add}@>::template
rebind_alloc<U>`, where `U` is an unspecified type whose size and
alignment are both `__STDCPP_DEFAULT_NEW_ALIGNMENT__`.

::: rm

[18]{.pnum}
_Mandates_:

<ul>
<li>[18.1]{.pnum} The first parameter of type `allocator_arg_t` (if any) is not the last parameter.</li>
<li>[18.2]{.pnum} `allocator_type(arg_next)` is a valid expression if there is a parameter of type `allocator_arg_t`.</li>
<li>[18.3]{.pnum} `allocator_traits<PAlloc>​::​pointer` is a pointer type.</li>
</ul>

:::

::: add

[18]{.pnum}
_Mandates_: `allocator_traits<PAlloc>​::​pointer` is a pointer type.

:::

[19]{.pnum} _Effects_: Initializes an allocator `palloc` of type
`PAlloc` with `alloc`. Uses `palloc` to allocate storage for the
smallest array of `U` sufficient to provide storage for a coroutine
state of size `size`, and unspecified additional state necessary to
ensure that `operator delete` can later deallocate this memory block
with an allocator equal to `palloc`.

[20]{.pnum} _Returns_: A pointer to the allocated storage.

# Use Allocator From Environment

<p>
During the discussion at Kona the conclusion was that the allocator
forwarded by `task`'s environment to child senders should be the
allocator from `get_allocator` on the receiver `task` gets `connect`ed
to. Let `rcvr` be the receiver a `task` got `connect`ed to and let `ev`
be the result of `get_env(rcvr)`. The implication is that the `task`'s
`allocator_type` is compatible with the allocator of `ev`:
</p>
<ul>
  <li>
    If `get_allocator(ev)` is not defined, `allocator_type` has to
    be default constructible.
  </li>
  <li>
    Otherwise `allocator_type(get_allocator(ev))` has to be well-formed.
  </li>
  <li>
    There is no need to store an <code><i>alloc</i></code> in the
    `promise_type`: it can be obtained when requested from `ev` which,
    in turn, can be obtained from `rcvr`. Thus, the ctor for `promise_type`
    isn't needed.
  </li>
  <li>
    The definition of `get_env` needs to be changed to get the allocator
    when needed.
  </li>
</ul>

## Wording Changes

::: ednote

In [[task.members](https://wg21.link/task.members#3)] add a _Mandates_ to `connect`:

:::

```
template<receiver Rcvr>
  state<Rcvr> connect(Rcvr&& recv) &&;
```

::: add

[?]{.pnum} _Mandates_: At least one of the expressions `allocator_type(get_allocator(get_env(rcvr)))` and `allocator_type()` is well-formed.

:::

[3]{.pnum} _Preconditions_: <code>bool(<i>handle</i>)</code> is `true`.

[4]{.pnum} _Effects_: Equivalent to:
```
return state<Rcvr>(exchange(handle, {}), std::forward<Rcvr>(recv));
```

::: ednote

In [[task.promise]](https://wg21.link/task.promise) in the synopsis remove the `promise_type` constructor and
the <code><i>alloc</i></code> exposition-only member.

:::

```
namespace std::execution {
  template<class T, class Environment>
  class task<T, Environment>::promise_type {
  public:
    @[template<class...]{.rm}@ @[Args>]{.rm}@
      @[promise_type(const]{.rm}@ @[Args&...]{.rm}@ @[args);]{.rm}@

    task get_return_object() noexcept;

    ...
  private:
    using error-variant = see below;    // @_exposition only_@

    @[allocator_type]{.rm}@    @[_alloc_;]{.rm}@            @[//]{.rm}@ @[_exposition_]{.rm}@ @[_only_]{.rm}@
    stop_source_type  @_source_@;           // @_exposition only_@
    stop_token_type   @_token_@;            // @_exposition only_@
    optional<T>       @_result_@;           // @_exposition only_@; present only if is_void_v<T> is false
    error-variant     @_errors_@;           // @_exposition only_@
  };
}
```

::: ednote

Remove the ctor for `promise_type`, i.e., [[task.promise]](https://wg21.link/task.promise) paragraph [3](https://wg21.link/task.promise#3) and [4](https://wg21.link/task.promise#4):

:::

::: rm

```
template<class... Args>
  promise_type(const Args&... args);
```

[3]{.pnum} _Mandates_: The first parameter of type `allocator_arg_t`
(if any) is not the last parameter.

[4]{.pnum} _Effects_: If `Args` contains an element of type
`allocator_arg_t` then <code><i>alloc</i></code> is initialized
with the corresponding next element of `args`. Otherwise,
<code><i>alloc</i></code> is initialized with `allocator_type()`.

:::

::: ednote

Change `get_env` to get the allocator from the receiver when needed
in [[task.promise] p16](https://wg21.link/task.promise#16):

:::

```
@_unspecified_@ get_env() const noexcept;
```

[16]{.pnum} _Returns_: An object `env` such that queries are forwarded as follows:
<ul>
<li>[16.1]{.pnum} `env.query(get_scheduler)` returns <code>scheduler_type(<i>SCHED</i>(*this))</code>.</li>
<li>[16.2]{.pnum} `env.query(get_allocator)` returns [<code><i>alloc</i></code>]{.rm}[<code>allocator_type(get_allocator(get_env(<i>RCVR</i>(*this))))</code> if this expression is well-formed and `allocator_type()` otherwise]{.add}.</li>
<li>[16.3]{.pnum} `env.query(get_stop_token)` returns <code><i>token</i></code>.</li>
<li>[16.4]{.pnum} For any other query `q` and arguments `a...` a call to `env.query(q, a...)` returns <code><i>STATE</i>(*this).environment.query(q, a...)</code> if this expression is well-formed and `forwarding_query(q)` is well-formed and is `true`. Otherwise `env.query(q, a...)` is ill-formed.</li>
</ul>
