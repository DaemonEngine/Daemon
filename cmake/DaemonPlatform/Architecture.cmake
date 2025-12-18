# Daemon BSD Source Code
# Copyright (c) 2022-2025, Daemon Developers
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
# Architecture detection.
################################################################################

# When adding a new architecture, look at all the places DAEMON_ARCH is used.

option(USE_ARCH_INTRINSICS "Enable custom code using intrinsics functions or asm declarations" ON)
mark_as_advanced(USE_ARCH_INTRINSICS)

function(daemon_detect_arch)
	daemon_run_detection("" "ARCH" "Architecture.c" "")

	set(DAEMON_ARCH_NAME "${arch_name}" PARENT_SCOPE)

	message(STATUS "Detected target architecture: ${arch_name}")

	add_definitions(-DDAEMON_ARCH_${arch_name})

	set(nacl_arch "${arch_name}")
	if (DAEMON_SYSTEM_Unix_COMPATIBILITY)
		set(armhf_usage "arm64;armel")
		if ("${arch_name}" IN_LIST armhf_usage)
			# Load 32-bit armhf nexe on 64-bit arm64 engine on Linux with multiarch.
			# The nexe is system agnostic so there should be no difference with armel.
			set(nacl_arch "armhf")
		endif()
	elseif(DAEMON_SYSTEM_macOS)
		if ("${arch_name}" STREQUAL "arm64")
			# You can get emulated NaCl going like this:
			# cp external_deps/macos-amd64-default_10/{nacl_loader,irt_core-amd64.nexe} build/
			set(nacl_arch "amd64")
		endif()
	endif()

	# The DAEMON_NACL_ARCH variable contributes to the nexe file name.
	set(DAEMON_NACL_ARCH_NAME "${nacl_arch}" PARENT_SCOPE)
endfunction()

function(daemon_set_arch_intrinsics name)
	message(STATUS "Enabling ${name} architecture intrinsics")
	add_definitions(-DDAEMON_USE_ARCH_INTRINSICS_${name})
endfunction()

function(daemon_set_intrinsics)
	if (USE_ARCH_INTRINSICS)
		# Makes possible to do that in C++ code:
		# > if defined(DAEMON_USE_ARCH_INTRINSICS)
		add_definitions(-DDAEMON_USE_ARCH_INTRINSICS)

		# Makes possible to do that in C++ code:
		# > if defined(DAEMON_USE_ARCH_INTRINSICS_amd64)
		# > if defined(DAEMON_USE_ARCH_INTRINSICS_i686)
		daemon_set_arch_intrinsics("${DAEMON_ARCH_NAME}")

		set(amd64_PARENT "i686")
		set(arm64_PARENT "armhf")

		if ("${DAEMON_ARCH_NAME}_PARENT")
			daemon_set_arch_intrinsics("${${DAEMON_ARCH_NAME}_PARENT}")
		endif()
	else()
		message(STATUS "Disabling ${DAEMON_ARCH_NAME} architecture intrinsics")
	endif()
endfunction()

daemon_detect_arch()
daemon_set_intrinsics()

# Makes possible to do that in CMake code:
# > if (DAEMON_ARCH_arm64)
# > if (DAEMON_NACL_ARCH_armhf)
set("DAEMON_ARCH_${DAEMON_ARCH_NAME}" ON)
set("DAEMON_NACL_ARCH_${DAEMON_NACL_ARCH_NAME}" ON)

if (DAEMON_SOURCE_GENERATOR)
	# Add printable strings to the executable.
	daemon_add_buildinfo("char*" "DAEMON_ARCH_STRING" "\"${DAEMON_ARCH_NAME}\"")
	daemon_add_buildinfo("char*" "DAEMON_NACL_ARCH_STRING" "\"${DAEMON_NACL_ARCH_NAME}\"")
endif()
