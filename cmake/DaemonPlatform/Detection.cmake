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

# Make sure to always call this macro from within a function, not in the global scope.
# As a macro it produces a lot of variables in the parent scope but it is meant to
# only be called by functions so they should never pollute the globale scope.
# It's a macro because we need to write a lot of variables in the calling function scope
# and we need to write some variables to the parent scope of the calling function.
macro(daemon_run_detection slug_prefix report_slug file_name compat_list)
	string(TOLOWER "${report_slug}" local_slug)

	# Setting -Werror in CXXFLAGS would produce errors instead of warning
	# but that should not break the detection,
	# so we only print a CMake warning there and use build_log content to
	# detect unknown platforms.
	# Catching compilation error is still useful, for example to detect
	# undefined types, missing header or things like that.
	# Setting USE_WERROR to ON doesn't print this warning as the flag
	# is set after the detection.
	try_compile(build_result
		"${CMAKE_BINARY_DIR}"
		"${CMAKE_CURRENT_LIST_DIR}/Detection/${file_name}"
		# TODO: Force -W#pragma-messages and -Wno-error
		# In case there is -Wno-#pragma-messages or -Werror in CFLAGS/CXXFLAGS
		CMAKE_FLAGS CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
		OUTPUT_VARIABLE build_log
	)

	if (NOT build_result)
		message(WARNING "Failed to build ${file_name}.\n"
			"Setting -Werror in CFLAGS can produce false positive errors\n"
			"${build_log}"
		)
		set(${local_slug}_name "Unknown" PARENT_SCOPE)
	else()
		set(build_log "\n${build_log}\n")

		string(REGEX REPLACE "\n[^\n]*<REPORT<" "\n" build_log "${build_log}")
		string(REGEX REPLACE ">REPORT>[^\n]*\n" "\n" build_log "${build_log}")

		string(REGEX REPLACE ".*\nDAEMON_${report_slug}_NAME=([^\n]*)\n.*" "\\1"
			${local_slug}_name "${build_log}")

		foreach(name ${compat_list};${${local_slug}_name})
			set(COMPATIBILITY_REGEX ".*\nDAEMON_${report_slug}_${name}_COMPATIBILITY=([^\n]*)\n.*")
			if ("${build_log}" MATCHES ${COMPATIBILITY_REGEX})
				string(REGEX REPLACE ${COMPATIBILITY_REGEX} "\\1"
				${local_slug}_${name}_compatibility "${build_log}")

				set("DAEMON_${slug_prefix}${report_slug}_${name}_COMPATIBILITY"
					"${${local_slug}_${name}_compatibility}"
					PARENT_SCOPE)
			endif()

			set(VERSION_REGEX ".*\nDAEMON_${report_slug}_${name}_VERSION=([^\n]*)\n.*")
			if ("${build_log}" MATCHES ${VERSION_REGEX})
				string(REGEX REPLACE ${VERSION_REGEX} "\\1"
				${local_slug}_${name}_version "${build_log}")

				set("DAEMON_${slug_prefix}${report_slug}_${name}_VERSION"
					"${${local_slug}_${name}_version}"
					PARENT_SCOPE)
			endif()

			set(VERSION_STRING_REGEX ".*\nDAEMON_${report_slug}_${name}_VERSION_STRING=([^\n]*)\n.*")
			if ("${build_log}" MATCHES ${VERSION_STRING_REGEX})
				string(REGEX REPLACE ${VERSION_STRING_REGEX} "\\1"
					${local_slug}_${name}_version_string "${build_log}")
			endif()
		endforeach()
	endif()
endmacro()

# Target detection.
include("${CMAKE_CURRENT_LIST_DIR}/System.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Architecture.cmake")

# Compiler detection.
include("${CMAKE_CURRENT_LIST_DIR}/Compiler.cmake")
