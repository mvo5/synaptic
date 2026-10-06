# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# This toolchain file is not meant to be used directly,
# but to be invoked by CMake preset and GitHub CI.
#
# This toolchain file configures for apple clang family of compiler.
# Note this is different from LLVM toolchain.
#
# BEMAN_BUILDSYS_SANITIZER:
# This optional CMake parameter is not meant for public use and is subject to
# change.
# Possible values:
# - MaxSan: configures clang and clang++ to use all available non-conflicting
#           sanitizers. Note that apple clang does not support leak sanitizer.
# - TSan:   configures clang and clang++ to enable the use of thread sanitizer.

include_guard(GLOBAL)

# Prevent PATH collision with an LLVM clang installation by using the system
# compiler shims
set(CMAKE_C_COMPILER cc)
set(CMAKE_CXX_COMPILER c++)

if(BEMAN_BUILDSYS_SANITIZER STREQUAL "MaxSan")
    set(SANITIZER_FLAGS
        "-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=undefined"
    )
elseif(BEMAN_BUILDSYS_SANITIZER STREQUAL "TSan")
    set(SANITIZER_FLAGS "-fsanitize=thread")
endif()

set(CMAKE_C_FLAGS_DEBUG_INIT "${SANITIZER_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "${SANITIZER_FLAGS}")

set(RELEASE_FLAGS "-O3 ${SANITIZER_FLAGS}")

set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "${RELEASE_FLAGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "${RELEASE_FLAGS}")

set(CMAKE_C_FLAGS_RELEASE_INIT "${RELEASE_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "${RELEASE_FLAGS}")

# Add this dir to the module path so that `find_package(beman-install-library)` works
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}")
