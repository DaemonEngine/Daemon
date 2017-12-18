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

// Object and string representations of keyboard keys.

#include "common/Common.h"

#include "key_identification.h"

#include <SDL_scancode.h>
#include <SDL_keyboard.h>

namespace Keyboard {

const Key Key::NONE = Key();
const Key Key::CONSOLE = Key(Key::Kind::CONSOLE, 0);

Key Key::FromScancode(int scancode) {
	if (scancode > static_cast<int>(SDL_SCANCODE_UNKNOWN)
		&& scancode < static_cast<int>(SDL_NUM_SCANCODES)) {
		return Key(Kind::SCANCODE, scancode);
	}
	return NONE;
}

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


struct keyname_t
{
	const char *name;
	Keyboard::Key keynum;
};

// names not in this list can either be lowercase ascii, or '0xnn' hex sequences
static const std::unordered_map<Str::StringRef, keyNum_t, Str::IHash, Str::IEqual> keynames
{
	{ "TAB",                    keyNum_t::K_TAB          },
	{ "ENTER",                  keyNum_t::K_ENTER        },
	{ "ESCAPE",                 keyNum_t::K_ESCAPE       },
	{ "BACKSPACE",              keyNum_t::K_BACKSPACE    },
	{ "UPARROW",                keyNum_t::K_UPARROW      },
	{ "DOWNARROW",              keyNum_t::K_DOWNARROW    },
	{ "LEFTARROW",              keyNum_t::K_LEFTARROW    },
	{ "RIGHTARROW",             keyNum_t::K_RIGHTARROW   },

	{ "ALT",                    keyNum_t::K_ALT          },
	{ "CTRL",                   keyNum_t::K_CTRL         },
	{ "SHIFT",                  keyNum_t::K_SHIFT        },

	{ "COMMAND",                keyNum_t::K_COMMAND      },

	{ "CAPSLOCK",               keyNum_t::K_CAPSLOCK     },

	{ "F1",                     keyNum_t::K_F1           },
	{ "F2",                     keyNum_t::K_F2           },
	{ "F3",                     keyNum_t::K_F3           },
	{ "F4",                     keyNum_t::K_F4           },
	{ "F5",                     keyNum_t::K_F5           },
	{ "F6",                     keyNum_t::K_F6           },
	{ "F7",                     keyNum_t::K_F7           },
	{ "F8",                     keyNum_t::K_F8           },
	{ "F9",                     keyNum_t::K_F9           },
	{ "F10",                    keyNum_t::K_F10          },
	{ "F11",                    keyNum_t::K_F11          },
	{ "F12",                    keyNum_t::K_F12          },
	{ "F13",                    keyNum_t::K_F13          },
	{ "F14",                    keyNum_t::K_F14          },
	{ "F15",                    keyNum_t::K_F15          },

	{ "INS",                    keyNum_t::K_INS          },
	{ "DEL",                    keyNum_t::K_DEL          },
	{ "PGDN",                   keyNum_t::K_PGDN         },
	{ "PGUP",                   keyNum_t::K_PGUP         },
	{ "HOME",                   keyNum_t::K_HOME         },
	{ "END",                    keyNum_t::K_END          },

	{ "MOUSE1",                 keyNum_t::K_MOUSE1       },
	{ "MOUSE2",                 keyNum_t::K_MOUSE2       },
	{ "MOUSE3",                 keyNum_t::K_MOUSE3       },
	{ "MOUSE4",                 keyNum_t::K_MOUSE4       },
	{ "MOUSE5",                 keyNum_t::K_MOUSE5       },

	{ "MWHEELUP",               keyNum_t::K_MWHEELUP     },
	{ "MWHEELDOWN",             keyNum_t::K_MWHEELDOWN   },

	{ "JOY1",                   keyNum_t::K_JOY1         },
	{ "JOY2",                   keyNum_t::K_JOY2         },
	{ "JOY3",                   keyNum_t::K_JOY3         },
	{ "JOY4",                   keyNum_t::K_JOY4         },
	{ "JOY5",                   keyNum_t::K_JOY5         },
	{ "JOY6",                   keyNum_t::K_JOY6         },
	{ "JOY7",                   keyNum_t::K_JOY7         },
	{ "JOY8",                   keyNum_t::K_JOY8         },
	{ "JOY9",                   keyNum_t::K_JOY9         },
	{ "JOY10",                  keyNum_t::K_JOY10        },
	{ "JOY11",                  keyNum_t::K_JOY11        },
	{ "JOY12",                  keyNum_t::K_JOY12        },
	{ "JOY13",                  keyNum_t::K_JOY13        },
	{ "JOY14",                  keyNum_t::K_JOY14        },
	{ "JOY15",                  keyNum_t::K_JOY15        },
	{ "JOY16",                  keyNum_t::K_JOY16        },
	{ "JOY17",                  keyNum_t::K_JOY17        },
	{ "JOY18",                  keyNum_t::K_JOY18        },
	{ "JOY19",                  keyNum_t::K_JOY19        },
	{ "JOY20",                  keyNum_t::K_JOY20        },
	{ "JOY21",                  keyNum_t::K_JOY21        },
	{ "JOY22",                  keyNum_t::K_JOY22        },
	{ "JOY23",                  keyNum_t::K_JOY23        },
	{ "JOY24",                  keyNum_t::K_JOY24        },
	{ "JOY25",                  keyNum_t::K_JOY25        },
	{ "JOY26",                  keyNum_t::K_JOY26        },
	{ "JOY27",                  keyNum_t::K_JOY27        },
	{ "JOY28",                  keyNum_t::K_JOY28        },
	{ "JOY29",                  keyNum_t::K_JOY29        },
	{ "JOY30",                  keyNum_t::K_JOY30        },
	{ "JOY31",                  keyNum_t::K_JOY31        },
	{ "JOY32",                  keyNum_t::K_JOY32        },

	{ "AUX1",                   keyNum_t::K_AUX1         },
	{ "AUX2",                   keyNum_t::K_AUX2         },
	{ "AUX3",                   keyNum_t::K_AUX3         },
	{ "AUX4",                   keyNum_t::K_AUX4         },
	{ "AUX5",                   keyNum_t::K_AUX5         },
	{ "AUX6",                   keyNum_t::K_AUX6         },
	{ "AUX7",                   keyNum_t::K_AUX7         },
	{ "AUX8",                   keyNum_t::K_AUX8         },
	{ "AUX9",                   keyNum_t::K_AUX9         },
	{ "AUX10",                  keyNum_t::K_AUX10        },
	{ "AUX11",                  keyNum_t::K_AUX11        },
	{ "AUX12",                  keyNum_t::K_AUX12        },
	{ "AUX13",                  keyNum_t::K_AUX13        },
	{ "AUX14",                  keyNum_t::K_AUX14        },
	{ "AUX15",                  keyNum_t::K_AUX15        },
	{ "AUX16",                  keyNum_t::K_AUX16        },

	{ "KP_HOME",                keyNum_t::K_KP_HOME      },
	{ "KP_7",                   keyNum_t::K_KP_HOME      },
	{ "KP_UPARROW",             keyNum_t::K_KP_UPARROW   },
	{ "KP_8",                   keyNum_t::K_KP_UPARROW   },
	{ "KP_PGUP",                keyNum_t::K_KP_PGUP      },
	{ "KP_9",                   keyNum_t::K_KP_PGUP      },
	{ "KP_LEFTARROW",           keyNum_t::K_KP_LEFTARROW },
	{ "KP_4",                   keyNum_t::K_KP_LEFTARROW },
	{ "KP_5",                   keyNum_t::K_KP_5         },
	{ "KP_RIGHTARROW",          keyNum_t::K_KP_RIGHTARROW},
	{ "KP_6",                   keyNum_t::K_KP_RIGHTARROW},
	{ "KP_END",                 keyNum_t::K_KP_END       },
	{ "KP_1",                   keyNum_t::K_KP_END       },
	{ "KP_DOWNARROW",           keyNum_t::K_KP_DOWNARROW },
	{ "KP_2",                   keyNum_t::K_KP_DOWNARROW },
	{ "KP_PGDN",                keyNum_t::K_KP_PGDN      },
	{ "KP_3",                   keyNum_t::K_KP_PGDN      },
	{ "KP_ENTER",               keyNum_t::K_KP_ENTER     },
	{ "KP_INS",                 keyNum_t::K_KP_INS       },
	{ "KP_0",                   keyNum_t::K_KP_INS       },
	{ "KP_DEL",                 keyNum_t::K_KP_DEL       },
	{ "KP_PERIOD",              keyNum_t::K_KP_DEL       },
	{ "KP_SLASH",               keyNum_t::K_KP_SLASH     },
	{ "KP_MINUS",               keyNum_t::K_KP_MINUS     },
	{ "KP_PLUS",                keyNum_t::K_KP_PLUS      },
	{ "KP_NUMLOCK",             keyNum_t::K_KP_NUMLOCK   },
	{ "KP_STAR",                keyNum_t::K_KP_STAR      },
	{ "KP_EQUALS",              keyNum_t::K_KP_EQUALS    },

	{ "PAUSE",                  keyNum_t::K_PAUSE        },

	{ "WINDOWS",                keyNum_t::K_SUPER        },
	{ "COMPOSE",                keyNum_t::K_COMPOSE      },
	{ "MODE",                   keyNum_t::K_MODE         },
	{ "HELP",                   keyNum_t::K_HELP         },
	{ "PRINT",                  keyNum_t::K_PRINT        },
	{ "SYSREQ",                 keyNum_t::K_SYSREQ       },
	{ "SCROLLLOCK",             keyNum_t::K_SCROLLLOCK   },
	{ "BREAK",                  keyNum_t::K_BREAK        },
	{ "MENU",                   keyNum_t::K_MENU         },
	{ "POWER",                  keyNum_t::K_POWER        },
	{ "EURO",                   keyNum_t::K_EURO         },
	{ "UNDO",                   keyNum_t::K_UNDO         },

	{ "XBOX360_A",              keyNum_t::K_XBOX360_A              },
	{ "XBOX360_B",              keyNum_t::K_XBOX360_B              },
	{ "XBOX360_X",              keyNum_t::K_XBOX360_X              },
	{ "XBOX360_Y",              keyNum_t::K_XBOX360_Y              },
	{ "XBOX360_LB",             keyNum_t::K_XBOX360_LB             },
	{ "XBOX360_RB",             keyNum_t::K_XBOX360_RB             },
	{ "XBOX360_START",          keyNum_t::K_XBOX360_START          },
	{ "XBOX360_GUIDE",          keyNum_t::K_XBOX360_GUIDE          },
	{ "XBOX360_LS",             keyNum_t::K_XBOX360_LS             },
	{ "XBOX360_RS",             keyNum_t::K_XBOX360_RS             },
	{ "XBOX360_BACK",           keyNum_t::K_XBOX360_BACK           },
	{ "XBOX360_LT",             keyNum_t::K_XBOX360_LT             },
	{ "XBOX360_RT",             keyNum_t::K_XBOX360_RT             },
	{ "XBOX360_DPAD_UP",        keyNum_t::K_XBOX360_DPAD_UP        },
	{ "XBOX360_DPAD_RIGHT",     keyNum_t::K_XBOX360_DPAD_RIGHT     },
	{ "XBOX360_DPAD_DOWN",      keyNum_t::K_XBOX360_DPAD_DOWN      },
	{ "XBOX360_DPAD_LEFT",      keyNum_t::K_XBOX360_DPAD_LEFT      },
	{ "XBOX360_DPAD_RIGHTUP",   keyNum_t::K_XBOX360_DPAD_RIGHTUP   },
	{ "XBOX360_DPAD_RIGHTDOWN", keyNum_t::K_XBOX360_DPAD_RIGHTDOWN },
	{ "XBOX360_DPAD_LEFTUP",    keyNum_t::K_XBOX360_DPAD_LEFTUP    },
	{ "XBOX360_DPAD_LEFTDOWN",  keyNum_t::K_XBOX360_DPAD_LEFTDOWN  },
};

static const Str::StringRef CHARACTER_BIND_PREFIX = "char:";
static const Str::StringRef SCANCODE_ASCII_BIND_PREFIX = "hw:";

static const std::unordered_map<char, Str::StringRef> SPECIAL_CHARACTER_NAMES {
    {';', "SEMICOLON"},
    {'\\', "BACKSLASH"},
    {' ', "SPACE"},
    {'"', "DOUBLEQUOTE"},
};

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
    SDL_Scancode sc = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(ch));
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
    size_t prefixLen = SCANCODE_ASCII_BIND_PREFIX.size();
    if ( Str::IsIPrefix( SCANCODE_ASCII_BIND_PREFIX, str ) )
    {
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

std::string CharToString(int ch)
{
    auto it = SPECIAL_CHARACTER_NAMES.find(ch);
    if (it != SPECIAL_CHARACTER_NAMES.end()) {
        return it->second;
    }
    return Q_UTF8_Encode(ch);
}

std::string KeyToString(Key key)
{
    if (key.kind() == Key::Kind::KEYNUM) {
        for (auto& kv : keynames) {
            if ( kv.second == key.AsKeynum() ) {
                return kv.first;
            }
        }
        Log::Warn("Unknown keynum %d", Util::ordinal(key.AsKeynum()));
    }

    if ( key.kind() == Key::Kind::UNICODE_CHAR )
    {
        return std::string(CHARACTER_BIND_PREFIX) + CharToString(key.AsCharacter());
    }

    if ( key.kind() == Key::Kind::SCANCODE ) {
        int sc = key.AsScancode();
        if ( char c = ScancodeToAscii(sc) ) {
            return std::string(SCANCODE_ASCII_BIND_PREFIX) + CharToString(c);
        } else {
            // make a hex string
            return Str::Format("0x%02x", key.AsScancode());
        }
    }

    return "<INVALID KEY>";
}

int GetCharForScancode(int scancode) {
    int keycode = static_cast<int>(SDL_GetKeyFromScancode(Util::enum_cast<SDL_Scancode>(scancode)));
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
}

} // namespace Keyboard
