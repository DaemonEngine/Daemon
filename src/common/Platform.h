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

#ifndef COMMON_PLATFORM_H_
#define COMMON_PLATFORM_H_

// Platform-specific configuration
#if defined(_WIN32)
#define PLATFORM_STRING "Windows"
#elif defined(__APPLE__)
#define PLATFORM_STRING "macOS"
#elif defined(__linux__)
#define PLATFORM_STRING "Linux"
#elif defined(__FreeBSD__)
#define PLATFORM_STRING "FreeBSD"
#elif defined(__native_client__)
#define PLATFORM_STRING "NaCl"
#else
#error "Platform not supported"
#endif

#if defined(__native_client__)
#elif defined(_WIN32)
#define DLL_EXT ".dll"
#define EXE_EXT ".exe"
#define PATH_SEP '\\'
#else
#define DLL_EXT ".so"
#define EXE_EXT ""
#define PATH_SEP '/'
#endif

#if defined(DAEMON_ARCH_i686)
#undef __i386__
#define __i386__ 1
#elif defined(DAEMON_ARCH_amd64)
#undef __x86_64__
#define __x86_64__ 1
#endif

/* The definition name syntax is: DAEMON_USE_ARCH_INTRINSICS_<architecture>[_extension]

Examples:

- DAEMON_USE_ARCH_INTRINSICS_i686: i686 specific code, including asm code.
- DAEMON_USE_ARCH_INTRINSICS_i686_sse: i686 SSE specific code.
- DAEMON_USE_ARCH_INTRINSICS_i686_sse2: i686 SSE2 specific code.

If a architecture inherits a feature from an parent architecture, the parent
architecture name is used. For example on amd64, the definition enabling
SSE code is DAEMON_USE_ARCH_INTRINSICS_i686_sse, enabling SSE code on both
i686 with SSE and amd64.

The definitions for the architecture itself are automatically set by CMake. */

#if defined(DAEMON_USE_ARCH_INTRINSICS)
	// Set architecture extensions definitions.

	#if defined(_MSC_VER)
		/* Detect MSVC providing SSE and SSE2.

		MSVC doesn't set __SSE*__, and only sets _M_IX86_FP on i686.
		We should look for _M_AMD64 or _M_X64 to know if SSE and SSE2
		are enabled when building code for amd64. Beware that _M_AMD64
		and _M_X64 are also enabled when building for ARM64EC:

		> - _M_AMD64 Defined as the integer literal value 100 for compilations
		>   that target x64 processors or ARM64EC. Otherwise, undefined.
		> - _M_X64 Defined as the integer literal value 100 for compilations
		>   that target x64 processors or ARM64EC. Otherwise, undefined.
		> - _M_ARM64EC Defined as 1 for compilations that target ARM64EC.
		>   Otherwise, undefined.
		> -- https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170

		It is unclear if xmmintrin.h is available on ARM64EC. */

		#if defined(_M_IX86_FP)
			#if _M_IX86_FP >= 2
				#define DAEMON_USE_ARCH_INTRINSICS_i686_sse
				#define DAEMON_USE_ARCH_INTRINSICS_i686_sse2
			#elif _M_IX86_FP == 1
				#define DAEMON_USE_ARCH_INTRINSICS_i686_sse
			#endif
		#elif defined(_M_AMD64) || defined(_M_X64)
			#if !defined(_M_ARM64EC)
				#define DAEMON_USE_ARCH_INTRINSICS_i686_sse
				#define DAEMON_USE_ARCH_INTRINSICS_i686_sse2
			#endif
		#endif
	#else
		#if defined(__SSE__) || defined(MSVC_SSE)
			#define DAEMON_USE_ARCH_INTRINSICS_i686_sse
		#endif

		#if defined(__SSE2__) || defined(MSVC_SSE2)
			#define DAEMON_USE_ARCH_INTRINSICS_i686_sse2
		#endif
	#endif

	// Include intrinsics-specific headers.

	#if defined(DAEMON_USE_ARCH_INTRINSICS_i686_sse)
		#include <xmmintrin.h>
	#endif

	#if defined(DAEMON_USE_ARCH_INTRINSICS_i686_sse2)
		#include <emmintrin.h>
	#endif
#endif

// VM Prefixes
#if !defined(VM_NAME)
#define VM_STRING_PREFIX ""
#else
#define VM_STRING_PREFIX XSTRING(VM_NAME) "."
#endif

#endif // COMMON_PLATFORM_H_
