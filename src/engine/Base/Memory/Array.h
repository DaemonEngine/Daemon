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

#ifndef ARRAY_H
#define ARRAY_H

#include <initializer_list>

#include "Sys/Type.h"
#include "Int.h"
#include "IteratorSeq.h"

template<typename T, uint32 size>
struct Array {
    T            memory[size];

	constexpr uint32 Size() const {
		return size;
	}

	constexpr T& operator[]( const uint32 index ) {
		return memory[index];
	}

	constexpr T& operator[]( const uint32 index ) const {
		return memory[index];
	}

	constexpr IteratorSeq<T> begin() {
		return IteratorSeq<T>{ &memory[0] };
	}

	constexpr IteratorSeq<T> end() {
		return IteratorSeq<T>{ &memory[size] };
	}

	constexpr IteratorSeq<const T> begin() const {
		return IteratorSeq<const T> { &memory[0] };
	}

	constexpr IteratorSeq<const T> end() const {
		return IteratorSeq<const T> { &memory[size] };
	}
};

template<typename T, typename... Args>
Array( T, Args... args ) -> Array<
	switchType<isSame<T, const char*>, T, const char*>,
	sizeof...( args ) + 1
>;

template<uint32 size, typename... Args>
Array( const char(&)[size], Args... args ) -> Array<const char*, sizeof...( args ) + 1>;

#endif // ARRAY_H