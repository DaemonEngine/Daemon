/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "common/Common.h"

#include "key_identification.h"

#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_keyboard.h>

namespace Keyboard {

static_assert(Scancode::LALT == SDL_SCANCODE_LALT, "");
static_assert(Scancode::RALT == SDL_SCANCODE_RALT, "");
static_assert(Scancode::LSHIFT == SDL_SCANCODE_LSHIFT, "");
static_assert(Scancode::RSHIFT == SDL_SCANCODE_RSHIFT, "");
static_assert(Scancode::LCTRL == SDL_SCANCODE_LCTRL, "");
static_assert(Scancode::RCTRL == SDL_SCANCODE_RCTRL, "");

// Returns the scancode of a char for a QWERTY keyboard
static int AsciiToScancode(int ch)
{
	ch = Str::ctolower(ch);
	if (ch >= 'a' && ch <= 'z') {
		return ch - 'a' + Util::ordinal<SDL_Scancode>(SDL_SCANCODE_A);
	}
	if (ch >= '1' && ch <= '9') {
		return ch - '1' + Util::ordinal<SDL_Scancode>(SDL_SCANCODE_1);
	}
	switch (ch) {
		case ' ': return SDL_SCANCODE_SPACE;
		case '\'': return SDL_SCANCODE_APOSTROPHE;
		case ',': return SDL_SCANCODE_COMMA;
		case '.': return SDL_SCANCODE_PERIOD;
		case '/': return SDL_SCANCODE_SLASH;
		case '-': return SDL_SCANCODE_MINUS;
		case '0': return SDL_SCANCODE_0;
		case ';': return SDL_SCANCODE_SEMICOLON;
		case '=': return SDL_SCANCODE_EQUALS;
		case '[': return SDL_SCANCODE_LEFTBRACKET;
		case '\\': return SDL_SCANCODE_BACKSLASH;
		case ']': return SDL_SCANCODE_RIGHTBRACKET;
		case '`': return SDL_SCANCODE_GRAVE;

		default: return SDL_SCANCODE_UNKNOWN;
	}

}


char ScancodeToAscii(int sc)
{
	if (SDL_SCANCODE_A <= sc && sc <= SDL_SCANCODE_Z) {
		return sc - Util::ordinal<SDL_Scancode>(SDL_SCANCODE_A) + 'a';
	}
	if (SDL_SCANCODE_1 <= sc && sc <= SDL_SCANCODE_9) {
		return sc - Util::ordinal<SDL_Scancode>(SDL_SCANCODE_1) + '1';
	}
	switch (sc) {
		case SDL_SCANCODE_SPACE: return ' ';
		case SDL_SCANCODE_APOSTROPHE: return '\'';
		case SDL_SCANCODE_COMMA: return ',';
		case SDL_SCANCODE_PERIOD: return '.';
		case SDL_SCANCODE_SLASH: return '/';
		case SDL_SCANCODE_MINUS: return '-';
		case SDL_SCANCODE_0: return '0';
		case SDL_SCANCODE_SEMICOLON: return ';';
		case SDL_SCANCODE_EQUALS: return '=';
		case SDL_SCANCODE_LEFTBRACKET: return '[';
		case SDL_SCANCODE_BACKSLASH: return '\\';
		case SDL_SCANCODE_RIGHTBRACKET: return ']';
		case SDL_SCANCODE_GRAVE: return '`';

		default: return '\0';
	}

}


static const Str::StringRef CHARACTER_BIND_PREFIX = "char:";
static const Str::StringRef SCANCODE_BIND_PREFIX = "hw:";

static int ParseCharacter(Str::StringRef s)
{
    for (auto kv: SPECIAL_CHARACTER_NAMES) {
        if (Str::IsIEqual(s, kv.second)) {
            return kv.first;
        }
    }
    if (Q_UTF8_Strlen(s.data()) == 1) {
        return Str::ctolower(Str::UTF8To32(s.data())[0]);
    }
    return 0;
}

// This takes a character name and tries to find a key that types that character.
// If the SDL lookup function doesn't find one, fall back to the scancode
// for the key on a QWERTY layout, if possible.
static Key KeyFromUnprefixedCharacter(int ch)
{
    SDL_Scancode sc = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(ch), nullptr);
    if (sc != SDL_SCANCODE_UNKNOWN) {
        return Key::FromScancode(sc);
    }
    return Key::FromScancode(AsciiToScancode(ch));
}

/*
    Accepted formats:
        hw:<ascii char> => QWERTY-based scancode
        char:<unicode char> => character-based key representation
        0xnnn => numeric scancode
        <unicode char> => Find key producing this char with SDL lookup function.
                          If that fails, try falling back to qwerty instead.
        "SHIFT", etc. => keynum representation for this name
*/
Key StringToKey(Str::StringRef str)
{
    if ( int c = ParseCharacter( str ) )
    {
        return KeyFromUnprefixedCharacter( c );
    }

    // check for hex code
    if ( Str::IsIPrefix("0x", str) )
    {
        int n = Com_HexStrToInt( str.c_str() );
        return Key::FromScancode( n );
    }

    // Physical key by QWERTY location, from ascii char
    size_t prefixLen = SCANCODE_BIND_PREFIX.size();
    if ( Str::IsIPrefix( SCANCODE_BIND_PREFIX, str ) )
    {
        for (auto& functionKey : leftRightFunctionKeys) {
            if ( Str::IsIEqual( str, functionKey.name ) ) {
                return Key::FromScancode( functionKey.scancode );
            }
        }
        return Key::FromScancode( AsciiToScancode( ParseCharacter ( str.substr( prefixLen ) ) ) );
    }

    // char:X forces a virtual key-based rather than scancode binding to be used for some (Unicode) char
    prefixLen = CHARACTER_BIND_PREFIX.size();
    if ( Str::IsIPrefix( CHARACTER_BIND_PREFIX, str ) )
    {
        return Key::FromCharacter( ParseCharacter( str.substr( prefixLen ) ) );
    }

    auto it = keynames.find(str);
    if (it != keynames.end()) {
        return Key(it->second);
    }

    return Key::NONE;
}

std::string KeyToString(Key key)
{
    if (key.kind() == Key::Kind::KEYNUM) {
        return KeynumToString(key.AsKeynum());
    }

    if ( key.kind() == Key::Kind::UNICODE_CHAR )
    {
        return std::string(CHARACTER_BIND_PREFIX) + CharToString(key.AsCharacter());
    }

    if ( key.kind() == Key::Kind::SCANCODE ) {
        int sc = key.AsScancode();
        if ( char c = ScancodeToAscii(sc) ) {
            return std::string(SCANCODE_BIND_PREFIX) + CharToString(c);
        }
        for (auto& functionKey : leftRightFunctionKeys) {
            if (sc == functionKey.scancode) {
                return functionKey.name;
            }
        }
        // make a hex string
        return Str::Format("0x%02x", key.AsScancode());
    }

    return "<INVALID KEY>";
}

int GetCharForScancode(int scancode) {
    int keycode = static_cast<int>(SDL_GetKeyFromScancode(Util::enum_cast<SDL_Scancode>(scancode), 0, false));
    // The keycode is a "large" number for keys such as Shift
    if (MIN_PRINTABLE_ASCII <= keycode && keycode <= UNICODE_MAX_CODE_POINT) {
        return keycode;
    }
    return 0;
}

void CompleteKeyName(Cmd::CompletionResult& completions, Str::StringRef prefix)
{
    for (auto& kv : keynames)
    {
        Cmd::AddToCompletion(completions, prefix, {{kv.first, ""}});
    }
    for (auto& functionKey : leftRightFunctionKeys)
    {
        Cmd::AddToCompletion(completions, prefix, {{functionKey.name, ""}});
    }
}

} // namespace Keyboard
