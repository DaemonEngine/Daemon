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

#include "../../Memory/Array.h"

#include "../GraphicsCoreStore.h"
#include "../QueuesConfig.h"

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

MemoryRequirements GetBufferRequirements( const VkBufferUsageFlags usage, const uint64 size ) {
	uint32           queueCount;
	Array<uint32, 4> concurrentQueues = GetConcurrentQueues( &queueCount );

	VkBufferCreateInfo bufferInfo {
		.size                  = size,
		.usage                 = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.sharingMode           = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = queueCount,
		.pQueueFamilyIndices   = concurrentQueues.memory
	};

	VkDeviceBufferMemoryRequirements reqs2 {
		.pCreateInfo = &bufferInfo
	};

	VkMemoryRequirements2 out {};

	vkGetDeviceBufferMemoryRequirements( device, &reqs2, &out );

	return {
		out.memoryRequirements.size,
		out.memoryRequirements.alignment,
		out.memoryRequirements.memoryTypeBits
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

MemoryHeap& EngineAllocator::MemoryHeapFromType( const MemoryHeap::MemoryType type ) {
	switch ( type ) {
		default:
		case MemoryHeap::ENGINE:
			return memoryHeapEngine;
		case MemoryHeap::CORE_TO_ENGINE:
			return memoryHeapCoreToEngine;
		case MemoryHeap::ENGINE_TO_CORE:
			return memoryHeapEngineToCore;
	}
}

MemoryHeap EngineAllocator::MemoryHeapForUsage( const MemoryHeap::MemoryType type, uint32 supportedTypes, const uint32 flags ) {
	VkPhysicalDeviceMemoryBudgetPropertiesEXT properties {};
	VkPhysicalDeviceMemoryProperties2 properties2 {
		.pNext = &properties
	};

	vkGetPhysicalDeviceMemoryProperties2( physicalDevice, &properties2 );

	VkPhysicalDeviceMemoryProperties& memoryProperties = properties2.memoryProperties;

	uint32 memoryRegion;

	if ( unifiedMemory ) {
		memoryRegion = memoryRegionEngine;
	} else {
		switch ( type ) {
			default:
			case MemoryHeap::ENGINE:
				memoryRegion = memoryRegionEngine;
				break;
			case MemoryHeap::CORE_TO_ENGINE:
				memoryRegion = memoryRegionBAR;
				break;
			case MemoryHeap::ENGINE_TO_CORE:
				memoryRegion = memoryRegionCore;
				break;
		}
	}

	MemoryHeap memoryHeap {
		properties.heapBudget[memoryRegion] - properties.heapUsage[memoryRegion],
		properties.heapBudget[memoryRegion],
		type
	};

	while ( supportedTypes ) {
		const uint32 id                = FindLSB( supportedTypes );
		const VkMemoryType& memoryType = memoryProperties.memoryTypes[id];

		if ( !BitSet( flags, id ) ) {
			UnSetBit( &supportedTypes, id );
			continue;
		}

		if ( type == MemoryHeap::ENGINE         && memoryType.propertyFlags & memoryTypeGPU ||
		     type == MemoryHeap::CORE_TO_ENGINE && memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		  || type == MemoryHeap::ENGINE_TO_CORE && memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
			memoryHeap.id    = id;
			memoryHeap.flags = flags;

			return memoryHeap;
		}

		UnSetBit( &supportedTypes, id );
	}

	Err( "No suitable MemoryHeap found" );

	return memoryHeap;
}

MemoryPool EngineAllocator::AllocMemoryPool( const MemoryHeap::MemoryType type, const uint64 size, const bool image, const void* dedicatedResource ) {
	MemoryPool memoryPool {
		.size = size
	};

	VkMemoryAllocateFlagsInfo memoryFlags {
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
	};

	VkMemoryDedicatedAllocateInfo dedicatedMemoryInfo {
		.image  = image ? ( VkImage ) dedicatedResource : nullptr,
		.buffer = image ? nullptr : ( VkBuffer ) dedicatedResource
	};

	if ( dedicatedResource ) {
		// memoryFlags.pNext = &dedicatedMemoryInfo;
	}

	MemoryHeap& heap = MemoryHeapFromType( type );

	VkMemoryAllocateInfo memoryInfo {
		.pNext           = &memoryFlags,
		.allocationSize  = size,
		.memoryTypeIndex = heap.id
	};

	vkAllocateMemory( device, &memoryInfo, nullptr, ( VkDeviceMemory* ) &memoryPool.memory );

	if ( heap.flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
		vkMapMemory( device, ( VkDeviceMemory ) memoryPool.memory, 0, size, 0, ( void** ) &memoryPool.mappedMemory );
	}
	
	return memoryPool;
}

Buffer EngineAllocator::AllocBuffer( const MemoryHeap::MemoryType type, MemoryPool& pool,
	const MemoryRequirements& reqs, const VkBufferUsageFlags usage ) {
	uint32           queueCount;
	Array<uint32, 4> concurrentQueues = GetConcurrentQueues( &queueCount );

	VkBufferCreateInfo bufferInfo {
		.size                  = reqs.size,
		.usage                 = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.sharingMode           = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = queueCount,
		.pQueueFamilyIndices   = concurrentQueues.memory
	};

	if ( reqs.dedicated ) {
		pool = AllocMemoryPool( type, reqs.size, false, nullptr );
	}

	VkBuffer buffer;
	vkCreateBuffer( device, &bufferInfo, nullptr, &buffer );

	uint64 address = ( uint64 ) pool.memory;

	if ( address & ( reqs.alignment - 1 ) ) {
		address = ( address & ~( reqs.alignment - 1 ) ) + reqs.alignment;
	}

	pool.offset += address + reqs.size - ( uint64 ) address;

	VkBindBufferMemoryInfo bindInfo {
		.buffer = buffer,
		.memory = ( VkDeviceMemory ) pool.memory
	};

	vkBindBufferMemory2( device, 1, &bindInfo );

	Buffer res {
		.buffer = buffer,
		.offset = address - ( uint64 ) pool.memory,
		.size   = reqs.size,
		.usage  = bufferInfo.usage
	};

	MemoryHeap& heap = MemoryHeapFromType( type );

	if ( heap.flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
		res.memory = pool.mappedMemory;
	}

	VkBufferDeviceAddressInfo bdaInfo {
		.buffer = buffer
	};

	res.engineMemory = vkGetBufferDeviceAddress( device, &bdaInfo );

	return res;
}

Buffer EngineAllocator::AllocDedicatedBuffer( const MemoryHeap::MemoryType type, const uint64 size, const VkBufferUsageFlags usage ) {
	MemoryPool pool;

	MemoryRequirements reqs = GetBufferRequirements( usage, size );
	reqs.dedicated          = true;

	return AllocBuffer( type, pool, reqs, usage );
}

void EngineAllocator::Init() {
	memoryPoolCount = 0;

	VkPhysicalDeviceMemoryBudgetPropertiesEXT properties  {};
	VkPhysicalDeviceMemoryProperties2         properties2 {
		.pNext = &properties
	};

	vkGetPhysicalDeviceMemoryProperties2( physicalDevice, &properties2 );

	VkPhysicalDeviceMemoryProperties& memoryProperties = properties2.memoryProperties;

	memoryRegionEngine          = UINT_MAX;
	memoryRegionBAR             = UINT_MAX;
	memoryRegionCore            = UINT_MAX;

	uint32 memoryIDEngine       = 0;
	uint32 memoryIDCoreToEngine = 0;
	uint32 memoryIDEngineToCore = 0;

	if ( memoryProperties.memoryHeapCount == 1 ) {
		unifiedMemory = true;
		rebar         = false;

		for ( uint32 i = 0; i < memoryProperties.memoryTypeCount; i++ ) {
			const VkMemoryType& memoryType = memoryProperties.memoryTypes[i];

			if ( ( memoryType.propertyFlags & memoryTypeUnified ) == memoryTypeUnified ) {
				SetBit( &memoryIDEngine, i );
				memoryIDFlags[i] = memoryType.propertyFlags;
				break;
			}
		}

		if ( memoryIDEngine == UINT_MAX ) {
			Err( "Couldn't find memory type for ENGINE" );
		}

		memoryRegionEngine     = 0;
		memoryRegionBAR        = 0;
		memoryRegionCore       = 0;

		memoryIDCoreToEngine   = memoryIDEngine;
		memoryIDEngineToCore   = memoryIDEngine;

		memoryIDFlags[0]       = memoryProperties.memoryTypes[memoryIDEngine].propertyFlags;
	} else {
		for ( uint32 i = 0; i < memoryProperties.memoryHeapCount; i++ ) {
			const VkMemoryHeap& memoryRegion = memoryProperties.memoryHeaps[i];
			
			for ( uint32 j = 0; j < memoryProperties.memoryTypeCount; j++ ) {
				const VkMemoryType& memoryType = memoryProperties.memoryTypes[j];

				if ( memoryType.heapIndex != i ) {
					continue;
				}

				memoryIDFlags[j] = memoryType.propertyFlags;

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
	}

	MemoryRequirements reqs;
	reqs = GetBufferRequirements( VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		| VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, 1024 * 1024 * 1024 );
	
	uint32 supportedTypes = reqs.type;

	reqs = GetImageRequirements( VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, { 1024, 1024 },
		10, 1, true, 1 );
	supportedTypes       &= reqs.type;

	reqs = GetImageRequirements( VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, { 1024, 1024 },
		10, 1, true, 1 );
	supportedTypes       &= reqs.type;

	reqs = GetImageRequirements( VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, { 1024, 1024 },
		10, 1, true, 1 );
	supportedTypes       &= reqs.type;

	memoryHeapEngine       = MemoryHeapForUsage( MemoryHeap::ENGINE, supportedTypes, memoryIDEngine );
	//memoryPoolImages             = AllocMemoryPool();

	reqs = GetBufferRequirements( VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 262144 );
	memoryHeapCoreToEngine = MemoryHeapForUsage( MemoryHeap::CORE_TO_ENGINE, reqs.type, memoryIDCoreToEngine );

	reqs = GetBufferRequirements( VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 262144 );
	memoryHeapEngineToCore = MemoryHeapForUsage( MemoryHeap::ENGINE_TO_CORE, reqs.type, memoryIDEngineToCore );
}

void EngineAllocator::Free() {
	for ( MemoryPool* memoryPool = memoryPools; memoryPool < memoryPools + memoryPoolCount; memoryPool++ ) {
		vkFreeMemory( device, ( VkDeviceMemory ) memoryPool->memory, nullptr );
	}
}