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
// GlobalMemory.cpp

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

static MemoryChunkRecord* AllocChunk( const uint64 size, const uint64 alignment,
                                      DynamicArray<ChunkAllocator>& chunkAllocators,
                                      uint32* chunkID ) {
	MemoryChunkRecord* record = nullptr;

	SetBit( chunkID, 31 );

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

				SetBits( chunkID, i,     6, 21 );
				SetBits( chunkID, chunk, 0, 6 );

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

		SetBits( chunkID, chunk.chunkArea, 6, 21 );
		SetBits( chunkID, chunk.chunk,     0, 6 );

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

	uint32             chunkID = SetBits( ( uint32 ) 0, level, 27, 4 );
	MemoryChunkRecord* record  = AllocChunk( paddedSize, alignment, chunkAllocators[level], &chunkID );

	#ifdef _MSC_VER
		static constexpr const char* pathStrip = "engine\\renderer-vulkan\\";
	#else
		static constexpr const char* pathStrip = "engine/renderer-vulkan/";
	#endif

	std::string source = FormatStackTrace( std::stacktrace::current(), true, true );

	uint32_t pos = 0;
	while ( ( pos = source.find( pathStrip, pos ) ) < source.size() ) {
		source = source.erase( pos, strlen( pathStrip ) );
	}

	GlobalAllocationRecord alloc {
		.size      = paddedSize - sizeof( GlobalAllocationRecord ),
		.alignment = ( uint32 ) alignment,
		.chunkID   = chunkID
	};

	Q_strncpyz( alloc.source, source.c_str(), 99 );
	alloc.source[99] = '\0';

	alloc.refCount.store( 0, std::memory_order_relaxed );

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

	UnSetBit( &record->chunkID, 31 );

	uint32             area           = GetBits( record->chunkID, 6, 21 );
	ChunkAllocator&    chunkAllocator = chunkAllocators[GetBits( record->chunkID, 27, 4 )][area];

	uint32             chunk          = GetBits( record->chunkID, 0, 6 );

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