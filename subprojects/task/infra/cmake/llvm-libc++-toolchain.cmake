# SPDX-License-Identifier: BSL-1.0

# This toolchain file is not meant to be used directly,
# but to be invoked by CMake preset and GitHub CI.
#
# This toolchain file configures for LLVM family of compiler.
#
# BEMAN_BUILDSYS_SANITIZER:
# This optional CMake parameter is not meant for public use and is subject to
# change.
# Possible values:
# - MaxSan: configures clang and clang++ to use all available non-conflicting
#           sanitizers.
# - TSan:   configures clang and clang++ to enable the use of thread sanitizer.

include(${CMAKE_CURRENT_LIST_DIR}/llvm-toolchain.cmake)

if(NOT CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
    string(APPEND CMAKE_CXX_FLAGS " -stdlib=libc++")
endif()
