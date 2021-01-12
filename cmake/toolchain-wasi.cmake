# Daemon BSD Source Code
# Copyright (c) 2013-2016, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the <organization> nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required (VERSION 3.13)

if (WIN32)
    set(WASI_EXE_SUFFIX ".exe")
else()
    set(WASI_EXE_SUFFIX "")
endif()

# TODO(WASM) change to WASI? Does we need to set other CMAKE_SYSTEM_*?
set(CMAKE_SYSTEM_NAME "Generic")

set(CMAKE_C_COMPILER "${WASI_SDK_DIR}/bin/clang${WASI_EXE_SUFFIX}")
set(CMAKE_CXX_COMPILER "${WASI_SDK_DIR}/bin/clang++${WASI_EXE_SUFFIX}")
set(CMAKE_AR "${WASI_SDK_DIR}/bin/ar${WASI_EXE_SUFFIX}")
set(CMAKE_RANLIB "${WASI_SDK_DIR}/bin/ranlib${WASI_EXE_SUFFIX}")

set(CMAKE_C_FLAGS "--sysroot=${WASI_SDK_DIR}/share/wasi-sysroot")
set(CMAKE_CXX_FLAGS "--sysroot=${WASI_SDK_DIR}/share/wasi-sysroot -fno-exceptions")

set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(WASM ON)
