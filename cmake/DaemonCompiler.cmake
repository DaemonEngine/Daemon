# Daemon BSD Source Code
# Copyright (c) 2024, Daemon Developers
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
# Determine compiler
################################################################################

# FIXME: Force -W#pragma-messages and -Wno-error
# In case there is -Wno-#pragma-messages or -Werror in CFLAGS/CXXFLAGS

function(detect_daemon_compiler lang)
	set(C_NAME "C")
	set(CXX_NAME "C++")
	set(C_EXT ".c")
	set(CXX_EXT ".cpp")

	try_compile(BUILD_RESULT
		"${CMAKE_BINARY_DIR}"
		"${DAEMON_DIR}/cmake/DaemonCompiler/DaemonCompiler${${lang}_EXT}"
		CMAKE_FLAGS CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
		OUTPUT_VARIABLE BUILD_LOG
	)

	get_filename_component(compiler_basename "${CMAKE_${lang}_COMPILER}" NAME)

	if (NOT BUILD_RESULT)
		message(WARNING "Failed to build DaemonCompiler${${lang}_EXT}, relying on CMake builtin detection.")
		set(compiler_name "Unknown")
	else()
		set(BUILD_LOG "\n${BUILD_LOG}\n")
		string(REGEX REPLACE "\n[^\n]*<REPORT<" "\n" BUILD_LOG "${BUILD_LOG}")
		string(REGEX REPLACE ">REPORT>[^\n]*\n" "\n" BUILD_LOG "${BUILD_LOG}")

		string(REGEX REPLACE ".*\nDAEMON_COMPILER_NAME=([^\n]*)\n.*" "\\1"
			compiler_name "${BUILD_LOG}")

		foreach(name GCC;Clang;generic;${compiler_name})
			set(compatibility_regex ".*\nDAEMON_COMPILER_${name}_COMPATIBILITY=([^\n]*)\n.*")
			if ("${BUILD_LOG}" MATCHES ${compatibility_regex})
				string(REGEX REPLACE ${compatibility_regex} "\\1"
				compiler_${name}_compatibility "${BUILD_LOG}")
			endif()

			set(version_regex ".*\nDAEMON_COMPILER_${name}_VERSION=([^\n]*)\n.*")
			if ("${BUILD_LOG}" MATCHES ${version_regex})
				string(REGEX REPLACE ${version_regex} "\\1"
				compiler_${name}_version "${BUILD_LOG}")
			endif()

			set(version_string_regex ".*\nDAEMON_COMPILER_${name}_VERSION_STRING=([^\n]*)\n.*")
			if ("${BUILD_LOG}" MATCHES ${version_string_regex})
				string(REGEX REPLACE ${version_string_regex} "\\1"
					compiler_${name}_version_string "${BUILD_LOG}")
			endif()

			set(DAEMON_${lang}_COMPILER_${name}_VERSION
				"${compiler_${name}_version}"
				PARENT_SCOPE)

			set(DAEMON_${lang}_COMPILER_${name}_COMPATIBILITY
				"${compiler_${name}_compatibility}"
				PARENT_SCOPE)
		endforeach()
	endif()

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

	# Compilers that use underlying Clang version as their own version.
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
		detect_daemon_compiler(${lang})

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

	set(compiler_var_name "DAEMON_${lang}_COMPILER_${DAEMON_${lang}_COMPILER_NAME}")
	set(${compiler_var_name} ON)
	add_definitions(-D${compiler_var_name}=1)

	daemon_add_buildinfo("char*" "DAEMON_${lang}_COMPILER_STRING" "\"${DAEMON_${lang}_COMPILER_STRING}\"")
endforeach()
