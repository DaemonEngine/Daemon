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
#include "../Memory/MemoryChunkSystem.h"
#include "../Error.h"

#include "ThreadMemory.h"

ThreadMemory::~ThreadMemory() {
	for ( DynamicArray<ChunkAllocator>& chunkAllocator : chunkAllocators ) {
		for ( ChunkAllocator& alloc : chunkAllocator ) {
			uint64 chunkArea = alloc.availableChunks;

			while ( chunkArea ) {
				uint32 chunk = FindLSB( chunkArea );
				Log::WarnTagT( "Unreturned memory chunk:" );

				PrintChunkInfo( &alloc.chunks[chunk] );

				UnSetBit( &chunkArea, chunk );
			}
		}
	}
}

void ThreadMemory::Init() {
	if ( initialised ) {
		Log::DebugTagT( "TLM already initialised" );
		return;
	}

	for ( uint32 i = 0; i < MAX_MEMORY_AREAS; i++ ) {
		DynamicArray<ChunkAllocator>& chunkAllocator = chunkAllocators[i];
		MemoryAreaConfig&             config         = memoryChunkSystem.config.areas[i];

		chunkAllocator.Resize( config.chunkAreas );
		chunkAllocator.Zero();
	}

	initialised = true;
}

byte* ThreadMemory::Alloc( const uint64 size, const uint64 alignment ) {
	if ( !size ) {
		return nullptr;
	}

	ASSERT_EQ( ( alignment & ( alignment - 1 ) ), 0 );

	const uint64 paddedSize = ( size + sizeof( AllocationRecord ) + alignment - 1 ) & ~( alignment - 1 );

	uint32 level;
	uint32 count;

	memoryChunkSystem.SizeToLevel( paddedSize, &level, &count );

	MemoryChunkRecord* record  = nullptr;
	uint32             chunkID = level | ( 1ull << 31 );

	for ( uint32 i = 0; i < chunkAllocators[level].size && !record; i++ ) {
		ChunkAllocator& chunkAllocator = chunkAllocators[level][i];

		uint64&         chunkArea      = chunkAllocator.availableChunks;
		uint64          area           = chunkArea;

		while ( area ) {
			uint64 chunk = FindLSB( area );
			record       = &chunkAllocator.chunks[chunk];

			if ( record->chunk.size >= record->offset + paddedSize ) {
				if ( record->offset + paddedSize == record->chunk.size ) {
					UnSetBit( &chunkArea, chunk );
				}

				chunkID |= ( i * 64 + chunk ) << 4;

				break;
			} else {
				record = nullptr;
			}

			UnSetBit( &area, chunk );
		}
	}

	if ( !record ) {
		MemoryChunk     chunk              = memoryChunkSystem.Alloc( paddedSize );

		ChunkAllocator& chunkAllocator     = chunkAllocators[chunk.level][chunk.chunkArea];
		const uint32    id                 = chunk.chunkArea * 64 + chunk.chunk;

		SetBit( &chunkAllocator.availableChunks, chunk.chunk );
		chunkAllocator.chunks[chunk.chunk] = { .chunk = chunk };

		SetBit( &chunkAllocator.allocatedChunks, chunk.chunk );

		record   = &chunkAllocator.chunks[chunk.chunk];
		chunkID |= id << 4;
	}

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

	AllocationRecord alloc {
		.size      = paddedSize - sizeof( AllocationRecord ),
		.alignment = ( uint32 ) alignment,
		.chunkID   = chunkID
	};

	Q_strncpyz( alloc.source, source.c_str(), 103 );
	alloc.source[103] = '\0';

	*( ( AllocationRecord* ) ( record->chunk.memory + record->offset ) ) = alloc;

	byte* ret         = record->chunk.memory + record->offset + sizeof( AllocationRecord );
	record->offset   += paddedSize;
	record->allocs++;

	return ret;
}

void ThreadMemory::Free( byte* memory ) {
	AllocationRecord* record = ( AllocationRecord* ) ( memory - sizeof( AllocationRecord ) );

	if ( record->guardValue != AllocationRecord::HEADER_MAGIC ) {
		Err( "Memory chunk corrupted: %s", record->Format() );
	}

	UnSetBit( &record->chunkID, 31 );

	uint32             chunkID        = record->chunkID >> 4;
	uint32             area           = chunkID / 64;
	ChunkAllocator&    chunkAllocator = chunkAllocators[record->chunkID & 0xF][area];

	uint32             chunk          = chunkID - area;

	MemoryChunkRecord& chunkRecord    = chunkAllocator.chunks[chunk];

	chunkRecord.allocs--;

	if ( !chunkRecord.allocs ) {
		chunkRecord.offset = 0;
	}

	SetBit( &chunkAllocator.availableChunks, chunk );
}

void ThreadMemory::FreeAllChunks() {
	for ( DynamicArray<ChunkAllocator>& allocs : chunkAllocators ) {
		for ( ChunkAllocator& chunkAllocator : allocs ) {
			uint64&                allocatedChunk = chunkAllocator.allocatedChunks;

			while ( allocatedChunk ) {
				uint32             chunk          = FindLSB( allocatedChunk );
				MemoryChunkRecord* record         = &chunkAllocator.chunks[chunk];

				if ( record->allocs ) {
					Log::WarnTagT( "Non-freed allocations in memory chunk:" );
					PrintChunkInfo( record );
				}

				memoryChunkSystem.Free( &record->chunk );

				UnSetBit( &allocatedChunk, chunk );
				UnSetBit( &chunkAllocator.availableChunks, chunk );
			}
		}
	}
}

void ThreadMemory::PrintChunkInfo( MemoryChunkRecord* memoryChunk ) {
	Log::NoticeTagT( "Chunk: size: %u, offset: %u, active allocations: %u",
		memoryChunk->chunk.size, memoryChunk->offset, memoryChunk->allocs );

	uint64            offset = 0;
	AllocationRecord* record = ( AllocationRecord* ) memoryChunk->chunk.memory;

	uint32 allocs = 0;
	while ( allocs < memoryChunk->allocs ) {
		if ( BitSet( record->chunkID, 31 ) ) {
			Log::NoticeTagT( record->Format() );
			allocs++;
		}

		if ( offset + record->size > memoryChunk->chunk.size || !record->size ) {
			Log::WarnTagT( "Chunk corrupted, allocation out of bounds (offset: %u, allocation size: %u, chunk size: %u),"
				" aborting chunk info print",
				offset, record->size, memoryChunk->chunk.size );
			return;
		}

		offset += record->size;
		record  = ( AllocationRecord* ) ( memoryChunk->chunk.memory + offset );
	}
}

void ThreadMemory::AddTask( Task* task ) {
	const uint32 taskID = FindLZeroBit( tasksState );

	if ( taskID == 64 ) {
		Err( "Overflowed internal task buffer in thread %u (max tasks: %u)", id, maxInternalTasks );

		return;
	}

	tasks[taskID]       = task;
	SetBit( &tasksState, taskID );
}

Task* ThreadMemory::FetchTask() {
	const uint32 taskID = FindLSB( tasksState );

	if ( taskID == 64 ) {
		return nullptr;
	}

	Task* task          = tasks[taskID];
	tasks[taskID]       = nullptr;
	UnSetBit( &tasksState, taskID );

	return task;
}

thread_local ThreadMemory TLM;