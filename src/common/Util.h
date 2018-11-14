/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
All rights reserved.

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

#ifndef COMMON_UTIL_H_
#define COMMON_UTIL_H_

#include <algorithm>
#include <memory>
#include <tuple>
#include <type_traits>

#include "Compiler.h"

// Various utilities

char     *QDECL PRINTF_LIKE(1) va( const char *format, ... );

namespace Util {

// Binary search function which returns an iterator to the result or end if not found
template<typename Iter, typename T>
Iter binary_find(Iter begin, Iter end, const T& value)
{
	Iter i = std::lower_bound(begin, end, value);
	if (i != end && !(value < *i))
		return i;
	else
		return end;
}
template<typename Iter, typename T, typename Compare>
Iter binary_find(Iter begin, Iter end, const T& value, Compare comp)
{
	Iter i = std::lower_bound(begin, end, value, comp);
	if (i != end && !comp(value, *i))
		return i;
	else
		return end;
}

/**
 * Enum to integral
 */
template<class E, class R = typename std::underlying_type<E>::type>
constexpr R ordinal(E e)
{
    static_assert(std::is_enum<E>::value, "Type should be an enum");
    return static_cast<R>(e);
}

/**
 * Integral to enum
 * Prefer ordinal, as that's guaranteed to be valid
 */
template<class E, class I = typename std::underlying_type<E>::type>
constexpr E enum_cast(I i) {
    static_assert(std::is_enum<E>::value, "Type should be an enum");
    return static_cast<E>(i);
}

/**
 * Enum to string
 */
template<class E>
const char* enum_str(E e) { return va("%d", ordinal<E, int>(e)); }

// Compile-time integer sequences
template<size_t...> struct seq {
	using type = seq;
};

template<class S1, class S2> struct concat;
template<size_t... I1, size_t... I2> struct concat<seq<I1...>, seq<I2...>>: seq<I1..., (sizeof...(I1) + I2)...> {};

template<size_t N> struct gen_seq;
template<size_t N> struct gen_seq: concat<typename gen_seq<N / 2>::type, typename gen_seq<N - N / 2>::type>::type {};
template<> struct gen_seq<0>: seq<>{};
template<> struct gen_seq<1>: seq<0>{};

// Simple type list template, useful for pattern matching in functions
template<typename... T> struct TypeList {};
template<typename T> struct TypeListFromTuple {};
template<typename... T> struct TypeListFromTuple<std::tuple<T...>>: TypeList<T...> {};

// Create a tuple of references from a tuple. The type of reference is the same as that with which the tuple is passed in.
template<typename Tuple, size_t... Seq> decltype(std::forward_as_tuple(std::get<Seq>(std::declval<Tuple>())...)) ref_tuple_impl(Tuple&& tuple, seq<Seq...>)
{
	return std::forward_as_tuple(std::get<Seq>(std::forward<Tuple>(tuple))...);
}
template<typename Tuple> decltype(ref_tuple_impl(std::declval<Tuple>(), gen_seq<std::tuple_size<typename std::decay<Tuple>::type>::value>())) ref_tuple(Tuple&& tuple)
{
	return ref_tuple_impl(std::forward<Tuple>(tuple), gen_seq<std::tuple_size<typename std::decay<Tuple>::type>::value>());
}

// Invoke a function using parameters from a tuple
template<typename Func, typename Tuple, size_t... Seq>
decltype(std::declval<Func>()(std::get<Seq>(std::declval<Tuple>())...)) apply_impl(Func&& func, Tuple&& tuple, seq<Seq...>)
{
	return std::forward<Func>(func)(std::get<Seq>(std::forward<Tuple>(tuple))...);
}
template<typename Func, typename Tuple>
decltype(apply_impl(std::declval<Func>(), std::declval<Tuple>(), gen_seq<std::tuple_size<typename std::decay<Tuple>::type>::value>())) apply(Func&& func, Tuple&& tuple)
{
	return apply_impl(std::forward<Func>(func), std::forward<Tuple>(tuple), gen_seq<std::tuple_size<typename std::decay<Tuple>::type>::value>());
}

// An equivalent of is_pod to workaround an MSVC bug where Vec3 is not POD
// while it is both trivial and standard layout.

template<typename T, typename = void>
struct IsPOD :std::false_type {};

template<typename T>
struct IsPOD<T, typename std::enable_if<std::is_trivial<T>::value && std::is_standard_layout<T>::value>::type> : std::true_type{};

// Utility class to hold a possibly uninitialized object.
template<typename T> class uninitialized {
public:
	uninitialized() {}

	uninitialized(const uninitialized&) = delete;
	uninitialized(uninitialized&&) = delete;
	uninitialized& operator=(const uninitialized&) = delete;
	uninitialized& operator=(uninitialized&&) = delete;

	template<typename... Args> void construct(Args&&... args)
	{
		new(&data) T(std::forward<Args>(args)...);
	}
	template<typename Arg> decltype(std::declval<T&>() = std::declval<Arg>()) assign(Arg&& arg)
	{
		return get() = std::forward<Arg>(arg);
	}
	void destroy()
	{
		get().~T();
	}
	T& get()
	{
		return *reinterpret_cast<T*>(&data);
	}
	const T& get() const
	{
		return *reinterpret_cast<const T*>(&data);
	}

private:
	typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type data;
};

// Use to perform an action only if a minimum time has passed since the last time it was performed.
// Not thread-safe.
class MinimumDelay {
public:
	// duration: length of delay in milliseconds
	MinimumDelay(int duration): duration(duration), lastTime(std::numeric_limits<int>::min()) {}

	// Returns true if it is OK to perform the action again. Takes current time in milliseconds.
	bool Check(int now) {
		if (now >= lastTime && now < lastTime + duration) {
			return false;
		} else {
			lastTime = now;
			return true;
		}
	}
private:
	const int duration;
	int lastTime;
};

// std::make_unique is not available until C++14.
template<typename T, typename ... U>
std::unique_ptr<T> make_unique(U&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<U>(args)...));
}

} // namespace Util

#endif // COMMON_UTIL_H_
