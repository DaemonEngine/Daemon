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
		"${DAEMON_DIR}/tools/DaemonCompiler/DaemonCompiler${${lang}_EXT}"
		CMAKE_FLAGS CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
		OUTPUT_VARIABLE BUILD_LOG
	)

	if (NOT BUILD_RESULT)
		message(WARNING "Failed to build DaemonCompiler${${lang}_EXT}, relying on CMake builtin detection.")
		set(compiler_name "Unknown")
	else()
		set(BUILD_LOG "\n${BUILD_LOG}\n")
		string(REGEX REPLACE "\n[^\n]*\\(###REPORT###\\(" "\n" BUILD_LOG "${BUILD_LOG}")
		string(REGEX REPLACE "\\)###REPORT###\\(" "=" BUILD_LOG "${BUILD_LOG}")
		string(REGEX REPLACE "\\)###REPORT###\\)[^\n]*\n" "\n" BUILD_LOG "${BUILD_LOG}")

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
		else()
			message(WARNING "Unknown ${${lang}_NAME} compiler")
		endif()
	endif()

	if (compiler_Clang_version_string)
		set(aocc_version_regex ".*CLANG: AOCC_([^ )]+).*")
		if(compiler_Clang_version_string MATCHES ${aocc_version_regex})
			set(compiler_name "AOCC")
			string(REGEX REPLACE ${aocc_version_regex} "\\1"
				compiler_AOCC_version "${compiler_Clang_version_string}")
			string(REGEX REPLACE "(.*)-Build.*" "\\1"
				compiler_AOCC_version "${compiler_AOCC_version}")
		endif()
	endif()

	set(string_version_regex "([^ ]+).*")
	foreach(name in PNaCl;AppleClang)
		if(compiler_name STREQUAL "${name}")
			if(compiler_generic_version_string)
				if(compiler_generic_version_string MATCHES ${string_version_regex})
					string(REGEX REPLACE ${string_version_regex} "\\1"
						compiler_${name}_version "${compiler_generic_version_string}")
				endif()
			endif()
		endif()
	endforeach()

	if (compiler_ARMClang_version_string)
		# There is no __armclang_patchlevel__ so we should parse __armclang_version__ to get it.
		if(compiler_ARMClang_version_string MATCHES ${string_version_regex})
			string(REGEX REPLACE ${string_version_regex} "\\1"
				compiler_ARMClang_version "${compiler_ARMClang_version_string}")
		endif()
	endif()

	if (compiler_ICX_version)
		# 20240000 becomes 2024.0.0
		string(REGEX REPLACE "(....)(..)(..)" "\\1.\\2.\\3"
			compiler_version "${compiler_ICX_version}")
		string(REGEX REPLACE "\\.0" "."
			compiler_version "${compiler_version}")
	elseif(compiler_${compiler_name}_version)
		set(compiler_version "${compiler_${compiler_name}_version}")
	elseif(CMAKE_${lang}_COMPILER_VERSION)
		set(compiler_version "${CMAKE_${lang}_COMPILER_VERSION}")
	else()
		set(compiler_version "Unknown")
		message(WARNING "Unknown ${${lang}_NAME} compiler version")
	endif()

	get_filename_component(compiler_basename "${CMAKE_${lang}_COMPILER}" NAME)

	if (compiler_Clang_compatibility)
		if(compiler_basename STREQUAL "zig")
			set(compiler_name "Zig")

			# There is a bug in CMake: if we set CMAKE_CXX_COMPILER to "zig;c++",
			# only "zig" is returned when using the ${CMAKE_CXX_COMPILER} variable.
			set(C_SUBCOMMAND "cc")
			set(CXX_SUBCOMMAND "c++")

			set(compiler_subcommand "${${lang}_SUBCOMMAND}")
			set(DAEMON_${lang}_COMPILER_SUBCOMMAND
				"${compiler_subcommand}" PARENT_SCOPE)
		endif()
	endif()

	set(DAEMON_${lang}_COMPILER_BASENAME
		"${compiler_basename}"
		PARENT_SCOPE)

	set(DAEMON_${lang}_COMPILER_NAME
		"${compiler_name}"
		PARENT_SCOPE)

	set(DAEMON_${lang}_COMPILER_VERSION
		"${compiler_version}"
		PARENT_SCOPE)
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
		endif()

		if (DAEMON_${lang}_COMPILER_Clang_COMPATIBILITY)
			if(NOT DAEMON_${lang}_COMPILER_NAME STREQUAL "Clang")
				set(DAEMON_${lang}_COMPILER_EXTENDED_VERSION
					"${DAEMON_${lang}_COMPILER_VERSION}/clang-${DAEMON_${lang}_COMPILER_Clang_VERSION}")
			endif()
		elseif(DAEMON_${lang}_COMPILER_GCC_COMPATIBILITY)
			if(NOT DAEMON_${lang}_COMPILER_NAME STREQUAL "GCC")
				# Almost all compilers on Earth pretend to be GCC compatible.
				# So we first have to check it's really a GCC variant.
				# Parse “<compiler> -v”
				execute_process(COMMAND "${CMAKE_${lang}_COMPILER}" -v
					ERROR_VARIABLE CUSTOM_${lang}_GCC_OUTPUT
					RESULT_VARIABLE CUSTOM_${lang}_RETURN_CODE
					OUTPUT_QUIET)

				if (NOT CUSTOM_${lang}_RETURN_CODE) # Success
					# The existence of this string tells us it's a GCC variant.
					# The version in this string is the same as __VERSION__,
					# the version of the GCC variant, not the version of the upstream
					# GCC we are looking for.
					if ("${CUSTOM_${lang}_GCC_OUTPUT}" MATCHES "\ngcc version ")
						set(DAEMON_${lang}_COMPILER_EXTENDED_VERSION
							"${DAEMON_${lang}_COMPILER_VERSION}/gcc-${DAEMON_${lang}_COMPILER_GCC_VERSION}")
					endif()
				endif()
			endif()
		endif()
	endif()

	if (NOT DAEMON_${lang}_COMPILER_EXTENDED_VERSION)
		set(DAEMON_${lang}_COMPILER_EXTENDED_VERSION "${DAEMON_${lang}_COMPILER_VERSION}")
	endif()

	set(DAEMON_${lang}_COMPILER_STRING
		"${DAEMON_${lang}_COMPILER_NAME} ${DAEMON_${lang}_COMPILER_EXTENDED_VERSION} ${DAEMON_${lang}_COMPILER_BASENAME}")

	message(STATUS "Detected ${${lang}_NAME} compiler: ${DAEMON_${lang}_COMPILER_STRING}")
endforeach()

# We only pass the C++ compiler string to the game for now.
# We may later print in game log all language compiler and
# interpreter versions used to build the game or embedded in the
# game, like CMake, C, C++, Python, Lua, etc.

# Preprocessor definitions containing '#' may not be passed on the compiler
# command line because many compilers do not support it.
string(REGEX REPLACE "\#" "~" DAEMON_DEFINE_CXX_COMPILER_STRING "${DAEMON_CXX_COMPILER_STRING}")

# Quotes cannot be part of the define as support for them is not reliable.
add_definitions(-DDAEMON_CXX_COMPILER_STRING=${DAEMON_DEFINE_CXX_COMPILER_STRING})
