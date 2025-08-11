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
// StackTrace.h

#ifndef STACKTRACE_H
#define STACKTRACE_H

#include "CPPStandard.h"
#include "String.h"

#include "Log.h"

#if defined( CPP_STACKTRACE )

    #include <stacktrace>

    inline std::string FormatStackTrace( const std::stacktrace& stackTrace,
        const bool skipCurrent = false, const bool compact = false ) {
        std::string out;
        
        bool skipped = !skipCurrent;
        bool addLineEnd = false;
        for ( const std::stacktrace_entry& entry : stackTrace ) {
            if ( !skipped ) {
                skipped = true;
                continue;
            }

            std::string file = entry.source_file();
            if ( compact ) {
                const size_t pos = std::min(
                    file.find( "src/engine/renderer-vulkan" ),
                    file.find( "src\\engine\\renderer-vulkan" )
                );

                if ( pos == std::string::npos ) {
                    continue;
                }

                file = file.substr( pos + 27 );
            }

            if( compact ) {
                out += Str::Format( addLineEnd ? "\n%s:%u" : "%s:%u", file, entry.source_line() );
            } else {
                out += Str::Format( addLineEnd ? "\n%s:%u: %s" : "%s:%u: %s", file, entry.source_line(), entry.description() );
            }
            addLineEnd = true;
        }

        return out;
    }

    inline void PrintStackTrace( const std::stacktrace& stackTrace = std::stacktrace::current() ) {
        Log::Warn( "\n\n====================\nStackTrace:\n%s\n====================\n\n", FormatStackTrace( stackTrace ) );
    }

#else

    inline void PrintStackTrace() {
        Log::Warn( "StackTrace unavailable: CPP23 required" );
    }

#endif

#endif // STACKTRACE_H