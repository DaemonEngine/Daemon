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

function(detect_custom_compiler lang)
	set(C_EXT ".c")
	set(CXX_EXT ".cpp")

	string(RANDOM RANDOM_SUFFIX)

	set(COMPILER_TEST_FILE "${PROJECT_BINARY_DIR}/test_compiler_${RANDOM_SUFFIX}${${lang}_EXT}")

	file(TOUCH "${COMPILER_TEST_FILE}")

	# Parse “<compiler> -E -dM <source file>”
	execute_process(COMMAND "${CMAKE_${lang}_COMPILER}" -E -dM "${COMPILER_TEST_FILE}"
		OUTPUT_VARIABLE CUSTOM_${lang}_COMPILER_OUTPUT
		RESULT_VARIABLE CUSTOM_${lang}_RETURN_CODE
		ERROR_QUIET)

	file(REMOVE "${COMPILER_TEST_FILE}")

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
	endif()
endfunction()

detect_custom_compiler("C")
detect_custom_compiler("CXX")
