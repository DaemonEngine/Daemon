# ===========================================================================
#
# Daemon BSD Source Code
# Copyright (c) 2025 Daemon Developers
# All rights reserved.
#
# This file is part of the Daemon BSD Source Code (Daemon Source Code).
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 	* Redistributions of source code must retain the above copyright
# 	  notice, this list of conditions and the following disclaimer.
# 	* Redistributions in binary form must reproduce the above copyright
# 	  notice, this list of conditions and the following disclaimer in the
# 	  documentation and/or other materials provided with the distribution.
# 	* Neither the name of the Daemon developers nor the
# 	  names of its contributors may be used to endorse or promote products
# 	  derived from this software without specific prior written permission.
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
#
# ===========================================================================

find_package( Vulkan REQUIRED COMPONENTS glslangValidator glslang )

if( NOT Vulkan_FOUND )
	message( FATAL_ERROR "Could NOT find libVulkan" )
endif()

set( generatorPath ${CMAKE_SOURCE_DIR}/cmake/DaemonVulkan/VulkanHeaders/ )
set( vulkanLoaderPath ${CMAKE_SOURCE_DIR}/src/engine/renderer-vulkan/VulkanLoader/vulkan/ )

macro( GenerateVulkanHeader target mode )
	add_custom_command(
		COMMAND ${PYTHON_EXECUTABLE} ${Python_EXECUTABLE} ${generatorPath}/genvk.py -registry ${generatorPath}/vk.xml -o ${vulkanLoaderPath} -apiname vulkan -mode ${mode} ${target}.h
		DEPENDS
			${generatorPath}/vk.xml
			${generatorPath}/genvk.py
			${generatorPath}/reg.py
			${generatorPath}/generator.py
			${generatorPath}/cgenerator.py
		OUTPUT
			${vulkanLoaderPath}/${target}.h
		COMMENT "Generating Vulkan header: ${target}.h"
	)
endmacro()

set( vulkanHeaders
	# vulkan_core
	"vulkan_beta"
	"vulkan_win32"
	"vulkan_wayland"
	"vulkan_xlib"
	"vulkan_xlib_xrandr"
)

function( GenerateVulkanHeaders )
	find_package( Python REQUIRED )

	if( EXISTS ${generatorPath}/vulkan_core.h )
		configure_file( ${vulkanLoaderPath}/vulkan_core.h ${generatorPath}/vulkan_core.h.tmp COPYONLY )
	else()
		set( REGENERATE )
	endif()

	foreach( header ${vulkanHeaders} )
		if( EXISTS ${vulkanLoaderPath}/${header}.h )
			configure_file( ${vulkanLoaderPath}/${header}.h ${generatorPath}/${header}.h.tmp COPYONLY )
		else()
			set( REGENERATE )
		endif()
	endforeach()
	
	if( REGENERATE OR ${generatorPath}/vk.xml IS_NEWER_THAN ${generatorPath}/vk.xml.tmp OR NOT EXISTS ${generatorPath}/vk.xml.tmp )
		configure_file( ${generatorPath}/vk.xml ${generatorPath}/vk.xml.tmp COPYONLY )
		
		GenerateVulkanHeader( "vulkan_core" "w" )
		
		foreach( header ${vulkanHeaders} )
			GenerateVulkanHeader( ${header} "a" )
		endforeach()

		add_custom_target(vht ALL
			DEPENDS ${vulkanLoaderPath}/vulkan_core.h ${vulkanHeaders}
		)
	endif()
endfunction()

function( GenerateVulkanHeaders2 )
	GenerateVulkanHeader( "vulkan_core" "w" )
	GenerateVulkanHeader( "vulkan_beta" "a" )
	GenerateVulkanHeader( "vulkan_win32" "a" )
	GenerateVulkanHeader( "vulkan_wayland" "a" )
	GenerateVulkanHeader( "vulkan_xlib" "a" )
	GenerateVulkanHeader( "vulkan_xlib_xrandr" "a" )
	
	foreach( header ${vulkanHeaders} )
		GenerateVulkanHeader( ${header} "a" )
	endforeach()

	add_custom_target(vht ALL
		DEPENDS ${vulkanLoaderPath}/vulkan_core.h ${vulkanHeaders}
	)
endfunction()

# GenerateVulkanHeaders()

add_executable( VulkanShaderParser "${DAEMON_DIR}/cmake/DaemonVulkan/VulkanShaderParser.cpp" )

set( GRAPHICS_ENGINE_PATH ${DAEMON_DIR}/src/engine/renderer-vulkan/GraphicsEngine/ )
set( GRAPHICS_ENGINE_PROCESSED_PATH ${DAEMON_GENERATED_DIR}/DaemonVulkan/GraphicsEngineProcessed/ )
target_compile_definitions( VulkanShaderParser PRIVATE "-DDAEMON_VULKAN_GRAPHICS_ENGINE_PATH=\"${GRAPHICS_ENGINE_PATH}\"" )
target_compile_definitions( VulkanShaderParser PRIVATE "-DDAEMON_VULKAN_GRAPHICS_ENGINE_PROCESSED_PATH=\"${GRAPHICS_ENGINE_PROCESSED_PATH}\"" )

file( MAKE_DIRECTORY ${GRAPHICS_ENGINE_PROCESSED_PATH} )

option( VULKAN_SPIRV_OUT "Output human-readable SPIR-V files alongside the binary format." ON )
option( VULKAN_SPIRV_OPTIMISE "Enable SPIR-V optimisations." ON )
option( VULKAN_SPIRV_LTO "Enable link-time SPIR-V optimisations." ON )

set( VULKAN_SPIRV_DEBUG "default" CACHE STRING "glslangValidator debug options (remove: g0, non-semantic: gV)")
set_property( CACHE VULKAN_SPIRV_DEBUG PROPERTY STRINGS default remove non-semantic )

macro( GenerateVulkanShaders target )
	# Pre-processing for #insert/#include
	foreach( src IN LISTS GRAPHICSENGINELIST )
		set( graphicsProcessedList ${graphicsProcessedList} ${src} )
		list( APPEND graphicsEngineOutputList ${GRAPHICS_ENGINE_PROCESSED_PATH}${src} )
	endforeach()

	add_custom_command(
		COMMAND VulkanShaderParser ${graphicsProcessedList}
		DEPENDS ${GRAPHICSENGINEIDELIST}
		OUTPUT ${graphicsEngineOutputList}
		COMMENT "Generating Vulkan graphics engine: ${graphicsProcessedList}"
	)

	add_custom_target( VulkanShaderParserTarget ALL
		DEPENDS ${graphicsEngineOutputList}
	)

	# glslangValidator
	find_program( glslangV glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/ )

	file( MAKE_DIRECTORY ${DAEMON_GENERATED_DIR}/DaemonVulkan/GraphicsEngine/spirv/ )
	file( MAKE_DIRECTORY ${DAEMON_GENERATED_DIR}/DaemonVulkan/GraphicsEngine/bin/ )

	set( spirvOptions --target-env vulkan1.3 --glsl-version 460 -e main -l -t )

	if( NOT VULKAN_SPIRV_OPTIMISE )
		set( spirvOptions ${spirvOptions} -Od )
	endif()

	if( VULKAN_SPIRV_LTO )
		set( spirvOptions ${spirvOptions} --lto )
	endif()

	if( VULKAN_SPIRV_DEBUG STREQUAL "remove" )
		set( spirvOptions ${spirvOptions} -g0 )
	elseif( VULKAN_SPIRV_DEBUG STREQUAL "non-semantic" )
		set( spirvOptions ${spirvOptions} -gV )
	endif()

	foreach( src IN LISTS GRAPHICSENGINELIST )
		set( srcPath ${GRAPHICS_ENGINE_PROCESSED_PATH}${src} )

		# set( spirvAsmPath ${GRAPHICS_ENGINE_PROCESSED_PATH}spirv/${src} )
		# string( REGEX REPLACE "[.]glsl$" ".spirv" spirvAsmPath ${spirvAsmPath} )
		get_filename_component( spirvAsmPath "${src}" NAME_WE )
		message( STATUS ${spirvAsmPath} )
		set( spirvAsmPath ${DAEMON_GENERATED_DIR}/DaemonVulkan/GraphicsEngine/spirv/${spirvAsmPath}.spirv )

		set( spirvBinPath ${DAEMON_GENERATED_DIR}/DaemonVulkan/GraphicsEngine/bin/${src} )
		string( REGEX REPLACE "[.]glsl$" ".spirvBin" spirvBinPath ${spirvBinPath} )

		#message(STATUS
		#	"${spirvOptions} -S comp -V ${srcPath} -o ${spirvBinPath} -H > ${spirvAsmPath}" )
		add_custom_command(
			OUTPUT ${spirvBinPath}
			COMMAND ${glslangV} ${spirvOptions} -S comp -V ${srcPath} -o ${spirvBinPath} -H > ${spirvAsmPath}
			DEPENDS ${srcPath}
			COMMENT "Generating Vulkan graphics engine binaries: ${src}"
		)
		
		list( APPEND spirvBinList ${spirvBinPath} )
	endforeach()

	add_custom_target( VulkanShaderBinTarget ALL
		DEPENDS ${spirvBinList}
	)

	target_sources( ${target} PRIVATE ${graphicsEngineOutputList} )
endmacro()