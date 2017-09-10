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

#include "keys.h"

#include "client.h"
#include "key_identification.h"
#include "framework/CommandSystem.h"


using Keyboard::Key;

static int ClipTeamNumber(int team)
{
	return Math::Clamp(team, 0, MAX_TEAMS - 1);
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
					Log::Notice( "can't have %s both pressed and not pressed", modifierKeys[ i ].name );
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
			Log::Notice( "unknown modifier key name in \"%s\"", mods );
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
		if ( ( mask.down & modifierKeys[ i ].bit ) && keys[ Keyboard::Key(modifierKeys[ i ].index) ].down == 0 )
		{
			return 0; // should be pressed, isn't pressed
		}

		if ( ( mask.up & modifierKeys[ i ].bit ) && keys[ Keyboard::Key(modifierKeys[ i ].index) ].down )
		{
			return 0; // should not be pressed, is pressed
		}
	}

	return 1; // all (not) pressed as requested
}



void CL_ClearKeyBinding()
{
	int team;

	for ( team = 0; team < MAX_TEAMS; team++ )
	{
		for ( auto& kv: keys )
		{
			kv.second.binding[ team ] = {};
		}
	}
}

namespace Keyboard {

static int bindTeam = DEFAULT_BINDING;

void SetTeam( int newTeam )
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

int GetTeam()
{
	return bindTeam;
}

// Sets a key binding, or clears it given an empty string.
// team == -1 clears all bindings for the key, then sets the spec/global binding
void SetBinding(Key key, int team, std::string binding)
{
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
			keys[ key ].binding[ team ] = {};
		}
		// team == 0...
	}

	team = ClipTeamNumber( team );

	// set the new binding, if not null/empty
	if ( !binding.empty() )
	{
		keys[ key ].binding[ team ] = std::move(binding);
	}
	else
	{
		keys[ key ].binding[ team ] = Util::nullopt;
	}

	bindingsModified = true;
}

// -ve team no. = don't return the default binding
Util::optional<std::string> GetBinding(Key key, int team)
{
	if ( !key.IsBindable() )
	{
		return {};
	}
	auto it = keys.find(key);
	if (it == keys.end()) {
		return {};
	}

	if ( team <= 0 )
	{
		return it->second.binding[ ClipTeamNumber( -team ) ];
	} else
	{
		auto bind = it->second.binding[ ClipTeamNumber( team ) ];
		if (!bind) {
			bind = it->second.binding[ 0 ];
		}
		return bind;
	}
}


void WriteBindings( fileHandle_t f )
{
	FS_Printf( f,"%s", "unbindall\n" );

	std::vector<std::string> lines;
	for (const auto& kv: keys)
	{
		if ( kv.second.binding[ 0 ]  )
		{
			lines.push_back( Str::Format( "bind       %s %s\n",
			                              Cmd::Escape( KeyToString( kv.first ) ),
			                              Cmd::Escape( kv.second.binding[ 0 ].value() ) ) );
		}

		for ( int team = 1; team < MAX_TEAMS; ++team )
		{
			if ( kv.second.binding[ team ] )
			{
				lines.push_back( Str::Format( "teambind %d %s %s\n", team, Cmd::Escape( KeyToString( kv.first ) ),
				    Cmd::Escape( kv.second.binding[ team ].value() ) ) );
			}
		}
	}
	std::sort( lines.begin(), lines.end() );
	for (const auto& line: lines)
	{
		FS_Printf( f, "%s", line.c_str() );
	}
}

namespace { // Key binding commands

// Assumes 'three' teams: spectators, aliens, humans
static const char *const teamName[] = { "default", "aliens", "humans", "others" };
int GetTeam(Str::StringRef arg)
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

	if ( arg.empty() )
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
		t = atoi( arg.c_str() );

		if ( t != ClipTeamNumber( t ) )
		{
			return -1;
		}

		return t;
	}

	l = arg.size();

	for ( unsigned t = 0; t < ARRAY_LEN( labels ); ++t )
	{
		// matching initial substring
		if ( !Q_strnicmp( arg.c_str(), labels[ t ].label, l ) )
		{
			return labels[ t ].team;
		}
	}

fail:
	return -1;
}

Cmd::CompletionResult CompleteEmbeddedCommand(const Cmd::Args& args, int startIndex, int completeIndex) {
	// TODO: make not O(n^2)?
	Cmd::Args trailingArgs(std::vector<std::string>(args.begin() + startIndex, args.end()));
	return Cmd::CompleteArgument(trailingArgs, completeIndex - startIndex);
}

void CompleteTeamName(Str::StringRef prefix, Cmd::CompletionResult& completions)
{
	Cmd::AddToCompletion(completions, prefix, {{"spectators", ""}, {"default", ""}, {"humans", ""}, {"aliens", ""}});
}


void CompleteTeamAndKey(int argNum, const Cmd::Args& args, Str::StringRef prefix, Cmd::CompletionResult& res) {
	bool hasTeam = argNum > 1 && GetTeam(args.Argv(1)) >= 0;
	if (argNum == 1) {
		CompleteTeamName(prefix, res);
	}
	if (argNum == 1 + +hasTeam) {
		CompleteKeyName(res, prefix);
	}
}

std::string InvalidKeyMessage(Str::StringRef name) {
	return Str::Format("\"%s\" is not a recognized key", name);
}
std::string InvalidTeamMessage(Str::StringRef name) {
	return Str::Format("\"%s\" is not a valid team", name);
}

class BindListCmd: public Cmd::StaticCmd
{
public:
	BindListCmd():
		StaticCmd("bindlist", Cmd::KEY_BINDING, "Lists all key bindings")
	{}

	void Run(const Cmd::Args&) const OVERRIDE
	{
		for (auto& kv: keys)
		{
			bool teamSpecific = false;

			for ( int team = 1; team < MAX_TEAMS; ++team )
			{
				if ( kv.second.binding[ team ] )
				{
					teamSpecific = true;
					break;
				}
			}

			if ( !teamSpecific )
			{
				if ( kv.second.binding[ 0 ] )
				{
					Print( "%s = %s", KeyToString( kv.first ), kv.second.binding[ 0 ].value() );
				}
			}
			else
			{
				for ( int team = 0; team < MAX_TEAMS; ++team )
				{
					if ( kv.second.binding[ team ] )
					{
						Print( "%s[%s] = %s", KeyToString( kv.first ), teamName[ team ], kv.second.binding[ team ].value() );
					}
				}
			}
		}
	}
};

std::vector<std::string> deferredBindCommands;

class BindCmd: public Cmd::StaticCmd
{
	bool teambind;

	void ShowBind(Key b, std::function<bool(int)> teamFilter) const
	{
		bool bound = false;

		auto it = keys.find(b);
		if (it != keys.end()) {
			const auto& bindings = it->second.binding;
			for ( int i = 0; i < MAX_TEAMS; ++i )
			{
				if ( teamFilter(i) && bindings[ i ] )
				{
					Print( "\"%s\"[%s] = %s", KeyToString(b), teamName[ i ],
					       Cmd_QuoteString( bindings[ i ].value().c_str() ) );
					bound = true;
				}
			}
		}
		if ( !bound )
		{
			Print( "\"%s\" is not bound", KeyToString(b) );
		}
	}

public:
	BindCmd(Str::StringRef name, Str::StringRef desc, bool teambind):
		StaticCmd(name, Cmd::KEY_BINDING, desc), teambind(teambind)
	{}

	void Run(const Cmd::Args& args) const OVERRIDE
	{
		if (!IN_IsKeyboardLayoutInfoAvailable()) {
			deferredBindCommands.push_back(args.EscapedArgs(0));
			return;
		}
		int team = -1;
		int c = args.Argc();

		if (c < 2 + +teambind) {
			PrintUsage(args, teambind ? "<team> <key> [<command>]" : "<key> [<command>]", "attach a command to a key");
			return;
		}

		if ( teambind )
		{
			team = GetTeam( args.Argv( 1 ) );
			if (team < 0) {
				Print(InvalidTeamMessage(args.Argv(1)));
				return;
			}
		}

		Str::StringRef key = args.Argv( 1 + +teambind );
		Key b = StringToKey( key );

		if ( !b.IsBindable() )
		{
			Print(InvalidKeyMessage(key));
			return;
		}

		if (c == 2 + +teambind) {
			if (teambind) {
				ShowBind(b, [team](int t) { return t == team; });
			} else {
				ShowBind(b, [](int) { return true; });
			}
			return;
		}

		SetBinding( b, team, args.ConcatArgs( 2 + +teambind ) );
	}

	Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, Str::StringRef prefix) const OVERRIDE {
		Cmd::CompletionResult res;
		if (teambind && argNum == 1) {
			CompleteTeamName(prefix, res);
		} else if (argNum == 1 + +teambind) {
			CompleteKeyName(res, prefix);
		} else {
			DAEMON_ASSERT_GE(argNum, 2 + +teambind);
			res = CompleteEmbeddedCommand(args, 2 + +teambind, argNum);
		}
		return res;
	}
};


class EditBindCmd: public Cmd::StaticCmd
{
public:
	EditBindCmd():
		StaticCmd("editbind", Cmd::KEY_BINDING, "Puts a key binding in the text field for editing")
	{}

	void Run(const Cmd::Args& args) const OVERRIDE
	{
		std::u32string buf;
		int            b;
		int            team = -1;

		b = args.Argc();

		if ( b < 2 || b > 3 )
		{
			PrintUsage(args, "[<team>] <key>");
			return;
		}

		if ( b > 2 )
		{
			team = GetTeam( args.Argv( 1 ) );

			if ( team < 0 )
			{
				Print(InvalidTeamMessage(args.Argv(1)));
				return;
			}
		}

		const char *key = args.Argv( b - 1 ).c_str();
		Key k = StringToKey( key );

		if ( !k.IsBindable() )
		{
			Print(InvalidKeyMessage(key));
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

		buf += Str::UTF8To32( Cmd::Escape( KeyToString( k ) ) );
		buf += Str::UTF8To32(" ");

		auto maybeBinding = GetBinding( k, -team );
		if ( maybeBinding )
		{
			buf += Str::UTF8To32( Cmd::Escape( maybeBinding.value() ) );
		}

		// FIXME: use text console if that's where the editbind command was entered
		Con_OpenConsole_f();
		g_consoleField.SetText( buf );
	}

	Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, Str::StringRef prefix) const OVERRIDE {
		Cmd::CompletionResult res;
		CompleteTeamAndKey(argNum, args, prefix, res);
		return res;
	}
};

class UnbindCmd: public Cmd::StaticCmd
{
public:
	UnbindCmd():
		StaticCmd("unbind", Cmd::KEY_BINDING, "Removes a key binding")
	{}

	void Run(const Cmd::Args& args) const OVERRIDE
	{
		int b = args.Argc();
		int team = -1;

		if ( b < 2 || b > 3 )
		{
			PrintUsage(args, "[<team>] <key>", "remove commands from a key");
			return;
		}

		if ( b > 2 )
		{
			team = GetTeam( args.Argv( 1 ) );

			if ( team < 0 )
			{
				Print(InvalidTeamMessage(args.Argv(1)));
				return;
			}
		}

		Key key = StringToKey( args.Argv( b - 1 ).c_str() );

		if ( !key.IsBindable() )
		{
			Print(InvalidKeyMessage(args.Argv(b - 1)));
			return;
		}

		SetBinding( key, team, "" );
	}

	Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, Str::StringRef prefix) const OVERRIDE {
		Cmd::CompletionResult res;
		CompleteTeamAndKey(argNum, args, prefix, res);
		return res;
	}
};

class UnbindAllCmd: public Cmd::StaticCmd
{
public:
	UnbindAllCmd():
		StaticCmd("unbindall", Cmd::KEY_BINDING, "Removes all key bindings")
	{}

	void Run(const Cmd::Args&) const OVERRIDE
	{
		for (auto& kv : keys)
		{
			SetBinding( kv.first, -1, "" );
		}
	}
};

class ModcaseCmd: public Cmd::StaticCmd
{
public:
	ModcaseCmd():
		StaticCmd("modcase", Cmd::KEY_BINDING, "Conditionally performs an action based on modifier keys")
	{}

	void Run(const Cmd::Args& args) const OVERRIDE
	{
		int argc = args.Argc();
		int index = 0;
		int max = 0;
		int count = ( argc - 1 ) / 2; // round down :-)
		const char *v;

		int mods[ 1 << NUM_RECOGNISED_MODIFIERS ];
		// want 'modifierMask_t mods[argc / 2 - 1];' (variable array, C99)
		// but MSVC apparently doesn't like that

		if ( argc < 3 )
		{
			PrintUsage( args, "<modifiers> <command> [<modifiers> <command>] … [<command>]" );
			return;
		}

		while ( index < count )
		{
			modifierMask_t mask = getModifierMask( args.Argv( 2 * index + 1 ).c_str() );

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
		v = ( argc & 1 ) ? nullptr : args.Argv( argc - 1 ).c_str();

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
					v = args.Argv( 2 * i + 2 ).c_str();
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

	// TODO: completion
};

const BindListCmd BindListCmdRegistration;
const BindCmd BindCmdRegistration{"bind", "Set or view command to be executed when a key is pressed", false};
const BindCmd TeambindCmdRegistration{"teambind", "Set key binding for a specific team", true};
const EditBindCmd EditBindCmdRegistration;
const UnbindCmd UnbindCmdRegistration;
const UnbindAllCmd UnbindAllCmdRegistration;
const ModcaseCmd ModcaseCmdRegistration;

} // namespace (key binding commands)

/* The SDL subsystem with keyboard functionality ("video") is not initialized when execing configs
   at startup. We could just initialize that first, but the subystem is also shut down and
   re-initialized on map changes, etc. and it's hard to prove that a command can't be executed
   during this period. */
void BufferDeferredBinds()
{
	if (!IN_IsKeyboardLayoutInfoAvailable()) {
		return;
	}
	for (const auto& bind: deferredBindCommands) {
		Cmd::BufferCommandText(bind, false);
	}
	deferredBindCommands.clear();
}

} // namespace Keyboard
