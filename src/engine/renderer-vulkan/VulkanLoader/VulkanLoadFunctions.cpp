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

void VulkanLoadInstanceFunctions( VkInstance instance ) {
	vkEnumeratePhysicalDevices = ( PFN_vkEnumeratePhysicalDevices ) vkGetInstanceProcAddr( instance, "vkEnumeratePhysicalDevices" );

	vkGetPhysicalDeviceFeatures = ( PFN_vkGetPhysicalDeviceFeatures ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFeatures" );

	vkGetPhysicalDeviceFormatProperties = ( PFN_vkGetPhysicalDeviceFormatProperties ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFormatProperties" );

	vkGetPhysicalDeviceImageFormatProperties = ( PFN_vkGetPhysicalDeviceImageFormatProperties ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceImageFormatProperties" );

	vkGetPhysicalDeviceProperties = ( PFN_vkGetPhysicalDeviceProperties ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceProperties" );

	vkGetPhysicalDeviceQueueFamilyProperties = ( PFN_vkGetPhysicalDeviceQueueFamilyProperties ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceQueueFamilyProperties" );

	vkGetPhysicalDeviceMemoryProperties = ( PFN_vkGetPhysicalDeviceMemoryProperties ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceMemoryProperties" );

	vkGetDeviceProcAddr = ( PFN_vkGetDeviceProcAddr ) vkGetInstanceProcAddr( instance, "vkGetDeviceProcAddr" );

	vkCreateDevice = ( PFN_vkCreateDevice ) vkGetInstanceProcAddr( instance, "vkCreateDevice" );

	vkEnumerateDeviceExtensionProperties = ( PFN_vkEnumerateDeviceExtensionProperties ) vkGetInstanceProcAddr( instance, "vkEnumerateDeviceExtensionProperties" );

	vkEnumerateDeviceLayerProperties = ( PFN_vkEnumerateDeviceLayerProperties ) vkGetInstanceProcAddr( instance, "vkEnumerateDeviceLayerProperties" );

	vkGetPhysicalDeviceSparseImageFormatProperties = ( PFN_vkGetPhysicalDeviceSparseImageFormatProperties ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSparseImageFormatProperties" );

	vkEnumeratePhysicalDeviceGroups = ( PFN_vkEnumeratePhysicalDeviceGroups ) vkGetInstanceProcAddr( instance, "vkEnumeratePhysicalDeviceGroups" );

	vkGetPhysicalDeviceFeatures2 = ( PFN_vkGetPhysicalDeviceFeatures2 ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFeatures2" );

	vkGetPhysicalDeviceProperties2 = ( PFN_vkGetPhysicalDeviceProperties2 ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceProperties2" );

	vkGetPhysicalDeviceFormatProperties2 = ( PFN_vkGetPhysicalDeviceFormatProperties2 ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFormatProperties2" );

	vkGetPhysicalDeviceImageFormatProperties2 = ( PFN_vkGetPhysicalDeviceImageFormatProperties2 ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceImageFormatProperties2" );

	vkGetPhysicalDeviceQueueFamilyProperties2 = ( PFN_vkGetPhysicalDeviceQueueFamilyProperties2 ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceQueueFamilyProperties2" );

	vkGetPhysicalDeviceMemoryProperties2 = ( PFN_vkGetPhysicalDeviceMemoryProperties2 ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceMemoryProperties2" );

	vkGetPhysicalDeviceSparseImageFormatProperties2 = ( PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSparseImageFormatProperties2" );

	vkGetPhysicalDeviceExternalBufferProperties = ( PFN_vkGetPhysicalDeviceExternalBufferProperties ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalBufferProperties" );

	vkGetPhysicalDeviceExternalFenceProperties = ( PFN_vkGetPhysicalDeviceExternalFenceProperties ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalFenceProperties" );

	vkGetPhysicalDeviceExternalSemaphoreProperties = ( PFN_vkGetPhysicalDeviceExternalSemaphoreProperties ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalSemaphoreProperties" );

	vkGetPhysicalDeviceToolProperties = ( PFN_vkGetPhysicalDeviceToolProperties ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceToolProperties" );

	vkDestroySurfaceKHR = ( PFN_vkDestroySurfaceKHR ) vkGetInstanceProcAddr( instance, "vkDestroySurfaceKHR" );

	vkGetPhysicalDeviceSurfaceSupportKHR = ( PFN_vkGetPhysicalDeviceSurfaceSupportKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceSupportKHR" );

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR = ( PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );

	vkGetPhysicalDeviceSurfaceFormatsKHR = ( PFN_vkGetPhysicalDeviceSurfaceFormatsKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceFormatsKHR" );

	vkGetPhysicalDeviceSurfacePresentModesKHR = ( PFN_vkGetPhysicalDeviceSurfacePresentModesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfacePresentModesKHR" );

	vkGetPhysicalDevicePresentRectanglesKHR = ( PFN_vkGetPhysicalDevicePresentRectanglesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDevicePresentRectanglesKHR" );

	vkGetPhysicalDeviceDisplayPropertiesKHR = ( PFN_vkGetPhysicalDeviceDisplayPropertiesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceDisplayPropertiesKHR" );

	vkGetPhysicalDeviceDisplayPlanePropertiesKHR = ( PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR" );

	vkGetDisplayPlaneSupportedDisplaysKHR = ( PFN_vkGetDisplayPlaneSupportedDisplaysKHR ) vkGetInstanceProcAddr( instance, "vkGetDisplayPlaneSupportedDisplaysKHR" );

	vkGetDisplayModePropertiesKHR = ( PFN_vkGetDisplayModePropertiesKHR ) vkGetInstanceProcAddr( instance, "vkGetDisplayModePropertiesKHR" );

	vkCreateDisplayModeKHR = ( PFN_vkCreateDisplayModeKHR ) vkGetInstanceProcAddr( instance, "vkCreateDisplayModeKHR" );

	vkGetDisplayPlaneCapabilitiesKHR = ( PFN_vkGetDisplayPlaneCapabilitiesKHR ) vkGetInstanceProcAddr( instance, "vkGetDisplayPlaneCapabilitiesKHR" );

	vkCreateDisplayPlaneSurfaceKHR = ( PFN_vkCreateDisplayPlaneSurfaceKHR ) vkGetInstanceProcAddr( instance, "vkCreateDisplayPlaneSurfaceKHR" );

	vkGetPhysicalDeviceVideoCapabilitiesKHR = ( PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceVideoCapabilitiesKHR" );

	vkGetPhysicalDeviceVideoFormatPropertiesKHR = ( PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceVideoFormatPropertiesKHR" );

	vkGetPhysicalDeviceFeatures2KHR = ( PFN_vkGetPhysicalDeviceFeatures2KHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFeatures2KHR" );

	vkGetPhysicalDeviceProperties2KHR = ( PFN_vkGetPhysicalDeviceProperties2KHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceProperties2KHR" );

	vkGetPhysicalDeviceFormatProperties2KHR = ( PFN_vkGetPhysicalDeviceFormatProperties2KHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFormatProperties2KHR" );

	vkGetPhysicalDeviceImageFormatProperties2KHR = ( PFN_vkGetPhysicalDeviceImageFormatProperties2KHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceImageFormatProperties2KHR" );

	vkGetPhysicalDeviceQueueFamilyProperties2KHR = ( PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceQueueFamilyProperties2KHR" );

	vkGetPhysicalDeviceMemoryProperties2KHR = ( PFN_vkGetPhysicalDeviceMemoryProperties2KHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceMemoryProperties2KHR" );

	vkGetPhysicalDeviceSparseImageFormatProperties2KHR = ( PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSparseImageFormatProperties2KHR" );

	vkEnumeratePhysicalDeviceGroupsKHR = ( PFN_vkEnumeratePhysicalDeviceGroupsKHR ) vkGetInstanceProcAddr( instance, "vkEnumeratePhysicalDeviceGroupsKHR" );

	vkGetPhysicalDeviceExternalBufferPropertiesKHR = ( PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalBufferPropertiesKHR" );

	vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = ( PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR" );

	vkGetPhysicalDeviceExternalFencePropertiesKHR = ( PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalFencePropertiesKHR" );

	vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR = ( PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR ) vkGetInstanceProcAddr( instance, "vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR" );

	vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR = ( PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR" );

	vkGetPhysicalDeviceSurfaceCapabilities2KHR = ( PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceCapabilities2KHR" );

	vkGetPhysicalDeviceSurfaceFormats2KHR = ( PFN_vkGetPhysicalDeviceSurfaceFormats2KHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceFormats2KHR" );

	vkGetPhysicalDeviceDisplayProperties2KHR = ( PFN_vkGetPhysicalDeviceDisplayProperties2KHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceDisplayProperties2KHR" );

	vkGetPhysicalDeviceDisplayPlaneProperties2KHR = ( PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceDisplayPlaneProperties2KHR" );

	vkGetDisplayModeProperties2KHR = ( PFN_vkGetDisplayModeProperties2KHR ) vkGetInstanceProcAddr( instance, "vkGetDisplayModeProperties2KHR" );

	vkGetDisplayPlaneCapabilities2KHR = ( PFN_vkGetDisplayPlaneCapabilities2KHR ) vkGetInstanceProcAddr( instance, "vkGetDisplayPlaneCapabilities2KHR" );

	vkGetPhysicalDeviceFragmentShadingRatesKHR = ( PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFragmentShadingRatesKHR" );

	vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR = ( PFN_vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR" );

	vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR = ( PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR" );

	vkGetPhysicalDeviceCalibrateableTimeDomainsKHR = ( PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceCalibrateableTimeDomainsKHR" );

	vkCreateDebugReportCallbackEXT = ( PFN_vkCreateDebugReportCallbackEXT ) vkGetInstanceProcAddr( instance, "vkCreateDebugReportCallbackEXT" );

	vkDestroyDebugReportCallbackEXT = ( PFN_vkDestroyDebugReportCallbackEXT ) vkGetInstanceProcAddr( instance, "vkDestroyDebugReportCallbackEXT" );

	vkDebugReportMessageEXT = ( PFN_vkDebugReportMessageEXT ) vkGetInstanceProcAddr( instance, "vkDebugReportMessageEXT" );

	vkGetPhysicalDeviceExternalImageFormatPropertiesNV = ( PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV" );

	vkReleaseDisplayEXT = ( PFN_vkReleaseDisplayEXT ) vkGetInstanceProcAddr( instance, "vkReleaseDisplayEXT" );

	vkGetPhysicalDeviceSurfaceCapabilities2EXT = ( PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceCapabilities2EXT" );

	vkCreateDebugUtilsMessengerEXT = ( PFN_vkCreateDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );

	vkDestroyDebugUtilsMessengerEXT = ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );

	vkSubmitDebugUtilsMessageEXT = ( PFN_vkSubmitDebugUtilsMessageEXT ) vkGetInstanceProcAddr( instance, "vkSubmitDebugUtilsMessageEXT" );

	vkGetPhysicalDeviceMultisamplePropertiesEXT = ( PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceMultisamplePropertiesEXT" );

	vkGetPhysicalDeviceCalibrateableTimeDomainsEXT = ( PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT" );

	vkGetPhysicalDeviceToolPropertiesEXT = ( PFN_vkGetPhysicalDeviceToolPropertiesEXT ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceToolPropertiesEXT" );

	vkGetPhysicalDeviceCooperativeMatrixPropertiesNV = ( PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceCooperativeMatrixPropertiesNV" );

	vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV = ( PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV" );

	vkCreateHeadlessSurfaceEXT = ( PFN_vkCreateHeadlessSurfaceEXT ) vkGetInstanceProcAddr( instance, "vkCreateHeadlessSurfaceEXT" );

	vkAcquireDrmDisplayEXT = ( PFN_vkAcquireDrmDisplayEXT ) vkGetInstanceProcAddr( instance, "vkAcquireDrmDisplayEXT" );

	vkGetDrmDisplayEXT = ( PFN_vkGetDrmDisplayEXT ) vkGetInstanceProcAddr( instance, "vkGetDrmDisplayEXT" );

	vkGetPhysicalDeviceExternalTensorPropertiesARM = ( PFN_vkGetPhysicalDeviceExternalTensorPropertiesARM ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalTensorPropertiesARM" );

	vkGetPhysicalDeviceOpticalFlowImageFormatsNV = ( PFN_vkGetPhysicalDeviceOpticalFlowImageFormatsNV ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceOpticalFlowImageFormatsNV" );

	vkGetPhysicalDeviceCooperativeVectorPropertiesNV = ( PFN_vkGetPhysicalDeviceCooperativeVectorPropertiesNV ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceCooperativeVectorPropertiesNV" );

	vkGetPhysicalDeviceQueueFamilyDataGraphPropertiesARM = ( PFN_vkGetPhysicalDeviceQueueFamilyDataGraphPropertiesARM ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceQueueFamilyDataGraphPropertiesARM" );

	vkGetPhysicalDeviceQueueFamilyDataGraphProcessingEnginePropertiesARM = ( PFN_vkGetPhysicalDeviceQueueFamilyDataGraphProcessingEnginePropertiesARM ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceQueueFamilyDataGraphProcessingEnginePropertiesARM" );

	vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV = ( PFN_vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV" );

#if defined( VK_ENABLE_BETA_EXTENSIONS )
#endif

#if defined( VK_USE_PLATFORM_WIN32_KHR )
	vkCreateWin32SurfaceKHR = ( PFN_vkCreateWin32SurfaceKHR ) vkGetInstanceProcAddr( instance, "vkCreateWin32SurfaceKHR" );

	vkGetPhysicalDeviceWin32PresentationSupportKHR = ( PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR" );

	vkGetPhysicalDeviceSurfacePresentModes2EXT = ( PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfacePresentModes2EXT" );

	vkAcquireWinrtDisplayNV = ( PFN_vkAcquireWinrtDisplayNV ) vkGetInstanceProcAddr( instance, "vkAcquireWinrtDisplayNV" );

	vkGetWinrtDisplayNV = ( PFN_vkGetWinrtDisplayNV ) vkGetInstanceProcAddr( instance, "vkGetWinrtDisplayNV" );

#endif

#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
	vkCreateWaylandSurfaceKHR = ( PFN_vkCreateWaylandSurfaceKHR ) vkGetInstanceProcAddr( instance, "vkCreateWaylandSurfaceKHR" );

	vkGetPhysicalDeviceWaylandPresentationSupportKHR = ( PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR" );

#endif

#if defined( VK_USE_PLATFORM_XLIB_KHR )
	vkCreateXlibSurfaceKHR = ( PFN_vkCreateXlibSurfaceKHR ) vkGetInstanceProcAddr( instance, "vkCreateXlibSurfaceKHR" );

	vkGetPhysicalDeviceXlibPresentationSupportKHR = ( PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR ) vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR" );

#endif

#if defined( VK_USE_PLATFORM_XLIB_XRANDR_EXT )
	vkAcquireXlibDisplayEXT = ( PFN_vkAcquireXlibDisplayEXT ) vkGetInstanceProcAddr( instance, "vkAcquireXlibDisplayEXT" );

	vkGetRandROutputDisplayEXT = ( PFN_vkGetRandROutputDisplayEXT ) vkGetInstanceProcAddr( instance, "vkGetRandROutputDisplayEXT" );

#endif

}

void VulkanLoadDeviceFunctions( VkDevice device ) {
	vkDestroyDevice = ( PFN_vkDestroyDevice ) vkGetDeviceProcAddr( device, "vkDestroyDevice" );

	vkGetDeviceQueue = ( PFN_vkGetDeviceQueue ) vkGetDeviceProcAddr( device, "vkGetDeviceQueue" );

	vkQueueSubmit = ( PFN_vkQueueSubmit ) vkGetDeviceProcAddr( device, "vkQueueSubmit" );

	vkQueueWaitIdle = ( PFN_vkQueueWaitIdle ) vkGetDeviceProcAddr( device, "vkQueueWaitIdle" );

	vkDeviceWaitIdle = ( PFN_vkDeviceWaitIdle ) vkGetDeviceProcAddr( device, "vkDeviceWaitIdle" );

	vkAllocateMemory = ( PFN_vkAllocateMemory ) vkGetDeviceProcAddr( device, "vkAllocateMemory" );

	vkFreeMemory = ( PFN_vkFreeMemory ) vkGetDeviceProcAddr( device, "vkFreeMemory" );

	vkMapMemory = ( PFN_vkMapMemory ) vkGetDeviceProcAddr( device, "vkMapMemory" );

	vkUnmapMemory = ( PFN_vkUnmapMemory ) vkGetDeviceProcAddr( device, "vkUnmapMemory" );

	vkFlushMappedMemoryRanges = ( PFN_vkFlushMappedMemoryRanges ) vkGetDeviceProcAddr( device, "vkFlushMappedMemoryRanges" );

	vkInvalidateMappedMemoryRanges = ( PFN_vkInvalidateMappedMemoryRanges ) vkGetDeviceProcAddr( device, "vkInvalidateMappedMemoryRanges" );

	vkGetDeviceMemoryCommitment = ( PFN_vkGetDeviceMemoryCommitment ) vkGetDeviceProcAddr( device, "vkGetDeviceMemoryCommitment" );

	vkBindBufferMemory = ( PFN_vkBindBufferMemory ) vkGetDeviceProcAddr( device, "vkBindBufferMemory" );

	vkBindImageMemory = ( PFN_vkBindImageMemory ) vkGetDeviceProcAddr( device, "vkBindImageMemory" );

	vkGetBufferMemoryRequirements = ( PFN_vkGetBufferMemoryRequirements ) vkGetDeviceProcAddr( device, "vkGetBufferMemoryRequirements" );

	vkGetImageMemoryRequirements = ( PFN_vkGetImageMemoryRequirements ) vkGetDeviceProcAddr( device, "vkGetImageMemoryRequirements" );

	vkGetImageSparseMemoryRequirements = ( PFN_vkGetImageSparseMemoryRequirements ) vkGetDeviceProcAddr( device, "vkGetImageSparseMemoryRequirements" );

	vkQueueBindSparse = ( PFN_vkQueueBindSparse ) vkGetDeviceProcAddr( device, "vkQueueBindSparse" );

	vkCreateFence = ( PFN_vkCreateFence ) vkGetDeviceProcAddr( device, "vkCreateFence" );

	vkDestroyFence = ( PFN_vkDestroyFence ) vkGetDeviceProcAddr( device, "vkDestroyFence" );

	vkResetFences = ( PFN_vkResetFences ) vkGetDeviceProcAddr( device, "vkResetFences" );

	vkGetFenceStatus = ( PFN_vkGetFenceStatus ) vkGetDeviceProcAddr( device, "vkGetFenceStatus" );

	vkWaitForFences = ( PFN_vkWaitForFences ) vkGetDeviceProcAddr( device, "vkWaitForFences" );

	vkCreateSemaphore = ( PFN_vkCreateSemaphore ) vkGetDeviceProcAddr( device, "vkCreateSemaphore" );

	vkDestroySemaphore = ( PFN_vkDestroySemaphore ) vkGetDeviceProcAddr( device, "vkDestroySemaphore" );

	vkCreateEvent = ( PFN_vkCreateEvent ) vkGetDeviceProcAddr( device, "vkCreateEvent" );

	vkDestroyEvent = ( PFN_vkDestroyEvent ) vkGetDeviceProcAddr( device, "vkDestroyEvent" );

	vkGetEventStatus = ( PFN_vkGetEventStatus ) vkGetDeviceProcAddr( device, "vkGetEventStatus" );

	vkSetEvent = ( PFN_vkSetEvent ) vkGetDeviceProcAddr( device, "vkSetEvent" );

	vkResetEvent = ( PFN_vkResetEvent ) vkGetDeviceProcAddr( device, "vkResetEvent" );

	vkCreateQueryPool = ( PFN_vkCreateQueryPool ) vkGetDeviceProcAddr( device, "vkCreateQueryPool" );

	vkDestroyQueryPool = ( PFN_vkDestroyQueryPool ) vkGetDeviceProcAddr( device, "vkDestroyQueryPool" );

	vkGetQueryPoolResults = ( PFN_vkGetQueryPoolResults ) vkGetDeviceProcAddr( device, "vkGetQueryPoolResults" );

	vkCreateBuffer = ( PFN_vkCreateBuffer ) vkGetDeviceProcAddr( device, "vkCreateBuffer" );

	vkDestroyBuffer = ( PFN_vkDestroyBuffer ) vkGetDeviceProcAddr( device, "vkDestroyBuffer" );

	vkCreateBufferView = ( PFN_vkCreateBufferView ) vkGetDeviceProcAddr( device, "vkCreateBufferView" );

	vkDestroyBufferView = ( PFN_vkDestroyBufferView ) vkGetDeviceProcAddr( device, "vkDestroyBufferView" );

	vkCreateImage = ( PFN_vkCreateImage ) vkGetDeviceProcAddr( device, "vkCreateImage" );

	vkDestroyImage = ( PFN_vkDestroyImage ) vkGetDeviceProcAddr( device, "vkDestroyImage" );

	vkGetImageSubresourceLayout = ( PFN_vkGetImageSubresourceLayout ) vkGetDeviceProcAddr( device, "vkGetImageSubresourceLayout" );

	vkCreateImageView = ( PFN_vkCreateImageView ) vkGetDeviceProcAddr( device, "vkCreateImageView" );

	vkDestroyImageView = ( PFN_vkDestroyImageView ) vkGetDeviceProcAddr( device, "vkDestroyImageView" );

	vkCreateShaderModule = ( PFN_vkCreateShaderModule ) vkGetDeviceProcAddr( device, "vkCreateShaderModule" );

	vkDestroyShaderModule = ( PFN_vkDestroyShaderModule ) vkGetDeviceProcAddr( device, "vkDestroyShaderModule" );

	vkCreatePipelineCache = ( PFN_vkCreatePipelineCache ) vkGetDeviceProcAddr( device, "vkCreatePipelineCache" );

	vkDestroyPipelineCache = ( PFN_vkDestroyPipelineCache ) vkGetDeviceProcAddr( device, "vkDestroyPipelineCache" );

	vkGetPipelineCacheData = ( PFN_vkGetPipelineCacheData ) vkGetDeviceProcAddr( device, "vkGetPipelineCacheData" );

	vkMergePipelineCaches = ( PFN_vkMergePipelineCaches ) vkGetDeviceProcAddr( device, "vkMergePipelineCaches" );

	vkCreateGraphicsPipelines = ( PFN_vkCreateGraphicsPipelines ) vkGetDeviceProcAddr( device, "vkCreateGraphicsPipelines" );

	vkCreateComputePipelines = ( PFN_vkCreateComputePipelines ) vkGetDeviceProcAddr( device, "vkCreateComputePipelines" );

	vkDestroyPipeline = ( PFN_vkDestroyPipeline ) vkGetDeviceProcAddr( device, "vkDestroyPipeline" );

	vkCreatePipelineLayout = ( PFN_vkCreatePipelineLayout ) vkGetDeviceProcAddr( device, "vkCreatePipelineLayout" );

	vkDestroyPipelineLayout = ( PFN_vkDestroyPipelineLayout ) vkGetDeviceProcAddr( device, "vkDestroyPipelineLayout" );

	vkCreateSampler = ( PFN_vkCreateSampler ) vkGetDeviceProcAddr( device, "vkCreateSampler" );

	vkDestroySampler = ( PFN_vkDestroySampler ) vkGetDeviceProcAddr( device, "vkDestroySampler" );

	vkCreateDescriptorSetLayout = ( PFN_vkCreateDescriptorSetLayout ) vkGetDeviceProcAddr( device, "vkCreateDescriptorSetLayout" );

	vkDestroyDescriptorSetLayout = ( PFN_vkDestroyDescriptorSetLayout ) vkGetDeviceProcAddr( device, "vkDestroyDescriptorSetLayout" );

	vkCreateDescriptorPool = ( PFN_vkCreateDescriptorPool ) vkGetDeviceProcAddr( device, "vkCreateDescriptorPool" );

	vkDestroyDescriptorPool = ( PFN_vkDestroyDescriptorPool ) vkGetDeviceProcAddr( device, "vkDestroyDescriptorPool" );

	vkResetDescriptorPool = ( PFN_vkResetDescriptorPool ) vkGetDeviceProcAddr( device, "vkResetDescriptorPool" );

	vkAllocateDescriptorSets = ( PFN_vkAllocateDescriptorSets ) vkGetDeviceProcAddr( device, "vkAllocateDescriptorSets" );

	vkFreeDescriptorSets = ( PFN_vkFreeDescriptorSets ) vkGetDeviceProcAddr( device, "vkFreeDescriptorSets" );

	vkUpdateDescriptorSets = ( PFN_vkUpdateDescriptorSets ) vkGetDeviceProcAddr( device, "vkUpdateDescriptorSets" );

	vkCreateFramebuffer = ( PFN_vkCreateFramebuffer ) vkGetDeviceProcAddr( device, "vkCreateFramebuffer" );

	vkDestroyFramebuffer = ( PFN_vkDestroyFramebuffer ) vkGetDeviceProcAddr( device, "vkDestroyFramebuffer" );

	vkCreateRenderPass = ( PFN_vkCreateRenderPass ) vkGetDeviceProcAddr( device, "vkCreateRenderPass" );

	vkDestroyRenderPass = ( PFN_vkDestroyRenderPass ) vkGetDeviceProcAddr( device, "vkDestroyRenderPass" );

	vkGetRenderAreaGranularity = ( PFN_vkGetRenderAreaGranularity ) vkGetDeviceProcAddr( device, "vkGetRenderAreaGranularity" );

	vkCreateCommandPool = ( PFN_vkCreateCommandPool ) vkGetDeviceProcAddr( device, "vkCreateCommandPool" );

	vkDestroyCommandPool = ( PFN_vkDestroyCommandPool ) vkGetDeviceProcAddr( device, "vkDestroyCommandPool" );

	vkResetCommandPool = ( PFN_vkResetCommandPool ) vkGetDeviceProcAddr( device, "vkResetCommandPool" );

	vkAllocateCommandBuffers = ( PFN_vkAllocateCommandBuffers ) vkGetDeviceProcAddr( device, "vkAllocateCommandBuffers" );

	vkFreeCommandBuffers = ( PFN_vkFreeCommandBuffers ) vkGetDeviceProcAddr( device, "vkFreeCommandBuffers" );

	vkBeginCommandBuffer = ( PFN_vkBeginCommandBuffer ) vkGetDeviceProcAddr( device, "vkBeginCommandBuffer" );

	vkEndCommandBuffer = ( PFN_vkEndCommandBuffer ) vkGetDeviceProcAddr( device, "vkEndCommandBuffer" );

	vkResetCommandBuffer = ( PFN_vkResetCommandBuffer ) vkGetDeviceProcAddr( device, "vkResetCommandBuffer" );

	vkCmdBindPipeline = ( PFN_vkCmdBindPipeline ) vkGetDeviceProcAddr( device, "vkCmdBindPipeline" );

	vkCmdSetViewport = ( PFN_vkCmdSetViewport ) vkGetDeviceProcAddr( device, "vkCmdSetViewport" );

	vkCmdSetScissor = ( PFN_vkCmdSetScissor ) vkGetDeviceProcAddr( device, "vkCmdSetScissor" );

	vkCmdSetLineWidth = ( PFN_vkCmdSetLineWidth ) vkGetDeviceProcAddr( device, "vkCmdSetLineWidth" );

	vkCmdSetDepthBias = ( PFN_vkCmdSetDepthBias ) vkGetDeviceProcAddr( device, "vkCmdSetDepthBias" );

	vkCmdSetBlendConstants = ( PFN_vkCmdSetBlendConstants ) vkGetDeviceProcAddr( device, "vkCmdSetBlendConstants" );

	vkCmdSetDepthBounds = ( PFN_vkCmdSetDepthBounds ) vkGetDeviceProcAddr( device, "vkCmdSetDepthBounds" );

	vkCmdSetStencilCompareMask = ( PFN_vkCmdSetStencilCompareMask ) vkGetDeviceProcAddr( device, "vkCmdSetStencilCompareMask" );

	vkCmdSetStencilWriteMask = ( PFN_vkCmdSetStencilWriteMask ) vkGetDeviceProcAddr( device, "vkCmdSetStencilWriteMask" );

	vkCmdSetStencilReference = ( PFN_vkCmdSetStencilReference ) vkGetDeviceProcAddr( device, "vkCmdSetStencilReference" );

	vkCmdBindDescriptorSets = ( PFN_vkCmdBindDescriptorSets ) vkGetDeviceProcAddr( device, "vkCmdBindDescriptorSets" );

	vkCmdBindIndexBuffer = ( PFN_vkCmdBindIndexBuffer ) vkGetDeviceProcAddr( device, "vkCmdBindIndexBuffer" );

	vkCmdBindVertexBuffers = ( PFN_vkCmdBindVertexBuffers ) vkGetDeviceProcAddr( device, "vkCmdBindVertexBuffers" );

	vkCmdDraw = ( PFN_vkCmdDraw ) vkGetDeviceProcAddr( device, "vkCmdDraw" );

	vkCmdDrawIndexed = ( PFN_vkCmdDrawIndexed ) vkGetDeviceProcAddr( device, "vkCmdDrawIndexed" );

	vkCmdDrawIndirect = ( PFN_vkCmdDrawIndirect ) vkGetDeviceProcAddr( device, "vkCmdDrawIndirect" );

	vkCmdDrawIndexedIndirect = ( PFN_vkCmdDrawIndexedIndirect ) vkGetDeviceProcAddr( device, "vkCmdDrawIndexedIndirect" );

	vkCmdDispatch = ( PFN_vkCmdDispatch ) vkGetDeviceProcAddr( device, "vkCmdDispatch" );

	vkCmdDispatchIndirect = ( PFN_vkCmdDispatchIndirect ) vkGetDeviceProcAddr( device, "vkCmdDispatchIndirect" );

	vkCmdCopyBuffer = ( PFN_vkCmdCopyBuffer ) vkGetDeviceProcAddr( device, "vkCmdCopyBuffer" );

	vkCmdCopyImage = ( PFN_vkCmdCopyImage ) vkGetDeviceProcAddr( device, "vkCmdCopyImage" );

	vkCmdBlitImage = ( PFN_vkCmdBlitImage ) vkGetDeviceProcAddr( device, "vkCmdBlitImage" );

	vkCmdCopyBufferToImage = ( PFN_vkCmdCopyBufferToImage ) vkGetDeviceProcAddr( device, "vkCmdCopyBufferToImage" );

	vkCmdCopyImageToBuffer = ( PFN_vkCmdCopyImageToBuffer ) vkGetDeviceProcAddr( device, "vkCmdCopyImageToBuffer" );

	vkCmdUpdateBuffer = ( PFN_vkCmdUpdateBuffer ) vkGetDeviceProcAddr( device, "vkCmdUpdateBuffer" );

	vkCmdFillBuffer = ( PFN_vkCmdFillBuffer ) vkGetDeviceProcAddr( device, "vkCmdFillBuffer" );

	vkCmdClearColorImage = ( PFN_vkCmdClearColorImage ) vkGetDeviceProcAddr( device, "vkCmdClearColorImage" );

	vkCmdClearDepthStencilImage = ( PFN_vkCmdClearDepthStencilImage ) vkGetDeviceProcAddr( device, "vkCmdClearDepthStencilImage" );

	vkCmdClearAttachments = ( PFN_vkCmdClearAttachments ) vkGetDeviceProcAddr( device, "vkCmdClearAttachments" );

	vkCmdResolveImage = ( PFN_vkCmdResolveImage ) vkGetDeviceProcAddr( device, "vkCmdResolveImage" );

	vkCmdSetEvent = ( PFN_vkCmdSetEvent ) vkGetDeviceProcAddr( device, "vkCmdSetEvent" );

	vkCmdResetEvent = ( PFN_vkCmdResetEvent ) vkGetDeviceProcAddr( device, "vkCmdResetEvent" );

	vkCmdWaitEvents = ( PFN_vkCmdWaitEvents ) vkGetDeviceProcAddr( device, "vkCmdWaitEvents" );

	vkCmdPipelineBarrier = ( PFN_vkCmdPipelineBarrier ) vkGetDeviceProcAddr( device, "vkCmdPipelineBarrier" );

	vkCmdBeginQuery = ( PFN_vkCmdBeginQuery ) vkGetDeviceProcAddr( device, "vkCmdBeginQuery" );

	vkCmdEndQuery = ( PFN_vkCmdEndQuery ) vkGetDeviceProcAddr( device, "vkCmdEndQuery" );

	vkCmdResetQueryPool = ( PFN_vkCmdResetQueryPool ) vkGetDeviceProcAddr( device, "vkCmdResetQueryPool" );

	vkCmdWriteTimestamp = ( PFN_vkCmdWriteTimestamp ) vkGetDeviceProcAddr( device, "vkCmdWriteTimestamp" );

	vkCmdCopyQueryPoolResults = ( PFN_vkCmdCopyQueryPoolResults ) vkGetDeviceProcAddr( device, "vkCmdCopyQueryPoolResults" );

	vkCmdPushConstants = ( PFN_vkCmdPushConstants ) vkGetDeviceProcAddr( device, "vkCmdPushConstants" );

	vkCmdBeginRenderPass = ( PFN_vkCmdBeginRenderPass ) vkGetDeviceProcAddr( device, "vkCmdBeginRenderPass" );

	vkCmdNextSubpass = ( PFN_vkCmdNextSubpass ) vkGetDeviceProcAddr( device, "vkCmdNextSubpass" );

	vkCmdEndRenderPass = ( PFN_vkCmdEndRenderPass ) vkGetDeviceProcAddr( device, "vkCmdEndRenderPass" );

	vkCmdExecuteCommands = ( PFN_vkCmdExecuteCommands ) vkGetDeviceProcAddr( device, "vkCmdExecuteCommands" );

	vkBindBufferMemory2 = ( PFN_vkBindBufferMemory2 ) vkGetDeviceProcAddr( device, "vkBindBufferMemory2" );

	vkBindImageMemory2 = ( PFN_vkBindImageMemory2 ) vkGetDeviceProcAddr( device, "vkBindImageMemory2" );

	vkGetDeviceGroupPeerMemoryFeatures = ( PFN_vkGetDeviceGroupPeerMemoryFeatures ) vkGetDeviceProcAddr( device, "vkGetDeviceGroupPeerMemoryFeatures" );

	vkCmdSetDeviceMask = ( PFN_vkCmdSetDeviceMask ) vkGetDeviceProcAddr( device, "vkCmdSetDeviceMask" );

	vkCmdDispatchBase = ( PFN_vkCmdDispatchBase ) vkGetDeviceProcAddr( device, "vkCmdDispatchBase" );

	vkGetImageMemoryRequirements2 = ( PFN_vkGetImageMemoryRequirements2 ) vkGetDeviceProcAddr( device, "vkGetImageMemoryRequirements2" );

	vkGetBufferMemoryRequirements2 = ( PFN_vkGetBufferMemoryRequirements2 ) vkGetDeviceProcAddr( device, "vkGetBufferMemoryRequirements2" );

	vkGetImageSparseMemoryRequirements2 = ( PFN_vkGetImageSparseMemoryRequirements2 ) vkGetDeviceProcAddr( device, "vkGetImageSparseMemoryRequirements2" );

	vkTrimCommandPool = ( PFN_vkTrimCommandPool ) vkGetDeviceProcAddr( device, "vkTrimCommandPool" );

	vkGetDeviceQueue2 = ( PFN_vkGetDeviceQueue2 ) vkGetDeviceProcAddr( device, "vkGetDeviceQueue2" );

	vkCreateSamplerYcbcrConversion = ( PFN_vkCreateSamplerYcbcrConversion ) vkGetDeviceProcAddr( device, "vkCreateSamplerYcbcrConversion" );

	vkDestroySamplerYcbcrConversion = ( PFN_vkDestroySamplerYcbcrConversion ) vkGetDeviceProcAddr( device, "vkDestroySamplerYcbcrConversion" );

	vkCreateDescriptorUpdateTemplate = ( PFN_vkCreateDescriptorUpdateTemplate ) vkGetDeviceProcAddr( device, "vkCreateDescriptorUpdateTemplate" );

	vkDestroyDescriptorUpdateTemplate = ( PFN_vkDestroyDescriptorUpdateTemplate ) vkGetDeviceProcAddr( device, "vkDestroyDescriptorUpdateTemplate" );

	vkUpdateDescriptorSetWithTemplate = ( PFN_vkUpdateDescriptorSetWithTemplate ) vkGetDeviceProcAddr( device, "vkUpdateDescriptorSetWithTemplate" );

	vkGetDescriptorSetLayoutSupport = ( PFN_vkGetDescriptorSetLayoutSupport ) vkGetDeviceProcAddr( device, "vkGetDescriptorSetLayoutSupport" );

	vkCmdDrawIndirectCount = ( PFN_vkCmdDrawIndirectCount ) vkGetDeviceProcAddr( device, "vkCmdDrawIndirectCount" );

	vkCmdDrawIndexedIndirectCount = ( PFN_vkCmdDrawIndexedIndirectCount ) vkGetDeviceProcAddr( device, "vkCmdDrawIndexedIndirectCount" );

	vkCreateRenderPass2 = ( PFN_vkCreateRenderPass2 ) vkGetDeviceProcAddr( device, "vkCreateRenderPass2" );

	vkCmdBeginRenderPass2 = ( PFN_vkCmdBeginRenderPass2 ) vkGetDeviceProcAddr( device, "vkCmdBeginRenderPass2" );

	vkCmdNextSubpass2 = ( PFN_vkCmdNextSubpass2 ) vkGetDeviceProcAddr( device, "vkCmdNextSubpass2" );

	vkCmdEndRenderPass2 = ( PFN_vkCmdEndRenderPass2 ) vkGetDeviceProcAddr( device, "vkCmdEndRenderPass2" );

	vkResetQueryPool = ( PFN_vkResetQueryPool ) vkGetDeviceProcAddr( device, "vkResetQueryPool" );

	vkGetSemaphoreCounterValue = ( PFN_vkGetSemaphoreCounterValue ) vkGetDeviceProcAddr( device, "vkGetSemaphoreCounterValue" );

	vkWaitSemaphores = ( PFN_vkWaitSemaphores ) vkGetDeviceProcAddr( device, "vkWaitSemaphores" );

	vkSignalSemaphore = ( PFN_vkSignalSemaphore ) vkGetDeviceProcAddr( device, "vkSignalSemaphore" );

	vkGetBufferDeviceAddress = ( PFN_vkGetBufferDeviceAddress ) vkGetDeviceProcAddr( device, "vkGetBufferDeviceAddress" );

	vkGetBufferOpaqueCaptureAddress = ( PFN_vkGetBufferOpaqueCaptureAddress ) vkGetDeviceProcAddr( device, "vkGetBufferOpaqueCaptureAddress" );

	vkGetDeviceMemoryOpaqueCaptureAddress = ( PFN_vkGetDeviceMemoryOpaqueCaptureAddress ) vkGetDeviceProcAddr( device, "vkGetDeviceMemoryOpaqueCaptureAddress" );

	vkCreatePrivateDataSlot = ( PFN_vkCreatePrivateDataSlot ) vkGetDeviceProcAddr( device, "vkCreatePrivateDataSlot" );

	vkDestroyPrivateDataSlot = ( PFN_vkDestroyPrivateDataSlot ) vkGetDeviceProcAddr( device, "vkDestroyPrivateDataSlot" );

	vkSetPrivateData = ( PFN_vkSetPrivateData ) vkGetDeviceProcAddr( device, "vkSetPrivateData" );

	vkGetPrivateData = ( PFN_vkGetPrivateData ) vkGetDeviceProcAddr( device, "vkGetPrivateData" );

	vkCmdSetEvent2 = ( PFN_vkCmdSetEvent2 ) vkGetDeviceProcAddr( device, "vkCmdSetEvent2" );

	vkCmdResetEvent2 = ( PFN_vkCmdResetEvent2 ) vkGetDeviceProcAddr( device, "vkCmdResetEvent2" );

	vkCmdWaitEvents2 = ( PFN_vkCmdWaitEvents2 ) vkGetDeviceProcAddr( device, "vkCmdWaitEvents2" );

	vkCmdPipelineBarrier2 = ( PFN_vkCmdPipelineBarrier2 ) vkGetDeviceProcAddr( device, "vkCmdPipelineBarrier2" );

	vkCmdWriteTimestamp2 = ( PFN_vkCmdWriteTimestamp2 ) vkGetDeviceProcAddr( device, "vkCmdWriteTimestamp2" );

	vkQueueSubmit2 = ( PFN_vkQueueSubmit2 ) vkGetDeviceProcAddr( device, "vkQueueSubmit2" );

	vkCmdCopyBuffer2 = ( PFN_vkCmdCopyBuffer2 ) vkGetDeviceProcAddr( device, "vkCmdCopyBuffer2" );

	vkCmdCopyImage2 = ( PFN_vkCmdCopyImage2 ) vkGetDeviceProcAddr( device, "vkCmdCopyImage2" );

	vkCmdCopyBufferToImage2 = ( PFN_vkCmdCopyBufferToImage2 ) vkGetDeviceProcAddr( device, "vkCmdCopyBufferToImage2" );

	vkCmdCopyImageToBuffer2 = ( PFN_vkCmdCopyImageToBuffer2 ) vkGetDeviceProcAddr( device, "vkCmdCopyImageToBuffer2" );

	vkCmdBlitImage2 = ( PFN_vkCmdBlitImage2 ) vkGetDeviceProcAddr( device, "vkCmdBlitImage2" );

	vkCmdResolveImage2 = ( PFN_vkCmdResolveImage2 ) vkGetDeviceProcAddr( device, "vkCmdResolveImage2" );

	vkCmdBeginRendering = ( PFN_vkCmdBeginRendering ) vkGetDeviceProcAddr( device, "vkCmdBeginRendering" );

	vkCmdEndRendering = ( PFN_vkCmdEndRendering ) vkGetDeviceProcAddr( device, "vkCmdEndRendering" );

	vkCmdSetCullMode = ( PFN_vkCmdSetCullMode ) vkGetDeviceProcAddr( device, "vkCmdSetCullMode" );

	vkCmdSetFrontFace = ( PFN_vkCmdSetFrontFace ) vkGetDeviceProcAddr( device, "vkCmdSetFrontFace" );

	vkCmdSetPrimitiveTopology = ( PFN_vkCmdSetPrimitiveTopology ) vkGetDeviceProcAddr( device, "vkCmdSetPrimitiveTopology" );

	vkCmdSetViewportWithCount = ( PFN_vkCmdSetViewportWithCount ) vkGetDeviceProcAddr( device, "vkCmdSetViewportWithCount" );

	vkCmdSetScissorWithCount = ( PFN_vkCmdSetScissorWithCount ) vkGetDeviceProcAddr( device, "vkCmdSetScissorWithCount" );

	vkCmdBindVertexBuffers2 = ( PFN_vkCmdBindVertexBuffers2 ) vkGetDeviceProcAddr( device, "vkCmdBindVertexBuffers2" );

	vkCmdSetDepthTestEnable = ( PFN_vkCmdSetDepthTestEnable ) vkGetDeviceProcAddr( device, "vkCmdSetDepthTestEnable" );

	vkCmdSetDepthWriteEnable = ( PFN_vkCmdSetDepthWriteEnable ) vkGetDeviceProcAddr( device, "vkCmdSetDepthWriteEnable" );

	vkCmdSetDepthCompareOp = ( PFN_vkCmdSetDepthCompareOp ) vkGetDeviceProcAddr( device, "vkCmdSetDepthCompareOp" );

	vkCmdSetDepthBoundsTestEnable = ( PFN_vkCmdSetDepthBoundsTestEnable ) vkGetDeviceProcAddr( device, "vkCmdSetDepthBoundsTestEnable" );

	vkCmdSetStencilTestEnable = ( PFN_vkCmdSetStencilTestEnable ) vkGetDeviceProcAddr( device, "vkCmdSetStencilTestEnable" );

	vkCmdSetStencilOp = ( PFN_vkCmdSetStencilOp ) vkGetDeviceProcAddr( device, "vkCmdSetStencilOp" );

	vkCmdSetRasterizerDiscardEnable = ( PFN_vkCmdSetRasterizerDiscardEnable ) vkGetDeviceProcAddr( device, "vkCmdSetRasterizerDiscardEnable" );

	vkCmdSetDepthBiasEnable = ( PFN_vkCmdSetDepthBiasEnable ) vkGetDeviceProcAddr( device, "vkCmdSetDepthBiasEnable" );

	vkCmdSetPrimitiveRestartEnable = ( PFN_vkCmdSetPrimitiveRestartEnable ) vkGetDeviceProcAddr( device, "vkCmdSetPrimitiveRestartEnable" );

	vkGetDeviceBufferMemoryRequirements = ( PFN_vkGetDeviceBufferMemoryRequirements ) vkGetDeviceProcAddr( device, "vkGetDeviceBufferMemoryRequirements" );

	vkGetDeviceImageMemoryRequirements = ( PFN_vkGetDeviceImageMemoryRequirements ) vkGetDeviceProcAddr( device, "vkGetDeviceImageMemoryRequirements" );

	vkGetDeviceImageSparseMemoryRequirements = ( PFN_vkGetDeviceImageSparseMemoryRequirements ) vkGetDeviceProcAddr( device, "vkGetDeviceImageSparseMemoryRequirements" );

	vkCmdSetLineStipple = ( PFN_vkCmdSetLineStipple ) vkGetDeviceProcAddr( device, "vkCmdSetLineStipple" );

	vkMapMemory2 = ( PFN_vkMapMemory2 ) vkGetDeviceProcAddr( device, "vkMapMemory2" );

	vkUnmapMemory2 = ( PFN_vkUnmapMemory2 ) vkGetDeviceProcAddr( device, "vkUnmapMemory2" );

	vkCmdBindIndexBuffer2 = ( PFN_vkCmdBindIndexBuffer2 ) vkGetDeviceProcAddr( device, "vkCmdBindIndexBuffer2" );

	vkGetRenderingAreaGranularity = ( PFN_vkGetRenderingAreaGranularity ) vkGetDeviceProcAddr( device, "vkGetRenderingAreaGranularity" );

	vkGetDeviceImageSubresourceLayout = ( PFN_vkGetDeviceImageSubresourceLayout ) vkGetDeviceProcAddr( device, "vkGetDeviceImageSubresourceLayout" );

	vkGetImageSubresourceLayout2 = ( PFN_vkGetImageSubresourceLayout2 ) vkGetDeviceProcAddr( device, "vkGetImageSubresourceLayout2" );

	vkCmdPushDescriptorSet = ( PFN_vkCmdPushDescriptorSet ) vkGetDeviceProcAddr( device, "vkCmdPushDescriptorSet" );

	vkCmdPushDescriptorSetWithTemplate = ( PFN_vkCmdPushDescriptorSetWithTemplate ) vkGetDeviceProcAddr( device, "vkCmdPushDescriptorSetWithTemplate" );

	vkCmdSetRenderingAttachmentLocations = ( PFN_vkCmdSetRenderingAttachmentLocations ) vkGetDeviceProcAddr( device, "vkCmdSetRenderingAttachmentLocations" );

	vkCmdSetRenderingInputAttachmentIndices = ( PFN_vkCmdSetRenderingInputAttachmentIndices ) vkGetDeviceProcAddr( device, "vkCmdSetRenderingInputAttachmentIndices" );

	vkCmdBindDescriptorSets2 = ( PFN_vkCmdBindDescriptorSets2 ) vkGetDeviceProcAddr( device, "vkCmdBindDescriptorSets2" );

	vkCmdPushConstants2 = ( PFN_vkCmdPushConstants2 ) vkGetDeviceProcAddr( device, "vkCmdPushConstants2" );

	vkCmdPushDescriptorSet2 = ( PFN_vkCmdPushDescriptorSet2 ) vkGetDeviceProcAddr( device, "vkCmdPushDescriptorSet2" );

	vkCmdPushDescriptorSetWithTemplate2 = ( PFN_vkCmdPushDescriptorSetWithTemplate2 ) vkGetDeviceProcAddr( device, "vkCmdPushDescriptorSetWithTemplate2" );

	vkCopyMemoryToImage = ( PFN_vkCopyMemoryToImage ) vkGetDeviceProcAddr( device, "vkCopyMemoryToImage" );

	vkCopyImageToMemory = ( PFN_vkCopyImageToMemory ) vkGetDeviceProcAddr( device, "vkCopyImageToMemory" );

	vkCopyImageToImage = ( PFN_vkCopyImageToImage ) vkGetDeviceProcAddr( device, "vkCopyImageToImage" );

	vkTransitionImageLayout = ( PFN_vkTransitionImageLayout ) vkGetDeviceProcAddr( device, "vkTransitionImageLayout" );

	vkCreateSwapchainKHR = ( PFN_vkCreateSwapchainKHR ) vkGetDeviceProcAddr( device, "vkCreateSwapchainKHR" );

	vkDestroySwapchainKHR = ( PFN_vkDestroySwapchainKHR ) vkGetDeviceProcAddr( device, "vkDestroySwapchainKHR" );

	vkGetSwapchainImagesKHR = ( PFN_vkGetSwapchainImagesKHR ) vkGetDeviceProcAddr( device, "vkGetSwapchainImagesKHR" );

	vkAcquireNextImageKHR = ( PFN_vkAcquireNextImageKHR ) vkGetDeviceProcAddr( device, "vkAcquireNextImageKHR" );

	vkQueuePresentKHR = ( PFN_vkQueuePresentKHR ) vkGetDeviceProcAddr( device, "vkQueuePresentKHR" );

	vkGetDeviceGroupPresentCapabilitiesKHR = ( PFN_vkGetDeviceGroupPresentCapabilitiesKHR ) vkGetDeviceProcAddr( device, "vkGetDeviceGroupPresentCapabilitiesKHR" );

	vkGetDeviceGroupSurfacePresentModesKHR = ( PFN_vkGetDeviceGroupSurfacePresentModesKHR ) vkGetDeviceProcAddr( device, "vkGetDeviceGroupSurfacePresentModesKHR" );

	vkAcquireNextImage2KHR = ( PFN_vkAcquireNextImage2KHR ) vkGetDeviceProcAddr( device, "vkAcquireNextImage2KHR" );

	vkCreateSharedSwapchainsKHR = ( PFN_vkCreateSharedSwapchainsKHR ) vkGetDeviceProcAddr( device, "vkCreateSharedSwapchainsKHR" );

	vkCreateVideoSessionKHR = ( PFN_vkCreateVideoSessionKHR ) vkGetDeviceProcAddr( device, "vkCreateVideoSessionKHR" );

	vkDestroyVideoSessionKHR = ( PFN_vkDestroyVideoSessionKHR ) vkGetDeviceProcAddr( device, "vkDestroyVideoSessionKHR" );

	vkGetVideoSessionMemoryRequirementsKHR = ( PFN_vkGetVideoSessionMemoryRequirementsKHR ) vkGetDeviceProcAddr( device, "vkGetVideoSessionMemoryRequirementsKHR" );

	vkBindVideoSessionMemoryKHR = ( PFN_vkBindVideoSessionMemoryKHR ) vkGetDeviceProcAddr( device, "vkBindVideoSessionMemoryKHR" );

	vkCreateVideoSessionParametersKHR = ( PFN_vkCreateVideoSessionParametersKHR ) vkGetDeviceProcAddr( device, "vkCreateVideoSessionParametersKHR" );

	vkUpdateVideoSessionParametersKHR = ( PFN_vkUpdateVideoSessionParametersKHR ) vkGetDeviceProcAddr( device, "vkUpdateVideoSessionParametersKHR" );

	vkDestroyVideoSessionParametersKHR = ( PFN_vkDestroyVideoSessionParametersKHR ) vkGetDeviceProcAddr( device, "vkDestroyVideoSessionParametersKHR" );

	vkCmdBeginVideoCodingKHR = ( PFN_vkCmdBeginVideoCodingKHR ) vkGetDeviceProcAddr( device, "vkCmdBeginVideoCodingKHR" );

	vkCmdEndVideoCodingKHR = ( PFN_vkCmdEndVideoCodingKHR ) vkGetDeviceProcAddr( device, "vkCmdEndVideoCodingKHR" );

	vkCmdControlVideoCodingKHR = ( PFN_vkCmdControlVideoCodingKHR ) vkGetDeviceProcAddr( device, "vkCmdControlVideoCodingKHR" );

	vkCmdDecodeVideoKHR = ( PFN_vkCmdDecodeVideoKHR ) vkGetDeviceProcAddr( device, "vkCmdDecodeVideoKHR" );

	vkCmdBeginRenderingKHR = ( PFN_vkCmdBeginRenderingKHR ) vkGetDeviceProcAddr( device, "vkCmdBeginRenderingKHR" );

	vkCmdEndRenderingKHR = ( PFN_vkCmdEndRenderingKHR ) vkGetDeviceProcAddr( device, "vkCmdEndRenderingKHR" );

	vkGetDeviceGroupPeerMemoryFeaturesKHR = ( PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR ) vkGetDeviceProcAddr( device, "vkGetDeviceGroupPeerMemoryFeaturesKHR" );

	vkCmdSetDeviceMaskKHR = ( PFN_vkCmdSetDeviceMaskKHR ) vkGetDeviceProcAddr( device, "vkCmdSetDeviceMaskKHR" );

	vkCmdDispatchBaseKHR = ( PFN_vkCmdDispatchBaseKHR ) vkGetDeviceProcAddr( device, "vkCmdDispatchBaseKHR" );

	vkTrimCommandPoolKHR = ( PFN_vkTrimCommandPoolKHR ) vkGetDeviceProcAddr( device, "vkTrimCommandPoolKHR" );

	vkGetMemoryFdKHR = ( PFN_vkGetMemoryFdKHR ) vkGetDeviceProcAddr( device, "vkGetMemoryFdKHR" );

	vkGetMemoryFdPropertiesKHR = ( PFN_vkGetMemoryFdPropertiesKHR ) vkGetDeviceProcAddr( device, "vkGetMemoryFdPropertiesKHR" );

	vkImportSemaphoreFdKHR = ( PFN_vkImportSemaphoreFdKHR ) vkGetDeviceProcAddr( device, "vkImportSemaphoreFdKHR" );

	vkGetSemaphoreFdKHR = ( PFN_vkGetSemaphoreFdKHR ) vkGetDeviceProcAddr( device, "vkGetSemaphoreFdKHR" );

	vkCmdPushDescriptorSetKHR = ( PFN_vkCmdPushDescriptorSetKHR ) vkGetDeviceProcAddr( device, "vkCmdPushDescriptorSetKHR" );

	vkCmdPushDescriptorSetWithTemplateKHR = ( PFN_vkCmdPushDescriptorSetWithTemplateKHR ) vkGetDeviceProcAddr( device, "vkCmdPushDescriptorSetWithTemplateKHR" );

	vkCreateDescriptorUpdateTemplateKHR = ( PFN_vkCreateDescriptorUpdateTemplateKHR ) vkGetDeviceProcAddr( device, "vkCreateDescriptorUpdateTemplateKHR" );

	vkDestroyDescriptorUpdateTemplateKHR = ( PFN_vkDestroyDescriptorUpdateTemplateKHR ) vkGetDeviceProcAddr( device, "vkDestroyDescriptorUpdateTemplateKHR" );

	vkUpdateDescriptorSetWithTemplateKHR = ( PFN_vkUpdateDescriptorSetWithTemplateKHR ) vkGetDeviceProcAddr( device, "vkUpdateDescriptorSetWithTemplateKHR" );

	vkCreateRenderPass2KHR = ( PFN_vkCreateRenderPass2KHR ) vkGetDeviceProcAddr( device, "vkCreateRenderPass2KHR" );

	vkCmdBeginRenderPass2KHR = ( PFN_vkCmdBeginRenderPass2KHR ) vkGetDeviceProcAddr( device, "vkCmdBeginRenderPass2KHR" );

	vkCmdNextSubpass2KHR = ( PFN_vkCmdNextSubpass2KHR ) vkGetDeviceProcAddr( device, "vkCmdNextSubpass2KHR" );

	vkCmdEndRenderPass2KHR = ( PFN_vkCmdEndRenderPass2KHR ) vkGetDeviceProcAddr( device, "vkCmdEndRenderPass2KHR" );

	vkGetSwapchainStatusKHR = ( PFN_vkGetSwapchainStatusKHR ) vkGetDeviceProcAddr( device, "vkGetSwapchainStatusKHR" );

	vkImportFenceFdKHR = ( PFN_vkImportFenceFdKHR ) vkGetDeviceProcAddr( device, "vkImportFenceFdKHR" );

	vkGetFenceFdKHR = ( PFN_vkGetFenceFdKHR ) vkGetDeviceProcAddr( device, "vkGetFenceFdKHR" );

	vkAcquireProfilingLockKHR = ( PFN_vkAcquireProfilingLockKHR ) vkGetDeviceProcAddr( device, "vkAcquireProfilingLockKHR" );

	vkReleaseProfilingLockKHR = ( PFN_vkReleaseProfilingLockKHR ) vkGetDeviceProcAddr( device, "vkReleaseProfilingLockKHR" );

	vkGetImageMemoryRequirements2KHR = ( PFN_vkGetImageMemoryRequirements2KHR ) vkGetDeviceProcAddr( device, "vkGetImageMemoryRequirements2KHR" );

	vkGetBufferMemoryRequirements2KHR = ( PFN_vkGetBufferMemoryRequirements2KHR ) vkGetDeviceProcAddr( device, "vkGetBufferMemoryRequirements2KHR" );

	vkGetImageSparseMemoryRequirements2KHR = ( PFN_vkGetImageSparseMemoryRequirements2KHR ) vkGetDeviceProcAddr( device, "vkGetImageSparseMemoryRequirements2KHR" );

	vkCreateSamplerYcbcrConversionKHR = ( PFN_vkCreateSamplerYcbcrConversionKHR ) vkGetDeviceProcAddr( device, "vkCreateSamplerYcbcrConversionKHR" );

	vkDestroySamplerYcbcrConversionKHR = ( PFN_vkDestroySamplerYcbcrConversionKHR ) vkGetDeviceProcAddr( device, "vkDestroySamplerYcbcrConversionKHR" );

	vkBindBufferMemory2KHR = ( PFN_vkBindBufferMemory2KHR ) vkGetDeviceProcAddr( device, "vkBindBufferMemory2KHR" );

	vkBindImageMemory2KHR = ( PFN_vkBindImageMemory2KHR ) vkGetDeviceProcAddr( device, "vkBindImageMemory2KHR" );

	vkGetDescriptorSetLayoutSupportKHR = ( PFN_vkGetDescriptorSetLayoutSupportKHR ) vkGetDeviceProcAddr( device, "vkGetDescriptorSetLayoutSupportKHR" );

	vkCmdDrawIndirectCountKHR = ( PFN_vkCmdDrawIndirectCountKHR ) vkGetDeviceProcAddr( device, "vkCmdDrawIndirectCountKHR" );

	vkCmdDrawIndexedIndirectCountKHR = ( PFN_vkCmdDrawIndexedIndirectCountKHR ) vkGetDeviceProcAddr( device, "vkCmdDrawIndexedIndirectCountKHR" );

	vkGetSemaphoreCounterValueKHR = ( PFN_vkGetSemaphoreCounterValueKHR ) vkGetDeviceProcAddr( device, "vkGetSemaphoreCounterValueKHR" );

	vkWaitSemaphoresKHR = ( PFN_vkWaitSemaphoresKHR ) vkGetDeviceProcAddr( device, "vkWaitSemaphoresKHR" );

	vkSignalSemaphoreKHR = ( PFN_vkSignalSemaphoreKHR ) vkGetDeviceProcAddr( device, "vkSignalSemaphoreKHR" );

	vkCmdSetFragmentShadingRateKHR = ( PFN_vkCmdSetFragmentShadingRateKHR ) vkGetDeviceProcAddr( device, "vkCmdSetFragmentShadingRateKHR" );

	vkCmdSetRenderingAttachmentLocationsKHR = ( PFN_vkCmdSetRenderingAttachmentLocationsKHR ) vkGetDeviceProcAddr( device, "vkCmdSetRenderingAttachmentLocationsKHR" );

	vkCmdSetRenderingInputAttachmentIndicesKHR = ( PFN_vkCmdSetRenderingInputAttachmentIndicesKHR ) vkGetDeviceProcAddr( device, "vkCmdSetRenderingInputAttachmentIndicesKHR" );

	vkWaitForPresentKHR = ( PFN_vkWaitForPresentKHR ) vkGetDeviceProcAddr( device, "vkWaitForPresentKHR" );

	vkGetBufferDeviceAddressKHR = ( PFN_vkGetBufferDeviceAddressKHR ) vkGetDeviceProcAddr( device, "vkGetBufferDeviceAddressKHR" );

	vkGetBufferOpaqueCaptureAddressKHR = ( PFN_vkGetBufferOpaqueCaptureAddressKHR ) vkGetDeviceProcAddr( device, "vkGetBufferOpaqueCaptureAddressKHR" );

	vkGetDeviceMemoryOpaqueCaptureAddressKHR = ( PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR ) vkGetDeviceProcAddr( device, "vkGetDeviceMemoryOpaqueCaptureAddressKHR" );

	vkCreateDeferredOperationKHR = ( PFN_vkCreateDeferredOperationKHR ) vkGetDeviceProcAddr( device, "vkCreateDeferredOperationKHR" );

	vkDestroyDeferredOperationKHR = ( PFN_vkDestroyDeferredOperationKHR ) vkGetDeviceProcAddr( device, "vkDestroyDeferredOperationKHR" );

	vkGetDeferredOperationMaxConcurrencyKHR = ( PFN_vkGetDeferredOperationMaxConcurrencyKHR ) vkGetDeviceProcAddr( device, "vkGetDeferredOperationMaxConcurrencyKHR" );

	vkGetDeferredOperationResultKHR = ( PFN_vkGetDeferredOperationResultKHR ) vkGetDeviceProcAddr( device, "vkGetDeferredOperationResultKHR" );

	vkDeferredOperationJoinKHR = ( PFN_vkDeferredOperationJoinKHR ) vkGetDeviceProcAddr( device, "vkDeferredOperationJoinKHR" );

	vkGetPipelineExecutablePropertiesKHR = ( PFN_vkGetPipelineExecutablePropertiesKHR ) vkGetDeviceProcAddr( device, "vkGetPipelineExecutablePropertiesKHR" );

	vkGetPipelineExecutableStatisticsKHR = ( PFN_vkGetPipelineExecutableStatisticsKHR ) vkGetDeviceProcAddr( device, "vkGetPipelineExecutableStatisticsKHR" );

	vkGetPipelineExecutableInternalRepresentationsKHR = ( PFN_vkGetPipelineExecutableInternalRepresentationsKHR ) vkGetDeviceProcAddr( device, "vkGetPipelineExecutableInternalRepresentationsKHR" );

	vkMapMemory2KHR = ( PFN_vkMapMemory2KHR ) vkGetDeviceProcAddr( device, "vkMapMemory2KHR" );

	vkUnmapMemory2KHR = ( PFN_vkUnmapMemory2KHR ) vkGetDeviceProcAddr( device, "vkUnmapMemory2KHR" );

	vkGetEncodedVideoSessionParametersKHR = ( PFN_vkGetEncodedVideoSessionParametersKHR ) vkGetDeviceProcAddr( device, "vkGetEncodedVideoSessionParametersKHR" );

	vkCmdEncodeVideoKHR = ( PFN_vkCmdEncodeVideoKHR ) vkGetDeviceProcAddr( device, "vkCmdEncodeVideoKHR" );

	vkCmdSetEvent2KHR = ( PFN_vkCmdSetEvent2KHR ) vkGetDeviceProcAddr( device, "vkCmdSetEvent2KHR" );

	vkCmdResetEvent2KHR = ( PFN_vkCmdResetEvent2KHR ) vkGetDeviceProcAddr( device, "vkCmdResetEvent2KHR" );

	vkCmdWaitEvents2KHR = ( PFN_vkCmdWaitEvents2KHR ) vkGetDeviceProcAddr( device, "vkCmdWaitEvents2KHR" );

	vkCmdPipelineBarrier2KHR = ( PFN_vkCmdPipelineBarrier2KHR ) vkGetDeviceProcAddr( device, "vkCmdPipelineBarrier2KHR" );

	vkCmdWriteTimestamp2KHR = ( PFN_vkCmdWriteTimestamp2KHR ) vkGetDeviceProcAddr( device, "vkCmdWriteTimestamp2KHR" );

	vkQueueSubmit2KHR = ( PFN_vkQueueSubmit2KHR ) vkGetDeviceProcAddr( device, "vkQueueSubmit2KHR" );

	vkCmdCopyBuffer2KHR = ( PFN_vkCmdCopyBuffer2KHR ) vkGetDeviceProcAddr( device, "vkCmdCopyBuffer2KHR" );

	vkCmdCopyImage2KHR = ( PFN_vkCmdCopyImage2KHR ) vkGetDeviceProcAddr( device, "vkCmdCopyImage2KHR" );

	vkCmdCopyBufferToImage2KHR = ( PFN_vkCmdCopyBufferToImage2KHR ) vkGetDeviceProcAddr( device, "vkCmdCopyBufferToImage2KHR" );

	vkCmdCopyImageToBuffer2KHR = ( PFN_vkCmdCopyImageToBuffer2KHR ) vkGetDeviceProcAddr( device, "vkCmdCopyImageToBuffer2KHR" );

	vkCmdBlitImage2KHR = ( PFN_vkCmdBlitImage2KHR ) vkGetDeviceProcAddr( device, "vkCmdBlitImage2KHR" );

	vkCmdResolveImage2KHR = ( PFN_vkCmdResolveImage2KHR ) vkGetDeviceProcAddr( device, "vkCmdResolveImage2KHR" );

	vkCmdTraceRaysIndirect2KHR = ( PFN_vkCmdTraceRaysIndirect2KHR ) vkGetDeviceProcAddr( device, "vkCmdTraceRaysIndirect2KHR" );

	vkGetDeviceBufferMemoryRequirementsKHR = ( PFN_vkGetDeviceBufferMemoryRequirementsKHR ) vkGetDeviceProcAddr( device, "vkGetDeviceBufferMemoryRequirementsKHR" );

	vkGetDeviceImageMemoryRequirementsKHR = ( PFN_vkGetDeviceImageMemoryRequirementsKHR ) vkGetDeviceProcAddr( device, "vkGetDeviceImageMemoryRequirementsKHR" );

	vkGetDeviceImageSparseMemoryRequirementsKHR = ( PFN_vkGetDeviceImageSparseMemoryRequirementsKHR ) vkGetDeviceProcAddr( device, "vkGetDeviceImageSparseMemoryRequirementsKHR" );

	vkCmdBindIndexBuffer2KHR = ( PFN_vkCmdBindIndexBuffer2KHR ) vkGetDeviceProcAddr( device, "vkCmdBindIndexBuffer2KHR" );

	vkGetRenderingAreaGranularityKHR = ( PFN_vkGetRenderingAreaGranularityKHR ) vkGetDeviceProcAddr( device, "vkGetRenderingAreaGranularityKHR" );

	vkGetDeviceImageSubresourceLayoutKHR = ( PFN_vkGetDeviceImageSubresourceLayoutKHR ) vkGetDeviceProcAddr( device, "vkGetDeviceImageSubresourceLayoutKHR" );

	vkGetImageSubresourceLayout2KHR = ( PFN_vkGetImageSubresourceLayout2KHR ) vkGetDeviceProcAddr( device, "vkGetImageSubresourceLayout2KHR" );

	vkWaitForPresent2KHR = ( PFN_vkWaitForPresent2KHR ) vkGetDeviceProcAddr( device, "vkWaitForPresent2KHR" );

	vkCreatePipelineBinariesKHR = ( PFN_vkCreatePipelineBinariesKHR ) vkGetDeviceProcAddr( device, "vkCreatePipelineBinariesKHR" );

	vkDestroyPipelineBinaryKHR = ( PFN_vkDestroyPipelineBinaryKHR ) vkGetDeviceProcAddr( device, "vkDestroyPipelineBinaryKHR" );

	vkGetPipelineKeyKHR = ( PFN_vkGetPipelineKeyKHR ) vkGetDeviceProcAddr( device, "vkGetPipelineKeyKHR" );

	vkGetPipelineBinaryDataKHR = ( PFN_vkGetPipelineBinaryDataKHR ) vkGetDeviceProcAddr( device, "vkGetPipelineBinaryDataKHR" );

	vkReleaseCapturedPipelineDataKHR = ( PFN_vkReleaseCapturedPipelineDataKHR ) vkGetDeviceProcAddr( device, "vkReleaseCapturedPipelineDataKHR" );

	vkReleaseSwapchainImagesKHR = ( PFN_vkReleaseSwapchainImagesKHR ) vkGetDeviceProcAddr( device, "vkReleaseSwapchainImagesKHR" );

	vkCmdSetLineStippleKHR = ( PFN_vkCmdSetLineStippleKHR ) vkGetDeviceProcAddr( device, "vkCmdSetLineStippleKHR" );

	vkGetCalibratedTimestampsKHR = ( PFN_vkGetCalibratedTimestampsKHR ) vkGetDeviceProcAddr( device, "vkGetCalibratedTimestampsKHR" );

	vkCmdBindDescriptorSets2KHR = ( PFN_vkCmdBindDescriptorSets2KHR ) vkGetDeviceProcAddr( device, "vkCmdBindDescriptorSets2KHR" );

	vkCmdPushConstants2KHR = ( PFN_vkCmdPushConstants2KHR ) vkGetDeviceProcAddr( device, "vkCmdPushConstants2KHR" );

	vkCmdPushDescriptorSet2KHR = ( PFN_vkCmdPushDescriptorSet2KHR ) vkGetDeviceProcAddr( device, "vkCmdPushDescriptorSet2KHR" );

	vkCmdPushDescriptorSetWithTemplate2KHR = ( PFN_vkCmdPushDescriptorSetWithTemplate2KHR ) vkGetDeviceProcAddr( device, "vkCmdPushDescriptorSetWithTemplate2KHR" );

	vkCmdSetDescriptorBufferOffsets2EXT = ( PFN_vkCmdSetDescriptorBufferOffsets2EXT ) vkGetDeviceProcAddr( device, "vkCmdSetDescriptorBufferOffsets2EXT" );

	vkCmdBindDescriptorBufferEmbeddedSamplers2EXT = ( PFN_vkCmdBindDescriptorBufferEmbeddedSamplers2EXT ) vkGetDeviceProcAddr( device, "vkCmdBindDescriptorBufferEmbeddedSamplers2EXT" );

	vkCmdCopyMemoryIndirectKHR = ( PFN_vkCmdCopyMemoryIndirectKHR ) vkGetDeviceProcAddr( device, "vkCmdCopyMemoryIndirectKHR" );

	vkCmdCopyMemoryToImageIndirectKHR = ( PFN_vkCmdCopyMemoryToImageIndirectKHR ) vkGetDeviceProcAddr( device, "vkCmdCopyMemoryToImageIndirectKHR" );

	vkDebugMarkerSetObjectTagEXT = ( PFN_vkDebugMarkerSetObjectTagEXT ) vkGetDeviceProcAddr( device, "vkDebugMarkerSetObjectTagEXT" );

	vkDebugMarkerSetObjectNameEXT = ( PFN_vkDebugMarkerSetObjectNameEXT ) vkGetDeviceProcAddr( device, "vkDebugMarkerSetObjectNameEXT" );

	vkCmdDebugMarkerBeginEXT = ( PFN_vkCmdDebugMarkerBeginEXT ) vkGetDeviceProcAddr( device, "vkCmdDebugMarkerBeginEXT" );

	vkCmdDebugMarkerEndEXT = ( PFN_vkCmdDebugMarkerEndEXT ) vkGetDeviceProcAddr( device, "vkCmdDebugMarkerEndEXT" );

	vkCmdDebugMarkerInsertEXT = ( PFN_vkCmdDebugMarkerInsertEXT ) vkGetDeviceProcAddr( device, "vkCmdDebugMarkerInsertEXT" );

	vkCmdBindTransformFeedbackBuffersEXT = ( PFN_vkCmdBindTransformFeedbackBuffersEXT ) vkGetDeviceProcAddr( device, "vkCmdBindTransformFeedbackBuffersEXT" );

	vkCmdBeginTransformFeedbackEXT = ( PFN_vkCmdBeginTransformFeedbackEXT ) vkGetDeviceProcAddr( device, "vkCmdBeginTransformFeedbackEXT" );

	vkCmdEndTransformFeedbackEXT = ( PFN_vkCmdEndTransformFeedbackEXT ) vkGetDeviceProcAddr( device, "vkCmdEndTransformFeedbackEXT" );

	vkCmdBeginQueryIndexedEXT = ( PFN_vkCmdBeginQueryIndexedEXT ) vkGetDeviceProcAddr( device, "vkCmdBeginQueryIndexedEXT" );

	vkCmdEndQueryIndexedEXT = ( PFN_vkCmdEndQueryIndexedEXT ) vkGetDeviceProcAddr( device, "vkCmdEndQueryIndexedEXT" );

	vkCmdDrawIndirectByteCountEXT = ( PFN_vkCmdDrawIndirectByteCountEXT ) vkGetDeviceProcAddr( device, "vkCmdDrawIndirectByteCountEXT" );

	vkCreateCuModuleNVX = ( PFN_vkCreateCuModuleNVX ) vkGetDeviceProcAddr( device, "vkCreateCuModuleNVX" );

	vkCreateCuFunctionNVX = ( PFN_vkCreateCuFunctionNVX ) vkGetDeviceProcAddr( device, "vkCreateCuFunctionNVX" );

	vkDestroyCuModuleNVX = ( PFN_vkDestroyCuModuleNVX ) vkGetDeviceProcAddr( device, "vkDestroyCuModuleNVX" );

	vkDestroyCuFunctionNVX = ( PFN_vkDestroyCuFunctionNVX ) vkGetDeviceProcAddr( device, "vkDestroyCuFunctionNVX" );

	vkCmdCuLaunchKernelNVX = ( PFN_vkCmdCuLaunchKernelNVX ) vkGetDeviceProcAddr( device, "vkCmdCuLaunchKernelNVX" );

	vkGetImageViewHandleNVX = ( PFN_vkGetImageViewHandleNVX ) vkGetDeviceProcAddr( device, "vkGetImageViewHandleNVX" );

	vkGetImageViewHandle64NVX = ( PFN_vkGetImageViewHandle64NVX ) vkGetDeviceProcAddr( device, "vkGetImageViewHandle64NVX" );

	vkGetImageViewAddressNVX = ( PFN_vkGetImageViewAddressNVX ) vkGetDeviceProcAddr( device, "vkGetImageViewAddressNVX" );

	vkCmdDrawIndirectCountAMD = ( PFN_vkCmdDrawIndirectCountAMD ) vkGetDeviceProcAddr( device, "vkCmdDrawIndirectCountAMD" );

	vkCmdDrawIndexedIndirectCountAMD = ( PFN_vkCmdDrawIndexedIndirectCountAMD ) vkGetDeviceProcAddr( device, "vkCmdDrawIndexedIndirectCountAMD" );

	vkGetShaderInfoAMD = ( PFN_vkGetShaderInfoAMD ) vkGetDeviceProcAddr( device, "vkGetShaderInfoAMD" );

	vkCmdBeginConditionalRenderingEXT = ( PFN_vkCmdBeginConditionalRenderingEXT ) vkGetDeviceProcAddr( device, "vkCmdBeginConditionalRenderingEXT" );

	vkCmdEndConditionalRenderingEXT = ( PFN_vkCmdEndConditionalRenderingEXT ) vkGetDeviceProcAddr( device, "vkCmdEndConditionalRenderingEXT" );

	vkCmdSetViewportWScalingNV = ( PFN_vkCmdSetViewportWScalingNV ) vkGetDeviceProcAddr( device, "vkCmdSetViewportWScalingNV" );

	vkDisplayPowerControlEXT = ( PFN_vkDisplayPowerControlEXT ) vkGetDeviceProcAddr( device, "vkDisplayPowerControlEXT" );

	vkRegisterDeviceEventEXT = ( PFN_vkRegisterDeviceEventEXT ) vkGetDeviceProcAddr( device, "vkRegisterDeviceEventEXT" );

	vkRegisterDisplayEventEXT = ( PFN_vkRegisterDisplayEventEXT ) vkGetDeviceProcAddr( device, "vkRegisterDisplayEventEXT" );

	vkGetSwapchainCounterEXT = ( PFN_vkGetSwapchainCounterEXT ) vkGetDeviceProcAddr( device, "vkGetSwapchainCounterEXT" );

	vkGetRefreshCycleDurationGOOGLE = ( PFN_vkGetRefreshCycleDurationGOOGLE ) vkGetDeviceProcAddr( device, "vkGetRefreshCycleDurationGOOGLE" );

	vkGetPastPresentationTimingGOOGLE = ( PFN_vkGetPastPresentationTimingGOOGLE ) vkGetDeviceProcAddr( device, "vkGetPastPresentationTimingGOOGLE" );

	vkCmdSetDiscardRectangleEXT = ( PFN_vkCmdSetDiscardRectangleEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDiscardRectangleEXT" );

	vkCmdSetDiscardRectangleEnableEXT = ( PFN_vkCmdSetDiscardRectangleEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDiscardRectangleEnableEXT" );

	vkCmdSetDiscardRectangleModeEXT = ( PFN_vkCmdSetDiscardRectangleModeEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDiscardRectangleModeEXT" );

	vkSetHdrMetadataEXT = ( PFN_vkSetHdrMetadataEXT ) vkGetDeviceProcAddr( device, "vkSetHdrMetadataEXT" );

	vkSetDebugUtilsObjectNameEXT = ( PFN_vkSetDebugUtilsObjectNameEXT ) vkGetDeviceProcAddr( device, "vkSetDebugUtilsObjectNameEXT" );

	vkSetDebugUtilsObjectTagEXT = ( PFN_vkSetDebugUtilsObjectTagEXT ) vkGetDeviceProcAddr( device, "vkSetDebugUtilsObjectTagEXT" );

	vkQueueBeginDebugUtilsLabelEXT = ( PFN_vkQueueBeginDebugUtilsLabelEXT ) vkGetDeviceProcAddr( device, "vkQueueBeginDebugUtilsLabelEXT" );

	vkQueueEndDebugUtilsLabelEXT = ( PFN_vkQueueEndDebugUtilsLabelEXT ) vkGetDeviceProcAddr( device, "vkQueueEndDebugUtilsLabelEXT" );

	vkQueueInsertDebugUtilsLabelEXT = ( PFN_vkQueueInsertDebugUtilsLabelEXT ) vkGetDeviceProcAddr( device, "vkQueueInsertDebugUtilsLabelEXT" );

	vkCmdBeginDebugUtilsLabelEXT = ( PFN_vkCmdBeginDebugUtilsLabelEXT ) vkGetDeviceProcAddr( device, "vkCmdBeginDebugUtilsLabelEXT" );

	vkCmdEndDebugUtilsLabelEXT = ( PFN_vkCmdEndDebugUtilsLabelEXT ) vkGetDeviceProcAddr( device, "vkCmdEndDebugUtilsLabelEXT" );

	vkCmdInsertDebugUtilsLabelEXT = ( PFN_vkCmdInsertDebugUtilsLabelEXT ) vkGetDeviceProcAddr( device, "vkCmdInsertDebugUtilsLabelEXT" );

	vkCmdSetSampleLocationsEXT = ( PFN_vkCmdSetSampleLocationsEXT ) vkGetDeviceProcAddr( device, "vkCmdSetSampleLocationsEXT" );

	vkGetImageDrmFormatModifierPropertiesEXT = ( PFN_vkGetImageDrmFormatModifierPropertiesEXT ) vkGetDeviceProcAddr( device, "vkGetImageDrmFormatModifierPropertiesEXT" );

	vkCreateValidationCacheEXT = ( PFN_vkCreateValidationCacheEXT ) vkGetDeviceProcAddr( device, "vkCreateValidationCacheEXT" );

	vkDestroyValidationCacheEXT = ( PFN_vkDestroyValidationCacheEXT ) vkGetDeviceProcAddr( device, "vkDestroyValidationCacheEXT" );

	vkMergeValidationCachesEXT = ( PFN_vkMergeValidationCachesEXT ) vkGetDeviceProcAddr( device, "vkMergeValidationCachesEXT" );

	vkGetValidationCacheDataEXT = ( PFN_vkGetValidationCacheDataEXT ) vkGetDeviceProcAddr( device, "vkGetValidationCacheDataEXT" );

	vkCmdBindShadingRateImageNV = ( PFN_vkCmdBindShadingRateImageNV ) vkGetDeviceProcAddr( device, "vkCmdBindShadingRateImageNV" );

	vkCmdSetViewportShadingRatePaletteNV = ( PFN_vkCmdSetViewportShadingRatePaletteNV ) vkGetDeviceProcAddr( device, "vkCmdSetViewportShadingRatePaletteNV" );

	vkCmdSetCoarseSampleOrderNV = ( PFN_vkCmdSetCoarseSampleOrderNV ) vkGetDeviceProcAddr( device, "vkCmdSetCoarseSampleOrderNV" );

	vkCreateAccelerationStructureNV = ( PFN_vkCreateAccelerationStructureNV ) vkGetDeviceProcAddr( device, "vkCreateAccelerationStructureNV" );

	vkDestroyAccelerationStructureNV = ( PFN_vkDestroyAccelerationStructureNV ) vkGetDeviceProcAddr( device, "vkDestroyAccelerationStructureNV" );

	vkGetAccelerationStructureMemoryRequirementsNV = ( PFN_vkGetAccelerationStructureMemoryRequirementsNV ) vkGetDeviceProcAddr( device, "vkGetAccelerationStructureMemoryRequirementsNV" );

	vkBindAccelerationStructureMemoryNV = ( PFN_vkBindAccelerationStructureMemoryNV ) vkGetDeviceProcAddr( device, "vkBindAccelerationStructureMemoryNV" );

	vkCmdBuildAccelerationStructureNV = ( PFN_vkCmdBuildAccelerationStructureNV ) vkGetDeviceProcAddr( device, "vkCmdBuildAccelerationStructureNV" );

	vkCmdCopyAccelerationStructureNV = ( PFN_vkCmdCopyAccelerationStructureNV ) vkGetDeviceProcAddr( device, "vkCmdCopyAccelerationStructureNV" );

	vkCmdTraceRaysNV = ( PFN_vkCmdTraceRaysNV ) vkGetDeviceProcAddr( device, "vkCmdTraceRaysNV" );

	vkCreateRayTracingPipelinesNV = ( PFN_vkCreateRayTracingPipelinesNV ) vkGetDeviceProcAddr( device, "vkCreateRayTracingPipelinesNV" );

	vkGetRayTracingShaderGroupHandlesKHR = ( PFN_vkGetRayTracingShaderGroupHandlesKHR ) vkGetDeviceProcAddr( device, "vkGetRayTracingShaderGroupHandlesKHR" );

	vkGetRayTracingShaderGroupHandlesNV = ( PFN_vkGetRayTracingShaderGroupHandlesNV ) vkGetDeviceProcAddr( device, "vkGetRayTracingShaderGroupHandlesNV" );

	vkGetAccelerationStructureHandleNV = ( PFN_vkGetAccelerationStructureHandleNV ) vkGetDeviceProcAddr( device, "vkGetAccelerationStructureHandleNV" );

	vkCmdWriteAccelerationStructuresPropertiesNV = ( PFN_vkCmdWriteAccelerationStructuresPropertiesNV ) vkGetDeviceProcAddr( device, "vkCmdWriteAccelerationStructuresPropertiesNV" );

	vkCompileDeferredNV = ( PFN_vkCompileDeferredNV ) vkGetDeviceProcAddr( device, "vkCompileDeferredNV" );

	vkGetMemoryHostPointerPropertiesEXT = ( PFN_vkGetMemoryHostPointerPropertiesEXT ) vkGetDeviceProcAddr( device, "vkGetMemoryHostPointerPropertiesEXT" );

	vkCmdWriteBufferMarkerAMD = ( PFN_vkCmdWriteBufferMarkerAMD ) vkGetDeviceProcAddr( device, "vkCmdWriteBufferMarkerAMD" );

	vkCmdWriteBufferMarker2AMD = ( PFN_vkCmdWriteBufferMarker2AMD ) vkGetDeviceProcAddr( device, "vkCmdWriteBufferMarker2AMD" );

	vkGetCalibratedTimestampsEXT = ( PFN_vkGetCalibratedTimestampsEXT ) vkGetDeviceProcAddr( device, "vkGetCalibratedTimestampsEXT" );

	vkCmdDrawMeshTasksNV = ( PFN_vkCmdDrawMeshTasksNV ) vkGetDeviceProcAddr( device, "vkCmdDrawMeshTasksNV" );

	vkCmdDrawMeshTasksIndirectNV = ( PFN_vkCmdDrawMeshTasksIndirectNV ) vkGetDeviceProcAddr( device, "vkCmdDrawMeshTasksIndirectNV" );

	vkCmdDrawMeshTasksIndirectCountNV = ( PFN_vkCmdDrawMeshTasksIndirectCountNV ) vkGetDeviceProcAddr( device, "vkCmdDrawMeshTasksIndirectCountNV" );

	vkCmdSetExclusiveScissorEnableNV = ( PFN_vkCmdSetExclusiveScissorEnableNV ) vkGetDeviceProcAddr( device, "vkCmdSetExclusiveScissorEnableNV" );

	vkCmdSetExclusiveScissorNV = ( PFN_vkCmdSetExclusiveScissorNV ) vkGetDeviceProcAddr( device, "vkCmdSetExclusiveScissorNV" );

	vkCmdSetCheckpointNV = ( PFN_vkCmdSetCheckpointNV ) vkGetDeviceProcAddr( device, "vkCmdSetCheckpointNV" );

	vkGetQueueCheckpointDataNV = ( PFN_vkGetQueueCheckpointDataNV ) vkGetDeviceProcAddr( device, "vkGetQueueCheckpointDataNV" );

	vkGetQueueCheckpointData2NV = ( PFN_vkGetQueueCheckpointData2NV ) vkGetDeviceProcAddr( device, "vkGetQueueCheckpointData2NV" );

	vkInitializePerformanceApiINTEL = ( PFN_vkInitializePerformanceApiINTEL ) vkGetDeviceProcAddr( device, "vkInitializePerformanceApiINTEL" );

	vkUninitializePerformanceApiINTEL = ( PFN_vkUninitializePerformanceApiINTEL ) vkGetDeviceProcAddr( device, "vkUninitializePerformanceApiINTEL" );

	vkCmdSetPerformanceMarkerINTEL = ( PFN_vkCmdSetPerformanceMarkerINTEL ) vkGetDeviceProcAddr( device, "vkCmdSetPerformanceMarkerINTEL" );

	vkCmdSetPerformanceStreamMarkerINTEL = ( PFN_vkCmdSetPerformanceStreamMarkerINTEL ) vkGetDeviceProcAddr( device, "vkCmdSetPerformanceStreamMarkerINTEL" );

	vkCmdSetPerformanceOverrideINTEL = ( PFN_vkCmdSetPerformanceOverrideINTEL ) vkGetDeviceProcAddr( device, "vkCmdSetPerformanceOverrideINTEL" );

	vkAcquirePerformanceConfigurationINTEL = ( PFN_vkAcquirePerformanceConfigurationINTEL ) vkGetDeviceProcAddr( device, "vkAcquirePerformanceConfigurationINTEL" );

	vkReleasePerformanceConfigurationINTEL = ( PFN_vkReleasePerformanceConfigurationINTEL ) vkGetDeviceProcAddr( device, "vkReleasePerformanceConfigurationINTEL" );

	vkQueueSetPerformanceConfigurationINTEL = ( PFN_vkQueueSetPerformanceConfigurationINTEL ) vkGetDeviceProcAddr( device, "vkQueueSetPerformanceConfigurationINTEL" );

	vkGetPerformanceParameterINTEL = ( PFN_vkGetPerformanceParameterINTEL ) vkGetDeviceProcAddr( device, "vkGetPerformanceParameterINTEL" );

	vkSetLocalDimmingAMD = ( PFN_vkSetLocalDimmingAMD ) vkGetDeviceProcAddr( device, "vkSetLocalDimmingAMD" );

	vkGetBufferDeviceAddressEXT = ( PFN_vkGetBufferDeviceAddressEXT ) vkGetDeviceProcAddr( device, "vkGetBufferDeviceAddressEXT" );

	vkCmdSetLineStippleEXT = ( PFN_vkCmdSetLineStippleEXT ) vkGetDeviceProcAddr( device, "vkCmdSetLineStippleEXT" );

	vkResetQueryPoolEXT = ( PFN_vkResetQueryPoolEXT ) vkGetDeviceProcAddr( device, "vkResetQueryPoolEXT" );

	vkCmdSetCullModeEXT = ( PFN_vkCmdSetCullModeEXT ) vkGetDeviceProcAddr( device, "vkCmdSetCullModeEXT" );

	vkCmdSetFrontFaceEXT = ( PFN_vkCmdSetFrontFaceEXT ) vkGetDeviceProcAddr( device, "vkCmdSetFrontFaceEXT" );

	vkCmdSetPrimitiveTopologyEXT = ( PFN_vkCmdSetPrimitiveTopologyEXT ) vkGetDeviceProcAddr( device, "vkCmdSetPrimitiveTopologyEXT" );

	vkCmdSetViewportWithCountEXT = ( PFN_vkCmdSetViewportWithCountEXT ) vkGetDeviceProcAddr( device, "vkCmdSetViewportWithCountEXT" );

	vkCmdSetScissorWithCountEXT = ( PFN_vkCmdSetScissorWithCountEXT ) vkGetDeviceProcAddr( device, "vkCmdSetScissorWithCountEXT" );

	vkCmdBindVertexBuffers2EXT = ( PFN_vkCmdBindVertexBuffers2EXT ) vkGetDeviceProcAddr( device, "vkCmdBindVertexBuffers2EXT" );

	vkCmdSetDepthTestEnableEXT = ( PFN_vkCmdSetDepthTestEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDepthTestEnableEXT" );

	vkCmdSetDepthWriteEnableEXT = ( PFN_vkCmdSetDepthWriteEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDepthWriteEnableEXT" );

	vkCmdSetDepthCompareOpEXT = ( PFN_vkCmdSetDepthCompareOpEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDepthCompareOpEXT" );

	vkCmdSetDepthBoundsTestEnableEXT = ( PFN_vkCmdSetDepthBoundsTestEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDepthBoundsTestEnableEXT" );

	vkCmdSetStencilTestEnableEXT = ( PFN_vkCmdSetStencilTestEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetStencilTestEnableEXT" );

	vkCmdSetStencilOpEXT = ( PFN_vkCmdSetStencilOpEXT ) vkGetDeviceProcAddr( device, "vkCmdSetStencilOpEXT" );

	vkCopyMemoryToImageEXT = ( PFN_vkCopyMemoryToImageEXT ) vkGetDeviceProcAddr( device, "vkCopyMemoryToImageEXT" );

	vkCopyImageToMemoryEXT = ( PFN_vkCopyImageToMemoryEXT ) vkGetDeviceProcAddr( device, "vkCopyImageToMemoryEXT" );

	vkCopyImageToImageEXT = ( PFN_vkCopyImageToImageEXT ) vkGetDeviceProcAddr( device, "vkCopyImageToImageEXT" );

	vkTransitionImageLayoutEXT = ( PFN_vkTransitionImageLayoutEXT ) vkGetDeviceProcAddr( device, "vkTransitionImageLayoutEXT" );

	vkGetImageSubresourceLayout2EXT = ( PFN_vkGetImageSubresourceLayout2EXT ) vkGetDeviceProcAddr( device, "vkGetImageSubresourceLayout2EXT" );

	vkReleaseSwapchainImagesEXT = ( PFN_vkReleaseSwapchainImagesEXT ) vkGetDeviceProcAddr( device, "vkReleaseSwapchainImagesEXT" );

	vkGetGeneratedCommandsMemoryRequirementsNV = ( PFN_vkGetGeneratedCommandsMemoryRequirementsNV ) vkGetDeviceProcAddr( device, "vkGetGeneratedCommandsMemoryRequirementsNV" );

	vkCmdPreprocessGeneratedCommandsNV = ( PFN_vkCmdPreprocessGeneratedCommandsNV ) vkGetDeviceProcAddr( device, "vkCmdPreprocessGeneratedCommandsNV" );

	vkCmdExecuteGeneratedCommandsNV = ( PFN_vkCmdExecuteGeneratedCommandsNV ) vkGetDeviceProcAddr( device, "vkCmdExecuteGeneratedCommandsNV" );

	vkCmdBindPipelineShaderGroupNV = ( PFN_vkCmdBindPipelineShaderGroupNV ) vkGetDeviceProcAddr( device, "vkCmdBindPipelineShaderGroupNV" );

	vkCreateIndirectCommandsLayoutNV = ( PFN_vkCreateIndirectCommandsLayoutNV ) vkGetDeviceProcAddr( device, "vkCreateIndirectCommandsLayoutNV" );

	vkDestroyIndirectCommandsLayoutNV = ( PFN_vkDestroyIndirectCommandsLayoutNV ) vkGetDeviceProcAddr( device, "vkDestroyIndirectCommandsLayoutNV" );

	vkCmdSetDepthBias2EXT = ( PFN_vkCmdSetDepthBias2EXT ) vkGetDeviceProcAddr( device, "vkCmdSetDepthBias2EXT" );

	vkCreatePrivateDataSlotEXT = ( PFN_vkCreatePrivateDataSlotEXT ) vkGetDeviceProcAddr( device, "vkCreatePrivateDataSlotEXT" );

	vkDestroyPrivateDataSlotEXT = ( PFN_vkDestroyPrivateDataSlotEXT ) vkGetDeviceProcAddr( device, "vkDestroyPrivateDataSlotEXT" );

	vkSetPrivateDataEXT = ( PFN_vkSetPrivateDataEXT ) vkGetDeviceProcAddr( device, "vkSetPrivateDataEXT" );

	vkGetPrivateDataEXT = ( PFN_vkGetPrivateDataEXT ) vkGetDeviceProcAddr( device, "vkGetPrivateDataEXT" );

	vkCmdDispatchTileQCOM = ( PFN_vkCmdDispatchTileQCOM ) vkGetDeviceProcAddr( device, "vkCmdDispatchTileQCOM" );

	vkCmdBeginPerTileExecutionQCOM = ( PFN_vkCmdBeginPerTileExecutionQCOM ) vkGetDeviceProcAddr( device, "vkCmdBeginPerTileExecutionQCOM" );

	vkCmdEndPerTileExecutionQCOM = ( PFN_vkCmdEndPerTileExecutionQCOM ) vkGetDeviceProcAddr( device, "vkCmdEndPerTileExecutionQCOM" );

	vkGetDescriptorSetLayoutSizeEXT = ( PFN_vkGetDescriptorSetLayoutSizeEXT ) vkGetDeviceProcAddr( device, "vkGetDescriptorSetLayoutSizeEXT" );

	vkGetDescriptorSetLayoutBindingOffsetEXT = ( PFN_vkGetDescriptorSetLayoutBindingOffsetEXT ) vkGetDeviceProcAddr( device, "vkGetDescriptorSetLayoutBindingOffsetEXT" );

	vkGetDescriptorEXT = ( PFN_vkGetDescriptorEXT ) vkGetDeviceProcAddr( device, "vkGetDescriptorEXT" );

	vkCmdBindDescriptorBuffersEXT = ( PFN_vkCmdBindDescriptorBuffersEXT ) vkGetDeviceProcAddr( device, "vkCmdBindDescriptorBuffersEXT" );

	vkCmdSetDescriptorBufferOffsetsEXT = ( PFN_vkCmdSetDescriptorBufferOffsetsEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDescriptorBufferOffsetsEXT" );

	vkCmdBindDescriptorBufferEmbeddedSamplersEXT = ( PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT ) vkGetDeviceProcAddr( device, "vkCmdBindDescriptorBufferEmbeddedSamplersEXT" );

	vkGetBufferOpaqueCaptureDescriptorDataEXT = ( PFN_vkGetBufferOpaqueCaptureDescriptorDataEXT ) vkGetDeviceProcAddr( device, "vkGetBufferOpaqueCaptureDescriptorDataEXT" );

	vkGetImageOpaqueCaptureDescriptorDataEXT = ( PFN_vkGetImageOpaqueCaptureDescriptorDataEXT ) vkGetDeviceProcAddr( device, "vkGetImageOpaqueCaptureDescriptorDataEXT" );

	vkGetImageViewOpaqueCaptureDescriptorDataEXT = ( PFN_vkGetImageViewOpaqueCaptureDescriptorDataEXT ) vkGetDeviceProcAddr( device, "vkGetImageViewOpaqueCaptureDescriptorDataEXT" );

	vkGetSamplerOpaqueCaptureDescriptorDataEXT = ( PFN_vkGetSamplerOpaqueCaptureDescriptorDataEXT ) vkGetDeviceProcAddr( device, "vkGetSamplerOpaqueCaptureDescriptorDataEXT" );

	vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT = ( PFN_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT ) vkGetDeviceProcAddr( device, "vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT" );

	vkCmdSetFragmentShadingRateEnumNV = ( PFN_vkCmdSetFragmentShadingRateEnumNV ) vkGetDeviceProcAddr( device, "vkCmdSetFragmentShadingRateEnumNV" );

	vkGetDeviceFaultInfoEXT = ( PFN_vkGetDeviceFaultInfoEXT ) vkGetDeviceProcAddr( device, "vkGetDeviceFaultInfoEXT" );

	vkCmdSetVertexInputEXT = ( PFN_vkCmdSetVertexInputEXT ) vkGetDeviceProcAddr( device, "vkCmdSetVertexInputEXT" );

	vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI = ( PFN_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI ) vkGetDeviceProcAddr( device, "vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI" );

	vkCmdSubpassShadingHUAWEI = ( PFN_vkCmdSubpassShadingHUAWEI ) vkGetDeviceProcAddr( device, "vkCmdSubpassShadingHUAWEI" );

	vkCmdBindInvocationMaskHUAWEI = ( PFN_vkCmdBindInvocationMaskHUAWEI ) vkGetDeviceProcAddr( device, "vkCmdBindInvocationMaskHUAWEI" );

	vkGetMemoryRemoteAddressNV = ( PFN_vkGetMemoryRemoteAddressNV ) vkGetDeviceProcAddr( device, "vkGetMemoryRemoteAddressNV" );

	vkGetPipelinePropertiesEXT = ( PFN_vkGetPipelinePropertiesEXT ) vkGetDeviceProcAddr( device, "vkGetPipelinePropertiesEXT" );

	vkCmdSetPatchControlPointsEXT = ( PFN_vkCmdSetPatchControlPointsEXT ) vkGetDeviceProcAddr( device, "vkCmdSetPatchControlPointsEXT" );

	vkCmdSetRasterizerDiscardEnableEXT = ( PFN_vkCmdSetRasterizerDiscardEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetRasterizerDiscardEnableEXT" );

	vkCmdSetDepthBiasEnableEXT = ( PFN_vkCmdSetDepthBiasEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDepthBiasEnableEXT" );

	vkCmdSetLogicOpEXT = ( PFN_vkCmdSetLogicOpEXT ) vkGetDeviceProcAddr( device, "vkCmdSetLogicOpEXT" );

	vkCmdSetPrimitiveRestartEnableEXT = ( PFN_vkCmdSetPrimitiveRestartEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetPrimitiveRestartEnableEXT" );

	vkCmdSetColorWriteEnableEXT = ( PFN_vkCmdSetColorWriteEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetColorWriteEnableEXT" );

	vkCmdDrawMultiEXT = ( PFN_vkCmdDrawMultiEXT ) vkGetDeviceProcAddr( device, "vkCmdDrawMultiEXT" );

	vkCmdDrawMultiIndexedEXT = ( PFN_vkCmdDrawMultiIndexedEXT ) vkGetDeviceProcAddr( device, "vkCmdDrawMultiIndexedEXT" );

	vkCreateMicromapEXT = ( PFN_vkCreateMicromapEXT ) vkGetDeviceProcAddr( device, "vkCreateMicromapEXT" );

	vkDestroyMicromapEXT = ( PFN_vkDestroyMicromapEXT ) vkGetDeviceProcAddr( device, "vkDestroyMicromapEXT" );

	vkCmdBuildMicromapsEXT = ( PFN_vkCmdBuildMicromapsEXT ) vkGetDeviceProcAddr( device, "vkCmdBuildMicromapsEXT" );

	vkBuildMicromapsEXT = ( PFN_vkBuildMicromapsEXT ) vkGetDeviceProcAddr( device, "vkBuildMicromapsEXT" );

	vkCopyMicromapEXT = ( PFN_vkCopyMicromapEXT ) vkGetDeviceProcAddr( device, "vkCopyMicromapEXT" );

	vkCopyMicromapToMemoryEXT = ( PFN_vkCopyMicromapToMemoryEXT ) vkGetDeviceProcAddr( device, "vkCopyMicromapToMemoryEXT" );

	vkCopyMemoryToMicromapEXT = ( PFN_vkCopyMemoryToMicromapEXT ) vkGetDeviceProcAddr( device, "vkCopyMemoryToMicromapEXT" );

	vkWriteMicromapsPropertiesEXT = ( PFN_vkWriteMicromapsPropertiesEXT ) vkGetDeviceProcAddr( device, "vkWriteMicromapsPropertiesEXT" );

	vkCmdCopyMicromapEXT = ( PFN_vkCmdCopyMicromapEXT ) vkGetDeviceProcAddr( device, "vkCmdCopyMicromapEXT" );

	vkCmdCopyMicromapToMemoryEXT = ( PFN_vkCmdCopyMicromapToMemoryEXT ) vkGetDeviceProcAddr( device, "vkCmdCopyMicromapToMemoryEXT" );

	vkCmdCopyMemoryToMicromapEXT = ( PFN_vkCmdCopyMemoryToMicromapEXT ) vkGetDeviceProcAddr( device, "vkCmdCopyMemoryToMicromapEXT" );

	vkCmdWriteMicromapsPropertiesEXT = ( PFN_vkCmdWriteMicromapsPropertiesEXT ) vkGetDeviceProcAddr( device, "vkCmdWriteMicromapsPropertiesEXT" );

	vkGetDeviceMicromapCompatibilityEXT = ( PFN_vkGetDeviceMicromapCompatibilityEXT ) vkGetDeviceProcAddr( device, "vkGetDeviceMicromapCompatibilityEXT" );

	vkGetMicromapBuildSizesEXT = ( PFN_vkGetMicromapBuildSizesEXT ) vkGetDeviceProcAddr( device, "vkGetMicromapBuildSizesEXT" );

	vkCmdDrawClusterHUAWEI = ( PFN_vkCmdDrawClusterHUAWEI ) vkGetDeviceProcAddr( device, "vkCmdDrawClusterHUAWEI" );

	vkCmdDrawClusterIndirectHUAWEI = ( PFN_vkCmdDrawClusterIndirectHUAWEI ) vkGetDeviceProcAddr( device, "vkCmdDrawClusterIndirectHUAWEI" );

	vkSetDeviceMemoryPriorityEXT = ( PFN_vkSetDeviceMemoryPriorityEXT ) vkGetDeviceProcAddr( device, "vkSetDeviceMemoryPriorityEXT" );

	vkGetDescriptorSetLayoutHostMappingInfoVALVE = ( PFN_vkGetDescriptorSetLayoutHostMappingInfoVALVE ) vkGetDeviceProcAddr( device, "vkGetDescriptorSetLayoutHostMappingInfoVALVE" );

	vkGetDescriptorSetHostMappingVALVE = ( PFN_vkGetDescriptorSetHostMappingVALVE ) vkGetDeviceProcAddr( device, "vkGetDescriptorSetHostMappingVALVE" );

	vkCmdCopyMemoryIndirectNV = ( PFN_vkCmdCopyMemoryIndirectNV ) vkGetDeviceProcAddr( device, "vkCmdCopyMemoryIndirectNV" );

	vkCmdCopyMemoryToImageIndirectNV = ( PFN_vkCmdCopyMemoryToImageIndirectNV ) vkGetDeviceProcAddr( device, "vkCmdCopyMemoryToImageIndirectNV" );

	vkCmdDecompressMemoryNV = ( PFN_vkCmdDecompressMemoryNV ) vkGetDeviceProcAddr( device, "vkCmdDecompressMemoryNV" );

	vkCmdDecompressMemoryIndirectCountNV = ( PFN_vkCmdDecompressMemoryIndirectCountNV ) vkGetDeviceProcAddr( device, "vkCmdDecompressMemoryIndirectCountNV" );

	vkGetPipelineIndirectMemoryRequirementsNV = ( PFN_vkGetPipelineIndirectMemoryRequirementsNV ) vkGetDeviceProcAddr( device, "vkGetPipelineIndirectMemoryRequirementsNV" );

	vkCmdUpdatePipelineIndirectBufferNV = ( PFN_vkCmdUpdatePipelineIndirectBufferNV ) vkGetDeviceProcAddr( device, "vkCmdUpdatePipelineIndirectBufferNV" );

	vkGetPipelineIndirectDeviceAddressNV = ( PFN_vkGetPipelineIndirectDeviceAddressNV ) vkGetDeviceProcAddr( device, "vkGetPipelineIndirectDeviceAddressNV" );

	vkCmdSetDepthClampEnableEXT = ( PFN_vkCmdSetDepthClampEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDepthClampEnableEXT" );

	vkCmdSetPolygonModeEXT = ( PFN_vkCmdSetPolygonModeEXT ) vkGetDeviceProcAddr( device, "vkCmdSetPolygonModeEXT" );

	vkCmdSetRasterizationSamplesEXT = ( PFN_vkCmdSetRasterizationSamplesEXT ) vkGetDeviceProcAddr( device, "vkCmdSetRasterizationSamplesEXT" );

	vkCmdSetSampleMaskEXT = ( PFN_vkCmdSetSampleMaskEXT ) vkGetDeviceProcAddr( device, "vkCmdSetSampleMaskEXT" );

	vkCmdSetAlphaToCoverageEnableEXT = ( PFN_vkCmdSetAlphaToCoverageEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetAlphaToCoverageEnableEXT" );

	vkCmdSetAlphaToOneEnableEXT = ( PFN_vkCmdSetAlphaToOneEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetAlphaToOneEnableEXT" );

	vkCmdSetLogicOpEnableEXT = ( PFN_vkCmdSetLogicOpEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetLogicOpEnableEXT" );

	vkCmdSetColorBlendEnableEXT = ( PFN_vkCmdSetColorBlendEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetColorBlendEnableEXT" );

	vkCmdSetColorBlendEquationEXT = ( PFN_vkCmdSetColorBlendEquationEXT ) vkGetDeviceProcAddr( device, "vkCmdSetColorBlendEquationEXT" );

	vkCmdSetColorWriteMaskEXT = ( PFN_vkCmdSetColorWriteMaskEXT ) vkGetDeviceProcAddr( device, "vkCmdSetColorWriteMaskEXT" );

	vkCmdSetTessellationDomainOriginEXT = ( PFN_vkCmdSetTessellationDomainOriginEXT ) vkGetDeviceProcAddr( device, "vkCmdSetTessellationDomainOriginEXT" );

	vkCmdSetRasterizationStreamEXT = ( PFN_vkCmdSetRasterizationStreamEXT ) vkGetDeviceProcAddr( device, "vkCmdSetRasterizationStreamEXT" );

	vkCmdSetConservativeRasterizationModeEXT = ( PFN_vkCmdSetConservativeRasterizationModeEXT ) vkGetDeviceProcAddr( device, "vkCmdSetConservativeRasterizationModeEXT" );

	vkCmdSetExtraPrimitiveOverestimationSizeEXT = ( PFN_vkCmdSetExtraPrimitiveOverestimationSizeEXT ) vkGetDeviceProcAddr( device, "vkCmdSetExtraPrimitiveOverestimationSizeEXT" );

	vkCmdSetDepthClipEnableEXT = ( PFN_vkCmdSetDepthClipEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDepthClipEnableEXT" );

	vkCmdSetSampleLocationsEnableEXT = ( PFN_vkCmdSetSampleLocationsEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetSampleLocationsEnableEXT" );

	vkCmdSetColorBlendAdvancedEXT = ( PFN_vkCmdSetColorBlendAdvancedEXT ) vkGetDeviceProcAddr( device, "vkCmdSetColorBlendAdvancedEXT" );

	vkCmdSetProvokingVertexModeEXT = ( PFN_vkCmdSetProvokingVertexModeEXT ) vkGetDeviceProcAddr( device, "vkCmdSetProvokingVertexModeEXT" );

	vkCmdSetLineRasterizationModeEXT = ( PFN_vkCmdSetLineRasterizationModeEXT ) vkGetDeviceProcAddr( device, "vkCmdSetLineRasterizationModeEXT" );

	vkCmdSetLineStippleEnableEXT = ( PFN_vkCmdSetLineStippleEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetLineStippleEnableEXT" );

	vkCmdSetDepthClipNegativeOneToOneEXT = ( PFN_vkCmdSetDepthClipNegativeOneToOneEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDepthClipNegativeOneToOneEXT" );

	vkCmdSetViewportWScalingEnableNV = ( PFN_vkCmdSetViewportWScalingEnableNV ) vkGetDeviceProcAddr( device, "vkCmdSetViewportWScalingEnableNV" );

	vkCmdSetViewportSwizzleNV = ( PFN_vkCmdSetViewportSwizzleNV ) vkGetDeviceProcAddr( device, "vkCmdSetViewportSwizzleNV" );

	vkCmdSetCoverageToColorEnableNV = ( PFN_vkCmdSetCoverageToColorEnableNV ) vkGetDeviceProcAddr( device, "vkCmdSetCoverageToColorEnableNV" );

	vkCmdSetCoverageToColorLocationNV = ( PFN_vkCmdSetCoverageToColorLocationNV ) vkGetDeviceProcAddr( device, "vkCmdSetCoverageToColorLocationNV" );

	vkCmdSetCoverageModulationModeNV = ( PFN_vkCmdSetCoverageModulationModeNV ) vkGetDeviceProcAddr( device, "vkCmdSetCoverageModulationModeNV" );

	vkCmdSetCoverageModulationTableEnableNV = ( PFN_vkCmdSetCoverageModulationTableEnableNV ) vkGetDeviceProcAddr( device, "vkCmdSetCoverageModulationTableEnableNV" );

	vkCmdSetCoverageModulationTableNV = ( PFN_vkCmdSetCoverageModulationTableNV ) vkGetDeviceProcAddr( device, "vkCmdSetCoverageModulationTableNV" );

	vkCmdSetShadingRateImageEnableNV = ( PFN_vkCmdSetShadingRateImageEnableNV ) vkGetDeviceProcAddr( device, "vkCmdSetShadingRateImageEnableNV" );

	vkCmdSetRepresentativeFragmentTestEnableNV = ( PFN_vkCmdSetRepresentativeFragmentTestEnableNV ) vkGetDeviceProcAddr( device, "vkCmdSetRepresentativeFragmentTestEnableNV" );

	vkCmdSetCoverageReductionModeNV = ( PFN_vkCmdSetCoverageReductionModeNV ) vkGetDeviceProcAddr( device, "vkCmdSetCoverageReductionModeNV" );

	vkCreateTensorARM = ( PFN_vkCreateTensorARM ) vkGetDeviceProcAddr( device, "vkCreateTensorARM" );

	vkDestroyTensorARM = ( PFN_vkDestroyTensorARM ) vkGetDeviceProcAddr( device, "vkDestroyTensorARM" );

	vkCreateTensorViewARM = ( PFN_vkCreateTensorViewARM ) vkGetDeviceProcAddr( device, "vkCreateTensorViewARM" );

	vkDestroyTensorViewARM = ( PFN_vkDestroyTensorViewARM ) vkGetDeviceProcAddr( device, "vkDestroyTensorViewARM" );

	vkGetTensorMemoryRequirementsARM = ( PFN_vkGetTensorMemoryRequirementsARM ) vkGetDeviceProcAddr( device, "vkGetTensorMemoryRequirementsARM" );

	vkBindTensorMemoryARM = ( PFN_vkBindTensorMemoryARM ) vkGetDeviceProcAddr( device, "vkBindTensorMemoryARM" );

	vkGetDeviceTensorMemoryRequirementsARM = ( PFN_vkGetDeviceTensorMemoryRequirementsARM ) vkGetDeviceProcAddr( device, "vkGetDeviceTensorMemoryRequirementsARM" );

	vkCmdCopyTensorARM = ( PFN_vkCmdCopyTensorARM ) vkGetDeviceProcAddr( device, "vkCmdCopyTensorARM" );

	vkGetTensorOpaqueCaptureDescriptorDataARM = ( PFN_vkGetTensorOpaqueCaptureDescriptorDataARM ) vkGetDeviceProcAddr( device, "vkGetTensorOpaqueCaptureDescriptorDataARM" );

	vkGetTensorViewOpaqueCaptureDescriptorDataARM = ( PFN_vkGetTensorViewOpaqueCaptureDescriptorDataARM ) vkGetDeviceProcAddr( device, "vkGetTensorViewOpaqueCaptureDescriptorDataARM" );

	vkGetShaderModuleIdentifierEXT = ( PFN_vkGetShaderModuleIdentifierEXT ) vkGetDeviceProcAddr( device, "vkGetShaderModuleIdentifierEXT" );

	vkGetShaderModuleCreateInfoIdentifierEXT = ( PFN_vkGetShaderModuleCreateInfoIdentifierEXT ) vkGetDeviceProcAddr( device, "vkGetShaderModuleCreateInfoIdentifierEXT" );

	vkCreateOpticalFlowSessionNV = ( PFN_vkCreateOpticalFlowSessionNV ) vkGetDeviceProcAddr( device, "vkCreateOpticalFlowSessionNV" );

	vkDestroyOpticalFlowSessionNV = ( PFN_vkDestroyOpticalFlowSessionNV ) vkGetDeviceProcAddr( device, "vkDestroyOpticalFlowSessionNV" );

	vkBindOpticalFlowSessionImageNV = ( PFN_vkBindOpticalFlowSessionImageNV ) vkGetDeviceProcAddr( device, "vkBindOpticalFlowSessionImageNV" );

	vkCmdOpticalFlowExecuteNV = ( PFN_vkCmdOpticalFlowExecuteNV ) vkGetDeviceProcAddr( device, "vkCmdOpticalFlowExecuteNV" );

	vkAntiLagUpdateAMD = ( PFN_vkAntiLagUpdateAMD ) vkGetDeviceProcAddr( device, "vkAntiLagUpdateAMD" );

	vkCreateShadersEXT = ( PFN_vkCreateShadersEXT ) vkGetDeviceProcAddr( device, "vkCreateShadersEXT" );

	vkDestroyShaderEXT = ( PFN_vkDestroyShaderEXT ) vkGetDeviceProcAddr( device, "vkDestroyShaderEXT" );

	vkGetShaderBinaryDataEXT = ( PFN_vkGetShaderBinaryDataEXT ) vkGetDeviceProcAddr( device, "vkGetShaderBinaryDataEXT" );

	vkCmdBindShadersEXT = ( PFN_vkCmdBindShadersEXT ) vkGetDeviceProcAddr( device, "vkCmdBindShadersEXT" );

	vkCmdSetDepthClampRangeEXT = ( PFN_vkCmdSetDepthClampRangeEXT ) vkGetDeviceProcAddr( device, "vkCmdSetDepthClampRangeEXT" );

	vkGetFramebufferTilePropertiesQCOM = ( PFN_vkGetFramebufferTilePropertiesQCOM ) vkGetDeviceProcAddr( device, "vkGetFramebufferTilePropertiesQCOM" );

	vkGetDynamicRenderingTilePropertiesQCOM = ( PFN_vkGetDynamicRenderingTilePropertiesQCOM ) vkGetDeviceProcAddr( device, "vkGetDynamicRenderingTilePropertiesQCOM" );

	vkConvertCooperativeVectorMatrixNV = ( PFN_vkConvertCooperativeVectorMatrixNV ) vkGetDeviceProcAddr( device, "vkConvertCooperativeVectorMatrixNV" );

	vkCmdConvertCooperativeVectorMatrixNV = ( PFN_vkCmdConvertCooperativeVectorMatrixNV ) vkGetDeviceProcAddr( device, "vkCmdConvertCooperativeVectorMatrixNV" );

	vkSetLatencySleepModeNV = ( PFN_vkSetLatencySleepModeNV ) vkGetDeviceProcAddr( device, "vkSetLatencySleepModeNV" );

	vkLatencySleepNV = ( PFN_vkLatencySleepNV ) vkGetDeviceProcAddr( device, "vkLatencySleepNV" );

	vkSetLatencyMarkerNV = ( PFN_vkSetLatencyMarkerNV ) vkGetDeviceProcAddr( device, "vkSetLatencyMarkerNV" );

	vkGetLatencyTimingsNV = ( PFN_vkGetLatencyTimingsNV ) vkGetDeviceProcAddr( device, "vkGetLatencyTimingsNV" );

	vkQueueNotifyOutOfBandNV = ( PFN_vkQueueNotifyOutOfBandNV ) vkGetDeviceProcAddr( device, "vkQueueNotifyOutOfBandNV" );

	vkCreateDataGraphPipelinesARM = ( PFN_vkCreateDataGraphPipelinesARM ) vkGetDeviceProcAddr( device, "vkCreateDataGraphPipelinesARM" );

	vkCreateDataGraphPipelineSessionARM = ( PFN_vkCreateDataGraphPipelineSessionARM ) vkGetDeviceProcAddr( device, "vkCreateDataGraphPipelineSessionARM" );

	vkGetDataGraphPipelineSessionBindPointRequirementsARM = ( PFN_vkGetDataGraphPipelineSessionBindPointRequirementsARM ) vkGetDeviceProcAddr( device, "vkGetDataGraphPipelineSessionBindPointRequirementsARM" );

	vkGetDataGraphPipelineSessionMemoryRequirementsARM = ( PFN_vkGetDataGraphPipelineSessionMemoryRequirementsARM ) vkGetDeviceProcAddr( device, "vkGetDataGraphPipelineSessionMemoryRequirementsARM" );

	vkBindDataGraphPipelineSessionMemoryARM = ( PFN_vkBindDataGraphPipelineSessionMemoryARM ) vkGetDeviceProcAddr( device, "vkBindDataGraphPipelineSessionMemoryARM" );

	vkDestroyDataGraphPipelineSessionARM = ( PFN_vkDestroyDataGraphPipelineSessionARM ) vkGetDeviceProcAddr( device, "vkDestroyDataGraphPipelineSessionARM" );

	vkCmdDispatchDataGraphARM = ( PFN_vkCmdDispatchDataGraphARM ) vkGetDeviceProcAddr( device, "vkCmdDispatchDataGraphARM" );

	vkGetDataGraphPipelineAvailablePropertiesARM = ( PFN_vkGetDataGraphPipelineAvailablePropertiesARM ) vkGetDeviceProcAddr( device, "vkGetDataGraphPipelineAvailablePropertiesARM" );

	vkGetDataGraphPipelinePropertiesARM = ( PFN_vkGetDataGraphPipelinePropertiesARM ) vkGetDeviceProcAddr( device, "vkGetDataGraphPipelinePropertiesARM" );

	vkCmdSetAttachmentFeedbackLoopEnableEXT = ( PFN_vkCmdSetAttachmentFeedbackLoopEnableEXT ) vkGetDeviceProcAddr( device, "vkCmdSetAttachmentFeedbackLoopEnableEXT" );

	vkCmdBindTileMemoryQCOM = ( PFN_vkCmdBindTileMemoryQCOM ) vkGetDeviceProcAddr( device, "vkCmdBindTileMemoryQCOM" );

	vkCreateExternalComputeQueueNV = ( PFN_vkCreateExternalComputeQueueNV ) vkGetDeviceProcAddr( device, "vkCreateExternalComputeQueueNV" );

	vkDestroyExternalComputeQueueNV = ( PFN_vkDestroyExternalComputeQueueNV ) vkGetDeviceProcAddr( device, "vkDestroyExternalComputeQueueNV" );

	vkGetExternalComputeQueueDataNV = ( PFN_vkGetExternalComputeQueueDataNV ) vkGetDeviceProcAddr( device, "vkGetExternalComputeQueueDataNV" );

	vkGetClusterAccelerationStructureBuildSizesNV = ( PFN_vkGetClusterAccelerationStructureBuildSizesNV ) vkGetDeviceProcAddr( device, "vkGetClusterAccelerationStructureBuildSizesNV" );

	vkCmdBuildClusterAccelerationStructureIndirectNV = ( PFN_vkCmdBuildClusterAccelerationStructureIndirectNV ) vkGetDeviceProcAddr( device, "vkCmdBuildClusterAccelerationStructureIndirectNV" );

	vkGetPartitionedAccelerationStructuresBuildSizesNV = ( PFN_vkGetPartitionedAccelerationStructuresBuildSizesNV ) vkGetDeviceProcAddr( device, "vkGetPartitionedAccelerationStructuresBuildSizesNV" );

	vkCmdBuildPartitionedAccelerationStructuresNV = ( PFN_vkCmdBuildPartitionedAccelerationStructuresNV ) vkGetDeviceProcAddr( device, "vkCmdBuildPartitionedAccelerationStructuresNV" );

	vkGetGeneratedCommandsMemoryRequirementsEXT = ( PFN_vkGetGeneratedCommandsMemoryRequirementsEXT ) vkGetDeviceProcAddr( device, "vkGetGeneratedCommandsMemoryRequirementsEXT" );

	vkCmdPreprocessGeneratedCommandsEXT = ( PFN_vkCmdPreprocessGeneratedCommandsEXT ) vkGetDeviceProcAddr( device, "vkCmdPreprocessGeneratedCommandsEXT" );

	vkCmdExecuteGeneratedCommandsEXT = ( PFN_vkCmdExecuteGeneratedCommandsEXT ) vkGetDeviceProcAddr( device, "vkCmdExecuteGeneratedCommandsEXT" );

	vkCreateIndirectCommandsLayoutEXT = ( PFN_vkCreateIndirectCommandsLayoutEXT ) vkGetDeviceProcAddr( device, "vkCreateIndirectCommandsLayoutEXT" );

	vkDestroyIndirectCommandsLayoutEXT = ( PFN_vkDestroyIndirectCommandsLayoutEXT ) vkGetDeviceProcAddr( device, "vkDestroyIndirectCommandsLayoutEXT" );

	vkCreateIndirectExecutionSetEXT = ( PFN_vkCreateIndirectExecutionSetEXT ) vkGetDeviceProcAddr( device, "vkCreateIndirectExecutionSetEXT" );

	vkDestroyIndirectExecutionSetEXT = ( PFN_vkDestroyIndirectExecutionSetEXT ) vkGetDeviceProcAddr( device, "vkDestroyIndirectExecutionSetEXT" );

	vkUpdateIndirectExecutionSetPipelineEXT = ( PFN_vkUpdateIndirectExecutionSetPipelineEXT ) vkGetDeviceProcAddr( device, "vkUpdateIndirectExecutionSetPipelineEXT" );

	vkUpdateIndirectExecutionSetShaderEXT = ( PFN_vkUpdateIndirectExecutionSetShaderEXT ) vkGetDeviceProcAddr( device, "vkUpdateIndirectExecutionSetShaderEXT" );

	vkCmdEndRendering2EXT = ( PFN_vkCmdEndRendering2EXT ) vkGetDeviceProcAddr( device, "vkCmdEndRendering2EXT" );

	vkCreateAccelerationStructureKHR = ( PFN_vkCreateAccelerationStructureKHR ) vkGetDeviceProcAddr( device, "vkCreateAccelerationStructureKHR" );

	vkDestroyAccelerationStructureKHR = ( PFN_vkDestroyAccelerationStructureKHR ) vkGetDeviceProcAddr( device, "vkDestroyAccelerationStructureKHR" );

	vkCmdBuildAccelerationStructuresKHR = ( PFN_vkCmdBuildAccelerationStructuresKHR ) vkGetDeviceProcAddr( device, "vkCmdBuildAccelerationStructuresKHR" );

	vkCmdBuildAccelerationStructuresIndirectKHR = ( PFN_vkCmdBuildAccelerationStructuresIndirectKHR ) vkGetDeviceProcAddr( device, "vkCmdBuildAccelerationStructuresIndirectKHR" );

	vkBuildAccelerationStructuresKHR = ( PFN_vkBuildAccelerationStructuresKHR ) vkGetDeviceProcAddr( device, "vkBuildAccelerationStructuresKHR" );

	vkCopyAccelerationStructureKHR = ( PFN_vkCopyAccelerationStructureKHR ) vkGetDeviceProcAddr( device, "vkCopyAccelerationStructureKHR" );

	vkCopyAccelerationStructureToMemoryKHR = ( PFN_vkCopyAccelerationStructureToMemoryKHR ) vkGetDeviceProcAddr( device, "vkCopyAccelerationStructureToMemoryKHR" );

	vkCopyMemoryToAccelerationStructureKHR = ( PFN_vkCopyMemoryToAccelerationStructureKHR ) vkGetDeviceProcAddr( device, "vkCopyMemoryToAccelerationStructureKHR" );

	vkWriteAccelerationStructuresPropertiesKHR = ( PFN_vkWriteAccelerationStructuresPropertiesKHR ) vkGetDeviceProcAddr( device, "vkWriteAccelerationStructuresPropertiesKHR" );

	vkCmdCopyAccelerationStructureKHR = ( PFN_vkCmdCopyAccelerationStructureKHR ) vkGetDeviceProcAddr( device, "vkCmdCopyAccelerationStructureKHR" );

	vkCmdCopyAccelerationStructureToMemoryKHR = ( PFN_vkCmdCopyAccelerationStructureToMemoryKHR ) vkGetDeviceProcAddr( device, "vkCmdCopyAccelerationStructureToMemoryKHR" );

	vkCmdCopyMemoryToAccelerationStructureKHR = ( PFN_vkCmdCopyMemoryToAccelerationStructureKHR ) vkGetDeviceProcAddr( device, "vkCmdCopyMemoryToAccelerationStructureKHR" );

	vkGetAccelerationStructureDeviceAddressKHR = ( PFN_vkGetAccelerationStructureDeviceAddressKHR ) vkGetDeviceProcAddr( device, "vkGetAccelerationStructureDeviceAddressKHR" );

	vkCmdWriteAccelerationStructuresPropertiesKHR = ( PFN_vkCmdWriteAccelerationStructuresPropertiesKHR ) vkGetDeviceProcAddr( device, "vkCmdWriteAccelerationStructuresPropertiesKHR" );

	vkGetDeviceAccelerationStructureCompatibilityKHR = ( PFN_vkGetDeviceAccelerationStructureCompatibilityKHR ) vkGetDeviceProcAddr( device, "vkGetDeviceAccelerationStructureCompatibilityKHR" );

	vkGetAccelerationStructureBuildSizesKHR = ( PFN_vkGetAccelerationStructureBuildSizesKHR ) vkGetDeviceProcAddr( device, "vkGetAccelerationStructureBuildSizesKHR" );

	vkCmdTraceRaysKHR = ( PFN_vkCmdTraceRaysKHR ) vkGetDeviceProcAddr( device, "vkCmdTraceRaysKHR" );

	vkCreateRayTracingPipelinesKHR = ( PFN_vkCreateRayTracingPipelinesKHR ) vkGetDeviceProcAddr( device, "vkCreateRayTracingPipelinesKHR" );

	vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = ( PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR ) vkGetDeviceProcAddr( device, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR" );

	vkCmdTraceRaysIndirectKHR = ( PFN_vkCmdTraceRaysIndirectKHR ) vkGetDeviceProcAddr( device, "vkCmdTraceRaysIndirectKHR" );

	vkGetRayTracingShaderGroupStackSizeKHR = ( PFN_vkGetRayTracingShaderGroupStackSizeKHR ) vkGetDeviceProcAddr( device, "vkGetRayTracingShaderGroupStackSizeKHR" );

	vkCmdSetRayTracingPipelineStackSizeKHR = ( PFN_vkCmdSetRayTracingPipelineStackSizeKHR ) vkGetDeviceProcAddr( device, "vkCmdSetRayTracingPipelineStackSizeKHR" );

	vkCmdDrawMeshTasksEXT = ( PFN_vkCmdDrawMeshTasksEXT ) vkGetDeviceProcAddr( device, "vkCmdDrawMeshTasksEXT" );

	vkCmdDrawMeshTasksIndirectEXT = ( PFN_vkCmdDrawMeshTasksIndirectEXT ) vkGetDeviceProcAddr( device, "vkCmdDrawMeshTasksIndirectEXT" );

	vkCmdDrawMeshTasksIndirectCountEXT = ( PFN_vkCmdDrawMeshTasksIndirectCountEXT ) vkGetDeviceProcAddr( device, "vkCmdDrawMeshTasksIndirectCountEXT" );

#if defined( VK_ENABLE_BETA_EXTENSIONS )
	vkCreateExecutionGraphPipelinesAMDX = ( PFN_vkCreateExecutionGraphPipelinesAMDX ) vkGetDeviceProcAddr( device, "vkCreateExecutionGraphPipelinesAMDX" );

	vkGetExecutionGraphPipelineScratchSizeAMDX = ( PFN_vkGetExecutionGraphPipelineScratchSizeAMDX ) vkGetDeviceProcAddr( device, "vkGetExecutionGraphPipelineScratchSizeAMDX" );

	vkGetExecutionGraphPipelineNodeIndexAMDX = ( PFN_vkGetExecutionGraphPipelineNodeIndexAMDX ) vkGetDeviceProcAddr( device, "vkGetExecutionGraphPipelineNodeIndexAMDX" );

	vkCmdInitializeGraphScratchMemoryAMDX = ( PFN_vkCmdInitializeGraphScratchMemoryAMDX ) vkGetDeviceProcAddr( device, "vkCmdInitializeGraphScratchMemoryAMDX" );

	vkCmdDispatchGraphAMDX = ( PFN_vkCmdDispatchGraphAMDX ) vkGetDeviceProcAddr( device, "vkCmdDispatchGraphAMDX" );

	vkCmdDispatchGraphIndirectAMDX = ( PFN_vkCmdDispatchGraphIndirectAMDX ) vkGetDeviceProcAddr( device, "vkCmdDispatchGraphIndirectAMDX" );

	vkCmdDispatchGraphIndirectCountAMDX = ( PFN_vkCmdDispatchGraphIndirectCountAMDX ) vkGetDeviceProcAddr( device, "vkCmdDispatchGraphIndirectCountAMDX" );

	vkCreateCudaModuleNV = ( PFN_vkCreateCudaModuleNV ) vkGetDeviceProcAddr( device, "vkCreateCudaModuleNV" );

	vkGetCudaModuleCacheNV = ( PFN_vkGetCudaModuleCacheNV ) vkGetDeviceProcAddr( device, "vkGetCudaModuleCacheNV" );

	vkCreateCudaFunctionNV = ( PFN_vkCreateCudaFunctionNV ) vkGetDeviceProcAddr( device, "vkCreateCudaFunctionNV" );

	vkDestroyCudaModuleNV = ( PFN_vkDestroyCudaModuleNV ) vkGetDeviceProcAddr( device, "vkDestroyCudaModuleNV" );

	vkDestroyCudaFunctionNV = ( PFN_vkDestroyCudaFunctionNV ) vkGetDeviceProcAddr( device, "vkDestroyCudaFunctionNV" );

	vkCmdCudaLaunchKernelNV = ( PFN_vkCmdCudaLaunchKernelNV ) vkGetDeviceProcAddr( device, "vkCmdCudaLaunchKernelNV" );

#endif

#if defined( VK_USE_PLATFORM_WIN32_KHR )
	vkGetMemoryWin32HandleKHR = ( PFN_vkGetMemoryWin32HandleKHR ) vkGetDeviceProcAddr( device, "vkGetMemoryWin32HandleKHR" );

	vkGetMemoryWin32HandlePropertiesKHR = ( PFN_vkGetMemoryWin32HandlePropertiesKHR ) vkGetDeviceProcAddr( device, "vkGetMemoryWin32HandlePropertiesKHR" );

	vkImportSemaphoreWin32HandleKHR = ( PFN_vkImportSemaphoreWin32HandleKHR ) vkGetDeviceProcAddr( device, "vkImportSemaphoreWin32HandleKHR" );

	vkGetSemaphoreWin32HandleKHR = ( PFN_vkGetSemaphoreWin32HandleKHR ) vkGetDeviceProcAddr( device, "vkGetSemaphoreWin32HandleKHR" );

	vkImportFenceWin32HandleKHR = ( PFN_vkImportFenceWin32HandleKHR ) vkGetDeviceProcAddr( device, "vkImportFenceWin32HandleKHR" );

	vkGetFenceWin32HandleKHR = ( PFN_vkGetFenceWin32HandleKHR ) vkGetDeviceProcAddr( device, "vkGetFenceWin32HandleKHR" );

	vkGetMemoryWin32HandleNV = ( PFN_vkGetMemoryWin32HandleNV ) vkGetDeviceProcAddr( device, "vkGetMemoryWin32HandleNV" );

	vkAcquireFullScreenExclusiveModeEXT = ( PFN_vkAcquireFullScreenExclusiveModeEXT ) vkGetDeviceProcAddr( device, "vkAcquireFullScreenExclusiveModeEXT" );

	vkReleaseFullScreenExclusiveModeEXT = ( PFN_vkReleaseFullScreenExclusiveModeEXT ) vkGetDeviceProcAddr( device, "vkReleaseFullScreenExclusiveModeEXT" );

	vkGetDeviceGroupSurfacePresentModes2EXT = ( PFN_vkGetDeviceGroupSurfacePresentModes2EXT ) vkGetDeviceProcAddr( device, "vkGetDeviceGroupSurfacePresentModes2EXT" );

#endif

#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
#endif

#if defined( VK_USE_PLATFORM_XLIB_KHR )
#endif

#if defined( VK_USE_PLATFORM_XLIB_XRANDR_EXT )
#endif

}