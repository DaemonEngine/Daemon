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
// ThreadMemory.cpp

#include "common/Common.h"

#include "../SrcDebug/Tag.h"

#include "ThreadMemory.h"

ThreadMemory::~ThreadMemory() {
	for ( ChunkAllocator& allocator : chunkAllocators ) {
		for ( uint32 i = 0; i < allocator.availableChunks.elements; i++ ) {
			uint64 chunkArea = allocator.availableChunks[i];

			while ( chunkArea ) {
				uint32 chunk = FindLSB( chunkArea );
				Log::WarnTagT( "Unreturned memory chunk:" );

				PrintChunkInfo( &allocator.chunks[i * 64 + chunk] );

				UnSetBit( &chunkArea, chunk );
			}
		}
	}
}

void ThreadMemory::Init() {
	for ( ChunkAllocator& allocator : chunkAllocators ) {
		allocator.allocatedChunks.Resize( memoryChunkSystem.config.areas->chunkAreas );
		allocator.availableChunks.Resize( memoryChunkSystem.config.areas->chunkAreas );
		allocator.chunks.Resize( memoryChunkSystem.config.areas->chunks );

		allocator.allocatedChunks.Zero();
		allocator.availableChunks.Zero();
		allocator.chunks.Zero();
	}
}

byte* ThreadMemory::Alloc( const uint64 size, const uint64 alignment ) {
	if ( !size ) {
		return nullptr;
	}

	ASSERT_EQ( ( alignment & ( alignment - 1 ) ), 0 );

	const uint64 paddedSize = ( size + sizeof( AllocationRecord ) + alignment - 1 ) & ~( alignment - 1 );
	const uint64 dataSize = paddedSize - sizeof( AllocationRecord );

	uint32 level;
	uint32 count;

	memoryChunkSystem.SizeToLevel( paddedSize, &level, &count );

	MemoryChunkRecord* found = nullptr;
	uint32 chunkID = level | ( 1ull << 31 );

	for ( uint64& chunkArea : chunkAllocators[level].availableChunks ) {
		uint64 area = chunkArea;
		while ( area ) {
			uint64 chunk = FindLSB( area );
			MemoryChunkRecord* record = &chunkAllocators[level].chunks[chunk];

			if ( record->chunk.size >= record->offset + paddedSize ) {
				found = record;

				if ( record->offset + paddedSize == record->chunk.size ) {
					UnSetBit( &chunkArea, chunk );
				}

				chunkID |= ( ( &chunkArea - chunkAllocators[level].availableChunks.memory ) * 64 + chunk ) << 4;

				break;
			}

			UnSetBit( &area, chunk );
		}

		if ( found ) {
			break;
		}
	}

	if ( !found ) {
		MemoryChunk chunk = memoryChunkSystem.Alloc( paddedSize );

		ChunkAllocator& allocator = chunkAllocators[chunk.level];
		const uint32 id = chunk.chunkArea * 64 + chunk.chunk;

		SetBit( &allocator.availableChunks[chunk.chunkArea], chunk.chunk );
		allocator.chunks[id].chunk = chunk;
		allocator.chunks[id].offset = 0;

		SetBit( &allocator.allocatedChunks[chunk.chunkArea], chunk.chunk );

		found = &allocator.chunks[id];
		chunkID |= id << 4;
	}

	std::string source = FormatStackTrace( std::stacktrace::current(), true, true );
	AllocationRecord alloc{ .size = dataSize, .alignment = ( uint32 ) alignment, .chunkID = chunkID };

	Q_strncpyz( alloc.source, source.size() < 104 ? source.c_str() : source.c_str() + ( source.size() - 103 ), 103 );
	alloc.source[103] = '\0';

	*( ( AllocationRecord* ) found->chunk.memory + found->offset ) = alloc;

	byte* ret = found->chunk.memory + found->offset + sizeof( AllocationRecord );
	found->offset += paddedSize;
	found->allocs++;

	return ret;
}

void ThreadMemory::Free( byte* memory ) {
	AllocationRecord* record = ( AllocationRecord* ) ( memory - sizeof( AllocationRecord ) );

	if ( record->guardValue != AllocationRecord::HEADER_MAGIC ) {
		Sys::Drop( "Memory chunk corrupted: %s", record->Format() );
	}

	ChunkAllocator& allocator = chunkAllocators[record->chunkID & 0xF];
	uint32 chunkID = record->chunkID >> 4;

	UnSetBit( &record->chunkID, 31 );

	uint32 area = chunkID / 64;
	uint32 chunk = chunkID - area;
	allocator.chunks[chunkID].allocs--;

	if ( !allocator.chunks[chunkID].allocs ) {
		allocator.chunks[chunkID].offset = 0;
	}

	SetBit( &allocator.availableChunks[area], chunk );
}

void ThreadMemory::FreeAllChunks() {
	for ( ChunkAllocator& allocator : chunkAllocators ) {
		for ( uint32 i = 0; i < allocator.allocatedChunks.elements; i++ ) {
			uint64& allocatedChunk = allocator.allocatedChunks[i];

			while ( allocatedChunk ) {
				uint32 chunk = FindLSB( allocatedChunk );
				MemoryChunkRecord* record = &allocator.chunks[i * 64 + chunk];

				if ( record->allocs ) {
					Log::WarnTagT( "Non-freed allocations in memory chunk:" );
					PrintChunkInfo( record );
				}

				memoryChunkSystem.Free( &record->chunk );

				UnSetBit( &allocatedChunk, chunk );
				UnSetBit( &allocator.availableChunks[i], chunk );
			}
		}
	}
}

void ThreadMemory::PrintChunkInfo( MemoryChunkRecord* memoryChunk ) {
	Log::NoticeTagT( "Chunk: size: %u, offset: %u, active allocations: %u",
		memoryChunk->chunk.size, memoryChunk->offset, memoryChunk->allocs );

	uint64 offset = 0;
	AllocationRecord* record = ( AllocationRecord* ) memoryChunk->chunk.memory;

	uint32 allocs = 0;
	while ( allocs < memoryChunk->allocs ) {
		if ( BitSet( record->chunkID, 31 ) ) {
			Log::NoticeTagT( record->Format() );
			allocs++;
		}

		if ( offset + record->size > memoryChunk->chunk.size ) {
			Log::WarnTagT( "Chunk corrupted, allocation out of bounds (offset: %u, allocation size: %u, chunk size: %u),"
				" aborting chunk info print",
				offset, record->size, memoryChunk->chunk.size );
			return;
		}

		offset += record->size;
		record = ( AllocationRecord* ) ( memoryChunk->chunk.memory + offset );
	}
}

thread_local ThreadMemory TLM;