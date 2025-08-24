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
// CPPStandard.h

#ifndef CPPSTANDARD_H
#define CPPSTANDARD_H

#undef CPP_AGGREGATE
#undef CPP_CONCEPTS
#undef CPP_CONSTEVAL
#undef CPP_CONSTEXPR
#undef CPP_CTAD
#undef CPP_DESIGNATED_INIT
#undef CPP_INVOKE_RESULT
#undef CPP_ATOMIC_WAIT_NOTIFY
#undef CPP_SOURCE_LOCATION
#undef CPP_STACKTRACE

#if defined( __cpp_aggregate_bases ) && defined ( __cpp_aggregate_nsdmi ) && defined ( __cpp_aggregate_paren_init )
    #define CPP_AGGREGATE
#endif

#if __cpp_concepts >= 201907L
    #define CPP_CONCEPTS
#endif

#if __cpp_consteval >= 201811L
    #define CPP_CONSTEVAL
#endif

#if __cpp_constexpr >= 202110L && __cpp_if_constexpr >= 201606L
    #define CPP_CONSTEXPR
#endif

#if __cpp_deduction_guides >= 201907L
    #define CPP_CTAD
#endif

#if __cpp_designated_initializers >= 201707L
    #define CPP_DESIGNATED_INIT
#endif

#if __cpp_lib_is_invocable >= 201703L
    #define CPP_INVOKE_RESULT
#endif

#if __cpp_lib_atomic_wait >= 201907L
    #define CPP_ATOMIC_WAIT_NOTIFY
#endif

#if __cpp_lib_source_location >= 201907L
    #define CPP_SOURCE_LOCATION
#endif

/* Clang/GCC support <stacktrace> with -lstdc++exp/-lstdlibc++_libbacktrace, but they don't define the feature macro,
so we set a custom macro in build system */
#if __cpp_lib_stacktrace >= 202011L || defined(DAEMON_CPP23_SUPPORT_LIBRARY_ENABLED)
    #define CPP_STACKTRACE
#endif

#endif // CPPSTANDARD_H