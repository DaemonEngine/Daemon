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


static char ScancodeToAscii(int sc)
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
static const keyname_t keynames[] =
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

	{ "WORLD_0",                keyNum_t::K_WORLD_0      },
	{ "WORLD_1",                keyNum_t::K_WORLD_1      },
	{ "WORLD_2",                keyNum_t::K_WORLD_2      },
	{ "WORLD_3",                keyNum_t::K_WORLD_3      },
	{ "WORLD_4",                keyNum_t::K_WORLD_4      },
	{ "WORLD_5",                keyNum_t::K_WORLD_5      },
	{ "WORLD_6",                keyNum_t::K_WORLD_6      },
	{ "WORLD_7",                keyNum_t::K_WORLD_7      },
	{ "WORLD_8",                keyNum_t::K_WORLD_8      },
	{ "WORLD_9",                keyNum_t::K_WORLD_9      },
	{ "WORLD_10",               keyNum_t::K_WORLD_10     },
	{ "WORLD_11",               keyNum_t::K_WORLD_11     },
	{ "WORLD_12",               keyNum_t::K_WORLD_12     },
	{ "WORLD_13",               keyNum_t::K_WORLD_13     },
	{ "WORLD_14",               keyNum_t::K_WORLD_14     },
	{ "WORLD_15",               keyNum_t::K_WORLD_15     },
	{ "WORLD_16",               keyNum_t::K_WORLD_16     },
	{ "WORLD_17",               keyNum_t::K_WORLD_17     },
	{ "WORLD_18",               keyNum_t::K_WORLD_18     },
	{ "WORLD_19",               keyNum_t::K_WORLD_19     },
	{ "WORLD_20",               keyNum_t::K_WORLD_20     },
	{ "WORLD_21",               keyNum_t::K_WORLD_21     },
	{ "WORLD_22",               keyNum_t::K_WORLD_22     },
	{ "WORLD_23",               keyNum_t::K_WORLD_23     },
	{ "WORLD_24",               keyNum_t::K_WORLD_24     },
	{ "WORLD_25",               keyNum_t::K_WORLD_25     },
	{ "WORLD_26",               keyNum_t::K_WORLD_26     },
	{ "WORLD_27",               keyNum_t::K_WORLD_27     },
	{ "WORLD_28",               keyNum_t::K_WORLD_28     },
	{ "WORLD_29",               keyNum_t::K_WORLD_29     },
	{ "WORLD_30",               keyNum_t::K_WORLD_30     },
	{ "WORLD_31",               keyNum_t::K_WORLD_31     },
	{ "WORLD_32",               keyNum_t::K_WORLD_32     },
	{ "WORLD_33",               keyNum_t::K_WORLD_33     },
	{ "WORLD_34",               keyNum_t::K_WORLD_34     },
	{ "WORLD_35",               keyNum_t::K_WORLD_35     },
	{ "WORLD_36",               keyNum_t::K_WORLD_36     },
	{ "WORLD_37",               keyNum_t::K_WORLD_37     },
	{ "WORLD_38",               keyNum_t::K_WORLD_38     },
	{ "WORLD_39",               keyNum_t::K_WORLD_39     },
	{ "WORLD_40",               keyNum_t::K_WORLD_40     },
	{ "WORLD_41",               keyNum_t::K_WORLD_41     },
	{ "WORLD_42",               keyNum_t::K_WORLD_42     },
	{ "WORLD_43",               keyNum_t::K_WORLD_43     },
	{ "WORLD_44",               keyNum_t::K_WORLD_44     },
	{ "WORLD_45",               keyNum_t::K_WORLD_45     },
	{ "WORLD_46",               keyNum_t::K_WORLD_46     },
	{ "WORLD_47",               keyNum_t::K_WORLD_47     },
	{ "WORLD_48",               keyNum_t::K_WORLD_48     },
	{ "WORLD_49",               keyNum_t::K_WORLD_49     },
	{ "WORLD_50",               keyNum_t::K_WORLD_50     },
	{ "WORLD_51",               keyNum_t::K_WORLD_51     },
	{ "WORLD_52",               keyNum_t::K_WORLD_52     },
	{ "WORLD_53",               keyNum_t::K_WORLD_53     },
	{ "WORLD_54",               keyNum_t::K_WORLD_54     },
	{ "WORLD_55",               keyNum_t::K_WORLD_55     },
	{ "WORLD_56",               keyNum_t::K_WORLD_56     },
	{ "WORLD_57",               keyNum_t::K_WORLD_57     },
	{ "WORLD_58",               keyNum_t::K_WORLD_58     },
	{ "WORLD_59",               keyNum_t::K_WORLD_59     },
	{ "WORLD_60",               keyNum_t::K_WORLD_60     },
	{ "WORLD_61",               keyNum_t::K_WORLD_61     },
	{ "WORLD_62",               keyNum_t::K_WORLD_62     },
	{ "WORLD_63",               keyNum_t::K_WORLD_63     },
	{ "WORLD_64",               keyNum_t::K_WORLD_64     },
	{ "WORLD_65",               keyNum_t::K_WORLD_65     },
	{ "WORLD_66",               keyNum_t::K_WORLD_66     },
	{ "WORLD_67",               keyNum_t::K_WORLD_67     },
	{ "WORLD_68",               keyNum_t::K_WORLD_68     },
	{ "WORLD_69",               keyNum_t::K_WORLD_69     },
	{ "WORLD_70",               keyNum_t::K_WORLD_70     },
	{ "WORLD_71",               keyNum_t::K_WORLD_71     },
	{ "WORLD_72",               keyNum_t::K_WORLD_72     },
	{ "WORLD_73",               keyNum_t::K_WORLD_73     },
	{ "WORLD_74",               keyNum_t::K_WORLD_74     },
	{ "WORLD_75",               keyNum_t::K_WORLD_75     },
	{ "WORLD_76",               keyNum_t::K_WORLD_76     },
	{ "WORLD_77",               keyNum_t::K_WORLD_77     },
	{ "WORLD_78",               keyNum_t::K_WORLD_78     },
	{ "WORLD_79",               keyNum_t::K_WORLD_79     },
	{ "WORLD_80",               keyNum_t::K_WORLD_80     },
	{ "WORLD_81",               keyNum_t::K_WORLD_81     },
	{ "WORLD_82",               keyNum_t::K_WORLD_82     },
	{ "WORLD_83",               keyNum_t::K_WORLD_83     },
	{ "WORLD_84",               keyNum_t::K_WORLD_84     },
	{ "WORLD_85",               keyNum_t::K_WORLD_85     },
	{ "WORLD_86",               keyNum_t::K_WORLD_86     },
	{ "WORLD_87",               keyNum_t::K_WORLD_87     },
	{ "WORLD_88",               keyNum_t::K_WORLD_88     },
	{ "WORLD_89",               keyNum_t::K_WORLD_89     },
	{ "WORLD_90",               keyNum_t::K_WORLD_90     },
	{ "WORLD_91",               keyNum_t::K_WORLD_91     },
	{ "WORLD_92",               keyNum_t::K_WORLD_92     },
	{ "WORLD_93",               keyNum_t::K_WORLD_93     },
	{ "WORLD_94",               keyNum_t::K_WORLD_94     },
	{ "WORLD_95",               keyNum_t::K_WORLD_95     },

	{ "WINDOWS",                keyNum_t::K_SUPER        },
	{ "COMPOSE",                keyNum_t::K_COMPOSE      },
	{ "MODE",                   keyNum_t::K_MODE         },
	{ "HELP",                   keyNum_t::K_HELP         },
	{ "PRINT",                  keyNum_t::K_PRINT        },
	{ "SYSREQ",                 keyNum_t::K_SYSREQ       },
	{ "SCROLLLOCK",              keyNum_t::K_SCROLLLOCK  },
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

	{ nullptr,                  Key::NONE                          }
};

static const char CHARACTER_BIND_PREFIX[] = "char:";
static const char SCANCODE_ASCII_BIND_PREFIX[] = "hw:";

static int ParseCharacter(Str::StringRef s)
{
	if (!Q_stricmp(s.data(), "SEMICOLON"))
		return ';';
	if (!Q_stricmp(s.data(), "BACKSLASH"))
		return '\\';
	if (!Q_stricmp(s.data(), "SPACE"))
		return ' ';
	if (Q_UTF8_Strlen(s.data()) == 1)
		return Str::ctolower(Str::UTF8To32(s.data())[0]);
	return 0;
}

// This takes a character name and tries to find a key that types that character.
// If the SDL lookup function doesn't find one, fall back to the scancode
// for the key on a QWERTY layout, if possible.
static Key KeyFromUnprefixedCharacter(int ch)
{
	SDL_Scancode sc = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(ch));
	if (sc != SDL_SCANCODE_UNKNOWN) {
		return Key::FromScancode(static_cast<int>(sc));
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
	const keyname_t *kn;

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
	int prefixLen = strlen(SCANCODE_ASCII_BIND_PREFIX);
	if ( Str::IsIPrefix( SCANCODE_ASCII_BIND_PREFIX, str ) && str.size() == prefixLen + 1 )
	{
		return Key::FromScancode( AsciiToScancode( ParseCharacter ( str.substr( prefixLen ) ) ) );
	}

	// char:X forces a virtual key-based rather than scancode binding to be used for some (Unicode) char
	prefixLen = strlen( CHARACTER_BIND_PREFIX );
	if ( Str::IsIPrefix( CHARACTER_BIND_PREFIX, str ) && Q_UTF8_Strlen( &str[prefixLen] ) == 1 )
	{
		return Key::FromCharacter( ParseCharacter( str.substr( prefixLen ) ) );
	}

	// scan for a text match
	for ( kn = keynames; kn->name; kn++ )
	{
		if ( Str::IsIEqual( str, kn->name ) )
		{
			return kn->keynum;
		}
	}

	return Key::NONE;
}



std::string KeyToString(Key key)
{
	const keyname_t *kn;

	// check for a key string
	for ( kn = keynames; kn->name; kn++ )
	{
		if ( key == kn->keynum )
		{
			return kn->name;
		}
	}

	if ( key.kind() == Key::Kind::UNICODE_CHAR )
	{
		return std::string(CHARACTER_BIND_PREFIX) + Q_UTF8_Encode(key.AsCharacter());
	}

	if ( key.kind() == Key::Kind::SCANCODE ) {
		int sc = key.AsScancode();
		if ( char c = ScancodeToAscii(sc) ) {
			return std::string(SCANCODE_ASCII_BIND_PREFIX) + c;
		} else {
			// make a hex string
			return Str::Format("0x%03x", key.AsScancode());
		}
	}

	return "<INVALID KEY>";
}


void CompleteKeyName(Cmd::CompletionResult& completions, Str::StringRef prefix)
{
	int i;

	for ( i = 0; keynames[ i ].name != nullptr; i++ )
	{
		Cmd::AddToCompletion(completions, prefix, {{keynames[i].name, ""}});
	}
}

} // namespace Keyboard
