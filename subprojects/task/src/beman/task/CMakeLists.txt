# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

add_library(beman.task STATIC)
add_library(beman::task ALIAS beman.task)

target_sources(beman.task PRIVATE task.cpp)

add_library(beman.task_headers INTERFACE)
add_library(beman::task_headers ALIAS beman.task_headers)
target_sources(
    beman.task_headers
    PUBLIC
        FILE_SET HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}/include
            FILES
                ${PROJECT_SOURCE_DIR}/include/beman/execution/task.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/lazy/lazy.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/task.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/allocator_of.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/allocator_support.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/awaiter.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/change_coroutine_scheduler.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/completion.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/error_types_of.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/final_awaiter.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/find_allocator.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/handle.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/into_optional.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/logger.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/poly.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/promise_base.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/promise_env.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/promise_type.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/result_type.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/scheduler_of.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/single_thread_context.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/state_base.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/state_rep.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/state.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/stop_source.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/sub_visit.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/task_scheduler.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/task.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/task/detail/with_error.hpp
)

set_target_properties(
    beman.task_headers
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ${PROJECT_IS_TOP_LEVEL}
)
target_link_libraries(beman.task_headers INTERFACE beman::execution_headers)

target_link_libraries(beman.task PUBLIC beman::task_headers)
