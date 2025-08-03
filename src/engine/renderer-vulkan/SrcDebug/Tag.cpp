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
// Tag.cpp

#include "Tag.h"

#include "../Thread/ThreadMemory.h"

std::string Tagged( const std::string& message, const bool useThreadID,
	const std::source_location& loc ) {
	const std::string threadID = useThreadID ? Str::Format( "[%u] ", TLM.id ) : "";

	switch ( r_vkLogExtendedFunctionNames.Get() ) {
		case LogExtendedFunctionMode::GLOBAL_NAME:
		case LogExtendedFunctionMode::NAME:
			return Str::Format( "%s:%s(): %s", threadID, FunctionName( loc.function_name() ), message );
		case LogExtendedFunctionMode::TEMPLATE:
			return Str::Format( "%s:%s(): %s", threadID, FunctionNameTemplate( loc.function_name() ), message );
		case LogExtendedFunctionMode::FULL:
			return Str::Format( "%s:%s(): %s", threadID, loc.function_name(), message );
		default:
			ASSERT_UNREACHABLE();
	}
}

/* Add this as a base class to be able to use Log::WarnTag() etc.
Allows either specifying a custom name for an object, or otherwise automatically using the class name */
std::string Tag::Tagged( const std::string& message, const bool useThreadID,
	const std::source_location& loc ) {
	const std::string threadID = useThreadID ?
		( TLM.id == ThreadMemory::MAIN_ID ? "main" : Str::Format( "Thread %u:", TLM.id ) )
		: "";

	switch ( r_vkLogExtendedFunctionNames.Get() ) {
		case LogExtendedFunctionMode::GLOBAL_NAME:
			return Str::Format( "%s%s: %s", threadID, name, message );
		case LogExtendedFunctionMode::NAME:
			return Str::Format( "%s%s:%s(): %s", threadID, name, FunctionName( loc.function_name() ), message );
		case LogExtendedFunctionMode::TEMPLATE:
			return Str::Format( "%s%s:%s(): %s", threadID, name, FunctionNameTemplate( loc.function_name() ), message );
		case LogExtendedFunctionMode::FULL:
			return Str::Format( "%s%s(): %s", threadID, loc.function_name(), message );
		default:
			ASSERT_UNREACHABLE();
	}
}