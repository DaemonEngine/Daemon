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

#include <intrin.h>

#include "NumberTypes.h"

#if defined(DAEMON_USE_COMPILER_INTRINSICS) && defined(__GNUC__)

	inline uint32 FindLSB( const uint32 value ) {
		return value ? __builtin_ctzl( value ) : 32;
	}

	inline uint32 FindLSB( const uint64 value ) {
		return value ? __builtin_ctzll( value ) : 64;
	}

	inline uint32 FindMSB( const uint32 value ) {
		return value ? __builtin_clzl( value ) : 32;
	}

	inline uint32 FindMSB( const uint64 value ) {
		return value ? __builtin_clzll( value ) : 64;
	}

	inline uint16 CountLeadingZeroes( const uint16 value ) {
		return FindLSB( value );
	}

	inline uint32 CountLeadingZeroes( const uint32 value ) {
		return FindLSB( value );
	}

	inline uint64 CountLeadingZeroes( const uint64 value ) {
		return FindLSB( value );
	}

	inline uint32 CountBits( const uint16 value ) {
		return __builtin_popcount( value );
	}

	inline uint32 CountBits( const uint32 value ) {
		return __builtin_popcountl( value );
	}

	inline uint32 CountBits( const uint64 value ) {
		return __builtin_popcountll( value );
	}

#elif defined(DAEMON_USE_COMPILER_INTRINSICS) && defined(_MSC_VER)

	inline uint32 FindLSB( const uint8 value ) {
		unsigned long index;
		const bool nonZero = _BitScanForward( &index, value );
		return nonZero ? index : 8;
	}

	inline uint32 FindLSB( const uint16 value ) {
		unsigned long index;
		const bool nonZero = _BitScanForward( &index, value );
		return nonZero ? index : 16;
	}

	inline uint32 FindLSB( const uint32 value ) {
		unsigned long index;
		const bool nonZero = _BitScanForward( &index, value );
		return nonZero ? index : 32;
	}

	inline uint32 FindLSB( const uint64 value ) {
		unsigned long index;
		const bool nonZero = _BitScanForward64( &index, value );
		return nonZero ? index : 64;
	}

	inline uint32 FindMSB( const uint8 value ) {
		unsigned long index;
		const bool nonZero = _BitScanReverse( &index, value );
		return nonZero ? index : 8;
	}

	inline uint32 FindMSB( const uint16 value ) {
		unsigned long index;
		const bool nonZero = _BitScanReverse( &index, value );
		return nonZero ? index : 16;
	}

	inline uint32 FindMSB( const uint32 value ) {
		unsigned long index;
		const bool nonZero = _BitScanReverse( &index, value );
		return nonZero ? index : 32;
	}

	inline uint32 FindMSB( const uint64 value ) {
		unsigned long index;
		const bool nonZero = _BitScanReverse64( &index, value );
		return nonZero ? index : 64;
	}

	inline uint16 CountLeadingZeroes( const uint16 value ) {
		return __lzcnt16( value );
	}

	inline uint32 CountLeadingZeroes( const uint32 value ) {
		return __lzcnt( value );
	}

	inline uint64 CountLeadingZeroes( const uint64 value ) {
		return __lzcnt64( value );
	}

	inline uint16 CountBits( const uint16 value ) {
		return __popcnt16( value );
	}

	inline uint32 CountBits( const uint32 value ) {
		return __popcnt( value );
	}

	inline uint64 CountBits( const uint64 value ) {
		return __popcnt64( value );
	}

#endif

// Set a single bit to 1

[[nodiscard]] inline uint8 SetBit(  const uint8 value, const uint32 bit ) {
	return value | ( 1u << bit );
}

[[nodiscard]] inline uint16 SetBit( const uint16 value, const uint32 bit ) {
	return value | ( 1u << bit );
}

[[nodiscard]] inline uint32 SetBit( const uint32 value, const uint32 bit ) {
	return value | ( 1u << bit );
}

[[nodiscard]] inline uint64 SetBit( const uint64 value, const uint32 bit ) {
	return value | ( 1ull << bit );
}

inline void SetBit( uint8* value,  const uint32 bit ) {
	*value |= ( 1u << bit );
}

inline void SetBit( uint16* value, const uint32 bit ) {
	*value |= ( 1u << bit );
}

inline void SetBit( uint32* value, const uint32 bit ) {
	*value |= ( 1u << bit );
}

inline void SetBit( uint64* value, const uint32 bit ) {
	*value |= ( 1ull << bit );
}

// Return a uint with only bits in range [start, start + count) set to 1

[[nodiscard]] inline uint8 BitMask8(   const uint32 start, const uint32 count ) {
	return ( UINT8_MAX  >> ( 8 - count ) ) << start;
}

[[nodiscard]] inline uint16 BitMask16( const uint32 start, const uint32 count ) {
	return ( UINT16_MAX >> ( 16 - count ) ) << start;
}

[[nodiscard]] inline uint32 BitMask32( const uint32 start, const uint32 count ) {
	return ( UINT32_MAX >> ( 32 - count ) ) << start;
}

[[nodiscard]] inline uint64 BitMask64( const uint32 start, const uint32 count ) {
	return ( UINT64_MAX >> ( 64 - count ) ) << start;
}

// Set bits in range [start, start + count) to 1

[[nodiscard]] inline uint8 SetBits( const uint8 value,  const uint32 start, const uint32 count ) {
	return value | BitMask8(  start, count );
}

[[nodiscard]] inline uint16 SetBits( const uint16 value, const uint32 start, const uint32 count ) {
	return value | BitMask16( start, count );
}

[[nodiscard]] inline uint32 SetBits( const uint32 value, const uint32 start, const uint32 count ) {
	return value | BitMask32( start, count );
}

[[nodiscard]] inline uint64 SetBits( const uint64 value, const uint32 start, const uint32 count ) {
	return value | BitMask64( start, count );
}

inline void SetBits( uint8* value, const uint32 start,  const uint32 count ) {
	*value |= BitMask8(  start,  count );
}

inline void SetBits( uint16* value, const uint32 start, const uint32 count ) {
	*value |= BitMask16( start, count );
}

inline void SetBits( uint32* value, const uint32 start, const uint32 count ) {
	*value |= BitMask32( start, count );
}

inline void SetBits( uint64* value, const uint32 start, const uint32 count ) {
	*value |= BitMask64( start, count );
}

// Set bits in range [start, start + count) to a given value

[[nodiscard]] inline uint8 SetBits(  const uint8 value,  const uint8 bits, const uint32 start, const uint32 count ) {
	return ( value & ~BitMask8(  start, count ) )
		| ( ( bits & BitMask8( 0, count ) ) << start );
}

[[nodiscard]] inline uint16 SetBits( const uint16 value, const uint16 bits, const uint32 start, const uint32 count ) {
	return ( value & ~BitMask16( start, count ) )
	     | ( ( bits & BitMask16( 0, count ) ) << start );
}

[[nodiscard]] inline uint32 SetBits( const uint32 value, const uint32 bits, const uint32 start, const uint32 count ) {
	return ( value & ~BitMask32( start, count ) )
	     | ( ( bits & BitMask32( 0, count ) ) << start );
}

[[nodiscard]] inline uint64 SetBits( const uint64 value, const uint64 bits, const uint32 start, const uint32 count ) {
	return ( value & ~BitMask64( start, count ) )
	     | ( ( bits & BitMask64( 0, count ) ) << start );
}

inline void SetBits( uint8* value,  const uint8 bits,  const uint32 start, const uint32 count ) {
	*value = SetBits( *value, bits, start, count );
}

inline void SetBits( uint16* value, const uint16 bits, const uint32 start, const uint32 count ) {
	*value = SetBits( *value, bits, start, count );
}

inline void SetBits( uint32* value, const uint32 bits, const uint32 start, const uint32 count ) {
	*value = SetBits( *value, bits, start, count );
}

inline void SetBits( uint64* value, const uint64 bits, const uint32 start, const uint32 count ) {
	*value = SetBits( *value, bits, start, count );
}

// Set a single bit to 0

[[nodiscard]] inline uint8 UnSetBit( const uint8 value,   const uint32 bit ) {
	return value & ~( 1u << bit );
}

[[nodiscard]] inline uint16 UnSetBit( const uint16 value, const uint32 bit ) {
	return value & ~( 1u << bit );
}

[[nodiscard]] inline uint32 UnSetBit( const uint32 value, const uint32 bit ) {
	return value & ~( 1u << bit );
}

[[nodiscard]] inline uint64 UnSetBit( const uint64 value, const uint32 bit ) {
	return value & ~( 1ull << bit );
}

inline void UnSetBit( uint8* value,  const uint32 bit ) {
	*value &= ~( 1u << bit );
}

inline void UnSetBit( uint16* value, const uint32 bit ) {
	*value &= ~( 1u << bit );
}

inline void UnSetBit( uint32* value, const uint32 bit ) {
	*value &= ~( 1u << bit );
}

inline void UnSetBit( uint64* value, const uint32 bit ) {
	*value &= ~( 1ull << bit );
}

// Return bits in range [start, start + count) in the lowest bits

[[nodiscard]] inline uint8 GetBits( const uint8 value,   const uint32 start, const uint32 count ) {
	return ( value >> start ) & BitMask8( 0, count );
}

[[nodiscard]] inline uint16 GetBits( const uint16 value, const uint32 start, const uint32 count ) {
	return ( value >> start ) & BitMask16( 0, count );
}

[[nodiscard]] inline uint32 GetBits( const uint32 value, const uint32 start, const uint32 count ) {
	return ( value >> start ) & BitMask32( 0, count );
}

[[nodiscard]] inline uint64 GetBits( const uint64 value, const uint32 start, const uint32 count ) {
	return ( value >> start ) & BitMask64( 0, count );
}

inline int CompareBit( const uint8 lhs,  const uint8 rhs, const uint32 bit ) {
	const uint8 lhsBit = lhs & ( 1u << bit );
	const uint8 rhsBit = rhs & ( 1u << bit );
	return lhsBit < rhsBit ? -1 : ( lhsBit > rhsBit ? 1 : 0 );
}

inline int CompareBit( const uint16 lhs, const uint16 rhs, const uint32 bit ) {
	const uint16 lhsBit = lhs & ( 1u << bit );
	const uint16 rhsBit = rhs & ( 1u << bit );
	return lhsBit < rhsBit ? -1 : ( lhsBit > rhsBit ? 1 : 0 );
}

inline int CompareBit( const uint32 lhs, const uint32 rhs, const uint32 bit ) {
	const uint32 lhsBit = lhs & ( 1u << bit );
	const uint32 rhsBit = rhs & ( 1u << bit );
	return lhsBit < rhsBit ? -1 : ( lhsBit > rhsBit ? 1 : 0 );
}

inline int CompareBit( const uint64 lhs, const uint64 rhs, const uint32 bit ) {
	const uint64 lhsBit = lhs & ( 1ull << bit );
	const uint64 rhsBit = rhs & ( 1ull << bit );
	return lhsBit < rhsBit ? -1 : ( lhsBit > rhsBit ? 1 : 0 );
}

inline const bool BitSet( const uint8 value, const uint32 bit ) {
	return value & ( 1u << bit );
}

inline const bool BitSet( const uint16 value, const uint32 bit ) {
	return value & ( 1u << bit );
}

inline const bool BitSet( const uint32 value, const uint32 bit ) {
	return value & ( 1u << bit );
}

inline const bool BitSet( const uint64 value, const uint32 bit ) {
	return value & ( 1ull << bit );
}

inline uint32 FindLZeroBit( uint8 value ) {
	return FindLSB( ( uint8 ) ~value );
}

inline uint32 FindLZeroBit( uint16 value ) {
	return FindLSB( ( uint16 ) ~value );
}

inline uint32 FindLZeroBit( uint32 value ) {
	return FindLSB( ~value );
}

inline uint32 FindLZeroBit( uint64 value ) {
	return FindLSB( ~value );
}

inline uint32 FindMZeroBit( uint64 value ) {
	return FindMSB( ~value );
}

inline uint32 FindZeroBitFast( const uint64 value ) {
	if ( value == UINT64_MAX ) {
		return 64;
	}

	const uint32 bit = FindLSB( value );
	return bit ? bit - 1 : FindLSB( ~value );
}

#endif // BIT_H