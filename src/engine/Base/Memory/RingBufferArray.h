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

#ifndef RINGBUFFER_ARRAY_H
#define RINGBUFFER_ARRAY_H

#include "SrcDebug/Tag.h"
#include "Sync/AlignedAtomic.h"
#include "Sys/CPUInfo.h"
#include "Thread/ThreadCommon.h"
#include "Thread/TLMAllocator.h"
#include "Int.h"

#include "Allocator.h"

template<typename T, const bool useTrailingAtomic = false>
class AtomicRingBufferArray :
	public Tag {

	public:
	T*         memory;
	Allocator* allocator;

	uint64     mask;
	uint64     size; // Per thread

	AtomicRingBufferArray( Allocator* newAllocator = &TLMAlloc ) :
		allocator( newAllocator ) {
	}

	AtomicRingBufferArray( const std::string name, Allocator* newAllocator = &TLMAlloc ) :
		Tag( name ),
		allocator( newAllocator ) {
	}

	void Alloc( const uint64 newSize ) {
		size        = newSize;
		memorySize  = ( size * sizeof( T ) + 63 ) & ~64;
		memorySize *= THREAD_ARRAY_SIZE;
		mask        = size - 1;

		memory      = ( T* ) allocator->Alloc( memorySize, 64 );

		memset( memory, 0, size );

		for ( uint8 i = 0; i < THREAD_ARRAY_SIZE; i++ ) {
			pointer[i] = 0;
			current[i] = 0;
		}
	}

	void Resize( uint64 newSize ) {
		if ( newSize < size ) {
			Log::WarnTag( "newSize < size (%u < %u)",
				newSize, size );
			return;
		}

		uint64 tempSize   = ( newSize * sizeof( T ) + 63 ) & ~64;
		tempSize         *= THREAD_ARRAY_SIZE;

		T*     tempMemory = ( T* ) allocator->Alloc( tempSize, 64 );

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

	T* GetNextElementMemory( const uint8 threadID, const uint64 count = 1 ) {
		if constexpr ( !useTrailingAtomic ) {
			ASSERT_EQ( count, 1 );
		}

		uint64 element     = pointer[threadID];

		if ( element % size + count > size ) {
			element = ( element / size + 1 ) * size;
		}

		pointer[threadID]  = element + count;

		if constexpr ( useTrailingAtomic ) {
			uint64 currentElement = current[threadID].value.load( std::memory_order_acquire );

			while ( currentElement > element || element - currentElement >= size ) {
				std::this_thread::yield();
				currentElement = current[threadID].value.load( std::memory_order_acquire );
			}

			element &= mask;
		} else {
			element &= mask;

			while ( memory[element + threadID * size].IsActive() ) {
				std::this_thread::yield();
			}

			memory[element + threadID * size].SetActive( true );
		}

		return &memory[element + threadID * size];
	}

	uint64 GetNextElement( const uint8 threadID, const uint64 count ) {
		static_assert( useTrailingAtomic, "GetNextElement() must only be used with useTrailingAtomic = true!" );

		uint64 element = pointer[threadID];

		if ( element % size + count > size ) {
			element = ( element / size + 1 ) * size;
		}

		pointer[threadID]     = element + count;

		uint64 currentElement = current[threadID].value.load( std::memory_order_acquire );

		while ( currentElement > element || element - currentElement >= size ) {
			std::this_thread::yield();
			currentElement = current[threadID].value.load( std::memory_order_acquire );
		}

		return element;
	}

	void UpdateCurrentElement( const uint8 threadID, uint64 newCurrent ) {
		static_assert( useTrailingAtomic, "UpdateCurrentElement() must only be used with useTrailingAtomic = true!" );

		uint64 expected = current[threadID].value.load( std::memory_order_relaxed );
		do {
			if ( expected > newCurrent ) {
				return;
			}
		} while ( !current[threadID].value.compare_exchange_weak( expected, newCurrent, std::memory_order_relaxed ) );
	}

	const T& operator[]( const uint8 threadID, const uint64 index ) const {
		return memory[index + threadID * size];
	}

	T& operator[]( const uint8 threadID, const uint64 index ) {
		return memory[index + threadID * size];
	}

	private:
	uint64              memorySize;
	uint32              pointer[THREAD_ARRAY_SIZE];
	AlignedAtomicUint64 current[THREAD_ARRAY_SIZE];
};

#endif // RINGBUFFER_ARRAY_H