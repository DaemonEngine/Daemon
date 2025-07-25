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
// MemoryChunk.cpp

#include "MemoryChunk.h"

MemoryChunkSystem memoryChunkSystem;

MemoryChunkSystem::MemoryChunkSystem():
	Tag( "MemoryChunkSystem" ) {
	UpdateMemoryChunkSystemConfig();

	for ( MemoryArea* area = memoryAreas; area < memoryAreas + MAX_MEMORY_AREAS; area++ ) {
		area->memory = ( byte* ) Alloc64( area->config.chunks * area->config.chunkSize );

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

MemoryChunk MemoryChunkSystem::Alloc( uint64_t size ) {
	uint32_t level;
	uint32_t count;

	SizeToLevel( size, &level, &count );

	if ( count > 64 ) {
		Sys::Drop( "Allocation size too large: %ull", size );
	}

	MemoryChunk out;

	if ( count > 1 ) {
		Sys::Drop( "Couldn't find memory chunk large enough to support allocation (%u bytes, requires %u * %u byte chunks)",
			size, count, memoryAreas[level].config.chunkSize );
	}

	uint32_t initialLevel = level;
	while ( !LockArea( &memoryAreas[level], &out.chunkArea, &out.chunk ) ) {
		if ( level == 0 ) {
			Log::WarnTagT( "No memory chunks available, yielding" );
			std::this_thread::yield();
			level = initialLevel;
		} else {
			level--;
		}
	}

	out.area = level;
	out.size = memoryAreas[level].config.chunkSize;
	out.memory = memoryAreas[level].memory + ( out.chunkArea * 64ull + out.chunk ) * memoryAreas[level].config.chunkSize;

	return out;
}

void MemoryChunkSystem::Free( MemoryChunk* memoryChunk ) {
	Log::DebugTagT( "Freeing area %u chunk %u:%u", memoryChunk->area, memoryChunk->chunkArea, memoryChunk->chunk );

	memoryAreas[memoryChunk->area].chunkLocks[memoryChunk->chunkArea].value -= 1ull << memoryChunk->chunk;
}

void MemoryChunkSystem::SizeToLevel( const uint64_t size, uint32_t* level, uint32_t* count ) {
	for ( uint32_t i = 0; i < MAX_MEMORY_AREAS; i++ ) {
		if ( memoryAreas[i].config.chunkSize >= size ) {
			*level = i;
			*count = 1;
			return;
		}

		// TODO: Allow contiguous memory chunks ranges?
		/* if ( i == 2 || size >= memoryAreasSizes[i + 1] * 32 ) {
			*level = i;
			*count = ( size + memoryAreasSizes[i] - 1 ) / memoryAreasSizes[i];
			return;
		} */
	}

	Sys::Drop( "Couldn't find memory area with large enough chunkSize, requested: %u bytes", size );
}

bool MemoryChunkSystem::LockArea( MemoryArea* memoryArea, uint32_t* chunkArea, uint8_t* chunk ) {
	uint64_t expectedLocks;
	uint64_t desiredLocks;
	uint32_t area;

	uint32_t loopCount = 0;

	uint32_t i = 0;
	uint32_t current = 0;

	Timer t;
	do {
		while ( true ) {
			if ( i == memoryArea->config.chunkAreas ) {
				std::this_thread::yield();
				return false;
				/* i = 0;
				continue; */
			}

			loopCount++;
			expectedLocks = memoryArea->chunkLocks[i].value.load();

			if ( expectedLocks == UINT64_MAX ) {
				i++;
				continue;
			}

			area = FindLZeroBit( expectedLocks );

			Log::DebugTagT( "Trying chunk %u", area );

			if ( i * 64 + area >= memoryArea->config.chunks ) {
				Log::DebugTagT( "Failed: chunk %u out of range", area );
				return false;
			}

			desiredLocks = SetBit( expectedLocks, area );

			current = i;
			i++;

			break;
		}
	} while (
		!memoryArea->chunkLocks[current].value.compare_exchange_strong( expectedLocks, desiredLocks, std::memory_order_relaxed )
	);

	Log::DebugTagT( "Locked area %u:%u in %s, loop: %u",
		current, area, Timer::FormatTime( t.Time() ),
		loopCount );

	*chunkArea = current;
	*chunk = area;

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

	for ( uint32_t i = 0; i < MAX_MEMORY_AREAS; i++ ) {
		memoryChunkSystem.memoryAreas[i].config = memoryChunkSystem.config.areas[i];
	}
}