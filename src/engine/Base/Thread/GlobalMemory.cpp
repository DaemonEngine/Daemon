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

#include "SrcDebug/StackTrace.h"
#include "SrcDebug/Tag.h"
#include "MemoryChunkSystem.h"
#include "Error.h"

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

ChunkRecord* GlobalMemory::IDToChunkRecord( const uint8 level, const uint8 area, const uint8 chunk ) {
	return &chunkAllocators[level][area].chunks[chunk];
}

ChunkRecord* GlobalMemory::IDToChunkRecord( const uint32 id ) {
	MemoryChunk chunk = IDToMemoryChunk( id );

	return &chunkAllocators[chunk.level][chunk.area].chunks[chunk.chunk];
}

uint32 GlobalMemory::AllocChunk( const uint64 size, const uint64 alignment, const uint8 level ) {
	for ( ChunkAllocator& chunkAllocator : chunkAllocators[level] ) {
		uint64& chunkArea = chunkAllocator.availableChunks;
		uint64  area      = chunkArea;

		while ( area ) {
			const uint32 chunk     = FindLSB( area );
			ChunkRecord* record    = IDToChunkRecord( level, &chunkAllocator - chunkAllocators[level].memory, chunk );

			const uint32 chunkSize = memoryChunkSystem.GetChunkSize( level );

			if ( record->offset + size <= chunkSize ) {
				if ( chunkSize - record->offset - size < 64 ) {
					UnSetBit( &chunkArea, chunk );
				}

				return MemoryChunkToID( level, &chunkAllocator - chunkAllocators[level].memory, chunk );
			}

			UnSetBit( &area, chunk );
		}
	}

	MemoryChunk     chunk          = memoryChunkSystem.Alloc( size );

	ChunkAllocator& chunkAllocator = chunkAllocators[level][chunk.area];

	SetBit( &chunkAllocator.availableChunks, chunk.chunk );
	SetBit( &chunkAllocator.allocatedChunks, chunk.chunk );

	return MemoryChunkToID( level, chunk.area, chunk.chunk );
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

	uint32      chunkID = AllocChunk( paddedSize, alignment, level );

	std::string source = FormatSrc( std::stacktrace::current(), true, true );

	GlobalAllocationRecord alloc {
		.size      = paddedSize - sizeof( GlobalAllocationRecord ),
		.alignment = ( uint32 ) alignment,
		.chunkID   = chunkID,
		.refCount  = 1
	};

	Q_strncpyz( alloc.source, source.c_str(), GlobalAllocationRecord::srcSize );
	alloc.source[GlobalAllocationRecord::srcSize] = '\0';
	
	ChunkRecord*                 record = IDToChunkRecord( chunkID );
	byte*                        memory = memoryChunkSystem.GetChunkMemory( chunkID ) + record->offset;
	*( GlobalAllocationRecord* ) memory = alloc;

	record->offset += paddedSize;
	record->allocs++;

	return memory + sizeof( GlobalAllocationRecord );
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

	MemoryChunk chunk = IDToMemoryChunk( record->chunkID );

	ChunkAllocator&    chunkAllocator = chunkAllocators[chunk.level][chunk.area];
	ChunkRecord& chunkRecord    = chunkAllocator.chunks[chunk.chunk];

	chunkRecord.allocs--;

	if ( !chunkRecord.allocs ) {
		chunkRecord.offset = 0;
	}

	SetBit( &chunkAllocator.availableChunks, chunk.chunk );
}

void InitGlobalMemory() {
	SM.Init();
}