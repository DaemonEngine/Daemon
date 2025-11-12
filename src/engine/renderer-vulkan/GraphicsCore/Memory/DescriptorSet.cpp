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
// DescriptorSet.cpp

#include "common/Log.h"

#include "../Vulkan.h"

#include "../GraphicsCoreStore.h"
#include "../ResultCheck.h"

#include "../../GraphicsShared/Bindings.h"

#include "DescriptorSet.h"

static VkDescriptorPool descriptorPool;

void AllocDescriptors( uint32 imageCount, uint32 storageImageCount ) {
	const uint32 initialImageCount        = imageCount;
	const uint32 initialStorageImageCount = storageImageCount;

	while ( true ) {
		VkDescriptorPoolSize imagePools[] {
			{
				.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = imageCount
			},
			{
				.type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = storageImageCount
			}
		};

		VkDescriptorPoolCreateInfo info {
			.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			.maxSets       = 1,
			.poolSizeCount = 2,
			.pPoolSizes    = imagePools
		};

		ResultCheckExt( vkCreateDescriptorPool( device, &info, nullptr, &descriptorPool ), VK_ERROR_FRAGMENTATION );

		// This can happen on Intel
		if ( resultCheck == VK_ERROR_FRAGMENTATION ) {
			imageCount        >>= 2;
			storageImageCount >>= 2;
		} else {
			if ( imageCount != initialImageCount ) {
				Log::Notice( "Decreasing descriptor size due to memory fragmentation "
					"(sampled images: %u -> %u, storage images: %u -> %u",
					initialImageCount, imageCount, initialStorageImageCount, storageImageCount );
			}
			break;
		}

		static constexpr uint32 minImages = 16384;

		if ( imageCount < minImages || storageImageCount < minImages ) {
			Err( "DescriptorPool: not enough or fragmented memory for min images: %u sampled and storage", minImages );
			return;
		}
	}

	VkDescriptorSetLayoutBinding imageBinds[] {
		{
			.binding         = BIND_IMAGES,
			.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = imageCount,
			.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT
		},
		{
			.binding         = BIND_STORAGE_IMAGES,
			.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = storageImageCount,
			.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT
		}
	};

	constexpr VkDescriptorBindingFlags descriptorFlags =
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
		VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT |
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

	constexpr VkDescriptorBindingFlags descriptorFlagsArray[] { descriptorFlags, descriptorFlags };

	VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorIndexingInfo {
		.bindingCount  = 2,
		.pBindingFlags = descriptorFlagsArray
	};

	VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo {
		.pNext        = &descriptorIndexingInfo,
		.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.bindingCount = 2,
		.pBindings    = imageBinds
	};

	ResultCheck( vkCreateDescriptorSetLayout( device, &descriptorLayoutInfo, nullptr, &descriptorSetLayout ) );

	VkDescriptorSetAllocateInfo allocInfo {
		.descriptorPool     = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts        = &descriptorSetLayout
	};

	ResultCheck( vkAllocateDescriptorSets( device, &allocInfo, &descriptorSet ) );
}

void FreeDescriptors() {
	ResultCheck( vkResetDescriptorPool( device, descriptorPool, 0 ) );
}