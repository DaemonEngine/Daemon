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

# When adding a new architecture, look at all the places YOKAI_ARCH is used.

option(USE_ARCH_INTRINSICS "Enable custom code using intrinsics functions or asm declarations" ON)
mark_as_advanced(USE_ARCH_INTRINSICS)

function(yokai_detect_arch)
	yokai_run_detection("" "ARCH" "Architecture.c" "")

	set(YOKAI_ARCH_NAME "${arch_name}" PARENT_SCOPE)

	message(STATUS "Detected target architecture: ${arch_name}")

	add_definitions(-DYOKAI_ARCH_${arch_name})

	set(nacl_arch "${arch_name}")

	if (YOKAI_SYSTEM_Linux_COMPATIBILITY OR YOKAI_SYSTEM_XDG_COMPATIBILITY)
		set(armhf_usage "arm64;armel")
		if ("${arch_name}" IN_LIST armhf_usage)
		# Load 32-bit armhf nexe on 64-bit arm64 engine on Linux with multiarch.
		# The nexe is system agnostic so there should be no difference with armel.
		set(nacl_arch "armhf")

		set(box64_usage ppc64el riscv64)
		if ("${arch_name}" IN_LIST box64_usage)
			option(YOKAI_NACL_BOX64_EMULATION "Use Box64 to emulate x86_64 NaCl loader on unsupported platforms" ON)
			if (YOKAI_NACL_BOX64_EMULATION)
				# Use Box64 to run x86_64 NaCl loader and amd64 nexe.
				# Box64 must be installed and available in PATH at runtime.
				set(nacl_arch "amd64")
				add_definitions(-DYOKAI_NACL_BOX64_EMULATION)
			endif()
		endif()
	endif()

	elseif (YOKAI_SYSTEM_macOS)
		if ("${arch_name}" STREQUAL "arm64")
			# You can get emulated NaCl going like this:
			# cp external_deps/macos-amd64-default_10/{nacl_loader,irt_core-amd64.nexe} build/
			set(nacl_arch "amd64")
		endif()
	endif()

	# The YOKAI_NACL_ARCH_NAME variable contributes to the nexe file name.
	set(YOKAI_NACL_ARCH_NAME "${nacl_arch}" PARENT_SCOPE)

	# NaCl runtime is only available on architectures that have a NaCl loader.
	set(nacl_runtime_arch amd64 i686 armhf)
	if ("${nacl_arch}" IN_LIST nacl_runtime_arch)
		add_definitions(-DYOKAI_NACL_RUNTIME_ENABLED)
	endif()
endfunction()

function(yokai_set_arch_intrinsics name)
	message(STATUS "Enabling ${name} architecture intrinsics")
	add_definitions(-DYOKAI_USE_ARCH_INTRINSICS_${name})
endfunction()

option(USE_ARCH_INTRINSICS "Enable custom code using intrinsics functions or asm declarations" ON)
mark_as_advanced(USE_ARCH_INTRINSICS)

function(yokai_set_intrinsics)
	if (USE_ARCH_INTRINSICS)
		# Makes possible to do that in C++ code:
		# > if defined(YOKAI_USE_ARCH_INTRINSICS)
		add_definitions(-DYOKAI_USE_ARCH_INTRINSICS)

		# Makes possible to do that in C++ code:
		# > if defined(YOKAI_USE_ARCH_INTRINSICS_amd64)
		# > if defined(YOKAI_USE_ARCH_INTRINSICS_i686)
		yokai_set_arch_intrinsics("${YOKAI_ARCH_NAME}")

		set(amd64_PARENT "i686")
		set(arm64_PARENT "armhf")
		set(ppc64el_PARENT "ppc64")

		if ("${YOKAI_ARCH_NAME}_PARENT")
			yokai_set_arch_intrinsics("${${YOKAI_ARCH_NAME}_PARENT}")
		endif()
	else()
		message(STATUS "Disabling ${YOKAI_ARCH_NAME} architecture intrinsics")
	endif()
endfunction()

yokai_detect_arch()
yokai_set_intrinsics()

# Makes possible to do that in CMake code:
# > if (YOKAI_ARCH_arm64)
# > if (YOKAI_NACL_ARCH_armhf)
set("YOKAI_ARCH_${YOKAI_ARCH_NAME}" ON)
set("YOKAI_NACL_ARCH_${YOKAI_NACL_ARCH_NAME}" ON)

if (YOKAI_SOURCE_GENERATOR)
	# Add printable strings to the executable.
	yokai_add_buildinfo("char*" "YOKAI_ARCH_STRING" "\"${YOKAI_ARCH_NAME}\"")
	yokai_add_buildinfo("char*" "YOKAI_NACL_ARCH_STRING" "\"${YOKAI_NACL_ARCH_NAME}\"")
endif()
