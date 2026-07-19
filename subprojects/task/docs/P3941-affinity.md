---
title: Scheduler Affinity
document: P3941R4
date: 2026-03-26
audience:
    - Concurrency Working Group (SG1)
    - Library Evolution Working Group (LEWG)
    - Library Working Group (LWG)
author:
    - name: Dietmar Kühl (Bloomberg)
      email: <dkuhl@bloomberg.net>
source:
    - https://github.com/bemanproject/task/doc/P3941-affinity.md
toc: true
---

<p>
One important design of `std::execution::task` is that a coroutine
resumes after a `co_await` on the same scheduler as the one it was
executing on prior to the `co_await`. To achieve this, `task`
transforms the awaited object `@_obj_@` using
`affine_on(@_obj_@, @_sched_@)` where `@_sched_@` is the corresponding
scheduler. There were multiple concerns raised against the specification
of `affine_on` and discussed as part of
[P3796R1](https://wg21.link/P3796R1).  This proposal is intended
to specifically address the concerns raised relating to `task`'s
scheduler affinity and in particular `affine_on`.  The gist of this
proposal is impose constraints on `affine_on` to guarantee it can
meet its objective at run-time.
</p>

# Change History

## R4

- remove the alternative wording using `get_scheduler`
- remove fallback `get_scheduler` for `get_start_scheduler`
- use `get_start_scheduler` instead of `get_scheduler` to refer to
  the scheduler an operation got started on (either for queries or
  for defining environments)
- use `start_scheduler_type` instead of `scheduler_type` in `task`
- give implementations permission to define `@_sndr_@.affine_on()`
  and `@_sndr_@.as_awaitable(@_p_@)`

## R3

- rebase changes on the customization changes [P3826r3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3826r3.html)
- use `transform_sender` in `as_awaitable` to locate possible customization of nested
    senders in [[exec.as.awaitable](https://wg21.link/exec.as.awaitable#7)]

## R2

- added requirement on `get_scheduler`/`get_start_scheduler`
- fixed typo in the wording for `affine_on::transform_sender`
- fixed various typos in the text

## R1

- added wording

## R0

- initial revision

# Overview of Changes

<p>
There are a few NB comments raised about the way `affine_on` works:
</p>
<ul>
   <li>[US 232-366](https://github.com/cplusplus/nbballot/issues/941): specify customization of `affine_on` when the scheduler doesn't change.</li>
   <li>[US 233-365](https://github.com/cplusplus/nbballot/issues/940): clarify `affine_on` vs. `continues_on`.</li>
   <li>[US 234-364](https://github.com/cplusplus/nbballot/issues/939): remove scheduler parameter from `affine_on`.</li>
   <li>[US 235-363](https://github.com/cplusplus/nbballot/issues/938): `affine_on` should not forward the stop token to the scheduling operation.</li>
   <li>[US 236-362](https://github.com/cplusplus/nbballot/issues/937): specify default implementation of `affine_on`.</li>
</ul>
<p>
The discussion on `affine_on` revealed some aspects which were not
quite clear previously and taking these into account points towards
a better design than was previously specified:
</p>
<ol>
  <li>
    To implement scheduler affinity the algorithm needs to know the
    scheduler on which it was started itself. The correct scheduler
    may actually be hard to determine while building the work graph.
    However, this scheduler can be communicated using
    `get_scheduler(get_env(@_rcvr_@))` when an algorithm
    is `start`ed. This requirement is more general than just
    `affine_on` and is introduced by
    [P3718R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3718r0.html):
    with this guarantee in place, `affine_on` only needs one
    parameter, i.e., the sender for the work to be executed.
    [P3718R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3718r0.html)
    is, however, discontinued (for unrelated reasons) and adding the guarantee
    to get the current scheduler from a receiver query is proposed
    here.
  </li>
  <li>
    The scheduler `@_sched_@` on which the work needs
    to resume has to guarantee that it is possible to resume on the
    correct execution agent. The implication is that scheduling work needs
    to be infallible, i.e., the completion signatures of
    `scheduler(@_sched_@)` cannot contain a
    `set_error_t(E)` completion signature. This requirement should
    be checked statically.
  </li>
  <li>
    The work needs to be resumed on the correct scheduler even when
    the work is stopped, i.e., the scheduling operation shall be
    `connect`ed to a receiver whose environment's `get_stop_token`
    query yields an `unstoppable_token`. In addition, the
    scheduling operation shall not have a `set_stopped_t()` completion
    signature if the environment's `get_stop_token` query yields
    an `unstoppable_token`. This requirement should also be checked
    statically.
  </li>
  <li>
    When a sender knows that it will complete on the scheduler it
    was started on, it should be possible to customize the `affine_on`
    algorithm to avoid rescheduling. This customization can be
    achieved by `connect`ing to the result of an `affine_on` member
    function called on the child sender, if such a member function
    is present, when `connect`ing an `affine_on` sender.
  </li>
</ol>

<p>
None of these changes really contradict any earlier design: the
shape and behavior of the `affine_on` algorithm wasn't fully fleshed
out. Tightening the behavior of scheduler affinity and the `affine_on`
algorithm has some implications on some other components:
</p>
<ol>
  <li>
    If `affine_on` requires an infallible scheduler at least
    `inline_scheduler`, `task_scheduler`, and `run_loop::scheduler`
    should be infallible (i.e., they always complete successfully
    with `set_value()`). `parallel_scheduler` can probably not be
    made infallible.
  </li>
  <li>
    The scheduling semantics when changing a `task`'s scheduler
    using `co_await change_coroutine_scheduler(@_sch_@)`
    become somewhat unclear and this functionality should be removed.
    Similar semantics are better modeled using
    `co_await on(@_sch_@, @_nested-task_@)`.
  </li>
  <li>
    The name `affine_on` isn't particular good and wasn't designed.
    It may be worth renaming the algorithms to something different.
  </li>
</ol>

# Discussion of Changes

## `affine_on` Shape

<p>
The original proposal for `task` used `continues_on` to schedule
the work back on the original scheduler. This algorithm takes the
work to be executed and the scheduler on which to continue as
arguments. When SG1 requested that a similar but different algorithms
is to be used to implement scheduler affinity, `continues_on` was
just replaced by `affine_on` with the same shape but the potential
to get customized differently.
</p>
<p>
The scheduler used for affinity is the scheduler communicated via
the `get_scheduler` query on the receiver's environment: the scheduler
argument passed to the `affine_on` algorithm would need to match
the scheduler obtained from `get_scheduler` query.  In the context
of the `task` coroutine this scheduler can be obtained via the
promise type but in general it is actually not straight forward to
get hold of this scheduler because the receiver and hence its
associated scheduler is only provided by `connect`. It is much more
reasonable to have `affine_on` only take the work, i.e., a sender,
as argument and determine the scheduler to resume on from the
receiver's
environment in `connect`.
</p>
<p>
Thus, instead of using
```c++
affine_on(@_sndr_@, @_sch_@)
```
the algorithm is used just with the sender:
```c++
affine_on(@_sndr_@)
```
</p>
<p>
Note that this change implies that an operation state resulting
from `connect`ing `affine_on` to a receiver `@_rcvr_@`
is `start`ed on the execution agent associated with the scheduler obtained
from `get_scheduler(get_env(@_rcvr_@))`. The same
requirement is also assumed to be met when `start`ing the operation
state resulting from `connect`ing a `task`. While it is possible to
statically detect whether the query is valid and provides a scheduler
it cannot be detected if the scheduler matches the execution agent on which
`start` was called.
[P3718r0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3718r0.html)
proposed to add this exact requirement to
[[exec.get.scheduler]](https://wg21.link/exec.get.scheduler) which is no moved
to this proposal.
</p>
<p>
This change addresses [US 234-364](https://github.com/cplusplus/nbballot/issues/939) ([LWG4331](https://cplusplus.github.io/LWG/issue4331)).
</p>

## Scheduler An Operation Was Started On

<p>
The `on` and `affine_on` algorithms both use the scheduler returned
from the `get_scheduler` query on the receiver to determine where
to resume execution after some other work completed. When using
these algorithms it is not always possible to determine which
scheduler an operation gets started on when creating a work graph.
Thus, the algorithms determine the scheduler from context. To
actually do that, it needs to be required that the scheduler is
made accessible when using these algorithms.
[P3718r0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3718r0.html)
proposed that `get_scheduler` should be required to yield the
scheduler an operation was started on.

</p>

<p>
It was pointed out that the meaning for the `get_scheduler` query isn't clear
and it is used for two separate purposes:
</p>
<ol>
<li>The `get_scheduler` query is used to get a scheduler to schedule new work on.</i>
<li>The `get_scheduler` query is used to get the scheduler an operation was `start`ed on.</i>
</ol>
<p>
This dual use doesn't necessary align. It would be reasonable to
add a new query, e.g., `get_start_scheduler` which is used to
determine the scheduler an operation was `start`ed on. The requirement
for getting the scheduler an operation was started on would be on
the `get_start_scheduler` query rather than on the `get_scheduler`
query. At the same time the `get_start_scheduler` query could be
defaulted to use the `get_scheduler` query.
</p>

## Infallible Schedulers
<p>
The objective of `affine_on(@_sndr_@)` is to execute `@_sndr_@` and
to complete on the execution agent on which the operation was
`start`ed. Let `sch` be the scheduler obtained from
`get_scheduler(get_env(@_rcvr_@))` where `@_rcvr_@` is the receiver
used when `connect`ing `affine_on(@_sndr_@)` (the discussion in
this section also applies if the scheduler would be taken as a
parameter, i.e., if the [previous change](#affine_on-shape) isn't
applied this discussion still applies). If `connect`ing the result
of `schedule(@_sch_@)` fails (i.e., `connect(schedule(@_sch_@),
@_rcvr_@)` throws where `@_rcvr_@` is a suitable receiver), `affine_on`
can avoid `start`ing the main work and fail on the execution agent
where it was `start`ed.  Otherwise, if it obtained an operation
state `@_os_@` from `connect(scheduler(@_sch_@), @_rcvr_@)`,
`affine_on` would `start` its main work and would `start(@_os_@)`
on the execution agent where the main work completed. If `start(@_os_@)`
is always successful, `affine_on` can achieve its objective. However,
if this scheduling operation fails, i.e., it completes with
`set_error(@_e_@)`, or if it gets cancelled, i.e., it completes
with `set_stopped()`, the execution agent on which the scheduling
operation resumes is unclear and `affine_on` cannot guarantee its
promise. Thus, it seems reasonable to require that a scheduler used
with `affine_on` is infallible, at least when used appropriately
(i.e., when providing a receiver whose associated stop token is an
`unstoppable_token`).

</p>
<p>
The current working draft specifies 4 schedulers:
</p>
<ol>
  <li>
  [`inline_scheduler`](https://wg21.link/exec.inline.scheduler) which
  just completes with `set_value()` when `start()`ed, i.e., this
  scheduler is already infallible.
  </li>
  <li>
  [`task_scheduler`](https://wg21.link/exec.task.scheduler) is a
  type-erased scheduler delegating to another scheduler. If the
  underlying scheduler is infallible, the only error case for
  `task_scheduler` is potential memory allocation during `connect`
  of its `@_ts-sender_@`. If `affine_on` creates an operation state
  for the scheduling operation during `connect`, it can guarantee
  that any necessary scheduling operation succeeds. Thus, this
  scheduler can be made infallible.
  </li>
  <li>
  The [`run_loop::@_run-loop-scheduler_@`](https://wg21.link/exec.run.loop)
  is used by [`run_loop`](https://wg21.link/exec.run.loop). The
  current specification allows the scheduling operation to fail
  with `set_error_t(std::exception_ptr)`. This permission allows
  an implementation to use [`std::mutex`](https://wg21.link/thread.mutex)
  and [`std::condition_variable`](https://wg21.link/thread.condition)
  whose operations may throw. It is possible to implement the logic
  using atomic operations which can't throw. The `set_stopped()`
  completion is only used when the receiver's stop token, i.e. the
  result of `get_stop_token(get_env(@_rcvr_@))`, was stopped. This
  receiver is controlled by `affine_on`, i.e., it can provide a
  [`never_stoptoken`](https://wg21.link/stoptoken.never) and this
  scheduler won't complete with `set_stopped()`.  If the
  [`get_completion_signatures`](https://wg21.link/exec.getcomplsigs) for
  the corresponding sender takes the environment into account, this
  scheduler can also be made infallible.
  </li>
  <li>
  The [`parallel_scheduler`](https://wg21.link/exec.par.scheduler)
  provides an interface to a replaceable implementation of a thread
  pool. The current interface allows
  [`parallel_scheduler`](https://wg21.link/exec.par.scheduler) to
  complete with `set_error_t(std::exception_ptr)` as well as with
  `set_stopped_t()`. It seems unlikely that this interface can be
  constrained to make it infallible.
  </li>
</ol>
<p>
In general it seems unlikely that all schedulers can be constrained
to be infallible. As a result `affine_on` and, by extension, `task`
won't be usable with all schedulers if `affine_on` insists on using
only infallible schedulers. If there are fallible schedulers, there
aren't any good options for using them with a `task`.  Note that
`affine_on` can fail and get cancelled (due to the main work failing
or getting cancelled) but `affine_on` can still guarantee that
execution resumes on the expect execution agent when it uses an
infallible scheduler.
</p>
<p>
This change addresses
[US 235-363](https://github.com/cplusplus/nbballot/issues/938)
([LWG4332](https://cplusplus.github.io/LWG/issue4332)). This change
goes beyond the actual issue and clarifies that the scheduling
operation used be `affine_on` needs to be always successful.
</p>

### Require Infallible Schedulers For `affine_on`

<p>
If `affine_on` promises in all cases that it resumes on the
original scheduler it can only work with infallible schedulers.
If a users wants to use a fallible scheduler with `affine_on` or
`task` the scheduler will need to be adapted. The adapted scheduler
can define what it means when the underlying scheduler fails. There
are conceptually only two options (the exact details may vary) on how
to deal with a failed scheduling operation:
</p>
<ol>
<li>
The user can transform the scheduling failure into a call to
`std::terminate`.
</li>
<li>
The user can consider resuming on an execution agent where the
adapting scheduler can schedule to infallibly (e.g., the execution
agent on which operation completed) but which is different from
execution agent associated with the adapted scheduler to be suitable
to continue running.  In that case the scheduling operation would
just succeed without necessarily running on the correct execution
agent. However, there is no indication that scheduling to the adapted
scheduler failed and the scheduler affinity may be impacted in this
failure case.
</li>
</ol>

The standard library doesn't provide a way to adapt schedulers
easily.  However, it can certainly be done.

### Allow Fallible Schedulers For `affine_on`

<p>
If the scheduler used with `affine_on` is allowed to fail, `affine_on`
can't guarantee that it completes on the correct scheduler in case of
an error completion. It could be specified that `affine_on` completes
with `set_error(@_rcvr_@, scheduling_error{@_e_@})` when the scheduling
operation completes with `set_error(@_r_@, @_e_@)` to make it detectable
that it didn't complete on the correct scheduler. This situation is
certainly not ideal but, at least, only affects the error completion and
it can be made detectable.
</p>
<p>
A use of `affine_on` which always needs to complete on a specific scheduler
is still possible: in that case the user will need to make sure that the
used scheduler is infallible. The main issue here is that there is no
automatic static checking whether that is the case.
</p>

### Considerations On Infallible Schedulers

In an ideal world, all schedulers would be infallible. It is unclear
if that is achievable. If schedulers need to be allowed to be fallible,
it may be viable to require that all standard library schedulers
are infallible. As outlined above that should be doable for all current
schedulers except, possibly, `parallel_scheduler`. So, the proposed
change is to require schedulers to be infallible when being used with
`affine_on` (and, thus, being used by `task`) and to change as many of
the standard C++ libraries to be infallible as possible.

If constraining `affine_on` to only infallible schedulers turns out
to be too strong, the constraint can be relaxed in a future revision
of the standard by explicitly opting out of that constraints, e.g.,
using an additional argument. For `task` to make use of it, it too
would need an explicit mechanisms to indicate that its `affine_on`
use should opt out of the constraint, e.g., by adding a suitable
`static` member to the environment template argument.

## `affine_on` Customization

Senders which don't cause the execution agent to be changed like
`just` or the various queries should be able to customize `affine_on`
to avoid unnecessary scheduling. Sadly, a proposal
([P3206](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3206r0.pdf))
to standardize properties which could be used to determine how a
sender completes didn't make much progress, yet. An implementation
can make use of similar techniques using an implementation-specific
protocol. If a future standard defines a standard approach to
determine the necessary properties the implementation can pick up
on those.

The idea is to have `affine_on` define a `transform_sender(s)`
member function which determines what sender should be returned.
By default the argument is returned but if the child sender indicates
that it doesn't actually change the execution agent the function
would return the child sender. There are a number of senders for
which this can be done:

- `just`, `just_error`, and `just_stopped`
- `read_env` and `write_env`
- `then`, `upon_error`, and `upon_stopped` if the child sender
    doesn't change the execution agent

The proposal is to define a `transform_sender` member which uses
an implementation-specific property to determine that a sender
completes on the same execution agent as the one it was started on.
In addition, it is recommended that this property gets defined by
the various standard library senders where it can make a difference.

This change addresses
[US 232-366](https://github.com/cplusplus/nbballot/issues/941)
([LWG4329](https://cplusplus.github.io/LWG/issue4329)), although
not in a way allowing application code to plug into this mechanism.
Such an approach can be designed in a future revision of the standard.

## Removing `change_coroutine_scheduler`

The current working paper specifies `change_coroutine_scheduler` to change
the scheduler used by the coroutine for scheduler affinity. It turns out that
this use is somewhat problematic in two ways:

1. Changing the scheduler affects the coroutine until the end of
    the coroutine or until `change_coroutine_scheduler` is `co_await`ed
    again. It doesn't automatically reset. Thus, local variables
    constructed before `change_coroutine_scheduler(s)` was
    `co_await`ed were constructed on the original scheduler and are
    destroyed on the replaced scheduler.
2. The `task`'s execution may finish on a different than the original
    scheduler. To allow symmetric transfer between two `task`s each
    `task` needs to complete on the correct scheduler. Thus, the
    `task` needs to be prepared to change to the original scheduler
    before actually completing.  To do so, it is necessary to know
    the original scheduler and also to have storage for the state
    needed to change to a different scheduler.  It can't be statically
    detected whether `change_coroutine_scheduler(s)` is `co_await`ed
    in the body of a coroutine and, thus, the necessary storage and
    checks are needed even for `task`s which don't use
    `change_coroutine_scheduler`.

If there were no way to change the scheduler it would still be possible
to execute using a different scheduler, although not as direct:
instead of using `co_await change_coroutine_scheduler(s)` to change
the scheduler used for affinity to `s` a nested `task` executing on `s`
could be `co_await`ed:

```c++
co_await ex::starts_on(s, [](@_parameters_@)->task<@_T_@, @_E_@> { @_logic_@ }(@_arguments_@));
```

Using this approach the use of the scheduler `s` is clearly limited
to the nested coroutine. The scheduler affinity is fully taken care
of by the use of `affine_on` when `co_await`ing work. There is no
need to provide storage or checks needed for the potential of
having a `task` return to the original scheduler if the scheduler
isn't actually changed by a `task`.

The proposal is remove `change_coroutine_scheduler` and the possibility
of changing the scheduler within a `task`. The alternative to
controlling the scheduler used for affinity from within a `task`
is a bit verbose. This need under the control of the coroutine is
likely relatively rare. Replacing the used scheduler for an existing
`task` by nesting it within `on(s, t)` or `starts_on(s, t)` is
fairly straightforward.

This functionality was originally included because it is present
for, at least, one of the existing libraries, although in a form
which was recommended against.  The existing use changes the scheduler
of a coroutine when `co_await`ing the result of `schedule(s)`; this
exact approach was found to be fragile and surprising and the
recommendation was to provide the functionality more explicit.

This change is not associated with any national body comment.
However, it is still important to do! It isn't adding any new
functionality but removes a problematic way to achieve something
which can be better achieved differently. If this change is not
made the inherent cost of having the possibility of having
`change_routine_scheduler` can't be removed later without breaking
existing code.

## `affine_on` Default Implementation

Using the previous discussion leads to a definition of `affine_on` which
is quite different from effectively just using `continues_on`:

1. The class `affine_on`
    should define a `transform_sender` member function which returns the
    child sender if this child sender indicates via an implementation
    specific way that it doesn't change the execution agent. It
    should be recommended that some of the standard library sender
    algorithms (see above) to indicate that they don't change the
    execution agent.
2. The `affine_on` algorithm should only allow to get `connect`ed to a
    receiver `r` whose scheduler `sched` obtained by
    `get_scheduler(get_env(r))` is infallible, i.e.,
    `get_completion_signatures(schedule(sched), e)` with an environment
    `e` where `get_stop_token(e)` yields `never_stop_token` returns
    `completion_signatures<set_value_t()>`.
3. When `affine_on` gets `connect`ed, the scheduling operation state needs
    to be created by `connect`ing the scheduler's sender to a suitable receiver to guarantee
    that the completion can be scheduled on the execution agent.
    The stop token `get_stop_token(get_env(r))` for the receiver
    `r` used for this `connect` shall be an `unstoppable_token`.
    The child sender also needs to be `connect`ed with a receiver
    which will capture the respective result upon completion and
    start the scheduling operation.
4. When the result operation state gets `start`ed it `start`s the
    operation state from the child operation.
5. Upon completion of the child operation the kind of completion and
    the parameters, if any, are stored. If this operation throws,
    the storage is set up to be as if `set_error(current_exception)`
    were called. Once the parameters are stored, the scheduling
    operation is started.
6. Upon completion of the scheduling operation, the appropriate
    completion function with the respective arguments is invoked.

This behavior is similar to `continues_on` but is subtly different
with respect to when the scheduling operation state needs to be
created and that any stop token from the receiver doesn't get
forwarded. In addition `affine_on` is more constrained with respect
to the schedulers it supports and the shape of the algorithm is
different: `affine_on` gets the scheduler to execute on from the
receiver it gets `connect`ed to.

This change addresses
[US 233-365](https://github.com/cplusplus/nbballot/issues/940)
([LWG4330](https://cplusplus.github.io/LWG/issue4330)) and
[US 236-362](https://github.com/cplusplus/nbballot/issues/937)
([LWG](https://cplusplus.github.io/LWG/issue4344); the proposed
resolution in this issue is incomplete).

## Name Change

The name `affine_on` isn't great. It may be worth giving the
algorithm a better name.

## Use get_start_scheduler Properly

During the LEWG discussion on 2026-03-24 it was brought up that
`get_start_scheduler` shouldn't fallback to `get_scheduler`. To
make proper use of `get_start_scheduler` without that fallback,
algorithms current using `get_scheduler` also need to use
`get_start_scheduler` and advertise it appropriately:

- `let*` pass the first operation's `completion_scheduler<set_value_t>`
  as the `get_start_scheduler` for the second operation
- `starts_on` should advertise `get_start_scheduler`
- `task` should propagate `get_start_scheduler` instead of
  `get_scheduler`. Since it names the type, the type is also
  renamed `start_scheduler_type`.
- `sync_wait` advertises its scheduler using both `get_scheduler`
  and `get_start_scheduler`.

# Wording Changes

::: ednote
This wording is relative to [N5032](https://wg21.link/N5032) with
the changes from
[P3826r3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3826r3.html)
applied.
:::

::: ednote

Add an exposition only concept `@_infallible-scheduler_@` to
[[exec.sched](https://wg21.link/exec.sched)]:

:::

...

[7]{.pnum} A scheduler type's destructor shall not block pending completion of any receivers connected to the sender objects returned from `schedule`.

::: add
[8]{.pnum} The exposition-only `@_infallible-scheduler_@` concept defines
the requirements of a scheduler type whose `schedule` asynchronous operation
can only complete with `set_value` unless stop can be requested:

```
template <class Sch, class Env>
concept @_infallible-scheduler_@ =
    scheduler<Sch> &&
    (same_as<completion_signatures<set_value_t()>,
             completion_signatures_of_t<decltype(schedule(declval<Sch>())), Env>
            > ||
     (!unstoppable_token<stop_token_of_t<Env>> && (
      same_as<completion_signatures<set_value_t(), set_stopped_t()>,
             completion_signatures_of_t<decltype(schedule(declval<Sch>())), Env>
            > ||
     same_as<completion_signatures<set_stopped_t(), set_value_t()>,
             completion_signatures_of_t<decltype(schedule(declval<Sch>())), Env>
            >)
     )
    );
```

:::


::: ednote
Add `get_start_scheduler` to the synopsis in [execution.syn] after
`get_scheduler` as follows:
:::

```
...
namespace std::execution {
  // [exec.queries], queries
  struct get_domain_t { @_unspecified_@ };
  struct get_scheduler_t { @_unspecified_@ };
  @[struct]{.add}@ @[get_start_scheduler_t]{.add}@ @@[{ @_unspecified_@ };]{.add}@@
  struct get_delegation_scheduler_t { @_unspecified_@ };
  struct get_forward_progress_guarantee_t { @_unspecified_@ };
  template<class CPO>
    struct get_completion_scheduler_t { @_unspecified_@ };
  template<class CPO = void>
    struct get_completion_domain_t { @_unspecified_@ };
  struct get_await_completion_adaptor_t { @_unspecified_@ };

  inline constexpr get_domain_t get_domain{};
  inline constexpr get_scheduler_t get_scheduler{};
  @[inline]{.add}@ @[constexpr]{.add}@ @[get_start_scheduler_t]{.add}@ @[get_start_scheduler{};]{.add}@
  inline constexpr get_delegation_scheduler_t get_delegation_scheduler{};
  enum class forward_progress_guarantee;
  inline constexpr get_forward_progress_guarantee_t get_forward_progress_guarantee{};
  template<class CPO>
    constexpr get_completion_scheduler_t<CPO> get_completion_scheduler{};
  template<class CPO = void>
    constexpr get_completion_domain_t<CPO> get_completion_domain{};
  inline constexpr get_await_completion_adaptor_t get_await_completion_adaptor{};
  ...
}
```

::: ednote
Add a new section after [exec.get.scheduler] as follows:
:::

:::add

### execution::get_start_scheduler [exec.get.start.scheduler]

[1]{.pnum} `get_start_scheduler` asks a queryable object for the scheduler
an operation will be or was started on.

[2]{.pnum} The name `get_start_scheduler` denotes a query object. For a
subexpression `env`, `get_start_scheduler(env)` is expression-equivalent to
<code><i>MANDATE-NOTHROW</i>(<i>AS-CONST</i>(env).query(get_start_scheduler))</code>.

<i>Mandates</i>: If the expression above is well-formed, its type satisfies `scheduler`.

[3]{.pnum} `forwarding_query(execution::get_start_scheduler)` is a core
constant expression and has value true.

[4]{.pnum} Given subexpressions `sndr` and `rcvr` such that
`sender_to<decltype((sndr)), decltype((rcvr))>` is `true` and the
expression `get_start_scheduler(get_env(rcvr))` is well-formed, an operation
state that is the result of calling `connect(sndr, rcvr)` shall, if
it is started, be started on an execution agent associated with the
scheduler `get_start_scheduler(get_env(rcvr))`.
:::

::: ednote

Add two paragraphs at the start of [exec.snd.expos] giving
implementations permission to add members `affine_on` and `as_awaitable`
to standard library sender types:

:::

::: add

[?]{.pnum} Given an expression `sndr`, whose type is any sender
type defined in the standard library, it is unspecified whether the
expression `sndr.affine_on()` is well-formed.  If that expression
is well-formed, then the evaluation thereof meets the
semantic requirements of the `affine_on` [exec.affine.on] algorithm.

[?]{.pnum} Given an expression `sndr`,  whose type is any sender type
defined in the standard library, and an expression `p`, whose type is a
promise type, it is unspecified whether the expression
`sndr.as_awaitable(p)` is well-formed.  If that expression is
well-formed, then the evaluation thereof meets the semantic
requirements of the `as_awaitable` [exec.as.awaitable] algorithm.

:::

::: ednote

Change [[exec.snd.expos](https://wg21.link/exec.snd.expos)] paragraph
8 to have <code><i>SCHED-ENV</i></code> use `get_start_scheduler`
instead of `get_scheduler`:

:::

[8]{.pnum} <code><i>SCHED-ENV</i>(sch)</code> is an expression `o2` whose type
satisfies <code><i>queryable</i></code> such that `o2.query(@[get_scheduler]{.rm}@@[get_start_scheduler]{.add}@)`
is a prvalue with the same type and value as `sch`, and such that
`o2.query(get_domain)` is expression-equivalent to `sch.query(get_domain)`.

::: ednote
The specification of `on` [[exec.on](https://wg21.link/exec.on)]
shouldn't use `write_env` as it does, i.e., this change removes
these (not removing them was an oversight in
[P3826](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3826r3.html)).

In addition, if `get_start_scheduler` is introduced change how `on`
gets its scheduler in [[exec.on](https://wg21.link/exec.on)], i.e.,
change the use from `get_scheduler` to use `get_start_scheduler`:

:::

<p>
...
</p>

<p>
[8]{.pnum} Otherwise, the expression `on.transform_sender(set_value, out_sndr, env)` has effects equivalent to:
</p>

```
auto&& [_, data, child] = out_sndr;
if constexpr (scheduler<decltype(data)>) {
  auto orig_sch =
    @_call-with-default_@(@[get_scheduler]{.rm}@@[get_start_scheduler]{.add}@, @_not-a-scheduler_@(), env);

  return continues_on(
    starts_on(std::forward_like<OutSndr>(data), std::forward_like<OutSndr>(child)),
    std::move(orig_sch));
} else {
  auto& [sch, closure] = data;
  auto orig_sch = @_call-with-default_@(
    get_completion_scheduler<set_value_t>, @_not-a-scheduler_@(), get_env(child), env);

  return continues_on(
      std::forward_like<OutSndr>(closure)(
        continues_on(
          std::forward_like<OutSndr>(child),
          sch)),
    orig_sch);
}
```
<p>
[9]{.pnum}
Let `out_sndr` be a subexpression denoting a sender returned from
`on(sch, sndr)` or one equal to such, and let `OutSndr` be the type
`decltype((out_sndr))`. Let `out_rcvr` be a subexpression denoting a
receiver that has an environment of type `Env` such that
`sender_in<OutSndr, Env>` is `true`. Let `op` be an lvalue referring
to the operation state that results from connecting `out_sndr` with
`out_rcvr`. Calling `start(op)` shall
<ul>
<li> [9.1]{.pnum} remember the current
scheduler[,]{.rm}[ which is obtained by]{.add} `@[get_scheduler]{.rm}@@[get_start_scheduler]{.add}@(get_env(rcvr))`;</li>
<li> [9.2]{.pnum} start `sndr` on an execution agent belonging to `sch`'s associated
execution resource;</li>
<li> [9.3]{.pnum}
upon `sndr`'s completion, transfer execution back to the execution resource associated with the scheduler remembered in step 1; and</li>
<li> [9.4]{.pnum}
forward `sndr`'s async result to `out_rcvr`.</li>
</ul>
If any scheduling operation fails, an error completion on `out_rcvr` shall be executed on an unspecified execution agent.
</p>

::: ednote

Change [[exec.sync.wait](https://wg21.link/exec.sync.wait#2)] p2 to also provide
a `get_start_scheduler` query:

:::

Let `@_sync-wait-env_@` be the following exposition-only class type:

```
namespace std::this_thread {
  struct sync-wait-env {
    execution::run_loop* loop;                                  // @_exposition only_@

    auto query(execution::get_scheduler_t) const noexcept {
      return loop->get_scheduler();
    }

    @[auto]{.add}@ @[query(execution::get_start_scheduler_t)]{.add}@ @[const]{.add}@ @[noexcept]{.add}@ @[{]{.add}@
      @[return]{.add}@ @[loop->get_scheduler();]{.add}@
    @[}]{.add}@

    auto query(execution::get_delegation_scheduler_t) const noexcept {
      return loop->get_scheduler();
    }
  };
}
```

::: ednote

Change [[exec.as.awaitable](https://wg21.link/exec.as.awaitable#7)]
paragraph 7 such that it tries to locate a customization for
`as_awaitable` on the transformed nested sender.

:::

[7]{.pnum} `as_awaitable` is a customization point object. For
subexpressions `expr` and `p` where `p` is an lvalue, `Expr` names
the type `decltype((expr))` and `Promise` names the type
`decay_t<decltype((p))>, as_awaitable(expr, p)` is expression-equivalent
to, except that the evaluations of `expr` and `p` are indeterminately
sequenced:

<ul>
<li>
<p>[7.1]{.pnum}
`expr.as_awaitable(p)` if that expression is well-formed.
</p>
<p>
Mandates: `@_is-awaitable_@<A, Promise>` is `true`, where `A` is
the type of the expression above.
</p>
</li>
<li>

[7.?]{.pnum}

[Otherwise,
`@_adapt-for-await-completion_@(transform_sender(expr,
get_env(p))).as_awaitable(p)` if this expression is well-formed,
`sender_in<Expr, env_of_t<Promise>>` is `true`, and
`@_single-sender-value-type_@<Expr, env_of_t<Promise>>` is well-formed,
except that `p` is only evaluated once.]{.add}

</li>
<li>
[7.2]{.pnum} Otherwise, `(void(p), expr)` if
`decltype(@_GET-AWAITER_@(expr))` satisfies `@_is-awaiter_@<Promise>`.
</li>
<li>
<p>
[7.3]{.pnum} [Otherwise, `@_sender-awaitable_@{@_adapted-expr_@, p}` if]{.rm}
</p>
<p>[`@_has-queryable-await-completion-adaptor_@<Expr>`]{.rm}</p>
<p>[and]{.rm}</p>
<p>[`@_awaitable-sender_@<decltype((@_adapted-expr_@)), Promise>`]{.rm}</p>
<p>
[are both satisfied, where `@_adapted-expr_@` is
`get_await_completion_adaptor(get_env(expr))(expr)`, except that
`expr` is evaluated only once.]{.rm}
</p>
</li>
<li>
[7.4]{.pnum} Otherwise, [`@_sender-awaitable_@{expr, p}` if
`@_awaitable-sender_@<Expr, Promise>` is `true`.]{.rm}
[`@_sender-awaitable_@{@_adapt-for-await-completion_@(transform_sender(expr, get_env(p))), p}` if `sender_in<Expr, env_of_t<Promise>>` is `true` and `@_single-sender-value-type_@<Expr, env_of_t<Promise>>` is well-formed], except that `p` is only evaluated once.{.add}

</li>
<li>[7.5]{.pnum} Otherwise, `(void(p), expr)`.
</li>
</ul>

::: add

[8]{.pnum} `@_adapt-for-await-completion_@(s)` is expression-equivalent
to
<ul>
<li>`get_await_completion_adaptor(get_env(s))(s)` if that is well-formed, except that `s` is evaluated only once,</li>
<li>otherwise, `s`.</li>
</ul>

:::

::: ednote
Change [exec.affine.on] to use only one parameter, require an
infallible scheduler from the receiver, and add a default implementation
which allows customization of `affine_on` for child senders.
The algorithm `affine_on` should use
`get_start_scheduler` to get the start scheduler:
:::

[1]{.pnum}
`affine_on` adapts a sender into one that completes on a [specified
scheduler]{.rm}[receiver's scheduler]{.add}. If the algorithm
determines that the adapted sender already completes on the correct
scheduler it can avoid any scheduling operation.

[2]{.pnum}
The name `affine_on` denotes a pipeable sender adaptor object. For
[a ]{.add} subexpression[s sch and]{.rm} `sndr`, if [`decltype((sch))`
does not satisfy scheduler, or]{.rm} `decltype((sndr))` does not
satisfy sender, <code>affine_on(sndr[, sch]{.rm})</code> is ill-formed.

[3]{.pnum}
Otherwise, the expression <code>affine_on(sndr[, sch]{.rm})</code>
is expression-equivalent to
`@_make-sender_@(affine_on, @[sch]{.rm}@@[env&lt;&gt;()]{.add}@, sndr)`.

:::{.add}
[?]{.pnum} For a subexpression `sch` whose type models `scheduler`,
let `@_UNSTOPPABLE-SCHEDULER_@(sch)` be an expression `e` whose type
models `scheduler` such that:

<ul>
<li>[?.1]{.pnum} `schedule(e)` is expression-equivalent to `unstoppable(schedule(sch))`.</li>
<li>[?.2]{.pnum} For any query object `q` and pack of subexpressions `args...`, `e.query(q, args...)`
is expression-equivalent to `sch.query(q, args...)`.</li>
<li>[?.3]{.pnum} The expression `e == @_UNSTOPPABLE-SCHEDULER_@(other)`
is expression-equivalent to `sch == other`.</li>
</ul>

[?]{.pnum}
Let `sndr` and `ev` be subexpressions such that `Sndr` is
`decltype((sndr))`. If <code><i>sender-for</i>&lt;Sndr,
affine_on_t&gt;</code> is `false`, then the expression
`affine_on.transform_sender(sndr, ev)` is ill-formed; otherwise,
it is equal to:

```
auto&[_, _, child] = sndr;
if constexpr (requires{ std::forward_like<Sndr>(child).affine_on(); })
    return std::forward_like<Sndr>(child).affine_on();
else
    return continues_on(std::forward_like<Sndr>(child), @_UNSTOPPABLE-SCHEDULER_@(get_start_scheduler(ev)));
```

[?]{.pnum}
_Recommended Practice_: Implementations should provide `affine_on`
member functions for senders that are known to resume on the
scheduler where they were started. Example senders for which that
is the case are `just`, `just_error`, `just_stopped`, `read_env`,
and `write_env`.

:::

[5]{.pnum}
Let <code>_out_sndr_</code> be a subexpression denoting a sender
returned from <code>affine_on(sndr[, sch]{.rm})</code> or one equal
to such, and let <code>_OutSndr_</code> be the type
<code>decltype((_out_sndr_))</code>. Let <code>out_rcvr</code>
be a subexpression denoting a receiver that has an environment of
type `Env` [such that <code>sender_in&lt;_OutSndr_, Env&gt;</code>
is `true`]{.rm}. [If <code>get_start_scheduler(get_env(out_rcvr))</code> is ill-formed
or does not satisfy `@_infallible-scheduler_@<Env>` then evaluation of the
expression `get_completion_signatures<@_OutSndr_@, Env>()` exits with
an exception.
]{.add} Let `op` be an lvalue referring to the operation
state that results from connecting <code>_out_sndr_</code> to
<code>_out_rcvr_</code>.  Calling <code>start(_op_)</code> will
start `sndr` on the current execution agent and execute completion
operations on <code>_out_rcvr_</code> on an execution agent of the
execution resource associated with [`sch`]{.rm}[<code>_sch_</code>]{.add}.
If the current execution resource is the same as the execution
resource associated with [`sch`]{.rm}[<code>_sch_</code>]{.add},
the completion operation on <code>_out_rcvr_</code> may be called
before <code>start(_op_)</code> completes. [If scheduling onto `sch`
fails, an error completion on <code>_out_rcvr_</code> shall be
executed on an unspecified execution agent.]{.rm}

::: ednote
Remove `change_coroutine_scheduler` from [execution.syn]:
:::

```
namespace std::execution {
  ...
  // [exec.task.scheduler], task scheduler
  class task_scheduler;

  template<class E>
  struct with_error {
    using type = remove_cvref_t<E>;
    type error;
  };
  template<class E>
    with_error(E) -> with_error<E>;
```
::: rm
```
  template<scheduler Sch>
  struct change_coroutine_scheduler {
    using type = remove_cvref_t<Sch>;
    type scheduler;
  };
  template<scheduler Sch>
    change_coroutine_scheduler(Sch) -> change_coroutine_scheduler<Sch>;
```
:::
```
  // [exec.task], class template task
  template<class T, class Environment>
    class task;
  ...
}
```

::: ednot

In [[task.class](https://wg21.link/task.class)] change `scheduler_type` to
`start_scheduler_type`:

:::

```
namespace std::execution {
  template<class T = void, class Environment = env<>>
  class task {
    // [task.state]
    template<receiver Rcvr>
      class @_state_@;                              // @_exposition only_@

  public:
    using sender_concept = sender_t;
    using completion_signatures = @_see below_@;
    using allocator_type = @_see below_@;
    using @[scheduler_type]{.rm}@@[start_scheduler_type]{.add}@ = @_see below_@;
    using stop_source_type = @_see below_@;
    using stop_token_type = decltype(declval<stop_source_type>().get_token());
    using error_types = @_see below_@;

    // [task.promise]
    class promise_type;

    task(task&&) noexcept;
    ~task();

    template<receiver Rcvr>
      @_state_@<Rcvr> connect(Rcvr&& rcvr) &&;

  private:
    coroutine_handle<promise_type> handle;      // @_exposition only_@
  };
}
```

[1]{.pnum}
`task<T, E>` models sender ([exec.snd]) if `T` is `void`, a reference
type, or a _cv_-unqualified non-array object type and `E` is a class
type. Otherwise a program that instantiates the definition of
`task<T, E>` is ill-formed.

[2]{.pnum}
The nested types of task template specializations are determined based on the `Environment` parameter:
<ul>
<li>[2.1]{.pnum}
`allocator_type` is `Environment::allocator_type` if that _qualified-id_
is valid and denotes a type, `allocator<byte>` otherwise.</li>
<li>[2.2]{.pnum}
`@[scheduler_type]{.rm}@@[start_scheduler_type]{.add}@` is `Environment::@[scheduler_type]{.rm}@@[start_scheduler_type]{.add}@` if that _qualified-id_
  is valid and denotes a type, `task_scheduler` otherwise.</li>
<li>[2.3]{.pnum}
`stop_source_type` is `Environment::stop_source_type` if that
_qualified-id_ is valid and denotes a type, `inplace_stop_source`
otherwise.</li>
<li>[2.4]{.pnum}
`error_types` is `Environment::error_types` if that _qualified-id_
is valid and denotes a type,
`completion_signatures<set_error_t(exception_ptr)>` otherwise.</li>
</ul>

[3]{.pnum}

A program is ill-formed if `error_types` is not a specialization
of `execution::completion_signatures` or if the template arguments
of that specialization contain an element which is not of the form
`set_error_t(E)` for some type `E`.

[4]{.pnum}
The type alias `completion_signatures` is a specialization of
`execution::completion_signatures` with the template arguments (in
unspecified order):

<ul>
<li>[4.1]{.pnum}
`set_value_t()` if `T` is `void`, and `set_value_t(T)` otherwise;</li>
<li>[4.2]{.pnum}
template arguments of the specialization of
`execution::completion_signatures` denoted by `error_types`; and</li>
<li>[4.3]{.pnum} `set_stopped_t()`.</li>
</ul>
[5]{.pnum} `allocator_type` shall meet the _Cpp17Allocator_ requirements.

::: ednote
Change the scheduler propagation to `start` to use  `get_start_scheduler` in [[task.state](https://wg21.link/task.state#4)] p4:
:::

```
void start() & noexcept;
```

[4]{.pnum} _Effects_: Let `@_prom_@` be the object `@_handle_@.promise()`. Associates `@_STATE_@(@_prom_@)`, `@_RCVR_@(@_prom_@)`, and `@_SCHED_@(@_prom_@)` with `*this` as follows:

<ul>
<li>[4.1]{.pnum} `@_STATE_@(prom)` is `*this`.</li>
<li>[4.2]{.pnum} `@_RCVR_@(@_prom_@)` is `@_rcvr_@`.</li>
<li>[4.3]{.pnum} `@_SCHED_@(@_prom_@)` is the object initialized with
`@[scheduler_type]{.rm}@@[start_scheduler_type]{.add}@(@[get_scheduler]{.rm}@@[get_start_scheduler]{.add}@(get_env(@_rcvr_@)))` if that expression is
valid and `@[scheduler_type]{.rm}@@[start_scheduler_type]{.add}@()` otherwise. If neither of these expressions
is valid, the program is ill-formed.</li>
</ul>

Let `@_st_@` be `get_stop_token(get_env(@_rcvr_@))`. Initializes `@_prom_@.@_token_@`
and `@_prom.source_@` such that

<ul>
<li>[4.4]{.pnum}
`@_prom_@.@_token_@.stop_requested()` returns `@_st_@.stop_requested()`;</li>
<li>[4.5]{.pnum}
`@_prom_@.@_token_@.stop_possible()` returns `@_st_@.stop_possible()`; and</li>
<li>[4.6]{.pnum}
for types `Fn` and `Init` such that both `invocable<Fn>` and
`constructible_from<Fn, Init>` are modeled,
`stop_token_type::callback_type<Fn>` models
`@_stoppable-callback-for_@<Fn, stop_token_type, Init>`.</li>
</ul>

After that invokes `@_handle_@.resume()`.

::: ednote
Use `get_start_scheduler` instead of `get_scheduler` when providing
an environment to the `co_await`ed sender in
[[task.state](https://wg21.link/task.state#16)] p16:
:::

```
@_unspecified_@ get_env() const noexcept;
```
[16]{.pnum}

_Returns_: An object `env` such that queries are forwarded as follows:
<ul>
<li>[16.1]{.pnum}
`env.query(@[get_scheduler]{.rm}@@[get_start_scheduler]{.add}@)` returns `@[scheduler_type]{.rm}@@[start_scheduler_type]{.add}@(@_SCHED_@(*this))`.</li>
<li>[16.2]{.pnum}
`env.query(get_allocator)` returns `@_alloc_@`.</li>
<li>[16.3]{.pnum}
`env.query(get_stop_token)` returns `@_token_@`.</li>
<li>[16.4]{.pnum} For any other query `q` and arguments `a...` a call
to `env.query(q, a...)` returns `STATE(*this).environment.query(q,
a...)` if this expression is well-formed and `forwarding_query(q)` is
well-formed and is `true`. Otherwise `env.query(q, a...)` is ill-formed.</li>
</ul>

::: ednote
Adjust the use of `affine_on` and remove `change_coroutine_scheduler` from [task.promise]:
:::

```
namespace std::execution {
  template<class T, class Environment>
  class task<T, Environment>::promise_type {
  public:
    ...

    template<class A>
      auto await_transform(A&& a);
```
::: rm
```
    template<class Sch>
      auto await_transform(change_coroutine_scheduler<Sch> sch);
```
:::
```

    @_unspecified_@ get_env() const noexcept;

    ...
  }
};
```
...

```
template<sender Sender>
  auto await_transform(Sender&& sndr) noexcept;
```
[9]{.pnum}
_Returns_: If `same_as<inline_scheduler, @[scheduler_type]{.rm}@@[start_scheduler_type]{.add}@>` is `true` returns `as_awaitable(​std​::​​forward<Sender>(sndr), *this);` otherwise returns `as_awaitable(affine_on(​std​::​​forward<Sender>(sndr)@[, SCHED(*this)]{.rm}@), *this)`.

::: rm
```
template<class Sch>
  auto await_transform(change_coroutine_scheduler<Sch> sch) noexcept;
```
[10]{.pnum}
_Effects_: Equivalent to:
```
return await_transform(just(exchange(SCHED(*this), scheduler_type(sch.scheduler))), *this);
```
:::

```
void unhandled_exception();
```
[11]{.pnum}
_Effects_: If the signature `set_error_t(exception_ptr)` is not an element of `error_types`, calls `terminate()` ([except.terminate]). Otherwise, stores `current_exception()` into <code>_errors_</code>.

...

::: ednote
In [exec.task.scheduler] change the constructor of `task_scheduler` to require that the scheduler passed
is infallible
:::

```
template<class Sch, class Allocator = allocator<void>>
  requires(!same_as<task_scheduler, remove_cvref_t<Sch>>) && scheduler<Sch>
explicit task_scheduler(Sch&& sch, Allocator alloc = {});
```

::: add
[?]{.pnum}
_Mandates_: `Sch` satisfies `@_infallible-scheduler_@<env<>>`.

:::

[2]{.pnum}
_Effects_: Initialize <code><i>sch_</i></code> with `allocate_shared<remove_cvref_t<Sch>>(alloc,​ std​::​forward<Sch>​(sch))`.

[3]{.pnum}
_Recommended practice_: Implementations should avoid the use of
dynamically allocated memory for small scheduler objects.

[4]{.pnum}
_Remarks_: Any allocations performed by construction of
<code>_ts-sender_</code> or <code>_state_</code> objects resulting
from calls on `*this` are performed using a copy of `alloc`.

::: ednote
In [exec.task.scheduler] change the <code><i>ts-sender</i></code> completion signatures
to indicate that `task_scheduler` is infallible:
:::

[8]{.pnum}
```
namespace std::execution {
  class task_scheduler::@_ts-sender_@ {     // @_exposition only_@
  public:
    using sender_concept = sender_t;

    template<receiver Rcvr>
      @_state_@<Rcvr> connect(Rcvr&& rcvr) &&;
  };
}
```

<code><i>ts-sender</i></code> is an exposition-only class that
models `sender` ([exec.snd]) and for which
<code>completion_signatures_of_t&lt;<i>ts-sender</i>[, E]{.add}&gt;</code>
denotes[:]{.rm}[ `completion_signatures<set_value_t()>` if `unstoppable_token<stop_token_of_t<E>>` is `true`, and
otherwise `completion_signatures<set_value_t(), set_stopped_t()>`.]{.add}

::: rm
```
completion_signatures<
  set_value_t(),
  set_error_t(error_code),
  set_error_t(exception_ptr),
  set_stopped_t()>
```
:::

::: ednote
In [exec.run.loop.types] change the paragraph defining the completion signatures:
:::

...

```
class run-loop-sender;
```

[6]{.pnum}
<code><i>run-loop-sender</i></code> is an exposition-only type that satisfies `sender`.
[Let `E` be the type of an environment. If `unstoppable_token<stop_token_of_t<E>>` is `true`,
then ]{.add} <code>completion_signatures_of_t&lt;<i>run-loop-sender</i>[, E]{.add}&gt;</code> is

::: rm
```
  completion_signatures<set_value_t(), set_error_t(exception_ptr), set_stopped_t()>
```
:::

::: add
```
  completion_signatures<set_value_t()>
```
Otherwise it is
```
  completion_signatures<set_value_t(), set_stopped_t()>
```
:::

[7]{.pnum} An instance of <code><i>run-loop-sender</i></code> remains
valid until the end of the lifetime of its associated `run_loop`
instance.

...

::: ednote
Change
[[exec.counting.scopes.general](https://wg21.link/exec.counting.scopes.general#4)]
p4 to use `get_start_scheduler` instead of `get_scheduler`:
:::

[4]{.pnum} The exposition-only class template `@_impls-for_@` ([exec.snd.expos]) is specialized for `@_scope-join-t_@` as follows:

```
namespace std::execution {
  template<>
  struct @_impls-for_@<@_scope-join-t_@> : @_default-impls_@ {
    template<class Scope, class Rcvr>
    struct state {                          // @_exposition only_@
      struct @_rcvr-t_@ {                       // @_exposition only_@
        using receiver_concept = receiver_t;

        Rcvr& @_rcvr_@;                         // @_exposition only_@

        void set_value() && noexcept {
          execution::set_value(std::move(@_rcvr_@));
        }

        template<class E>
          void set_error(E&& e) && noexcept {
            execution::set_error(std::move(@_rcvr_@), std::forward<E>(e));
          }

        void set_stopped() && noexcept {
          execution::set_stopped(std::move(@_rcvr_@));
        }

        decltype(auto) get_env() const noexcept {
          return execution::get_env(@_rcvr_@);
        }
      };

      using @_sched-sender_@ =                  // @_exposition only_@
        decltype(schedule(@[get_scheduler]{.rm}@@[get_start_scheduler]{.add}@(get_env(declval<Rcvr&>()))));
      using @_op-t_@ =                          // @_exposition only_@
        connect_result_t<@_sched-sender_@, @_rcvr-t_@>;

      Scope* @_scope_@;                         // @_exposition only_@
      Rcvr& @_receiver_@;                       // @_exposition only_@
      @_op-t_@ @_op_@;                              // @_exposition only_@

      @_state_@(Scope* scope, Rcvr& rcvr)       // @_exposition only_@
        noexcept(@_nothrow-callable_@<connect_t, @_sched-sender_@, @_rcvr-t_@>)
        : @_scope_@(scope),
          @_receiver_@(rcvr),
          @_op_@(connect(schedule(@[get_scheduler]{.rm}@@[get_start_scheduler]{.add}@(get_env(rcvr))), @_rcvr-t_@(rcvr))) {}

      void @_complete_@() noexcept {            // @_exposition only_@
        start(@_op_@);
      }

      void @_complete-inline_@() noexcept {     // @_exposition only_@
        set_value(std::move(@_receiver_@));
      }
    };

    static constexpr auto @_get-state_@ =       // @_exposition only_@
      []<class Rcvr>(auto&& sender, Rcvr& receiver)
        noexcept(is_nothrow_constructible_v<@_state_@<Rcvr>, @_data-type_@<decltype(sender)>, Rcvr&>) {
        auto[_, self] = sender;
        return @_state_@(self, receiver);
      };

    static constexpr auto @_start_@ =           // @_exposition only_@
      [](auto& s, auto&) noexcept {
        if (s.@_scope_@->@_start-join-sender_@(s))
          s.@_complete-inline_@();
      };
  };
}
```
