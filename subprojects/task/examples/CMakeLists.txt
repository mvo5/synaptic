# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.30...4.3)

# include(../cmake/prelude.cmake)

project(beman_task.example LANGUAGES CXX)

# TODO(CK): cmake -S . -G Ninja -B build -D CMAKE_BUILD_TYPE=RelWithDebInfo --fresh
if(PROJECT_IS_TOP_LEVEL)
    # include(../cmake/cxx-modules-rules.cmake)
    set(BEMAN_USE_MODULES OFF)
    set(BEMAN_USE_STD_MODULE OFF)
    set(CMAKE_CXX_STANDARD 23)

    find_package(beman.task 0.2.0 EXACT REQUIRED)

    enable_testing()
endif()

set(TODO tls-scheduler into_optional issue-start-reschedules loop)

set(ALL_EXAMPLES
    task_scheduler
    rvalue-task
    customize
    issue-symmetric-transfer
    container
)

if(NOT MSVC)
    list(
        APPEND ALL_EXAMPLES
        task-sender
        alloc-1
        alloc-2
        c++now-allocator
        c++now-cancel
        c++now-errors
        alloc
        affinity
        odd-return
        dangling-references
        aggregate-return
        issue-affine_on
        c++now-affinity
        c++now-basic
        # FIXME: c++now-query
        c++now-result-types
        c++now-return
        # FIXME: c++now-stop_token
        c++now-with_error
        co_await-result
        co_await-task
        error
        escaped-exception
        friendly
        hello
        issue-frame-allocator
        # FIXME: query
        result_example
        # FIXME: stop
    )
endif()

set(xALL_EXAMPLES issue-symmetric-transfer)
set(xALL_EXAMPLES customize)

message(STATUS "Examples to be built: ${ALL_EXAMPLES}")

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.task.examples.${example})
    target_sources(beman.task.examples.${example} PRIVATE ${example}.cpp)
    target_link_libraries(beman.task.examples.${example} beman::task_headers)
    add_test(
        NAME beman.task.examples.${example}
        COMMAND $<TARGET_FILE:beman.task.examples.${example}>
    )
endforeach()
