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

from os import system, remove, environ

import Globals

headers = [
	( "vulkan_core",        "" ),
	( "vulkan_beta",        "VK_ENABLE_BETA_EXTENSIONS" ),
	( "vulkan_win32",       "VK_USE_PLATFORM_WIN32_KHR" ),
	( "vulkan_wayland",     "VK_USE_PLATFORM_WAYLAND_KHR" ),
	( "vulkan_xlib",        "VK_USE_PLATFORM_XLIB_KHR" ),
	( "vulkan_xlib_xrandr", "VK_USE_PLATFORM_XLIB_XRANDR_EXT" )
]

def CreateFiles( files ):
	for file in files:
		open( file, "w", encoding = "utf-8" )
	
def DeleteFiles( files ):
	for file in files:
		remove( file )

vulkanLoaderPath       = "../../src/engine/renderer-vulkan/VulkanLoader/"
vulkanGraphicsCorePath = "../../src/engine/renderer-vulkan/GraphicsCore/"

tmpFiles = ( "FunctionDecls.h", "FunctionLoaderInstance.cpp", "FunctionLoaderDevice.cpp", "FunctionStore.cpp",
			 "FeaturesConfig.h", "FeaturesConfigGet", "FeaturesConfigCreate", "FeaturesConfigCreateDevice", "FeaturesConfigMap",
			 "FeaturesConfigMain.h", "FeaturesConfigGetMain", "FeaturesConfigCreateMain", "FeaturesConfigCreateDeviceMain", "FeaturesConfigMapMain",
			 "prev" )

CreateFiles( tmpFiles )

with open( "../../LICENSE-DAEMON-VULKAN.txt", "r" ) as inp:
	license = "/*\n" + inp.read() + "\n*/\n\n"

with open( "Vulkan.h", mode = "r", encoding = "utf-8", newline = "\n" ) as inp:
	with open( "FunctionDecls.h", mode = "w", encoding = "utf-8", newline = "\n" ) as out:
		out.write( inp.read() )


for header in headers:
	environ["DAEMON_VULKAN_HEADER_DEFINE"] = header[1]

	system( "python genvk.py -o " + vulkanLoaderPath + "vulkan -apiname vulkan " + header[0] + ".h" )


with open( "FunctionDecls.h", "r" ) as inp:
	with open( vulkanLoaderPath + "Vulkan.h", mode = "w", encoding = "utf-8", newline = "" ) as out:
		out.write( license )
		out.write( inp.read() )
		out.write( "\n\n#endif // VULKAN_LOADER_H" )

with open( "VulkanLoadFunctions.cpp", mode = "r", encoding = "utf-8", newline = "\n" ) as inp:
	functionLoadStart = inp.read()

with open( "FunctionLoaderInstance.cpp", "r" ) as inp:
	with open( "FunctionLoaderDevice.cpp", "r" ) as inp2:
		with open( vulkanLoaderPath + "VulkanLoadFunctions.cpp", mode = "w", encoding = "utf-8", newline = "" ) as out:
			out.write( license )
			out.write( functionLoadStart )
			out.write( "\n\nvoid VulkanLoadInstanceFunctions( VkInstance instance ) {\n" )
			out.write( inp.read() )
			out.write( "}\n\n" )
			out.write( "void VulkanLoadDeviceFunctions( VkDevice device ) {\n" )
			out.write( inp2.read() )
			out.write( "}" )

with open( "FunctionStore.cpp", "r" ) as inp:
	with open( vulkanLoaderPath + "Vulkan.cpp", mode = "w", encoding = "utf-8", newline = "" ) as out:
		out.write( license )
		out.write( "// Auto-generated, do not modify\n\n" )
		out.write( "#include \"Vulkan.h\"" )
		out.write( inp.read() )

with open( "FeaturesConfig.cpp", mode = "r", encoding = "utf-8", newline = "" ) as inp:
	featuresConfigStart = inp.read()

with open( "FeaturesConfigGet", mode = "r" ) as inpGet:
	with open( "FeaturesConfigCreate", mode = "r" ) as inpCreate:
		with open( "FeaturesConfigGetMain", mode = "r" ) as inpGetMain:
			with open( "FeaturesConfigCreateMain", mode = "r" ) as inpCreateMain:
				with open( "FeaturesConfigCreateDevice", mode = "r" ) as inpCreateDevice:
					with open( "FeaturesConfigCreateDeviceMain", mode = "r" ) as inpCreateDeviceMain:
						with open( vulkanGraphicsCorePath + "FeaturesConfig.cpp", mode = "w", encoding = "utf-8", newline = "" ) as out:
							out.write( license )
							out.write( featuresConfigStart )
							out.write( inpGet.read() )
							out.write( inpGetMain.read() )
							out.write( "\tFeaturesConfig cfg {\n" )
							out.write( inpCreate.read() )
							out.write( inpCreateMain.read() )
							out.write( "int CreateDevice( VkDeviceCreateInfo& deviceInfo, const VkAllocationCallbacks* allocator,\n" )
							out.write( "                  const EngineConfig& engineCfg, const FeaturesConfig& cfg, VkDevice* device ) {\n" )
							out.write( "\tconst bool intelWorkaround = std::string( engineCfg.driverName ).find( \"Intel\" ) != std::string::npos;\n\n" )
							out.write( inpCreateDevice.read() )
							out.write( inpCreateDeviceMain.read() )

with open( "FeaturesConfig.h", mode = "r" ) as inpCfg:
	with open( "FeaturesConfigMain.h", mode = "r" ) as inpCfgMain:
		with open( vulkanGraphicsCorePath + "FeaturesConfig.h", mode = "w", encoding = "utf-8", newline = "" ) as out:
			out.write( license )
			out.write( "// Auto-generated, do not modify\n\n" )
			out.write( "#ifndef FEATURES_CONFIG_H\n" )
			out.write( "#define FEATURES_CONFIG_H\n\n" )
			out.write( "#include \"Decls.h\"\n\n" )
			out.write( "struct FeaturesConfig {\n" )
			out.write( inpCfg.read() )
			out.write( inpCfgMain.read() )
			out.write( "};\n\n" )
			out.write( "FeaturesConfig GetPhysicalDeviceFeatures( const VkPhysicalDevice physicalDevice, const EngineConfig& engineCfg );\n\n" )
			out.write( "int            CreateDevice( VkDeviceCreateInfo& deviceInfo, const VkAllocationCallbacks* allocator,\n" )
			out.write( "                             const EngineConfig& engineCfg, const FeaturesConfig& cfg, VkDevice* device );\n\n" )
			out.write( "#endif // FEATURES_CONFIG_H\n" )

with open( "FeaturesConfigMap", mode = "r" ) as inpMap:
	with open( "FeaturesConfigMapMain", mode = "r" ) as inpMapMain:
		with open( vulkanGraphicsCorePath + "FeaturesConfigMap.cpp", mode = "w", encoding = "utf-8", newline = "" ) as out:
			out.write( license )
			out.write( "// Auto-generated, do not modify\n\n" )
			out.write( "#include \"FeaturesConfig.h\"\n\n" )
			out.write( "#include \"FeaturesConfigMap.h\"\n\n" )
			out.write( "std::unordered_map<std::string, FeatureData> featuresConfigMap {\n" )
			out.write( inpMap.read() )
			out.write( inpMapMain.read().rstrip( "," ) + "};" )

DeleteFiles( tmpFiles )