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
// StackAllocator.cpp

#include "StackAllocator.h"

StackAllocator::StackAllocator( Allocator* newAllocator ) :
	allocator( newAllocator ) {
}

void StackAllocator::Init( const uint64_t newSize ) {
	size = ( newSize + 63 ) & ~64;
	memory = allocator->Alloc( size, 64 );

	persistentIndex = 0;
	tempIndex = 0;
}

void StackAllocator::Resize( const uint64_t newSize ) {
	if ( newSize > persistentIndex + tempIndex ) {
		Sys::Drop( "StackAllocator: failed to resize: %u bytes > %u persistent bytes + %u temp bytes",
			newSize, persistentIndex, tempIndex );
	}

	const uint64_t tempSize = ( newSize + 63 ) & ~64;
	byte* tempMemory = allocator->Alloc( tempSize, 64 );

	memcpy( tempMemory, memory, persistentIndex );
	memcpy( tempMemory + ( tempSize - tempIndex ), memory + ( size - tempIndex ), persistentIndex );

	Free();

	memory = tempMemory;
	size = tempSize;
}

void StackAllocator::Free() {
	allocator->Free( memory );
}

byte* StackAllocator::Alloc( const uint64_t allocationSize, const uint64_t alignment ) {
	const uint64_t paddedSize = ( allocationSize + alignment - 1 ) & ~alignment;

	if ( paddedSize > size - persistentIndex - tempIndex ) {
		return nullptr;
	}

	/* if ( temp ) {
		tempIndex += paddedSize;
		return ( void* ) ( memory - tempIndex );
	} */

	byte* ptr = memory + persistentIndex;
	persistentIndex += paddedSize;

	return ptr;
}

void StackAllocator::Free( byte* memory ) {
	if ( !memory ) {
		return;
	}

	allocator->Free( memory );
}
