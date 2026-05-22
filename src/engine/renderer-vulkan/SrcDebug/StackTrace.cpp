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

#include "StackTrace.h"

#if defined( CPP_STACKTRACE )
	#include "../Parser.h"

	std::string FormatSrc( const std::stacktrace& stackTrace, const bool skipCurrent, const bool compact ) {
		bool        skipped    = !skipCurrent;
		bool        addLineEnd = false;

		std::string out;

		for ( const std::stacktrace_entry& entry : stackTrace ) {
			if ( !skipped ) {
				skipped = true;

				continue;
			}

			const std::string src = entry.source_file();

			if ( !src.size() ) {
				out += addLineEnd ? "\n.." : "..";

				continue;
			}

			if ( compact ) {
				StringView  v { src.c_str(), src.size() };
				StringView  o;
				std::string name;

				do {
					std::string outStr;
					o = Parse( v, &outStr, "" );

					if ( o == "." ) {
						break;
					}

					name = outStr;
				} while ( v.size );

				std::string extension;
				Parse( v, &extension );

				out += Str::Format( addLineEnd ? "\n%s.%s:%u" : "%s.%s:%u", name, extension, entry.source_line() );
			} else {
				out += Str::Format( addLineEnd ? "\n%s:%u: %s" : "%s:%u: %s", entry.source_file(), entry.source_line(), entry.description() );
			}

			addLineEnd = true;
		}

		return out;
	}
#else
	std::string FormatSrc( const std::stacktrace& stackTrace, const bool skipCurrent, const bool compact ) {
		return "unavailable";
	}
#endif