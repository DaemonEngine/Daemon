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

#include <cstdint>
#include <type_traits>

#include "Memory.h"
#include "IteratorSeq.h"
#include "../SrcDebug/Tag.h"

template<typename T>
class DynamicArray :
	public Tag {

	public:
	uint64_t elements = 0;
	uint64_t size = 0;
	uint64_t highestUsed = 0;
	T* memory;

	DynamicArray() {
	}

	DynamicArray( const std::string name ) :
		Tag( name ) {
	}

	DynamicArray( std::initializer_list<T> args ) {
		Resize( args.size() );
		highestUsed = elements;

		for ( uint64_t i = 0; i < args.size(); i++ ) {
			memory[i] = args.begin()[i];
		}
	}

	DynamicArray( const std::string name, std::initializer_list<T> args ) :
		Tag( name ) {
		Resize( args.size() );
		highestUsed = elements;

		for ( uint64_t i = 0; i < args.size(); i++ ) {
			memory[i] = args.begin()[i];
		}
	}

	~DynamicArray() {
		Resize( 0 );
	}

	void Resize( const uint64_t newElements ) {
		ASSERT_GE( newElements, 0 );

		const uint64_t newSize = newElements * sizeof( T );

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

			FreeAligned( memory );

			if ( elements ) {
				memory = ( T* ) Alloc64( newSize );
			}

			return;
		}

		T* newMemory = ( T* ) Alloc64( newSize );

		if ( elements ) {
			if constexpr ( std::is_trivially_copy_constructible<T>() ) {
				memcpy( newMemory, memory, size );
			} else {
				for ( T* current = memory + newElements, newMem = newMemory; current < memory + elements; current++, newMem++ ) {
					*newMem = *current;
				}
			}

			FreeAligned( memory );
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

	const T& operator[]( const uint64_t index ) const {
		ASSERT_LT( index, elements );
		return memory[index];
	}

	T& operator[]( const uint64_t index ) {
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
};

#endif // DYNAMIC_ARRAY_H