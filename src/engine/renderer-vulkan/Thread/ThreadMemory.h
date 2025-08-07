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
// ThreadMemory.h

#ifndef THREAD_MEMORY_H
#define THREAD_MEMORY_H

#include <thread>

#include "../Math/NumberTypes.h"

#include "../Memory/DynamicArray.h"
#include "../Memory/MemoryChunk.h"
#include "Task.h"

struct AllocationRecord {
	static constexpr uint64 HEADER_MAGIC = 0xACC0500D66666666;

	std::string Format() const {
		if ( guardValue == HEADER_MAGIC ) {
			return Str::Format( "guard value: %u, size: %u, alignment: %u, chunkID: %u, source: %s",
				guardValue, size, alignment, chunkID,
				source );
		}
		
		return Str::Format( "guard value: %u (should be: %u), size: %u, alignment: %u, chunkID: %u, source: %s",
			guardValue, HEADER_MAGIC, size, alignment, chunkID,
			source );
	}

	uint64 guardValue = HEADER_MAGIC;
	uint64 size;
	uint32 alignment;
	uint32 chunkID; // LSB->MSB: 4 bits - level, 27 bits - chunk, 1 bit - allocated
	char source[104];
};

struct MemoryChunkRecord {
	MemoryChunk chunk;
	uint64 offset;
	uint32 allocs;
};

struct ChunkAllocator {
	DynamicArray<uint64> allocatedChunks;
	DynamicArray<uint64> availableChunks;
	DynamicArray<MemoryChunkRecord> chunks;
};

struct TaskTime {
	uint64 count = 0;
	uint64 time = 0;
	bool syncedWithSM = false;
};

class ThreadMemory {
	public:
	static constexpr uint32 MAIN_ID = UINT32_MAX;
	uint32 id;

	bool main = false;

	ChunkAllocator chunkAllocators[MAX_MEMORY_AREAS];

	std::unordered_map<Task::TaskFunction, TaskTime> taskTimes;
	uint64 unknownTaskCount = 0;

	uint32 idleThreads[256]{ 0 };

	GlobalTimer fetchOuterTimer;
	GlobalTimer fetchQueueLockTimer;

	GlobalTimer addQueueWaitTimer;

	GlobalTimer addTimer;
	GlobalTimer syncTimer;

	GlobalTimer exitTimer;

	~ThreadMemory();

	void Init();

	byte* AllocAligned( const uint64 size, const uint64 alignment );
	void Free( byte* memory );

	void FreeAllChunks();

	private:
	void PrintChunkInfo( MemoryChunkRecord* memoryChunk );
};

extern thread_local ThreadMemory TLM;

#endif // THREAD_MEMORY_H