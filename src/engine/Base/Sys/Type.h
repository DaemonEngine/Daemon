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

#ifndef TYPE_H
#define TYPE_H

template<typename T>
struct TriviallyDestructible {
	#if defined( __clang__ ) || defined( _MSC_VER )
		static constexpr bool value = __is_trivially_destructible( T );
	#else // GCC
		static constexpr bool value = __has_trivial_destructor( T );
	#endif
};

template<typename T>
struct TriviallyDestructible<T&> {
	static constexpr bool value = false;
};

template<typename T>
struct TriviallyDestructible<T&&> {
	static constexpr bool value = false;
};

template<typename T>
struct TriviallyDestructible<T[]> {
	static constexpr bool value = false;
};

template<>
struct TriviallyDestructible<void> {
	static constexpr bool value = false;
};

template<>
struct TriviallyDestructible<const void> {
	static constexpr bool value = false;
};

template<>
struct TriviallyDestructible<volatile void> {
	static constexpr bool value = false;
};

template<>
struct TriviallyDestructible<volatile const void> {
	static constexpr bool value = false;
};

template<typename T>
constexpr bool triviallyDestructible = TriviallyDestructible<T>::value;


template<typename T, typename U>
struct IsSame {
	static constexpr bool value = false;
};

template<typename T>
struct IsSame<T, T> {
	static constexpr bool value = true;
};

template<typename T, typename U>
constexpr bool isSame = IsSame<T, U>::value;


template<bool condition, typename T, typename U>
struct SwitchType {
	using value = T;
};

template<typename T, typename U>
struct SwitchType<true, T, U> {
	using value = U;
};

template<bool condition, typename T, typename U>
using switchType = SwitchType<condition, T, U>::value;

#endif // TYPE_H