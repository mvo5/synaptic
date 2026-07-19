# beman.task: Beman Library Implementation of `task` (P3552)

<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

![Continuous Integration Tests](https://github.com/bemanproject/task/actions/workflows/ci_tests.yml/badge.svg)
![Standard Target](https://github.com/bemanproject/beman/blob/main/images/badges/cpp26.svg)
![Coverage](https://coveralls.io/repos/github/bemanproject/task/badge.svg?branch=main)
![Library Status](https://raw.githubusercontent.com/bemanproject/beman/refs/heads/main/images/badges/beman_badge-beman_library_under_development.svg)

`beman::execution::task<T, Context>` is a class template which
is used as the the type of coroutine tasks. The corresponding objects
are senders.  The first template argument (`T`) defines the result
type which becomes a `set_value_t(T)` completion signatures. The
second template argument (`Context`) provides a way to configure
the behavior of the coroutine. By default it can be left alone.

**Implements**: `std::execution::task` proposed in [Add a Coroutine Lazy Type (P3552)](https://wg21.link/P3552).

**Status**: [Under development and not yet ready for production use.](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#under-development-and-not-yet-ready-for-production-use)

## License

`beman.task` is licensed under the Apache License v2.0 with LLVM Exceptions.

## Usage

The following code snippet shows a basic use of `beman::task::task`
using sender/receiver facilities to implement version of `hello,
world`:

```cpp
#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>

namespace ex = beman::execution;
namespace ly = beman::task;

int main() {
    return std::get<0>(*ex::sync_wait([]->ex::task<int> {
        std::cout << "Hello, world!\n";
        co_return co_await ex::just(0);
    }()));
}
```

Full runnable examples can be found in [`examples/`](`./example`) (e.g., [`./examples/hello.cpp`](./examples/hello.cpp)). For some explanation see [`./docs/examples.md`](./docs/examples.md).

## Help Welcome

There are plenty of things which need to be done. See the
[contributions page](https://github.com/bemanproject/task/blob/main/docs/contributing.md)
for some ideas how to contribute. The [resources page](https://github.com/bemanproject/task/blob/main/docs/resources.md)
contains some links for general information about coroutines.

## Building beman.task

### Dependencies

This project depends on
[`beman::execution`](https://github.com/bemanproject/execution) (which
will be automatically obtained using `cmake`'s `FetchContent*`).

Build-time dependencies:

- `cmake`
- `ninja`, `make`, or another CMake-supported build system
  - CMake defaults to "Unix Makefiles" on POSIX systems

### Supported Platforms

| Compiler | Version | C++ Standards | Standard Library |
|----------|---------|---------------|------------------|
| GCC      | 15-14   | C++26, C++23  | libstdc++        |
| Clang    | 21-19   | C++26, C++23  | libc++           |
| MSVC     | latest  | C++23         | MSVC STL         |

### How to build beman.task

<!-- TODO: Apply all install related practices from exemplar into this repo. -->

This project strives to be as normal and simple a CMake project as
possible.  This build workflow in particular will work, producing
a static `libbeman.task.a` library, ready to package with its
headers:

```shell
# Build beman.task.
$ cmake --workflow --preset gcc-debug
$ cmake --workflow --preset gcc-release

# Install beman.task into your system.
$ cmake --install build/gcc-release/ --prefix /opt/beman.task/
-- Install configuration: "RelWithDebInfo"
-- Installing: /opt/beman.task/lib/libbeman.task.a
-- Installing: /opt/beman.task/include/beman/task/task.hpp
[...]

$ tree /opt/beman.task/
/opt/beman.task/
├── include
│   └── beman
│       └── task
│           ├── detail
│           │   ├── affine_on.hpp
│           │   ├── ...
│           │   ├── task.hpp
│           │   └── with_error.hpp
│           └── task.hpp
└── lib
    ├── cmake
    │   └── beman
    │       └── task
    │           └── BemanTaskConfig.cmake
    └── libbeman.task.a

9 directories, 26 files
```

## Contributing

Please do! Issues and pull requests are appreciated.
