# Daemon BSD Source Code
# Copyright (c) 2023-2026, Daemon Developers
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

# We can't use CMAKE_EXECUTABLE_SUFFIX because the Generic Platform will reset it
# and creating a Platform module for setting CMAKE_EXECUTABLE_SUFFIX is overkill.
set(NACL_EXECUTABLE_SUFFIX ".nexe")

set(SAIGO_TRIPLET "${SAIGO_ARCH}-nacl")

if (NOT DEFINED PREFIX_SAIGO)
	find_program(PATH_SAIGO_CLANG NAMES "${SAIGO_TRIPLET}-clang")
	get_filename_component(PREFIX_SAIGO ${PATH_SAIGO_CLANG} DIRECTORY)
endif()

list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES PREFIX_SAIGO)

set(CMAKE_SYSTEM_NAME "Generic")

set(CMAKE_C_COMPILER "${PREFIX_SAIGO}/bin/${SAIGO_TRIPLET}-clang")
set(CMAKE_CXX_COMPILER "${PREFIX_SAIGO}/bin/${SAIGO_TRIPLET}-clang++")
set(CMAKE_AR "${PREFIX_SAIGO}/bin/${SAIGO_TRIPLET}-ar" CACHE FILEPATH "Archiver" FORCE)
set(CMAKE_RANLIB "${PREFIX_SAIGO}/bin/${SAIGO_TRIPLET}-ranlib")
set(CMAKE_STRIP "${PREFIX_SAIGO}/bin/${SAIGO_TRIPLET}-strip")
set(CMAKE_FIND_ROOT_PATH "${PREFIX_SAIGO}/${SAIGO_TRIPLET}")

# Copy-pasted from the PNaCl toolchain, it's not sure we need it.
set(CMAKE_C_USE_RESPONSE_FILE_FOR_LIBRARIES 1)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_LIBRARIES 1)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES 1)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 1)
set(CMAKE_C_RESPONSE_FILE_LINK_FLAG "@")
set(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")

# Copy-pasted from the PNaCl toolchain, it's not sure we need it.
# These commands can fail on windows if there is a space at the beginning
set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> rc <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_AR> rc <TARGET> <LINK_FLAGS> <OBJECTS>")

set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")

set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

# Copy-pasted from the PNaCl toolchain, it's not sure we need it.
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)

set(NACL ON)

set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")
