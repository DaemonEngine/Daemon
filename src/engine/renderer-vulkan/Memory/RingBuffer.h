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

#include "../Math/NumberTypes.h"

#include "../Shared/Timer.h"
#include "../SrcDebug/Tag.h"

#include "../Sys/MemoryInfo.h"
#include "../Math/Bit.h"

#include "Memory.h"

template<typename T>
class RingBuffer :
	public Tag {

	public:
	RingBuffer() {
	}

	RingBuffer( const std::string name ) :
		Tag( name ) {
	}

	void Alloc( const uint64 newElementCount ) {
		elementCount = newElementCount;
		size = ( elementCount * sizeof( T ) + 63 ) & ~63;
		memory = ( T* ) Alloc64( size );

		pointer = 0;
	}

	void Resize( const uint64 newElementCount ) {
		if ( newElementCount < elementCount ) {
			Log::WarnTag( "newElementCount < elementCount (%u < %u)", newElementCount, elementCount );
			return;
		}

		const uint64 tempSize = ( newElementCount * sizeof( T ) + 63 ) & ~64;
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

	const T& operator[]( const uint64 index ) const {
		ASSERT_LT( index, elementCount );
		return memory[index];
	}

	T& operator[]( const uint64 index ) {
		ASSERT_LT( index, elementCount );
		return memory[index];
	}

	private:
	T* memory;
	uint64 size;
	uint64 elementCount;
	uint64 pointer;
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

	void Alloc( const uint64 newElementCount ) {
		elementCount = newElementCount;
		size = ( elementCount * sizeof( T ) + 63 ) & ~64;
		mask = elementCount - 1;
		memory = ( T* ) Alloc64( size );

		memset( memory, 0, size );

		pointer = 0;
		current = 0;

		addTimer.Clear();
		getTimer.Clear();
	}

	void Resize( const uint64 newElementCount ) {
		if ( newElementCount < elementCount ) {
			Log::WarnTag( "newElementCount < elementCount (%u < %u)",
				newElementCount, elementCount );
			return;
		}

		const uint64 tempSize = ( newElementCount * sizeof( T ) + 63 ) & ~64;
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
		uint64 element = pointer.fetch_add( 1, std::memory_order_relaxed );
		element &= mask;

		while ( memory[element].active ) {
			std::this_thread::yield();
			Log::DebugTag( "Yielding" );
		}

		memory[element].active = true;

		return &memory[element];
	}

	T* GetCurrentElement() {
		uint64 expected = current.load( std::memory_order_acquire );
		uint64 desired;

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

	const T& operator[]( const uint64 index ) const {
		ASSERT_LT( index, elementCount );
		return memory[index];
	}

	T& operator[]( const uint64 index ) {
		ASSERT_LT( index, elementCount );
		return memory[index];
	}

	private:
	uint64 size;
	uint64 mask;
	ALIGN_CACHE uint64 elementCount;
	ALIGN_CACHE std::atomic<uint64> pointer;
	ALIGN_CACHE std::atomic<uint64> current;
};

#endif // RINGBUFFER_H