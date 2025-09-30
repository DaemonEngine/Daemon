from os import system, remove

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

for header in headers:
	if header[2]:
		define = " -define " + header[2]
	else:
		define = ""
	system( "python genvk.py -o ../../../src/engine/renderer-vulkan/VulkanLoader/vulkan -apiname vulkan -mode " + header[1] + define + " " + header[0] + ".h" )

with open( "FunctionDecls.h", "r" ) as inp:
	with open( "../../../src/engine/renderer-vulkan/VulkanLoader/Vulkan.h", "w" ) as out:
		out.write( inp.read() )
		out.write( '#endif // VULKAN_LOADER_H' )

with open( 'VulkanLoadFunctions.cpp', mode = 'r', encoding = 'utf-8', newline = '\n' ) as inp:
	functionLoadStart = inp.read()

with open( "FunctionLoaderInstance.cpp", "r" ) as inp:
	with open( "FunctionLoaderDevice.cpp", "r" ) as inp2:
		with open( "../../../src/engine/renderer-vulkan/VulkanLoader/VulkanLoadFunctions.cpp", "w" ) as out:
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