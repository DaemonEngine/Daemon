# =============================================================================
# Daemon-Vulkan BSD Source Code
# Copyright (c) 2025-2026 Reaper
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Reaper nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL REAPER BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# =============================================================================

option( VULKAN_SPIRV_OPTIMISE "Enable SPIR-V optimisations." ON )
option( VULKAN_SPIRV_LTO "Enable link-time SPIR-V optimisations." ON )

set( VULKAN_SPIRV_DEBUG "default" CACHE STRING "glslangValidator debug options (remove: g0, non-semantic: gV)" )
set_property( CACHE VULKAN_SPIRV_DEBUG PROPERTY STRINGS default remove non-semantic )

if( USE_VULKAN )
	add_executable( GraphicsEngineProcessor "${DAEMON_DIR}/src/engine/renderer-vulkan/GraphicsEngineProcessor.cpp" )

	set( GRAPHICS_CORE_PATH ${DAEMON_DIR}/src/engine/renderer-vulkan/GraphicsCore/ )
	set( GRAPHICS_ENGINE_PATH ${DAEMON_DIR}/src/engine/renderer-vulkan/GraphicsEngine/ )
	set( GRAPHICS_SHARED_PATH ${DAEMON_DIR}/src/engine/renderer-vulkan/GraphicsShared/ )
	set( GRAPHICS_ENGINE_PROCESSED_PATH ${CMAKE_CURRENT_BINARY_DIR}/GraphicsEngine/ )
	target_compile_definitions( GraphicsEngineProcessor PRIVATE "-DDAEMON_VULKAN_GRAPHICS_CORE_PATH=\"${GRAPHICS_CORE_PATH}\"" )
	target_compile_definitions( GraphicsEngineProcessor PRIVATE "-DDAEMON_VULKAN_GRAPHICS_ENGINE_PATH=\"${GRAPHICS_ENGINE_PATH}\"" )
	target_compile_definitions( GraphicsEngineProcessor PRIVATE "-DDAEMON_VULKAN_GRAPHICS_SHARED_PATH=\"${GRAPHICS_SHARED_PATH}\"" )
	target_compile_definitions( GraphicsEngineProcessor PRIVATE "-DDAEMON_VULKAN_GRAPHICS_ENGINE_PROCESSED_PATH=\"${GRAPHICS_ENGINE_PROCESSED_PATH}\"" )

	file( MAKE_DIRECTORY ${GRAPHICS_ENGINE_PROCESSED_PATH} )
	
	file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/GraphicsEngine/processed/ )
	file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/GraphicsEngine/spirv/ )
	file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/GraphicsEngine/bin/ )
endif()

macro( GenerateVulkanShaders target )
	foreach( src IN LISTS graphicsEngineList )
		set( graphicsEngineProcessedList "${graphicsEngineProcessedList} ${src}" )
		list( APPEND graphicsEngineOutputList ${GRAPHICS_ENGINE_PROCESSED_PATH}processed/${src} )
		
		get_filename_component( name "${src}" NAME_WE )
		
		set( spirvAsmPath ${CMAKE_CURRENT_BINARY_DIR}/GraphicsEngine/spirv/${name}.spirv )
		set( spirvBinPath ${CMAKE_CURRENT_BINARY_DIR}/GraphicsEngine/bin/${name}.spirvBin )
		
		list( APPEND graphicsEngineOutputList ${spirvAsmPath} )
		list( APPEND graphicsEngineOutputList ${spirvBinPath} )
	endforeach()

	# glslang
	find_program( glslangV glslang HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ )

	if( glslangV STREQUAL "glslangV-NOTFOUND" )
		message( FATAL_ERROR "glslang not found; make sure you have the Vulkan SDK installed or build glslang from source" )
	endif()

	set( spirvOptions "--target-env vulkan1.3 --glsl-version 460 -e main -l -t" )

	if( NOT VULKAN_SPIRV_OPTIMISE )
		set( spirvOptions "${spirvOptions} -Od" )
	endif()

	if( VULKAN_SPIRV_LTO )
		set( spirvOptions "${spirvOptions} --lto" )
	endif()

	if( VULKAN_SPIRV_DEBUG STREQUAL "remove" )
		set( spirvOptions "${spirvOptions} -g0" )
	elseif( VULKAN_SPIRV_DEBUG STREQUAL "non-semantic" )
		set( spirvOptions "${spirvOptions} -gV" )
	endif()

	string( REPLACE ";" " " graphicsEngineLogList "${graphicsEngineProcessedList}" )

	set( fuckingCmdArgsBecauseCmakeIsRetarded "${glslangV} \"${spirvOptions}\" ${graphicsEngineProcessedList}" )

	message( STATUS ${fuckingCmdArgsBecauseCmakeIsRetarded} )

	add_custom_command(
		COMMAND GraphicsEngineProcessor ${fuckingCmdArgsBecauseCmakeIsRetarded}
		DEPENDS ${graphicsEngineIDEList} ${graphicsSharedList} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DaemonVulkan.cmake GraphicsEngineProcessor
		OUTPUT ${graphicsEngineOutputList} ${GRAPHICS_CORE_PATH}/ExecutionGraph/SPIRV.h ${GRAPHICS_CORE_PATH}/ExecutionGraph/SPIRVBin.h
		COMMENT "Generating Vulkan Graphics Engine: ${graphicsEngineLogList}"
	)
	
	foreach( src IN LISTS graphicsEngineList )
		set( srcPath ${GRAPHICS_ENGINE_PROCESSED_PATH}processed/${src} )

		get_filename_component( name "${src}" NAME_WE )
		
		set( spirvAsmPath ${CMAKE_CURRENT_BINARY_DIR}/GraphicsEngine/spirv/${name}.spirv )

		set( spirvBinPath ${CMAKE_CURRENT_BINARY_DIR}/GraphicsEngine/bin/${name}.spirvBin )

		list( POP_FRONT stagesList stage )
		
		list( APPEND spirvBinList ${spirvBinPath} )
	endforeach()

	target_sources( ${target} PRIVATE ${GRAPHICS_CORE_PATH}/ExecutionGraph/SPIRVBin.h ${GRAPHICS_CORE_PATH}/ExecutionGraph/SPIRV.h )
endmacro()