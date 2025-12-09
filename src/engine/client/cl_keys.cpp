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

	// return if it's a keypad key but numlock is on
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
			if ( IN_IsNumLockOn() )
			{
				return;
			}

			break;

		default:
			break;
		}
	}

	// escape closes the console
	if ( key == Key(K_ESCAPE) )
	{
		if ( consoleState.isOpened )
		{
			Con_ToggleConsole_f();
		}

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
	if ( Cmd_Argc() == 4 && atoi( Cmd_Argv( 1 ) ) == plusCommand.check )
	{
		plusCommand.key = Keyboard::StringToKey( Cmd_Argv( 2 ) );
		plusCommand.time = atoi( Cmd_Argv( 3 ) );
	}
	else
	{
		plusCommand.key = Keyboard::Key::NONE;
		plusCommand.time = 0;
	}
}

Keyboard::Key Key_GetKeyNumber()
{
	return plusCommand.key;
}

unsigned int Key_GetKeyTime()
{
	return plusCommand.time;
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


void CL_ConsoleKeyEvent()
{
	Con_ToggleConsole_f();
	Key_ClearStates();
}

// keyboard shortcuts hard-coded into the engine which are always active regardless of key catchers
static bool DetectBuiltInShortcut( Keyboard::Key key )
{
	using Keyboard::Key;

#ifdef MACOS_X
	if ( keys[ Key(K_COMMAND) ].down )
	{
		if ( key == Key::FromCharacter('f') )
		{
			Key_ClearStates();
			r_fullscreen.Set( !r_fullscreen.Get() );
			return true;
		}
		else if ( key == Key::FromCharacter('q') )
		{
			Key_ClearStates();
			Cmd::BufferCommandText("quit");
			return true;
		}
		else if ( key == Key(K_TAB) )
		{
			Key_ClearStates();
			Cmd::BufferCommandText("minimize");
			return true;
		}
	}
#else
	if ( key == Key(K_ENTER) && keys[ Key(K_ALT) ].down )
	{
		r_fullscreen.Set( !r_fullscreen.Get() );
		return true;
	}

	// When not in full-screen mode, the OS should intercept this first
	if ( cl_altTab->integer && keys[ Key(K_ALT) ].down && key == Key(K_TAB) )
	{
		Key_ClearStates();
		Cmd::BufferCommandText("minimize");
		return true;
	}
#endif

	// console key combination is hardcoded, so the user can never unbind it
	if ( keys[ Key(K_SHIFT) ].down && key == Key(K_ESCAPE) )
	{
		CL_ConsoleKeyEvent();
		return true;
	}

	return false;
}

// non-repeat key down
// This must handle both (if applicable) Keyboard::Key interperations together in order to
// properly allow the UI to decide to consume a key press, preventing it from activating
// binds. Say the user selects "3" in a menu. The UI probably used char:3 but the bind
// probably used hw:3. So it would just do both things if you processed the Key's separately.
void CL_KeyDownEvent( const Keyboard::Key& key1, const Keyboard::Key& key2, unsigned time )
{
	using Keyboard::Key;

	for ( Key key : {key1, key2} )
	{
		if ( key.IsValid() && !keys[ key ].down )
		{
			keys[ key ].down = true;
			anykeydown++; // update BUTTON_ANY status
		}
	}

	if ( DetectBuiltInShortcut( key1 ) || DetectBuiltInShortcut( key2 ) )
	{
		return;
	}

	// When the console is open, binds do not act and the UI can't receive input.
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		Console_Key( key1 );
		Console_Key( key2 );
		return;
	}

	bool uiConsumed = cls.keyCatchers & KEYCATCH_UI_KEY;
	uiConsumed |= cgvm.CGameKeyDownEvent( key1, false );
	uiConsumed |= cgvm.CGameKeyDownEvent( key2, false ); // non-short-circuiting!

	// do binds only if KEYCATCH_UI_KEY is off and also the UI didn't indicate that it
	// consumed the key
	if ( uiConsumed )
	{
		return;
	}

	if ( cls.state == connstate_t::CA_DISCONNECTED )
	{
		return;
	}

	for ( Key key : {key1, key2} )
	{
		// send the bound action
		auto kb = Keyboard::GetBinding( key, Keyboard::GetTeam(), true );

		if ( kb )
		{
			// The first line of this command sets a global variable (plusCommand) saying what key
			// was pressed and when, needed for button commands (e.g. +forward). Second line is the
			// bound command.
			Cmd::BufferCommandText(Str::Format("setkeydata %d %s %u\n%s", plusCommand.check, Cmd::Escape(KeyToString(key)), time, kb.value()), true);

			// Afterwards, clear the aforementioned global variable.
			Cmd::BufferCommandText("setkeydata");
		}
	}
}

// key down event from pressing and holding a key
// Only does a subset of the things that an initial key press does.
void CL_KeyRepeatEvent( const Keyboard::Key& key )
{
	if ( !key.IsValid() )
	{
		return;
	}

	// skip built-in shortcuts which mostly do something drastic like changing to another window
	// also don't set keys[ key ].down (if Key_ClearStates() was called, keep it clear until pressed anew)

	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		// stuff like PageDown can repeat
		Console_Key( key );
	}
	else
	{
		cgvm.CGameKeyDownEvent( key, true );
	}

	// repeat events don't trigger binds
	// sad... I can't bind "buy gren; itemact gren" and hold down a key
	// to spam nades like I did in Tremulous
}

void CL_KeyUpEvent( const Keyboard::Key& key, unsigned time )
{
	if ( !key.IsValid() )
	{
		return;
	}

	if ( keys[ key ].down )
	{
		keys[ key ].down = false;
		anykeydown--; // update BUTTON_ANY status
	}

	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).
	//
	// Handle any +commands which were invoked on the corresponding key-down
	Cmd::BufferCommandText(
		Str::Format( "keyup %d %s %u", plusCommand.check, Cmd::Escape( KeyToString( key ) ), time ) );

	// Send key up event to UI unless console is active
	if ( !( cls.keyCatchers & KEYCATCH_CONSOLE ) )
	{
		cgvm.CGameKeyUpEvent( key );
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

Sets all keys to the "not pressed" state. The purpose is to stop all key binds from executing.

Does not send key up events to the UI.
===================
*/
void Key_ClearStates()
{
	anykeydown = 0;

	for ( auto& kv: keys )
	{
		if ( kv.second.down )
		{
			kv.second.down = false;
			Cmd::BufferCommandText( Str::Format(
				"keyup %d %s 0", plusCommand.check, Cmd::Escape( KeyToString( kv.first ) ) ) );
		}
	}

	plusCommand.check = rand();
}
