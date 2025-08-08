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
// MemoryChunkSystem.cpp

#include "Memory.h"

#include "MemoryChunkSystem.h"

MemoryChunkSystem memoryChunkSystem;

/* MemoryChunkSystem should be used for allocating memory wherever possible.
The system has multiple memory areas: each area consists of a number of memory chunks.
The memory for each memory area is allocated all at once, so they're contiguous. The chunks within a memory area are arranged into
chunk areas with 64 chunks each. This allows using a single uint64 for each chunk area to keep track of the ones that are allocated.

Chunks are allocated in a lock-free manner into TLMs/SM.
TLM is thread-local, so its contents should only be used within the same thread (all TLM chunks are freed at the end of Thread::Run()).

The amount of chunks is largely static, so most of the time memory allocation is just gonna be a fast lookup, + a few atomics if
the thread/SM needs a new chunk. Chunk allocation from the MemoryChunkSystem is usually on the order of 100-300ns,
even if there are a lot of threads doing that, at least on systems with CAS.

Chunks are identified as `level[chunkArea:chunk]`: level is the MemoryArea, chunkArea is 64-chunk area within it, chunk is 0-63.

TLM has its own allocators which mirror the memory structure of MemoryChunkSystem memory areas.
This also allows using fast bit-finding functions to find allocations.

Chunk allocation is generally handled by TLM/SM, so containers or other custom allocators only need to call TLM.AllocAligned( size ).

********************* | ********************* | ... | *********************
*    MemoryArea0    * | *    MemoryArea1    * | ... | *    MemoryAreaN    *
* chunk0|...|chunkM * | * chunk0|...|chunkM * | ... | * chunk0|...|chunkM *
********************* | ********************* | ... | *********************
          ^
          |
		  v
*********************   *****************   ***************************
*  ChunkAllocatorJ  *<--* TLM (threadI) *<->*     Alloc( size )       *
* chunk0|...|chunkM *   *****************   * Selects ChunkAllocatorJ *
*********************                       * Keeping track of allocs *
                                            * is done here            *
											* The return is byte*     *
											***************************
*/

MemoryChunkSystem::MemoryChunkSystem():
	Tag( "MemoryChunkSystem" ) {
	UpdateMemoryChunkSystemConfig();

	for ( MemoryArea* area = memoryAreas; area < memoryAreas + MAX_MEMORY_AREAS; area++ ) {
		area->memory = ( byte* ) Alloc64( area->config.chunks * area->config.chunkSize );
		memset( area->memory, 0, area->config.chunks * area->config.chunkSize );

		area->chunkLocks = ( AlignedAtomicUint64* ) Alloc64( area->config.chunkAreas * sizeof( AlignedAtomicUint64 ) );
		memset( area->chunkLocks, 0, area->config.chunkAreas * sizeof( AlignedAtomicUint64 ) );
	}
}

MemoryChunkSystem::~MemoryChunkSystem() {
	for ( MemoryArea* area = memoryAreas; area < memoryAreas + MAX_MEMORY_AREAS; area++ ) {
		FreeAligned( area->memory );
		FreeAligned( area->chunkLocks );
	}
}

MemoryChunk MemoryChunkSystem::Alloc( uint64 size ) {
	uint32 level;
	uint32 count;

	SizeToLevel( size, &level, &count );

	if ( count > 64 ) {
		Sys::Drop( "Allocation size too large: %ull", size );
	}

	MemoryChunk out;

	if ( count > 1 ) {
		Sys::Drop( "Couldn't find memory chunk large enough to support allocation (%u bytes, requires %u * %u byte chunks)",
			size, count, memoryAreas[level].config.chunkSize );
	}

	uint32 initialLevel = level;
	while ( !LockArea( level, &out.chunkArea, &out.chunk ) ) {
		if ( level == 0 ) {
			Log::WarnTagT( "No memory chunks available, yielding" );
			std::this_thread::yield();
			level = initialLevel;
		} else {
			level--;
		}
	}

	out.level = level;
	out.size = memoryAreas[level].config.chunkSize;
	out.memory = memoryAreas[level].memory + ( out.chunkArea * 64ull + out.chunk ) * memoryAreas[level].config.chunkSize;

	return out;
}

void MemoryChunkSystem::Free( MemoryChunk* memoryChunk ) {
	Log::DebugTagT( "Freeing chunk %u[%u:%u]", memoryChunk->level, memoryChunk->chunkArea, memoryChunk->chunk );

	memoryAreas[memoryChunk->level].chunkLocks[memoryChunk->chunkArea].value -= 1ull << memoryChunk->chunk;
}

void MemoryChunkSystem::SizeToLevel( const uint64 size, uint32* level, uint32* count ) {
	for ( uint32 i = 0; i < MAX_MEMORY_AREAS; i++ ) {
		if ( memoryAreas[i].config.chunkSize >= size ) {
			*level = i;
			*count = 1;
			return;
		}

		// TODO: Allow contiguous memory chunk ranges?
		/* if ( i == 2 || size >= memoryAreasSizes[i + 1] * 32 ) {
			*level = i;
			*count = ( size + memoryAreasSizes[i] - 1 ) / memoryAreasSizes[i];
			return;
		} */
	}

	Sys::Drop( "Couldn't find memory area with large enough chunkSize, requested: %u bytes", size );
}

bool MemoryChunkSystem::LockArea( const uint32 level, uint32* chunkArea, uint8* chunk ) {
	uint64 expectedLocks;
	uint64 desiredLocks;
	uint32 foundChunk;

	uint32 loopCount = 0;

	uint32 i = 0;
	uint32 area = 0;
	MemoryArea& memoryArea = memoryAreas[level];

	Timer t;
	do {
		while ( true ) {
			if ( i == memoryArea.config.chunkAreas ) {
				std::this_thread::yield();
				return false;
				/* i = 0;
				continue; */
			}

			loopCount++;
			expectedLocks = memoryArea.chunkLocks[i].value.load();

			if ( expectedLocks == UINT64_MAX ) {
				i++;
				continue;
			}

			foundChunk = FindLZeroBit( expectedLocks );

			Log::DebugTagT( "Trying chunk %u:%u", i, foundChunk );

			if ( i * 64 + foundChunk >= memoryArea.config.chunks ) {
				Log::DebugTagT( "Failed: chunk %u:%u out of range", i, foundChunk );
				return false;
			}

			desiredLocks = SetBit( expectedLocks, foundChunk );

			area = i;
			i++;

			break;
		}
	} while (
		!memoryArea.chunkLocks[area].value.compare_exchange_strong( expectedLocks, desiredLocks, std::memory_order_relaxed )
	);

	Log::DebugTagT( "Locked area %u[%u:%u] in %s, loop: %u",
		level, area, foundChunk, Timer::FormatTime( t.Time() ),
		loopCount );

	*chunkArea = area;
	*chunk = foundChunk;

	return true;
}

void UpdateMemoryChunkSystemConfig() {
	// TODO: Add cvars for this
	memoryChunkSystem.config.areas[0].chunkSize = 16 * 1024ull;
	memoryChunkSystem.config.areas[0].chunks = 640;
	memoryChunkSystem.config.areas[1].chunkSize = 1024 * 1024ull;
	memoryChunkSystem.config.areas[1].chunks = 640;
	memoryChunkSystem.config.areas[2].chunkSize = 64 * 1024ull * 1024;
	memoryChunkSystem.config.areas[2].chunks = 64;

	for ( MemoryAreaConfig& area : memoryChunkSystem.config.areas ) {
		area.chunkAreas = ( area.chunks + 63 ) / 64;
	}

	for ( uint32 i = 0; i < MAX_MEMORY_AREAS; i++ ) {
		memoryChunkSystem.memoryAreas[i].config = memoryChunkSystem.config.areas[i];
	}
}