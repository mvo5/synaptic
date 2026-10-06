# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-22.04

USER vscode

# Install latest cmake
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
RUN echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
RUN sudo apt-get update && sudo apt-get install -y cmake

# Install pre-commit
RUN sudo apt-get install -y python3-pip && pip3 install pre-commit

# Avoid ASAN Stalling
# See: https://github.com/google/sanitizers/issues/1614
# Alternative is to update to above clang-18 and gcc-13.2
# Maybe crashing codespace???
RUN sudo sysctl -w vm.mmap_rnd_bits=28
