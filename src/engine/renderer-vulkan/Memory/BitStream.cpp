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
// BitStream.cpp

#include "BitStream.h"

uint8 BitRead8( const uint8* stream, const uint32 offset, const uint32 size ) {
	static constexpr uint32 typeSize = 8;

	const uint32 subOffset = offset % typeSize;

	const uint8* start     = stream + offset / typeSize;

	if ( ( offset + size - 1 ) / typeSize > offset / typeSize ) {
		const uint32 startSize = ( offset / typeSize + 1 ) * typeSize - offset;
		const uint32 endSize   = size - startSize;

		return GetBits( *start, 0, startSize ) | GetBits( *( start + 1 ), typeSize - endSize, endSize );
	}

	return GetBits( *start, subOffset, size );
}

uint16 BitRead16( const uint16* stream, const uint32 offset, const uint32 size ) {
	static constexpr uint32 typeSize = 16;

	const uint32 subOffset = offset % typeSize;

	const uint16* start    = stream + offset / typeSize;

	if ( ( offset + size - 1 ) / typeSize > offset / typeSize ) {
		const uint32 startSize = ( offset / typeSize + 1 ) * typeSize - offset;
		const uint32 endSize   = size - startSize;

		return GetBits( *start, 0, startSize ) | GetBits( *( start + 1 ), typeSize - endSize, endSize );
	}

	return GetBits( *start, subOffset, size );
}

uint32 BitRead32( const uint32* stream, const uint32 offset, const uint32 size ) {
	static constexpr uint32 typeSize = 32;

	const uint32 subOffset = offset % typeSize;

	const uint32* start    = stream + offset / typeSize;

	if ( ( offset + size - 1 ) / typeSize > offset / typeSize ) {
		const uint32 startSize = ( offset / typeSize + 1 ) * 32 - offset;
		const uint32 endSize   = size - startSize;

		return GetBits( *start, 0, startSize ) | GetBits( *( start + 1 ), typeSize - endSize, endSize );
	}

	return GetBits( *start, subOffset, size );
}

uint64 BitRead64( const uint64* stream, const uint32 offset, const uint32 size ) {
	static constexpr uint32 typeSize = 64;

	const uint32 subOffset = offset % typeSize;

	const uint64* start    = stream + offset / typeSize;

	if ( ( offset + size - 1 ) / typeSize > offset / typeSize ) {
		const uint32 startSize = ( offset / typeSize + 1 ) * typeSize - offset;
		const uint32 endSize   = size - startSize;

		return GetBits( *start, 0, startSize ) | GetBits( *( start + 1 ), typeSize - endSize, endSize );
	}

	return GetBits( *start, subOffset, size );
}

void BitWrite8( uint8* stream, const uint32 offset, const uint32 size, const uint8 value ) {
	static constexpr uint32 typeSize = 8;

	const uint32 subOffset = offset % typeSize;

	uint8* start           = stream + offset / typeSize;

	if ( ( offset + size - 1 ) / typeSize > offset / typeSize ) {
		const uint32 startSize = ( offset / typeSize + 1 ) * typeSize - offset;
		const uint32 endSize   = size - startSize;

		SetBits( start,     GetBits( value, endSize, startSize ), 0,   startSize );
		SetBits( start + 1, GetBits( value, 0,       endSize ),   0,   endSize );
	}

	SetBits( start, value, subOffset, size);
}

void BitWrite16( uint16* stream, const uint32 offset, const uint32 size, const uint16 value ) {
	static constexpr uint32 typeSize = 16;

	const uint32 subOffset = offset % typeSize;

	uint16* start          = stream + offset / typeSize;

	if ( ( offset + size - 1 ) / typeSize > offset / typeSize ) {
		const uint32 startSize = ( offset / typeSize + 1 ) * typeSize - offset;
		const uint32 endSize   = size - startSize;

		SetBits( start,     GetBits( value, endSize, startSize ), 0,   startSize );
		SetBits( start + 1, GetBits( value, 0,       endSize ),   0,   endSize );
	}

	SetBits( start, value, subOffset, size);
}

void BitWrite32( uint32* stream, const uint32 offset, const uint32 size, const uint32 value ) {
	static constexpr uint32 typeSize = 32;

	const uint32 subOffset = offset % typeSize;

	uint32* start          = stream + offset / typeSize;

	if ( ( offset + size - 1 ) / typeSize > offset / typeSize ) {
		const uint32 startSize = ( offset / typeSize + 1 ) * typeSize - offset;
		const uint32 endSize   = size - startSize;

		SetBits( start,     GetBits( value, endSize, startSize ), startSize, 0 );
		SetBits( start + 1, GetBits( value, 0,       endSize ),   endSize,   0 );
	}

	SetBits( start, value, size, subOffset);
}

void BitWrite64( uint64* stream, const uint32 offset, const uint32 size, const uint64 value ) {
	static constexpr uint32 typeSize = 64;

	const uint32 subOffset = offset % typeSize;

	uint64* start          = stream + offset / typeSize;

	if ( ( offset + size - 1 ) / typeSize > offset / typeSize ) {
		const uint32 startSize = ( offset / typeSize + 1 ) * typeSize - offset;
		const uint32 endSize   = size - startSize;

		SetBits( start,     GetBits( value, endSize, startSize ), 0,   startSize );
		SetBits( start + 1, GetBits( value, 0,       endSize ),   0,   endSize );
	}

	SetBits( start, value, subOffset, size);
}

void BitWrite( void* stream, const uint32 offset, const uint32 size, const uint64 value ) {
	static constexpr uint32 typeSize = 64;

	const uint32 subOffset = offset % typeSize;

	uint64* start          = ( uint64* ) stream + offset / typeSize;

	if ( ( offset + size - 1 ) / typeSize > offset / typeSize ) {
		const uint32 startSize = ( offset / typeSize + 1 ) * typeSize - offset;
		const uint32 endSize   = size - startSize;

		SetBits( start,     GetBits( value, endSize, startSize ), 0,   startSize );
		SetBits( start + 1, GetBits( value, 0,       endSize ),   0,   endSize );
	}

	SetBits( start, value, subOffset, size);
}

BitStream::BitStream( const uint32 newElementSize ) :
	elementSize( newElementSize ) {
}

BitStream::BitStream( void* newMemory, const uint32 newElementSize ) :
	memory( ( uint64* ) newMemory ),
	elementSize( newElementSize ) {
}

uint8 BitStream::Read8( const uint32 size ) {
	const uint8 out = BitRead8( ( uint8* ) memory, offset, size );
	offset         += size;

	return out;
}

uint16 BitStream::Read16( const uint32 size ) {
	const uint16 out = BitRead16( ( uint16* ) memory, offset, size );
	offset          += size;

	return out;
}

uint32 BitStream::Read32( const uint32 size ) {
	const uint32 out = BitRead32( ( uint32* ) memory, offset, size );
	offset          += size;

	return out;
}

uint64 BitStream::Read64( const uint32 size ) {
	const uint64 out = BitRead64( memory, offset, size );
	offset          += size;

	return out;
}

void BitStream::Write( const uint8 value, const uint32 size ) {
	BitWrite64( memory, offset, size, value );
	offset += size;
}

void BitStream::Write( const uint16 value, const uint32 size ) {
	BitWrite64( memory, offset, size, value );
	offset += size;
}

void BitStream::Write( const uint32 value, const uint32 size ) {
	BitWrite64( memory, offset, size, value );
	offset += size;
}

void BitStream::Write( const uint64 value, const uint32 size ) {
	BitWrite64( memory, offset, size, value );
	offset += size;
}