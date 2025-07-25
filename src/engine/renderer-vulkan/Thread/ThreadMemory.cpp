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

#include "ThreadMemory.h"

#include "common/Common.h"

ThreadMemory::~ThreadMemory() {

}

void ThreadMemory::Init() {
	for ( ChunkAllocator& allocator : chunkAllocators ) {
		allocator.availableChunks.Resize( memoryChunkSystem.config.areas->chunkAreas );
		allocator.chunks.Resize( memoryChunkSystem.config.areas->chunks );

		allocator.availableChunks.Zero();
		allocator.chunks.Zero();
	}
}

byte* ThreadMemory::AllocAligned( const uint64_t size, const uint64_t alignment ) {
	if ( !size ) {
		return nullptr;
	}

	ASSERT_EQ( ( alignment & ( alignment - 1 ) ), 0 );

	const uint64_t paddedSize = ( size + sizeof( AllocationRecord ) + alignment - 1 ) & ~( alignment - 1 );
	const uint64_t dataSize = paddedSize - sizeof( AllocationRecord );

	uint32_t level;
	uint32_t count;

	memoryChunkSystem.SizeToLevel( paddedSize, &level, &count );

	MemoryChunkRecord* found = nullptr;
	uint32_t chunkID = level;

	for ( uint64_t& chunkArea : chunkAllocators[level].availableChunks ) {
		uint64_t area = chunkArea;
		while ( area ) {
			uint64_t chunk = FindLSB( area );
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

		SetBit( &chunkAllocators[chunk.area].availableChunks[chunk.chunkArea], chunk.chunk );
		chunkAllocators[chunk.area].chunks[chunk.chunkArea * 64 + chunk.chunk].chunk = chunk;
		chunkAllocators[chunk.area].chunks[chunk.chunkArea * 64 + chunk.chunk].offset = 0;

		found = &chunkAllocators[chunk.area].chunks[chunk.chunkArea * 64 + chunk.chunk];
		chunkID |= ( chunk.chunkArea * 64 + chunk.chunk ) << 4;
	}

	AllocationRecord alloc{ .size = dataSize, .alignment = ( uint32_t ) alignment, .chunkID = chunkID };
	*( ( AllocationRecord* ) found->chunk.memory + found->offset ) = alloc;

	byte* ret = found->chunk.memory + found->offset + sizeof( AllocationRecord );
	found->offset += paddedSize;
	found->allocs++;

	return ret;
}

void ThreadMemory::Free( byte* memory ) {
	AllocationRecord* record = ( AllocationRecord* ) ( memory - sizeof( AllocationRecord ) );

	if ( record->guardValue != AllocationRecord::HEADER_MAGIC ) {
		Sys::Drop( "Memory chunk corrupted: guard value: %u (should be: %u), size: %u, alignment: %u, chunkID: %u, source: %s",
			record->guardValue, AllocationRecord::HEADER_MAGIC, record->size, record->alignment, record->chunkID,
			record->source );
	}

	ChunkAllocator& allocator = chunkAllocators[record->chunkID & 0xF];
	uint32_t chunkID = record->chunkID >> 4;

	uint32_t area = chunkID / 64;
	uint32_t chunk = chunkID - area;
	allocator.chunks[chunkID].allocs--;

	if ( !allocator.chunks[chunkID].allocs ) {
		allocator.chunks[chunkID].offset = 0;
	}

	SetBit( &allocator.availableChunks[area], chunk );
}

thread_local ThreadMemory TLM;