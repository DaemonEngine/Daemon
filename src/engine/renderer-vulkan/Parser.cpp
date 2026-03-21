/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2026 Daemon Developers
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
// Parser.cpp

#include "Parser.h"

StringView& StringView::operator++() {
	( *this ) += 1;

	return *this;
}

StringView  StringView::operator++( int ) {
	StringView t = *this;
	( *this )++;

	return t;
}

StringView& StringView::operator+=( const uint32 offset ) {
	size   -= offset;
	memory += offset;

	return *this;
}

bool operator==( const StringView& lhs, const char* rhs ) {
	if ( !lhs.size || !lhs.memory && rhs ) {
		return false;
	}

	if ( lhs.size && !rhs ) {
		return false;
	}

	const uint32_t strSize = strlen( rhs );

	if ( lhs.size != strSize ) {
		return false;
	}

	for ( const char* c = lhs.memory, *c2 = rhs; c < lhs.memory + lhs.size; c++, c2++ ) {
		if ( *c != *c2 ) {
			return false;
		}
	}

	return true;
}

StringView Parse( StringView& data, std::string* outStr, const char* allowedSymbols ) {
	const char* text = data.memory;

	uint32 newLines = 0;

	if ( !text ) {
		return {};
	}

	const char* current = text;

	if ( !current ) {
		return { text };
	}

	if ( outStr ) {
		outStr->reserve( data.size );
		*outStr = "";
	}

	uint32 offset = 0;

	int c;

	while ( *current ) {
		// Whitespace
		while ( ( c = *current & 0xFF ) <= ' ' ) {
			if ( !c ) {
				break;
			}

			if ( c == '\n' ) {
				newLines++;
			}

			current++;
			offset++;

			if ( outStr ) {
				outStr->push_back( c );
			}
		}

		if ( !current ) {
			return {};
		}

		c = *current;

		// Comments
		if ( c == '/' && current[1] == '/' ) {
			current += 2;
			offset  += 2;

			newLines++;

			while ( *current && *current != '\n' ) {
				current++;
				offset++;
			}
		} else if ( c == '/' && current[1] == '*' ) {
			current += 2;
			offset  += 2;

			while ( *current && ( *current != '*' || current[1] != '/' ) ) {
				if ( c == '\n' ) {
					newLines++;
				}

				current++;
				offset++;
			}

			if ( *current ) {
				current += 2;
				offset  += 2;
			}
		} else {
			break;
		}
	}

	uint32 size = 0;

	// Quoted strings
	if ( c == '\"' ) {
		current++;
		offset++;

		while ( true ) {
			c = *current;

			current++;

			if ( c == '\n' ) {
				newLines++;
			}

			if ( outStr ) {
				outStr->push_back( c );
			}

			if ( ( c == '\\' ) && ( *current == '\"' ) ) {
				// Allow quoted strings to use \" to indicate the " character
				current++;
			} else if ( c == '\"' || !c ) {
				data += offset + size + 1;
				return { text + offset, size };
			}

			size++;
		}
	}

	bool id = false;
	while ( true ) {
		c = *current;

		while ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) ) {
			if ( outStr ) {
				outStr->push_back( c );
			}

			current++;
			size++;
			c = *current;

			id = true;
		}

		const char* allowedSymbol;

		for ( allowedSymbol = allowedSymbols; allowedSymbol < allowedSymbols + strlen( allowedSymbols ); allowedSymbol++ ) {
			if ( c == *allowedSymbol ) {
				if ( outStr ) {
					outStr->push_back( c );
				}

				current++;
				size++;

				id = true;

				break;
			}
		}

		c = *current;

		if ( !*allowedSymbol ) {
			break;
		}
	}

	if ( id ) {
		data += offset + size;
		return { text + offset, size };
	}

	// Single character punctuation / EOF
	data += offset + ( c ? 1 : 0 );

	if ( outStr && c ) {
		outStr->push_back( c );
	}

	return { text + offset, c ? 1u : 0u };
}