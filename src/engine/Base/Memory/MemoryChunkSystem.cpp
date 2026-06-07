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

#include "Bit.h"
#include "Error.h"
#include "Timer.h"

#include "SysAllocator.h"

#include "MemoryChunkSystem.h"

MemoryChunkSystem memoryChunkSystem { &sysAllocator };

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

Chunk allocation is generally handled by TLM/SM, so containers or other custom allocators only need to call TLM.Alloc( size ).

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

MemoryChunkSystem::MemoryChunkSystem( Allocator* newAllocator ):
	Tag( "MemoryChunkSystem" ),
	allocator( newAllocator ) {
}

MemoryChunkSystem::~MemoryChunkSystem() {
	for ( MemoryArea* area = memoryAreas; area < memoryAreas + MAX_MEMORY_AREAS; area++ ) {
		allocator->Free( area->memory );
		allocator->Free( ( byte* ) area->chunkLocks );
	}
}

uint32 MemoryChunkToID( const uint8 level, const uint8 area, const uint8 chunk ) {
	uint32 id;

	SetBits( &id, chunk, 0,                chunkBits );
	SetBits( &id, area,  chunkAreaOffset,  chunkAreaBits );
	SetBits( &id, level, chunkLevelOffset, chunkLevelBits );
	SetBits( &id,        chunkAllocOffset, 1 );

	return id;
}

MemoryChunk IDToMemoryChunk( const uint32 id ) {
	return {
		.level = ( uint8 ) GetBits( id, chunkLevelOffset, chunkLevelBits ),
		.area  = ( uint8 ) GetBits( id, chunkAreaOffset,  chunkAreaBits ),
		.chunk = ( uint8 ) GetBits( id, 0,                chunkBits ),
		.allocated = (bool ) GetBits( id, chunkAllocOffset, 1 )
	};
}

void MemoryChunkSystem::InitConfig( const char* configText ) {
	const char** text = &configText;

	Log::NoticeTag( "Parsing memoryChunkConfig: %s", configText );

	bool parsed = true;
	for ( uint32 i = 0; i < MAX_MEMORY_AREAS; i++ ) {
		const char* token = COM_ParseExt2( text, false );
		if ( !token || *token == '\0' ) {
			break;
		}

		if ( i >= MAX_MEMORY_AREAS ) {
			parsed = false;
			break;
		}

		int out;
		if ( !Q_strtoi( token, &out ) ) {
			parsed = false;
			break;
		}

		config.areas[i].chunkSize = out * 1024ull;

		COM_ParseExt2( text, false );

		token = COM_ParseExt2( text, false );
		if ( !Q_strtoi( token, &out ) ) {
			parsed = false;
			break;
		}

		config.areas[i].chunks = out;
	}

	if( !parsed ) {
		Log::WarnTag( "Bad memoryChunkConfig: %s, using default (%s)", configText, defaultMemoryChunkConfig );
		InitConfig( defaultMemoryChunkConfig );
		return;
	}

	uint32 notValid = 3;
	for ( uint32 i = 0; i < MAX_MEMORY_AREAS; i++ ) {
		MemoryAreaConfig& areaConfig = config.areas[i];

		if ( areaConfig.chunkSize < memoryChunkConfigRequired[i][0] ) {
			areaConfig.chunkSize = memoryChunkConfigRequired[i][0];
		}

		if ( areaConfig.chunks < memoryChunkConfigRequired[i][1] ) {
			areaConfig.chunks    = memoryChunkConfigRequired[i][1];
		}

		notValid = notValid ? notValid - 1 : 0;
	}

	if ( notValid ) {
		Log::WarnTag( "Bad memoryChunkConfig: %s, using default (%s)", configText, defaultMemoryChunkConfig );
		InitConfig( defaultMemoryChunkConfig );
		return;
	}

	std::sort( config.areas, config.areas + MAX_MEMORY_AREAS,
		[]( const MemoryAreaConfig& lhs, const MemoryAreaConfig& rhs ) {
			return lhs.chunkSize < rhs.chunkSize;
		} );

	for ( MemoryAreaConfig& area : config.areas ) {
		area.chunkAreas = ( area.chunks + 63 ) / 64;
	}

	for ( uint32 i = 0; i < MAX_MEMORY_AREAS; i++ ) {
		memoryAreas[i].config = config.areas[i];
	}

	Log::NoticeTag( "Parsed memoryChunkConfig" );

	for ( MemoryArea* area = memoryAreas; area < memoryAreas + MAX_MEMORY_AREAS; area++ ) {
		area->memory = ( byte* ) allocator->Alloc( area->config.chunks * area->config.chunkSize, 64 );
		memset( area->memory, 0, area->config.chunks * area->config.chunkSize );

		area->chunkLocks = ( AlignedAtomicUint64* ) allocator->Alloc( area->config.chunkAreas * sizeof( AlignedAtomicUint64 ), 64 );
		memset( area->chunkLocks, 0, area->config.chunkAreas * sizeof( AlignedAtomicUint64 ) );
	}
}

MemoryChunk MemoryChunkSystem::Alloc( uint64 size ) {
	uint32 level;
	uint32 count;

	SizeToLevel( size, &level, &count );

	if ( count > 64 ) {
		Err( "Allocation size too large: %ull", size );

		return {};
	}


	if ( count > 1 ) {
		Err( "Couldn't find memory chunk large enough to support allocation (%u bytes, requires %u * %u byte chunks)",
			size, count, memoryAreas[level].config.chunkSize );

		return {};
	}

	uint32 initialLevel = level;
	uint8  chunkArea;
	uint8  chunk;
	while ( !LockArea( level, &chunkArea, &chunk ) ) {
		if ( level == 0 ) {
			Log::WarnTagT( "No memory chunks available, yielding" );
			std::this_thread::yield();
			level = initialLevel;
		} else {
			level--;
		}
	}

	return {
		.level = ( uint8 ) level,
		.area  = chunkArea,
		.chunk = chunk
	};
}

static std::string FormatChunk( const MemoryChunk& chunk ) {
	return Str::Format( "level: %u area: %u chunk: %u", chunk.level, chunk.area, chunk.chunk );
}

void MemoryChunkSystem::Free( const uint8 level, const uint8 area, const uint8 chunk ) {
	Log::DebugTagT( "Freeing chunk %u[%u:%u]", level, area, chunk );

	uint64 expectedLocks;
	uint64 desiredLocks;

	std::atomic<uint64>& chunkLock = memoryAreas[level].chunkLocks[area].value;

	do {
		expectedLocks = chunkLock.load( std::memory_order_relaxed );
		desiredLocks  = UnSetBit( expectedLocks, chunk );
	} while (
		!chunkLock.compare_exchange_strong( expectedLocks, desiredLocks, std::memory_order_relaxed )
	);
}

void MemoryChunkSystem::Free( const uint32 id ) {
	MemoryChunk chunk = IDToMemoryChunk( id );

	if ( !chunk.allocated ) {
		Log::WarnTagT( "Tried to free unallocated chunk: %s", FormatChunk( chunk ) );
	}

	Free( chunk.level, chunk.area, chunk.chunk );
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

	Err( "Couldn't find memory area with large enough chunkSize, requested: %u bytes", size );
}

byte* MemoryChunkSystem::GetChunkMemory( const uint8 level, const uint8 area, const uint8 chunk ) {
	return memoryAreas[level].memory + ( area * 64ull + chunk ) * memoryAreas[level].config.chunkSize;
}

byte* MemoryChunkSystem::GetChunkMemory( const uint32 id ) {
	MemoryChunk chunk = IDToMemoryChunk( id );

	if ( !chunk.allocated ) {
		Log::WarnTagT( "Tried to get memory for unallocated chunk: %s", FormatChunk( chunk ) );
	}

	return GetChunkMemory( chunk.level, chunk.area, chunk.chunk );
}

uint32 MemoryChunkSystem::GetChunkSize( const uint8 level ) {
	return memoryAreas[level].config.chunkSize;
}

bool MemoryChunkSystem::LockArea( const uint32 level, uint8* chunkArea, uint8* chunk ) {
	uint64      expectedLocks;
	uint64      desiredLocks;
	uint8       foundChunk;

	uint32      loopCount  = 0;

	uint32      i          = 0;
	uint8       area       = 0;
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
			expectedLocks = memoryArea.chunkLocks[i].value.load( std::memory_order_relaxed );

			if ( expectedLocks == UINT64_MAX ) {
				i++;

				continue;
			}

			foundChunk   = FindLZeroBit( expectedLocks );

			Log::DebugTagT( "Trying chunk %u:%u", i, foundChunk );

			if ( i * 64 + foundChunk >= memoryArea.config.chunks ) {
				Log::DebugTagT( "Failed: chunk %u:%u out of range", i, foundChunk );

				return false;
			}

			desiredLocks = SetBit( expectedLocks, foundChunk );

			area         = i;
			i++;

			break;
		}
	} while (
		!memoryArea.chunkLocks[area].value.compare_exchange_strong( expectedLocks, desiredLocks, std::memory_order_relaxed )
	);

	Log::DebugTagT( "Locked area %u[%u:%u] in %s, loop: %u",
		level, area, foundChunk, t.FormatTime(),
		loopCount );

	*chunkArea = area;
	*chunk     = foundChunk;

	return true;
}

void InitMemoryChunkSystemConfig( std::string* config ) {
	memoryChunkSystem.InitConfig( config->c_str() );
}