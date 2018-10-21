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

#include "common/KeyIdentification.h"

namespace Keyboard {

const Key Key::NONE = Key();
const Key Key::CONSOLE = Key(Key::Kind::CONSOLE, 0);

const std::unordered_map<Str::StringRef, keyNum_t, Str::IHash, Str::IEqual> keynames
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

	{ "CONTROLLER_A",              keyNum_t::K_CONTROLLER_A              },
	{ "CONTROLLER_B",              keyNum_t::K_CONTROLLER_B              },
	{ "CONTROLLER_X",              keyNum_t::K_CONTROLLER_X              },
	{ "CONTROLLER_Y",              keyNum_t::K_CONTROLLER_Y              },
	{ "CONTROLLER_LB",             keyNum_t::K_CONTROLLER_LB             },
	{ "CONTROLLER_RB",             keyNum_t::K_CONTROLLER_RB             },
	{ "CONTROLLER_START",          keyNum_t::K_CONTROLLER_START          },
	{ "CONTROLLER_GUIDE",          keyNum_t::K_CONTROLLER_GUIDE          },
	{ "CONTROLLER_LS",             keyNum_t::K_CONTROLLER_LS             },
	{ "CONTROLLER_RS",             keyNum_t::K_CONTROLLER_RS             },
	{ "CONTROLLER_BACK",           keyNum_t::K_CONTROLLER_BACK           },
	{ "CONTROLLER_LT",             keyNum_t::K_CONTROLLER_LT             },
	{ "CONTROLLER_RT",             keyNum_t::K_CONTROLLER_RT             },
	{ "CONTROLLER_DPAD_UP",        keyNum_t::K_CONTROLLER_DPAD_UP        },
	{ "CONTROLLER_DPAD_RIGHT",     keyNum_t::K_CONTROLLER_DPAD_RIGHT     },
	{ "CONTROLLER_DPAD_DOWN",      keyNum_t::K_CONTROLLER_DPAD_DOWN      },
	{ "CONTROLLER_DPAD_LEFT",      keyNum_t::K_CONTROLLER_DPAD_LEFT      },
};

const std::unordered_map<char, Str::StringRef> SPECIAL_CHARACTER_NAMES {
    {';', "SEMICOLON"},
    {'\\', "BACKSLASH"},
    {' ', "SPACE"},
    {'"', "DOUBLEQUOTE"},
};

std::string CharToString(int ch)
{
    auto it = SPECIAL_CHARACTER_NAMES.find(ch);
    if (it != SPECIAL_CHARACTER_NAMES.end()) {
        return it->second;
    }
    return Q_UTF8_Encode(ch);
}

std::string KeynumToString(keyNum_t keynum)
{
    for (auto& kv : keynames) {
        if (kv.second == keynum) {
            return kv.first;
        }
    }
    Log::Warn("Unknown keynum %d", Util::ordinal(keynum));
    return "<UNKNOWN KEYNUM>";
}

} // namespace Keyboard
