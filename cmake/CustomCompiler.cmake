# Daemon BSD Source Code
# Copyright (c) 2023, Daemon Developers
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

function(detect_custom_clang_version lang file)
	# If CMake already detected the compiler as Clang but we know
	# it's something else.
	if(CUSTOM_${lang}_COMPILER_ID AND CMAKE_${lang}_COMPILER_ID STREQUAL "Clang")
		set(CUSTOM_${lang}_CLANG_VERSION "${CMAKE_${lang}_COMPILER_VERSION}")
		set(CUSTOM_${lang}_CLANG_VERSION "${CUSTOM_${lang}_CLANG_VERSION}" PARENT_SCOPE)
		return()
	endif()

	# Parse “<compiler> -E -dM <source file>”
	execute_process(COMMAND "${CMAKE_${lang}_COMPILER}" ${CUSTOM_${lang}_COMPILER_SUBCOMMAND} -E -dM "${file}"
		OUTPUT_VARIABLE CUSTOM_${lang}_CLANG_OUTPUT
		RESULT_VARIABLE CUSTOM_${lang}_RETURN_CODE
		ERROR_QUIET)

	if (NOT CUSTOM_${lang}_RETURN_CODE) # Success
		if ("${CUSTOM_${lang}_CLANG_OUTPUT}" MATCHES "\#define __clang_version__ ")
			string(REGEX REPLACE ".*#define __clang_version__ \"([^ ]+)[\" ]*.*" "\\1" CUSTOM_${lang}_CLANG_VERSION "${CUSTOM_${lang}_CLANG_OUTPUT}")
			set(CUSTOM_${lang}_CLANG_VERSION "${CUSTOM_${lang}_CLANG_VERSION}" PARENT_SCOPE)
		endif()
	endif()
endfunction()

function(detect_custom_gcc_version lang file)
	# If CMake already detected the compiler as GCC but we know
	# it's something else.
	if(CUSTOM_${lang}_COMPILER_ID AND CMAKE_${lang}_COMPILER_ID STREQUAL "GNU")
		set(CUSTOM_${lang}_GCC_VERSION "${CMAKE_${lang}_COMPILER_VERSION}")
		set(CUSTOM_${lang}_GCC_VERSION "${CUSTOM_${lang}_GCC_VERSION}" PARENT_SCOPE)
		return()
	endif()

	# Almost all compilers on Earth define __GNUC__, __GNUC_MINOR__, and __GNUC_PATCHLEVEL__,
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
			# Parse “<compiler> -E -dM <source file>”
			# No subcommand implemented for now, there may be no usage.
			execute_process(COMMAND "${CMAKE_${lang}_COMPILER}" -E -dM "${file}"
			OUTPUT_VARIABLE CUSTOM_${lang}_GCC_OUTPUT
			RESULT_VARIABLE CUSTOM_${lang}_RETURN_CODE
			ERROR_QUIET)

			if (NOT CUSTOM_${lang}_RETURN_CODE) # Success
				string(REGEX REPLACE ".*#define __GNUC__ ([^ \n]+).*" "\\1" CUSTOM_${lang}_GCC_MAJOR "${CUSTOM_${lang}_GCC_OUTPUT}")
				string(REGEX REPLACE ".*#define __GNUC_MINOR__ ([^ \n]+).*" "\\1" CUSTOM_${lang}_GCC_MINOR "${CUSTOM_${lang}_GCC_OUTPUT}")
				string(REGEX REPLACE ".*#define __GNUC_PATCHLEVEL__ ([^ \n]+).*" "\\1" CUSTOM_${lang}_GCC_PATCHLEVEL "${CUSTOM_${lang}_GCC_OUTPUT}")

				set(CUSTOM_${lang}_GCC_VERSION "${CUSTOM_${lang}_GCC_MAJOR}.${CUSTOM_${lang}_GCC_MINOR}.${CUSTOM_${lang}_GCC_PATCHLEVEL}")
				set(CUSTOM_${lang}_GCC_VERSION "${CUSTOM_${lang}_GCC_VERSION}" PARENT_SCOPE)
				return()
			endif()
		endif()
	endif()
endfunction()

function(detect_custom_compiler_id_version lang file)
	# Parse “<compiler> -E -dM <source file>”
	execute_process(COMMAND "${CMAKE_${lang}_COMPILER}" -E -dM "${file}"
		OUTPUT_VARIABLE CUSTOM_${lang}_COMPILER_OUTPUT
		RESULT_VARIABLE CUSTOM_${lang}_RETURN_CODE
		ERROR_QUIET)

	if (NOT CUSTOM_${lang}_RETURN_CODE) # Success
		# PNaCL
		if ("${CUSTOM_${lang}_COMPILER_OUTPUT}" MATCHES "\#define __pnacl__ 1")
			set(CUSTOM_${lang}_COMPILER_ID "PNaCl" PARENT_SCOPE)

			string(REGEX REPLACE ".*#define __VERSION__ \"([^ ]+).*" "\\1" CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_OUTPUT}")
			set(CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_VERSION}" PARENT_SCOPE)

			return()
		endif()

		# Saigo
		if ("${CUSTOM_${lang}_COMPILER_OUTPUT}" MATCHES "\#define __saigo__ 1")
			set(CUSTOM_${lang}_COMPILER_ID "Saigo" PARENT_SCOPE)

			string(REGEX REPLACE ".*#define __VERSION__ \"Clang ([^ \"]+).*" "\\1" CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_OUTPUT}")
			set(CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_VERSION}" PARENT_SCOPE)

			return()
		endif()

		# AOCC
		if ("${CUSTOM_${lang}_COMPILER_OUTPUT}" MATCHES "CLANG: AOCC")
			set(CUSTOM_${lang}_COMPILER_ID "AOCC" PARENT_SCOPE)

			string(REGEX REPLACE ".*CLANG: AOCC_([^ \"]+).*" "\\1" CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_OUTPUT}")
			set(CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_VERSION}" PARENT_SCOPE)

			return()
		endif()

		# MinGW64
		# This must be tested before MinGW32 since MinGW64 also defines "__MINGW32__".
		if ("${CUSTOM_${lang}_COMPILER_OUTPUT}" MATCHES "\#define __MINGW64__ 1")
			set(CUSTOM_${lang}_COMPILER_ID "MinGW64" PARENT_SCOPE)

			string(REGEX REPLACE ".*#define __VERSION__ \"([^ \"]+).*" "\\1" CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_OUTPUT}")
			set(CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_VERSION}" PARENT_SCOPE)

			return()
		endif()

		# MinGW32
		if ("${CUSTOM_${lang}_COMPILER_OUTPUT}" MATCHES "\#define __MINGW32__ 1")
			set(CUSTOM_${lang}_COMPILER_ID "MinGW32" PARENT_SCOPE)

			string(REGEX REPLACE ".*#define __VERSION__ \"([^ \"]+).*" "\\1" CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_OUTPUT}")
			set(CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_VERSION}" PARENT_SCOPE)

			return()
		endif()
	endif()

	# Parse “<compiler> --help”
	# There is a bug in cmake: if we set CMAKE_C_COMPILER to "zig:cc",
	# only "zig" is returned when using the ${CMAKE_C_COMPILER} variable,
	# so here the command will be "zig --help".
	execute_process(COMMAND "${CMAKE_${lang}_COMPILER}" --help
		OUTPUT_VARIABLE CUSTOM_${lang}_COMPILER_OUTPUT
		RESULT_VARIABLE CUSTOM_${lang}_RETURN_CODE
		ERROR_QUIET)

	if (NOT CUSTOM_${lang}_RETURN_CODE) # Success
		# Zig
		if ("${CUSTOM_${lang}_COMPILER_OUTPUT}" MATCHES ": zig ")
			set(CUSTOM_${lang}_COMPILER_ID "Zig" PARENT_SCOPE)

			execute_process(COMMAND "${CMAKE_${lang}_COMPILER}" version OUTPUT_VARIABLE CUSTOM_${lang}_COMPILER_VERSION)
			string(STRIP "${CUSTOM_${lang}_COMPILER_VERSION}" CUSTOM_${lang}_COMPILER_VERSION)
			set(CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_VERSION}" PARENT_SCOPE)

			# There is a bug in CMake: if we set CMAKE_C_COMPILER to "zig;cc",
			# only "zig" is returned when using the ${CMAKE_C_COMPILER} variable.
			set(C_SUBCOMMAND "cc")
			set(CXX_SUBCOMMAND "c++")

			set(CUSTOM_${lang}_COMPILER_SUBCOMMAND "${${lang}_SUBCOMMAND}")
			set(CUSTOM_${lang}_COMPILER_SUBCOMMAND "${CUSTOM_${lang}_COMPILER_SUBCOMMAND}" PARENT_SCOPE)

			return()
		endif()
	endif()
endfunction()

function(detect_custom_compiler lang)
	set(C_EXT ".c")
	set(CXX_EXT ".cpp")

	string(RANDOM RANDOM_SUFFIX)
	set(COMPILER_TEST_FILE "${PROJECT_BINARY_DIR}/test_compiler_${RANDOM_SUFFIX}${${lang}_EXT}")
	file(TOUCH "${COMPILER_TEST_FILE}")

	detect_custom_compiler_id_version("${lang}" "${COMPILER_TEST_FILE}")
	detect_custom_clang_version("${lang}" "${COMPILER_TEST_FILE}")
	detect_custom_gcc_version("${lang}" "${COMPILER_TEST_FILE}")

	file(REMOVE "${COMPILER_TEST_FILE}")

	set(CUSTOM_${lang}_COMPILER_ID "${CUSTOM_${lang}_COMPILER_ID}" PARENT_SCOPE)
	set(CUSTOM_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_VERSION}" PARENT_SCOPE)
	set(CUSTOM_${lang}_COMPILER_SUBCOMMAND "${CUSTOM_${lang}_COMPILER_SUBCOMMAND}" PARENT_SCOPE)

	set(CUSTOM_${lang}_CLANG_VERSION "${CUSTOM_${lang}_CLANG_VERSION}" PARENT_SCOPE)
	set(CUSTOM_${lang}_GCC_VERSION "${CUSTOM_${lang}_GCC_VERSION}" PARENT_SCOPE)
endfunction()
