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

function(yokai_detect_host_arch)
	set(arch_name "unknown")

	string(TOLOWER "${CMAKE_HOST_SYSTEM_PROCESSOR}" processor_lower)

	foreach(name
		"amd64"
		"arm"
		"arm64"
		"i686"
		"mipsel"
		"ppc64"
		"riscv64"
	)
		if ("${processor_lower}" STREQUAL "${name}")
			set(arch_name "${processor_lower}")
		endif()
	endforeach()

	if ("${arch_name}" STREQUAL "unknown")
		set(ARCHES_amd64 "em64t" "x86_64")
		set(ARCHES_arm "armv6l" "armv7l")
		set(ARCHES_arm64 "armv8l" "aarch64")
		set(ARCHES_loong64 "loongarch64")
		set(ARCHES_ppc64el "ppc64le")

		foreach(name
			"amd64"
			"arm"
			"arm64"
			"loong64"
			"ppc64el"
		)
			if ("${processor_lower}" IN_LIST ARCHES_${name})
				set(arch_name "${name}")
			endif()
		endforeach()
	endif()

	if ("${arch_name}" STREQUAL "unknown")
		if ("${processor_lower}")
			message(WARNING "Undocumented host architecture: ${processor_lower}")
		else()
			message(WARNING "Undocumented host architecture")
		endif()

		set(arch_name "${processor_lower}")
	endif()

	string(TOUPPER "${arch_name}" arch_name_upper)

	set(YOKAI_HOST_ARCH_NAME "${arch_name}" PARENT_SCOPE)
	set(YOKAI_HOST_ARCH_NAME_UPPER "${arch_name_upper}" PARENT_SCOPE)

	# Makes possible to do that in CMake code:
	# > if (YOKAI_HOST_ARCH_AMD64)
	set("YOKAI_HOST_ARCH_${arch_name_upper}" ON PARENT_SCOPE)

	if ("${arch_name}" STREQUAL "unknown")
		message(WARNING "Unknown host architecture: ${processor_lower}")

		set(arch_name "${processor_lower}")
	endif()

	set(arm_CHILD
		"armel"
		"armhf"
	)

	foreach(name
		"arm"
	)
		if ("${arch_name}" STREQUAL "${name}")
			message(STATUS "Parent architecture: ${name}, can be ${${name}_CHILD}")

			foreach(child_name ${${name}_CHILD})
				string(TOUPPER "${child_name}" child_name_upper)
				set(YOKAI_HOST_ARCH_${child_name_upper}_PARENT ON PARENT_SCOPE)
			endforeach()
		endif()
	endforeach()
endfunction()

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

yokai_detect_host_arch()
yokai_detect_target_arch()

if (YOKAI_HOST_ARCH_UNKNOWN AND NOT YOKAI_TARGET_ARCH_UNKNOWN)
	message(WARNING "Assuming the host architecture is the same as the target: ${YOKAI_TARGET_ARCH_NAME}")
	set(YOKAI_HOST_ARCH_NAME "${YOKAI_TARGET_ARCH_NAME}")
	set(YOKAI_HOST_ARCH_NAME_UPPER "${YOKAI_TARGET_ARCH_NAME_UPPER}")
	set(YOKAI_HOST_ARCH_${YOKAI_HOST_ARCH_NAME_UPPER} ON)
	unset(YOKAI_HOST_ARCH_UNKNOWN)
endif()

if (YOKAI_TARGET_ARCH_UNKNOWN AND NOT YOKAI_HOST_ARCH_UNKNOWN)
	message(WARNING "Assuming the target architecture is the same as the host: ${YOKAI_TARGET_ARCH_NAME}")
	set(YOKAI_TARGET_ARCH_NAME "${YOKAI_HOST_ARCH_NAME}")
	set(YOKAI_TARGET_ARCH_NAME_UPPER "${YOKAI_HOST_ARCH_NAME_UPPER}")
	set(YOKAI_TARGET_ARCH_${YOKAI_TARGET_ARCH_NAME_UPPER} ON)
	unset(YOKAI_TARGET_ARCH_UNKNOWN)
endif()

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

if (NOT "${YOKAI_HOST_ARCH_NAME}" STREQUAL "${YOKAI_TARGET_ARCH_NAME}")
	if ("${YOKAI_HOST_ARCH_${YOKAI_TARGET_ARCH_NAME}_PARENT}")
		message(STATUS "Assuming no architecture cross-compilation")
	else()
		message(STATUS "Detected architecture cross-compilation")
		set(YOKAI_ARCH_CROSS ON)
	endif()
else()
	message(STATUS "No architecture cross-compilation detected")
endif()

if (YOKAI_SOURCE_GENERATOR)
	# Add printable strings to the executable.
	yokai_add_buildinfo("char*" "YOKAI_ARCH_STRING" "\"${YOKAI_TARGET_ARCH_NAME}\"")
endif()

yokai_set_intrinsics()
