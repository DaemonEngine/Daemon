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

include(CustomCompiler)

function(detect_daemon_compiler lang)
	set(C_NAME "C")
	set(CXX_NAME "C++")

	get_filename_component(DAEMON_${LANG}_COMPILER_BASENAME "${CMAKE_${lang}_COMPILER}" NAME)

	if (CUSTOM_${lang}_COMPILER_ID)
		set(DAEMON_${lang}_COMPILER_ID "${CUSTOM_${lang}_COMPILER_ID}")
	elseif (CMAKE_${lang}_COMPILER_ID)
		set(DAEMON_${lang}_COMPILER_ID "${CMAKE_${lang}_COMPILER_ID}")
	else()
		message(WARNING "Unknown ${${lang}_NAME} compiler")
		set(DAEMON_${lang}_COMPILER_ID "Unknown")
	endif()

	set(DAEMON_${lang}_COMPILER_ID "${DAEMON_${lang}_COMPILER_ID}" PARENT_SCOPE)

	if (CUSTOM_${lang}_COMPILER_VERSION)
		set(DAEMON_${lang}_COMPILER_VERSION "${CUSTOM_${lang}_COMPILER_VERSION}")
	elseif (CMAKE_${lang}_COMPILER_VERSION)
		set(DAEMON_${lang}_COMPILER_VERSION "${CMAKE_${lang}_COMPILER_VERSION}")
	else()
		message(WARNING "Unknown ${${lang}_NAME} compiler version")
		set(DAEMON_${lang}_COMPILER_VERSION "Unknown")
	endif()

	set(DAEMON_${lang}_COMPILER_STRING "${DAEMON_${lang}_COMPILER_ID} ${DAEMON_${lang}_COMPILER_VERSION} ${DAEMON_${lang}_COMPILER_BASENAME}")
	set(DAEMON_${lang}_COMPILER_STRING "${DAEMON_${lang}_COMPILER_STRING}" PARENT_SCOPE)

	message(STATUS "Detected ${${lang}_NAME} compiler: ${DAEMON_${lang}_COMPILER_STRING}")
endfunction()

message(STATUS "CMake generator: ${CMAKE_GENERATOR}")

detect_daemon_compiler("C")
detect_daemon_compiler("CXX")

# We only pass the C++ compiler string to the game for now.
# We may later print in the game log all language compiler and
# interpreter versions used to build the game or embedded in the
# game, like CMake, C, C++, Python, Lua, etc.

# Preprocessor definitions containing '#' may not be passed on the compiler
# command line because many compilers do not support it.
string(REGEX REPLACE "\#" "~" DAEMON_DEFINE_CXX_COMPILER_STRING "${DAEMON_CXX_COMPILER_STRING}")

# Quotes cannot be part of the define as support for them is not reliable.
add_definitions(-DDAEMON_CXX_COMPILER_STRING=${DAEMON_DEFINE_CXX_COMPILER_STRING})
