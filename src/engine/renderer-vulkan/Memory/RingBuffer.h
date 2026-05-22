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

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "../Math/NumberTypes.h"

#include "../Shared/Timer.h"
#include "../SrcDebug/Tag.h"

#include "../Math/Bit.h"

#include "Memory.h"
#include "Allocator.h"
#include "../Thread/TLMAllocator.h"

template<typename T>
class RingBuffer :
	public Tag {

	public:
	RingBuffer( Allocator* newAllocator = &TLMAlloc ) :
		allocator( newAllocator ) {
	}

	RingBuffer( const std::string name, Allocator* newAllocator = &TLMAlloc ) :
		Tag( name ),
		allocator( newAllocator ) {
	}

	void Alloc( const uint64 newSize ) {
		size       = newSize;
		memorySize = ( size * sizeof( T ) + 63 ) & ~63;
		memory     = ( T* ) allocator->Alloc( memorySize, 64 );

		pointer = 0;
	}

	void Resize( const uint64 newSize ) {
		if ( newSize < size ) {
			Log::WarnTag( "newSize < size (%u < %u)", newSize, size );
			return;
		}

		const uint64 tempSize   = ( newSize * sizeof( T ) + 63 ) & ~64;
		T*           tempMemory = ( T* ) allocator->Alloc( tempSize, 64 );

		memcpy( tempMemory, memory, memorySize );

		Free();

		memory     = tempMemory;
		size       = newSize;
		memorySize = tempSize;
	}

	void Free() {
		allocator->Free( memory );
	}

	T* GetNextElement() {
		pointer++;
		pointer &= ~( size - 1 );

		return memory[pointer];
	}

	const T& operator[]( const uint64 index ) const {
		ASSERT_LT( index, size );
		return memory[index];
	}

	T& operator[]( const uint64 index ) {
		ASSERT_LT( index, size );
		return memory[index];
	}

	private:
	T*         memory;
	Allocator* allocator;
	uint64     memorySize;
	uint64     size;
	uint64     pointer;
};

template<typename T, const bool useTrailingAtomic = false>
class AtomicRingBuffer :
	public Tag {

	public:
	T*         memory;
	Allocator* allocator;

	uint64     mask;

	Timer      getTimer;
	Timer      addTimer;

	AtomicRingBuffer( Allocator* newAllocator = &TLMAlloc ) :
		allocator( newAllocator ) {
	}

	AtomicRingBuffer( const std::string name, Allocator* newAllocator = &TLMAlloc ) :
		Tag( name ),
		allocator( newAllocator ) {
	}

	void Alloc( const uint64 newSize ) {
		size       = newSize;
		memorySize = ( size * sizeof( T ) + 63 ) & ~64;
		mask       = size - 1;
		memory     = ( T* ) allocator->Alloc( memorySize, 64 );

		memset( memory, 0, size );

		pointer    = 0;
		current    = 0;

		addTimer.Clear();
		getTimer.Clear();
	}

	void Resize( const uint64 newSize ) {
		if ( newSize < size ) {
			Log::WarnTag( "newSize < size (%u < %u)",
				newSize, size );
			return;
		}

		const uint64 tempSize   = ( newSize * sizeof( T ) + 63 ) & ~64;
		T*           tempMemory = ( T* ) allocator->Alloc( tempSize, 64 );

		memcpy( tempMemory, memory, memorySize );

		memset( tempMemory + ( newSize - size ), 0, memorySize );

		Free();

		memory     = tempMemory;
		size       = newSize;
		memorySize = tempSize;
	}

	void Free() {
		allocator->Free( memory );
	}

	T* GetNextElementMemory( const uint64 count = 1 ) {
		if constexpr ( !useTrailingAtomic ) {
			ASSERT_EQ( count, 1 );
		}

		uint64 element;
		do {
			element = pointer.fetch_add( count, std::memory_order_relaxed );
		} while ( element % size + count > size );

		if constexpr ( useTrailingAtomic ) {
			uint64 currentElement = current.load( std::memory_order_acquire );

			while ( currentElement > element || element - currentElement >= size ) {
				std::this_thread::yield();
				currentElement = current.load( std::memory_order_acquire );
			}

			element &= mask;
		} else {
			element &= mask;
			while ( memory[element].active ) {
				std::this_thread::yield();

			}

			memory[element].active = true;
		}

		return &memory[element];
	}

	uint64 GetNextElement( const uint64 count = 1 ) {
		static_assert( useTrailingAtomic, "GetNextElement() must only be used with useTrailingAtomic = true!" );

		uint64 element;
		do {
			element = pointer.fetch_add( count, std::memory_order_relaxed );
		} while ( element % size + count > size );

		uint64 currentElement = current.load( std::memory_order_acquire );

		while ( currentElement > element || element - currentElement >= size ) {
			std::this_thread::yield();
			currentElement = current.load( std::memory_order_acquire );
		}

		return element;
	}

	void UpdateCurrentElement( uint64 newCurrent ) {
		static_assert( useTrailingAtomic, "UpdateCurrentElement() must only be used with useTrailingAtomic = true!" );

		uint64 expected = current.load( std::memory_order_relaxed );
		do {
			if ( expected > newCurrent ) {
				return;
			}
		} while ( !current.compare_exchange_weak( expected, newCurrent, std::memory_order_relaxed ) );
	}

	const T& operator[]( const uint64 index ) const {
		ASSERT_LT( index, size );
		return memory[index];
	}

	T& operator[]( const uint64 index ) {
		ASSERT_LT( index, size );
		return memory[index];
	}

	private:
	uint64              memorySize;
	uint64              size;
	std::atomic<uint64> pointer;
	std::atomic<uint64> current;
};

#endif // RINGBUFFER_H