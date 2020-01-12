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

#include <SDL.h>
#include "client/client.h"
#include "client/key_identification.h"
#include "qcommon/q_unicode.h"
#include "qcommon/qcommon.h"
#include "qcommon/sys.h"
#include "framework/CommandSystem.h"
#include "sys/sys_events.h"

static cvar_t       *in_keyboardDebug = nullptr;

static SDL_Joystick *stick = nullptr;
static SDL_GameController *gamepad = nullptr;

static bool     mouseAvailable = false;

static cvar_t       *in_mouse = nullptr;

static cvar_t       *in_nograb;

static cvar_t       *in_joystick = nullptr;
static cvar_t       *in_joystickDebug = nullptr;
static cvar_t       *in_joystickThreshold = nullptr;
static cvar_t       *in_joystickNo = nullptr;
static cvar_t       *in_joystickUseAnalog = nullptr;
static cvar_t       *in_gameControllerTriggerDeadzone = nullptr;

static cvar_t *in_gameController = nullptr;
static cvar_t *in_gameControllerDebug = nullptr;

static SDL_Window *window = nullptr;


/*
===============
IN_PrintKey
===============
*/
static void IN_PrintKey( const SDL_Keysym *keysym, Keyboard::Key key, bool down )
{
	if ( keysym->mod & KMOD_LSHIFT ) { Log::Notice( " KMOD_LSHIFT" ); }

	if ( keysym->mod & KMOD_RSHIFT ) { Log::Notice( " KMOD_RSHIFT" ); }

	if ( keysym->mod & KMOD_LCTRL ) { Log::Notice( " KMOD_LCTRL" ); }

	if ( keysym->mod & KMOD_RCTRL ) { Log::Notice( " KMOD_RCTRL" ); }

	if ( keysym->mod & KMOD_LALT ) { Log::Notice( " KMOD_LALT" ); }

	if ( keysym->mod & KMOD_RALT ) { Log::Notice( " KMOD_RALT" ); }

	if ( keysym->mod & KMOD_LGUI ) { Log::Notice( " KMOD_LGUI" ); }

	if ( keysym->mod & KMOD_RGUI ) { Log::Notice( " KMOD_RGUI" ); }

	if ( keysym->mod & KMOD_NUM ) { Log::Notice( " KMOD_NUM" ); }

	if ( keysym->mod & KMOD_CAPS ) { Log::Notice( " KMOD_CAPS" ); }

	if ( keysym->mod & KMOD_MODE ) { Log::Notice( " KMOD_MODE" ); }

	if ( keysym->mod & KMOD_RESERVED ) { Log::Notice( " KMOD_RESERVED" ); }

	Log::Notice( "%c scancode = 0x%03x \"%s\" engine keycode name = %s\n", down ? '+' : '-',
	    keysym->scancode, SDL_GetKeyName( keysym->sym ), KeyToString( key ) );
}

/*
===============
IN_IsConsoleKey

===============
*/
static bool IN_IsConsoleKey( Keyboard::Key key )
{
	for (Keyboard::Key k : Keyboard::GetConsoleKeys())
	{
		if (k == key) {
			return true;
		}
	}

	return false;
}

// Translates based on keycode, not scancode.
static Keyboard::Key IN_TranslateSDLToQ3Key( SDL_Keysym *keysym, bool down )
{
	using Keyboard::Key;
	Key key;

	if ( keysym->sym == SDLK_DELETE ) // SDLK_DELETE is anomalously located in the Unicode range.
	{
		key = Key(K_DEL);
	}
	else if ( keysym->sym >= SDLK_SPACE && keysym->sym < UNICODE_MAX_CODE_POINT )
	{
		key = Key::FromCharacter(keysym->sym);
	}
	else
	{
		switch ( keysym->sym )
		{
			case SDLK_PAGEUP:
				key = Key(K_PGUP);
				break;

			case SDLK_KP_9:
				key = Key(K_KP_PGUP);
				break;

			case SDLK_PAGEDOWN:
				key = Key(K_PGDN);
				break;

			case SDLK_KP_3:
				key = Key(K_KP_PGDN);
				break;

			case SDLK_KP_7:
				key = Key(K_KP_HOME);
				break;

			case SDLK_HOME:
				key = Key(K_HOME);
				break;

			case SDLK_KP_1:
				key = Key(K_KP_END);
				break;

			case SDLK_END:
				key = Key(K_END);
				break;

			case SDLK_KP_4:
				key = Key(K_KP_LEFTARROW);
				break;

			case SDLK_LEFT:
				key = Key(K_LEFTARROW);
				break;

			case SDLK_KP_6:
				key = Key(K_KP_RIGHTARROW);
				break;

			case SDLK_RIGHT:
				key = Key(K_RIGHTARROW);
				break;

			case SDLK_KP_2:
				key = Key(K_KP_DOWNARROW);
				break;

			case SDLK_DOWN:
				key = Key(K_DOWNARROW);
				break;

			case SDLK_KP_8:
				key = Key(K_KP_UPARROW);
				break;

			case SDLK_UP:
				key = Key(K_UPARROW);
				break;

			case SDLK_ESCAPE:
				key = Key(K_ESCAPE);
				break;

			case SDLK_KP_ENTER:
				key = Key(K_KP_ENTER);
				break;

			case SDLK_RETURN:
				key = Key(K_ENTER);
				break;

			case SDLK_TAB:
				key = Key(K_TAB);
				break;

			case SDLK_F1:
				key = Key(K_F1);
				break;

			case SDLK_F2:
				key = Key(K_F2);
				break;

			case SDLK_F3:
				key = Key(K_F3);
				break;

			case SDLK_F4:
				key = Key(K_F4);
				break;

			case SDLK_F5:
				key = Key(K_F5);
				break;

			case SDLK_F6:
				key = Key(K_F6);
				break;

			case SDLK_F7:
				key = Key(K_F7);
				break;

			case SDLK_F8:
				key = Key(K_F8);
				break;

			case SDLK_F9:
				key = Key(K_F9);
				break;

			case SDLK_F10:
				key = Key(K_F10);
				break;

			case SDLK_F11:
				key = Key(K_F11);
				break;

			case SDLK_F12:
				key = Key(K_F12);
				break;

			case SDLK_F13:
				key = Key(K_F13);
				break;

			case SDLK_F14:
				key = Key(K_F14);
				break;

			case SDLK_F15:
				key = Key(K_F15);
				break;

			case SDLK_BACKSPACE:
				key = Key(K_BACKSPACE);
				break;

			case SDLK_KP_PERIOD:
				key = Key(K_KP_DEL);
				break;

			case SDLK_DELETE:
				key = Key(K_DEL);
				break;

			case SDLK_PAUSE:
				key = Key(K_PAUSE);
				break;

			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
				key = Key(K_SHIFT);
				break;

			case SDLK_LCTRL:
			case SDLK_RCTRL:
				key = Key(K_CTRL);
				break;

			case SDLK_RGUI:
			case SDLK_LGUI:
				key = Key(K_COMMAND);
				break;

			case SDLK_RALT:
			case SDLK_LALT:
				key = Key(K_ALT);
				break;

			case SDLK_KP_5:
				key = Key(K_KP_5);
				break;

			case SDLK_INSERT:
				key = Key(K_INS);
				break;

			case SDLK_KP_0:
				key = Key(K_KP_INS);
				break;

			case SDLK_KP_MULTIPLY:
				key = Key(K_KP_STAR);
				break;

			case SDLK_KP_PLUS:
				key = Key(K_KP_PLUS);
				break;

			case SDLK_KP_MINUS:
				key = Key(K_KP_MINUS);
				break;

			case SDLK_KP_DIVIDE:
				key = Key(K_KP_SLASH);
				break;

			case SDLK_MODE:
				key = Key(K_MODE);
				break;

			case SDLK_HELP:
				key = Key(K_HELP);
				break;

			case SDLK_PRINTSCREEN:
				key = Key(K_PRINT);
				break;

			case SDLK_SYSREQ:
				key = Key(K_SYSREQ);
				break;

			case SDLK_MENU:
				key = Key(K_MENU);
				break;

			case SDLK_APPLICATION:
				key = Key(K_COMPOSE);
				break;

			case SDLK_POWER:
				key = Key(K_POWER);
				break;

			case SDLK_UNDO:
				key = Key(K_UNDO);
				break;

			case SDLK_SCROLLLOCK:
				key = Key(K_SCROLLLOCK);
				break;

			case SDLK_NUMLOCKCLEAR:
				key = Key(K_KP_NUMLOCK);
				break;

			case SDLK_CAPSLOCK:
				key = Key(K_CAPSLOCK);
				break;

			default:
				break;
		}
	}

	if ( in_keyboardDebug->integer )
	{
		IN_PrintKey( keysym, key, down );
	}

	return key;
}


// Whether the cursor is enabled
static MouseMode mouse_mode = MouseMode::SystemCursor;

/*
 * Returns whether the cursor is enabled
 */
MouseMode IN_GetMouseMode()
{
    return mouse_mode;
}

/*
 * Enables or disables the cursor
 */
void IN_SetMouseMode(MouseMode newMode)
{
	if ( newMode != mouse_mode )
	{
		if ( !mouseAvailable || !SDL_WasInit( SDL_INIT_VIDEO ) )
		{
			return;
		}

		SDL_ShowCursor( newMode == MouseMode::SystemCursor ? SDL_ENABLE : SDL_DISABLE );
		auto grab = newMode != MouseMode::Deltas || in_nograb->integer ? SDL_FALSE : SDL_TRUE;
		SDL_SetRelativeMouseMode( grab );
		SDL_SetWindowGrab( window, grab );

		if ( mouse_mode == MouseMode::Deltas )
		{
			IN_CenterMouse();
		}
		mouse_mode = newMode;
	}
}

static bool in_focus = false;

static void IN_SetFocus(bool hasFocus)
{
	if ( hasFocus == in_focus )
	{
		return;
	}

	in_focus = hasFocus;

	Com_QueueEvent( Util::make_unique<Sys::FocusEvent>(hasFocus) );

}

/*
 * Moves the mouse at the center of the window
 */
void IN_CenterMouse()
{
	int w, h;
	SDL_GetWindowSize( window, &w, &h );
	SDL_WarpMouseInWindow( window, w / 2, h / 2 );
}

// We translate axes movement into keypresses
static keyNum_t joy_keys[ 16 ] =
{
	K_LEFTARROW, K_RIGHTARROW,
	K_UPARROW,   K_DOWNARROW,
	K_JOY16,     K_JOY17,
	K_JOY18,     K_JOY19,
	K_JOY20,     K_JOY21,
	K_JOY22,     K_JOY23,

	K_JOY24,     K_JOY25,
	K_JOY26,     K_JOY27
};

// translate hat events into keypresses
// the 4 highest buttons are used for the first hat ...
static keyNum_t hat_keys[ 16 ] =
{
	K_JOY29, K_JOY30,
	K_JOY31, K_JOY32,
	K_JOY25, K_JOY26,
	K_JOY27, K_JOY28,
	K_JOY21, K_JOY22,
	K_JOY23, K_JOY24,
	K_JOY17, K_JOY18,
	K_JOY19, K_JOY20
};

struct
{
	bool     buttons[ 16 ]; // !!! FIXME: these might be too many.
	unsigned int oldaxes;
	int          oldaaxes[ 16 ];
	unsigned int oldhats;
} stick_state;

static const char* JoystickNameForIndex(int index)
{
    const char* name = SDL_JoystickNameForIndex(index);
    return name ? name : "<no name found>";
}

/*
===============
IN_InitJoystick
===============
*/
static void IN_InitJoystick()
{
	int i = 0;
	int total = 0;

	if ( stick != nullptr )
	{
		SDL_JoystickClose( stick );
	}

	stick = nullptr;
	memset( &stick_state, '\0', sizeof( stick_state ) );

	if ( !in_joystick->integer )
	{
		Log::Debug( "Joystick is not active." );

		if ( !gamepad )
		{
			Log::Debug( "Gamepad is not active." );
		}

		return;
	}

	if ( !SDL_WasInit( SDL_INIT_JOYSTICK ) )
	{
		Log::Debug( "Calling SDL_Init(SDL_INIT_JOYSTICK)..." );

		if ( SDL_Init( SDL_INIT_JOYSTICK ) == -1 )
		{
			Log::Debug( "SDL_Init(SDL_INIT_JOYSTICK) failed: %s", SDL_GetError() );
			return;
		}

		Log::Debug( "SDL_Init(SDL_INIT_JOYSTICK) passed." );
	}

	total = SDL_NumJoysticks();
	Log::Debug( "%d possible joysticks", total );

	for ( i = 0; i < total; i++ )
	{
		Log::Debug( "[%d] %s", i, JoystickNameForIndex( i ) );
	}

	in_joystickNo = Cvar_Get( "in_joystickNo", "0", 0 );

	if ( in_joystickNo->integer < 0 || in_joystickNo->integer >= total )
	{
		Cvar_Set( "in_joystickNo", "0" );
	}

	in_joystickUseAnalog = Cvar_Get( "in_joystickUseAnalog", "0", 0 );

	stick = SDL_JoystickOpen( in_joystickNo->integer );

	if ( stick == nullptr )
	{
		Log::Debug( "No joystick opened." );
		return;
	}

	if ( SDL_IsGameController( in_joystickNo->integer ) )
	{
		gamepad = SDL_GameControllerOpen( in_joystickNo->integer );
		if ( gamepad )
		{
			Cvar_Set( "in_gameControllerAvailable", "1" );
			SDL_GameControllerEventState( SDL_QUERY );
		}
	}

	Log::Debug( "Joystick %d opened", in_joystickNo->integer );
	Log::Debug( "Name:    %s", JoystickNameForIndex( in_joystickNo->integer ) );
	Log::Debug( "Axes:    %d", SDL_JoystickNumAxes( stick ) );
	Log::Debug( "Hats:    %d", SDL_JoystickNumHats( stick ) );
	Log::Debug( "Buttons: %d", SDL_JoystickNumButtons( stick ) );
	Log::Debug( "Balls: %d", SDL_JoystickNumBalls( stick ) );
	Log::Debug( "Use Analog: %s", in_joystickUseAnalog->integer ? "Yes" : "No" );
	Log::Debug( "Use SDL2 GameController mappings: %s", gamepad ? "Yes" : "No" );

	SDL_JoystickEventState( SDL_QUERY );
}

/*
===============
IN_ShutdownJoystick
===============
*/
static void IN_ShutdownJoystick()
{
	if ( gamepad )
	{
		SDL_GameControllerClose( gamepad );
		gamepad = nullptr;
	}
	if ( stick )
	{
		SDL_JoystickClose( stick );
		stick = nullptr;
	}

	SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
}

static void QueueKeyEvent(Keyboard::Key key, bool down)
{
	if (!key.IsValid()) {
		return;
	}
	Com_QueueEvent( Util::make_unique<Sys::KeyEvent>(key, down, Sys_Milliseconds()) );
}
static void QueueKeyEvent(keyNum_t key, bool down) {
	QueueKeyEvent(Keyboard::Key(key), down);
}

/*
===============
IN_JoyMove
===============
*/
static void IN_JoyMove()
{
	unsigned int axes = 0;
	unsigned int hats = 0;
	int          total = 0;
	int          i = 0;

	if ( !stick )
	{
		return;
	}

	SDL_JoystickUpdate();

	// update the ball state.
	total = SDL_JoystickNumBalls( stick );

	if ( total > 0 )
	{
		int balldx = 0;
		int balldy = 0;

		for ( i = 0; i < total; i++ )
		{
			int dx = 0;
			int dy = 0;
			SDL_JoystickGetBall( stick, i, &dx, &dy );
			balldx += dx;
			balldy += dy;
		}

		if ( balldx || balldy )
		{
			// !!! FIXME: is this good for stick balls, or just mice?
			// Scale like the mouse input...
			if ( abs( balldx ) > 1 )
			{
				balldx *= 2;
			}

			if ( abs( balldy ) > 1 )
			{
				balldy *= 2;
			}

			Com_QueueEvent( Util::make_unique<Sys::MouseEvent>(balldx, balldy) );
		}
	}

	// now query the stick buttons...
	total = SDL_JoystickNumButtons( stick );

	if ( total > 0 )
	{
		if ( total > (int) ARRAY_LEN( stick_state.buttons ) )
		{
			total = ARRAY_LEN( stick_state.buttons );
		}

		for ( i = 0; i < total; i++ )
		{
			bool pressed = ( SDL_JoystickGetButton( stick, i ) != 0 );

			if ( pressed != stick_state.buttons[ i ] )
			{
				QueueKeyEvent( Util::enum_cast<keyNum_t>(K_JOY1 + i), pressed );

				stick_state.buttons[ i ] = pressed;
			}
		}
	}

	// look at the hats...
	total = SDL_JoystickNumHats( stick );

	if ( total > 0 )
	{
		if ( total > 4 ) { total = 4; }

		for ( i = 0; i < total; i++ )
		{
			( ( Uint8 * ) &hats ) [ i ] = SDL_JoystickGetHat( stick, i );
		}
	}

	// update hat state
	if ( hats != stick_state.oldhats )
	{
		for ( i = 0; i < 4; i++ )
		{
			if ( ( ( Uint8 * ) &hats ) [ i ] != ( ( Uint8 * ) &stick_state.oldhats ) [ i ] )
			{
				// release event
				switch ( ( ( Uint8 * ) &stick_state.oldhats ) [ i ] )
				{
					case SDL_HAT_UP:
						QueueKeyEvent( hat_keys[ 4 * i + 0 ], false );
						break;

					case SDL_HAT_RIGHT:
						QueueKeyEvent( hat_keys[ 4 * i + 1 ], false );
						break;

					case SDL_HAT_DOWN:
						QueueKeyEvent( hat_keys[ 4 * i + 2 ], false );
						break;

					case SDL_HAT_LEFT:
						QueueKeyEvent( hat_keys[ 4 * i + 3 ], false );
						break;

					case SDL_HAT_RIGHTUP:
						QueueKeyEvent( hat_keys[ 4 * i + 0 ], false );
						QueueKeyEvent( hat_keys[ 4 * i + 1 ], false );
						break;

					case SDL_HAT_RIGHTDOWN:
						QueueKeyEvent( hat_keys[ 4 * i + 2 ], false );
						QueueKeyEvent( hat_keys[ 4 * i + 1 ], false );
						break;

					case SDL_HAT_LEFTUP:
						QueueKeyEvent( hat_keys[ 4 * i + 0 ], false );
						QueueKeyEvent( hat_keys[ 4 * i + 3 ], false );
						break;

					case SDL_HAT_LEFTDOWN:
						QueueKeyEvent( hat_keys[ 4 * i + 2 ], false );
						QueueKeyEvent( hat_keys[ 4 * i + 3 ], false );
						break;

					default:
						break;
				}

				// press event
				switch ( ( ( Uint8 * ) &hats ) [ i ] )
				{
					case SDL_HAT_UP:
						QueueKeyEvent( hat_keys[ 4 * i + 0 ], true );
						break;

					case SDL_HAT_RIGHT:
						QueueKeyEvent( hat_keys[ 4 * i + 1 ], true );
						break;

					case SDL_HAT_DOWN:
						QueueKeyEvent( hat_keys[ 4 * i + 2 ], true );
						break;

					case SDL_HAT_LEFT:
						QueueKeyEvent( hat_keys[ 4 * i + 3 ], true );
						break;

					case SDL_HAT_RIGHTUP:
						QueueKeyEvent( hat_keys[ 4 * i + 0 ], true );
						QueueKeyEvent( hat_keys[ 4 * i + 1 ], true );
						break;

					case SDL_HAT_RIGHTDOWN:
						QueueKeyEvent( hat_keys[ 4 * i + 2 ], true );
						QueueKeyEvent( hat_keys[ 4 * i + 1 ], true );
						break;

					case SDL_HAT_LEFTUP:
						QueueKeyEvent( hat_keys[ 4 * i + 0 ], true );
						QueueKeyEvent( hat_keys[ 4 * i + 3 ], true );
						break;

					case SDL_HAT_LEFTDOWN:
						QueueKeyEvent( hat_keys[ 4 * i + 2 ], true );
						QueueKeyEvent( hat_keys[ 4 * i + 3 ], true );
						break;

					default:
						break;
				}
			}
		}
	}

	// save hat state
	stick_state.oldhats = hats;

	// finally, look at the axes...
	total = SDL_JoystickNumAxes( stick );

	if ( total > 0 )
	{
		if ( total > 16 ) { total = 16; }

		for ( i = 0; i < total; i++ )
		{
			Sint16 axis = SDL_JoystickGetAxis( stick, i );

			if ( !in_joystickUseAnalog->integer )
			{
				float f = ( ( float ) axis ) / 32767.0f;

				if ( f < -in_joystickThreshold->value )
				{
					axes |= ( 1 << ( i * 2 ) );
				}
				else if ( f > in_joystickThreshold->value )
				{
					axes |= ( 1 << ( ( i * 2 ) + 1 ) );
				}
			}
			else
			{
				float f = ( ( float ) abs( axis ) ) / 32767.0f;

				if ( f < in_joystickThreshold->value ) { axis = 0; }

				if ( axis != stick_state.oldaaxes[ i ] )
				{
					Com_QueueEvent( Util::make_unique<Sys::JoystickEvent>(i, axis) );

					stick_state.oldaaxes[ i ] = axis;
				}
			}
		}
	}

	/* Time to update axes state based on old vs. new. */
	if ( axes != stick_state.oldaxes )
	{
		for ( i = 0; i < 16; i++ )
		{
			if ( ( axes & ( 1 << i ) ) && !( stick_state.oldaxes & ( 1 << i ) ) )
			{
				QueueKeyEvent( joy_keys[ i ], true );
			}

			if ( !( axes & ( 1 << i ) ) && ( stick_state.oldaxes & ( 1 << i ) ) )
			{
				QueueKeyEvent( joy_keys[ i ], false );
			}
		}
	}

	/* Save for future generations. */
	stick_state.oldaxes = axes;
}

static void IN_GameControllerAxis( SDL_GameControllerAxis controllerAxis, joystickAxis_t gameAxis, float scale )
{
	Sint16 axis = SDL_GameControllerGetAxis( gamepad, controllerAxis );
	float  f = ( ( float ) axis ) / 32767.0f;

	if ( f > -in_joystickThreshold->value && f < in_joystickThreshold->value )
	{
		Com_QueueEvent( Util::make_unique<Sys::JoystickEvent>(Util::ordinal(gameAxis), 0) );
	}
	else
	{
		if ( in_gameControllerDebug->integer )
		{
			Log::Notice( "GameController axis %i = %f\n", controllerAxis, f );
		}

		Com_QueueEvent( Util::make_unique<Sys::JoystickEvent>(Util::ordinal(gameAxis), static_cast<int>(f * scale)) );
	}
}

static int IN_GameControllerAxisToButton( SDL_GameControllerAxis controllerAxis, keyNum_t key )
{
	using Keyboard::Key;
	unsigned int axes = 0;

	Sint16       axis = SDL_GameControllerGetAxis( gamepad, controllerAxis );
	float        f = ( ( float ) axis ) / 32767.0f;

	if ( f > in_gameControllerTriggerDeadzone->value )
	{
		axes |= ( 1 << ( controllerAxis ) );
	}

	if ( ( axes & ( 1 << controllerAxis ) ) && !( stick_state.oldaxes & ( 1 << controllerAxis ) ) )
	{
		QueueKeyEvent( key, true );
		if ( in_gameControllerDebug->integer )
		{
			Log::Notice( "GameController axis = %s to key = Q:0x%02x(%s), value = %f\n",
						 SDL_GameControllerGetStringForAxis( controllerAxis ), key,
						 Keyboard::KeyToString( Key(key) ), f );
		}
	}

	if ( !( axes & ( 1 << controllerAxis ) ) && ( stick_state.oldaxes & ( 1 << controllerAxis ) ) )
	{
		QueueKeyEvent( key, false );
		if ( in_gameControllerDebug->integer )
		{
			Log::Notice( "GameController axis = %s to key = Q:0x%02x(%s), value = %f\n",
						 SDL_GameControllerGetStringForAxis( controllerAxis ), key,
						 Keyboard::KeyToString( Key(key) ), f );
		}
	}

	return axes;
}

/*
===============
IN_GameControllerMove
===============
*/
static void IN_GameControllerMove()
{
	using Keyboard::Key;
	unsigned int axes = 0;
	int          i = 0;

	if ( !gamepad )
	{
		return;
	}

	SDL_GameControllerUpdate();

	for ( i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++ )
	{
		bool pressed = SDL_GameControllerGetButton( gamepad, Util::enum_cast<SDL_GameControllerButton>(i) );

		if ( pressed != stick_state.buttons[ i ] )
		{
			QueueKeyEvent( Util::enum_cast<keyNum_t>(K_CONTROLLER_A + i), pressed );

			if ( in_gameControllerDebug->integer )
			{
				Log::Notice( "GameController button %s = %s\n",
							 SDL_GameControllerGetStringForButton( Util::enum_cast<SDL_GameControllerButton>(i) ),
							 pressed ? "Pressed" : "Released" );
			}

			stick_state.buttons[ i ] = pressed;
		}
	}

	// use left stick for strafing
	IN_GameControllerAxis( SDL_CONTROLLER_AXIS_LEFTX, joystickAxis_t::AXIS_SIDE, 127 );
	IN_GameControllerAxis( SDL_CONTROLLER_AXIS_LEFTY, joystickAxis_t::AXIS_FORWARD, -127 );

	// use right stick for viewing
	IN_GameControllerAxis( SDL_CONTROLLER_AXIS_RIGHTX, joystickAxis_t::AXIS_YAW, -127 );
	IN_GameControllerAxis( SDL_CONTROLLER_AXIS_RIGHTY, joystickAxis_t::AXIS_PITCH, 127 );

	axes |= IN_GameControllerAxisToButton( SDL_CONTROLLER_AXIS_TRIGGERLEFT, K_CONTROLLER_LT );
	axes |= IN_GameControllerAxisToButton( SDL_CONTROLLER_AXIS_TRIGGERRIGHT, K_CONTROLLER_RT );

	/* Save for future generations. */
	stick_state.oldaxes = axes;
}


static std::unordered_map<int, Keyboard::Key> downKeys;

/*
===============
IN_ProcessEvents
===============
*/
static void IN_ProcessEvents( bool dropInput )
{
	using Keyboard::Key;
	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		return;
	}

	// HACK: ignore a text event if this is true, to avoid text in the console being generated by
	//       pressing the key to open it.
	bool lastEventWasConsoleKeyDown = false;

	SDL_Event e;
	while ( SDL_PollEvent( &e ) )
	{
		switch ( e.type )
		{
			case SDL_KEYDOWN:
				if ( !dropInput && ( !e.key.repeat || cls.keyCatchers ) )
				{
					// Send events for both scancode- and keycode-based Keys
					Key kScan = Keyboard::Key::FromScancode( e.key.keysym.scancode );
					Key kKeycode = IN_TranslateSDLToQ3Key( &e.key.keysym, true );
					bool consoleFound = false;
					for (Key k: {kScan, kKeycode} ) {
						if ( IN_IsConsoleKey( k ) && !keys[ Key(K_ALT) ].down) {
							// Console keys can't be bound or generate characters
							// (but allow Alt+key for text input)
							QueueKeyEvent( Key::CONSOLE, true );
							consoleFound = true;
							break;
						}
					}
					if ( consoleFound ) {
						lastEventWasConsoleKeyDown = true;
						continue;
					}
					for (Key k: {kKeycode, kScan} ) {
						QueueKeyEvent( k, true );
					}
				}
				break;

			case SDL_KEYUP:
				if ( !dropInput )
				{
					QueueKeyEvent( Keyboard::Key::FromScancode( e.key.keysym.scancode ), false );
					Key k = IN_TranslateSDLToQ3Key( &e.key.keysym, false );
					QueueKeyEvent( k, false );
				}

				break;
			case SDL_TEXTINPUT:
				if ( !lastEventWasConsoleKeyDown )
				{
					char *c = e.text.text;

					while ( *c )
					{
						int width = Q_UTF8_Width( c );
						Com_QueueEvent( Util::make_unique<Sys::CharEvent>( Q_UTF8_CodePoint( c ) ) );
						c += width;
					}
				}
				break;
			case SDL_MOUSEMOTION:
				if ( !dropInput )
				{
					if ( mouse_mode != MouseMode::Deltas )
					{
						Com_QueueEvent( Util::make_unique<Sys::MousePosEvent>(e.motion.x, e.motion.y) );
					}
					else
					{
						Com_QueueEvent( Util::make_unique<Sys::MouseEvent>(e.motion.xrel, e.motion.yrel) );
#if defined( __linux__ ) || defined( __BSD__ )
						if ( !in_nograb->integer )
						{
							// work around X window managers and edge-based workspace flipping
							// - without this, we get LeaveNotify, no mouse button events, EnterNotify;
							//   we would likely miss some button presses and releases.
							IN_CenterMouse();
						}
#endif
					}
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if ( !dropInput )
				{
					keyNum_t b;

					switch ( e.button.button )
					{
						case SDL_BUTTON_LEFT:
							b = K_MOUSE1;
							break;

						case SDL_BUTTON_MIDDLE:
							b = K_MOUSE3;
							break;

						case SDL_BUTTON_RIGHT:
							b = K_MOUSE2;
							break;
						case SDL_BUTTON_X1:
							b = K_MOUSE4;
							break;

						case SDL_BUTTON_X2:
							b = K_MOUSE5;
							break;

						default:
							b = Util::enum_cast<keyNum_t>(K_AUX1 + ( e.button.button - ( SDL_BUTTON_X2 + 1 ) ) % 16);
							break;
					}
					QueueKeyEvent( b, e.type == SDL_MOUSEBUTTONDOWN );
				}
				break;
			case SDL_MOUSEWHEEL:
				// FIXME: mouse wheel support shouldn't use keys!
				if ( e.wheel.y > 0 )
				{
					QueueKeyEvent( K_MWHEELUP, true );
					QueueKeyEvent( K_MWHEELUP, false );
				}
				else
				{
					QueueKeyEvent( K_MWHEELDOWN, true );
					QueueKeyEvent( K_MWHEELDOWN, false );
				}
				break;

			case SDL_WINDOWEVENT:
				switch( e.window.event )
				{
					case SDL_WINDOWEVENT_RESIZED:
						{
							char width[32], height[32];
							Com_sprintf( width, sizeof( width ), "%d", e.window.data1 );
							Com_sprintf( height, sizeof( height ), "%d", e.window.data2 );
							Cvar_Set( "r_customwidth", width );
							Cvar_Set( "r_customheight", height );
							Cvar_Set( "r_mode", "-1" );
						}
						break;

					case SDL_WINDOWEVENT_MINIMIZED:    Cvar_SetValue( "com_minimized", 1 ); break;
					case SDL_WINDOWEVENT_RESTORED:
					case SDL_WINDOWEVENT_MAXIMIZED:    Cvar_SetValue( "com_minimized", 0 ); break;
					case SDL_WINDOWEVENT_FOCUS_LOST:   Cvar_SetValue( "com_unfocused", 1 ); break;
					case SDL_WINDOWEVENT_FOCUS_GAINED: Cvar_SetValue( "com_unfocused", 0 ); break;
				}
				break;
			case SDL_QUIT:
				Cmd::ExecuteCommand("quit Closed window");
				break;
			default:
				break;
		} // switch
		lastEventWasConsoleKeyDown = false;
	}
}

/*
===============
IN_IsNumLockDown
===============
*/

bool IN_IsNumLockDown() {
    return SDL_GetModState() & KMOD_NUM;
}
/*
===============
IN_Frame
===============
*/
static bool dropInput = false;

void IN_Frame()
{
	if ( gamepad )
	{
		IN_GameControllerMove();
	}
	else
	{
		IN_JoyMove();
	}

	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		// Console is down in windowed mode
		IN_SetFocus( false );
	}
	else if ( !( SDL_GetWindowFlags( window ) & SDL_WINDOW_INPUT_FOCUS ) )
	{
		// Window doesn't have focus
		IN_SetFocus( false );
	}
	else if ( com_minimized->integer )
	{
		// Minimized
		IN_SetFocus( false );
	}
	else
	{
		IN_SetFocus( true );
	}

	IN_ProcessEvents( dropInput );
}

void IN_DropInputsForFrame()
{
	dropInput = true;
	in_focus = false;
}

void IN_FrameEnd()
{
	dropInput = false;
}

/*
===============
IN_Init
===============
*/
void IN_Init( void *windowData )
{
	int appState;

	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		Sys::Error( "IN_Init called before SDL_Init( SDL_INIT_VIDEO )" );
	}

	window = (SDL_Window*) windowData;

	Log::Debug( "------- Input Initialization -------" );

	in_keyboardDebug = Cvar_Get( "in_keyboardDebug", "0", CVAR_TEMP );

	// mouse variables
	in_mouse = Cvar_Get( "in_mouse", "1", 0 );
	in_nograb = Cvar_Get( "in_nograb", "0", 0 );

	in_joystick = Cvar_Get( "in_joystick", "0",  CVAR_LATCH );
	in_joystickDebug = Cvar_Get( "in_joystickDebug", "0", CVAR_TEMP );
	in_joystickThreshold = Cvar_Get( "in_joystickThreshold", "0.15", 0 );
	in_gameControllerTriggerDeadzone = Cvar_Get( "in_gameControllerTriggerDeadzone", "0.5", 0);

	in_gameController = Cvar_Get( "in_gameController", "1", CVAR_TEMP );
	in_gameControllerDebug = Cvar_Get( "in_gameControllerDebug", "0", CVAR_TEMP );
	SDL_StartTextInput();
	mouseAvailable = ( in_mouse->value != 0 );
	IN_SetMouseMode( MouseMode::CustomCursor );

	appState = SDL_GetWindowFlags( window );
	Cvar_SetValue( "com_unfocused", !( appState & SDL_WINDOW_INPUT_FOCUS ) );
	Cvar_SetValue( "com_minimized", ( appState & SDL_WINDOW_MINIMIZED ) );
	IN_InitJoystick();
	Log::Debug( "------------------------------------" );
}

// SDL_GetScancodeFromKey won't work before initialization
bool IN_IsKeyboardLayoutInfoAvailable()
{
	return SDL_WasInit(SDL_INIT_VIDEO) == SDL_INIT_VIDEO;
}

/*
===============
IN_Shutdown
===============
*/
void IN_Shutdown()
{
	SDL_StopTextInput();
	IN_SetMouseMode(MouseMode::SystemCursor);
	mouseAvailable = false;

	IN_ShutdownJoystick();

	window = nullptr;
}

/*
===============
IN_Restart
===============
*/
void IN_Restart()
{
	IN_ShutdownJoystick();
	IN_Init( window );
}
