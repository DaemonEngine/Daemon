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
// SwapChain.h

#include <unordered_map>

#include <SDL3/SDL_vulkan.h>

#include "Vulkan.h"

#include "../Surface/Surface.h"

#include "../Memory/DynamicArray.h"

#include "../GraphicsCore/GraphicsCoreStore.h"
#include "../GraphicsCore/GraphicsCoreCVars.h"

#include "SwapChain.h"

struct VkFormatHasher {
	size_t operator()( const VkFormat format ) const {
		return format;
	}
};

static std::unordered_map<VkFormat, int, VkFormatHasher> surfaceFormatPriorities {
	{ VK_FORMAT_UNDEFINED,                -1 },

	{ VK_FORMAT_B8G8R8A8_SRGB,            2 },
	{ VK_FORMAT_B8G8R8A8_UNORM,           1 },
	{ VK_FORMAT_A2B10G10R10_UNORM_PACK32, 0 },
};

static VkSurfaceFormat2KHR SelectSurfaceFormat( DynamicArray<VkSurfaceFormat2KHR>& formats ) {
	VkFormat bestFormat = VK_FORMAT_UNDEFINED;
	VkSurfaceFormat2KHR* bestFmt = &formats[0];

	for ( VkSurfaceFormat2KHR& format : formats ) {
		if ( format.surfaceFormat.colorSpace != VK_COLORSPACE_SRGB_NONLINEAR_KHR ) {
			continue;
		}

		VkFormat surfaceFormat = format.surfaceFormat.format;

		if ( surfaceFormatPriorities.find( surfaceFormat ) == surfaceFormatPriorities.end() ) {
			continue;
		}

		if ( surfaceFormatPriorities[surfaceFormat] > surfaceFormatPriorities[bestFormat] ) {
			bestFormat = surfaceFormat;
			bestFmt = &format;
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

static std::unordered_map<VkPresentModeKHR, uint32, VkPresentModeKHRHasher> presentModePriorities {
	{ VK_PRESENT_MODE_IMMEDIATE_KHR,         4 },
	{ VK_PRESENT_MODE_FIFO_LATEST_READY_KHR, 3 },
	{ VK_PRESENT_MODE_FIFO_RELAXED_KHR,      2 },
	{ VK_PRESENT_MODE_FIFO_KHR,              1 },
	{ VK_PRESENT_MODE_MAILBOX_KHR,           0 },
};

static VkPresentModeKHR SelectPresentMode( DynamicArray<VkPresentModeKHR>& presentModes ) {
	VkPresentModeKHR bestMode = presentModes[0];
	for ( VkPresentModeKHR mode : presentModes ) {
		if ( mode == r_vkPresentMode.Get() ) {
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

	#ifdef _MSC_VER
		VkSurfaceFullScreenExclusiveInfoEXT fullscreenInfo {
			.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT
		};

		surfaceInfo.pNext = &fullscreenInfo;
	#endif

	VkSurfaceCapabilities2KHR capabilities2 {};
	vkGetPhysicalDeviceSurfaceCapabilities2KHR( physicalDevice, &surfaceInfo, &capabilities2 );

	uint32 count;
	vkGetPhysicalDeviceSurfaceFormats2KHR( physicalDevice, &surfaceInfo, &count, nullptr );

	DynamicArray<VkSurfaceFormat2KHR> formats;
	formats.Resize( count );

	vkGetPhysicalDeviceSurfaceFormats2KHR( physicalDevice, &surfaceInfo, &count, formats.memory );

	VkSurfaceFormat2KHR format = SelectSurfaceFormat( formats );

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
	imageCount =
		capabilities.minImageCount > 2
		?   capabilities.minImageCount
		: ( 2 <= capabilities.maxImageCount ? 2 : capabilities.maxImageCount );

	minImages = capabilities.minImageCount;
	maxImages = capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR swapChainInfo {
		.flags = 0,

		.surface = surface,

		.minImageCount   = imageCount,
		.imageFormat     = format.surfaceFormat.format,
		.imageColorSpace = format.surfaceFormat.colorSpace,

		.imageExtent      = capabilities.minImageExtent,
		.imageArrayLayers = 1,
		.imageUsage       = capabilities.supportedUsageFlags,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,

		.preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,

		.presentMode = presentMode,

		.clipped = VK_TRUE,

		.oldSwapchain = nullptr
	};

	vkCreateSwapchainKHR( device, &swapChainInfo, nullptr, &swapChain );
}

void SwapChain::Free() {
	vkDestroySwapchainKHR( device, swapChain, nullptr );
}