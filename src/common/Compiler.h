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

// Abstract away compiler-specific stuff

#ifndef COMMON_COMPILER_H_
#define COMMON_COMPILER_H_

// Code making use of compiler intrinsics.

/* CountTrailingZeroes returns the number of
trailing zeroes of the argument in binary.
The result is unspecified if the input is 0. */
#if defined(DAEMON_USE_COMPILER_INTRINSICS) && defined(__GNUC__)
inline int CountTrailingZeroes(unsigned int x)
{
	return __builtin_ctz(x);
}
inline int CountTrailingZeroes(unsigned long x)
{
	return __builtin_ctzl(x);
}
inline int CountTrailingZeroes(unsigned long long x)
{
	return __builtin_ctzll(x);
}
#elif defined(DAEMON_USE_COMPILER_INTRINSICS) && defined(_MSC_VER)
inline int CountTrailingZeroes(unsigned int x)
{
	unsigned long ans; _BitScanForward(&ans, x); return ans;
}
inline int CountTrailingZeroes(unsigned long x)
{
	unsigned long ans; _BitScanForward(&ans, x); return ans;
}
inline int CountTrailingZeroes(unsigned long long x)
{
	unsigned long ans;
	#ifdef _WIN64
	_BitScanForward64(&ans, x); return ans;
	#else
	bool nonzero = _BitScanForward(&ans, static_cast<unsigned long>(x));
	if (!nonzero) { _BitScanForward(&ans, x >> 32); }
	#endif
	return ans;
}
#else
inline int CountTrailingZeroes(unsigned int x)
{
	int i = 0; while (i < 32 && !(x & 1)) { ++i; x >>= 1; } return i;
}
inline int CountTrailingZeroes(unsigned long x)
{
	int i = 0; while (i < 64 && !(x & 1)) { ++i; x >>= 1; } return i;
}
inline int CountTrailingZeroes(unsigned long long x)
{
	int i = 0; while (i < 64 && !(x & 1)) { ++i; x >>= 1; } return i;
}
#endif

// Sanitizer detection

#if defined(__SANITIZE_ADDRESS__) // Detects GCC and MSVC AddressSanitizer
	#define USING_ADDRESS_SANITIZER
	#define USING_SANITIZER
#elif defined(__SANITIZE_THREAD__) // Detects GCC ThreadSanitizer
	#define USING_SANITIZER
#elif defined(__has_feature)
	#if __has_feature(address_sanitizer) // Detects Clang AddressSanitizer
		#define USING_ADDRESS_SANITIZER
		#define USING_SANITIZER
	#elif __has_feature(leak_sanitizer) // Detects Clang LeakSanitizer
		#define USING_SANITIZER
	#elif __has_feature(memory_sanitizer) // Detects Clang MemorySanitizer
		#define USING_SANITIZER
	#elif __has_feature(thread_sanitizer) // Detects Clang ThreadSanitizer
		#define USING_SANITIZER
	#endif
#endif

// GCC and Clang attribute and operator customization.
#if defined(DAEMON_USE_COMPILER_CUSTOMIZATION) && defined(__GNUC__)

// Emit a nice warning when a function is used
#define DEPRECATED __attribute__((__deprecated__))

// Warn when a function's return argument must be used, but isn't
// Caution: to keep MSVC compat, this must be put before the function name
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))

// A cold function is rarely called, so branches that lead to one are assumed
// to be unlikely
#define COLD __attribute__((__cold__))

// Indicates that a function does not return
#define NORETURN __attribute__((__noreturn__))
#define NORETURN_PTR __attribute__((__noreturn__))

// Expect printf-style arguments for a function: a is the index of the format
// string, and b is the index of the first variable argument
#define PRINTF_LIKE(n) __attribute__((__format__(__printf__, n, (n) + 1)))
#define VPRINTF_LIKE(n) __attribute__((__format__(__printf__, n, 0)))
#define PRINTF_TRANSLATE_ARG(a) __attribute__((__format_arg__(a)))

// Marks that this function will return a pointer that is not aliased to any
// other pointer
#define MALLOC_LIKE __attribute__((__malloc__))

// Shared library function import/export
#ifdef _WIN32
#define DLLEXPORT __attribute__((__dllexport__))
#define DLLIMPORT __attribute__((__dllimport__))
#else
#define DLLEXPORT __attribute__((__visibility__("default")))
#define DLLIMPORT __attribute__((__visibility__("default")))
#endif

// Raise an exception and break in the debugger
#if defined(DAEMON_ARCH_i686) || defined(DAEMON_ARCH_amd64)
	// Always run this asm code even if DAEMON_USE_ARCH_INTRINSICS is not defined.
	#define BREAKPOINT() __asm__ __volatile__("int $3\n\t")
#endif

/* Compiler can be fooled when calling ASSERT_UNREACHABLE() macro at end of non-void function.
In this case, compiler is complaining because control reaches end of non-void function,
even if the execution flow is expected to be taken down by assert before.

That's why we use these compiler specific unreachable builtin on modern compilers,
ASSERT_UNREACHABLE() macro makes use of this UNREACHABLE() macro, preventing useless warnings.
Unsupported compilers will raise "control reaches end of non-void function" warnings but
that's not a big issue and that's likely to never happen (these compilers would be too old and
would lack too much features to compile Daemon anyway).

See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0627r0.pdf */
#define UNREACHABLE() __builtin_unreachable()

// To mark functions which cause issues with address sanitizer
#ifdef USING_ADDRESS_SANITIZER
	#define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#endif

// The new -Wimplicit-fallthrough warning...
#if defined(__clang__) && __clang_major__ >= 6
	#define DAEMON_FALLTHROUGH [[clang::fallthrough]]
#elif __GNUC__ >= 7
	#define DAEMON_FALLTHROUGH [[gnu::fallthrough]]
#endif

// Microsoft Visual C++ attribute and operator customization.
#elif defined(DAEMON_USE_COMPILER_CUSTOMIZATION) && defined(_MSC_VER)

// See descriptions above
#define DEPRECATED __declspec(deprecated)
#define WARN_UNUSED_RESULT _Check_return_
#define NORETURN __declspec(noreturn)

// Marks this function as memory allocator
#if _MSC_VER >= 1900 && !defined( _CORECRT_BUILD )
	#define ALLOCATOR __declspec(allocator)
#endif

#define MALLOC_LIKE ALLOCATOR __declspec(restrict)
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)
#define BREAKPOINT() __debugbreak()
#define UNREACHABLE() __assume(0)

// Other compilers, unsupported
#else
	#warning "Unsupported compiler"
#endif

// Work around lack of compiler customization.
#if !defined(DEPRECATED)
	#define DEPRECATED
#endif
#if !defined(WARN_UNUSED_RESULT)
	#define WARN_UNUSED_RESULT
#endif
#if !defined(COLD)
	#define COLD
#endif
#if !defined(NORETURN)
	#define NORETURN
#endif
#if !defined(NORETURN_PTR)
	#define NORETURN_PTR
#endif
#if !defined(PRINTF_LIKE)
	#define PRINTF_LIKE(n)
#endif
#if !defined(VPRINTF_LIKE)
	#define VPRINTF_LIKE(n)
#endif
#if !defined(PRINTF_TRANSLATE_ARG)
	#define PRINTF_TRANSLATE_ARG(a)
#endif
#if !defined(MALLOC_LIKE)
	#define MALLOC_LIKE
#endif
#if !defined(ALLOCATOR)
	#define ALLOCATOR
#endif
#if !defined(DLLEXPORT)
	#define DLLEXPORT
#endif
#if !defined(DLLIMPORT)
	#define DLLIMPORT
#endif
#if !defined(BREAKPOINT)
	#define BREAKPOINT()
#endif
#if !defined(UNREACHABLE)
	#define UNREACHABLE()
#endif
#if !defined(ATTRIBUTE_NO_SANITIZE_ADDRESS)
	#define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif
#if !defined(DAEMON_FALLTHROUGH)
	#define DAEMON_FALLTHROUGH
#endif

// Keywords specific to C++ versions

/* TODO: Rewrite all NOEXCEPT usages from the whole code base.

The noexcept keyword should be used on all move constructors and move
assignments so that containers move objects instead of copying them.
That keyword was added in C++11, all compilers should now support it. */
#define NOEXCEPT noexcept
#define NOEXCEPT_IF(x) noexcept(x)
#define NOEXCEPT_EXPR(x) noexcept(x)

// Uses SD-6 Feature Test Recommendations
#if defined(__cpp_constexpr)
	#define CONSTEXPR constexpr
	#define CONSTEXPR_FUNCTION constexpr
	#if __cpp_constexpr >= 201304
		#define CONSTEXPR_FUNCTION_RELAXED constexpr
	#endif
#endif

// Work around lack of language keywords.
#if !defined(CONSTEXPR)
	#define CONSTEXPR
#endif
#if !defined(CONSTEXPR_FUNCTION)
	#define CONSTEXPR_FUNCTION
#endif
#if !defined(CONSTEXPR_FUNCTION_RELAXED)
	#define CONSTEXPR_FUNCTION_RELAXED
#endif

// Compiler specificities we can't disable.

#if defined(__MINGW32__) && defined(__i386__)
// On x86, GCC expects 16-byte stack alignment (used for SSE instructions), but MSVC only uses 4-byte alignment.
// Therefore the stack needs to be adjusted whenever MSVC code calls into GCC code.
	#define ALIGN_STACK_FOR_MINGW __attribute__((force_align_arg_pointer))
#else
	#define ALIGN_STACK_FOR_MINGW
#endif

/* Use a C++11 braced initializer on MSVC instead of a bracket initializer
when zeroing a struct. This works around a bug in how MSVC generates implicit
default constructors. -- Amanieu

The MSVC workaround is known to crash ICC, there is no reason to apply MSVC
workarounds on non-MSVC compilers. -- illwieckz

I believe GCC, Clang, and MSVC have all been seen to crash during compilation
on either the () or {} forms of these constructors. -- slipher

The actual occasion for using this function is when you want to clear a struct
with a very large size. Supposedly with some compiler this could cause a
temporary copy to be placed on the stack and cause a stack overflow if you did
it the normal way like foo = {}. -- slipher */
template<typename T>
void ResetStruct( T& object ) {
	object.~T();
#if defined(_MSC_VER)
	new( &object ) T{};
#else
	new( &object ) T();
#endif
}

#endif // COMMON_COMPILER_H_
