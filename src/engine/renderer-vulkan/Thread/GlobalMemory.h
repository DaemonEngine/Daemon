/*
=============================================================================
Daemon-Vulkan BSD Source Code
Copyright (c) 2025-2026 Reaper
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the Reaper nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL REAPER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=============================================================================
*/

#ifndef GLOBAL_MEMORY_H
#define GLOBAL_MEMORY_H

#include <unordered_map>
#include <atomic>

#include "../Math/NumberTypes.h"

#include "../Memory/Allocator.h"
#include "../Memory/DynamicArray.h"
#include "../Memory/MemoryChunk.h"
#include "../Memory/SysAllocator.h"

#include "Task.h"

struct GlobalAllocationRecord {
	static constexpr uint64 HEADER_MAGIC = 0xACC0500D66666666;
	static constexpr uint32 srcSize      = 227;

	std::string Format() const;

	void operator=( const GlobalAllocationRecord& other );

	uint64              guardValue = HEADER_MAGIC;
	uint64              size;
	uint32              alignment;
	uint32              chunkID; // LSB->MSB: 0-5 - chunk, 6-26 - area, 27-31 - level, 31 - allocated

	std::atomic<uint32> refCount;

	char                source[srcSize + 1];
};

struct GlobalTaskTime {
	std::atomic<uint64> count = 0;
	std::atomic<uint64> time = 0;
};

class GlobalMemory : public Allocator {
	public:
	DynamicArray<ChunkAllocator> chunkAllocators[MAX_MEMORY_AREAS] { { &sysAllocator }, { &sysAllocator }, { &sysAllocator } };

	std::unordered_map<TaskFunction, GlobalTaskTime> taskTimes;
	AccessLock                                             taskTimesLock;

	void         Init();

	byte*        Alloc( const uint64 size, const uint64 alignment );
	void         Free( byte* memory );

	private:
	ChunkRecord* IDToChunkRecord( const uint8 level, const uint8 area, const uint8 chunk );
	ChunkRecord* IDToChunkRecord( const uint32 id );

	uint32       AllocChunk( const uint64 size, const uint64 alignment, const uint8 level );
};

void InitGlobalMemory();

extern GlobalMemory SM;

#endif // GLOBAL_MEMORY_H