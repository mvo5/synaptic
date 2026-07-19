// include/beman/task/detail/logger.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_LOGGER
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_LOGGER

#include <algorithm>
#include <iterator>
#include <iostream>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
struct logger {
    static auto level(int i) -> int {
        static int rc{};
        return rc += i;
    }
    ::std::ostream& log(const char* pre, const char* msg) {
        ::std::fill_n(::std::ostreambuf_iterator<char>(std::cout.rdbuf()), level(0), ' ');
        std::cout << pre;
        return std::cout << msg << "\n";
    }
    ::std::ostream& log(const char* msg) { return log("| ", msg); }
    const char*     msg;
    explicit logger(const char* m) : msg(m) {
        level(1);
        log("\\ ", this->msg);
    }
    logger(logger&&) = delete;
    ~logger() {
        log("/ ", this->msg);
        level(-1);
    }
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
