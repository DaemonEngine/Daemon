# Daemon BSD Source Code
# Copyright (c) 2013-2016, Daemon Developers
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

set(PLATFORM_PREFIX ${DEPS_DIR}/pnacl/bin)
set(PLATFORM_TRIPLET "pnacl")
if (WIN32)
    set(PNACL_BIN_EXT ".bat")
else()
    set(PNACL_BIN_EXT "")
endif()
set(PLATFORM_EXE_SUFFIX ".pexe")

set(CMAKE_SYSTEM_NAME "Generic")

set(CMAKE_C_COMPILER      "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-clang${PNACL_BIN_EXT}")
set(CMAKE_CXX_COMPILER    "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-clang++${PNACL_BIN_EXT}")
set(CMAKE_AR              "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-ar${PNACL_BIN_EXT}" CACHE FILEPATH "Archiver" FORCE)
set(CMAKE_RANLIB          "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-ranlib${PNACL_BIN_EXT}")
set(CMAKE_FIND_ROOT_PATH  "${PLATFORM_PREFIX}/../le32-nacl")

set(CMAKE_C_USE_RESPONSE_FILE_FOR_LIBRARIES 1)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_LIBRARIES 1)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES 1)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 1)
set(CMAKE_C_RESPONSE_FILE_LINK_FLAG "@")
set(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")

if (NOT CMAKE_HOST_WIN32)
    find_package(Python2)
    if (NOT Python2_Interpreter_FOUND)
        message(FATAL_ERROR "Please install python2 (and run in a python2 virtualenv if that's not enough).")
    endif()
    set(PNACLPYTHON_PREFIX "env PNACLPYTHON=${Python2_EXECUTABLE} ")
    set(PNACLPYTHON_PREFIX2 env "PNACLPYTHON=${Python2_EXECUTABLE} ")
endif()

# These commands can fail on windows if there is a space at the beginning
set(CMAKE_C_CREATE_STATIC_LIBRARY "${PNACLPYTHON_PREFIX}<CMAKE_AR> rc <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_CREATE_STATIC_LIBRARY "${PNACLPYTHON_PREFIX}<CMAKE_AR> rc <TARGET> <LINK_FLAGS> <OBJECTS>")

set(CMAKE_C_COMPILE_OBJECT "${PNACLPYTHON_PREFIX}<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
set(CMAKE_CXX_COMPILE_OBJECT "${PNACLPYTHON_PREFIX}<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")

set(CMAKE_C_LINK_EXECUTABLE "${PNACLPYTHON_PREFIX}<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "${PNACLPYTHON_PREFIX}<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)

set(NACL ON)

set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")

function(pnacl_finalize target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMENT "Finalising ${target}"
        COMMAND
            ${PNACLPYTHON_PREFIX2}
            "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-finalize${PNACL_BIN_EXT}"
            "$<TARGET_FILE:${target}>"
    )
endfunction()

set(NACL_TRANSLATE_OPTIONS
    --allow-llvm-bitcode-input # FIXME: finalize as part of the build process
    --pnacl-allow-exceptions
    $<$<CONFIG:None>:-O3>
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:RelWithDebInfo>:-O2>
    $<$<CONFIG:MinSizeRel>:-O2>
)

function(pnacl_translate dir module arch suffix)
    set(PEXE ${dir}/${module}.pexe)
    set(NEXE ${dir}/${module}${suffix}.nexe)
    set(STRIPPED_NEXE ${dir}/${module}${suffix}-stripped.nexe)

    add_custom_command(
        OUTPUT ${NEXE}
        COMMENT "Translating ${target} (${arch})"
        DEPENDS ${PEXE}
        COMMAND
            ${PNACLPYTHON_PREFIX2}
            "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-translate${PNACL_BIN_EXT}"
            ${NACL_TRANSLATE_OPTIONS}
            -arch ${arch}
            ${PEXE}
            -o ${NEXE}
    )
    add_custom_command(
        OUTPUT ${STRIPPED_NEXE}
        COMMENT "Stripping ${target} (${arch})"
        DEPENDS ${NEXE}
        COMMAND
            ${PNACLPYTHON_PREFIX2}
            "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-strip${PNACL_BIN_EXT}"
            -s
            ${NEXE}
            -o ${STRIPPED_NEXE}
    )

    add_custom_target(${module}${suffix} ALL DEPENDS ${STRIPPED_NEXE})
    add_dependencies(${module}${suffix} ${module}-nacl)
endfunction()
