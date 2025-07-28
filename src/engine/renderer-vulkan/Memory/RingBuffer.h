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
// RingBuffer.h

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "Memory.h"

#include "../Shared/Timer.h"
#include "../SrcDebug/Tag.h"

#include "../Sys/MemoryInfo.h"

template<typename T>
class RingBuffer :
	public Tag {

	public:
	RingBuffer() {
	}

	RingBuffer( const std::string name ) :
		Tag( name ) {
	}

	void Alloc( const uint64_t newElementCount ) {
		elementCount = newElementCount;
		size = ( elementCount * sizeof( T ) + 63 ) & ~63;
		memory = ( T* ) Alloc64( size );

		pointer = 0;
	}

	void Resize( const uint64_t newElementCount ) {
		if ( newElementCount < elementCount ) {
			Log::WarnTag( "newElementCount < elementCount (%u < %u)", newElementCount, elementCount );
			return;
		}

		const uint64_t tempSize = ( newElementCount * sizeof( T ) + 63 ) & ~64;
		T* tempMemory = ( T* ) Alloc64( tempSize );

		memcpy( tempMemory, memory, size );

		Free();

		memory = tempMemory;
		elementCount = newElementCount;
		size = tempSize;
	}

	void Free() {
		FreeAligned( memory );
	}

	T* GetNextElement() {
		pointer++;
		pointer &= ~( size - 1 );

		return memory[pointer];
	}

	private:
	T* memory;
	uint64_t size;
	uint64_t elementCount;
	uint64_t pointer;
};

template<typename T>
class AtomicRingBuffer :
	public Tag {

	public:
	T* memory;

	Timer getTimer;
	Timer addTimer;

	AtomicRingBuffer() {
	}

	AtomicRingBuffer( const std::string name ) :
		Tag( name ) {
	}

	void Alloc( const uint64_t newElementCount ) {
		elementCount = newElementCount;
		size = ( elementCount * sizeof( T ) + 63 ) & ~64;
		mask = UINT64_MAX >> __lzcnt64( elementCount - 1 );
		memory = ( T* ) Alloc64( size );

		memset( memory, 0, size );

		pointer = 0;
		current = 0;

		addTimer.Clear();
		getTimer.Clear();
	}

	void Resize( const uint64_t newElementCount ) {
		if ( newElementCount < elementCount ) {
			Log::WarnTag( "newElementCount < elementCount (%u < %u)",
				newElementCount, elementCount );
			return;
		}

		const uint64_t tempSize = ( newElementCount * sizeof( T ) + 63 ) & ~64;
		T* tempMemory = ( T* ) Alloc64( tempSize );

		memcpy( tempMemory, memory, size );

		memset( tempMemory + ( newElementCount - elementCount ), 0, size );

		Free();

		memory = tempMemory;
		elementCount = newElementCount;
		size = tempSize;
	}

	void Free() {
		FreeAligned( memory );
	}

	T* GetNextElementMemory() {
		uint64_t element = pointer.fetch_add( 1 );
		element &= mask;

		while ( memory[element].active ) {
			std::this_thread::yield();
			Log::DebugTag( "Yielding" );
		}

		memory[element].active = true;

		return &memory[element];
	}

	T* GetCurrentElement() {
		uint64_t expected = current.load();
		uint64_t desired;

		Timer t;
		do {
			Log::DebugTag( "Retrying" );
			if ( memory[expected].active ) {
				desired = ( expected + 1 ) & mask;
			} else if ( expected < ( pointer & mask ) ) {
				desired = expected + 1;
			} else {
				Log::DebugTag( "None: %s", Timer::FormatTime( t.Time() ) );
				return nullptr;
			}
		} while ( !current.compare_exchange_weak( expected, desired, std::memory_order_relaxed ) );

		Log::DebugTag( Timer::FormatTime( t.Time() ) );

		return &memory[expected];
	}

	private:
	uint64_t size;
	uint64_t mask;
	ALIGN_CACHE uint64_t elementCount;
	ALIGN_CACHE std::atomic<uint64_t> pointer;
	ALIGN_CACHE std::atomic<uint64_t> current;
};

#endif // RINGBUFFER_H