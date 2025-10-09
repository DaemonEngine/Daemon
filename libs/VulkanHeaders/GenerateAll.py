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

from os import system, remove
from sys import executable
from subprocess import run

headers = [
	( "vulkan_core",        "w", "" ),
	( "vulkan_beta",        "a", "VK_ENABLE_BETA_EXTENSIONS" ),
	( "vulkan_win32",       "a", "VK_USE_PLATFORM_WIN32_KHR" ),
	( "vulkan_wayland",     "a", "VK_USE_PLATFORM_WAYLAND_KHR" ),
	( "vulkan_xlib",        "a", "VK_USE_PLATFORM_XLIB_KHR" ),
	( "vulkan_xlib_xrandr", "a", "VK_USE_PLATFORM_XLIB_XRANDR_EXT" )
]

with open( "FunctionDecls.h", "w" ) as f:
	with open( "FunctionLoaderInstance.cpp", "w" ) as f1:
		with open( "FunctionLoaderDevice.cpp", "w" ) as f2:
			print( "" )

vulkanLoaderPath = "../../src/engine/renderer-vulkan/VulkanLoader/"

for header in headers:
	if header[2]:
		run( [executable, "genvk.py", "-o", vulkanLoaderPath + "vulkan/", "-apiname", "vulkan", "-mode", header[1],
			"-define", header[2], header[0] + ".h"], check = True )
	else:
		run( [executable, "genvk.py", "-o", vulkanLoaderPath + "vulkan/", "-apiname", "vulkan", "-mode", header[1],
			header[0] + ".h"], check = True )

with open( "FunctionDecls.h", "r" ) as inp:
	with open( vulkanLoaderPath + "Vulkan.h", "w" ) as out:
		out.write( inp.read() )
		out.write( '#endif // VULKAN_LOADER_H' )

with open( 'VulkanLoadFunctions.cpp', mode = 'r', encoding = 'utf-8', newline = '\n' ) as inp:
	functionLoadStart = inp.read()

with open( "FunctionLoaderInstance.cpp", "r" ) as inp:
	with open( "FunctionLoaderDevice.cpp", "r" ) as inp2:
		with open( vulkanLoaderPath + "VulkanLoadFunctions.cpp", "w" ) as out:
			out.write( functionLoadStart )
			out.write( '\n\nvoid VulkanLoadInstanceFunctions( VkInstance instance ) {\n' )
			out.write( inp.read() )
			out.write( '}\n\n' )
			out.write( 'void VulkanLoadDeviceFunctions( VkDevice device ) {\n' )
			out.write( inp2.read() )
			out.write( '}' )

remove( "FunctionDecls.h" )
remove( "FunctionLoaderInstance.cpp" )
remove( "FunctionLoaderDevice.cpp" )