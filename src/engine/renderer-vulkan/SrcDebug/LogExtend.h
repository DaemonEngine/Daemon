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
// LogExtend.h

#ifndef LOGEXTEND_H
#define LOGEXTEND_H

#include <source_location>

#include "common/Common.h"

extern Cvar::Cvar<bool> r_vkLogExtend;

extern Cvar::Cvar<bool> r_vkLogExtendWarn;
extern Cvar::Cvar<bool> r_vkLogExtendNotice;
extern Cvar::Cvar<bool> r_vkLogExtendVerbose;
extern Cvar::Cvar<bool> r_vkLogExtendDebug;

namespace Log {
    inline Str::StringRef AddSrc( Str::StringRef format, const bool extend, const std::source_location& loc ) {
        if ( r_vkLogExtend.Get() || extend ) {
            return format + Str::Format( " (file: %s, line: %u:%u, func: %s",
                loc.file_name(), loc.line(), loc.column(), loc.function_name() );
        }

        return format;
    }

	#define WarnExt( format, ... ) ( Log::Warn( Log::AddSrc( format, r_vkLogExtendWarn.Get(), std::source_location::current() ), __VA_ARGS__ ) )
	#define NoticeExt( format, ... ) ( Log::Warn( Log::AddSrc( format, r_vkLogExtendNotice.Get(), std::source_location::current() ), __VA_ARGS__ ) )
	#define VerboseExt( format, ... ) ( Log::Warn( Log::AddSrc( format, r_vkLogExtendVerbose.Get(), std::source_location::current() ), __VA_ARGS__ ) )
	#define DebugExt( format, ... ) ( Log::Warn( Log::AddSrc( format, r_vkLogExtendDebug.Get(), std::source_location::current() ), __VA_ARGS__ ) )
}

#endif // LOGEXTEND_H