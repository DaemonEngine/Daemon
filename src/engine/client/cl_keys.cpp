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

#include "client.h"
#include "engine/client/key_identification.h"
#include "qcommon/q_unicode.h"
#include "framework/CommandSystem.h"

/*

key up events are sent even if in console mode

*/

static bool overstrikeModeOn;
bool bindingsModified;

static int anykeydown;
std::unordered_map<Keyboard::Key, qkey_t, Keyboard::Key::hash> keys;

static struct {
	Keyboard::Key key;
	unsigned int time;
	bool     valid;
	int          check;
} plusCommand;


/*
=============================================================================

EDIT FIELDS

=============================================================================
*/

/*
===================
Field_Draw

Handles horizontal scrolling and cursor blinking
x, y, and width are in pixels
===================
*/
void Field_Draw(const Util::LineEditData& edit, int x, int y, bool showCursor, bool noColorEscape, float alpha)
{
    //TODO support UTF-8 once LineEditData does
    //Extract the text we want to draw
    int len = edit.GetText().size();
    int lineStart = edit.GetViewStartPos();
    int cursorPos = edit.GetViewCursorPos();
    int drawWidth = std::min<size_t>(edit.GetWidth() - 1, len - lineStart);
    std::string text = Str::UTF32To8(std::u32string(edit.GetViewText(), drawWidth));

    // draw the text
	Color::Color color { 1.0f, 1.0f, 1.0f, alpha };
	SCR_DrawSmallStringExt(x, y, text.c_str(), color, false, noColorEscape);

    // draw the line scrollbar
    if (len > drawWidth) {
        static const Color::Color yellow = { 1.0f, 1.0f, 0.0f, 0.25f };
        float width = SCR_ConsoleFontStringWidth(text.c_str(), drawWidth);

        re.SetColor( yellow );
        re.DrawStretchPic(x + (width * lineStart) / len, y + 3, (width * drawWidth) / len, 2, 0, 0, 0, 0, cls.whiteShader);
    }

    // draw the cursor
    if (showCursor) {
        //Blink changes state approximately 4 times per second
        if (cls.realtime >> 8 & 1) {
            return;
        }

        Color::Color supportElementsColor = {1.0f, 1.0f, 1.0f, 0.66f * consoleState.currentAlphaFactor};
        re.SetColor( supportElementsColor );

        //Compute the position of the cursor
        float xpos, width, height;
		xpos = x + SCR_ConsoleFontStringWidth(text.c_str(), cursorPos);
		height = overstrikeModeOn ? SMALLCHAR_HEIGHT / (CONSOLE_FONT_VPADDING + 1) : 2;
		width = SMALLCHAR_WIDTH;

        re.DrawStretchPic(xpos, y + 2 - height, width, height, 0, 0, 0, 0, cls.whiteShader);
    }
}

/*
================
Field_Paste
================
*/
static void Field_Paste(Util::LineEditData& edit)
{
	int        pasteLen, width;
    char buffer[1024];

    CL_GetClipboardData(buffer, sizeof(buffer));

    const char* cbd = buffer;

	// send as if typed, so insert / overstrike works properly
	pasteLen = strlen( cbd );

	while ( pasteLen >= ( width = Q_UTF8_Width( cbd ) ) )
	{
		Field_CharEvent( edit, Q_UTF8_CodePoint( cbd ) );

		cbd += width;
		pasteLen -= width;
	}
}

/*
=================
Field_KeyDownEvent

Performs the basic line editing functions for the console,
in-game talk, and menu fields

Key events are used for non-printable characters, others are gotten from char events.
=================
*/
void Field_KeyDownEvent(Util::LineEditData& edit, Keyboard::Key key) {
    using Keyboard::Key;
    if (key.kind() == Key::Kind::KEYNUM) {
        switch (key.AsKeynum()) {
        case K_DEL:
            edit.DeleteNext();
            break;

        case K_BACKSPACE:
            edit.DeletePrev();
            break;

        case K_RIGHTARROW:
            if (keys[ Key(K_CTRL) ].down) {
                //TODO: Skip a full word
                edit.CursorRight();
            } else {
                edit.CursorRight();
            }
            break;

        case K_LEFTARROW:
            if (keys[ Key(K_CTRL) ].down) {
                //TODO: Skip a full word
                edit.CursorLeft();
            } else {
                edit.CursorLeft();
            }
            break;
        case K_HOME:
            edit.CursorStart();
            break;
        case K_END:
            edit.CursorEnd();
            break;

        case K_INS:
            if (keys[ Key(K_SHIFT) ].down) {
                Field_Paste(edit);
            } else {
                overstrikeModeOn = !overstrikeModeOn;
            }
            break;
        default:
            break;
        }
    } else if (key.kind() == Key::Kind::UNICODE_CHAR) {
        switch ((char) key.AsCharacter()) {
        case 'a':
            if (keys[ Key(K_CTRL) ].down) {
                edit.CursorStart();
            }
            break;
        case 'e':
            if (keys[ Key(K_CTRL) ].down) {
                edit.CursorEnd();
            }
            break;
        case 'v':
            if (keys[ Key(K_CTRL) ].down) {
                Field_Paste( edit );
            }
            break;
        case 'd':
            if (keys[ Key(K_CTRL) ].down) {
                edit.DeleteNext();
            }
            break;
        case 'c':
        case 'u':
            if (keys[ Key(K_CTRL) ].down) {
                edit.Clear();
            }
            break;
        case 'k':
            if (keys[ Key(K_CTRL) ].down) {
                edit.DeleteEnd();
            }
            break;
        default:
            break;
        }
    }
}

/*
==================
Field_CharEvent
==================
*/
void Field_CharEvent(Util::LineEditData& edit, int c )
{
    //
    // ignore any non printable chars
    //
    if ( c < 32 || c == 0x7f )
    {
        return;
    }

    if (overstrikeModeOn) {
        edit.DeleteNext();
    }
    edit.AddChar(c);
}

/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

/*
====================
Console_Key

Handles history and console scrollback for the ingame console
====================
*/
static void Console_Key( Keyboard::Key key )
{
	using Keyboard::Key;
	// just return if any of the listed modifiers are pressed
	// - no point in passing on, since they Just Get In The Way
	if ( keys[ Key(K_ALT) ].down || keys[ Key(K_COMMAND) ].down ||
			keys[ Key(K_MODE) ].down || keys[ Key(K_SUPER) ].down )
	{
		return;
	}

	// ctrl-L clears screen
	if (key == Key::FromCharacter('l') && keys[ Key(K_CTRL) ].down) {
		Cmd::BufferCommandText("clear");
		return;
	}

	// enter finishes the line
	if (key == Key(K_ENTER) or key == Key(K_KP_ENTER)) {

		//scroll lock state 1 or smaller will scroll down on own output
		if (con_scrollLock->integer <= 1) {
			consoleState.scrollLineIndex = consoleState.lines.size() - 1;
		}

		Log::CommandInteractionMessage(Str::Format("]%s", Str::UTF32To8(g_consoleField.GetText())));

		// if not in the game always treat the input as a command
		if (cls.state != connstate_t::CA_ACTIVE) {
			g_consoleField.RunCommand();
		} else {
			g_consoleField.RunCommand(cl_consoleCommand->string);
		}

		if (cls.state == connstate_t::CA_DISCONNECTED) {
			SCR_UpdateScreen(); // force an update, because the command may take some time
		}
		return;
	}

	// command completion

	if ( key == Key(K_TAB) )
	{
		g_consoleField.AutoComplete();
		return;
	}

	// command history (ctrl-p ctrl-n for unix style)

	//----(SA)  added some mousewheel functionality to the console
	if ( ( key == Key(K_MWHEELUP) && keys[ Key(K_SHIFT) ].down ) || ( key == Key(K_UPARROW) ) || ( key == Key(K_KP_UPARROW) ) ||
			( ( key == Key::FromCharacter('p') ) && keys[ Key(K_CTRL) ].down ) )
	{
		g_consoleField.HistoryPrev();
		return;
	}

	//----(SA)  added some mousewheel functionality to the console
	if ( ( key == Key(K_MWHEELDOWN) && keys[ Key(K_SHIFT) ].down ) || ( key == Key(K_DOWNARROW) ) || ( key == Key(K_KP_DOWNARROW) ) ||
			( ( key == Key::FromCharacter('n') ) && keys[ Key(K_CTRL) ].down ) )
	{
		g_consoleField.HistoryNext();
		return;
	}

	// console scrolling
	if ( key == Key(K_PGUP) || key == Key(K_KP_PGUP) )
	{
		Con_PageUp();
		return;
	}

	if ( key == Key(K_PGDN) || key == Key(K_KP_PGDN) )
	{
		Con_PageDown();
		return;
	}

	if ( key == Key(K_MWHEELUP) ) //----(SA) added some mousewheel functionality to the console
	{
		Con_PageUp();

		if ( keys[ Key(K_CTRL) ].down ) // hold <ctrl> to accelerate scrolling
		{
			Con_ScrollUp( consoleState.visibleAmountOfLines );
		}

		return;
	}

	if ( key == Key(K_MWHEELDOWN) ) //----(SA) added some mousewheel functionality to the console
	{
		Con_PageDown();

		if ( keys[ Key(K_CTRL) ].down ) // hold <ctrl> to accelerate scrolling
		{
			Con_ScrollDown( consoleState.visibleAmountOfLines );
		}

		return;
	}

	// ctrl-home = top of console
	if ( ( key == Key(K_HOME) || key == Key(K_KP_HOME) ) && keys[ Key(K_CTRL) ].down )
	{
		Con_JumpUp();
		return;
	}

	// ctrl-end = bottom of console
	if ( ( key == Key(K_END) || key == Key(K_KP_END) ) && keys[ Key(K_CTRL) ].down )
	{
		Con_ScrollToBottom();
		return;
	}

	// pass to the next editline routine
	Field_KeyDownEvent(g_consoleField, key);
}

//============================================================================

namespace Keyboard {

bool IsDown( Key key )
{
	if ( !key.IsValid() )
	{
		return false;
	}

	return keys[ key ].down;
}

bool AnyKeyDown() {
	return anykeydown > 0;
}

} // namespace Keyboard


/*
============
Key_SetKeyData
============
*/
static void Key_SetKeyData_f()
{
	if ( atoi( Cmd_Argv( 1 ) ) == plusCommand.check )
	{
		plusCommand.key = Keyboard::StringToKey( Cmd_Argv( 2 ) );
		plusCommand.time = atoi( Cmd_Argv( 3 ) );
		plusCommand.valid = true;
	}
	else
	{
		plusCommand.valid = false;
	}
}

Keyboard::Key Key_GetKeyNumber()
{
	return plusCommand.valid ? plusCommand.key : Keyboard::Key::NONE;
}

unsigned int Key_GetKeyTime()
{
	return plusCommand.valid ? plusCommand.time : 0;
}


/*
===================
CL_InitKeyCommands
===================
*/
void CL_InitKeyCommands()
{
	// register our functions
	Cmd_AddCommand( "setkeydata", Key_SetKeyData_f );
}


/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/

void CL_KeyEvent( const Keyboard::Key& key, bool down, unsigned time )
{
	using Keyboard::Key;
	bool onlybinds = false;

	if ( !key.IsValid() )
	{
		return;
	}

	if ( key.kind() == Key::Kind::KEYNUM ) {
		switch ( key.AsKeynum() ) {
		case K_KP_PGUP:
		case K_KP_EQUALS:
		case K_KP_5:
		case K_KP_LEFTARROW:
		case K_KP_UPARROW:
		case K_KP_RIGHTARROW:
		case K_KP_DOWNARROW:
		case K_KP_END:
		case K_KP_PGDN:
		case K_KP_INS:
		case K_KP_DEL:
		case K_KP_HOME:
			if ( IN_IsNumLockDown() )
			{
				onlybinds = true;
			}

			break;

		default:
			break;
		}
	}

	// update auto-repeat status and BUTTON_ANY status
	keys[ key ].down = down;

	if ( down )
	{
		keys[ key ].repeats++;

		if ( keys[ key ].repeats == 1 )
		{
			anykeydown++;
		}
	}
	else
	{
		keys[ key ].repeats = 0;
		anykeydown--;

		if ( anykeydown < 0 )
		{
			anykeydown = 0;
		}
	}

#ifdef MACOS_X
	if ( down && keys[ Key(K_COMMAND) ].down )
	{
		if ( key == Key::FromCharacter('f') )
		{
			Key_ClearStates();
			Cmd::BufferCommandText("toggle r_fullscreen; vid_restart");
			return;
		}
		else if ( key == Key::FromCharacter('q') )
		{
			Key_ClearStates();
			Cmd::BufferCommandText("quit");
			return;
		}
		else if ( key == Key(K_TAB) )
		{
			Key_ClearStates();
			Cmd::BufferCommandText("minimize");
			return;
		}
	}
#else
	if ( key == Key(K_ENTER) )
	{
		if ( down )
		{
			if ( keys[ Key(K_ALT) ].down )
			{
				Cvar_SetValue( "r_fullscreen", !Cvar_VariableIntegerValue( "r_fullscreen" ) );
				return;
			}
		}
	}

	if ( cl_altTab->integer && keys[ Key(K_ALT) ].down && key == Key(K_TAB) )
	{
		Key_ClearStates();
		Cmd::BufferCommandText("minimize");
		return;
	}
#endif

	// console key is hardcoded, so the user can never unbind it
	if ( key == Key(K_CONSOLE) || ( keys[ Key(K_SHIFT) ].down && key == Key(K_ESCAPE) ) )
	{
		if ( !down )
		{
			return;
		}

		Con_ToggleConsole_f();
		Key_ClearStates();
		return;
	}

	// escape is always handled special
	if ( key == Key(K_ESCAPE) && down )
	{
		if ( !( cls.keyCatchers & KEYCATCH_UI ) )
		{
			if ( cls.state == connstate_t::CA_ACTIVE )
			{
				// Arnout: on request
				if ( cls.keyCatchers & KEYCATCH_CONSOLE ) // get rid of the console
				{
					Con_ToggleConsole_f();
				}
				else
				{
					Cmd::BufferCommandText( "toggleMenu" );
				}
			}
			return;
		}

		cgvm.CGameKeyEvent(key, down);
		return;
	}

	// Don't do anything if libRocket menus have focus
	// Everything is handled by libRocket. Also we don't want
	// to run any binds (since they won't be found).
	if ( cls.keyCatchers & KEYCATCH_UI && !( cls.keyCatchers & KEYCATCH_CONSOLE ) )
	{
		cgvm.CGameKeyEvent(key, down);
		return;
	}

	//
	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).  These will be processed even in
	// console mode and menu mode, to keep the character from continuing
	// an action started before a mode switch.
	//
	if ( !down )
	{
		// Handle any +commands which were invoked on the corresponding key-down
		Cmd::BufferCommandText(Str::Format("keyup %d %s %u", plusCommand.check, Cmd::Escape(KeyToString(key)), time));

		return;
	}

	// distribute the key down event to the appropriate handler
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		if ( !onlybinds )
		{
			Console_Key( key );
		}
	}
	else if ( cls.state == connstate_t::CA_DISCONNECTED )
	{
		if ( !onlybinds )
		{
			Console_Key( key );
		}
	}
	else
	{
		// send the bound action
		auto kb = Keyboard::GetBinding( key, Keyboard::GetTeam(), true );

		if ( kb )
		{
			// down-only command
			Cmd::BufferCommandTextAfter(Str::Format("setkeydata %d %s %u\n%s", plusCommand.check, Cmd::Escape(KeyToString(key)), time, kb.value()), true);
			Cmd::BufferCommandTextAfter(va("setkeydata %d", plusCommand.check), true);
		}
	}
}

/*
===================
CL_CharEvent

Characters, already shifted/capslocked/etc.
===================
*/
void CL_CharEvent( int c )
{
	// filter out Apple control codes
#ifdef __APPLE__
	if ( (unsigned int)(c - 0xF700) < 0x200u )
	{
		return;
	}
#endif
	// distribute the key down event to the appropriate handler
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		Field_CharEvent(g_consoleField, c);
	}
	else if ( cls.state == connstate_t::CA_DISCONNECTED )
	{
		Field_CharEvent(g_consoleField, c);
	}

	cgvm.CGameTextInputEvent(c);
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates()
{
	anykeydown = 0;

	int oldKeyCatcher = Key_GetCatcher();
	Key_SetCatcher( 0 );

	for ( auto& kv: keys )
	{
		if ( kv.second.down )
		{
			CL_KeyEvent(kv.first, false, 0 );
		}

		kv.second.down = false;
		kv.second.repeats = 0;
	}

	plusCommand.check = rand();

	Key_SetCatcher( oldKeyCatcher );
}
