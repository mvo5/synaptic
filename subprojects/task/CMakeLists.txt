# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.30...4.3)

project(
    beman.task # CMake Project Name, which is also the name of the top-level
    # targets (e.g., library, executable, etc.).
    DESCRIPTION "A Beman library task (coroutine task)"
    LANGUAGES CXX
    VERSION 0.2.0
)

# [CMAKE.SKIP_TESTS]
option(
    BEMAN_TASK_BUILD_TESTS
    "Enable building tests and test infrastructure. Default: ON. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

# [CMAKE.SKIP_EXAMPLES]
option(
    BEMAN_TASK_BUILD_EXAMPLES
    "Enable building examples. Default: ON. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

include(FetchContent)

FetchContent_Declare(
    execution
    # FETCHCONTENT_SOURCE_DIR_EXECUTION ${CMAKE_SOURCE_DIR}/../execution
    GIT_REPOSITORY https://github.com/bemanproject/execution
    GIT_TAG 7df0f75
    SYSTEM
    FIND_PACKAGE_ARGS
        0.2.0
        EXACT
        NAMES
        beman.execution
        COMPONENTS
        execution_headers
)
FetchContent_MakeAvailable(execution)

add_subdirectory(src/beman/task)

#===============================================================================
# NOTE: this must be done before tests! CK
include(infra/cmake/beman-install-library.cmake)
beman_install_library(${PROJECT_NAME} TARGETS ${PROJECT_NAME} beman.task_headers
    DEPENDENCIES [===[beman.execution 0.2.0 EXACT]===]
)
#===============================================================================

enable_testing()

if(BEMAN_TASK_BUILD_TESTS)
    add_subdirectory(tests/beman/task)
endif()

if(BEMAN_TASK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
