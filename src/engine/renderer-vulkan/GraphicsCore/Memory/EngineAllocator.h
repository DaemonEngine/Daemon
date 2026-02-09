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

	uint64     size;
	uint64     maxSize;

	MemoryType type;

	uint32     id;
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
	VkBuffer buffer;

	uint32   offset;
	uint32   size;

	uint32   usage;

	uint32*  memory;
	uint64   engineMemory;
};

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
	static constexpr uint32 maxMemoryPools = 32;

	// In megabytes
	static constexpr int minGraphicsMemorySize = 1024;
	static constexpr int maxGraphicsMemorySize = 16384;

	MemoryHeap memoryHeapEngine;
	MemoryHeap memoryHeapStagingBuffer;
	MemoryHeap memoryHeapEngineToCoreBuffer;

	void Init();
	void Free();

	MemoryHeap MemoryHeapForUsage( const MemoryHeap::MemoryType type, uint32 supportedTypes );

	MemoryPool AllocMemoryPool( const MemoryHeap::MemoryType type, const uint64 size, const bool image,
		const bool engineAccess, const void* dedicatedResource = nullptr );

	Buffer     AllocBuffer( const MemoryHeap::MemoryType type, MemoryPool& pool, const MemoryRequirements& reqs, const VkBufferUsageFlags flags,
		const bool engineAccess );
	Buffer     AllocDedicatedBuffer( const MemoryHeap::MemoryType type, const uint32 size, const VkBufferUsageFlags flags,
		const bool engineAccess );

	private:
	uint32     memoryPoolCount;
	MemoryPool memoryPools[maxMemoryPools];

	int memoryIDFlags[32];

	uint32 memoryRegionEngine;
	uint32 memoryRegionBAR;
	uint32 memoryRegionCore;

	uint32 memoryIDEngine;
	uint32 memoryIDCoreToEngine;
	uint32 memoryIDEngineToCore;

	bool rebar;
	bool unifiedMemory;
};

MemoryRequirements GetBufferRequirements( const VkBufferUsageFlags type, const uint64 size, const bool engineAccess );

#endif // ENGINE_ALLOCATOR_H