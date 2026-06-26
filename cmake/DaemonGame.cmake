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

include_directories(${MOUNT_DIR} ${LIB_DIR} ${LIB_DIR}/zlib)

## About the different ways to host/play games:
## Native DLL: no sandboxing, no cleaning up but debugger support. Use for dev.
## NaCl exe: sandboxing, no leaks, slightly slower, hard to debug. Use for regular players.
## Native exe: no sandboxing, no leaks, hard to debug. Might be used by server owners for perf.
## See VirtualMachine.h for code.

# can be loaded by daemon with vm.[sc]game.type 3
option(BUILD_GAME_NATIVE_DLL "Build the shared library files, mostly useful for debugging changes locally." ON)

# can be loaded by daemon with vm.[sc]game.type 2
option(BUILD_GAME_NATIVE_EXE "Build native executable, which might be used for better performances by server owners" OFF)

include(ExternalProject)

include(Yokai/All)

# Do not report unused native compiler if native vms are not built.
# If only NACL vms are built, this will be reported in chainloaded build.
if (BUILD_GAME_NATIVE_DLL OR BUILD_GAME_NATIVE_EXE OR GAME_BUILD_SUBINVOCATION)
    include(DaemonFlags)
    include(DaemonNaclFlags)
endif()

# Source lists for src/shared
set(SHAREDLIST
    ${MOUNT_DIR}/shared/CommandBufferClient.cpp
    ${MOUNT_DIR}/shared/CommandBufferClient.h
    ${MOUNT_DIR}/shared/CommonProxies.cpp
    ${MOUNT_DIR}/shared/CommonProxies.h
    ${MOUNT_DIR}/shared/VMMain.cpp
    ${MOUNT_DIR}/shared/VMMain.h
)
set(SHAREDLIST_cgame
    ${MOUNT_DIR}/shared/client/cg_api.cpp ${MOUNT_DIR}/shared/client/cg_api.h
)
set(SHAREDLIST_sgame
    ${MOUNT_DIR}/shared/server/sg_api.cpp ${MOUNT_DIR}/shared/server/sg_api.h
)

# Function to setup all the Sgame/Cgame libraries
include(CMakeParseArguments)

list(APPEND NACL_ALL_TARGETS "amd64" "i686" "armhf")

if (NOT GAME_BUILD_SUBINVOCATION)
	option(USE_NACL_SAIGO "Use the Saigo toolchain to build NaCl executables" ON)

	if (USE_NACL_SAIGO)
		set(HAS_NACL_SDK ON)
	elseif (CMAKE_SYSTEM_NAME STREQUAL CMAKE_HOST_SYSTEM_NAME
	AND (YOKAI_TARGET_ARCH_AMD64 OR YOKAI_TARGET_ARCH_I686))
		# The PNaCl SDK only runs on amd64 or i686.
		set(HAS_NACL_SDK ON)
	endif()

	if (HAS_NACL_SDK)
		include(DaemonNaclArchitecture)

		# can be loaded by daemon with vm.[sc]game.type 0 or 1
		option(BUILD_GAME_NACL "Build the NaCl \"pexe\" and \"nexe\" gamelogic modules for enabled architecture targets, required to host mods." OFF)

		set(BUILD_GAME_NACL_TARGETS "all" CACHE STRING "Enabled NaCl \"nexe\" architecture targets, values: ${NACL_ALL_TARGETS};all;native;none")
		mark_as_advanced(BUILD_GAME_NACL_TARGETS)

		if (BUILD_GAME_NACL)
			foreach(nacl_target ${BUILD_GAME_NACL_TARGETS})
				if (BUILD_GAME_NACL_TARGETS STREQUAL "all")
					list(APPEND nacl_target_list ${NACL_ALL_TARGETS})
				elseif (BUILD_GAME_NACL_TARGETS STREQUAL "native")
					list(APPEND nacl_target_list "${YOKAI_TARGET_ARCH_NAME}")
				elseif (BUILD_GAME_NACL_TARGETS STREQUAL "none")
					set(nacl_target_list "")
				else()
					list(APPEND nacl_target_list "${nacl_target}")
				endif()
			endforeach()

			foreach(nacl_target IN LISTS nacl_target_list)
				daemon_detect_nacl_arch("${nacl_target}")

				foreach(detected_target IN LISTS ${DAEMON_NACL_ARCH_NAME_LIST})
					if (NOT "${detected_target}" IN_LIST NACL_TARGETS)
						list(APPEND NACL_TARGETS ${detected_target})
					endif()
				endforeach()
			endforeach()

			foreach(nacl_target IN LISTS NACL_TARGETS)
				if (NOT nacl_target IN_LIST NACL_ALL_TARGETS)
						message(FATAL_ERROR "Invalid NaCl target ${nacl_target}, must be one of ${NACL_ALL_TARGETS}")
				endif()
			endforeach()

			message(STATUS "Building NaCl targets: ${NACL_TARGETS}")
		endif()
	else()
		set(BUILD_GAME_NACL OFF)
		set(NACL_TARGETS "")
	endif()
endif()

yokai_write_buildinfo("DaemonGame")

function(buildGameModule module_slug)
	set(module_target "${GAMEMODULE_NAME}-${module_slug}")

	set(module_target_args ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST_${GAMEMODULE_NAME}} ${SHAREDLIST} ${BUILDINFOLIST} ${COMMONLIST})

	if (module_slug STREQUAL "native-dll")
		add_library("${module_target}" MODULE ${module_target_args})
		set_target_properties(${module_target} PROPERTIES PREFIX "")
		set(GAMEMODULE_DEFINITIONS "${GAMEMODULE_DEFINITIONS};BUILD_VM_IN_PROCESS")
	else()
		if (module_slug STREQUAL "native-exe")
			set(GAMEMODULE_DEFINITIONS "${GAMEMODULE_DEFINITIONS};BUILD_VM_NATIVE_EXE")
		endif()
		add_executable("${module_target}" ${module_target_args})
	endif()

	set_target_properties(${module_target} PROPERTIES
		COMPILE_DEFINITIONS "VM_NAME=${GAMEMODULE_NAME};${GAMEMODULE_DEFINITIONS};BUILD_VM"
		COMPILE_OPTIONS "${GAMEMODULE_FLAGS}"
		FOLDER ${GAMEMODULE_NAME}
	)

	if (module_slug STREQUAL "nacl")
		set_target_properties(${module_target} PROPERTIES
			OUTPUT_NAME "${GAMEMODULE_NAME}"
			SUFFIX "${NACL_EXECUTABLE_SUFFIX}")

		target_link_libraries(${module_target} ${GAMEMODULE_LIBS} ${LIBS_BASE})
	else()
		target_link_libraries(${module_target} ${GAMEMODULE_LIBS} ${LIBS_BASE} ${CPP23SupportLibrary})
	endif()

	ADD_PRECOMPILED_HEADER(${module_target})
endfunction()

function(gameSubProject)
	ExternalProject_Add(${NACL_VMS_PROJECT}
		SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
		BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${NACL_VMS_PROJECT}
		CMAKE_GENERATOR ${VM_GENERATOR}
		CMAKE_ARGS
			"-DDAEMON_DIR=${Daemon_SOURCE_DIR}"
			"-DDEPS_DIR=${DEPS_DIR}"
			-DBUILD_CLIENT=OFF
			-DBUILD_TTY_CLIENT=OFF
			-DBUILD_SERVER=OFF
			"-DBUILD_DUMMY_GAMELOGIC=${BUILD_DUMMY_GAMELOGIC}"
			-DGAME_BUILD_SUBINVOCATION=ON
			${ARGV}
			${INHERITED_OPTION_ARGS}
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
endfunction()

function(GAMEMODULE)
	# ParseArguments setup
	set(oneValueArgs NAME)
	set(multiValueArgs DEFINITIONS FLAGS FILES LIBS)
	cmake_parse_arguments(GAMEMODULE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if (BUILD_GAME_NATIVE_DLL)
		buildGameModule("native-dll")
	endif()

	if (BUILD_GAME_NATIVE_EXE)
		buildGameModule("native-exe")
	endif()

	if (NOT NACL_VMS_TARGET_CREATED AND NOT GAME_BUILD_SUBINVOCATION)
		# Create the nacl-vms target only the first time that GAMEMODULE is called.
		set(NACL_VMS_TARGET_CREATED ON PARENT_SCOPE)

		if (CMAKE_GENERATOR MATCHES "Visual Studio")
			set(VM_GENERATOR "NMake Makefiles")
		else()
			set(VM_GENERATOR ${CMAKE_GENERATOR})
		endif()

		set(INHERITED_OPTION_ARGS)

		foreach(inherited_option ${NACL_VM_INHERITED_OPTIONS})
			set(INHERITED_OPTION_ARGS ${INHERITED_OPTION_ARGS}
				"-D${inherited_option}=${${inherited_option}}")
		endforeach(inherited_option)

		if (BUILD_GAME_NACL AND NOT YOKAI_TARGET_SYSTEM_NACL)
			if (USE_NACL_SAIGO)
				add_custom_target(nacl-vms ALL)
				unset(NACL_VMS_PROJECTS)

				foreach(nacl_target IN LISTS NACL_TARGETS)
					if (NOT nacl_target IN_LIST NACL_ALL_TARGETS)
						message(FATAL_ERROR "Unknown NaCl architecture ${NACL_TARGET}")
					endif()

					set(NACL_VMS_PROJECT nacl-vms-${nacl_target})
					list(APPEND NACL_VMS_PROJECTS ${NACL_VMS_PROJECT})
					add_dependencies(nacl-vms ${NACL_VMS_PROJECT})

					# TODO: Remove USE_NACL_SAIGO once the game uses YOKAI_TARGET_SYSTEM_NACL.
					gameSubProject(
						"-DCMAKE_TOOLCHAIN_FILE=${Daemon_SOURCE_DIR}/cmake/cross-toolchain-saigo-${nacl_target}.cmake"
						"-DPREFIX_SAIGO=${DEPS_DIR}/saigo_newlib"
						-DBUILD_GAME_NACL=ON
						-DBUILD_GAME_NATIVE_DLL=OFF
						-DBUILD_GAME_NATIVE_EXE=OFF
						-DUSE_NACL_SAIGO=ON
					)
				endforeach()
			else()
				set(NACL_VMS_PROJECT nacl-vms)
				set(NACL_VMS_PROJECTS ${NACL_VMS_PROJECT})

				# Workaround a bug where CMake ExternalProject lists-as-args are cut on first “;”
				string(REPLACE ";" "," NACL_TARGETS_STRING "${NACL_TARGETS}")

				gameSubProject(
					"-DCMAKE_TOOLCHAIN_FILE=${Daemon_SOURCE_DIR}/cmake/cross-toolchain-pnacl.cmake"
					"-DPREFIX_PNACL=${DEPS_DIR}/pnacl"
					-DBUILD_GAME_NACL=ON
					-DBUILD_GAME_NATIVE_DLL=OFF
					-DBUILD_GAME_NATIVE_EXE=OFF
					"-DNACL_TARGETS_STRING=${NACL_TARGETS_STRING}"
				)
			endif()
		endif()

		set(NACL_VMS_PROJECTS ${NACL_VMS_PROJECTS} PARENT_SCOPE)
	elseif (GAME_BUILD_SUBINVOCATION)
		if (BUILD_GAME_NACL)
			if (YOKAI_CXX_COMPILER_SAIGO)
				set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
			else()
				# Put the .nexe and .pexe files in the same directory as the engine.
				set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/..)
			endif()

			buildGameModule("nacl")

			if (YOKAI_CXX_COMPILER_SAIGO)
				include(DaemonSaigoFinalize)

				# Finalize NaCl executables for supported architectures.
				saigo_finalize(
					"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
					"${GAMEMODULE_NAME}"
					"${YOKAI_TARGET_ARCH_NAME}"
				)
			else()
				include(DaemonPNaClFinalize)

				# Revert a workaround for a bug where CMake ExternalProject lists-as-args are cut on first “;”
				string(REPLACE "," ";" NACL_TARGETS "${NACL_TARGETS_STRING}")

				# Generate NaCl executables for supported architectures.
				foreach(nacl_target ${NACL_TARGETS})
					pnacl_finalize(
						"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
						"${GAMEMODULE_NAME}"
						"${nacl_target}"
					)
				endforeach()
			endif()
		endif()
	endif()
endfunction()
