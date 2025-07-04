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
// Bit.h

#ifndef BIT_H
#define BIT_H

#include "common/Common.h"

#if defined(DAEMON_USE_COMPILER_INTRINSICS) && defined(__GNUC__)

uint32_t FindLSB( const uint32_t value ) {
	return value ? __builtin_ctzl( value ) : 32;
}

uint32_t FindLSB( const uint64_t value ) {
	return value ? __builtin_ctzll( value ) : 64;
}

uint32_t FindMSB( const uint32_t value ) {
	return value ? __builtin_clzl( value ) : 32;
}

uint32_t FindMSB( const uint64_t value ) {
	return value ? __builtin_clzll( value ) : 64;
}

uint16_t CountLeadingZeroes( const uint16_t value ) {
	return FindLSB( value );
}

uint32_t CountLeadingZeroes( const uint32_t value ) {
	return FindLSB( value );
}

uint64_t CountLeadingZeroes( const uint64_t value ) {
	return FindLSB( value );
}

#elif defined(DAEMON_USE_COMPILER_INTRINSICS) && defined(_MSC_VER)

	uint32_t FindLSB( const uint32_t value ) {
		unsigned long index;
		const bool nonZero = _BitScanForward( &index, value );
		return nonZero ? index : 32;
	}

	uint32_t FindLSB( const uint64_t value ) {
		unsigned long index;
		const bool nonZero = _BitScanForward64( &index, value );
		return nonZero ? index : 64;
	}

	uint32_t FindMSB( const uint32_t value ) {
		unsigned long index;
		const bool nonZero = _BitScanReverse( &index, value );
		return nonZero ? index : 32;
	}

	uint32_t FindMSB( const uint64_t value ) {
		unsigned long index;
		const bool nonZero = _BitScanReverse64( &index, value );
		return nonZero ? index : 64;
	}

	uint16_t CountLeadingZeroes( const uint16_t value ) {
		return __lzcnt16( value );
	}

	uint32_t CountLeadingZeroes( const uint32_t value ) {
		return __lzcnt( value );
	}

	uint64_t CountLeadingZeroes( const uint64_t value ) {
		return __lzcnt64( value );
	}

#else

	uint32_t FindLSB( const uint32_t value ) {
		for ( int i = 0; i < 32; i++ ) {
			if ( value & ( 1u << i ) ) {
				return i;
			}
		}

		return 32;
	}

	uint32_t FindLSB( const uint64_t value ) {
		for ( int i = 0; i < 64; i++ ) {
			if ( value & ( 1ull << i ) ) {
				return i;
			}
		}

		return 64;
	}

	uint32_t FindMSB( const uint32_t value ) {
		for ( int i = 31; i >= 0; i-- ) {
			if ( value & ( 1u << i ) ) {
				return i;
			}
		}

		return 32;
	}

	uint32_t FindMSB( const uint64_t value ) {
		for ( int i = 63; i >= 0; i-- ) {
			if ( value & ( 1ull << i ) ) {
				return i;
			}
		}

		return 63;
	}

	uint16_t CountLeadingZeroes( const uint16_t value ) {
		return value ? FindLSB( value ) - 1 : 0;
	}

	uint32_t CountLeadingZeroes( const uint32_t value ) {
		return value ? FindLSB( value ) - 1 : 0;
	}

	uint64_t CountLeadingZeroes( const uint64_t value ) {
		return value ? FindLSB( value ) - 1 : 0;
	}

#endif

[[nodiscard]] uint8_t SetBit( const uint8_t value, const uint32_t bit ) {
	return value | ( 1u << bit );
}

[[nodiscard]] uint16_t SetBit( const uint16_t value, const uint32_t bit ) {
	return value | ( 1u << bit );
}

[[nodiscard]] uint32_t SetBit( const uint32_t value, const uint32_t bit ) {
	return value | ( 1u << bit );
}

[[nodiscard]] uint64_t SetBit( const uint64_t value, const uint32_t bit ) {
	return value | ( 1ull << bit );
}

void SetBit( uint8_t* value, const uint32_t bit ) {
	*value |= ( 1u << bit );
}

void SetBit( uint16_t* value, const uint32_t bit ) {
	*value |= ( 1u << bit );
}

void SetBit( uint32_t* value, const uint32_t bit ) {
	*value |= ( 1u << bit );
}

void SetBit( uint64_t* value, const uint32_t bit ) {
	*value |= ( 1ull << bit );
}

[[nodiscard]] uint8_t UnSetBit( const uint8_t value, const uint32_t bit ) {
	return value & ~( 1u << bit );
}

[[nodiscard]] uint16_t UnSetBit( const uint16_t value, const uint32_t bit ) {
	return value & ~( 1u << bit );
}

[[nodiscard]] uint32_t UnSetBit( const uint32_t value, const uint32_t bit ) {
	return value & ~( 1u << bit );
}

[[nodiscard]] uint64_t UnSetBit( const uint64_t value, const uint32_t bit ) {
	return value & ~( 1ull << bit );
}

void UnSetBit( uint8_t* value, const uint32_t bit ) {
	*value &= ~( 1u << bit );
}

void UnSetBit( uint16_t* value, const uint32_t bit ) {
	*value &= ~( 1u << bit );
}

void UnSetBit( uint32_t* value, const uint32_t bit ) {
	*value &= ~( 1u << bit );
}

void UnSetBit( uint64_t* value, const uint32_t bit ) {
	*value &= ~( 1ull << bit );
}

int CompareBit( const uint8_t lhs, const uint8_t rhs, const uint32_t bit ) {
	const uint8_t lhsBit = lhs & ( 1u << bit );
	const uint8_t rhsBit = rhs & ( 1u << bit );
	return lhsBit < rhsBit ? -1 : ( lhsBit > rhsBit ? 1 : 0 );
}

int CompareBit( const uint16_t lhs, const uint16_t rhs, const uint32_t bit ) {
	const uint16_t lhsBit = lhs & ( 1u << bit );
	const uint16_t rhsBit = rhs & ( 1u << bit );
	return lhsBit < rhsBit ? -1 : ( lhsBit > rhsBit ? 1 : 0 );
}

int CompareBit( const uint32_t lhs, const uint32_t rhs, const uint32_t bit ) {
	const uint32_t lhsBit = lhs & ( 1u << bit );
	const uint32_t rhsBit = rhs & ( 1u << bit );
	return lhsBit < rhsBit ? -1 : ( lhsBit > rhsBit ? 1 : 0 );
}

int CompareBit( const uint64_t lhs, const uint64_t rhs, const uint32_t bit ) {
	const uint64_t lhsBit = lhs & ( 1ull << bit );
	const uint64_t rhsBit = rhs & ( 1ull << bit );
	return lhsBit < rhsBit ? -1 : ( lhsBit > rhsBit ? 1 : 0 );
}

const bool BitSet( const uint8_t value, const uint32_t bit ) {
	return value & ( 1u << bit );
}

const bool BitSet( const uint16_t value, const uint32_t bit ) {
	return value & ( 1u << bit );
}

const bool BitSet( const uint32_t value, const uint32_t bit ) {
	return value & ( 1u << bit );
}

const bool BitSet( const uint64_t value, const uint32_t bit ) {
	return value & ( 1ull << bit );
}

uint32_t FindMZeroBit( uint64_t value ) {
	if ( value == UINT64_MAX ) {
		return 64;
	}

	uint32_t bit;
	while ( !( bit = FindMSB( value ) ) ) {
		UnSetBit( &value, bit );
	}

	return bit - 1;
}

uint32_t FindZeroBitFast( const uint64_t value ) {
	const uint32_t bit = FindLSB( value );
	return bit ? bit - 1 : FindLSB( ~value ) - 1;
}

#endif // BIT_H