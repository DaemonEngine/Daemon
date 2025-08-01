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
// Tag.h

#ifndef TAG_H
#define TAG_H

#include "common/Common.h"

namespace LogExtendedFunctionMode {
	enum {
		GLOBAL_NAME,
		NAME,
		TEMPLATE,
		FULL
	};
};

extern Cvar::Range<Cvar::Cvar<int>> r_vkLogExtendedFunctionNames;
extern Cvar::Cvar<bool> r_vkLogShowThreadID;

// This is kinda ugly, but it's the only way we can make it constexpr

constexpr size_t TypeDelimiter( const std::string_view name ) {
	return name.find_first_of( "::" );
}

constexpr size_t TypeWhiteSpace( const std::string_view name ) {
	return name.find_last_of( " " );
}

constexpr size_t TypeParenthesis( const std::string_view name ) {
	return name.find_first_of( "(" );
}

// GCC/Clang adds template specialisation at the end in []
constexpr size_t TypeClosingBracket( const std::string_view name ) {
	return name.find_last_of( "]" );
}

// MSVC adds template specialisation as <...> before the opening parenthesis
constexpr size_t TypeTemplateStart( const std::string_view name ) {
	return name.find_first_of( "<" );
}

constexpr size_t TypeStart( const std::string_view name ) {
	return TypeWhiteSpace( name ) == std::string_view::npos ? 0 : TypeWhiteSpace( name ) + 1;
}

constexpr std::string_view TypeLeft( const std::string_view name ) {
	return name.substr( 0, TypeDelimiter( name ) );
}

constexpr std::string_view TypeRight( const std::string_view name ) {
	return name.substr( TypeDelimiter( name ) == std::string_view::npos ? 0 : TypeDelimiter( name ) + 2 );
}

constexpr const std::string_view TypeName( const std::string_view name ) {
	return TypeLeft( name ).substr( TypeStart( TypeLeft( name ) ) );
}

constexpr const std::string_view FunctionName( const std::string_view name ) {
	#if defined(__GNUC__)
		return TypeRight( name ).substr( 0, TypeParenthesis( name ) );
	#else
		return TypeRight( name ).substr( 0, std::min( TypeParenthesis( TypeRight( name ) ), TypeTemplateStart( TypeRight( name ) ) ) );
	#endif
}

constexpr const std::string_view FunctionNameTemplate( const std::string_view name ) {
	#if defined(__GNUC__)
		return TypeRight( name ).substr( 0, TypeClosingBracket( TypeRight( name ) ) );
	#else
		return TypeRight( name ).substr( 0, TypeParenthesis( TypeRight( name ) ) );
	#endif
}

std::string Tagged( const std::string& message, const bool useThreadID,
	const std::source_location& loc = std::source_location::current() );

/* Add this as a base class to be able to use Log::WarnTag() etc.
Allows either specifying a custom name for an object, or otherwise automatically using the class name */
struct Tag {
	std::string Tagged( const std::string& message, const bool useThreadID,
		const std::source_location& loc = std::source_location::current() );

	protected:
	const bool useCustomName;
	const std::string name;

	Tag( const std::source_location loc = std::source_location::current() ) :
		useCustomName( false ),
		name( TypeName( loc.function_name() ) ) {
	}

	Tag( const std::string newName ) :
		useCustomName( true ),
		name( newName ) {
	}
};

// Use ##__VA_ARGS__ instead of __VA_ARGS__ because args may be empty. __VA_OPT__( , ) currently doesn't seem to work on MSVC
#define WarnTag( format, ... ) Warn( Tagged( format, r_vkLogShowThreadID.Get() ), ##__VA_ARGS__ )
#define NoticeTag( format, ... ) Notice( Tagged( format, r_vkLogShowThreadID.Get() ), ##__VA_ARGS__ )
#define VerboseTag( format, ... ) Verbose( Tagged( format, r_vkLogShowThreadID.Get() ), ##__VA_ARGS__ )
#define DebugTag( format, ... ) Debug( Tagged( format, r_vkLogShowThreadID.Get() ), ##__VA_ARGS__ )

#define WarnTagT( format, ... ) Warn( Tagged( format, true ), ##__VA_ARGS__ )
#define NoticeTagT( format, ... ) Notice( Tagged( format, true ), ##__VA_ARGS__ )
#define VerboseTagT( format, ... ) Verbose( Tagged( format, true ), ##__VA_ARGS__ )
#define DebugTagT( format, ... ) Debug( Tagged( format, true ), ##__VA_ARGS__ )

#endif // TAG_H