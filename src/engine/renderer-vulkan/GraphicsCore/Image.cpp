/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2026 Daemon Developers
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
// Image.cpp

#include "../Math/Bit.h"

#include "Queue.h"

#include "Memory/EngineAllocator.h"
#include "ResourceSystem.h"

#include "GraphicsCoreStore.h"

#include "Image.h"

void Image::Init( const Format newFormat, const VkExtent3D imageSize, const bool useMipLevels, const ImageUsage::ImageUsage usage,
                  const bool newCube, const bool newDepthStencil, const bool shared ) {
	type         = cube ? VK_IMAGE_VIEW_TYPE_CUBE : ( imageSize.depth ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D );
	format       = newFormat;

	mipLevels    = useMipLevels
	               ? log2f( std::max( std::max( imageSize.width, imageSize.height ), imageSize.depth ) ) + 1
	               : 1;

	cube         = newCube;
	storage      = usage & ImageUsage::ATTACHMENT | usage & ImageUsage::STORAGE;
	depthStencil = newDepthStencil;

	external     = false;

	VkImageUsageFlags imageUsage = 0;

	if ( usage & ImageUsage::ATTACHMENT ) {
		imageUsage |= depthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if ( usage & ImageUsage::STORAGE ) {
		imageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if ( usage & ImageUsage::RESOURCE ) {
		imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	VkImageCreateFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

	if ( usage & ImageUsage::COMPRESSED_VIEW ) {
		flags = VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
	}

	uint32           queueCount;
	Array<uint32, 4> concurrentQueues = GetConcurrentQueues( &queueCount );

	VkImageCreateInfo imageInfo {
		.flags                 = flags,
		.imageType             = imageSize.depth ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D,
		.format                = formats[format],
		.extent                = imageSize,
		.mipLevels             = mipLevels,
		.arrayLayers           = cube ? 6u : 1u,
		.samples               = VK_SAMPLE_COUNT_1_BIT,
		.tiling                = VK_IMAGE_TILING_OPTIMAL,
		.usage                 = imageUsage,
		.sharingMode           = shared ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = queueCount,
		.pQueueFamilyIndices   = concurrentQueues.memory,
		.initialLayout         = engineAllocator.zeroInitMemory ? VK_IMAGE_LAYOUT_ZERO_INITIALIZED_EXT : VK_IMAGE_LAYOUT_UNDEFINED
	};

	vkCreateImage( device, &imageInfo, nullptr, &image );

	MemoryRequirements reqs = GetImageRequirements( imageInfo );

	resourceSystem.AllocImage( reqs, image );
}

void Image::Init( VkImage newImage, const SwapChainFormat newFormat ) {
	image        = newImage;
	type         = VK_IMAGE_VIEW_TYPE_2D;

	Format formatFromSwapChainFormat[] {
		RGBA8,     // S_RGBA8
		RGBA8S,    // S_RGBA8S
		ABGR_2_10, // S_ABGR_2_10
	};

	format       = formatFromSwapChainFormat[newFormat];

	mipLevels    = 1;

	cube         = false;
	storage      = false;
	depthStencil = false;

	external     = true;
}

VkImageView Image::GenView() {
	VkImageViewCreateInfo imageViewInfo {
		.image              = image,
		.viewType           = type,
		.format             = formats[format],
		.subresourceRange   = {
			.aspectMask     = ( VkImageAspectFlags ) ( depthStencil ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT ),
			.baseMipLevel   = 0,
			.levelCount     = mipLevels,
			.baseArrayLayer = 0,
			.layerCount     = cube ? 6u : 1u
		}
	};

	VkImageView view;
	vkCreateImageView( device, &imageViewInfo, nullptr, &view );

	return view;
}

FormatConfig formatConfigs[FORMAT_COUNT]                    {};
FormatConfig swapchainFormatConfigs[SWAPCHAIN_FORMAT_COUNT] {};

static FormatConfig GetFormatConfig( const VkFormat format, const VkImageUsageFlags usage, const VkImageCreateFlags flags ) {
	VkPhysicalDeviceImageFormatInfo2 formatInfo {
		.format = format,
		.type   = VK_IMAGE_TYPE_2D,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage  = usage,
		.flags  = flags
	};

	VkHostImageCopyDevicePerformanceQuery hostImageCopyInfo    {};
	VkImageFormatProperties2              formatPropertiesInfo { .pNext = resourceSystem.hostImageCopy ? &hostImageCopyInfo : nullptr };

	VkResult res = vkGetPhysicalDeviceImageFormatProperties2( physicalDevice, &formatInfo, &formatPropertiesInfo );

	VkFormatProperties3                   formatProperties3    {};
	VkFormatProperties2                   formatProperties2    { .pNext = &formatProperties3 };

	vkGetPhysicalDeviceFormatProperties2( physicalDevice, format, &formatProperties2 );

	VkImageFormatProperties& formatProperties = formatPropertiesInfo.imageFormatProperties;

	return {
		.maxSize             = formatProperties.maxExtent,
		.maxLayers           = formatProperties.maxArrayLayers,
		.maxSamples          = formatProperties.sampleCounts,

		.hostCopyOptimal     = resourceSystem.hostImageCopy ? ( ( bool ) hostImageCopyInfo.optimalDeviceAccess )   : false,
		.hostIdenticalLayout = resourceSystem.hostImageCopy ? ( ( bool ) hostImageCopyInfo.identicalMemoryLayout ) : false,

		.indirectCopy        = ( bool ) ( formatProperties3.optimalTilingFeatures & VK_FORMAT_FEATURE_2_COPY_IMAGE_INDIRECT_DST_BIT_KHR ),
		.minMaxSampler       = ( bool ) ( formatProperties3.optimalTilingFeatures & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_MINMAX_BIT ),
		.atomicStorage       = ( bool ) ( formatProperties3.optimalTilingFeatures & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_ATOMIC_BIT ),

		.supported           = res == VK_SUCCESS
	};
}

void InitFormatConfigs() {
	for ( FormatConfig& cfg : formatConfigs ) {
		Format format            = ( Format ) ( &cfg - formatConfigs );

		VkImageUsageFlags  usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		VkImageCreateFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		
		if ( format <= FORMAT_COLOUR ) {
			usage |= ( format == RGBA8S ? 0 : VK_IMAGE_USAGE_STORAGE_BIT ) | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		} else if ( format <= FORMAT_DEPTH ) {
			usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		} else if ( format <= FORMAT_DEPTH_STENCIL ) {
			usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		} else {
			usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			flags |= VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
		}

		if ( resourceSystem.hostImageCopy && ( usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT ) ) {
			usage |= VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
		}

		cfg = GetFormatConfig( formats[format], usage, flags );
	}
	
	for ( FormatConfig& cfg : swapchainFormatConfigs ) {
		Format format            = ( Format ) ( &cfg - swapchainFormatConfigs );

		VkImageUsageFlags  usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		VkImageCreateFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

		cfg = GetFormatConfig( swapchainFormats[format], usage, flags );
	}
}