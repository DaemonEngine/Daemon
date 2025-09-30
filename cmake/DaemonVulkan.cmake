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

#try_compile( BUILD_RESULT
#	"${CMAKE_BINARY_DIR}"
#	"${DAEMON_DIR}/cmake/DaemonVulkan/VulkanHeaderParser.cpp"
#	CMAKE_FLAGS ""
#	OUTPUT_VARIABLE BUILD_LOG
#)

#if ( NOT BUILD_RESULT )
#	message( FATAL
#		"Failed to build VulkanHeaderParser.cpp\n"
#	)
#endif()

#set( vulkanHeaderPath "${DAEMON_DIR}/cmake/DaemonVulkan/" )
#set( vulkanLoaderPath "${DAEMON_DIR}/src/engine/renderer-vulkan/VulkanLoader/" )
#if( MSVC )
#	string( REPLACE "/" "\\" vulkanHeaderPath ${vulkanHeaderPath} )
#	string( REPLACE "/" "\\" vulkanLoaderPath ${vulkanHeaderPath} )
#endif()

# option( BUILD_VULKAN_HEADER_PARSER "Build Vulkan header parser for default sType and function declarations/definitions." OFF )
# mark_as_advanced( BUILD_VULKAN_HEADER_PARSER )

# add_executable( VulkanHeaderParser "${DAEMON_DIR}/cmake/DaemonVulkan/VulkanHeaderParser.cpp" )
# target_compile_definitions( VulkanHeaderParser PRIVATE "-DDAEMON_VULKAN_HEADER_PATH=\"${DAEMON_DIR}/cmake/DaemonVulkan/\"" )
# target_compile_definitions( VulkanHeaderParser PRIVATE "-DDAEMON_VULKAN_LOADER_PATH=\"${DAEMON_DIR}/src/engine/renderer-vulkan/VulkanLoader/\"" )