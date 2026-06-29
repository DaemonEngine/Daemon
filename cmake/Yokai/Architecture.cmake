# Daemon BSD Source Code
# Copyright (c) 2022-2026, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the Daemon developers nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

################################################################################
# Architecture detection.
################################################################################

# When adding a new architecture, look at all the places YOKAI_HOST_ARCH
# and YOKAI_TARGET_ARCH are used.

option(USE_ARCH_INTRINSICS "Enable custom code using intrinsics functions or asm declarations" ON)
mark_as_advanced(USE_ARCH_INTRINSICS)

function(yokai_detect_target_arch)
	yokai_run_detection("TARGET" "ARCH" "Architecture.c" "")

	set(YOKAI_TARGET_ARCH_NAME "${arch_name}" PARENT_SCOPE)
	set(YOKAI_TARGET_ARCH_NAME_UPPER "${arch_name_upper}" PARENT_SCOPE)

	add_definitions(-DYOKAI_ARCH_${arch_name_upper})

	# Makes possible to do that in CMake code:
	# > if (YOKAI_TARGET_ARCH_AMD64)
	set("YOKAI_TARGET_ARCH_${arch_name_upper}" ON PARENT_SCOPE)
endfunction()

function(yokai_set_arch_intrinsics name)
	message(STATUS "Enabling ${name} architecture intrinsics")
	string(TOUPPER "${name}" name_upper)
	add_definitions(-DYOKAI_USE_ARCH_INTRINSICS_${name_upper})
endfunction()

function(yokai_set_intrinsics)
	if (USE_ARCH_INTRINSICS)
		# Makes possible to do that in C++ code:
		# > if defined(YOKAI_USE_ARCH_INTRINSICS)
		add_definitions(-DYOKAI_USE_ARCH_INTRINSICS)

		# Makes possible to do that in C++ code:
		# > if defined(YOKAI_USE_ARCH_INTRINSICS_AMD64)
		yokai_set_arch_intrinsics("${YOKAI_TARGET_ARCH_NAME}")

		set(amd64_PARENT "i686")
		set(arm64_PARENT "armhf")
		set(ppc64el_PARENT "ppc64")

		if ("${YOKAI_TARGET_ARCH_NAME}_PARENT")
			yokai_set_arch_intrinsics("${${YOKAI_TARGET_ARCH_NAME}_PARENT}")
		endif()
	else()
		message(STATUS "Disabling ${YOKAI_TARGET_ARCH_NAME} architecture intrinsics")
	endif()
endfunction()

yokai_detect_target_arch()

if (YOKAI_HOST_ARCH_UNKNOWN)
	message(WARNING "Unknown host architecture")
else()
	message(STATUS "Detected host architecture: ${YOKAI_HOST_ARCH_NAME}")
endif()

if (YOKAI_TARGET_ARCH_UNKNOWN)
	message(WARNING "Unknown target architecture")
else()
	message(STATUS "Detected target architecture: ${YOKAI_TARGET_ARCH_NAME}")
endif()

if (YOKAI_SOURCE_GENERATOR)
	# Add printable strings to the executable.
	yokai_add_buildinfo("char*" "YOKAI_ARCH_STRING" "\"${YOKAI_TARGET_ARCH_NAME}\"")
endif()

yokai_set_intrinsics()
