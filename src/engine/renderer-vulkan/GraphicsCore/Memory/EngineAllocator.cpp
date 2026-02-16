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
#include "../FeaturesConfig.h"
#include "../EngineConfig.h"
#include "../QueuesConfig.h"

#include "../../GraphicsShared/Bindings.h"

#include "EngineAllocator.h"

MemoryRequirements GetImageRequirements( const VkImageCreateInfo& imageInfo ) {
	VkDeviceImageMemoryRequirements reqs {
		.pCreateInfo = &imageInfo
	};

	VkMemoryDedicatedRequirements dedicatedReqs {};

	VkMemoryRequirements2 out { .pNext = &dedicatedReqs };

	vkGetDeviceImageMemoryRequirements( device, &reqs, &out );

	return {
		out.memoryRequirements.size,
		out.memoryRequirements.alignment,
		out.memoryRequirements.memoryTypeBits,
		( bool ) ( dedicatedReqs.requiresDedicatedAllocation | dedicatedReqs.prefersDedicatedAllocation )
	};
}

static MemoryRequirements GetImageRequirements( const VkImageType type, const VkFormat format, const VkImageCreateFlags flags,
	const VkImageUsageFlags usage,
	const VkExtent3D imageSize, const uint32 mipLevels, const uint32 layers ) {
	uint32           queueCount;
	Array<uint32, 4> concurrentQueues = GetConcurrentQueues( &queueCount );

	VkImageCreateInfo imageInfo {
		.flags         = flags,
		.imageType     = type,
		.format        = format,
		.extent        = imageSize,
		.mipLevels     = mipLevels,
		.arrayLayers   = layers,
		.samples       = VK_SAMPLE_COUNT_1_BIT,
		.tiling        = VK_IMAGE_TILING_OPTIMAL,
		.usage         = usage,
		.sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	return GetImageRequirements( imageInfo );
}

MemoryHeap& EngineAllocator::MemoryHeapFromType( const MemoryHeap::MemoryType type, const bool image ) {
	switch ( type ) {
		default:
		case MemoryHeap::ENGINE:
			return image ? memoryHeapEngineImages : memoryHeapEngine;
		case MemoryHeap::CORE_TO_ENGINE:
			return memoryHeapCoreToEngine;
		case MemoryHeap::ENGINE_TO_CORE:
			return memoryHeapEngineToCore;
	}
}

MemoryPool EngineAllocator::AllocMemoryPool( const MemoryHeap::MemoryType type, const uint64 size, const bool image, const void* dedicatedResource ) {
	MemoryPool memoryPool {
		.size = size
	};

	VkMemoryAllocateFlagsInfo memoryFlags {
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
	};

	if ( zeroInitMemory ) {
		memoryFlags.flags |= VK_MEMORY_ALLOCATE_ZERO_INITIALIZE_BIT_EXT;
	}

	VkMemoryDedicatedAllocateInfo dedicatedMemoryInfo {
		.image  = image ? ( VkImage ) dedicatedResource : nullptr,
		.buffer = image ? nullptr : ( VkBuffer ) dedicatedResource
	};

	if ( dedicatedResource ) {
		// memoryFlags.pNext = &dedicatedMemoryInfo;
	}

	MemoryHeap& heap = MemoryHeapFromType( type, image );

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

Buffer::Usage operator|( const Buffer::Usage& lhs, const Buffer::Usage& rhs ) {
	return ( Buffer::Usage ) ( ( uint32 ) lhs | ( uint32 ) rhs );
}

static VkBufferUsageFlags2 GetBufferUsageFlags( const MemoryHeap::MemoryType type, const Buffer::Usage usage ) {
	static std::unordered_map<uint32, VkBufferUsageFlags2> bufferUsage {
		{ Buffer::VERTEX,          VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT                                    },
		{ Buffer::INDEX,           VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT                                     },
		{ Buffer::INDIRECT,        VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT                                  },
		{ Buffer::DESCRIPTOR_HEAP, VK_BUFFER_USAGE_2_DESCRIPTOR_HEAP_BIT_EXT                              },
		{ Buffer::AS,              VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR               },
		{ Buffer::AS_BUILD,        VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR },
		{ Buffer::SBT,             VK_BUFFER_USAGE_2_SHADER_BINDING_TABLE_BIT_KHR                         },
		{ Buffer::MICROMAP,        VK_BUFFER_USAGE_2_MICROMAP_STORAGE_BIT_EXT                             },
		{ Buffer::MICROMAP_BUILD,  VK_BUFFER_USAGE_2_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT               },
		{ Buffer::DGC_PREPROCESS,  VK_BUFFER_USAGE_2_PREPROCESS_BUFFER_BIT_EXT                            }
	};

	VkBufferUsageFlags2 usageFlags = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
	switch ( type ) {
		case MemoryHeap::CORE_TO_ENGINE:
			usageFlags |= VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
			break;
		case MemoryHeap::ENGINE_TO_CORE:
			usageFlags |= VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
			break;
		case MemoryHeap::ENGINE:
		default:
			usageFlags |= VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
			break;
	}

	uint64 usageExt = usage;

	while ( usageExt ) {
		usageFlags |= bufferUsage[FindLSB( usageExt )];
		UnSetBit( &usageExt, FindLSB( usageExt ) );
	}

	return usageFlags;
}

MemoryRequirements GetBufferRequirements( const MemoryHeap::MemoryType type, const uint64 size, const Buffer::Usage usage ) {
	uint32           queueCount;
	Array<uint32, 4> concurrentQueues = GetConcurrentQueues( &queueCount );

	VkBufferUsageFlags2CreateInfo bufferFlagsInfo {
		.usage = GetBufferUsageFlags( type, usage )
	};

	VkBufferCreateInfo bufferInfo {
		.pNext                 = &bufferFlagsInfo,
		.size                  = size,
		.sharingMode           = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = queueCount,
		.pQueueFamilyIndices   = concurrentQueues.memory
	};

	VkDeviceBufferMemoryRequirements reqs2 {
		.pCreateInfo = &bufferInfo
	};

	VkMemoryDedicatedRequirements dedicatedReqs {};

	VkMemoryRequirements2 out { .pNext = &dedicatedReqs };

	vkGetDeviceBufferMemoryRequirements( device, &reqs2, &out );

	return {
		.size      = out.memoryRequirements.size,
		.alignment = out.memoryRequirements.alignment,
		.type      = out.memoryRequirements.memoryTypeBits,
		.dedicated = ( bool ) ( dedicatedReqs.requiresDedicatedAllocation | dedicatedReqs.prefersDedicatedAllocation )
	};
}

Buffer EngineAllocator::AllocBuffer( const MemoryHeap::MemoryType type, MemoryPool& pool,
	const MemoryRequirements& reqs, const Buffer::Usage usage ) {
	uint32           queueCount;
	Array<uint32, 4> concurrentQueues = GetConcurrentQueues( &queueCount );

	VkBufferUsageFlags2CreateInfo bufferFlagsInfo {
		.usage = GetBufferUsageFlags( type, usage )
	};

	VkBufferCreateInfo bufferInfo {
		.pNext                 = &bufferFlagsInfo,
		.size                  = reqs.size,
		.sharingMode           = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = queueCount,
		.pQueueFamilyIndices   = concurrentQueues.memory
	};

	if ( reqs.dedicated ) {
		pool = AllocMemoryPool( type, reqs.size, false, nullptr );
	}

	VkBuffer buffer;
	vkCreateBuffer( device, &bufferInfo, nullptr, &buffer );

	uint64 address   = ( uint64 ) pool.memory;
	uint64 alignment = reqs.dedicated ? reqs.alignment : std::max( reqs.alignment, coherentAccessAlignment );

	if ( address & ( alignment - 1 ) ) {
		address = ( address & ~( alignment - 1 ) ) + alignment;
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

	MemoryHeap& heap = MemoryHeapFromType( type, false );

	if ( heap.flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
		res.memory = pool.mappedMemory + res.offset;
	}

	VkBufferDeviceAddressInfo bdaInfo {
		.buffer = buffer
	};

	res.engineMemory = vkGetBufferDeviceAddress( device, &bdaInfo );

	return res;
}

Buffer EngineAllocator::AllocDedicatedBuffer( const MemoryHeap::MemoryType type, const uint64 size, const Buffer::Usage usage ) {
	MemoryPool pool;

	MemoryRequirements reqs = GetBufferRequirements( type, size, usage );
	reqs.dedicated          = true;

	return AllocBuffer( type, pool, reqs, usage );
}

void EngineAllocator::AllocImage( MemoryPool& pool, const MemoryRequirements& reqs, const VkImage image,
	uint64* offset, uint64* size ) {
	if ( reqs.dedicated ) {
		pool = AllocMemoryPool( MemoryHeap::ENGINE, reqs.size, true, nullptr );
	}

	uint64 address = ( uint64 ) pool.memory;

	if ( address & ( reqs.alignment - 1 ) ) {
		address    = ( address & ~( reqs.alignment - 1 ) ) + reqs.alignment;
	}

	pool.offset += address + reqs.size - ( uint64 ) address;

	VkBindImageMemoryInfo bindInfo {
		.image  = image,
		.memory = ( VkDeviceMemory ) pool.memory
	};

	vkBindImageMemory2( device, 1, &bindInfo );

	*offset = address - ( uint64 ) pool.memory;
	*size   = reqs.size;
}

MemoryHeap EngineAllocator::MemoryHeapForUsage( const uint32 memoryRegion, const bool image, uint32 supportedTypes, const uint32 flags ) {
	VkPhysicalDeviceMemoryBudgetPropertiesEXT properties {};
	VkPhysicalDeviceMemoryProperties2 properties2 {
		.pNext = &properties
	};

	vkGetPhysicalDeviceMemoryProperties2( physicalDevice, &properties2 );

	VkPhysicalDeviceMemoryProperties& memoryProperties = properties2.memoryProperties;

	while ( supportedTypes ) {
		const uint32 id                = FindLSB( supportedTypes );
		const VkMemoryType& memoryType = memoryProperties.memoryTypes[id];

		if ( ( flags & memoryType.propertyFlags ) != flags ) {
			UnSetBit( &supportedTypes, id );
			continue;
		}

		return {
			.size         = properties.heapBudget[memoryRegion] - properties.heapUsage[memoryRegion],
			.maxSize      = properties.heapBudget[memoryRegion],
			.memoryRegion = memoryRegion,
			.id           = id,
			.flags        = flags
		};
	}

	Err( "No suitable MemoryHeap found" );

	return {};
}

void EngineAllocator::Init() {
	VkPhysicalDeviceMemoryBudgetPropertiesEXT properties  {};
	VkPhysicalDeviceMemoryProperties2         properties2 {
		.pNext = &properties
	};

	vkGetPhysicalDeviceMemoryProperties2( physicalDevice, &properties2 );

	VkPhysicalDeviceMemoryProperties& memoryProperties = properties2.memoryProperties;

	static constexpr VkMemoryPropertyFlags memoryTypeGPU =
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	static constexpr VkMemoryPropertyFlags memoryTypeBAR =
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	static constexpr VkMemoryPropertyFlags memoryTypeGPUToCPU =
		VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	static constexpr VkMemoryPropertyFlags memoryTypeUnified =
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	uint32 memoryRegionEngine   = UINT_MAX;
	uint32 memoryRegionBAR      = UINT_MAX;
	uint32 memoryRegionCore     = UINT_MAX;

	uint32 memoryIDEngine       = 0;
	uint32 memoryIDEngineImages = 0;
	uint32 memoryIDCoreToEngine = 0;
	uint32 memoryIDEngineToCore = 0;

	if ( memoryProperties.memoryHeapCount == 1 ) {
		unifiedMemory = true;
		rebar         = false;

		for ( uint32 i = 0; i < memoryProperties.memoryTypeCount; i++ ) {
			const VkMemoryType& memoryType = memoryProperties.memoryTypes[i];

			if ( ( memoryType.propertyFlags & memoryTypeUnified ) == memoryTypeUnified ) {
				memoryIDEngine = memoryType.propertyFlags;
				break;
			}
		}

		if ( !memoryIDEngine ) {
			Err( "Couldn't find memory type for ENGINE" );
		}

		memoryRegionEngine     = 0;
		memoryRegionBAR        = 0;
		memoryRegionCore       = 0;

		memoryIDEngineImages   = memoryIDEngine;
		memoryIDCoreToEngine   = memoryIDEngine;
		memoryIDEngineToCore   = memoryIDEngine;
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

						memoryIDEngine       = memoryType.propertyFlags;
						continue;
					}

					if ( memoryType.propertyFlags & memoryTypeBAR
						&& !( memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) ) {
						if ( memoryRegionBAR == UINT_MAX ) {
							memoryRegionBAR = i;
						}

						memoryIDCoreToEngine = memoryType.propertyFlags;
						continue;
					}
				} else {
					if ( memoryType.propertyFlags & memoryTypeGPUToCPU ) {
						if ( memoryRegionCore == UINT_MAX ) {
							memoryRegionCore = i;
						}
						
						memoryIDEngineToCore = memoryType.propertyFlags;
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

		memoryIDEngineImages = memoryIDEngine;

		if ( memoryRegionEngine == memoryRegionBAR ) {
			memoryIDEngine   = memoryIDCoreToEngine;
			rebar            = true;
		}
	}

	MemoryRequirements reqs;
	reqs = GetBufferRequirements( MemoryHeap::ENGINE, 1024 * 1024 * 1024, Buffer::VERTEX | Buffer::INDEX | Buffer::INDIRECT );

	memoryHeapEngine        = MemoryHeapForUsage( memoryRegionEngine, false,     reqs.type, memoryIDEngine );

	reqs = GetImageRequirements( VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, { 1024, 1024 },
		10, 1 );
	uint32 supportedTypes   = reqs.type;

	reqs = GetImageRequirements( VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, { 1024, 1024 },
		10, 1 );
	supportedTypes         &= reqs.type;

	reqs = GetImageRequirements( VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, { 4096, 4096 },
		12, 1 );
	supportedTypes         &= reqs.type;

	memoryHeapEngineImages  = MemoryHeapForUsage( memoryRegionEngine, true, supportedTypes, memoryIDEngineImages );

	reqs = GetBufferRequirements( MemoryHeap::CORE_TO_ENGINE, 262144 );
	memoryHeapCoreToEngine  = MemoryHeapForUsage( memoryRegionBAR,    false,     reqs.type, memoryIDCoreToEngine );

	reqs = GetBufferRequirements( MemoryHeap::ENGINE_TO_CORE, 262144 );
	memoryHeapEngineToCore  = MemoryHeapForUsage( memoryRegionCore,   false,     reqs.type, memoryIDEngineToCore );

	coherentAccessAlignment = engineConfig.coherentAccessAlignment;

	zeroInitMemory          = featuresConfig.zeroInitializeDeviceMemory;

	memoryPoolCount         = 0;
}

void EngineAllocator::Free() {
	for ( MemoryPool* memoryPool = memoryPools; memoryPool < memoryPools + memoryPoolCount; memoryPool++ ) {
		vkFreeMemory( device, ( VkDeviceMemory ) memoryPool->memory, nullptr );
	}
}