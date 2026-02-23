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

#include "common/Common.h"

#include "../../Math/NumberTypes.h"
#include "../../Math/Bit.h"

#include "../Vulkan.h"

#include "../GraphicsCoreStore.h"
#include "../ResultCheck.h"

#include "../Image.h"

#include "../../GraphicsShared/Bindings.h"

#include "DescriptorSet.h"

static VkDescriptorPool descriptorPool;

enum AddressMode : uint32 {
	REPEAT,
	CLAMP_TO_EDGE,
	CLAMP_TO_BORDER,
	MIRROR_CLAMP_TO_EDGE,
	MAX_ADDRESS_MODES
};

enum ReductionMode : uint32 {
	AVG,
	MIN,
	MAX,
	MAX_REDUCTION_MODES
};

enum BorderColor : uint32 {
	BORDER_WHITE,
	BORDER_BLACK,
	BORDER_BLACK_TRANSPARENT,
	MAX_BORDER_COLOURS
};

struct Sampler {
	AddressMode   addressModeU;
	AddressMode   addressModeV;
	AddressMode   addressModeW;
	ReductionMode reductionMode;
	float         anisotropy;
	bool          shadowMap;
	BorderColor   borderColour;
};

static constexpr uint32 maxSamplers = MAX_ADDRESS_MODES * MAX_ADDRESS_MODES
                                    * 2  // anisotropy
	                                * MAX_BORDER_COLOURS
	                                + MAX_REDUCTION_MODES
                                    * 2; // shadowmap

static VkSampler        samplers[maxSamplers];

static Sampler UnpackSampler( const uint8 sampler ) {
	if ( GetBits( sampler, 0, 1 ) ) {
		return {
			.addressModeU  = REPEAT,
			.addressModeV  = REPEAT,
			.addressModeW  = CLAMP_TO_EDGE,
			.reductionMode = ( ReductionMode ) GetBits( sampler, 3, 2 ),
			.anisotropy    = GetBits( sampler, 1, 1 ) ? 8.0f : 0.0f,
			.shadowMap     = ( bool ) GetBits( sampler, 2, 1 ),
			.borderColour  = BORDER_WHITE
		};
	} else {
		return {
			.addressModeU  = ( AddressMode ) GetBits( sampler, 2, 2 ),
			.addressModeV  = ( AddressMode ) GetBits( sampler, 4, 2 ),
			.addressModeW  = CLAMP_TO_EDGE,
			.reductionMode = AVG,
			.anisotropy    = GetBits( sampler, 1, 1 ) ? 8.0f : 0.0f,
			.shadowMap     = false,
			.borderColour  = ( BorderColor ) GetBits( sampler, 6, 2 )
		};
	}
}

static uint16 PackSampler( const Sampler sampler ) {
	uint16 out = 0;

	SetBits(     &out, sampler.anisotropy,    1, 1 );

	if ( sampler.shadowMap || sampler.reductionMode ) {
		SetBits( &out, sampler.shadowMap,     2, 1 );
		SetBits( &out, sampler.reductionMode, 3, 2 );
	} else {
		SetBits( &out, sampler.addressModeU,  2, 2 );
		SetBits( &out, sampler.addressModeV,  4, 2 );
		SetBits( &out, sampler.borderColour,  6, 2 );
	}

	return out;
}

static VkSamplerAddressMode GetSamplerAddressMode( const AddressMode addressMode ) {
	switch ( addressMode ) {
		case REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case CLAMP_TO_EDGE:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case CLAMP_TO_BORDER:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case MIRROR_CLAMP_TO_EDGE:
			return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	}
}

VkSampler CreateSampler( const Sampler sampler ) {
	VkSamplerReductionMode samplerReductionMode;

	switch ( sampler.reductionMode ) {
		default:
		case AVG:
			samplerReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
			break;
		case MIN:
			samplerReductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;
			break;
		case MAX:
			samplerReductionMode = VK_SAMPLER_REDUCTION_MODE_MAX;
			break;
	}

	VkBorderColor border = sampler.borderColour == BORDER_WHITE
	                       ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
	                       : ( sampler.borderColour == BORDER_BLACK ? VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK : VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK );

	VkSamplerReductionModeCreateInfo samplerReductionInfo {
		.reductionMode    = samplerReductionMode
	};

	VkSamplerCreateInfo samplerInfo  {
		.pNext            = &samplerReductionInfo,
		.magFilter        = VK_FILTER_LINEAR,
		.minFilter        = VK_FILTER_LINEAR,
		.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU     = GetSamplerAddressMode( sampler.addressModeU ),
		.addressModeV     = GetSamplerAddressMode( sampler.addressModeV ),
		.addressModeW     = GetSamplerAddressMode( sampler.addressModeW ),
		.anisotropyEnable = sampler.anisotropy ? VK_TRUE : VK_FALSE,
		.maxAnisotropy    = sampler.anisotropy,
		.compareEnable    = sampler.shadowMap  ? VK_TRUE : VK_FALSE,
		.compareOp        = VK_COMPARE_OP_GREATER_OR_EQUAL,
		.maxLod           = VK_LOD_CLAMP_NONE,
		.borderColor      = border
	};

	VkSampler out;
	vkCreateSampler( device, &samplerInfo, nullptr, &out );

	return out;
}

void AllocDescriptors( uint32 imageCount, uint32 storageImageCount ) {
	const uint32 initialImageCount        = imageCount;
	const uint32 initialStorageImageCount = storageImageCount;

	while ( true ) {
		VkDescriptorPoolSize imagePools[] {
			{
				.type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = storageImageCount
			},
			{
				.type            = VK_DESCRIPTOR_TYPE_SAMPLER,
				.descriptorCount = maxSamplers
			},
			{
				.type            = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				.descriptorCount = imageCount
			}
		};

		VkDescriptorPoolCreateInfo info {
			.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			.maxSets       = 1,
			.poolSizeCount = 3,
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
			.binding         = BIND_STORAGE_IMAGES,
			.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = storageImageCount,
			.stageFlags      = VK_SHADER_STAGE_ALL
		},
		{
			.binding         = BIND_SAMPLERS,
			.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = imageCount,
			.stageFlags      = VK_SHADER_STAGE_ALL
		},
		{
			.binding         = BIND_IMAGES,
			.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptorCount = imageCount,
			.stageFlags      = VK_SHADER_STAGE_ALL
		}
	};

	constexpr VkDescriptorBindingFlags descriptorFlags =
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
		VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT |
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

	constexpr VkDescriptorBindingFlags descriptorFlagsArray[] { descriptorFlags, descriptorFlags, descriptorFlags };

	VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorIndexingInfo {
		.bindingCount  = 3,
		.pBindingFlags = descriptorFlagsArray
	};

	VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo {
		.pNext        = &descriptorIndexingInfo,
		.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.bindingCount = 3,
		.pBindings    = imageBinds
	};

	ResultCheck( vkCreateDescriptorSetLayout( device, &descriptorLayoutInfo, nullptr, &descriptorSetLayout ) );

	VkDescriptorSetAllocateInfo allocInfo {
		.descriptorPool     = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts        = &descriptorSetLayout
	};

	ResultCheck( vkAllocateDescriptorSets( device, &allocInfo, &descriptorSet ) );

	VkDescriptorImageInfo samplerInfos[maxSamplers] {};

	for ( uint8 sampler = 0; sampler < maxSamplers; sampler++ ) {
		samplers[sampler] = CreateSampler( UnpackSampler( sampler ) );

		samplerInfos[sampler]           = {
			.sampler = samplers[sampler]
		};
	}

	VkWriteDescriptorSet writeDescriptorInfo {
		.dstSet          = descriptorSet,
		.dstBinding      = BIND_SAMPLERS,
		.dstArrayElement = 0,
		.descriptorCount = maxSamplers,
		.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
		.pImageInfo      = samplerInfos
	};

	vkUpdateDescriptorSets( device, 1, &writeDescriptorInfo, 0, nullptr );
}

void UpdateDescriptor( const uint32 id, Image image, Format format ) {
	VkDescriptorImageInfo imageDescriptorInfo {
		.imageView   = image.GenView( format ),
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
	};

	VkWriteDescriptorSet writeDescriptorInfo {
		.dstSet          = descriptorSet,
		.dstBinding      = image.storage ? BIND_STORAGE_IMAGES              : BIND_IMAGES,
		.dstArrayElement = id,
		.descriptorCount = 1,
		.descriptorType  = image.storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo      = &imageDescriptorInfo
	};

	vkUpdateDescriptorSets( device, 1, &writeDescriptorInfo, 0, nullptr );
}

void FreeDescriptors() {
	ResultCheck( vkResetDescriptorPool( device, descriptorPool, 0 ) );
}