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
// EngineAllocator.h

#ifndef ENGINE_ALLOCATOR_H
#define ENGINE_ALLOCATOR_H

#include "../../Math/NumberTypes.h"

#include "../Decls.h"

#include "../../GraphicsShared/MemoryPool.h"

struct MemoryHeap {
	enum MemoryType {
		ENGINE,
		CORE_TO_ENGINE,
		ENGINE_TO_CORE
	};

	uint64 size;
	uint64 maxSize;

	uint64 offset;

	uint32 memoryRegion;

	uint32 id;
	uint32 flags;
};

struct MemoryRequirements {
	uint64 size;
	uint64 alignment;
	uint32 type;
	bool   dedicated;
};

struct MemoryRegionUsage {
	uint64 allocated;
	uint64 size;
};

struct Buffer {
	enum Usage {
		VERTEX          = 1,
		INDEX           = 2,
		INDIRECT        = 4,
		DESCRIPTOR_HEAP = 8,
		AS              = 16,
		AS_BUILD        = 32,
		SBT             = 64,
		MICROMAP        = 128,
		MICROMAP_BUILD  = 256,
		DGC_PREPROCESS  = 512
	};

	VkBuffer buffer;

	uint64   offset;
	uint64   size;

	uint32   usage;

	uint32*  memory;
	uint64   engineMemory;
};

Buffer::Usage operator|( const Buffer::Usage& lhs, const Buffer::Usage& rhs );

struct Data {
	uint32* memory;

	Data( Buffer& buffer ) {
		memory = buffer.memory;
	}

	~Data() {
	}
};

class EngineAllocator {
	public:
	bool rebar;
	bool unifiedMemory;

	bool zeroInitMemory;

	void        Init();
	void        Free();

	MemoryHeap& MemoryHeapFromType( const MemoryHeap::MemoryType type, const bool image );
	MemoryHeap  MemoryHeapForUsage( const uint32 memoryRegion, const bool image, uint32 supportedTypes, const uint32 flags );

	MemoryPool  AllocMemoryPool( const MemoryHeap::MemoryType type, const uint64 size, const bool image, const void* dedicatedResource = nullptr );

	Buffer      AllocBuffer( const MemoryHeap::MemoryType type, const uint64 size, const Buffer::Usage usage = ( Buffer::Usage ) 0 );

	void        AllocImage( MemoryPool& pool, const MemoryRequirements& reqs, const VkImage image );

	private:
	static constexpr uint32 maxMemoryPools = 32;

	MemoryHeap memoryHeapEngine;
	MemoryHeap memoryHeapEngineImages;
	MemoryHeap memoryHeapCoreToEngine;
	MemoryHeap memoryHeapEngineToCore;

	uint32     memoryPoolCount;
	MemoryPool memoryPools[maxMemoryPools];

	uint64     coherentAccessAlignment;
};

MemoryRequirements  GetBufferRequirements( const MemoryHeap::MemoryType type, const uint64 size, const Buffer::Usage usage = ( Buffer::Usage ) 0 );

MemoryRequirements GetImageRequirements( const VkImageCreateInfo& imageInfo );

#endif // ENGINE_ALLOCATOR_H