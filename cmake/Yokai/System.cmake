# Daemon BSD Source Code
# Copyright (c) 2025, Daemon Developers
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
# System detection.
################################################################################

# When adding a new system, look at all the places YOKAI_HOST_SYSTEM
# and YOKAI_SYSTEM are used.

function(yokai_detect_host_system)
	set(system_name "Unknown")

	foreach(name Linux;FreeBSD;Android;Windows)
		if (CMAKE_HOST_SYSTEM_NAME MATCHES "${name}")
			set(system_name "${CMAKE_SYSTEM_NAME}")
		endif()
	endforeach()

	if (system_name STREQUAL "Unknown")
		message(WARNING "Host system detection failed, may misdetect the target as the host.")

		if (WIN32)
			set(system_name "Windows")
		elseif (APPLE)
			set(system_name "macOS")
		endif()
	endif()

	if (system_name STREQUAL "Unknown")
		set(SYSTEM_Darwin "macOS")
		set(SYSTEM_MSYS "Windows")

		foreach(name Darwin;MSYS)
			if ("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "${name}")
				set(system_name "${SYSTEM_${name}}")
			endif()
		endforeach()

		detect_cmake_host_system("system_name")
	endif()

	set(YOKAI_HOST_SYSTEM_NAME "${system_name}" PARENT_SCOPE)
endfunction()

function(yokai_detect_system)
	yokai_run_detection("" "SYSTEM" "System.c" "Linux")

	if (system_name STREQUAL "Unknown")
		detect_cmake_host_system("system_name")
	endif()

	set(YOKAI_SYSTEM_NAME "${system_name}" PARENT_SCOPE)
endfunction()

yokai_detect_host_system()
yokai_detect_system()

if ("${YOKAI_HOST_SYSTEM_NAME}" STREQUAL "Unknown")
	message(WARNING "Unknown host system")
endif()

if ("${YOKAI_SYSTEM_NAME}" STREQUAL "Unknown")
	message(WARNING "Unknown target system")
endif()

if ("${YOKAI_HOST_SYSTEM_NAME}" STREQUAL "Unknown" AND NOT "${YOKAI_SYSTEM_NAME}" STREQUAL "Unknown")
	message(WARNING "Assuming the host system is the same as the target: ${YOKAI_SYSTEM_NAME}")
	set(YOKAI_HOST_SYSTEM_NAME "${YOKAI_SYSTEM_NAME}")
endif()

if ("${YOKAI_SYSTEM_NAME}" STREQUAL "Unknown" AND NOT "${YOKAI_HOST_SYSTEM_NAME}" STREQUAL "Unknown")
	message(WARNING "Assuming the target system is the same as the host: ${YOKAI_SYSTEM_NAME}")
	set(YOKAI_SYSTEM_NAME "${YOKAI_HOST_SYSTEM_NAME}")
endif()

message(STATUS "Detected host system: ${YOKAI_HOST_SYSTEM_NAME}")
message(STATUS "Detected target system: ${YOKAI_SYSTEM_NAME}")

if (NOT "${YOKAI_HOST_SYSTEM_NAME}" STREQUAL "${YOKAI_SYSTEM_NAME}")
	message(STATUS "Detected cross-compilation")
endif()

# Makes possible to do that in CMake code:
# > if (YOKAI_HOST_SYSTEM_Linux)
# > if (YOKAI_SYSTEM_Windows)
set("YOKAI_HOST_SYSTEM_${YOKAI_HOST_SYSTEM_NAME}" ON)
set("YOKAI_SYSTEM_${YOKAI_SYSTEM_NAME}" ON)

# This is for systems behaving similarly to a Linux Desktop,
# implementing standards like FHS, XDG, GLVND…
# It makes possible to do that in CMake code:
# > if (${YOKAI_CMAKE_SLUG}_HOST_SYSTEM_XDG_COMPATIBILITY)
# > if (${YOKAI_CMAKE_SLUG}_SYSTEM_XDG_COMPATIBILITY)
foreach(name Linux;FreeBSD)
	foreach(slug HOST_SYSTEM;SYSTEM)
		if (YOKAI_${slug}_${name})
			set(YOKAI_${slug}_XDG_COMPATIBILITY ON)
		endif()
	endforeach()
endforeach()

if (YOKAI_SOURCE_GENERATOR)
	# Add printable string to the executable.
	yokai_add_buildinfo("char*" "YOKAI_SYSTEM_STRING" "\"${YOKAI_SYSTEM_NAME}\"")
endif()
