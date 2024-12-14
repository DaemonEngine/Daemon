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

include_directories(${MOUNT_DIR} ${LIB_DIR} ${LIB_DIR}/zlib)
include(DaemonPlatform)
include(DaemonNacl)
include(DaemonFlags)

# Function to setup all the Sgame/Cgame libraries
include(CMakeParseArguments)

function(GAMEMODULE)
    # ParseArguments setup
    set(oneValueArgs NAME)
    set(multiValueArgs DEFINITIONS FLAGS FILES LIBS)
    cmake_parse_arguments(GAMEMODULE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT NACL)
        if (BUILD_GAME_NATIVE_DLL)
            add_library(${GAMEMODULE_NAME}-native-dll MODULE ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST_${GAMEMODULE_NAME}} ${SHAREDLIST} ${COMMONLIST})
            target_link_libraries(${GAMEMODULE_NAME}-native-dll ${GAMEMODULE_LIBS} ${LIBS_BASE})
            set_target_properties(${GAMEMODULE_NAME}-native-dll PROPERTIES
                PREFIX ""
                COMPILE_DEFINITIONS "VM_NAME=${GAMEMODULE_NAME};${GAMEMODULE_DEFINITIONS};BUILD_VM;BUILD_VM_IN_PROCESS"
                COMPILE_OPTIONS "${GAMEMODULE_FLAGS}"
                FOLDER ${GAMEMODULE_NAME}
            )
            ADD_PRECOMPILED_HEADER(${GAMEMODULE_NAME}-native-dll)
        endif()

        if (BUILD_GAME_NATIVE_EXE)
            add_executable(${GAMEMODULE_NAME}-native-exe ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST_${GAMEMODULE_NAME}} ${SHAREDLIST} ${COMMONLIST})
            target_link_libraries(${GAMEMODULE_NAME}-native-exe ${GAMEMODULE_LIBS} ${LIBS_BASE})
            set_target_properties(${GAMEMODULE_NAME}-native-exe PROPERTIES
                COMPILE_DEFINITIONS "VM_NAME=${GAMEMODULE_NAME};${GAMEMODULE_DEFINITIONS};BUILD_VM"
                COMPILE_OPTIONS "${GAMEMODULE_FLAGS}"
                FOLDER ${GAMEMODULE_NAME}
            )
            ADD_PRECOMPILED_HEADER(${GAMEMODULE_NAME}-native-exe)
        endif()

        if (NOT FORK AND BUILD_GAME_NACL)
            if (CMAKE_GENERATOR MATCHES "Visual Studio")
                set(VM_GENERATOR "NMake Makefiles")
            else()
                set(VM_GENERATOR ${CMAKE_GENERATOR})
            endif()

            set(FORK 1 PARENT_SCOPE)
            include(ExternalProject)
            set(inherited_option_args)

            foreach(inherited_option ${NACL_VM_INHERITED_OPTIONS})
                set(inherited_option_args ${inherited_option_args}
                    "-D${inherited_option}=${${inherited_option}}")
            endforeach(inherited_option)

            add_custom_target(nacl-vms ALL)

            if (USE_NACL_SAIGO)
                unset(NACL_VMS_PROJECTS)

                foreach(NACL_TARGET ${NACL_TARGETS})
                    if (NACL_TARGET STREQUAL "i686")
                        set(SAIGO_ARCH "i686")
                    elseif (NACL_TARGET STREQUAL "amd64")
                        set(SAIGO_ARCH "x86_64")
                    elseif (NACL_TARGET STREQUAL "armhf")
                        set(SAIGO_ARCH "arm")
                    else()
                        message(FATAL_ERROR "Unknown NaCl architecture ${NACL_TARGET}")
                    endif()

                    set(NACL_VMS_PROJECT nacl-vms-${NACL_TARGET})
                    list(APPEND NACL_VMS_PROJECTS ${NACL_VMS_PROJECT})
                    add_dependencies(nacl-vms ${NACL_VMS_PROJECT})

                    ExternalProject_Add(${NACL_VMS_PROJECT}
                        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
                        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${NACL_VMS_PROJECT}
                        CMAKE_GENERATOR ${VM_GENERATOR}
                        CMAKE_ARGS
                            -DFORK=2
                            -DCMAKE_TOOLCHAIN_FILE=${Daemon_SOURCE_DIR}/cmake/toolchain-saigo.cmake
                            -DDAEMON_DIR=${Daemon_SOURCE_DIR}
                            -DDEPS_DIR=${DEPS_DIR}
                            -DBUILD_GAME_NACL=ON
                            -DUSE_NACL_SAIGO=ON
                            -DNACL_TARGET=${NACL_TARGET}
                            -DSAIGO_ARCH=${SAIGO_ARCH}
                            -DBUILD_GAME_NATIVE_DLL=OFF
                            -DBUILD_GAME_NATIVE_EXE=OFF
                            -DBUILD_CLIENT=OFF
                            -DBUILD_TTY_CLIENT=OFF
                            -DBUILD_SERVER=OFF
                            ${inherited_option_args}
                        INSTALL_COMMAND ""
                    )

                    # Force the rescan and rebuild of the subproject.
                    ExternalProject_Add_Step(${NACL_VMS_PROJECT} forcebuild
                        COMMAND ${CMAKE_COMMAND} -E remove
                            ${CMAKE_CURRENT_BINARY_DIR}/${NACL_VMS_PROJECT}-prefix/src/${NACL_VMS_PROJECT}-stamp/${NACL_VMS_PROJECT}-configure
                        COMMENT "Forcing build step for '${NACL_VMS_PROJECT}'"
                        DEPENDEES build
                        ALWAYS 1
                    )
                endforeach()
            else()
                set(NACL_VMS_PROJECT nacl-vms-pexe)
                set(NACL_VMS_PROJECTS ${NACL_VMS_PROJECT})
                add_dependencies(nacl-vms ${NACL_VMS_PROJECT})

                # Workaround a bug where CMake ExternalProject lists-as-args are cut on first “;”
                string(REPLACE ";" "," NACL_TARGETS_STRING "${NACL_TARGETS}")

                ExternalProject_Add(${NACL_VMS_PROJECT}
                    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
                    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${NACL_VMS_PROJECT}
                    CMAKE_GENERATOR ${VM_GENERATOR}
                    CMAKE_ARGS
                        -DFORK=2
                        -DCMAKE_TOOLCHAIN_FILE=${Daemon_SOURCE_DIR}/cmake/toolchain-pnacl.cmake
                        -DDAEMON_DIR=${Daemon_SOURCE_DIR}
                        -DDEPS_DIR=${DEPS_DIR}
                        -DBUILD_GAME_NACL=ON
                        -DNACL_TARGETS_STRING=${NACL_TARGETS_STRING}
                        -DBUILD_GAME_NATIVE_DLL=OFF
                        -DBUILD_GAME_NATIVE_EXE=OFF
                        -DBUILD_CLIENT=OFF
                        -DBUILD_TTY_CLIENT=OFF
                        -DBUILD_SERVER=OFF
                        ${inherited_option_args}
                    INSTALL_COMMAND ""
                )

                # Force the rescan and rebuild of the subproject.
                ExternalProject_Add_Step(${NACL_VMS_PROJECT} forcebuild
                    COMMAND ${CMAKE_COMMAND} -E remove
                        ${CMAKE_CURRENT_BINARY_DIR}/${NACL_VMS_PROJECT}-prefix/src/${NACL_VMS_PROJECT}-stamp/${NACL_VMS_PROJECT}-configure
                    COMMENT "Forcing build step for '${NACL_VMS_PROJECT}'"
                    DEPENDEES build
                    ALWAYS 1
                )
            endif()
        endif()
    else()
        if (FORK EQUAL 2)
            if(USE_NACL_SAIGO)
                set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
            else()
                # Put the .nexe and .pexe files in the same directory as the engine
                set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/..)
            endif()
        endif()

        add_executable(${GAMEMODULE_NAME}-nacl ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST_${GAMEMODULE_NAME}} ${SHAREDLIST} ${COMMONLIST})
        target_link_libraries(${GAMEMODULE_NAME}-nacl ${GAMEMODULE_LIBS} ${LIBS_BASE})
        # PLATFORM_EXE_SUFFIX is .pexe when building with PNaCl
        # as translating to .nexe is a separate task.
        set_target_properties(${GAMEMODULE_NAME}-nacl PROPERTIES
            OUTPUT_NAME ${GAMEMODULE_NAME}${PLATFORM_EXE_SUFFIX}
            COMPILE_DEFINITIONS "VM_NAME=${GAMEMODULE_NAME};${GAMEMODULE_DEFINITIONS};BUILD_VM"
            COMPILE_OPTIONS "${GAMEMODULE_FLAGS}"
            FOLDER ${GAMEMODULE_NAME}
        )
        ADD_PRECOMPILED_HEADER(${GAMEMODULE_NAME}-nacl)

        # Revert a workaround for a bug where CMake ExternalProject lists-as-args are cut on first “;”
        string(REPLACE "," ";" NACL_TARGETS "${NACL_TARGETS_STRING}")

        if (USE_NACL_SAIGO)
            # Finalize NaCl executables for supported architectures.
            saigo_finalize(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.. ${GAMEMODULE_NAME} ${NACL_TARGET})
        else()
            # Generate NaCl executables for supported architectures.
            foreach(NACL_TARGET ${NACL_TARGETS})
                pnacl_finalize(${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${GAMEMODULE_NAME} ${NACL_TARGET})
            endforeach()
        endif()
    endif()
endfunction()
