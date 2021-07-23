/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <SDL.h>

#ifdef USE_SMP
#include <SDL_thread.h>
#endif

#include "renderer/tr_local.h"

#include "sdl_icon.h"
#include "SDL_syswm.h"
#include "framework/CommandSystem.h"
#include "framework/CvarSystem.h"

extern Log::Logger glConfigLogger;
#define logger glConfigLogger

SDL_Window         *window = nullptr;
static SDL_GLContext glContext = nullptr;
static int colorBits = 0;

#ifdef USE_SMP
static void GLimp_SetCurrentContext( bool enable )
{
	if ( enable )
	{
		SDL_GL_MakeCurrent( window, glContext );
	}
	else
	{
		SDL_GL_MakeCurrent( window, nullptr );
	}
}

/*
===========================================================

SMP acceleration

===========================================================
*/

/*
 * I have no idea if this will even work...most platforms don't offer
 * thread-safe OpenGL libraries.
 */

static SDL_mutex  *smpMutex = nullptr;
static SDL_cond   *renderCommandsEvent = nullptr;
static SDL_cond   *renderCompletedEvent = nullptr;
static void ( *renderThreadFunction )() = nullptr;
static SDL_Thread *renderThread = nullptr;

/*
===============
GLimp_RenderThreadWrapper
===============
*/
ALIGN_STACK_FOR_MINGW static int GLimp_RenderThreadWrapper( void* )
{
	// These printfs cause race conditions which mess up the console output
	logger.Notice( "Render thread starting\n" );

	renderThreadFunction();

	GLimp_SetCurrentContext( false );

	logger.Notice( "Render thread terminating\n" );

	return 0;
}

/*
===============
GLimp_SpawnRenderThread
===============
*/
bool GLimp_SpawnRenderThread( void ( *function )() )
{
	static bool warned = false;

	if ( !warned )
	{
		logger.Warn( "You enable r_smp at your own risk!\n" );
		warned = true;
	}

	if ( renderThread != nullptr ) /* hopefully just a zombie at this point... */
	{
		logger.Notice( "Already a render thread? Trying to clean it up...\n" );
		GLimp_ShutdownRenderThread();
	}

	smpMutex = SDL_CreateMutex();

	if ( smpMutex == nullptr )
	{
		logger.Notice( "smpMutex creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return false;
	}

	renderCommandsEvent = SDL_CreateCond();

	if ( renderCommandsEvent == nullptr )
	{
		logger.Notice( "renderCommandsEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return false;
	}

	renderCompletedEvent = SDL_CreateCond();

	if ( renderCompletedEvent == nullptr )
	{
		logger.Notice( "renderCompletedEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return false;
	}

	renderThreadFunction = function;
	renderThread = SDL_CreateThread( GLimp_RenderThreadWrapper, "render thread", nullptr );

	if ( renderThread == nullptr )
	{
		logger.Notice("SDL_CreateThread() returned %s", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return false;
	}

	return true;
}

/*
===============
GLimp_ShutdownRenderThread
===============
*/
void GLimp_ShutdownRenderThread()
{
	if ( renderThread != nullptr )
	{
		GLimp_WakeRenderer( nullptr );
		SDL_WaitThread( renderThread, nullptr );
		renderThread = nullptr;
		glConfig.smpActive = false;
	}

	if ( smpMutex != nullptr )
	{
		SDL_DestroyMutex( smpMutex );
		smpMutex = nullptr;
	}

	if ( renderCommandsEvent != nullptr )
	{
		SDL_DestroyCond( renderCommandsEvent );
		renderCommandsEvent = nullptr;
	}

	if ( renderCompletedEvent != nullptr )
	{
		SDL_DestroyCond( renderCompletedEvent );
		renderCompletedEvent = nullptr;
	}

	renderThreadFunction = nullptr;
}

static volatile void     *smpData = nullptr;
static volatile bool smpDataReady;

/*
===============
GLimp_RendererSleep
===============
*/
void           *GLimp_RendererSleep()
{
	void *data = nullptr;

	GLimp_SetCurrentContext( false );

	SDL_LockMutex( smpMutex );
	{
		smpData = nullptr;
		smpDataReady = false;

		// after this, the front end can exit GLimp_FrontEndSleep
		SDL_CondSignal( renderCompletedEvent );

		while ( !smpDataReady )
		{
			SDL_CondWait( renderCommandsEvent, smpMutex );
		}

		data = ( void * ) smpData;
	}
	SDL_UnlockMutex( smpMutex );

	GLimp_SetCurrentContext( true );

	return data;
}

/*
===============
GLimp_FrontEndSleep
===============
*/
void GLimp_FrontEndSleep()
{
	SDL_LockMutex( smpMutex );
	{
		while ( smpData )
		{
			SDL_CondWait( renderCompletedEvent, smpMutex );
		}
	}
	SDL_UnlockMutex( smpMutex );
}

/*
===============
GLimp_SyncRenderThread
===============
*/
void GLimp_SyncRenderThread()
{
	GLimp_FrontEndSleep();

	GLimp_SetCurrentContext( true );
}

/*
===============
GLimp_WakeRenderer
===============
*/
void GLimp_WakeRenderer( void *data )
{
	GLimp_SetCurrentContext( false );

	SDL_LockMutex( smpMutex );
	{
		ASSERT(smpData == nullptr);
		smpData = data;
		smpDataReady = true;

		// after this, the renderer can continue through GLimp_RendererSleep
		SDL_CondSignal( renderCommandsEvent );
	}
	SDL_UnlockMutex( smpMutex );
}

#else

// No SMP - stubs
void GLimp_RenderThreadWrapper( void* )
{
}

bool GLimp_SpawnRenderThread( void ( * )() )
{
	logger.Warn("SMP support was disabled at compile time" );
	return false;
}

void GLimp_ShutdownRenderThread()
{
}

void *GLimp_RendererSleep()
{
	return nullptr;
}

void GLimp_FrontEndSleep()
{
}

void GLimp_SyncRenderThread()
{
}

void GLimp_WakeRenderer( void* )
{
}

#endif

enum class rserr_t
{
  RSERR_OK,

  RSERR_INVALID_FULLSCREEN,
  RSERR_INVALID_MODE,
  RSERR_OLD_GL,

  RSERR_UNKNOWN
};

cvar_t                     *r_allowResize; // make window resizable
cvar_t                     *r_centerWindow;
cvar_t                     *r_displayIndex;
cvar_t                     *r_sdlDriver;

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown()
{
	logger.Debug("Shutting down OpenGL subsystem" );

	ri.IN_Shutdown();

#if defined( USE_SMP )

	if ( renderThread != nullptr )
	{
		logger.Notice( "Destroying renderer thread...\n" );
		GLimp_ShutdownRenderThread();
	}

#endif

	if ( glContext )
	{
		SDL_GL_DeleteContext( glContext );
		glContext = nullptr;
	}

	if ( window )
	{
		SDL_DestroyWindow( window );
		window = nullptr;
	}

	SDL_QuitSubSystem( SDL_INIT_VIDEO );

	Com_Memset( &glConfig, 0, sizeof( glConfig ) );
	Com_Memset( &glState, 0, sizeof( glState ) );
}

static void GLimp_Minimize()
{
	SDL_MinimizeWindow( window );
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void *a, const void *b )
{
	const float ASPECT_EPSILON = 0.001f;
	SDL_Rect    *modeA = ( SDL_Rect * ) a;
	SDL_Rect    *modeB = ( SDL_Rect * ) b;
	float       aspectA = ( float ) modeA->w / ( float ) modeA->h;
	float       aspectB = ( float ) modeB->w / ( float ) modeB->h;
	int         areaA = modeA->w * modeA->h;
	int         areaB = modeB->w * modeB->h;
	float       aspectDiffA = fabs( aspectA - displayAspect );
	float       aspectDiffB = fabs( aspectB - displayAspect );
	float       aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if ( aspectDiffsDiff > ASPECT_EPSILON )
	{
		return 1;
	}
	else if ( aspectDiffsDiff < -ASPECT_EPSILON )
	{
		return -1;
	}
	else
	{
		return areaA - areaB;
	}
}

/*
===============
GLimp_DetectAvailableModes
===============
*/
static void GLimp_DetectAvailableModes()
{
	char     buf[ MAX_STRING_CHARS ] = { 0 };
	SDL_Rect modes[ 128 ];
	int      numModes = 0;
	int      i;
	SDL_DisplayMode windowMode;
	int      display;

	display = SDL_GetWindowDisplayIndex( window );

	if ( SDL_GetWindowDisplayMode( window, &windowMode ) < 0 )
	{
		logger.Warn("Couldn't get window display mode: %s", SDL_GetError() );
		return;
	}

	for ( i = 0; i < SDL_GetNumDisplayModes( display ); i++ )
	{
		SDL_DisplayMode mode;

		if ( SDL_GetDisplayMode( display, i, &mode ) < 0 )
		{
			continue;
		}

		if ( !mode.w || !mode.h )
		{
			logger.Notice("Display supports any resolution" );
			return;
		}

		if ( windowMode.format != mode.format || windowMode.refresh_rate != mode.refresh_rate )
		{
			continue;
		}

		modes[ numModes ].w = mode.w;
		modes[ numModes ].h = mode.h;
		numModes++;
	}

	if ( numModes > 1 )
	{
		qsort( modes, numModes, sizeof( SDL_Rect ), GLimp_CompareModes );
	}

	for ( i = 0; i < numModes; i++ )
	{
		const char *newModeString = va( "%ux%u ", modes[ i ].w, modes[ i ].h );

		if ( strlen( newModeString ) < sizeof( buf ) - strlen( buf ) )
		{
			Q_strcat( buf, sizeof( buf ), newModeString );
		}
		else
		{
			logger.Warn("Skipping mode %ux%x, buffer too small", modes[ i ].w, modes[ i ].h );
		}
	}

	if ( *buf )
	{
		logger.Notice("Available modes: '%s'", buf );
		ri.Cvar_Set( "r_availableModes", buf );
	}
}

/*
===============
GLimp_SetMode
===============
*/
static rserr_t GLimp_SetMode( int mode, bool fullscreen, bool noborder )
{
	const char  *glstring;
	int         perChannelColorBits;
	int         alphaBits, depthBits, stencilBits;
	int         samples;
	int         i = 0;
	SDL_Surface *icon = nullptr;
	SDL_DisplayMode desktopMode;
	Uint32      flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	int         x, y;
	GLenum      glewResult;
	int         GLmajor, GLminor;
	int         GLEWmajor, GLEWminor, GLEWmicro;

	logger.Notice("Initializing OpenGL display" );

	if ( r_allowResize->integer )
	{
		flags |= SDL_WINDOW_RESIZABLE;
	}

	if ( r_centerWindow->integer )
	{
		// center window on specified display
		x = SDL_WINDOWPOS_CENTERED_DISPLAY( r_displayIndex->integer );
		y = SDL_WINDOWPOS_CENTERED_DISPLAY( r_displayIndex->integer );
	}
	else
	{
		x = SDL_WINDOWPOS_UNDEFINED_DISPLAY( r_displayIndex->integer );
		y = SDL_WINDOWPOS_UNDEFINED_DISPLAY( r_displayIndex->integer );
	}

	icon = SDL_CreateRGBSurfaceFrom( ( void * ) CLIENT_WINDOW_ICON.pixel_data,
			        CLIENT_WINDOW_ICON.width,
			        CLIENT_WINDOW_ICON.height,
			        CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
			        CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
			        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
			        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
					);

	if ( SDL_GetDesktopDisplayMode( r_displayIndex->integer, &desktopMode ) == 0 )
	{
		displayAspect = ( float ) desktopMode.w / ( float ) desktopMode.h;

		logger.Notice("Display aspect: %.3f", displayAspect );
	}
	else
	{
		Com_Memset( &desktopMode, 0, sizeof( SDL_DisplayMode ) );

		logger.Notice("Cannot determine display aspect (%s), assuming 1.333", SDL_GetError() );
	}

	logger.Notice("...setting mode %d:", mode );

	if ( mode == -2 )
	{
		// use desktop video resolution
		if ( desktopMode.h > 0 )
		{
			glConfig.vidWidth = desktopMode.w;
			glConfig.vidHeight = desktopMode.h;
		}
		else
		{
			glConfig.vidWidth = 640;
			glConfig.vidHeight = 480;
			logger.Notice("Cannot determine display resolution, assuming 640x480" );
		}
	}
	else if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, mode ) )
	{
		logger.Notice(" invalid mode" );
		return rserr_t::RSERR_INVALID_MODE;
	}

	logger.Notice(" %d %d", glConfig.vidWidth, glConfig.vidHeight );
	// HACK: We want to set the current value, not the latched value
	Cvar::ClearFlags("r_customwidth", CVAR_LATCH);
	Cvar::ClearFlags("r_customheight", CVAR_LATCH);
	Cvar_Set( "r_customwidth", va("%d", glConfig.vidWidth ) );
	Cvar_Set( "r_customheight", va("%d", glConfig.vidHeight ) );
	Cvar::AddFlags("r_customwidth", CVAR_LATCH);
	Cvar::AddFlags("r_customheight", CVAR_LATCH);

	sscanf( ( const char * ) glewGetString( GLEW_VERSION ), "%d.%d.%d",
		&GLEWmajor, &GLEWminor, &GLEWmicro );
	if( GLEWmajor < 2 ) {
		logger.Warn( "GLEW version < 2.0.0 doesn't support GL core profiles" );
	}

	do
	{
		if ( glContext != nullptr )
		{
			SDL_GL_DeleteContext( glContext );
			glContext = nullptr;
		}

		if ( window != nullptr )
		{
			SDL_GetWindowPosition( window, &x, &y );
			logger.Debug("Existing window at %dx%d before being destroyed", x, y );
			SDL_DestroyWindow( window );
			window = nullptr;
		}
		// we come back here if we couldn't get a visual and there's
		// something we can switch off

		if ( fullscreen )
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}

		if ( noborder )
		{
			flags |= SDL_WINDOW_BORDERLESS;
		}

		colorBits = r_colorbits->integer;

		if ( ( !colorBits ) || ( colorBits >= 32 ) )
		{
			colorBits = 24;
		}

		alphaBits = r_alphabits->integer;

		if ( alphaBits < 0 )
		{
			alphaBits = 0;
		}

		depthBits = r_depthbits->integer;

		if ( !depthBits )
		{
			depthBits = 24;
		}

		stencilBits = r_stencilbits->integer;
		samples = r_ext_multisample->integer;

		for ( i = 0; i < 4; i++ )
		{
			int testColorBits, testCore;
			int major = r_glMajorVersion->integer;
			int minor = r_glMinorVersion->integer;

			// 0 - 24 bit color, core
			// 1 - 24 bit color, compat
			// 2 - 16 bit color, core
			// 3 - 16 bit color, compat
			testColorBits = (i >= 2) ? 16 : 24;
			testCore = ((i & 1) == 0);

			if( testCore && !Q_stricmp(r_glProfile->string, "compat") )
				continue;

			if( testCore && GLEWmajor < 2 )
				continue;

			if( !testCore && !Q_stricmp(r_glProfile->string, "core") )
				continue;

			if( testColorBits > colorBits )
				continue;

			if ( testColorBits == 24 )
			{
				perChannelColorBits = 8;
			}
			else
			{
				perChannelColorBits = 4;
			}

			SDL_GL_SetAttribute( SDL_GL_RED_SIZE, perChannelColorBits );
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, perChannelColorBits );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, perChannelColorBits );
			SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

			if ( !r_glAllowSoftware->integer )
			{
				SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
			}

			if( testCore && (major < 3 || (major == 3 && minor < 2)) ) {
				major = 3;
				minor = 2;
			}

			if( major < 2 || (major == 2 && minor < 1)) {
				major = 2;
				minor = 1;
			}

			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, major );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, minor );

			if ( testCore )
			{
				SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
			}
			else
			{
				SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
			}

			if ( r_glDebugProfile->integer )
			{
				SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
			}

			if ( !window )
			{
				window = SDL_CreateWindow( CLIENT_WINDOW_TITLE, x, y, glConfig.vidWidth, glConfig.vidHeight, flags );

				if ( !window )
				{
					logger.Warn("SDL_CreateWindow failed: %s\n", SDL_GetError() );
					continue;
				}
			}

			SDL_SetWindowIcon( window, icon );

			glContext = SDL_GL_CreateContext( window );

			if ( !glContext )
			{
				logger.Warn("SDL_GL_CreateContext failed: %s\n", SDL_GetError() );
				continue;
			}
			SDL_GL_SetSwapInterval( r_swapInterval->integer );

			// Fill window with a dark grey (#141414) background.
			glClearColor( 0.08f, 0.08f, 0.08f, 1.0f );
			glClear( GL_COLOR_BUFFER_BIT );
			GLimp_EndFrame();

			glConfig.colorBits = testColorBits;
			glConfig.depthBits = depthBits;
			glConfig.stencilBits = stencilBits;
			glConfig2.glCoreProfile = testCore;

			logger.Notice("Using %d Color bits, %d depth, %d stencil display.",
				glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );

			break;
		}

		if ( samples && ( !glContext || !window ) )
		{
			r_ext_multisample->integer = 0;
		}

	} while ( ( !glContext || !window ) && samples );

	SDL_FreeSurface( icon );

	glewResult = glewInit();

#ifdef GLEW_ERROR_NO_GLX_DISPLAY
	if ( glewResult != GLEW_OK && glewResult != GLEW_ERROR_NO_GLX_DISPLAY )
#else
	if ( glewResult != GLEW_OK )
#endif
	{
		// glewInit failed, something is seriously wrong
		Sys::Error( "GLW_StartOpenGL() - could not load OpenGL subsystem: %s", glewGetErrorString( glewResult ) );
	}
	else
	{
		logger.Notice("Using GLEW %s", glewGetString( GLEW_VERSION ) );
	}

	sscanf( ( const char * ) glGetString( GL_VERSION ), "%d.%d", &GLmajor, &GLminor );
	if ( GLmajor < 2 || ( GLmajor == 2 && GLminor < 1 ) )
	{
		// missing shader support, there is no 1.x renderer anymore
		return rserr_t::RSERR_OLD_GL;
	}

	if ( GLmajor < 3 || ( GLmajor == 3 && GLminor < 2 ) )
	{
		// shaders are supported, but not all GL3.x features
		logger.Notice("Using enhanced (GL3) Renderer in GL 2.x mode..." );
	}
	else
	{
		logger.Notice("Using enhanced (GL3) Renderer in GL 3.x mode..." );
		glConfig.driverType = glDriverType_t::GLDRV_OPENGL3;
	}
	GLimp_DetectAvailableModes();

	glstring = ( char * ) glGetString( GL_RENDERER );
	logger.Notice("GL_RENDERER: %s", glstring );

	return rserr_t::RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static bool GLimp_StartDriverAndSetMode( int mode, bool fullscreen, bool noborder )
{
	int numDisplays;

	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		const char *driverName;
		SDL_version v;
		SDL_GetVersion( &v );

		logger.Notice("SDL_Init( SDL_INIT_VIDEO )... " );
		logger.Notice("Using SDL Version %u.%u.%u", v.major, v.minor, v.patch );

		if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE ) == -1 )
		{
			logger.Notice("SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) FAILED (%s)", SDL_GetError() );
			return false;
		}

		driverName = SDL_GetCurrentVideoDriver();

		if ( !driverName )
		{
			Sys::Error( "No video driver initialized\n" );
		}

		logger.Notice("SDL using driver \"%s\"", driverName );
		ri.Cvar_Set( "r_sdlDriver", driverName );
	}

	numDisplays = SDL_GetNumVideoDisplays();

	if ( numDisplays <= 0 )
	{
		Sys::Error( "SDL_GetNumVideoDisplays FAILED (%s)\n", SDL_GetError() );
	}

	AssertCvarRange( r_displayIndex, 0, numDisplays - 1, true );

	if ( fullscreen && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
	{
		logger.Notice("Fullscreen not allowed with in_nograb 1" );
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = false;
		fullscreen = false;
	}

	rserr_t err = GLimp_SetMode(mode, fullscreen, noborder);

	switch ( err )
	{
		case rserr_t::RSERR_INVALID_FULLSCREEN:
			logger.Warn("GLimp: Fullscreen unavailable in this mode" );
			return false;

		case rserr_t::RSERR_INVALID_MODE:
			logger.Warn("GLimp: Could not set the given mode (%d)", mode );
			return false;

		case rserr_t::RSERR_OLD_GL:
			logger.Warn("GLimp: OpenGL too old" );
			return false;

		default:
			break;
	}

	return true;
}

static const int R_MODE_FALLBACK = 3; // 640 * 480

/* Support code for GLimp_Init */

static void reportDriverType( bool force )
{
	static const char *const drivers[] = {
		"integrated", "stand-alone", "OpenGL 3+", "Mesa"
	};
	if (glConfig.driverType > glDriverType_t::GLDRV_UNKNOWN && (unsigned) glConfig.driverType < ARRAY_LEN( drivers ) )
	{
		logger.Notice("%s graphics driver class '%s'",
		           force ? "User has forced" : "Detected",
		           drivers[Util::ordinal(glConfig.driverType)] );
	}
}

static void reportHardwareType( bool force )
{
	static const char *const hardware[] = {
		"generic", "ATI R300"
	};
	if (glConfig.hardwareType > glHardwareType_t::GLHW_UNKNOWN && (unsigned) glConfig.hardwareType < ARRAY_LEN( hardware ) )
	{
		logger.Notice("%s graphics hardware class '%s'",
		           force ? "User has forced" : "Detected",
		           hardware[Util::ordinal(glConfig.hardwareType)] );
	}
}

/*
===============
GLimp_Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
bool GLimp_Init()
{
	glConfig.driverType = glDriverType_t::GLDRV_ICD;

	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_LATCH );
	r_centerWindow = ri.Cvar_Get( "r_centerWindow", "0", 0 );
	r_displayIndex = ri.Cvar_Get( "r_displayIndex", "0", 0 );
	ri.Cvar_Get( "r_availableModes", "", CVAR_ROM );

	ri.Cmd_AddCommand( "minimize", GLimp_Minimize );

	if ( ri.Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		ri.Cvar_Set( "r_mode", va( "%d", R_MODE_FALLBACK ) );
		ri.Cvar_Set( "r_fullscreen", "0" );
		ri.Cvar_Set( "r_centerWindow", "0" );
		ri.Cvar_Set( "r_noBorder", "0" );
		ri.Cvar_Set( "com_abnormalExit", "0" );
	}

	// Create the window and set up the context
	if ( GLimp_StartDriverAndSetMode( r_mode->integer, r_fullscreen->integer, r_noBorder->value ) )
	{
		goto success;
	}

	// Finally, try the default screen resolution
	if ( r_mode->integer != R_MODE_FALLBACK )
	{
		logger.Notice("Setting r_mode %d failed, falling back on r_mode %d", r_mode->integer, R_MODE_FALLBACK );

		if ( GLimp_StartDriverAndSetMode( R_MODE_FALLBACK, false, false ) )
		{
			goto success;
		}
	}

	// Nothing worked, give up
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	return false;

success:
	// These values force the UI to disable driver selection
	glConfig.hardwareType = glHardwareType_t::GLHW_GENERIC;

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, ( char * ) glGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, ( char * ) glGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );

	if ( *glConfig.renderer_string && glConfig.renderer_string[ strlen( glConfig.renderer_string ) - 1 ] == '\n' )
	{
		glConfig.renderer_string[ strlen( glConfig.renderer_string ) - 1 ] = 0;
	}

	Q_strncpyz( glConfig.version_string, ( char * ) glGetString( GL_VERSION ), sizeof( glConfig.version_string ) );

	if ( glConfig.driverType == glDriverType_t::GLDRV_OPENGL3 )
	{
		GLint numExts, i;

		glGetIntegerv( GL_NUM_EXTENSIONS, &numExts );

		glConfig.extensions_string[ 0 ] = '\0';
		for ( i = 0; i < numExts; ++i )
		{
			if ( i != 0 )
			{
				Q_strcat( glConfig.extensions_string, sizeof( glConfig.extensions_string ), ( char * ) " " );
			}
			Q_strcat( glConfig.extensions_string, sizeof( glConfig.extensions_string ), ( char * ) glGetStringi( GL_EXTENSIONS, i ) );
		}
	}
	else
	{
		Q_strncpyz( glConfig.extensions_string, ( char * ) glGetString( GL_EXTENSIONS ), sizeof( glConfig.extensions_string ) );
	}

	if ( Q_stristr( glConfig.renderer_string, "amd " ) ||
	     Q_stristr( glConfig.renderer_string, "ati " ) )
	{
		if ( glConfig.driverType != glDriverType_t::GLDRV_OPENGL3 )
		{
			glConfig.hardwareType = glHardwareType_t::GLHW_R300;
		}
	}

	reportDriverType( false );
	reportHardwareType( false );

	{ // allow overriding where the user really does know better
		cvar_t          *forceGL;
		glDriverType_t   driverType   = glDriverType_t::GLDRV_UNKNOWN;
		glHardwareType_t hardwareType = glHardwareType_t::GLHW_UNKNOWN;

		forceGL = ri.Cvar_Get( "r_glForceDriver", "", CVAR_LATCH );

		if      ( !Q_stricmp( forceGL->string, "icd" ))
		{
			driverType = glDriverType_t::GLDRV_ICD;
		}
		else if ( !Q_stricmp( forceGL->string, "standalone" ))
		{
			driverType = glDriverType_t::GLDRV_STANDALONE;
		}
		else if ( !Q_stricmp( forceGL->string, "opengl3" ))
		{
			driverType = glDriverType_t::GLDRV_OPENGL3;
		}

		forceGL = ri.Cvar_Get( "r_glForceHardware", "", CVAR_LATCH );

		if      ( !Q_stricmp( forceGL->string, "generic" ))
		{
			hardwareType = glHardwareType_t::GLHW_GENERIC;
		}
		else if ( !Q_stricmp( forceGL->string, "r300" ))
		{
			hardwareType = glHardwareType_t::GLHW_R300;
		}

		if ( driverType != glDriverType_t::GLDRV_UNKNOWN )
		{
			glConfig.driverType = driverType;
			reportDriverType( true );
		}

		if ( hardwareType != glHardwareType_t::GLHW_UNKNOWN )
		{
			glConfig.hardwareType = hardwareType;
			reportHardwareType( true );
		}
	}

	// initialize extensions
	GLimp_InitExtensions();

	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init( window );

	return true;
}

/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers
===============
*/
void GLimp_EndFrame()
{
	// don't flip if drawing to front buffer
	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		SDL_GL_SwapWindow( window );
	}
}

/*
===============
GLimp_HandleCvars

Responsible for handling cvars that change the window or GL state
Should only be called by the main thread
===============
*/
void GLimp_HandleCvars()
{
	if ( r_swapInterval->modified )
	{
		/* Set the swap interval for the GL context.

		* -1 : adaptive sync
		* 0 : immediate update
		* 1 : generic sync, updates synchronized with the vertical refresh
		* N : generic sync occurring on Nth vertical refresh
		* -N : adaptive sync occurring on Nth vertical refresh

		For example if screen has 60 Hz refresh rate:

		* -1 will update the screen 60 times per second,
		  using adaptive sync if supported,
		* 0 will update the screen as soon as it can,
		* 1 will update the screen 60 times per second,
		* 2 will update the screen 30 times per second.
		* 3 will update the screen 20 times per second,
		* 4 will update the screen 15 times per second,
		* -4 will update the screen 15 times per second,
		  using adaptive sync if supported,

		About adaptive sync:

		> Some systems allow specifying -1 for the interval,
		> to enable adaptive vsync.
		> Adaptive vsync works the same as vsync, but if you've
		> already missed the vertical retrace for a given frame,
		> it swaps buffers immediately, which might be less
		> jarring for the user during occasional framerate drops.
		> -- https://wiki.libsdl.org/SDL_GL_SetSwapInterval

		About the accepted values:

		> A swap interval greater than 0 means that the GPU may force
		> the CPU to wait due to previously issued buffer swaps.
		> -- https://www.khronos.org/opengl/wiki/Swap_Interval

		> If <interval> is negative, the minimum number of video frames
		> between buffer swaps is the absolute value of <interval>.
		> -- https://www.khronos.org/registry/OpenGL/extensions/EXT/GLX_EXT_swap_control_tear.txt

		The max value is said to be implementation-dependent:

		> The current swap interval and implementation-dependent max
		> swap interval for a particular drawable can be obtained by
		> calling glXQueryDrawable with the attribute […]
		> -- https://www.khronos.org/registry/OpenGL/extensions/EXT/GLX_EXT_swap_control_tear.txt

		About how to deal with errors:

		> If an application requests adaptive vsync and the system
		> does not support it, this function will fail and return -1.
		> In such a case, you should probably retry the call with 1
		> for the interval.
		> -- https://wiki.libsdl.org/SDL_GL_SetSwapInterval

		Given what's written in Swap Interval Khronos page, setting r_finish
		to 1 or 0 to call or not call glFinish may impact the behaviour.
		See https://www.khronos.org/opengl/wiki/Swap_Interval#GPU_vs_CPU_synchronization

		According to the SDL documentation, only arguments from -1 to 1
		are allowed to SDL_GL_SetSwapInterval. But investigation of SDL
		internals shows that larger intervals should work on Linux and
		Windows. See https://github.com/DaemonEngine/Daemon/pull/497
		Only 0 and 1 work on Mac.

		5 and -5 are arbitrarily set as ceiling and floor value
		to prevent mistakes making the game unresponsive. */

		AssertCvarRange( r_swapInterval, -5, 5, true );

		R_SyncRenderThread();

		int sign = r_swapInterval->integer < 0 ? -1 : 1;
		int interval = std::abs( r_swapInterval->integer );

		while ( SDL_GL_SetSwapInterval( sign * interval ) == -1 )
		{
			if ( sign == -1 )
			{
				logger.Warn("Adaptive sync is unsupported, fallback to generic sync: %s", SDL_GetError() );
				sign = 1;
			}
			else
			{
				if ( interval > 1 )
				{
					logger.Warn("Sync interval %d is unsupported, fallback to 1: %s", interval, SDL_GetError() );
					interval = 1;
				}
				else if ( interval == 1 )
				{
					logger.Warn("Sync is unsupported, disabling sync: %s", SDL_GetError() );
					interval = 0;
				}
				else if ( interval == 0 )
				{
					logger.Warn("Can't disable sync, something is wrong: %s", SDL_GetError() );
					break;
				}
			}
		}

		r_swapInterval->modified = false;
	}

	if ( r_fullscreen->modified )
	{
		int sdlToggled = false;
		bool needToToggle = true;
		bool fullscreen = !!( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN );

		if ( r_fullscreen->integer && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
		{
			logger.Notice("Fullscreen not allowed with in_nograb 1" );
			ri.Cvar_Set( "r_fullscreen", "0" );
			r_fullscreen->modified = false;
		}

		// Is the state we want different from the current state?
		needToToggle = !!r_fullscreen->integer != fullscreen;

		if ( needToToggle )
		{
			Uint32 flags = r_fullscreen->integer == 0 ? 0 : SDL_WINDOW_FULLSCREEN;
			sdlToggled = SDL_SetWindowFullscreen( window, flags );

			if ( sdlToggled < 0 )
			{
				Cmd::BufferCommandText("vid_restart");
			}

			ri.IN_Restart();
		}

		r_fullscreen->modified = false;
	}

	if ( r_noBorder->modified )
	{
		SDL_bool bordered = r_noBorder->integer == 0 ? SDL_TRUE : SDL_FALSE;
		SDL_SetWindowBordered( window, bordered );

		r_noBorder->modified = false;
	}

	// TODO: Update r_allowResize using SDL_SetWindowResizable when we have SDL 2.0.5
}
