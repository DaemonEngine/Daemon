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
// MemoryChunk.h

#ifndef MEMORY_CHUNK_H
#define MEMORY_CHUNK_H

#include <cstdint>
#include <atomic>
#include <iostream>
#include <thread>

#include "../Math/Bit.h"
#include "../Shared/Timer.h"
#include "../SrcDebug/Tag.h"

#include "Memory.h"

struct MemoryChunk {
	uint8_t area;
	uint8_t chunk;
	uint32_t chunkArea;
	uint32_t size;
	byte* memory;
};

struct AlignedAtomicUint64 {
	alignas( 64 ) std::atomic<uint64_t> value;
};

struct MemoryAreaConfig {
	uint64_t chunkSize;
	uint32_t chunks;
	uint32_t chunkAreas;
};

struct MemoryArea {
	MemoryAreaConfig config;

	byte* memory;
	AlignedAtomicUint64* chunkLocks; // 1 - locked
};

constexpr uint32_t MAX_MEMORY_AREAS = 3;

struct MemoryChunkConfig {
	MemoryAreaConfig areas[MAX_MEMORY_AREAS];
};

class MemoryChunkSystem :
	public Tag {
	
	public:
	MemoryChunkConfig config;
	MemoryArea memoryAreas[MAX_MEMORY_AREAS];

	MemoryChunkSystem();
	~MemoryChunkSystem();

	MemoryChunk Alloc( uint64_t size );
	void Free( MemoryChunk* memoryChunk );

	void SizeToLevel( const uint64_t size, uint32_t* level, uint32_t* count );

	private:
	bool LockArea( MemoryArea* memoryArea, uint32_t* chunkArea, uint8_t* chunk );
};

void UpdateMemoryChunkSystemConfig();

extern MemoryChunkSystem memoryChunkSystem;

#endif // MEMORY_CHUNK_H