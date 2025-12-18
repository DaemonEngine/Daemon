# Daemon BSD Source Code
# Copyright (c) 2024-2025, Daemon Developers
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
# Compiler detection.
################################################################################

# When adding a new compiler, look at all the places DAEMON_C_COMPILER
# and DAEMON_CXX_COMPILER are used.

function(daemon_detect_compiler lang)
	set(C_NAME "C")
	set(CXX_NAME "C++")
	set(C_EXT ".c")
	set(CXX_EXT ".cpp")

	get_filename_component(compiler_basename "${CMAKE_${lang}_COMPILER}" NAME)

	daemon_run_detection("${lang}_" "COMPILER" "Compiler${${lang}_EXT}" "GCC;Clang;generic")

	if (compiler_name STREQUAL "Unknown")
		if (CMAKE_${lang}_COMPILER_ID)
			set(compiler_name "${CMAKE_${lang}_COMPILER_ID}")
			# Compiler version is done below.
		else()
			message(WARNING "Unknown ${${lang}_NAME} compiler")
		endif()
	endif()

	# AOCC
	if (compiler_Clang_version_string)
		set(aocc_version_regex ".*CLANG: AOCC_([^ )]+).*")
		if (compiler_Clang_version_string MATCHES ${aocc_version_regex})
			set(compiler_name "AOCC")
			string(REGEX REPLACE ${aocc_version_regex} "\\1"
				compiler_AOCC_version "${compiler_Clang_version_string}")
			string(REGEX REPLACE "(.*)-Build.*" "\\1"
				compiler_AOCC_version "${compiler_AOCC_version}")
		endif()
	endif()

	# Zig
	if (compiler_Clang_version_string)
		set(zig_version_regex ".*[(]https://github.com/ziglang/zig-bootstrap .*[)]")
		if (compiler_Clang_version_string MATCHES ${zig_version_regex})
			set(compiler_name "Zig")
		endif()

		# Parse “zig version”
		execute_process(COMMAND "${CMAKE_${lang}_COMPILER}" version
			OUTPUT_VARIABLE CUSTOM_${lang}_ZIG_OUTPUT
			RESULT_VARIABLE CUSTOM_${lang}_ZIG_RETURN_CODE
			ERROR_QUIET
			OUTPUT_STRIP_TRAILING_WHITESPACE)

		if (NOT CUSTOM_${lang}_ZIG_RETURN_CODE) # Success
			set(compiler_Zig_version "${CUSTOM_${lang}_ZIG_OUTPUT}")
		endif()
	endif()

	# Compilers that use the underlying Clang version as their own version.
	foreach(name in AppleClang)
		if (compiler_name STREQUAL "${name}")
			set(compiler_${name}_version "${compiler_Clang_version}")
		endif()
	endforeach()

	# Compilers that write the version number at the beginning of the VERSION string.
	set(string_version_regex "([^ ]+).*")
	foreach(name in PNaCl)
		if (compiler_name STREQUAL "${name}")
			if (compiler_generic_version_string)
				if (compiler_generic_version_string MATCHES ${string_version_regex})
					string(REGEX REPLACE ${string_version_regex} "\\1"
						compiler_${name}_version "${compiler_generic_version_string}")
				endif()
			endif()
		endif()
	endforeach()

	if (compiler_ARMClang_version_string)
		# There is no __armclang_patchlevel__ so we should parse __armclang_version__ to get it.
		if (compiler_ARMClang_version_string MATCHES ${string_version_regex})
			string(REGEX REPLACE ${string_version_regex} "\\1"
				compiler_ARMClang_version "${compiler_ARMClang_version_string}")
		endif()
	endif()

	if (compiler_ICX_version)
		# 20240000 becomes 2024.0.0
		string(REGEX REPLACE "(....)(..)(..)" "\\1.\\2.\\3"
			compiler_ICX_version "${compiler_ICX_version}")
		string(REGEX REPLACE "\\.0" "."
			compiler_ICX_version "${compiler_ICX_version}")
	endif()

	if (compiler_${compiler_name}_version)
		set(compiler_version "${compiler_${compiler_name}_version}")
	elseif (CMAKE_${lang}_COMPILER_VERSION)
		set(compiler_version "${CMAKE_${lang}_COMPILER_VERSION}")
	else()
		set(compiler_version "Unknown")
		message(WARNING "Unknown ${${lang}_NAME} compiler version")
	endif()

	set(DAEMON_${lang}_COMPILER_BASENAME "${compiler_basename}" PARENT_SCOPE)
	set(DAEMON_${lang}_COMPILER_NAME "${compiler_name}" PARENT_SCOPE)
	set(DAEMON_${lang}_COMPILER_VERSION "${compiler_version}" PARENT_SCOPE)
endfunction()

message(STATUS "CMake generator: ${CMAKE_GENERATOR}")

foreach(lang C;CXX)
	set(C_NAME "C")
	set(CXX_NAME "C++")

	if (MSVC)
		# Let CMake do the job, it does it very well,
		# and there is probably no variant to take care about.
		set(DAEMON_${lang}_COMPILER_NAME "${CMAKE_${lang}_COMPILER_ID}")
		set(DAEMON_${lang}_COMPILER_VERSION "${CMAKE_${lang}_COMPILER_VERSION}")
		get_filename_component(DAEMON_${lang}_COMPILER_BASENAME "${CMAKE_${lang}_COMPILER}" NAME)
	else()
		daemon_detect_compiler(${lang})

		if (DAEMON_${lang}_COMPILER_Clang_COMPATIBILITY)
			if (NOT DAEMON_${lang}_COMPILER_NAME STREQUAL "Clang")
				set(DAEMON_${lang}_COMPILER_EXTENDED_VERSION
					"${DAEMON_${lang}_COMPILER_VERSION}/Clang_${DAEMON_${lang}_COMPILER_Clang_VERSION}")
			endif()
		elseif (DAEMON_${lang}_COMPILER_GCC_COMPATIBILITY)
			if (NOT DAEMON_${lang}_COMPILER_NAME STREQUAL "GCC")
				# Almost all compilers on Earth pretend to be GCC compatible.
				# So we first have to check it's really a GCC variant.
				# Parse “<compiler> -v”
				execute_process(COMMAND "${CMAKE_${lang}_COMPILER}" -v
					ERROR_VARIABLE CUSTOM_${lang}_GCC_OUTPUT
					RESULT_VARIABLE CUSTOM_${lang}_GCC_RETURN_CODE
					OUTPUT_QUIET)

				if (NOT CUSTOM_${lang}_GCC_RETURN_CODE) # Success
					# The existence of this string tells us it's a GCC variant.
					# The version in this string is the same as __VERSION__,
					# the version of the GCC variant, not the version of the upstream
					# GCC we are looking for.
					if ("${CUSTOM_${lang}_GCC_OUTPUT}" MATCHES "\ngcc version ")
						set(DAEMON_${lang}_COMPILER_EXTENDED_VERSION
							"${DAEMON_${lang}_COMPILER_VERSION}/GCC_${DAEMON_${lang}_COMPILER_GCC_VERSION}")
					endif()
				endif()
			endif()
		endif()
	endif()

	if (NOT DAEMON_${lang}_COMPILER_EXTENDED_VERSION)
		set(DAEMON_${lang}_COMPILER_EXTENDED_VERSION "${DAEMON_${lang}_COMPILER_VERSION}")
	endif()

	set(DAEMON_${lang}_COMPILER_STRING
		"${DAEMON_${lang}_COMPILER_NAME}_${DAEMON_${lang}_COMPILER_EXTENDED_VERSION}:${DAEMON_${lang}_COMPILER_BASENAME}")

	if (CMAKE_CXX_COMPILER_ARG1)
		set(DAEMON_${lang}_COMPILER_STRING "${DAEMON_${lang}_COMPILER_STRING}:${CMAKE_CXX_COMPILER_ARG1}")
	endif()

	message(STATUS "Detected ${${lang}_NAME} compiler: ${DAEMON_${lang}_COMPILER_STRING}")

	# Makes possible to do that in C++ code:
	# > if defined(DAEMON_CXX_COMPILER_Clang)
	set(compiler_var_name "DAEMON_${lang}_COMPILER_${DAEMON_${lang}_COMPILER_NAME}")
	add_definitions(-D${compiler_var_name})

	# Makes possible to do that in CMake code:
	# > if (DAEMON_CXX_COMPILER_Clang)
	set("${compiler_var_name}" ON)

	if (DAEMON_SOURCE_GENERATOR)
		# Add printable string to the executable.
		daemon_add_buildinfo("char*" "DAEMON_${lang}_COMPILER_STRING" "\"${DAEMON_${lang}_COMPILER_STRING}\"")
	endif()
endforeach()
