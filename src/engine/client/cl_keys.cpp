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
#include "qcommon/q_unicode.h"
#include "framework/CommandSystem.h"

/*

key up events are sent even if in console mode

*/

using Keyboard::Key;

#define CLIP(t) Math::Clamp( (t), 0, MAX_TEAMS - 1 )

Console::Field g_consoleField(INT_MAX);

bool key_overstrikeMode;
bool bindingsModified;

int      anykeydown;
std::unordered_map<Key, qkey_t, Key::hash> keys;

int      bindTeam = DEFAULT_BINDING;

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
		height = key_overstrikeMode ? SMALLCHAR_HEIGHT / (CONSOLE_FONT_VPADDING + 1) : 2;
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
	if (key.kind() == Key::Kind::KEYNUM) {
        switch (key.AsKeynum()) {
        case K_DEL:
            edit.DeleteNext();
            break;

        case K_BACKSPACE:
            edit.DeletePrev();
            break;

        case K_RIGHTARROW:
            if (keys[ K_CTRL ].down) {
                //TODO: Skip a full word
                edit.CursorRight();
            } else {
                edit.CursorRight();
            }
            break;

        case K_LEFTARROW:
            if (keys[ K_CTRL ].down) {
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
            if (keys[ K_SHIFT ].down) {
                Field_Paste(edit);
            } else {
                key_overstrikeMode = !key_overstrikeMode;
            }
            break;
        }
    } else if (key.kind() == Key::Kind::UNICODE_CHAR) {
        switch ((char) key.AsCharacter()) {
        case 'a':
            if (keys[ K_CTRL ].down) {
                edit.CursorStart();
            }
            break;
        case 'e':
            if (keys[ K_CTRL ].down) {
                edit.CursorEnd();
            }
            break;
        case 'v':
            if (keys[ K_CTRL ].down) {
                Field_Paste( edit );
            }
            break;
        case 'd':
            if (keys[ K_CTRL ].down) {
                edit.DeleteNext();
            }
            break;
        case 'c':
        case 'u':
            if (keys[ K_CTRL ].down) {
                edit.Clear();
            }
            break;
        case 'k':
            if (keys[ K_CTRL ].down) {
				edit.DeleteEnd();
            }
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

    // 'unprintable' on Mac - used for cursor keys, function keys etc.
    if ( (unsigned int)( c - 0xF700 ) < 0x200u )
    {
        return;
    }

    if (key_overstrikeMode) {
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
void Console_Key( Key key )
{
	// just return if any of the listed modifiers are pressed
	// - no point in passing on, since they Just Get In The Way
	if ( keys[ K_ALT     ].down || keys[ K_COMMAND ].down ||
			keys[ K_MODE    ].down || keys[ K_SUPER   ].down )
	{
		return;
	}

	// ctrl-L clears screen
	if (key == Key::FromCharacter('l') && keys[ K_CTRL ].down) {
		Cmd::BufferCommandText("clear");
		return;
	}

	// enter finishes the line
	if (key == K_ENTER or key == K_KP_ENTER) {

		//scroll lock state 1 or smaller will scroll down on own output
		if (con_scrollLock->integer <= 1) {
			consoleState.scrollLineIndex = consoleState.lines.size() - 1;
		}

		Log::Notice("]%s\n", Str::UTF32To8(g_consoleField.GetText()).c_str());

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

	if ( key == K_TAB )
	{
		g_consoleField.AutoComplete();
		return;
	}

	// command history (ctrl-p ctrl-n for unix style)

	//----(SA)  added some mousewheel functionality to the console
	if ( ( key == K_MWHEELUP && keys[ K_SHIFT ].down ) || ( key == K_UPARROW ) || ( key == K_KP_UPARROW ) ||
			( ( key == Key::FromCharacter('p') ) && keys[ K_CTRL ].down ) )
	{
		g_consoleField.HistoryPrev();
		return;
	}

	//----(SA)  added some mousewheel functionality to the console
	if ( ( key == K_MWHEELDOWN && keys[ K_SHIFT ].down ) || ( key == K_DOWNARROW ) || ( key == K_KP_DOWNARROW ) ||
			( ( key == Key::FromCharacter('n') ) && keys[ K_CTRL ].down ) )
	{
		g_consoleField.HistoryNext();
		return;
	}

	// console scrolling
	if ( key == K_PGUP || key == K_KP_PGUP )
	{
		Con_PageUp();
		return;
	}

	if ( key == K_PGDN || key == K_KP_PGDN )
	{
		Con_PageDown();
		return;
	}

	if ( key == K_MWHEELUP ) //----(SA) added some mousewheel functionality to the console
	{
		Con_PageUp();

		if ( keys[ K_CTRL ].down ) // hold <ctrl> to accelerate scrolling
		{
			Con_ScrollUp( consoleState.visibleAmountOfLines );
		}

		return;
	}

	if ( key == K_MWHEELDOWN ) //----(SA) added some mousewheel functionality to the console
	{
		Con_PageDown();

		if ( keys[ K_CTRL ].down ) // hold <ctrl> to accelerate scrolling
		{
			Con_ScrollDown( consoleState.visibleAmountOfLines );
		}

		return;
	}

	// ctrl-home = top of console
	if ( ( key == K_HOME || key == K_KP_HOME ) && keys[ K_CTRL ].down )
	{
		Con_JumpUp();
		return;
	}

	// ctrl-end = bottom of console
	if ( ( key == K_END || key == K_KP_END ) && keys[ K_CTRL ].down )
	{
		Con_ScrollToBottom();
		return;
	}

	// pass to the next editline routine
	Field_KeyDownEvent(g_consoleField, key);
}

//============================================================================

/*
===================
Key_IsDown
===================
*/
bool Key_IsDown( Key key )
{
	if ( !key.IsValid() )
	{
		return false;
	}

	return keys[ key ].down;
}



/*
===================
Key_GetTeam
Assumes 'three' teams: spectators, aliens, humans
===================
*/
static const char *const teamName[] = { "default", "aliens", "humans", "others" };

int Key_GetTeam( const char *arg, const char *cmd )
{
	static const struct {
		char team;
		char label[11];
	} labels[] = {
		{ 0, "spectators" },
		{ 0, "default" },
		{ 1, "aliens" },
		{ 2, "humans" }
	};
	int t, l;

	if ( !*arg ) // empty string
	{
		goto fail;
	}

	for ( t = 0; arg[ t ]; ++t )
	{
		if ( !Str::cisdigit( arg[ t ] ) )
		{
			break;
		}
	}

	if ( !arg[ t ] )
	{
		t = atoi( arg );

		if ( t != CLIP( t ) )
		{
			Log::Notice( "^3%s:^7 %d is not a valid team number\n", cmd, t );
			return -1;
		}

		return t;
	}

	l = strlen( arg );

	for ( unsigned t = 0; t < ARRAY_LEN( labels ); ++t )
	{
		// matching initial substring
		if ( !Q_strnicmp( arg, labels[ t ].label, l ) )
		{
			return labels[ t ].team;
		}
	}

fail:
	Log::Notice( "^3%s:^7 '%s^7' is not a valid team name\n", cmd, arg );
	return -1;
}

/*
===================
Key_SetBinding

team == -1 clears all bindings for the key, then sets the spec/global binding
===================
*/
void Key_SetBinding( Key key, int team, const char *binding )
{
	char *lcbinding; // fretn - make a copy of our binding lowercase
	// so name toggle scripts work again: bind x name BzZIfretn?
	// resulted into bzzifretn?

	if ( !key.IsBindable() )
	{
		return;
	}

	// free old bindings
	if ( team == -1 )
	{
		// just the team-specific ones here
		for ( team = MAX_TEAMS - 1; team; --team )
		{
			if ( keys[ key ].binding[ team ] )
			{
				Z_Free( keys[ key ].binding[ team ] );
				keys[ key ].binding[ team ] = nullptr;
			}
		}
		// team == 0...
	}

	team = CLIP( team );

	if ( keys[ key ].binding[ team ] )
	{
		Z_Free( keys[ key ].binding[ team ] );
	}

	// set the new binding, if not null/empty
	if ( binding && binding[ 0 ] )
	{
		// allocate memory for new binding
		keys[ key ].binding[ team ] = CopyString( binding );
		lcbinding = CopyString( binding );
		Q_strlwr( lcbinding );  // saves doing it on all the generateHashValues in Key_GetBindingByString
		Z_Free( lcbinding );
	}
	else
	{
		keys[ key ].binding[ team ] = nullptr;
	}

	bindingsModified = true;
}

/*
===================
Key_GetBinding

-ve team no. = don't return the default binding
===================
*/
const char *Key_GetBinding( Key key, int team )
{
	const char *bind;

	if ( !key.IsBindable() )
	{
		return nullptr;
	}

	if ( team <= 0 )
	{
		return keys[ key ].binding[ CLIP( -team ) ];
	}

	bind = keys[ key ].binding[ CLIP( team ) ];
	return bind ? bind : keys[ key ].binding[ 0 ];
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f()
{
	int b = Cmd_Argc();
	int team = -1;

	if ( b < 2 || b > 3 )
	{
		Cmd_PrintUsage("[<team>] <key>", "remove commands from a key");
		return;
	}

	if ( b > 2 )
	{
		team = Key_GetTeam( Cmd_Argv( 1 ), "unbind" );

		if ( team < 0 )
		{
			return;
		}
	}

	Key key = Key_StringToKeynum( Cmd_Argv( b - 1 ) );

	if ( !key.IsBindable() )
	{
		Log::Notice( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ) );
		return;
	}

	Key_SetBinding( key, team, nullptr );
}

/*
===================
Key_Unbindall_f
===================
*/
void Key_Unbindall_f()
{
	for (auto& kv : keys)
	{
		Key_SetBinding( kv.first, -1, nullptr );
	}
}

/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f()
{
	int        c;
	const char *key;
	const char *cmd = nullptr;
	int        team = -1;

	int teambind = !Q_stricmp( Cmd_Argv( 0 ), "teambind" );

	c = Cmd_Argc();

	if ( c < 2 + teambind )
	{
		Cmd_PrintUsage( teambind ? "<team> <key> [<command>]" : "<key> [<command>]", "attach a command to a key");
		return;
	}
	else if ( c > 2 + teambind )
	{
		cmd = Cmd_Argv( 2 + teambind );
	}

	if ( teambind )
	{
		team = Key_GetTeam( Cmd_Argv( 1 ), teambind ? "teambind" : "bind" );

		if ( team < 0 )
		{
			return;
		}
	}

	key = Cmd_Argv( 1 + teambind );
	Key b = Key_StringToKeynum( key );

	if ( !b.IsBindable() )
	{
		Log::Notice( "\"%s\" isn't a valid key\n", key );
		return;
	}

	key = Key_KeynumToString( b );

	if ( !cmd )
	{
		if ( teambind )
		{
			if ( keys[ b ].binding[ team ] )
			{
				Log::Notice( "\"%s\"[%s] = %s\n", key, teamName[ team ], Cmd_QuoteString( keys[ b ].binding[ team ] ) );
			}
			else
			{
				Log::Notice( "\"%s\"[%s] is not bound\n", key, teamName[ team ] );
			}
		}
		else
		{
			bool bound = false;
			int      i;

			for ( i = 0; i < MAX_TEAMS; ++i )
			{
				if ( keys[ b ].binding[ i ] )
				{
					Log::Notice( "\"%s\"[%s] = %s\n", key, teamName[ i ], Cmd_QuoteString( keys[ b ].binding[ i ] ) );
					bound = true;
				}
			}

			if ( !bound )
			{
				Log::Notice( "\"%s\" is not bound\n", key );
			}
		}


		return;
	}


	if ( c <= 3 + teambind )
	{
		Key_SetBinding( b, team, cmd );
	}
	else
	{
		// set to 3rd arg onwards (4th if team binding)
		Key_SetBinding( b, team, Cmd_ArgsFrom( 2 + teambind ) );
	}
}

/*
===================
Key_EditBind_f
===================
*/
void Key_EditBind_f()
{
	std::u32string buf;
	int            b;
	int            team = -1;

	b = Cmd_Argc();

	if ( b < 2 || b > 3 )
	{
		Cmd_PrintUsage("[<team>] <key>", nullptr);
		return;
	}

	if ( b > 2 )
	{
		team = Key_GetTeam( Cmd_Argv( 1 ), "editbind" );

		if ( team < 0 )
		{
			return;
		}
	}

	const char *key = Cmd_Argv( b - 1 );
	Key k = Key_StringToKeynum( key );

	if ( !k.IsBindable() )
	{
		Log::Notice( "\"%s\" isn't a valid key\n", key );
		return;
	}

	if ( team >= 0 )
	{
		buf = Str::UTF8To32("/teambind ");
		buf += Str::UTF8To32( teamName[ team ] );
		buf += Str::UTF8To32(" ");
	}
	else
	{
		buf = Str::UTF8To32("/bind ");
	}

	buf += Str::UTF8To32( Key_KeynumToString( k ) );
	buf += Str::UTF8To32(" ");

	const char *binding = Key_GetBinding( k, -team );
	if ( binding )
	{
		buf += Str::UTF8To32( Cmd::Escape( std::string( binding ) ) );
	}

	// FIXME: use text console if that's where the editbind command was entered
	Con_OpenConsole_f();
	g_consoleField.SetText( buf );
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings( fileHandle_t f )
{
	int team;

	FS_Printf( f,"%s", "unbindall\n" );

	for (const auto& kv: keys)
	{
		if ( kv.second.binding[ 0 ] && kv.second.binding[ 0 ][ 0 ] )
		{
			FS_Printf( f, "bind       %s %s\n", Key_KeynumToString( kv.first ), 
                       Cmd_QuoteString( kv.second.binding[ 0 ] ) );
		}

		for ( team = 1; team < MAX_TEAMS; ++team )
		{
			if ( kv.second.binding[ team ] && kv.second.binding[ team ][ 0 ] )
			{
				FS_Printf( f, "teambind %d %s %s\n", team, Key_KeynumToString( kv.first ),
                           Cmd_QuoteString( kv.second.binding[ team ] ) );
			}
		}
	}
}

/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f()
{
	int team;

	for (auto& kv: keys)
	{
		bool teamSpecific = false;

		for ( team = 1; team < MAX_TEAMS; ++team )
		{
			if ( kv.second.binding[ team ] && kv.second.binding[ team ][ 0 ] )
			{
				teamSpecific = true;
				break;
			}
		}

		if ( !teamSpecific )
		{
			if ( kv.second.binding[ 0 ] && kv.second.binding[ 0 ][ 0 ] )
			{
				Log::Notice( "%s = %s\n", Key_KeynumToString( kv.first ), kv.second.binding[ 0 ] );
			}
		}
		else
		{
			for ( team = 0; team < MAX_TEAMS; ++team )
			{
				if ( kv.second.binding[ team ] && kv.second.binding[ team ][ 0 ] )
				{
					Log::Notice( "%s[%s] = %s\n", Key_KeynumToString( kv.first ), teamName[ team ], kv.second.binding[ team ] );
				}
			}
		}
	}
}

/*
============
Key_SetKeyData
============
*/
void Key_SetKeyData_f()
{
	if ( atoi( Cmd_Argv( 1 ) ) == plusCommand.check )
	{
		plusCommand.key = Key_StringToKeynum( Cmd_Argv( 2 ) );
		plusCommand.time = atoi( Cmd_Argv( 3 ) );
		plusCommand.valid = true;
	}
	else
	{
		plusCommand.valid = false;
	}
}

Key Key_GetKeyNumber()
{
	return plusCommand.valid ? plusCommand.key : Key::NONE;
}

unsigned int Key_GetKeyTime()
{
	return plusCommand.valid ? plusCommand.time : 0;
}

/*
===============
FindMatches

===============
*/
//TODO (kangz) rework the bind commands and their completion
static void FindMatches( const char *s )
{
    Cmd_OnCompleteMatch(s);
}

static void Field_TeamnameCompletion( void ( *callback )( const char *s ), int flags )
{
	if ( flags & FIELD_TEAM_SPECTATORS )
	{
		callback( "spectators" );
	}

	if ( flags & FIELD_TEAM_DEFAULT )
	{
		callback( "default" );
	}

	callback( "humans" );
	callback( "aliens" );
}

/*
===============
Field_CompleteKeyname
===============
*/
void Field_CompleteKeyname( int flags )
{
	if ( flags & FIELD_TEAM )
	{
		Field_TeamnameCompletion( FindMatches, flags );
	}

	Key_KeynameCompletion( FindMatches );
}

/*
===============
Field_CompleteTeamname
===============
*/
void Field_CompleteTeamname( int flags )
{
	Field_TeamnameCompletion( FindMatches, flags );
}



/*
====================
Key_CompleteUnbind
====================
*/
static void Key_CompleteUnbind( char *args, int argNum )
{
	if ( argNum < 4 )
	{
		// Skip "unbind "
		char *p = Com_SkipTokens( args, 1, " " );

		if ( p > args )
		{
			Field_CompleteKeyname( argNum > 2 ? 0 : FIELD_TEAM | FIELD_TEAM_SPECTATORS | FIELD_TEAM_DEFAULT );
		}
	}
}

/*
====================
Key_CompleteBind
Key_CompleteTeambind
====================
*/
static void Key_CompleteBind_Internal( char *args, int argNum, int nameArg )
{
	// assumption: nameArg is 2 or 3
	char *p;

	if ( argNum == nameArg )
	{
		// Skip "bind "
		p = Com_SkipTokens( args, nameArg - 1, " " );

		if ( p > args )
		{
			Field_CompleteKeyname( 0 );
		}
	}
	else if ( argNum > nameArg )
	{
		Cmd::Args arg(args);
		Cmd::CompletionResult res = Cmd::CompleteArgument(Cmd::Args(arg.EscapedArgs(nameArg)), argNum - nameArg - 1);
		for (const auto& candidate : res)
		{
			FindMatches(candidate.first.c_str());
		}
	}
}

void Key_CompleteBind( char *args, int argNum )
{
	Key_CompleteBind_Internal( args, argNum, 2 );
}

void Key_CompleteTeambind( char *args, int argNum )
{
	if ( argNum == 2 )
	{
		Field_CompleteTeamname( FIELD_TEAM_SPECTATORS | FIELD_TEAM_DEFAULT );
	}
	else
	{
		Key_CompleteBind_Internal( args, argNum, 3 );
	}
}

static void Key_CompleteEditbind( char *, int argNum )
{
	if ( argNum < 4 )
	{
		Field_CompleteKeyname( argNum > 2 ? 0 : FIELD_TEAM | FIELD_TEAM_SPECTATORS | FIELD_TEAM_DEFAULT );
	}
}

/*
===============
Helper functions for Cmd_If_f & Cmd_ModCase_f
===============
*/
static const struct
{
	char name[ 8 ];
	unsigned short count;
	unsigned short bit;
	keyNum_t index;
} modifierKeys[] =
{
	{ "shift", 5, 1, K_SHIFT },
	{ "ctrl", 4, 2, K_CTRL },
	{ "alt", 3, 4, K_ALT },
	{ "command", 7, 8, K_COMMAND },
	{ "cmd", 3, 8, K_COMMAND },
	{ "mode", 4, 16, K_MODE },
	{ "super", 5, 32, K_SUPER },
	{ "compose", 6, 64, K_COMPOSE },
	{ "menu", 7, 128, K_MENU },
	{ "" }
};
// Following is no. of bits required for modifiers in the above list
// (it doesn't reflect the array length)
static const int NUM_RECOGNISED_MODIFIERS = 8;

struct modifierMask_t
{
	uint16_t down, up;
	int bits;
};

static modifierMask_t getModifierMask( const char *mods )
{
	int i;
	modifierMask_t mask;
	const char *ptr;
	static const modifierMask_t none = {0, 0, 0};

	mask = none;

	--mods;

	while ( *++mods == ' ' ) { /* skip leading spaces */; }

	ptr = mods;

	while ( *ptr )
	{
		int invert = ( *ptr == '!' );

		if ( invert )
		{
			++ptr;
		}

		for ( i = 0; modifierKeys[ i ].bit; ++i )
		{
			// is it this modifier?
			if ( !Q_strnicmp( ptr, modifierKeys[ i ].name, modifierKeys[ i ].count )
					&& ( ptr[ modifierKeys[ i ].count ] == ' ' ||
						ptr[ modifierKeys[ i ].count ] == ',' ||
						ptr[ modifierKeys[ i ].count ] == 0 ) )
			{
				if ( invert )
				{
					mask.up |= modifierKeys[ i ].bit;
				}
				else
				{
					mask.down |= modifierKeys[ i ].bit;
				}

				if ( ( mask.down & mask.up ) & modifierKeys[ i ].bit )
				{
					Log::Notice( "can't have %s both pressed and not pressed\n", modifierKeys[ i ].name );
					return none;
				}

				// right, parsed a word - skip it, maybe a comma, and any spaces
				ptr += modifierKeys[ i ].count - 1;

				while ( *++ptr == ' ' ) { /**/; }

				if ( *ptr == ',' )
				{
					while ( *++ptr == ' ' ) { /**/; }
				}

				// ready to parse the next one
				break;
			}
		}

		if ( !modifierKeys[ i ].bit )
		{
			Log::Notice( "unknown modifier key name in \"%s\"\n", mods );
			return none;
		}
	}

	for ( i = 0; i < NUM_RECOGNISED_MODIFIERS; ++i )
	{
		if ( mask.up & ( 1 << i ) )
		{
			++mask.bits;
		}

		if ( mask.down & ( 1 << i ) )
		{
			++mask.bits;
		}
	}

	return mask;
}

static int checkKeysDown( modifierMask_t mask )
{
	int i;

	for ( i = 0; modifierKeys[ i ].bit; ++i )
	{
		if ( ( mask.down & modifierKeys[ i ].bit ) && keys[ modifierKeys[ i ].index ].down == 0 )
		{
			return 0; // should be pressed, isn't pressed
		}

		if ( ( mask.up & modifierKeys[ i ].bit ) && keys[ modifierKeys[ i ].index ].down )
		{
			return 0; // should not be pressed, is pressed
		}
	}

	return 1; // all (not) pressed as requested
}

/*
===============
Key_ModCase_f

Takes a sequence of modifier/command pairs
Executes the command for the first matching modifier set

===============
*/
void Key_ModCase_f()
{
	int argc = Cmd_Argc();
	int index = 0;
	int max = 0;
	int count = ( argc - 1 ) / 2; // round down :-)
	const char *v;

	int mods[ 1 << NUM_RECOGNISED_MODIFIERS ];
	// want 'modifierMask_t mods[argc / 2 - 1];' (variable array, C99)
	// but MSVC apparently doesn't like that

	if ( argc < 3 )
	{
		Cmd_PrintUsage( "<modifiers> <command> [<modifiers> <command>] … [<command>]", nullptr );
		return;
	}

	while ( index < count )
	{
		modifierMask_t mask = getModifierMask( Cmd_Argv( 2 * index + 1 ) );

		if ( mask.bits == 0 )
		{
			return; // parse failure (reported) - abort
		}

		mods[ index ] = checkKeysDown( mask ) ? mask.bits : 0;

		if ( max < mods[ index ] )
		{
			max = mods[ index ];
		}

		++index;
	}

	// If we have a tail command, use it as default
	v = ( argc & 1 ) ? nullptr : Cmd_Argv( argc - 1 );

	// Search for a suitable command to execute.
	// Search is done as if the commands are sorted by modifier count
	// (descending) then parameter index no. (ascending).
	for ( ; max > 0; --max )
	{
		int i;

		for ( i = 0; i < index; ++i )
		{
			if ( mods[ i ] == max )
			{
				v = Cmd_Argv( 2 * i + 2 );
				goto found;
			}
		}
	}

found:

	if ( v && *v )
	{
		if ( *v == '/' || *v == '\\' )
		{
			Cmd::BufferCommandTextAfter(va("%s\n", v + 1), true);
		}
		else
		{
			Cmd::BufferCommandTextAfter(va("vstr %s\n", v), true);
		}
	}
}

/*
===================
CL_InitKeyCommands
===================
*/
void CL_InitKeyCommands()
{
	// register our functions
	Cmd_AddCommand( "bind", Key_Bind_f );
	Cmd_SetCommandCompletionFunc( "bind", Key_CompleteBind );
	Cmd_AddCommand( "teambind", Key_Bind_f );
	Cmd_SetCommandCompletionFunc( "teambind", Key_CompleteTeambind );
	Cmd_AddCommand( "unbind", Key_Unbind_f );
	Cmd_SetCommandCompletionFunc( "unbind", Key_CompleteUnbind );
	Cmd_AddCommand( "unbindall", Key_Unbindall_f );
	Cmd_AddCommand( "bindlist", Key_Bindlist_f );
	Cmd_AddCommand( "editbind", Key_EditBind_f );
	Cmd_AddCommand( "modcase", Key_ModCase_f );
	Cmd_SetCommandCompletionFunc( "editbind", Key_CompleteEditbind );
	Cmd_AddCommand( "setkeydata", Key_SetKeyData_f );
}

void CL_ClearKeyBinding()
{
	int team;

	for ( team = 0; team < MAX_TEAMS; team++ )
	{
		for ( auto& kv: keys )
		{
			if ( kv.second.binding[ team ] )
			{
				Z_Free( kv.second.binding[ team ] );
				kv.second.binding[ team ] = nullptr;
			}
		}
	}
}

/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
//static consoleCount = 0;
// fretn
bool consoleButtonWasPressed = false;

void CL_KeyEvent( const Key& key, bool down, unsigned time )
{
	char     *kb;
	bool bypassMenu = false; // NERVE - SMF
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
	if ( down && keys[ K_COMMAND ].down )
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
		else if ( key == K_TAB )
		{
			Key_ClearStates();
			Cmd::BufferCommandText("minimize");
			return;
		}
	}
#else
	if ( key == K_ENTER )
	{
		if ( down )
		{
			if ( keys[ K_ALT ].down )
			{
				Cvar_SetValue( "r_fullscreen", !Cvar_VariableIntegerValue( "r_fullscreen" ) );
				return;
			}
		}
	}

	if ( cl_altTab->integer && keys[ K_ALT ].down && key == K_TAB )
	{
		Key_ClearStates();
		Cmd::BufferCommandText("minimize");
		return;
	}
#endif

	// console key is hardcoded, so the user can never unbind it
	if ( key == K_CONSOLE || ( keys[ K_SHIFT ].down && key == K_ESCAPE ) )
	{
		if ( !down )
		{
			return;
		}

		Con_ToggleConsole_f();
		Key_ClearStates();
		return;
	}

	// most keys during demo playback will bring up the menu, but non-ascii

	// escape is always handled special
	if ( key == K_ESCAPE && down )
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
			else
			{
				CL_Disconnect_f();
				Audio::StopAllSounds();
			}

			return;
		}

		cgvm.CGameKeyEvent(key.AsLegacyInt(), down);
		return;
	}

	// Don't do anything if libRocket menus have focus
	// Everything is handled by libRocket. Also we don't want
	// to run any binds (since they won't be found).
	if ( cls.keyCatchers & KEYCATCH_UI && !( cls.keyCatchers & KEYCATCH_CONSOLE ) )
	{
        int intKey = key.AsLegacyInt();
        if (intKey > 0) {
		    cgvm.CGameKeyEvent(intKey, down);
        }
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
		Cmd::BufferCommandText(Str::Format("keyup %d %s %u", plusCommand.check, Key_KeynumToString(key), time));

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
		kb = keys[ key ].binding[ bindTeam ]
		   ? keys[ key ].binding[ bindTeam ] // prefer the team bind
		   : keys[ key ].binding[ 0 ];       // default to global

		if ( kb )
		{
			// down-only command
			Cmd::BufferCommandTextAfter(Str::Format("setkeydata %d %s %u\n%s", plusCommand.check, Key_KeynumToString(key), time, kb), true);
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
static int CL_UTF8_unpack( int c )
{
	const char *str = Q_UTF8_Unstore( c );
	int chr = Q_UTF8_CodePoint( str );

	// filter out Apple control codes
	return (unsigned int)( chr - 0xF700 ) < 0x200u ? 0 : chr;
}

void CL_CharEvent( int c )
{
	// the console key should never be used as a char
	// ydnar: added uk equivalent of shift+`
	// the RIGHT way to do this would be to have certain keys disable the equivalent SE_CHAR event

	// fretn - this should be fixed in Com_EventLoop
	// but I can't be arsed to leave this as is

	// distribute the key down event to the appropriate handler
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		Field_CharEvent(g_consoleField, CL_UTF8_unpack(c));
	}
	else if ( cls.state == connstate_t::CA_DISCONNECTED )
	{
		Field_CharEvent(g_consoleField, CL_UTF8_unpack(c));
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

		kv.second.down = 0;
		kv.second.repeats = 0;
	}

	plusCommand.check = rand();

	Key_SetCatcher( oldKeyCatcher );
}

/*
===================
Key_SetTeam
===================
*/
void Key_SetTeam( int newTeam )
{
	if ( newTeam < 0 || newTeam >= MAX_TEAMS )
	{
		newTeam = DEFAULT_BINDING;
	}

	if ( bindTeam != newTeam )
	{
		Log::Debug( "%sSetting binding team index to %d",
			Color::ToString( Color::Green ),
			newTeam );
	}

	bindTeam = newTeam;
}
