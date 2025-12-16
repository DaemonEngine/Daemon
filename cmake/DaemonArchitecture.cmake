# Daemon BSD Source Code
# Copyright (c) 2022, Daemon Developers
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

################################################################################
# Determine architecture
################################################################################

# When adding a new architecture, look at all the places ARCH is used

try_compile(BUILD_RESULT
	"${CMAKE_BINARY_DIR}"
	"${DAEMON_DIR}/cmake/DaemonArchitecture/DaemonArchitecture.cpp"
	CMAKE_FLAGS CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
	OUTPUT_VARIABLE BUILD_LOG
)

# Setting -Werror in CXXFLAGS would produce errors instead of warning
# but that should not break the architecture detection,
# so we only print a CMake warning there and use BUILD_LOG content to
# detect unknown platforms.
# Catching compilation error is still useful, for example to detect
# undefined types, missing header or things like that.
# Setting USE_WERROR to ON doesn't print this warning.
if (NOT BUILD_RESULT)
	message(WARNING
		"Failed to build DaemonArchitecture.cpp\n"
		"Setting -Werror in CXXFLAGS can produce false positive errors\n"
		"${BUILD_LOG}"
	)
endif()

string(REGEX MATCH "DAEMON_ARCH_([a-zA-Z0-9_]+)" ARCH_DEFINE "${BUILD_LOG}")
string(REPLACE "DAEMON_ARCH_" "" ARCH "${ARCH_DEFINE}")

if (NOT ARCH)
	message(FATAL_ERROR
		"Missing DAEMON_ARCH, there is a mistake in DaemonArchitecture.cpp\n"
		"${BUILD_LOG}"
	)
endif()

message(STATUS "Detected architecture: ${ARCH}")

add_definitions(-D${ARCH_DEFINE})

# This string can be modified without breaking compatibility.
daemon_add_buildinfo("char*" "DAEMON_ARCH_STRING" "\"${ARCH}\"")

# Modifying NACL_ARCH breaks engine compatibility with nexe game binaries
# since NACL_ARCH contributes to the nexe file name.
set(NACL_ARCH "${ARCH}")
if (LINUX OR FREEBSD)
	set(ARMHF_USAGE arm64 armel)
	if (ARCH IN_LIST ARMHF_USAGE)
		# Load 32-bit armhf nexe on 64-bit arm64 engine on Linux with multiarch.
		# The nexe is system agnostic so there should be no difference with armel.
		set(NACL_ARCH "armhf")
	endif()
elseif(APPLE)
	if ("${ARCH}" STREQUAL arm64)
		# You can get emulated NaCl going like this:
		# cp external_deps/macos-amd64-default_10/{nacl_loader,irt_core-amd64.nexe} build/
		set(NACL_ARCH "amd64")
	endif()
endif()

daemon_add_buildinfo("char*" "DAEMON_NACL_ARCH_STRING" "\"${NACL_ARCH}\"")

option(USE_ARCH_INTRINSICS "Enable custom code using intrinsics functions or asm declarations" ON)
mark_as_advanced(USE_ARCH_INTRINSICS)

macro(set_arch_intrinsics name)
	if (USE_ARCH_INTRINSICS)
		message(STATUS "Enabling ${name} architecture intrinsics")
		add_definitions(-DDAEMON_USE_ARCH_INTRINSICS_${name}=1)
	else()
		message(STATUS "Disabling ${name} architecture intrinsics")
	endif()
endmacro()

if (USE_ARCH_INTRINSICS)
    add_definitions(-DDAEMON_USE_ARCH_INTRINSICS=1)
endif()

set_arch_intrinsics(${ARCH})

set(amd64_PARENT "i686")
set(arm64_PARENT "armhf")

if (${ARCH}_PARENT)
	set_arch_intrinsics(${${ARCH}_PARENT})
endif()
