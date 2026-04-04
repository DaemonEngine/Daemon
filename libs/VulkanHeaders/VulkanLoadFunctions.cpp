// Auto-generated, do not modify

#ifdef _MSC_VER
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

#include "Vulkan.h"

#include "VulkanLoadFunctions.h"

#ifdef _MSC_VER
	HMODULE libVulkan;
#else
	void*   libVulkan;
#endif

void VulkanLoaderInit() {
	#ifdef _MSC_VER
		libVulkan = LoadLibrary( "vulkan-1.dll" );
		vkGetInstanceProcAddr = ( PFN_vkGetInstanceProcAddr ) GetProcAddress( libVulkan, "vkGetInstanceProcAddr" );
	#else
		libVulkan = dlopen( "libvulkan.so", RTLD_NOW );
		vkGetInstanceProcAddr = dlsym( libVulkan, "vkGetInstanceProcAddr" );
	#endif

	vkEnumerateInstanceVersion = ( PFN_vkEnumerateInstanceVersion ) vkGetInstanceProcAddr( nullptr, "vkEnumerateInstanceVersion" );

	vkEnumerateInstanceExtensionProperties = ( PFN_vkEnumerateInstanceExtensionProperties ) vkGetInstanceProcAddr( nullptr, "vkEnumerateInstanceExtensionProperties" );

	vkEnumerateInstanceLayerProperties = ( PFN_vkEnumerateInstanceLayerProperties ) vkGetInstanceProcAddr( nullptr, "vkEnumerateInstanceLayerProperties" );

	vkCreateInstance = ( PFN_vkCreateInstance ) vkGetInstanceProcAddr( nullptr, "vkCreateInstance" );
}

void VulkanLoaderFree() {
	#ifdef _MSC_VER
		FreeLibrary( libVulkan );
	#else
		dlclose( libVulkan );
	#endif
}