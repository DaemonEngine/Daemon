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

#include "QueuesConfig.h"

#include "Memory/EngineAllocator.h"
#include "ResourceSystem.h"

#include "GraphicsCoreStore.h"

#include "Image.h"

void Image::Init( VkFormat newFormat, VkExtent3D imageSize, const bool useMipLevels, const ImageUsage::ImageUsage usage,
                  bool newCube, bool newDepthStencil, bool shared ) {
	type         = cube ? VK_IMAGE_VIEW_TYPE_CUBE : ( imageSize.depth ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D );
	format       = newFormat;

	mipLevels    = useMipLevels
	               ? log2f( std::max( std::max( imageSize.width, imageSize.height ), imageSize.depth ) ) + 1
	               : 1;

	cube         = newCube;
	depthStencil = newDepthStencil;

	external     = false;

	VkImageUsageFlags imageUsage = 0;

	if ( usage & ImageUsage::ATTACHMENT ) {
		imageUsage |= depthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if ( usage & ImageUsage::SAMPLED ) {
		imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
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
		.format                = format,
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

	uint64 offset;
	uint64 size;
	resourceSystem.AllocImage( reqs, image, &offset, &size );
}

void Image::Init( VkImage newImage, VkFormat newFormat, const ImageUsage::ImageUsage usage ) {
	image        = newImage;
	type         = VK_IMAGE_VIEW_TYPE_2D;
	format       = newFormat;

	mipLevels    = 1;

	cube         = false;
	depthStencil = false;

	external     = true;
}

VkImageView Image::GenView() {
	VkImageViewCreateInfo imageViewInfo {
		.image              = image,
		.viewType           = type,
		.format             = format,
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
