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
// DynamicArray.h

#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <type_traits>

#include "../Math/NumberTypes.h"

#include "Memory.h"
#include "../Thread/TLMAllocator.h"
#include "IteratorSeq.h"
#include "../SrcDebug/Tag.h"

template<typename T>
class DynamicArray :
	public Tag {

	public:
	uint64     size   = 0;
	uint64     memorySize = 0;
	uint64     highestID  = 0;
	T*         memory;
	Allocator* allocator;

	DynamicArray( Allocator* newAllocator = &TLMAlloc ) :
		allocator( newAllocator ) {
	}

	DynamicArray( const std::string name, Allocator* newAllocator = &TLMAlloc ) :
		Tag( name ),
		allocator( newAllocator ) {
	}

	DynamicArray( std::initializer_list<T> args, Allocator* newAllocator = &TLMAlloc ) :
		allocator( newAllocator ) {
		Resize( args.size() );
		highestID = size;

		for ( uint64 i = 0; i < args.size(); i++ ) {
			memory[i] = args.begin()[i];
		}
	}

	DynamicArray( const std::string name, std::initializer_list<T> args, Allocator* newAllocator = &TLMAlloc ) :
		Tag( name ),
		allocator( newAllocator ) {
		Resize( args.size() );
		highestID = size;

		for ( uint64 i = 0; i < args.size(); i++ ) {
			memory[i] = args.begin()[i];
		}
	}

	DynamicArray( const DynamicArray<T>& other ) {
		*this = other;
	}

	DynamicArray( DynamicArray<T>&& other ) {
		*this = other;

		other.Resize( 0 );
	}

	~DynamicArray() {
		Resize( 0 );
	}

	DynamicArray<T>& operator=( const DynamicArray<T>& other ) {
		allocator = other.allocator;
		highestID = other.highestID;

		Resize( other.size );

		if ( size ) {
			if constexpr ( std::is_trivially_copy_constructible<T>() ) {
				memcpy( memory, other.memory, memorySize );
			} else {
				for ( T* current = other.memory, *newMem = memory; current < other.memory + size; current++, newMem++ ) {
					*newMem = *current;
				}
			}
		}

		return *this;
	}

	DynamicArray<T>& operator=( DynamicArray<T>&& other ) {
		*this = other;

		other.Resize( 0 );

		return *this;
	}

	void Resize( const uint64 newSize ) {
		ASSERT_GE( newSize, 0 );

		const uint64 newMemorySize = newSize * sizeof( T );

		if ( newSize == size ) {
			return;
		}

		if ( newSize < size ) {
			if constexpr ( !std::is_trivially_destructible<T>() ) {
				for ( T* current = memory + newSize; current < memory + size; current++ ) {
					current->~T();
				}
			}

			size       = newSize;
			memorySize = newMemorySize;

			allocator->Free( ( byte* ) memory );

			if ( size ) {
				memory = ( T* ) allocator->Alloc( newMemorySize, 64 );
			}

			return;
		}

		T* newMemory = ( T* ) allocator->Alloc( newMemorySize, 64 );

		if ( size ) {
			if constexpr ( std::is_trivially_copy_constructible<T>() ) {
				memcpy( newMemory, memory, memorySize );
			} else {
				for ( T* current = memory, *newMem = newMemory; current < memory + size; current++, newMem++ ) {
					*newMem = *current;
				}
			}

			allocator->Free( ( byte* ) memory );
		}

		memory     = newMemory;
		size       = newSize;
		memorySize = newMemorySize;
	}

	void Condense() {
		Resize( highestID + 1 );
	}

	void Zero() {
		memset( memory, 0, memorySize );
	}

	void Init() {
		for ( T* current = memory; current < memory + size; current++ ) {
			if constexpr ( std::is_trivially_constructible<T>() ) {
				*current = {};
			} else {
				*current = T();
			}
		}
	}

	void Push( T element ) {
		( *this )[highestID] = element;
	}

	const T& operator[]( const uint64 index ) const {
		ASSERT_LT( index, size );
		return memory[index];
	}

	T& operator[]( const uint64 index ) {
		if ( index >= size ) {
			Resize( index + 1 );
		}

		highestID = std::max( index + 1, highestID );
		return memory[index];
	}

	constexpr IteratorSeq<T> begin() {
		return IteratorSeq<T>{ &memory[0] };
	}

	constexpr IteratorSeq<T> end() {
		return IteratorSeq<T>{ &memory[size] };
	}

	constexpr IteratorSeq<T> begin() const {
		return IteratorSeq<T>{ &memory[0] };
	}

	constexpr IteratorSeq<T> end() const {
		return IteratorSeq<T>{ &memory[size] };
	}
};

#endif // DYNAMIC_ARRAY_H