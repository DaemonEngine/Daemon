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
// EngineAllocator.cpp

#include "../Vulkan.h"

#include "../../Error.h"

#include "../../Math/Bit.h"

#include "../GraphicsCoreStore.h"

#include "../../GraphicsShared/Bindings.h"

#include "EngineAllocator.h"

constexpr VkSampleCountFlags SamplesToEnum( const uint32 samples ) {
	switch ( samples ) {
		case 1:
			return VK_SAMPLE_COUNT_1_BIT;
		case 2:
			return VK_SAMPLE_COUNT_2_BIT;
		case 4:
			return VK_SAMPLE_COUNT_4_BIT;
		case 8:
			return VK_SAMPLE_COUNT_8_BIT;
		case 16:
			return VK_SAMPLE_COUNT_16_BIT;
		case 32:
			return VK_SAMPLE_COUNT_32_BIT;
		case 64:
			return VK_SAMPLE_COUNT_64_BIT;
		default:
			Err( "Image sample count must be one of: 1, 2, 4, 8, 16, 32, 64" );
			return VK_SAMPLE_COUNT_1_BIT;
	}
}

MemoryRequirements GetImageRequirements( const VkImageType type, const VkFormat format, const bool useMipMaps,
	const bool storageImage, const uint32 width, const uint32 height, const uint32 depth, const uint32 layers,
	const uint32 samples ) {
	const uint32 mips = useMipMaps ? log2f( std::max( std::max( width, height ), depth ) ) + 1 : 1;

	VkImageCreateInfo imageInfo {
		.imageType     = type,
		.format        = format,
		.extent        = { width, height, depth },
		.mipLevels     = mips,
		.arrayLayers   = layers,
		.samples       = ( VkSampleCountFlagBits ) SamplesToEnum( samples ),
		.tiling        = VK_IMAGE_TILING_OPTIMAL,
		.usage         = ( VkFlags ) ( storageImage ? VK_IMAGE_USAGE_STORAGE_BIT : VK_IMAGE_USAGE_SAMPLED_BIT ),
		.sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED
	};

	VkDeviceImageMemoryRequirements reqs {
		.pCreateInfo = &imageInfo
	};

	VkMemoryDedicatedRequirements dedicatedReqs {};

	VkMemoryRequirements2 out { .pNext = &dedicatedReqs };

	vkGetDeviceImageMemoryRequirements( device, &reqs, &out );

	return {
		out.memoryRequirements.size, out.memoryRequirements.alignment, out.memoryRequirements.memoryTypeBits,
		( bool ) dedicatedReqs.prefersDedicatedAllocation
	};
}

MemoryRequirements GetImage2DRequirements( const VkFormat format, const bool useMipMaps,
	const bool storageImage, const uint32 width, const uint32 height ) {
	return GetImageRequirements( VK_IMAGE_TYPE_2D, format, useMipMaps, storageImage, width, height, 0, 1, 1 );
}

MemoryRequirements GetImage3DRequirements( const VkFormat format, const bool useMipMaps,
	const bool storageImage, const uint32 width, const uint32 depth, const uint32 height ) {
	return GetImageRequirements( VK_IMAGE_TYPE_2D, format, useMipMaps, storageImage, width, height, depth, 1, 1 );
}

MemoryRequirements GetBufferRequirements( const VkBufferUsageFlags type, const uint64 size ) {
	VkBufferCreateInfo bufferInfo {
		.size        = size,
		.usage       = type,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VkDeviceBufferMemoryRequirements reqs2 {
		.pCreateInfo = &bufferInfo
	};

	VkMemoryRequirements2 out {};

	vkGetDeviceBufferMemoryRequirements( device, &reqs2, &out );

	return {
		out.memoryRequirements.size, out.memoryRequirements.alignment, out.memoryRequirements.memoryTypeBits
	};
}

static constexpr VkMemoryPropertyFlags memoryTypeGPU =
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
static constexpr VkMemoryPropertyFlags memoryTypeBAR =
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
static constexpr VkMemoryPropertyFlags memoryTypeGPUToCPU =
	VK_MEMORY_PROPERTY_HOST_CACHED_BIT  | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

static constexpr VkMemoryPropertyFlags memoryTypeUnified = 
	  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT  | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

MemoryHeap EngineAllocator::MemoryHeapForUsage( const MemoryHeap::MemoryType type, const MemoryRequirements& reqs ) {
	VkPhysicalDeviceMemoryBudgetPropertiesEXT properties {};
	VkPhysicalDeviceMemoryProperties2 properties2 {
		.pNext = &properties
	};

	vkGetPhysicalDeviceMemoryProperties2( physicalDevice, &properties2 );

	VkPhysicalDeviceMemoryProperties& memoryProperties = properties2.memoryProperties;

	uint32 memoryRegion;
	uint32 memoryID;

	if ( unifiedMemory ) {
		memoryRegion = memoryRegionEngine;
		memoryID     = memoryIDEngine;
	} else {
		switch ( type ) {
			default:
			case MemoryHeap::ENGINE:
				memoryRegion = memoryRegionEngine;
				memoryID     = memoryIDEngine;
				break;
			case MemoryHeap::CORE_TO_ENGINE:
				memoryRegion = memoryRegionBAR;
				memoryID     = memoryIDCoreToEngine;
				break;
			case MemoryHeap::ENGINE_TO_CORE:
				memoryRegion = memoryRegionCore;
				memoryID     = memoryIDEngineToCore;
				break;
		}
	}

	MemoryHeap memoryHeap {
		properties.heapBudget[memoryRegion] - properties.heapUsage[memoryRegion],
		properties.heapBudget[memoryRegion],
		type
	};

	uint32 supportedTypes = reqs.type;

	while ( supportedTypes ) {
		const uint32 id = FindLSB( supportedTypes );
		const VkMemoryType& memoryType = memoryProperties.memoryTypes[id];

		if ( !BitSet( memoryID, id ) ) {
			UnSetBit( &supportedTypes, id );
			continue;
		}

		if ( type == MemoryHeap::ENGINE         && memoryType.propertyFlags & memoryTypeGPU ||
		     type == MemoryHeap::CORE_TO_ENGINE && memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		  || type == MemoryHeap::ENGINE_TO_CORE && memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
			memoryHeap.id = id;

			return memoryHeap;
		}

		UnSetBit( &supportedTypes, id );
	}

	Err( "No suitable MemoryHeap found" );
	return memoryHeap;
}

void EngineAllocator::Init() {
	memoryPoolCount = 0;

	VkPhysicalDeviceMemoryBudgetPropertiesEXT properties {};
	VkPhysicalDeviceMemoryProperties2 properties2 {
		.pNext = &properties
	};

	vkGetPhysicalDeviceMemoryProperties2( physicalDevice, &properties2 );

	VkPhysicalDeviceMemoryProperties& memoryProperties = properties2.memoryProperties;

	memoryRegionEngine = UINT_MAX;
	memoryRegionBAR    = UINT_MAX;
	memoryRegionCore   = UINT_MAX;

	memoryIDEngine       = 0;
	memoryIDCoreToEngine = 0;
	memoryIDEngineToCore = 0;

	if ( memoryProperties.memoryHeapCount == 1 ) {
		unifiedMemory = true;
		rebar = false;

		for ( uint32 i = 0; i < memoryProperties.memoryTypeCount; i++ ) {
			const VkMemoryType& memoryType = memoryProperties.memoryTypes[i];

			if ( memoryType.propertyFlags & memoryTypeUnified ) {
				memoryIDEngine = i;
				break;
			}
		}

		if ( memoryIDEngine == UINT_MAX ) {
			Err( "Couldn't find memory type for ENGINE" );
		}

		memoryTypeEngine       = memoryTypeUnified;
		memoryTypeCoreToEngine = memoryTypeUnified;
		memoryTypeEngineToCore = memoryTypeUnified;

		memoryIDCoreToEngine = memoryIDEngine;
		memoryIDEngineToCore = memoryIDEngine;
	} else {
		for ( uint32 i = 0; i < memoryProperties.memoryHeapCount; i++ ) {
			const VkMemoryHeap& memoryRegion = memoryProperties.memoryHeaps[i];
			
			for ( uint32 j = 0; j < memoryProperties.memoryTypeCount; j++ ) {
				const VkMemoryType& memoryType = memoryProperties.memoryTypes[j];

				if ( memoryType.heapIndex != i ) {
					continue;
				}

				if ( memoryRegion.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ) {
					if ( memoryType.propertyFlags & memoryTypeGPU
						&& !( memoryType.propertyFlags & ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
							| VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) ) ) {
						if ( memoryRegionEngine == UINT_MAX ) {
							memoryRegionEngine = i;
						}

						SetBit( &memoryIDEngine, j );
						continue;
					}

					if ( memoryType.propertyFlags & memoryTypeBAR
						&& !( memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) ) {
						if ( memoryRegionBAR == UINT_MAX ) {
							memoryRegionBAR = i;
						}

						SetBit( &memoryIDCoreToEngine, j );
						continue;
					}
				} else {
					if ( memoryType.propertyFlags & memoryTypeGPUToCPU ) {
						if ( memoryRegionCore == UINT_MAX ) {
							memoryRegionCore = i;
						}

						SetBit( &memoryIDEngineToCore, j );
						continue;
					}
				}
			}
		}

		if ( memoryRegionCore   == UINT_MAX ) {
			Err( "Couldn't find memory type for ENGINE_TO_CORE" );
		}

		if ( memoryRegionEngine == UINT_MAX ) {
			Err( "Couldn't find memory type for ENGINE" );
		}

		if ( memoryRegionBAR    == UINT_MAX ) {
			Err( "Couldn't find memory type for CORE_TO_ENGINE" );
		}

		if ( memoryRegionEngine == memoryRegionBAR ) {
			rebar = true;
		}

		memoryTypeEngine       = memoryTypeGPU;
		memoryTypeCoreToEngine = memoryTypeBAR;
		memoryTypeEngineToCore = memoryTypeGPUToCPU;
	}

	MemoryRequirements reqs;
	reqs = GetImage2DRequirements( VK_FORMAT_R8G8B8A8_UNORM, true, false, 1024, 1024 );
	memoryHeapImages        = MemoryHeapForUsage( MemoryHeap::ENGINE, reqs );

	reqs = GetImage2DRequirements( VK_FORMAT_R8G8B8A8_UNORM, false, true, 1024, 1024 );
	memoryHeapStorageImages = MemoryHeapForUsage( MemoryHeap::ENGINE, reqs );

	reqs = GetBufferRequirements( VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 262144 );
	memoryHeapStagingBuffer = MemoryHeapForUsage( MemoryHeap::CORE_TO_ENGINE, reqs );
}

void EngineAllocator::Free() {
	for ( MemoryPool* memoryPool = memoryPools; memoryPool < memoryPools + memoryPoolCount; memoryPool++ ) {
		vkFreeMemory( device, ( VkDeviceMemory ) memoryPool->memory, nullptr );
	}
}