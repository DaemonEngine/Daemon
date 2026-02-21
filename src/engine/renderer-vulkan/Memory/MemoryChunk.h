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

#include "../Math/NumberTypes.h"

#include "../Sync/AccessLock.h"
#include "../Sync/AlignedAtomic.h"

struct MemoryChunk {
	uint8  level;
	uint8  chunk;
	uint8  chunkArea;
	uint32 size;
	byte*  memory;
};

struct MemoryAreaConfig {
	uint64 chunkSize;
	uint32 chunks;
	uint32 chunkAreas;
};

struct MemoryArea {
	MemoryAreaConfig     config;

	byte*                memory;
	AlignedAtomicUint64* chunkLocks; // 1 - locked
};

struct MemoryChunkRecord {
	MemoryChunk chunk;
	uint64      offset;
	uint32      allocs;
};

struct ChunkAllocator {
	uint64            allocatedChunks;
	uint64            availableChunks;
	MemoryChunkRecord chunks[64];
	AccessLock        accessLock;
};

constexpr uint32 MAX_MEMORY_AREAS = 3;

struct MemoryChunkConfig {
	MemoryAreaConfig areas[MAX_MEMORY_AREAS];
};

constexpr uint64 memoryChunkConfigRequired[][2] {
	{ 16 * 1024, 640 }, { 1024 * 1024, 640 }, { 64 * 1024 * 1024, 16 }
};

constexpr const char* defaultMemoryChunkConfig = "16:640 1024:640 65536:16";

#endif // MEMORY_CHUNK_H