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

#include "../SrcDebug/StackTrace.h"
#include "../SrcDebug/Tag.h"
#include "../Error.h"

#include "../Memory/MemoryChunkSystem.h"

#include "ThreadMemory.h"

#include "GlobalMemory.h"

GlobalMemory SM;

void GlobalMemory::Init() {
	for ( uint32 i = 0; i < MAX_MEMORY_AREAS; i++ ) {
		DynamicArray<ChunkAllocator>& chunkAllocator = chunkAllocators[i];
		MemoryAreaConfig&             config         = memoryChunkSystem.config.areas[i];

		chunkAllocator.Resize( config.chunkAreas );
		chunkAllocator.Zero();
	}
}

std::string GlobalAllocationRecord::Format() const {
	if ( guardValue == HEADER_MAGIC ) {
		return Str::Format( "guard value: %u, size: %u, alignment: %u, chunkID: %u, source: %s",
			guardValue, size, alignment, chunkID,
			source );
	}

	return Str::Format( "guard value: %u (corrupted, should be: %u), size: %u, alignment: %u, chunkID: %u, source: %s",
		guardValue, HEADER_MAGIC, size, alignment, chunkID,
		source );
}

void GlobalAllocationRecord::operator=( const GlobalAllocationRecord& other ) {
	guardValue = other.guardValue;
	size       = other.size;
	alignment  = other.alignment;
	chunkID    = other.chunkID;

	refCount.store( other.refCount.load( std::memory_order_relaxed ), std::memory_order_relaxed );

	Q_strncpyz( source, other.source, srcSize + 1 );
}

static MemoryChunkRecord* AllocChunk( const uint64 size, const uint64 alignment, const uint8 level,
                                      DynamicArray<ChunkAllocator>& chunkAllocators,
                                      uint32* chunkID ) {
	MemoryChunkRecord* record = nullptr;

	for ( uint32 i = 0; i < chunkAllocators.size && !record; i++ ) {
		ChunkAllocator& chunkAllocator = chunkAllocators[i];

		if ( !chunkAllocator.accessLock.LockWrite() ) {
			continue;
		}

		uint64&         chunkArea      = chunkAllocator.availableChunks;
		uint64          area           = chunkArea;

		while ( area ) {
			uint32 chunk = FindLSB( area );
			record       = &chunkAllocator.chunks[chunk];

			if ( record->chunk.size >= record->offset + size ) {
				if ( record->offset + size == record->chunk.size ) {
					UnSetBit( &chunkArea, chunk );
				}

				*chunkID = MemoryChunkToID( level, i, chunk );

				break;
			} else {
				record = nullptr;
			}

			UnSetBit( &area, chunk );
		}

		chunkAllocator.accessLock.UnlockWrite();
	}

	if ( !record ) {
		MemoryChunk     chunk              = memoryChunkSystem.Alloc( size );

		ChunkAllocator& chunkAllocator     = chunkAllocators[chunk.chunkArea];

		chunkAllocator.accessLock.LockWrite();

		SetBit( &chunkAllocator.availableChunks, chunk.chunk );
		chunkAllocator.chunks[chunk.chunk] = { .chunk = chunk };

		SetBit( &chunkAllocator.allocatedChunks, chunk.chunk );

		record = &chunkAllocator.chunks[chunk.chunk];

		*chunkID = MemoryChunkToID( level, chunk.chunkArea, chunk.chunk );

		chunkAllocator.accessLock.UnlockWrite();
	}

	return record;
}

byte* GlobalMemory::Alloc( const uint64 size, const uint64 alignment ) {
	if ( !size ) {
		return nullptr;
	}

	ASSERT_EQ( ( alignment & ( alignment - 1 ) ), 0 );

	const uint64 paddedSize = ( size + sizeof( GlobalAllocationRecord ) + alignment - 1 ) & ~( alignment - 1 );

	uint32 level;
	uint32 count;

	memoryChunkSystem.SizeToLevel( size, &level, &count );

	uint32             chunkID;
	MemoryChunkRecord* record = AllocChunk( paddedSize, alignment, level, chunkAllocators[level], &chunkID );

	std::string source = FormatSrc( std::stacktrace::current(), true, true );

	GlobalAllocationRecord alloc {
		.size      = paddedSize - sizeof( GlobalAllocationRecord ),
		.alignment = ( uint32 ) alignment,
		.chunkID   = chunkID,
		.refCount  = 1
	};

	Q_strncpyz( alloc.source, source.c_str(), GlobalAllocationRecord::srcSize );
	alloc.source[GlobalAllocationRecord::srcSize] = '\0';

	*( ( GlobalAllocationRecord* ) ( record->chunk.memory + record->offset ) ) = alloc;

	byte* ret         = record->chunk.memory + record->offset + sizeof( GlobalAllocationRecord );
	record->offset   += paddedSize;
	record->allocs++;

	return ret;
}

void GlobalMemory::Free( byte* memory ) {
	GlobalAllocationRecord* record = ( GlobalAllocationRecord* ) ( memory - sizeof( GlobalAllocationRecord ) );

	if ( record->guardValue != GlobalAllocationRecord::HEADER_MAGIC ) {
		Err( "Memory chunk corrupted: %s", record->Format() );
	}

	const uint32 refCount = record->refCount.fetch_sub( 1, std::memory_order_release ) - 1;

	if ( refCount ) {
		return;
	}

	UnSetBit( &record->chunkID, chunkAllocOffset );

	uint8 level;
	uint8 area;
	uint8 chunk;

	IDToMemoryChunk( record->chunkID, &level, &area, &chunk );

	ChunkAllocator&    chunkAllocator = chunkAllocators[level][area];
	MemoryChunkRecord& chunkRecord    = chunkAllocator.chunks[chunk];

	chunkRecord.allocs--;

	if ( !chunkRecord.allocs ) {
		chunkRecord.offset = 0;
	}

	SetBit( &chunkAllocator.availableChunks, chunk );
}

void InitGlobalMemory() {
	SM.Init();
}