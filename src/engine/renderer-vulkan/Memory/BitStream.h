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
// BitStream.h

#ifndef BIT_STREAM_H
#define BIT_STREAM_H

#include "../Math/NumberTypes.h"
#include "../Math/Bit.h"

uint8  BitRead8(  const uint8*  stream, const uint32 offset, const uint32 size );
uint16 BitRead16( const uint16* stream, const uint32 offset, const uint32 size );
uint32 BitRead32( const uint32* stream, const uint32 offset, const uint32 size );
uint64 BitRead64( const uint64* stream, const uint32 offset, const uint32 size );

void BitWrite8(  uint8*  stream, const uint32 offset, const uint32 size, const uint8  value );
void BitWrite16( uint16* stream, const uint32 offset, const uint32 size, const uint16 value );
void BitWrite32( uint32* stream, const uint32 offset, const uint32 size, const uint32 value );
void BitWrite64( uint64* stream, const uint32 offset, const uint32 size, const uint64 value );
void BitWrite(   void*   stream, const uint32 offset, const uint32 size, const uint64 value );

struct BitStream {
	uint64* memory;
	uint32  offset      = 0;
	uint32  elementSize = 0;

	BitStream( const uint32 newElementSize = 0 );

	BitStream( void* newMemory, const uint32 newElementSize = 0 );

	uint8 Read8(   const uint32 size );
	uint16 Read16( const uint32 size );
	uint32 Read32( const uint32 size );
	uint64 Read64( const uint32 size );

	void Write( const uint8 value,  const uint32 size );
	void Write( const uint16 value, const uint32 size );
	void Write( const uint32 value, const uint32 size );
	void Write( const uint64 value, const uint32 size );
};

#endif // BIT_STREAM_H