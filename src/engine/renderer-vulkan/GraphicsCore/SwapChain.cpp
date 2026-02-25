/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2025 Daemon Developers
All rights reserved.

This file is part of the Daemon BSD Source Code (Daemon Source Code).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the Daemon developers nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===========================================================================
*/
// SwapChain.cpp

#include <unordered_map>

#include <SDL3/SDL_vulkan.h>

#include "../Surface/Surface.h"

#include "../Memory/DynamicArray.h"

#include "Vulkan.h"

#include "GraphicsCoreStore.h"
#include "GraphicsCoreCVars.h"

#include "SwapChain.h"

struct VkFormatHasher {
	size_t operator()( const VkFormat format ) const {
		return format;
	}
};

static std::unordered_map<VkFormat, int, VkFormatHasher> surfaceFormatPriorities {
	{ VK_FORMAT_UNDEFINED,               -1 },

	{ VK_FORMAT_R8G8B8A8_UNORM,           2 },
	{ VK_FORMAT_A2B10G10R10_UNORM_PACK32, 1 },
	{ VK_FORMAT_R8G8B8A8_SRGB,            0 }
};

static FormatConfig FormatConfigFromFormat( const VkFormat surfaceFormat, SwapChainFormat* format ) {
	switch ( surfaceFormat ) {
		default:
		case VK_FORMAT_R8G8B8A8_UNORM:
			*format = S_RGBA8;
			break;
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
			*format = S_ABGR_2_10;
			break;
		case VK_FORMAT_R8G8B8A8_SRGB:
			*format = S_RGBA8S;
			break;
	}

	return swapchainFormatConfigs[*format];
}

static VkSurfaceFormat2KHR SelectSurfaceFormat( DynamicArray<VkSurfaceFormat2KHR>& formats, SwapChainFormat* format ) {
	VkFormat bestFormat          = VK_FORMAT_UNDEFINED;
	VkSurfaceFormat2KHR* bestFmt = &formats[0];

	for ( VkSurfaceFormat2KHR& srfFormat : formats ) {
		if ( srfFormat.surfaceFormat.colorSpace != VK_COLORSPACE_SRGB_NONLINEAR_KHR ) {
			continue;
		}

		VkFormat surfaceFormat = srfFormat.surfaceFormat.format;

		if ( surfaceFormatPriorities.find( surfaceFormat ) == surfaceFormatPriorities.end() ) {
			continue;
		}

		SwapChainFormat newFormat;
		if ( !FormatConfigFromFormat( surfaceFormat, &newFormat ).supported ) {
			continue;
		}

		if ( surfaceFormatPriorities[surfaceFormat] > surfaceFormatPriorities[bestFormat] ) {
			bestFormat = surfaceFormat;
			bestFmt    = &srfFormat;
			*format    = newFormat;
		}
	}

	if ( bestFormat == VK_FORMAT_UNDEFINED ) {
		Log::Warn( "No suitable SwapChain format found" );
	}

	return *bestFmt;
}

struct VkPresentModeKHRHasher {
	size_t operator()( const VkPresentModeKHR mode ) const {
		return mode;
	}
};

static std::unordered_map<int, VkPresentModeKHR> presentModeMap {
	{ PresentMode::IMMEDIATE,             VK_PRESENT_MODE_IMMEDIATE_KHR         },
	{ PresentMode::SCANOUT_ONE,           VK_PRESENT_MODE_MAILBOX_KHR           },
	{ PresentMode::SCANOUT_FIRST,         VK_PRESENT_MODE_FIFO_KHR              },
	{ PresentMode::SCANOUT_FIRST_RELAXED, VK_PRESENT_MODE_FIFO_RELAXED_KHR      },
	{ PresentMode::SCANOUT_LATEST,        VK_PRESENT_MODE_FIFO_LATEST_READY_KHR },
};

static std::unordered_map<VkPresentModeKHR, uint32, VkPresentModeKHRHasher> presentModePriorities {
	{ VK_PRESENT_MODE_IMMEDIATE_KHR,         4 },
	{ VK_PRESENT_MODE_FIFO_LATEST_READY_KHR, 3 },
	{ VK_PRESENT_MODE_FIFO_RELAXED_KHR,      2 },
	{ VK_PRESENT_MODE_FIFO_KHR,              1 },
	{ VK_PRESENT_MODE_MAILBOX_KHR,           0 },
};

static VkPresentModeKHR SelectPresentMode( DynamicArray<VkPresentModeKHR>& presentModes ) {
	VkPresentModeKHR bestMode   = presentModes[0];

	VkPresentModeKHR customMode = presentModeMap[r_vkPresentMode.Get()];

	for ( VkPresentModeKHR mode : presentModes ) {
		if ( mode == customMode ) {
			return mode;
		}

		if ( presentModePriorities.find( mode ) != presentModePriorities.end()
			&& presentModePriorities[mode] > presentModePriorities[bestMode] ) {
			bestMode = mode;
		}
	}

	return bestMode;
}

void SwapChain::Init( const VkInstance instance ) {
	SDL_Vulkan_CreateSurface( mainSurface.window, instance, nullptr, &surface );

	VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo { .surface = surface };

	/* #ifdef _MSC_VER
		VkSurfaceFullScreenExclusiveWin32InfoEXT fullScreenInfo2 {
			.hmonitor = mainSurface.hmonitor
		};

		VkSurfaceFullScreenExclusiveInfoEXT fullscreenInfo {
			.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT
		};

		surfaceInfo.pNext = &fullscreenInfo;
	#endif */

	VkSurfaceCapabilities2KHR capabilities2 {};
	vkGetPhysicalDeviceSurfaceCapabilities2KHR( physicalDevice, &surfaceInfo, &capabilities2 );

	uint32 count;
	vkGetPhysicalDeviceSurfaceFormats2KHR( physicalDevice, &surfaceInfo, &count, nullptr );

	DynamicArray<VkSurfaceFormat2KHR> surfaceFormats;
	surfaceFormats.Resize( count );
	surfaceFormats.Init();

	vkGetPhysicalDeviceSurfaceFormats2KHR( physicalDevice, &surfaceInfo, &count, surfaceFormats.memory );

	SwapChainFormat format;
	VkSurfaceFormat2KHR surfaceFormat = SelectSurfaceFormat( surfaceFormats, &format );

	vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, surface, &count, nullptr );

	DynamicArray<VkPresentModeKHR> presentModes;
	presentModes.Resize( count );

	vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, surface, &count, presentModes.memory );

	VkPresentModeKHR presentMode = SelectPresentMode( presentModes );

	if ( presentMode != r_vkPresentMode.Get() ) {
		Log::Warn( "Requested present mode %s not available, using %s instead",
			string_VkPresentModeKHR( ( VkPresentModeKHR ) r_vkPresentMode.Get() ), string_VkPresentModeKHR( presentMode ) );
	} else {
		Log::Notice( "Using requested present mode: %s", string_VkPresentModeKHR( presentMode ) );
	}

	VkSurfaceCapabilitiesKHR& capabilities = capabilities2.surfaceCapabilities;
	const uint32 minImageCount =
		capabilities.minImageCount > 2
		? capabilities.minImageCount
		: ( 2 <= capabilities.maxImageCount ? 2 : capabilities.maxImageCount );

	VkExtent2D swapChainSize;

	switch ( r_mode.Get() ) {
		case -2:
		default:
			swapChainSize = capabilities.minImageExtent;
			break;
		case -1:
			swapChainSize = { ( uint32 ) r_customWidth.Get(), ( uint32 ) r_customHeight.Get() };
			break;
	}

	VkImageFormatListCreateInfo swapChainFormatsInfo {
		.viewFormatCount = 1,
		.pViewFormats    = &formats[RGBA8]
	};

	VkSwapchainCreateInfoKHR swapChainInfo {
		.pNext            = surfaceFormat.surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB ? &swapChainFormatsInfo : nullptr,

		.flags            = surfaceFormat.surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB
		                                                        ? VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR
		                                                        : 0u,

		.surface          = surface,

		.minImageCount    = minImageCount,
		.imageFormat      = surfaceFormat.surfaceFormat.format,
		.imageColorSpace  = surfaceFormat.surfaceFormat.colorSpace,

		.imageExtent      = swapChainSize,
		.imageArrayLayers = 1,
		.imageUsage       = capabilities.supportedUsageFlags,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,

		.preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,

		.presentMode      = presentMode,

		.clipped          = VK_TRUE,

		.oldSwapchain     = nullptr
	};

	VkResult res = vkCreateSwapchainKHR( device, &swapChainInfo, nullptr, &swapChain );

	/* #ifdef _MSC_VER
		res = vkAcquireFullScreenExclusiveModeEXT( device, swapChain );

		if ( res == VK_SUCCESS ) {
			Log::Notice( "SwapChain: acquired exclusive fullscreen" );
		}
	#endif */

	res = vkGetSwapchainImagesKHR( device, swapChain, &imageCount, nullptr );

	DynamicArray<VkImage> swapchainImages;
	swapchainImages.Resize( imageCount );
	images.Resize( imageCount );

	presentSemaphores.Resize( imageCount );

	res = vkGetSwapchainImagesKHR( device, swapChain, &imageCount, swapchainImages.memory );

	for ( uint32 i = 0; i < images.size; i++ ) {
		images[i].Init( swapchainImages[i], format );

		VkSemaphoreTypeCreateInfo semaphoreTypeInfo {
			.semaphoreType = VK_SEMAPHORE_TYPE_BINARY
		};

		VkSemaphoreCreateInfo     semaphoreInfo {
			.pNext         = &semaphoreTypeInfo
		};

		vkCreateSemaphore( device, &semaphoreInfo, nullptr, &presentSemaphores[i] );
	}
}

void SwapChain::Free() {
	vkDestroySwapchainKHR( device, swapChain, nullptr );
}

uint32 SwapChain::AcquireNextImage( const uint64 timeout, VkFence fence, VkSemaphore semaphore ) {
	VkAcquireNextImageInfoKHR info {
		.swapchain  = swapChain,
		.timeout    = timeout,
		.semaphore  = semaphore,
		.fence      = fence,
		.deviceMask = 1
	};

	uint32 imageID;
	VkResult res = vkAcquireNextImage2KHR( device, &info, &imageID );

	return imageID;
}
