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
	uint64 elements = 0;
	uint64 size = 0;
	uint64 highestUsed = 0;
	T* memory;
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
		highestUsed = elements;

		for ( uint64 i = 0; i < args.size(); i++ ) {
			memory[i] = args.begin()[i];
		}
	}

	DynamicArray( const std::string name, std::initializer_list<T> args, Allocator* newAllocator = &TLMAlloc ) :
		Tag( name ),
		allocator( newAllocator ) {
		Resize( args.size() );
		highestUsed = elements;

		for ( uint64 i = 0; i < args.size(); i++ ) {
			memory[i] = args.begin()[i];
		}
	}

	~DynamicArray() {
		Resize( 0 );
	}

	void Resize( const uint64 newElements ) {
		ASSERT_GE( newElements, 0 );

		const uint64 newSize = newElements * sizeof( T );

		if ( newElements == elements ) {
			return;
		}

		if ( newElements < elements ) {
			if constexpr ( !std::is_trivially_destructible<T>() ) {
				for ( T* current = memory + newElements; current < memory + elements; current++ ) {
					current->~T();
				}
			}

			elements = newElements;
			size = newSize;

			allocator->Free( ( byte* ) memory );

			if ( elements ) {
				memory = ( T* ) allocator->Alloc( newSize, 64 );
			}

			return;
		}

		T* newMemory = ( T* ) allocator->Alloc( newSize, 64 );

		if ( elements ) {
			if constexpr ( std::is_trivially_copy_constructible<T>() ) {
				memcpy( newMemory, memory, size );
			} else {
				for ( T* current = memory + newElements, *newMem = newMemory; current < memory + elements; current++, newMem++ ) {
					*newMem = *current;
				}
			}

			allocator->Free( ( byte* ) memory );
		}

		memory = newMemory;
		elements = newElements;
		size = newSize;
	}

	void Condense() {
		Resize( highestUsed + 1 );
	}

	void Zero() {
		memset( memory, 0, size );
	}

	void Init() {
		for ( T* current = memory; current < memory + elements; current++ ) {
			if constexpr ( true || std::is_trivially_constructible<T>() ) {
				*current = {};
			} else {
				current->T();
			}
		}
	}

	const T& operator[]( const uint64 index ) const {
		ASSERT_LT( index, elements );
		return memory[index];
	}

	T& operator[]( const uint64 index ) {
		ASSERT_LT( index, elements );
		highestUsed = std::max( index, highestUsed );
		return memory[index];
	}

	constexpr IteratorSeq<T> begin() {
		return IteratorSeq<T>{ &memory[0] };
	}

	constexpr IteratorSeq<T> end() {
		return IteratorSeq<T>{ &memory[elements] };
	}

	constexpr IteratorSeq<T> begin() const {
		return IteratorSeq<T>{ &memory[0] };
	}

	constexpr IteratorSeq<T> end() const {
		return IteratorSeq<T>{ &memory[elements] };
	}
};

#endif // DYNAMIC_ARRAY_H