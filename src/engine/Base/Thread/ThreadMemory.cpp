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

#include "common/Common.h"

#include "SrcDebug/StackTrace.h"
#include "SrcDebug/Tag.h"
#include "MemoryChunkSystem.h"
#include "Error.h"

#include "ThreadMemory.h"

ThreadMemory::~ThreadMemory() {
	for ( DynamicArray<ChunkAllocator>& chunkAllocator : chunkAllocators ) {
		for ( ChunkAllocator& alloc : chunkAllocator ) {
			uint64 chunkArea = alloc.availableChunks;

			while ( chunkArea ) {
				uint32 chunk = FindLSB( chunkArea );
				Log::WarnTagT( "Unreturned memory chunk:" );

				PrintChunkInfo( &alloc.chunks[chunk], &chunkAllocator - chunkAllocators,
					memoryChunkSystem.GetChunkMemory( &chunkAllocator - chunkAllocators, &alloc - chunkAllocator.memory, chunk ) );

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

ChunkRecord* ThreadMemory::IDToChunkRecord( const uint8 level, const uint8 area, const uint8 chunk ) {
	return &chunkAllocators[level][area].chunks[chunk];
}

ChunkRecord* ThreadMemory::IDToChunkRecord( const uint32 id ) {
	MemoryChunk chunk = IDToMemoryChunk( id );

	return &chunkAllocators[chunk.level][chunk.area].chunks[chunk.chunk];
}

uint32 ThreadMemory::AllocChunk( const uint64 size, const uint64 alignment, const uint8 level ) {
	for ( ChunkAllocator& chunkAllocator : chunkAllocators[level] ) {
		uint64& chunkArea = chunkAllocator.availableChunks;
		uint64  area      = chunkArea;

		while ( area ) {
			const uint32 chunk  = FindLSB( area );
			ChunkRecord* record = IDToChunkRecord( level, &chunkAllocator - chunkAllocators[level].memory, chunk );

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

std::string AllocationRecord::Format() const {
	if ( guardValue == HEADER_MAGIC ) {
		return Str::Format( "guard value: %u, size: %u, alignment: %u, chunkID: %u, source: %s",
			guardValue, size, alignment, chunkID,
			source );
	}

	return Str::Format( "guard value: %u (corrupted, should be: %u), size: %u, alignment: %u, chunkID: %u, source: %s",
		guardValue, HEADER_MAGIC, size, alignment, chunkID,
		source );
}

byte* ThreadMemory::Alloc( const uint64 size, const uint64 alignment ) {
	if ( !size ) {
		return nullptr;
	}

	ASSERT_EQ( ( alignment & ( alignment - 1 ) ), 0 );

	const uint64 paddedSize = ( size + sizeof( AllocationRecord ) + alignment - 1 ) & ~( alignment - 1 );

	uint32 level;
	uint32 count;

	memoryChunkSystem.SizeToLevel( size, &level, &count );

	uint32      chunkID = AllocChunk( paddedSize, alignment, level );

	std::string source  = FormatSrc( std::stacktrace::current(), true, true );

	AllocationRecord alloc {
		.size      = paddedSize - sizeof( AllocationRecord ),
		.alignment = ( uint32 ) alignment,
		.chunkID   = chunkID
	};

	Q_strncpyz( alloc.source, source.c_str(), AllocationRecord::srcSize );
	alloc.source[AllocationRecord::srcSize] = '\0';

	ChunkRecord*           record = IDToChunkRecord( chunkID );
	byte*                  memory = memoryChunkSystem.GetChunkMemory( chunkID ) + record->offset;
	*( AllocationRecord* ) memory = alloc;

	record->offset += paddedSize;
	record->allocs++;

	return memory + sizeof( AllocationRecord );
}

void ThreadMemory::Free( byte* memory ) {
	AllocationRecord* record = ( AllocationRecord* ) ( memory - sizeof( AllocationRecord ) );

	if ( record->guardValue != AllocationRecord::HEADER_MAGIC ) {
		Err( "Memory chunk corrupted: %s", record->Format() );
	}

	UnSetBit( &record->chunkID, chunkAllocOffset );

	MemoryChunk chunk = IDToMemoryChunk( record->chunkID );

	ChunkAllocator& chunkAllocator = chunkAllocators[chunk.level][chunk.area];
	ChunkRecord&    chunkRecord    = chunkAllocator.chunks[chunk.chunk];

	chunkRecord.allocs--;

	if ( !chunkRecord.allocs ) {
		chunkRecord.offset = 0;
	}

	SetBit( &chunkAllocator.availableChunks, chunk.chunk );
}

void ThreadMemory::FreeAllChunks() {
	for ( DynamicArray<ChunkAllocator>& allocs : chunkAllocators ) {
		for ( ChunkAllocator& chunkAllocator : allocs ) {
			uint64& allocatedChunk = chunkAllocator.allocatedChunks;

			while ( allocatedChunk ) {
				uint32       chunk  = FindLSB( allocatedChunk );
				ChunkRecord* record = &chunkAllocator.chunks[chunk];

				if ( record->allocs ) {
					Log::WarnTagT( "Non-freed allocations in memory chunk:" );

					PrintChunkInfo( record, &allocs - chunkAllocators,
						memoryChunkSystem.GetChunkMemory( &allocs - chunkAllocators, &chunkAllocator - allocs.memory, chunk ) );
				}

				memoryChunkSystem.Free( &allocs - chunkAllocators, &chunkAllocator - allocs.memory, chunk );

				UnSetBit( &allocatedChunk, chunk );
				UnSetBit( &chunkAllocator.availableChunks, chunk );
			}
		}
	}
}

void ThreadMemory::PrintChunkInfo( ChunkRecord* memoryChunk, const uint8 level, const byte* memory ) {
	const uint32 chunkSize = memoryChunkSystem.GetChunkSize( level );

	Log::NoticeTagT( "Chunk: size: %u, offset: %u, active allocations: %u",
		chunkSize, memoryChunk->offset, memoryChunk->allocs );

	uint64            offset = 0;
	AllocationRecord* record = ( AllocationRecord* ) memory;

	uint32            allocs = 0;

	while ( allocs < memoryChunk->allocs ) {
		if ( !BitSet( record->chunkID, chunkAllocOffset ) ) {
			Log::NoticeTagT( record->Format() );
			allocs++;
		}

		if ( offset + record->size > chunkSize || !record->size ) {
			Log::WarnTagT( "Chunk corrupted, allocation out of bounds (offset: %u, allocation size: %u, chunk size: %u),"
				" aborting chunk info print",
				offset, record->size, chunkSize );

			return;
		}

		offset += record->size;
		record  = ( AllocationRecord* ) ( memory + offset );
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