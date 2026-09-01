# Getting The Lazy Task Done

## Description

The C++26 specificatiom includes the sender/receiver framework for
dealing with asynchronous operations. It defines a vocualary and
various algorithms and utilities using this vocabulary. The current
specification doesn't include matching coroutine task class template
although it is expected that most users would use coroutines to
actually use the facilities. This presentation discusses various
aspects for creating a coroutine task suitable for many needs:

- The basic operation of a sender/receiver aware task.
- Scheduler affinity and why it is important.
- Allocator support for the coroutine frame.
- Providing an environment to child operations.
- Propagating errors without exceptions.

## Outline

The presentation starts off with motivating why coroutines are a
reasonable approach to implementing asynchronous algorithms and
show some basic usage. That will include some motivation of why an
environment is important as a replacement for thread local variables
and why executing from common scheduler is normally desirable.

The most contentious aspect is probably scheduler affinity and some
time is spent on showing how it is implement, motivating why it is
important, and why it should be the default. There are some use
cases where scheduler affinity may be undesirable and it is also
shown how it can be avoided.

The coroutine frame typically can't live on the stack and needs to
be allocated while the HALO operations are not always applicable. It
is shown how allocation of the coroutine frame can be customized if
it is desired using an allocator.

Any use allocation would likely be part of an environment exposed to
child operations. It should also be possible to customize the available
environment with user-specified properties. It is showing how to get
access to environment properties and how they can be customized.

Error handling and cancellation are important aspects which are directly
supported by the sender/receiver framework. There are a few ways how a
coroutine can interact with error handling and cancellation and the
different options are discussed.
