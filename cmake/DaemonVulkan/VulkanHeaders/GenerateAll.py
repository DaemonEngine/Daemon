from os import system, remove

headers = [
	( "vulkan_core",        "w", "" ),
	( "vulkan_beta",        "a", "VK_ENABLE_BETA_EXTENSIONS" ),
	( "vulkan_win32",       "a", "VK_USE_PLATFORM_WIN32_KHR" ),
	( "vulkan_wayland",     "a", "VK_USE_PLATFORM_WAYLAND_KHR" ),
	( "vulkan_xlib",        "a", "VK_USE_PLATFORM_XLIB_KHR" ),
	( "vulkan_xlib_xrandr", "a", "VK_USE_PLATFORM_XLIB_XRANDR_EXT" )
]

with open( "FunctionDecls.h", "w", encoding = 'utf-8' ) as f:
	with open( "FunctionLoaderInstance.cpp", "w", encoding = 'utf-8' ) as f1:
		with open( "FunctionLoaderDevice.cpp", "w", encoding = 'utf-8' ) as f2:
			print( "" )

for header in headers:
	if header[2]:
		define = " -define " + header[2]
	else:
		define = ""
	system( "python genvk.py -o ../../../src/engine/renderer-vulkan/VulkanLoader/vulkan -apiname vulkan -mode " + header[1] + define + " " + header[0] + ".h" )

with open( "FunctionDecls.h", "r" ) as inp:
	with open( "../../../src/engine/renderer-vulkan/VulkanLoader/Vulkan.h", mode = 'w', encoding = 'utf-8', newline = '' ) as out:
		out.write( inp.read() )
		out.write( '#endif // VULKAN_LOADER_H' )

with open( 'VulkanLoadFunctions.cpp', mode = 'r', encoding = 'utf-8', newline = '\n' ) as inp:
	functionLoadStart = inp.read()

with open( "FunctionLoaderInstance.cpp", "r" ) as inp:
	with open( "FunctionLoaderDevice.cpp", "r" ) as inp2:
		with open( "../../../src/engine/renderer-vulkan/VulkanLoader/VulkanLoadFunctions.cpp", mode = 'w', encoding = 'utf-8', newline = '' ) as out:
			out.write( functionLoadStart )
			out.write( '\n\nvoid VulkanLoadInstanceFunctions( VkInstance instance ) {\n' )
			out.write( inp.read() )
			out.write( '}\n\n' )
			out.write( 'void VulkanLoadDeviceFunctions( VkDevice device ) {\n' )
			out.write( inp2.read() )
			out.write( '}' )

with open( 'FeaturesConfig.cpp', mode = 'r', encoding = 'utf-8', newline = '' ) as inp:
	featuresConfigStart = inp.read()

with open( "FeaturesConfigGet", mode = 'r' ) as inpGet:
	with open( "FeaturesConfigCreate", mode = 'r' ) as inpCreate:
		with open( "FeaturesConfigGetMain", mode = 'r' ) as inpGetMain:
			with open( "FeaturesConfigCreateMain", mode = 'r' ) as inpCreateMain:
				with open( "FeaturesConfigCreateDevice", mode = 'r' ) as inpCreateDevice:
					with open( "FeaturesConfigCreateDeviceMain", mode = 'r' ) as inpCreateDeviceMain:
						with open( "../../../src/engine/renderer-vulkan/GraphicsCore/FeaturesConfig.cpp", mode = 'w', encoding = 'utf-8', newline = '' ) as out:
							out.write( featuresConfigStart )
							out.write( inpGet.read() )
							out.write( inpGetMain.read() )
							out.write( "\tFeaturesConfig cfg {\n" )
							out.write( inpCreate.read() )
							out.write( inpCreateMain.read() )
							out.write( "int CreatePhysicalDevice( VkDeviceCreateInfo& deviceInfo, const VkAllocationCallbacks* allocator,\n" )
							out.write( "                          const EngineConfig& engineCfg, const FeaturesConfig& cfg, VkDevice* device ) {\n" )
							out.write( "\tconst bool intelWorkaround = std::string( engineCfg.driverName ).find( \"Intel\" ) != std::string::npos;\n\n" )
							out.write( inpCreateDevice.read() )
							out.write( inpCreateDeviceMain.read() )

with open( "FeaturesConfig.h", mode = 'r' ) as inpCfg:
	with open( "FeaturesConfigMain.h", mode = 'r' ) as inpCfgMain:
		with open( "../../../src/engine/renderer-vulkan/GraphicsCore/FeaturesConfig.h", mode = 'w', encoding = 'utf-8', newline = '' ) as out:
			out.write( "// Auto-generated, do not modify\n\n" )
			out.write( "#ifndef FEATURES_CONFIG_H\n" )
			out.write( "#define FEATURES_CONFIG_H\n\n" )
			out.write( "#include \"Decls.h\"\n\n" )
			out.write( "struct FeaturesConfig {\n" )
			out.write( inpCfg.read() )
			out.write( inpCfgMain.read() )
			out.write( "};\n\n" )
			out.write( "FeaturesConfig GetPhysicalDeviceFeatures( const VkPhysicalDevice physicalDevice, const EngineConfig& engineCfg );\n\n" )
			out.write( "int            CreatePhysicalDevice( VkDeviceCreateInfo& deviceInfo, const VkAllocationCallbacks* allocator,\n" )
			out.write( "                                     const EngineConfig& engineCfg, const FeaturesConfig& cfg, VkDevice* device );\n\n" )
			out.write( "#endif // FEATURES_CONFIG_H\n" )

with open( "FeaturesConfigMap", mode = 'r' ) as inpMap:
	with open( "FeaturesConfigMapMain", mode = 'r' ) as inpMapMain:
		with open( "../../../src/engine/renderer-vulkan/GraphicsCore/FeaturesConfigMap.cpp", mode = 'w', encoding = 'utf-8', newline = '' ) as out:
			out.write( "// Auto-generated, do not modify\n\n" )
			out.write( "#include \"FeaturesConfig.h\"\n\n" )
			out.write( "#include \"FeaturesConfigMap.h\"\n\n" )
			out.write( "std::unordered_map<std::string, FeatureData> featuresConfigMap {\n" )
			out.write( inpMap.read() )
			out.write( inpMapMain.read().rstrip( "," ) + "};" )

remove( "FunctionDecls.h" )
remove( "FunctionLoaderInstance.cpp" )
remove( "FunctionLoaderDevice.cpp" )
remove( "FeaturesConfig.h" )
remove( "FeaturesConfigMain.h" )
remove( "FeaturesConfigGet" )
remove( "FeaturesConfigGetMain" )
remove( "FeaturesConfigCreate" )
remove( "FeaturesConfigCreateMain" )
remove( "FeaturesConfigCreateDevice" )
remove( "FeaturesConfigCreateDeviceMain" )
remove( "FeaturesConfigMap" )
remove( "FeaturesConfigMapMain" )
remove( "prev" )