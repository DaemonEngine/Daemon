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

#include "qcommon/q_shared.h" // Include before SDL.h due to M_PI issue...
#include <SDL3/SDL.h>

#ifdef USE_SMP
#include <SDL3/SDL_thread.h>
#endif

#include "renderer/tr_local.h"
#include "renderer/DetectGLVendors.h"
#include "renderer/GLUtils.h"

#pragma warning(push)
#pragma warning(disable : 4125) // "decimal digit terminates octal escape sequence"
#include "sdl_icon.h"
#pragma warning(pop)

#include "framework/CommandSystem.h"
#include "framework/CvarSystem.h"

static Log::Logger logger("glconfig", "", Log::Level::NOTICE);

static Cvar::Modified<Cvar::Cvar<bool>> r_noBorder(
	"r_noBorder", "draw window without border", Cvar::ARCHIVE, false);
static Cvar::Modified<Cvar::Range<Cvar::Cvar<int>>> r_swapInterval(
	"r_swapInterval", "enable vsync on every Nth frame, negative for apdative", Cvar::ARCHIVE, 0, -5, 5 );

static Cvar::Cvar<std::string> r_glForceHardware(
	"r_glForceHardware", "treat the GPU type as: 'r300' or 'generic'", Cvar::NONE, "");

static Cvar::Cvar<std::string> r_availableModes(
	"r_availableModes", "list of available resolutions", Cvar::ROM, "");

static Cvar::Range<Cvar::Cvar<int>> r_glDebugSeverity(
	"r_glDebugSeverity",
	"minimum severity of r_glDebugProfile messages (1=NOTIFICATION, 2=LOW, 3=MEDIUM, 4=HIGH)",
	Cvar::NONE, 2, 1, 4);

// OpenGL extension cvars.
/* Driver bug: Mesa versions > 24.0.9 produce garbage rendering when bindless textures are enabled,
and the shader compiler crashes with material shaders
24.0.9 is the latest known working version, 24.1.1 is the earliest known broken version
So this defaults to disabled */
static Cvar::Cvar<bool> r_arb_bindless_texture( "r_arb_bindless_texture",
	"Use GL_ARB_bindless_texture if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_buffer_storage( "r_arb_buffer_storage",
	"Use GL_ARB_buffer_storage if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_compute_shader( "r_arb_compute_shader",
	"Use GL_ARB_compute_shader if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_direct_state_access( "r_arb_direct_state_access",
	"Use GL_ARB_direct_state_access if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_framebuffer_object( "r_arb_framebuffer_object",
	"Use GL_ARB_framebuffer_object if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_explicit_uniform_location( "r_arb_explicit_uniform_location",
	"Use GL_ARB_explicit_uniform_location if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_gpu_shader5( "r_arb_gpu_shader5",
	"Use GL_ARB_gpu_shader5 if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_half_float_pixel( "r_arb_half_float_pixel",
	"Use GL_ARB_half_float_pixel if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_half_float_vertex( "r_arb_half_float_vertex",
	"Use GL_ARB_half_float_vertex if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_indirect_parameters( "r_arb_indirect_parameters",
	"Use GL_ARB_indirect_parameters if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_internalformat_query2( "r_arb_internalformat_query2",
	"Use GL_ARB_internalformat_query2 if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_map_buffer_range( "r_arb_map_buffer_range",
	"Use GL_ARB_map_buffer_range if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_multi_draw_indirect( "r_arb_multi_draw_indirect",
	"Use GL_ARB_multi_draw_indirect if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_program_interface_query( "r_arb_program_interface_query",
	"Load GL_ARB_program_interface_query if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_shader_draw_parameters( "r_arb_shader_draw_parameters",
	"Use GL_ARB_shader_draw_parameters if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_shader_atomic_counters( "r_arb_shader_atomic_counters",
	"Use GL_ARB_shader_atomic_counters if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_shader_atomic_counter_ops( "r_arb_shader_atomic_counter_ops",
	"Use GL_ARB_shader_atomic_counter_ops if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_shader_image_load_store( "r_arb_shader_image_load_store",
	"Use GL_ARB_shader_image_load_store if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_shader_storage_buffer_object( "r_arb_shader_storage_buffer_object",
	"Use GL_ARB_shader_storage_buffer_object if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_shading_language_420pack( "r_arb_shading_language_420pack",
	"Use GL_ARB_shading_language_420pack if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_sync( "r_arb_sync",
	"Use GL_ARB_sync if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_texture_barrier( "r_arb_texture_barrier",
	"Use GL_ARB_texture_barrier if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_texture_gather( "r_arb_texture_gather",
	"Use GL_ARB_texture_gather if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_uniform_buffer_object( "r_arb_uniform_buffer_object",
	"Use GL_ARB_uniform_buffer_object if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_arb_vertex_attrib_binding( "r_arb_vertex_attrib_binding",
	"Use GL_ARB_vertex_attrib_binding if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_ext_draw_buffers( "r_ext_draw_buffers",
	"Use GL_EXT_draw_buffers if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_ext_gpu_shader4( "r_ext_gpu_shader4",
	"Use GL_EXT_gpu_shader4 if available", Cvar::NONE, true );
static Cvar::Range<Cvar::Cvar<float>> r_ext_texture_filter_anisotropic( "r_ext_texture_filter_anisotropic",
	"Use GL_EXT_texture_filter_anisotropic if available: anisotropy value", Cvar::NONE, 4.0f, 0.0f, 16.0f );
static Cvar::Cvar<bool> r_ext_texture_float( "r_ext_texture_float",
	"Use GL_EXT_texture_float if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_ext_texture_integer( "r_ext_texture_integer",
	"Use GL_EXT_texture_integer if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_ext_texture_rg( "r_ext_texture_rg",
	"Use GL_EXT_texture_rg if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_khr_debug( "r_khr_debug",
	"Use GL_KHR_debug if available", Cvar::NONE, true );
static Cvar::Cvar<bool> r_khr_shader_subgroup( "r_khr_shader_subgroup",
	"Use GL_KHR_shader_subgroup if available", Cvar::NONE, true );

static Cvar::Cvar<bool> workaround_glDriver_amd_adrenalin_disableBindlessTexture(
	"workaround.glDriver.amd.adrenalin.disableBindlessTexture",
	"Disable ARB_bindless_texture on AMD Adrenalin driver",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glDriver_amd_oglp_disableBindlessTexture(
	"workaround.glDriver.amd.oglp.disableBindlessTexture",
	"Disable ARB_bindless_texture on AMD OGLP driver",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glDriver_mesa_ati_rv300_useFloatVertex(
	"workaround.glDriver.mesa.ati.rv300.useFloatVertex",
	"Use float vertex instead of supported-but-slower half-float vertex on Mesa driver on ATI RV300 hardware",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glDriver_mesa_ati_rv600_disableHyperZ(
	"workaround.glDriver.mesa.ati.rv600.disableHyperZ",
	"Disable Hyper-Z on Mesa driver on RV600 hardware",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glDriver_mesa_broadcom_vc4_useFloatVertex(
	"workaround.glDriver.mesa.broadcom.vc4.useFloatVertex",
	"Use float vertex instead of supported-but-slower half-float vertex on Mesa driver on Broadcom VC4 hardware",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glDriver_mesa_forceS3tc(
	"workaround.glDriver.mesa.forceS3tc",
	"Enable S3TC on Mesa even when libtxc-dxtn is not available",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glDriver_mesa_intel_gma3_forceFragmentShader(
	"workaround.glDriver.mesa.intel.gma3.forceFragmentShader",
	"Force fragment shader on Intel GMA Gen 3 hardware",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glDriver_mesa_intel_gma3_stubOcclusionQuery(
	"workaround.glDriver.mesa.intel.gma3.stubOcclusionQuery",
	"Stub out occlusion query on Intel GMA Gen 3 hardware",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glDriver_mesa_v241_disableBindlessTexture(
	"workaround.glDriver.mesa.v241.disableBindlessTexture",
	"Disable ARB_bindless_texture on Mesa 24.1 driver",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glDriver_nvidia_v340_disableTextureGather(
	"workaround.glDriver.nvidia.v340.disableTextureGather",
	"Disable ARB_texture_gather on Nvidia 340 driver",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glExtension_missingArbFbo_useExtFbo(
	"workaround.glExtension.missingArbFbo.useExtFbo",
	"Use EXT_framebuffer_object and EXT_framebuffer_blit when ARB_framebuffer_object is not available",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glExtension_glsl120_disableShaderDrawParameters(
	"workaround.glExtension.glsl120.disableShaderDrawParameters",
	"Disable ARB_shader_draw_parameters on GLSL 1.20",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glExtension_glsl120_disableGpuShader4(
	"workaround.glExtension.glsl120.disableGpuShader4",
	"Disable EXT_gpu_shader4 on GLSL 1.20",
	Cvar::NONE, true );
static Cvar::Cvar<bool> workaround_glHardware_intel_useFirstProvokinVertex(
	"workaround.glHardware.intel.useFirstProvokinVertex",
	"Use first provoking vertex on Intel hardware supporting ARB_provoking_vertex",
	Cvar::NONE, true );

SDL_Window *window = nullptr;
static SDL_PropertiesID windowProperties;
static SDL_GLContext glContext = nullptr;

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

static SDL_Mutex *smpMutex = nullptr;
static SDL_Condition *renderCommandsEvent = nullptr;
static SDL_Condition *renderCompletedEvent = nullptr;
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
	logger.Notice( "Render thread starting" );

	renderThreadFunction();

	GLimp_SetCurrentContext( false );

	logger.Notice( "Render thread terminating" );

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
		logger.Warn( "You enable r_smp at your own risk!" );
		warned = true;
	}

	if ( renderThread != nullptr ) /* hopefully just a zombie at this point... */
	{
		logger.Notice( "Already a render thread? Trying to clean it up..." );
		GLimp_ShutdownRenderThread();
	}

	smpMutex = SDL_CreateMutex();

	if ( smpMutex == nullptr )
	{
		logger.Notice( "smpMutex creation failed: %s", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return false;
	}

	renderCommandsEvent = SDL_CreateCondition();

	if ( renderCommandsEvent == nullptr )
	{
		logger.Notice( "renderCommandsEvent creation failed: %s", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return false;
	}

	renderCompletedEvent = SDL_CreateCondition();

	if ( renderCompletedEvent == nullptr )
	{
		logger.Notice( "renderCompletedEvent creation failed: %s", SDL_GetError() );
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
		SDL_DestroyCondition( renderCommandsEvent );
		renderCommandsEvent = nullptr;
	}

	if ( renderCompletedEvent != nullptr )
	{
		SDL_DestroyCondition( renderCompletedEvent );
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
		SDL_SignalCondition( renderCompletedEvent );

		while ( !smpDataReady )
		{
			SDL_WaitCondition( renderCommandsEvent, smpMutex );
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
			SDL_WaitCondition( renderCompletedEvent, smpMutex );
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
		SDL_SignalCondition( renderCommandsEvent );
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
  RSERR_RESTART,

  RSERR_INVALID_FULLSCREEN,
  RSERR_INVALID_MODE,
  RSERR_MISSING_GL,
  RSERR_OLD_GL,

  RSERR_UNKNOWN
};

cvar_t                     *r_allowResize; // make window resizable
cvar_t                     *r_displayIndex;
cvar_t                     *r_sdlDriver;

static void GLimp_DestroyContextIfExists();
static void GLimp_DestroyWindowIfExists();

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
		logger.Notice( "Destroying renderer thread..." );
		GLimp_ShutdownRenderThread();
	}

#endif

	GLimp_DestroyContextIfExists();
	GLimp_DestroyWindowIfExists();

	SDL_QuitSubSystem( SDL_INIT_VIDEO );

	ResetStruct( windowConfig );
	ResetStruct( glState );
}

static Cmd::LambdaCmd minimizeCmd(
	"minimize", Cmd::CLIENT, "minimize the window",
	[]( const Cmd::Args & ) { SDL_MinimizeWindow( window ); });

static void SetSwapInterval( int swapInterval )
{
	/* Set the swap interval for the OpenGL context.

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

	> Returns true on success or false on failure;
	> call SDL_GetError() for more information.
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

	R_SyncRenderThread();

	int sign = swapInterval < 0 ? -1 : 1;
	int interval = std::abs( swapInterval );

	while ( !SDL_GL_SetSwapInterval( sign * interval ) )
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
}

struct displayMode_t
{
	int w;
	int h;
};
/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void *a, const void *b )
{
	const float ASPECT_EPSILON = 0.001f;
	displayMode_t *modeA = ( displayMode_t * ) a;
	displayMode_t *modeB = ( displayMode_t * ) b;
	float       aspectA = ( float ) modeA->w / ( float ) modeA->h;
	float       aspectB = ( float ) modeB->w / ( float ) modeB->h;
	int         areaA = modeA->w * modeA->h;
	int         areaB = modeB->w * modeB->h;
	float       aspectDiffA = fabsf( aspectA - windowConfig.displayAspect );
	float       aspectDiffB = fabsf( aspectB - windowConfig.displayAspect );
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
static bool GLimp_DetectAvailableModes()
{
	SDL_DisplayID display = SDL_GetDisplayForWindow( window );

	int allModes;
	SDL_DisplayMode **displayModes = SDL_GetFullscreenDisplayModes( display, &allModes );

	if ( !displayModes )
	{
		Sys::Error( "Couldn't get display modes: %s", SDL_GetError() );
	}

	std::vector<displayMode_t> modes;

	for ( int i = 0; i < allModes; i++ )
	{
		SDL_DisplayMode *mode = displayModes[ i ];

		if ( !mode->w || !mode->h )
		{
			// FIXME is this really a thing? I don't see it in SDL2 or SDL3 documentation
			logger.Notice("Display supports any resolution" );
			SDL_free( displayModes );
			return true;
		}

		if ( !modes.empty() && modes.back().w == mode->w && modes.back().h == mode->h )
		{
			continue;
		}

		modes.push_back( { mode->w, mode->h } );
	}

	SDL_free( displayModes );

	qsort( modes.data(), modes.size(), sizeof( modes[ 0 ] ), GLimp_CompareModes );

	std::string modesString;

	for ( displayMode_t mode : modes )
	{
		if ( !modesString.empty() )
		{
			modesString.push_back( ' ' );
		}

		modesString += Str::Format( "%ux%u", mode.w, mode.h );
	}

	if ( !modesString.empty() )
	{
		logger.Notice("Available modes: %s", modesString );
		Cvar::SetValueForce( r_availableModes.Name(), modesString );
	}

	return true;
}

enum class glProfile {
	UNDEFINED = 0,
	COMPATIBILITY = 1,
	CORE = 2,
};

struct glConfiguration {
	int major;
	int minor;
	glProfile profile;
	int colorBits;
};

static bool operator!=(const glConfiguration& c1, const glConfiguration& c2) {
	return c1.major != c2.major
		|| c1.minor != c2.minor
		|| c1.profile != c2.profile
		|| c1.colorBits != c2.colorBits;
}

static const char* GLimp_getProfileName( glProfile profile )
{
	ASSERT(profile != glProfile::UNDEFINED);
	return profile == glProfile::CORE ? "core" : "compatibility";
}

static std::string ContextDescription( const glConfiguration& configuration )
{
	return Str::Format( "%d-bit OpenGL %d.%d %s",
		configuration.colorBits,
		configuration.major,
		configuration.minor,
		GLimp_getProfileName( configuration.profile ) );
}

static void GLimp_SetAttributes( const glConfiguration &configuration )
{
	// FIXME: 3 * 4 = 12 which is more than 8
	int perChannelColorBits = configuration.colorBits == 24 ? 8 : 4;

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, perChannelColorBits );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, perChannelColorBits );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, perChannelColorBits );

	// Depth/stencil channels are not needed since all 3D rendering is done in FBOs
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 0 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	if ( !r_glAllowSoftware->integer )
	{
		SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
	}

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, configuration.major );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, configuration.minor );

	if ( configuration.profile == glProfile::CORE )
	{
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	}
	else
	{
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
	}

	if ( r_glDebugProfile.Get() )
	{
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
	}
}

// Copied from https://github.com/libsdl-org/SDL/blob/main/docs/README-migration.md
static SDL_Surface *SDL_CreateRGBSurfaceFrom(
	void *pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask )
{
	return SDL_CreateSurfaceFrom( width, height,
	                              SDL_GetPixelFormatForMasks( depth, Rmask, Gmask, Bmask, Amask ),
	                              pixels, pitch);
}

static bool GLimp_CreateWindow( bool fullscreen, bool bordered, const glConfiguration &configuration )
{
	/* The requested attributes should be set before creating
	an OpenGL window.

	-- http://wiki.libsdl.org/SDL_GL_SetAttribute */
	GLimp_SetAttributes( configuration );

	Uint32 flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL;

	if ( r_allowResize->integer )
	{
		flags |= SDL_WINDOW_RESIZABLE;
	}

	SDL_Surface *icon = nullptr;

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

	const char *windowType = nullptr;

	if ( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		windowType = "fullscreen";
	}

	/* We need to set borderless flag even when fullscreen
	because otherwise when disabling fullscreen the window
	will be bordered while the borderless option is enabled. */

	if ( !bordered )
	{
		flags |= SDL_WINDOW_BORDERLESS;

		/* Don't tell fullscreen window is borderless,
		it's meaningless. */
		if ( ! fullscreen )
		{
			windowType = "borderless";
		}
	}

	int x = SDL_WINDOWPOS_CENTERED_DISPLAY( glConfig.sdlDisplayID );
	int y = SDL_WINDOWPOS_CENTERED_DISPLAY( glConfig.sdlDisplayID );

	windowProperties = SDL_CreateProperties();
	if ( !windowProperties )
	{
		Sys::Error( "SDL_CreateProperties failed" );
	}
	SDL_SetStringProperty( windowProperties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, CLIENT_WINDOW_TITLE );
	SDL_SetNumberProperty( windowProperties, SDL_PROP_WINDOW_CREATE_X_NUMBER, x );
	SDL_SetNumberProperty( windowProperties, SDL_PROP_WINDOW_CREATE_Y_NUMBER, y );
	SDL_SetNumberProperty( windowProperties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, windowConfig.vidWidth );
	SDL_SetNumberProperty( windowProperties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, windowConfig.vidHeight );
	SDL_SetNumberProperty( windowProperties, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, flags );
	window = SDL_CreateWindowWithProperties( windowProperties );

	if ( window )
	{
		int w, h;
		SDL_GetWindowPosition( window, &x, &y );
		SDL_GetWindowSize( window, &w, &h );
		logger.Debug( "SDL %s%swindow created at %d,%d with %d×%d size",
			windowType ? windowType : "",
			windowType ? " ": "",
			x, y, w, h );
	}
	else
	{
		logger.Warn( "SDL %d×%d %s%swindow not created",
			windowConfig.vidWidth, windowConfig.vidHeight,
			windowType ? windowType : "",
			windowType ? " ": "" );
		logger.Warn("SDL_CreateWindow failed: %s", SDL_GetError() );
		SDL_DestroyProperties( windowProperties );
		windowProperties = 0;
		return false;
	}

	SDL_SetWindowIcon( window, icon );

	SDL_DestroySurface( icon );

	return true;
}

static void GLimp_DestroyContextIfExists()
{
	if ( glContext != nullptr )
	{
		SDL_GL_DestroyContext( glContext );
		glContext = nullptr;
	}
}

static void GLimp_DestroyWindowIfExists()
{
	// Do not let orphaned context alive.
	GLimp_DestroyContextIfExists();

	if ( window != nullptr )
	{
		int x, y, w, h;
		SDL_GetWindowPosition( window, &x, &y );
		SDL_GetWindowSize( window, &w, &h );
		logger.Debug("Destroying %d×%d SDL window at %d,%d", w, h, x, y );
		SDL_DestroyWindow( window );
		window = nullptr;
		SDL_DestroyProperties( windowProperties );
		windowProperties = 0;
	}
}

static bool GLimp_CreateContext( const glConfiguration &configuration )
{
	GLimp_DestroyContextIfExists();
	glContext = SDL_GL_CreateContext( window );

	if ( glContext == nullptr )
	{
		logger.Debug( "Invalid context: %s", ContextDescription( configuration ) );
		return false;
	}

	if ( glGetString( GL_VERSION ) == nullptr )
	{
		Sys::Error(
			"SDL returned a broken OpenGL context.\n\n"
			"Please report the bug and tell us what is your operating system,\n"
			"OpenGL driver, graphic card, and if you built the game yourself.\n\n"

#if defined(DAEMON_OPENGL_ABI_GLVND)
			"This engine was built with the \"GLVND\" OpenGL ABI,\n"
			"try to reconfigure the build with the \"LEGACY\" one:\n\n"
			"  cmake -DOpenGL_GL_PREFERENCE=LEGACY\n\n"
#endif

			"See: https://github.com/DaemonEngine/Daemon/issues/945"
			);
	}

	return true;
}

/* GLimp_DestroyWindowIfExists checks if window exists before
destroying it so we can call GLimp_RecreateWindowWhenChange even
if no window is created yet. */
static bool GLimp_RecreateWindowWhenChange( const bool fullscreen, const bool bordered, const glConfiguration &configuration )
{
	/* Those values doen't contribute to anything
	when the window isn't created yet. */
	static bool currentFullscreen = false;
	static bool currentBordered = false;
	static int currentWidth = 0;
	static int currentHeight = 0;
	static glConfiguration currentConfiguration = {};

	if ( window == nullptr
		/* We don't care if comparing default values
		is wrong when the window isn't created yet as
		the first thing we do is to overwrite them. */
		|| windowConfig.vidWidth != currentWidth
		|| windowConfig.vidHeight != currentHeight
		|| configuration != currentConfiguration )
	{
		currentWidth = windowConfig.vidWidth;
		currentHeight = windowConfig.vidHeight;
		currentConfiguration = configuration;

		GLimp_DestroyWindowIfExists();

		if ( !GLimp_CreateWindow( fullscreen, bordered, configuration ) )
		{
			return false;
		}
	}

	if ( fullscreen != currentFullscreen )
	{
		if ( !SDL_SetWindowFullscreen( window, fullscreen ) )
		{
			GLimp_DestroyWindowIfExists();

			if ( !GLimp_CreateWindow( fullscreen, bordered, configuration ) )
			{
				return false;
			}

			const char* windowType = fullscreen ? "fullscreen" : "windowed";
			logger.Debug( "SDL window recreated as %s.", windowType );
		}
		else
		{
			const char* windowType = fullscreen ? "fullscreen" : "windowed";
			logger.Debug( "SDL window set as %s.", windowType );
		}
	}

	if ( bordered != currentBordered )
	{
		SDL_SetWindowBordered( window, bordered );

		const char* windowType = bordered ? "bordered" : "borderless";
		logger.Debug( "SDL window set as %s.", windowType );
	}

	currentFullscreen = fullscreen;
	currentBordered = bordered;

	return true;
}

static rserr_t GLimp_SetModeAndResolution( const int mode )
{
	int numDisplays;
	SDL_DisplayID *displayIDs = SDL_GetDisplays( &numDisplays );

	if ( !displayIDs )
	{
		Sys::Error( "SDL_GetDisplays failed: %s", SDL_GetError() );
	}

	if ( numDisplays <= 0 )
	{
		Sys::Error( "SDL_GetDisplays returned 0 displays" );
	}

	glConfig.sdlDisplayID = r_displayIndex->integer >= 0 && r_displayIndex->integer < numDisplays
	                        ? displayIDs[ r_displayIndex->integer ]
	                        : 0; // 0 indicates primary display

	SDL_free( displayIDs );

	const SDL_DisplayMode *desktopMode = SDL_GetDesktopDisplayMode( glConfig.sdlDisplayID );

	if ( desktopMode )
	{
		windowConfig.displayAspect = ( float ) desktopMode->w / ( float ) desktopMode->h;
		logger.Notice( "Display aspect: %.3f", windowConfig.displayAspect );
	}
	else
	{
		windowConfig.displayAspect = 1.333f;
		logger.Warn("Cannot determine display aspect, assuming %.3f: %s", windowConfig.displayAspect, SDL_GetError() );
	}

	windowConfig.displayWidth = desktopMode->w;
	windowConfig.displayHeight = desktopMode->h;

	if ( mode == -2 )
	{
		// use desktop video resolution
		if ( desktopMode->h > 0 )
		{
			windowConfig.vidWidth = desktopMode->w;
			windowConfig.vidHeight = desktopMode->h;
		}
		else
		{
			windowConfig.vidWidth = 640;
			windowConfig.vidHeight = 480;
			logger.Warn("Cannot determine display resolution, assuming %dx%d", windowConfig.vidWidth, windowConfig.vidHeight );
		}

		logger.Notice("Display resolution: %dx%d", windowConfig.vidWidth, windowConfig.vidHeight);
	}
	else if ( !R_GetModeInfo( &windowConfig.vidWidth, &windowConfig.vidHeight, mode ) )
	{
		logger.Notice("Invalid mode %d", mode );
		return rserr_t::RSERR_INVALID_MODE;
	}

	logger.Notice("...setting mode %d: %d×%d", mode, windowConfig.vidWidth, windowConfig.vidHeight );

	return rserr_t::RSERR_OK;
}

static rserr_t GLimp_ValidateBestContext(
	const int GLEWmajor, glConfiguration &bestValidatedConfiguration, glConfiguration& extendedValidationConfiguration )
{
	/* We iterate known OpenGL versions from highest to lowest,
	iterating core profiles first, then compatibility profile,
	iterating 24-bit display first, then 16-bit.

	Once we get a core profile working, we stop and attempt
	to use 3.2 core whatever the highest version we validated.
	The 3.2 is the oldest version with core profile, meaning
	every extension not in 3.2 code are expected to be loaded
	explicitely. This may make extension loading more predictable.

	For debugging and knowledge purpose we log the best supported
	version even if we're not gonna use it.

	User can request explicit version or profile with related cvars.

	We test down to the lowest known OpenGL version so we can provide
	useful and precise error message when an OpenGL version is too low.

	The idea of going from high to low is to make the engine load fast
	on actual hardware likely to support highest core profile version,
	then spend more time on more rarest configuration. The highest
	loading time affects hardware and related drivers that can't run
	then engine anyway.

	For known OpenGL version,
	see https://en.wikipedia.org/wiki/OpenGL#Version_history

	For information about core, compatibility and forward profiles,
	see https://www.khronos.org/opengl/wiki/OpenGL_Context */

	struct {
		int major;
		int minor;
		glProfile profile;
		bool testByDefault;
	} glSupportArray[] {
		{ 4, 6, glProfile::CORE, false },
		{ 4, 5, glProfile::CORE, false },
		{ 4, 4, glProfile::CORE, false },
		{ 4, 3, glProfile::CORE, false },
		{ 4, 2, glProfile::CORE, false },
		{ 4, 1, glProfile::CORE, false },
		{ 4, 0, glProfile::CORE, false },
		{ 3, 3, glProfile::CORE, false },
		{ 3, 2, glProfile::CORE, true },
		{ 3, 1, glProfile::COMPATIBILITY, false },
		{ 3, 0, glProfile::COMPATIBILITY, false },
		{ 2, 1, glProfile::COMPATIBILITY, true },
		{ 2, 0, glProfile::COMPATIBILITY, true },
		{ 1, 5, glProfile::COMPATIBILITY, true },
		{ 1, 4, glProfile::COMPATIBILITY, true },
		{ 1, 3, glProfile::COMPATIBILITY, true },
		{ 1, 2, glProfile::COMPATIBILITY, true },
		{ 1, 1, glProfile::COMPATIBILITY, true },
		{ 1, 0, glProfile::COMPATIBILITY, true },
	};

	logger.Debug( "Validating best OpenGL context." );

	bool needHighestExtended = !!r_glExtendedValidation->integer;
	for ( int colorBits : {24, 16} )
	{
		for ( auto& row : glSupportArray )
		{
			if ( GLEWmajor < 2 && row.profile == glProfile::CORE )
			{
				// GLEW version < 2.0.0 doesn't support OpenGL core profiles.
				continue;
			}

			if ( !needHighestExtended && !row.testByDefault )
			{
				continue;
			}

			glConfiguration testConfiguration;
			testConfiguration.major = row.major;
			testConfiguration.minor = row.minor;
			testConfiguration.profile = row.profile;
			testConfiguration.colorBits = colorBits;

			if ( !GLimp_RecreateWindowWhenChange( false, false, testConfiguration ) )
			{
				return rserr_t::RSERR_INVALID_MODE;
			}

			if ( GLimp_CreateContext( testConfiguration ) )
			{
				if ( needHighestExtended )
				{
					needHighestExtended = false;
					extendedValidationConfiguration = testConfiguration;
				}

				if ( row.testByDefault )
				{
					bestValidatedConfiguration = testConfiguration;
					return rserr_t::RSERR_OK;
				}
			}
		}
	}

	if ( bestValidatedConfiguration.major == 0 )
	{
		return rserr_t::RSERR_MISSING_GL;
	}

	return rserr_t::RSERR_OLD_GL;
}

static glConfiguration GLimp_ApplyCustomOptions( const int GLEWmajor, const glConfiguration &bestConfiguration )
{
	glConfiguration customConfiguration = {};

	if ( bestConfiguration.profile == glProfile::CORE && !Q_stricmp( r_glProfile->string, "compat" ) )
	{
		logger.Debug( "Compatibility profile is forced by r_glProfile" );

		customConfiguration.profile = glProfile::COMPATIBILITY;
	}

	if ( bestConfiguration.profile == glProfile::COMPATIBILITY && !Q_stricmp( r_glProfile->string, "core" ) )
	{
		if ( GLEWmajor < 2 )
		{
			// GLEW version < 2.0.0 doesn't support OpenGL core profiles.
			logger.Debug( "Core profile is ignored from r_glProfile" );
		}
		else
		{
			logger.Debug( "Core profile is forced by r_glProfile" );

			customConfiguration.profile = glProfile::CORE;
		}
	}

	customConfiguration.major = std::max( 0, r_glMajorVersion->integer );
	customConfiguration.minor = std::max( 0, r_glMinorVersion->integer );

	if ( customConfiguration.major == 0 )
	{
		customConfiguration.major = bestConfiguration.major;
		customConfiguration.minor = bestConfiguration.minor;
	}
	else if ( customConfiguration.major == 1 )
	{
		logger.Warn( "OpenGL %d.%d is not supported, trying %d.%d instead",
			customConfiguration.major,
			customConfiguration.minor,
			bestConfiguration.major,
			bestConfiguration.minor );

		customConfiguration.major = bestConfiguration.major;
		customConfiguration.minor = bestConfiguration.minor;
	}
	else
	{
		if ( customConfiguration.major == 3
			&& customConfiguration.minor < 2
			&& customConfiguration.profile == glProfile::UNDEFINED )
		{
			customConfiguration.profile = glProfile::COMPATIBILITY;
		}
		else if ( customConfiguration.major == 2 )
		{
			if ( customConfiguration.profile == glProfile::UNDEFINED )
			{
				customConfiguration.profile = glProfile::COMPATIBILITY;
			}

			if ( customConfiguration.minor == 0 )
			{
				logger.Warn( "OpenGL 2.0 is not supported, trying 2.1 instead" );

				customConfiguration.minor = 1;
			}
		}

		logger.Debug( "GL version %d.%d is forced by r_glMajorVersion and r_glMinorVersion",
			customConfiguration.major,
			customConfiguration.minor );
	}

	if ( customConfiguration.profile == glProfile::UNDEFINED )
	{
		customConfiguration.profile = bestConfiguration.profile;
	}

	customConfiguration.colorBits = std::max( 0, r_colorbits->integer );

	if ( customConfiguration.colorBits == 0 )
	{
		customConfiguration.colorBits = bestConfiguration.colorBits;
	}
	else
	{
		if ( customConfiguration.colorBits != bestConfiguration.colorBits )
		{
			logger.Debug( "Color framebuffer bitness %d is forced by r_colorbits",
				customConfiguration.colorBits );
		}
	}

	return customConfiguration;
}

static bool CreateWindowAndContext(
	bool fullscreen, bool bordered,
	Str::StringRef contextAdjective,
	const glConfiguration& customConfiguration)
{
	if ( !GLimp_RecreateWindowWhenChange( fullscreen, bordered, customConfiguration ) )
	{
		logger.Warn( "Failed to create window for %s context - %s",
			contextAdjective,
			ContextDescription( customConfiguration ) );
		return false;
	}

	if ( !GLimp_CreateContext( customConfiguration ) )
	{
		logger.Warn( "Failed to initialize %s context - %s",
			contextAdjective,
			ContextDescription( customConfiguration ) );
		logger.Warn( "SDL_GL_CreateContext failed: %s", SDL_GetError() );
		return false;
	}

	logger.Notice( "Using %s context - %s",
		contextAdjective,
		ContextDescription( customConfiguration ) );

	return true;
}

static void GLimp_RegisterConfiguration( const glConfiguration& highestConfiguration, const glConfiguration &requestedConfiguration )
{
	glConfig.glHighestMajor = highestConfiguration.major;
	glConfig.glHighestMinor = highestConfiguration.minor;

	glConfig.glRequestedMajor = requestedConfiguration.major;
	glConfig.glRequestedMinor = requestedConfiguration.minor;

	SetSwapInterval( r_swapInterval.Get() );
	r_swapInterval.GetModifiedValue(); // clear modified flag

	{
		/* Make sure we don't silence any useful error that would
		already have happened. */
		GL_CheckErrors();

		// Check if we have a core profile.
		int profileBit;
		glGetIntegerv( GL_CONTEXT_PROFILE_MASK, &profileBit );

		/* OpenGL implementations not supporting core profile like the ones only
		implementing OpenGL version older than 3.2 may raise a GL_INVALID_ENUM
		error while implementations supporting core profile may not raise the
		error when forcing an OpenGL version older than 3.2, so we better want
		to catch and silence this expected error. */

		if ( glGetError() != GL_NO_ERROR )
		{
			glConfig.glCoreProfile = false;
		}
		else
		{
			glConfig.glCoreProfile = ( profileBit == GL_CONTEXT_CORE_PROFILE_BIT );
		}

		glProfile providedProfile = glConfig.glCoreProfile ? glProfile::CORE : glProfile::COMPATIBILITY ;
		const char *providedProfileName = GLimp_getProfileName( providedProfile );

		if ( providedProfile != requestedConfiguration.profile )
		{
			const char *requestedProfileName = GLimp_getProfileName( requestedConfiguration.profile );

			logger.Warn( "Provided OpenGL %s profile is not the same as requested %s profile.",
				providedProfileName,
				requestedProfileName );
		}
		else
		{
			logger.Debug( "Provided OpenGL context uses %s profile.",
			providedProfileName );
		}
	}

	{
		int providedRedChannelColorBits;
		SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &providedRedChannelColorBits );

		glConfig.colorBits = providedRedChannelColorBits == 8 ? 24 : 16;

		if ( requestedConfiguration.colorBits != glConfig.colorBits )
		{
			logger.Warn( "Provided OpenGL %d-bit channel depth is not the same as requested %d-bit depth.",
				glConfig.colorBits, requestedConfiguration.colorBits );
		}
		else
		{
			logger.Debug( "Provided OpenGL context uses %d-bit channel depth.", glConfig.colorBits );
		}
	}

	{
		int GLmajor, GLminor;
		if ( 2 != sscanf( ( const char * ) glGetString( GL_VERSION ), "%d.%d", &GLmajor, &GLminor ) )
		{
			Sys::Error( "Indecipherable GL_VERSION" );
		}

		glConfig.glMajor = GLmajor;
		glConfig.glMinor = GLminor;
	}

	// CONTEXT_FLAGS and forward compatibility were added in OpenGL 3.0
	if ( glConfig.glMajor >= 3 )
	{
		// Check if context is forward compatible.
		int contextFlags;
		glGetIntegerv( GL_CONTEXT_FLAGS, &contextFlags );

		glConfig.glForwardCompatibleContext = contextFlags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT;

		if ( glConfig.glForwardCompatibleContext )
		{
			logger.Debug( "Provided OpenGL context is forward compatible." );
		}
		else
		{
			logger.Debug( "Provided OpenGL context is not forward compatible." );
		}
	}
	else
	{
		glConfig.glForwardCompatibleContext = false;
	}

	// Get our config strings.
	Q_strncpyz( glConfig.vendor_string, ( char * ) glGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, ( char * ) glGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );
	Q_strncpyz( glConfig.version_string, ( char * ) glGetString( GL_VERSION ), sizeof( glConfig.version_string ) );

	if ( *glConfig.renderer_string )
	{
		int last = strlen( glConfig.renderer_string ) - 1;
		if ( glConfig.renderer_string[ last ] == '\n' )
		{
			glConfig.renderer_string[ last ] = '\0';
		}
	}

	logger.Notice("OpenGL vendor: %s", glConfig.vendor_string );
	logger.Notice("OpenGL renderer: %s", glConfig.renderer_string );
	logger.Notice("OpenGL version: %s", glConfig.version_string );
}

static void GLimp_DrawWindow()
{
	// Unhide the window.
	SDL_ShowWindow( window );

	// Fill window with a dark grey (#141414) background.
	glClearColor( 0.08f, 0.08f, 0.08f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT );
	GLimp_EndFrame();
}

static rserr_t GLimp_CheckOpenGLVersion( const glConfiguration &requestedConfiguration )
{
	if ( glConfig.glMajor != requestedConfiguration.major
		|| glConfig.glMinor != requestedConfiguration.minor )
	{
		logger.Warn( "Provided OpenGL %d.%d is not the same as requested %d.%d version",
			glConfig.glMajor,
			glConfig.glMinor,
			requestedConfiguration.major,
			requestedConfiguration.minor );
	}
	else
	{
		logger.Debug( "Provided OpenGL %d.%d version.",
			glConfig.glMajor,
			glConfig.glMinor );
	}

	if ( glConfig.glMajor < 2 || ( glConfig.glMajor == 2 && glConfig.glMinor < 1 ) )
	{
		GLimp_DestroyWindowIfExists();

		// Missing shader support, there is no OpenGL 1.x renderer anymore.
		return rserr_t::RSERR_OLD_GL;
	}

	return rserr_t::RSERR_OK;
}

static void GLimp_CheckGLEW( const glConfiguration &requestedConfiguration )
{
	GLenum glewResult = glewInit();

#ifdef GLEW_ERROR_NO_GLX_DISPLAY
	if ( glewResult != GLEW_OK && glewResult != GLEW_ERROR_NO_GLX_DISPLAY )
#else
	if ( glewResult != GLEW_OK )
#endif
	{
		// glewInit failed, something is seriously wrong

		GLimp_DestroyWindowIfExists();

		Sys::Error( "GLEW initialization failed: %s.\n\n"
			"Engine successfully created\n"
			"%s context,\n"
			"This is a GLEW issue.",
			glewGetErrorString( glewResult ),
			ContextDescription( requestedConfiguration ) );
	}
}

// We should make sure every workaround returns false if restart already happened.
static bool IsSdlVideoRestartNeeded()
{
	/* We call RV600 the first generation of R600 cards, to make a difference
	with RV700 and RV800 cards that are also supported by the Mesa r600 driver.

	The Mesa r600 driver has broken Hyper-Z wth RV600, not RV700 nor RV800. */
	if ( workaround_glDriver_mesa_ati_rv600_disableHyperZ.Get() )
	{
		if ( getenv( "R600_HYPERZ" ) )
		{
			return false;
		}

		if ( glConfig.driverVendor == glDriverVendor_t::MESA
			&& glConfig.hardwareVendor == glHardwareVendor_t::ATI )
		{
			bool foundRv600 = false;

			std::string cardName = "";

			static const std::string codenames[] = {
				// Radeon HD 2000 Series
				"R600", "RV610", "RV630",
				// Radeon HD 3000 Series
				"RV620", "RV635", "RV670",
			};

			for ( auto& codename : codenames )
			{
				cardName = Str::Format( "AMD %s", codename );

				if ( Q_stristr( glConfig.renderer_string, cardName.c_str() ) )
				{
					foundRv600 = true;
					break;
				}
			}

			if ( foundRv600 )
			{
				logger.Warn( "Found buggy Mesa driver with %s card, disabling Hyper-Z.", cardName );

				Sys::SetEnv( "R600_HYPERZ", "false" );

				return true;
			}
		}
	}

	return false;
}

/*
===============
GLimp_SetMode
===============
*/
static rserr_t GLimp_SetMode( const int mode, const bool fullscreen, const bool bordered )
{
	logger.Notice("Initializing OpenGL display" );

	int GLEWmajor;

	{
		const GLubyte * glewVersion = glewGetString( GLEW_VERSION );

		logger.Notice("Using GLEW version %s", glewVersion );

		int GLEWminor, GLEWmicro;

		sscanf( ( const char * ) glewVersion, "%d.%d.%d",
			&GLEWmajor, &GLEWminor, &GLEWmicro );

		if ( GLEWmajor < 2 )
		{
			logger.Warn( "GLEW version < 2.0.0 doesn't support OpenGL core profiles." );
		}
	}

	{
		rserr_t err = GLimp_SetModeAndResolution( mode );

		if ( err != rserr_t::RSERR_OK )
		{
			return err;
		}
	}

	// HACK: We want to set the current value, not the latched value
	Cvar::ClearFlags("r_customwidth", CVAR_LATCH);
	Cvar::ClearFlags("r_customheight", CVAR_LATCH);
	Cvar_Set( "r_customwidth", va("%d", windowConfig.vidWidth ) );
	Cvar_Set( "r_customheight", va("%d", windowConfig.vidHeight ) );
	Cvar::AddFlags("r_customwidth", CVAR_LATCH);
	Cvar::AddFlags("r_customheight", CVAR_LATCH);

	// Reuse best configuration on vid_restart
	// unless glExtendedValidation is modified.
	static glConfiguration bestValidatedConfiguration = {}; // considering only up to OpenGL 3.2
	static glConfiguration extendedValidationResult = {}; // max available OpenGL version for diagnostic purposes

	if ( r_glExtendedValidation->integer && extendedValidationResult.major != 0 )
	{
		logger.Debug( "Previously best validated context: %s", ContextDescription( extendedValidationResult ) );
	}
	else if ( bestValidatedConfiguration.major == 0 || r_glExtendedValidation->integer )
	{
		// Detect best configuration.
		rserr_t err = GLimp_ValidateBestContext( GLEWmajor, bestValidatedConfiguration, extendedValidationResult );

		if ( err != rserr_t::RSERR_OK )
		{
			if ( err == rserr_t::RSERR_OLD_GL )
			{
				// Used by error message.
				glConfig.glMajor = bestValidatedConfiguration.major;
				glConfig.glMinor = bestValidatedConfiguration.minor;
			}

			GLimp_DestroyWindowIfExists();
			return err;
		}
	}

	if ( r_glExtendedValidation->integer )
	{
		logger.Notice( "Highest available context: %s", ContextDescription( extendedValidationResult ) );
	}

	glConfiguration requestedConfiguration = {}; // The one we end up using in CreateContext calls etc.

	// Attempt to apply custom configuration if exists.
	glConfiguration customConfiguration = GLimp_ApplyCustomOptions( GLEWmajor, bestValidatedConfiguration );
	if ( customConfiguration != bestValidatedConfiguration &&
	     CreateWindowAndContext( fullscreen, bordered, "custom", customConfiguration ) )
	{
		requestedConfiguration = customConfiguration;
	}

	if ( requestedConfiguration.major == 0 )
	{
		if ( !CreateWindowAndContext(fullscreen, bordered, "preferred", bestValidatedConfiguration ) )
		{
			GLimp_DestroyWindowIfExists();
			return rserr_t::RSERR_INVALID_MODE;
		}
		requestedConfiguration = bestValidatedConfiguration;
	}

	GLimp_RegisterConfiguration( extendedValidationResult, requestedConfiguration );

	if ( IsSdlVideoRestartNeeded() )
	{
		GLimp_DestroyWindowIfExists();
		return rserr_t::RSERR_RESTART;
	}

	GLimp_DrawWindow();

	{
		rserr_t err = GLimp_CheckOpenGLVersion( requestedConfiguration );

		if ( err != rserr_t::RSERR_OK )
		{
			return err;
		}
	}

	/* GLimp_CheckGLEW() calls Sys::Error directly so it does not return
	in case of error. */

	GLimp_CheckGLEW( requestedConfiguration );

	/* When calling GLimp_CreateContext() some drivers may provide a valid
	context that is unusable while GL_CheckErrors() catches nothing.

	For example 3840×2160 is too large for the Radeon 9700 and
	the Mesa r300 driver may print this error when the requested
	resolution is higher than what is supported by hardware:

	> r300: Implementation error: Render targets are too big in r300_set_framebuffer_state, refusing to bind framebuffer state!

	The engine would later make a segfault when calling GL_SetDefaultState
	from tr_init.cpp if we don't do anything.

	Hopefully it looks like SDL_GetWindowDisplayMode can raise this error:

	> Couldn't find display mode match

	so we catch this SDL error when calling GLimp_DetectAvailableModes()
	and force a safe lower resolution before we start to draw to prevent
	an engine segfault when calling GL_SetDefaultState(). */

	if ( GLimp_DetectAvailableModes() )
	{
		return rserr_t::RSERR_OK;
	}

	// The engine will retry with a default mode.
	return rserr_t::RSERR_UNKNOWN;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static rserr_t GLimp_StartDriverAndSetMode( int mode, bool fullscreen, bool bordered )
{
	// See the SDL wiki page for details: https://wiki.libsdl.org/SDL3/SDL_SetAppMetadataProperty
	SDL_SetAppMetadataProperty( SDL_PROP_APP_METADATA_NAME_STRING, PRODUCT_NAME );
	SDL_SetAppMetadataProperty( SDL_PROP_APP_METADATA_VERSION_STRING, PRODUCT_VERSION );
	SDL_SetAppMetadataProperty( SDL_PROP_APP_METADATA_TYPE_STRING, "game" );

	/* Let X11 and Wayland desktops (Linux, FreeBSD…) associate the game
	window with the XDG .desktop file, with the proper name and icon.
	The .desktop file should have PRODUCT_APPID as base name or set the
	StartupWMClass variable to PRODUCT_APPID. */
	SDL_SetAppMetadataProperty( SDL_PROP_APP_METADATA_IDENTIFIER_STRING, PRODUCT_APPID );

	/* Disable DPI scaling.
	See the SDL wiki page for details: https://wiki.libsdl.org/SDL3/SDL_HINT_VIDEO_WAYLAND_SCALE_TO_DISPLAY */
	SDL_SetHint( SDL_HINT_VIDEO_WAYLAND_SCALE_TO_DISPLAY, "1" );

	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		const char *driverName;

		const int linked = SDL_GetVersion();
		const int compiled = SDL_VERSION;

		logger.Notice("SDL_Init( SDL_INIT_VIDEO )... " );
		logger.Notice("Using SDL version %d.%d.%d (compiled against SDL version %d.%d.%d)",
			SDL_VERSIONNUM_MAJOR(linked),
			SDL_VERSIONNUM_MINOR(linked),
			SDL_VERSIONNUM_MICRO(linked),
			SDL_VERSIONNUM_MAJOR(compiled),
			SDL_VERSIONNUM_MINOR(compiled),
			SDL_VERSIONNUM_MICRO(compiled));

		if ( !SDL_Init( SDL_INIT_VIDEO ) )
		{
			Sys::Error("SDL_Init( SDL_INIT_VIDEO ) failed: %s", SDL_GetError() );
		}

		driverName = SDL_GetCurrentVideoDriver();

		if ( !driverName )
		{
			Sys::Error( "No video driver initialized" );
		}

		logger.Notice("SDL using driver \"%s\"", driverName );
		Cvar_Set( "r_sdlDriver", driverName );
	}

	int numDisplays;
	SDL_DisplayID *displayIDs = SDL_GetDisplays( &numDisplays );

	if ( !displayIDs )
	{
		Sys::Error( "SDL_GetDisplays failed: %s", SDL_GetError() );
	}

#if defined(DAEMON_OPENGL_ABI)
	logger.Notice( "Using OpenGL ABI \"%s\"", DAEMON_OPENGL_ABI_STRING );
#endif

	rserr_t err = GLimp_SetMode(mode, fullscreen, bordered);

	const char* glRequirements =
		"You need a graphics card with drivers supporting at least OpenGL 3.2\n"
		"or OpenGL 2.1 with EXT_framebuffer_object and ARB_vertex_array_object.";

	switch ( err )
	{
		case rserr_t::RSERR_OK:
		case rserr_t::RSERR_RESTART:
			break;

		case rserr_t::RSERR_INVALID_FULLSCREEN:
			logger.Warn("GLimp: Fullscreen unavailable in this mode" );
			break;

		case rserr_t::RSERR_INVALID_MODE:
			logger.Warn("GLimp: Could not set mode %d", mode );
			break;

		case rserr_t::RSERR_MISSING_GL:
			Sys::Error( "OpenGL is not available.\n\n%s", glRequirements );

			// Sys:Error calls OSExit() so the break and the return is unreachable.
			break;

		case rserr_t::RSERR_OLD_GL:
			Sys::Error( "OpenGL %d.%d is too old.\n\n%s", glConfig.glMajor, glConfig.glMinor, glRequirements );

			// Sys:Error calls OSExit() so the break and the return is unreachable.
			break;

		case rserr_t::RSERR_UNKNOWN:
		default:
			logger.Warn("GLimp: Unknown error for mode %d", mode );
			break;
	}

	return err;
}

static GLenum debugTypes[] =
{
	0,
	GL_DEBUG_TYPE_ERROR_ARB,
	GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB,
	GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB,
	GL_DEBUG_TYPE_PORTABILITY_ARB,
	GL_DEBUG_TYPE_PERFORMANCE_ARB,
	GL_DEBUG_TYPE_OTHER_ARB
};

#ifdef _WIN32
#define DEBUG_CALLBACK_CALL __stdcall //APIENTRY
#else
#define DEBUG_CALLBACK_CALL
#endif
static void DEBUG_CALLBACK_CALL GLimp_DebugCallback( GLenum, GLenum type, GLuint,
                                       GLenum severity, GLsizei, const GLchar *message, const void* )
{
	const char *debugTypeName;
	const char *debugSeverity;

	if ( r_glDebugMode.Get() <= Util::ordinal(glDebugModes_t::GLDEBUG_NONE))
	{
		return;
	}

	if ( r_glDebugMode.Get() < Util::ordinal(glDebugModes_t::GLDEBUG_ALL))
	{
		if ( debugTypes[ r_glDebugMode.Get()] != type )
		{
			return;
		}
	}

	switch ( type )
	{
		case GL_DEBUG_TYPE_ERROR:
			debugTypeName = "DEBUG_TYPE_ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			debugTypeName = "DEBUG_TYPE_DEPRECATED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			debugTypeName = "DEBUG_TYPE_UNDEFINED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			debugTypeName = "DEBUG_TYPE_PORTABILITY";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			debugTypeName = "DEBUG_TYPE_PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_OTHER:
			debugTypeName = "DEBUG_TYPE_OTHER";
			break;
		case GL_DEBUG_TYPE_MARKER:
			debugTypeName = "DEBUG_TYPE_MARKER";
			break;
		default:
			debugTypeName = "DEBUG_TYPE_UNKNOWN";
			break;
	}

	int severityNum;

	switch ( severity )
	{
		case GL_DEBUG_SEVERITY_HIGH:
			debugSeverity = "high";
			severityNum = 4;
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			debugSeverity = "med";
			severityNum = 3;
			break;
		case GL_DEBUG_SEVERITY_LOW:
			debugSeverity = "low";
			severityNum = 2;
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			debugSeverity = "notification";
			severityNum = 1;
			break;
		default:
			debugSeverity = "none";
			severityNum = 2;
			break;
	}

	if ( severityNum >= r_glDebugSeverity.Get() )
	{
		Log::defaultLogger.WithoutSuppression().Warn(
			"%s: severity: %s msg: %s", debugTypeName, debugSeverity, message );
	}
}

/*
===============
GLimp_InitExtensions
===============
*/

/* ExtFlag_CORE means the extension is known to be an OpenGL 3 core extension.
The code considers the extension is available even if the extension is not listed
if the driver pretends to support OpenGL Core 3 and we know this extension is part
of OpenGL Core 3. */

enum {
	ExtFlag_NONE,
	ExtFlag_REQUIRED = BIT( 1 ),
	ExtFlag_CORE = BIT( 2 ),
};

static bool LoadExt( int flags, bool hasExt, const char* name, bool test = true )
{
	if ( hasExt || ( flags & ExtFlag_CORE && glConfig.glCoreProfile) )
	{
		if ( test )
		{
			if ( flags & ExtFlag_REQUIRED )
			{
				logger.WithoutSuppression().Notice( "...using required extension GL_%s.", name );
			}
			else
			{
				logger.WithoutSuppression().Notice( "...using optional extension GL_%s.", name );
			}

			if ( glConfig.glEnabledExtensionsString.length() != 0 )
			{
				glConfig.glEnabledExtensionsString += " ";
			}

			glConfig.glEnabledExtensionsString += "GL_";
			glConfig.glEnabledExtensionsString += name;

			return true;
		}
		else
		{
			// Required extension can't be made optional
			ASSERT( !( flags & ExtFlag_REQUIRED ) );

			logger.WithoutSuppression().Notice( "...ignoring optional extension GL_%s.", name );
		}
	}
	else
	{
		if ( flags & ExtFlag_REQUIRED )
		{
			Sys::Error( "Missing required extension GL_%s.", name );
		}
		else
		{
			logger.WithoutSuppression().Notice( "...missing optional extension GL_%s.", name );

			if ( glConfig.glMissingExtensionsString.length() != 0 )
			{
				glConfig.glMissingExtensionsString += " ";
			}

			glConfig.glMissingExtensionsString += "GL_";
			glConfig.glMissingExtensionsString += name;
		}
	}
	return false;
}

#define SILENTLY_CHECK_EXTENSION( ext ) ( GLEW_##ext )

#define LOAD_EXTENSION(flags, ext) LoadExt(flags, GLEW_##ext, #ext)

#define LOAD_EXTENSION_WITH_TEST(flags, ext, test) LoadExt(flags, GLEW_##ext, #ext, test)

glFboShim_t GL_fboShim;

static void GLimp_InitExtensions()
{
	logger.Notice("Initializing OpenGL extensions" );

	Cvar::Latch( r_arb_bindless_texture );
	Cvar::Latch( r_arb_buffer_storage );
	Cvar::Latch( r_arb_compute_shader );
	Cvar::Latch( r_arb_direct_state_access );
	Cvar::Latch( r_arb_explicit_uniform_location );
	Cvar::Latch( r_arb_framebuffer_object );
	Cvar::Latch( r_arb_gpu_shader5 );
	Cvar::Latch( r_arb_half_float_pixel );
	Cvar::Latch( r_arb_half_float_vertex );
	Cvar::Latch( r_arb_indirect_parameters );
	Cvar::Latch( r_arb_internalformat_query2 );
	Cvar::Latch( r_arb_map_buffer_range );
	Cvar::Latch( r_arb_multi_draw_indirect );
	Cvar::Latch( r_arb_shader_atomic_counters );
	Cvar::Latch( r_arb_shader_atomic_counter_ops );
	Cvar::Latch( r_arb_shader_draw_parameters );
	Cvar::Latch( r_arb_shader_image_load_store );
	Cvar::Latch( r_arb_shading_language_420pack );
	Cvar::Latch( r_arb_shader_storage_buffer_object );
	Cvar::Latch( r_arb_sync );
	Cvar::Latch( r_arb_texture_barrier );
	Cvar::Latch( r_arb_texture_gather );
	Cvar::Latch( r_arb_uniform_buffer_object );
	Cvar::Latch( r_arb_vertex_attrib_binding );
	Cvar::Latch( r_ext_draw_buffers );
	Cvar::Latch( r_ext_gpu_shader4 );
	Cvar::Latch( r_ext_texture_filter_anisotropic );
	Cvar::Latch( r_ext_texture_float );
	Cvar::Latch( r_ext_texture_integer );
	Cvar::Latch( r_ext_texture_rg );
	Cvar::Latch( r_khr_debug );
	Cvar::Latch( r_khr_shader_subgroup );

	glConfig.glEnabledExtensionsString = std::string();
	glConfig.glMissingExtensionsString = std::string();

	if ( LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_debug_output,
		r_khr_debug.Get() && r_glDebugProfile.Get() ) )
	{
		glDebugMessageCallback( (GLDEBUGPROCARB)GLimp_DebugCallback, nullptr );
		glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
	}

	/* On OpenGL Core profile the ARB_fragment_program extension doesn't exist and the related getter functions
	return 0. We can assume OpenGL 3 Core hardware is featureful enough to not care about those limits. */
	if ( !glConfig.glCoreProfile )
	{
		if ( LOAD_EXTENSION( ExtFlag_REQUIRED, ARB_fragment_program ) )
		{
			glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &glConfig.maxAluInstructions );
			glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &glConfig.maxTexIndirections );
		}
	}

	// GLSL

	Q_strncpyz( glConfig.shadingLanguageVersionString, ( char * ) glGetString( GL_SHADING_LANGUAGE_VERSION_ARB ),
				sizeof( glConfig.shadingLanguageVersionString ) );
	int majorVersion, minorVersion;
	if ( sscanf( glConfig.shadingLanguageVersionString, "%i.%i", &majorVersion, &minorVersion ) != 2 )
	{
		logger.Warn("unrecognized shading language version string format" );
	}
	glConfig.shadingLanguageVersion = majorVersion * 100 + minorVersion;

	logger.Notice("...using shading language version %i", glConfig.shadingLanguageVersion );


	// OpenGL driver constants.

	glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &glConfig.maxTextureUnits );
	glGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize );
	glGetIntegerv( GL_MAX_3D_TEXTURE_SIZE, &glConfig.max3DTextureSize );
	glGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.maxCubeMapTextureSize );

	// Stubbed or broken drivers may report garbage.

	if ( glConfig.maxTextureUnits < 0 )
	{
		Log::Warn( "Bad GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS value: %d", glConfig.maxTextureUnits );
		glConfig.maxTextureUnits = 0;
	}

	if ( glConfig.maxTextureSize < 0 )
	{
		Log::Warn( "Bad GL_MAX_TEXTURE_SIZE value: %d", glConfig.maxTextureSize );
		glConfig.maxTextureSize = 0;
	}

	if ( glConfig.max3DTextureSize < 0 )
	{
		Log::Warn( "Bad GL_MAX_3D_TEXTURE_SIZE value: %d", glConfig.max3DTextureSize );
		glConfig.max3DTextureSize = 0;
	}

	if ( glConfig.maxCubeMapTextureSize < 0 )
	{
		Log::Warn( "Bad GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB value: %d", glConfig.maxCubeMapTextureSize );
		glConfig.maxCubeMapTextureSize = 0;
	}

	logger.Notice( "...using up to %d texture size.", glConfig.maxTextureSize );
	logger.Notice( "...using up to %d 3D texture size.", glConfig.max3DTextureSize );
	logger.Notice( "...using up to %d cube map texture size.", glConfig.maxCubeMapTextureSize );
	logger.Notice( "...using up to %d texture units.", glConfig.maxTextureUnits );

	// Texture formats and compression.

	// made required in OpenGL 3.0
	glConfig.textureHalfFloatAvailable =  LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_half_float_pixel, r_arb_half_float_pixel.Get() );

	// made required in OpenGL 3.0
	glConfig.textureFloatAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_texture_float, r_ext_texture_float.Get() );

	bool gpuShader4Enabled = r_ext_gpu_shader4.Get();

	if ( gpuShader4Enabled
		&& SILENTLY_CHECK_EXTENSION( EXT_gpu_shader4 )
		&& glConfig.shadingLanguageVersion <= 120
		&& workaround_glExtension_glsl120_disableGpuShader4.Get() )
	{
		// EXT_gpu_shader4 behaves slightly differently when running on GLSL 1.20.
		// See: https://gitlab.freedesktop.org/mesa/mesa/-/issues/12803#note_2819461
		logger.Warn( "Found EXT_gpu_shader4 with incompatible GLSL 1.20, disabling EXT_gpu_shader4." );
		gpuShader4Enabled = false;
	}

	// made required in OpenGL 3.0
	glConfig.gpuShader4Available = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, EXT_gpu_shader4, gpuShader4Enabled );

	// made required in OpenGL 4.0
	glConfig.gpuShader5Available = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_gpu_shader5, r_arb_gpu_shader5.Get() );

	// made required in OpenGL 3.0
	// GL_EXT_texture_integer can be used in shaders only if GL_EXT_gpu_shader4 is also available
	glConfig.textureIntegerAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, EXT_texture_integer, r_ext_texture_integer.Get() )
	  && glConfig.gpuShader4Available;

	// made required in OpenGL 3.0
	glConfig.textureRGAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_texture_rg, r_ext_texture_rg.Get() );

	{
		bool textureGatherEnabled = r_arb_texture_gather.Get();

		/* GT218-based GPU with Nvidia 340.108 driver advertising
		ARB_texture_gather extension is know to fail to compile
		the depthtile1 GLSL shader.

		See https://github.com/DaemonEngine/Daemon/issues/368

		Unfortunately this workaround may also disable the feature for
		all GPUs using this driver even if we don't know if some of them
		are not affected by the bug while advertising this extension, but
		there is no known easy way to detect GT218-based cards. Not all cards
		using 340 driver supports this extension anyway, like the G92 one.

		We can assume cards not using the 340 driver are not GT218 ones and
		are not affected.

		Usually, those GT218 cards are not powerful enough for dynamic
		lighting so it is likely this feature would be disabled to
		get acceptable framerate on this hardware anyway, making the
		need for such extension and the related shader code useless. */
		if ( workaround_glDriver_nvidia_v340_disableTextureGather.Get() )
		{
			if ( glConfig.driverVendor == glDriverVendor_t::NVIDIA
				&& Q_stristr( glConfig.version_string, " NVIDIA 340." ) )
			{
				// No need for WithoutSuppression for something which can only be printed once per renderer restart.
				logger.Warn( "Found buggy Nvidia 340 driver, disabling ARB_texture_gather. ");
				textureGatherEnabled = false;
			}
		}

		// made required in OpenGL 4.0
		glConfig.textureGatherAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_texture_gather, textureGatherEnabled );
	}
	
	if ( workaround_glHardware_intel_useFirstProvokinVertex.Get()
		&& glConfig.hardwareVendor == glHardwareVendor_t::INTEL )
	{
		if ( LOAD_EXTENSION( ExtFlag_NONE, ARB_provoking_vertex ) )
		{
			/* Workaround texture distorsion bug on Intel GPU

			This bug seems to only affect gfx9 but detecting gfx9
			may not be easy.

			See:
			- https://github.com/DaemonEngine/Daemon/issues/909
			- https://gitlab.freedesktop.org/mesa/mesa/-/issues/10224 */
			logger.Warn( "Found Intel hardware and driver with ARB_provoking_vertex, using first vertex convention." );
			glProvokingVertex( GL_FIRST_VERTEX_CONVENTION );
		}
	}

	// made required in OpenGL 1.3
	glConfig.textureCompression = textureCompression_t::TC_NONE;

	/* ExtFlag_REQUIRED could be turned into ExtFlag_NONE if s3tc-to-rgba is implemented.
	See https://github.com/DaemonEngine/Daemon/pull/738 */
	if ( LOAD_EXTENSION( ExtFlag_REQUIRED, EXT_texture_compression_s3tc ) )
	{
		glConfig.textureCompression = textureCompression_t::TC_S3TC;
	}

	// made required in OpenGL 3.0
	glConfig.textureCompressionRGTCAvailable = LOAD_EXTENSION( ExtFlag_CORE, ARB_texture_compression_rgtc );

	// Texture - others
	glConfig.textureAnisotropyAvailable = false;
	glConfig.textureAnisotropy = 0.0f;
	if ( LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, EXT_texture_filter_anisotropic, r_ext_texture_filter_anisotropic.Get() > 0 ) )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureAnisotropy );
		glConfig.textureAnisotropyAvailable = true;

		// Bound texture anisotropy.
		glConfig.textureAnisotropy = std::max( std::min( r_ext_texture_filter_anisotropic.Get(), glConfig.maxTextureAnisotropy ), 1.0f );
	}

	// VAO and VBO

	// made required in OpenGL 3.0
	LOAD_EXTENSION( ExtFlag_REQUIRED | ExtFlag_CORE, ARB_vertex_array_object );

	// made required in OpenGL 2.1
	LOAD_EXTENSION( ExtFlag_REQUIRED | ExtFlag_CORE, ARB_vertex_buffer_object );

	/* We call RV300 the first generation of R300 cards, to make a difference
	with RV400 and RV500 cards that are also supported by the Mesa r300 driver.

	Mesa r300 implements half-float vertex for the RV300 hardware generation,
	but it is likely emulated and it is very slow. We better use float vertex
	instead. */
	{
		bool halfFloatVertexEnabled = r_arb_half_float_vertex.Get();

		if ( halfFloatVertexEnabled && glConfig.driverVendor == glDriverVendor_t::MESA )
		{
			if ( glConfig.hardwareVendor == glHardwareVendor_t::ATI )
			{
				bool foundRv300 = false;

				std::string cardName = "";

				static const std::string codenames[] = {
					"R300", "R350", "R360",
					"RV350", "RV360", "RV370", "RV380",
				};

				for ( auto& codename : codenames )
				{
					cardName = Str::Format( "ATI %s", codename );

					if ( Str::IsPrefix( cardName, glConfig.renderer_string ) )
					{
						foundRv300 = true;
						break;
					}
				}

				/* The RV300 generation only has 64 ALU instructions while RV400 and RV500
				have 512 of them, so we can also use that value to detect RV300. */
				if ( !foundRv300 )
				{
					if ( glConfig.hardwareType == glHardwareType_t::GLHW_R300
						&& glConfig.maxAluInstructions == 64 )
					{
						cardName = "unknown ATI RV3xx";
						foundRv300 = true;
					}
				}

				if ( foundRv300 && workaround_glDriver_mesa_ati_rv300_useFloatVertex.Get() )
				{
					logger.Notice( "Found slow Mesa half-float vertex implementation with %s hardware, disabling ARB_half_float_vertex.", cardName );
					halfFloatVertexEnabled = false;
				}
			}
			else if ( glConfig.hardwareVendor == glHardwareVendor_t::BROADCOM )
			{
				bool foundVc4 = Str::IsPrefix( "VC4 ", glConfig.renderer_string );

				if ( foundVc4 && workaround_glDriver_mesa_broadcom_vc4_useFloatVertex.Get() )
				{
					logger.Notice( "Found slow Mesa half-float vertex implementation with Broadcom VC4 hardware, disabling ARB_half_float_vertex." );
					halfFloatVertexEnabled = false;
				}
			}
		}

		// made required in OpenGL 3.0
		glConfig.halfFloatVertexAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_half_float_vertex, halfFloatVertexEnabled );

		if ( !halfFloatVertexEnabled )
		{
			logger.Notice( "Missing half-float vertex, using float vertex instead." );
		}
	}

	// FBO

	if ( !workaround_glExtension_missingArbFbo_useExtFbo.Get() )
	{
		// made required in OpenGL 3.0
		LOAD_EXTENSION( ExtFlag_REQUIRED | ExtFlag_CORE, ARB_framebuffer_object );
		glFboSetArb();
	}
	else if ( LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_framebuffer_object, r_arb_framebuffer_object.Get() ) )
	{
		glFboSetArb();
	}
	else
	{
		LOAD_EXTENSION( ExtFlag_REQUIRED, EXT_framebuffer_object );

		/* Both EXT_framebuffer_object and EXT_framebuffer_blit are said to be
		parts of ARB_framebuffer_object:

		> So there may be hardware that supports EXT_FBO and not ARB_FBO,
		> even thought they support things like EXT_FBO_blit and other parts
		> of ARB_FBO.
		-- https://www.khronos.org/opengl/wiki/Framebuffer_Object

		Our code is known to require EXT_framebuffer_blit so if we don't find
		ARB_framebuffer_object but find EXT_framebuffer_object we must
		check for EXT_framebuffer_blit too. */
		LOAD_EXTENSION( ExtFlag_REQUIRED, EXT_framebuffer_blit );

		logger.Warn( "Missing ARB_framebuffer_object, using EXT_framebuffer_object with EXT_framebuffer_blit instead." );

		glFboSetExt();
	}

	glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &glConfig.maxColorAttachments );

	// made required in OpenGL 2.0
	glConfig.drawBuffersAvailable = false;
	if ( r_ext_draw_buffers.Get() )
	{
		glGetIntegerv( GL_MAX_DRAW_BUFFERS, &glConfig.maxDrawBuffers );
		glConfig.drawBuffersAvailable = true;
	}

	{
		int formats = 0;

		glGetIntegerv( GL_NUM_PROGRAM_BINARY_FORMATS, &formats );

		if ( formats == 0 )
		{
			// No need for WithoutSuppression for something which can only be printed once per renderer restart.
			logger.Notice("...no program binary formats");
		}

		glConfig.getProgramBinaryAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_get_program_binary, formats > 0 );
	}

	glConfig.bufferStorageAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_buffer_storage, r_arb_buffer_storage.Get() );

	// made required since OpenGL 3.1
	glConfig.uniformBufferObjectAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_uniform_buffer_object, r_arb_uniform_buffer_object.Get() );

	// made required in OpenGL 3.0
	glConfig.mapBufferRangeAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_map_buffer_range, r_arb_map_buffer_range.Get() );

	// made required in OpenGL 3.2
	glConfig.syncAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_sync, r_arb_sync.Get() );

	// made required in OpenGL 4.5
	glConfig.textureBarrierAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_texture_barrier, r_arb_texture_barrier.Get() );

	// made required in OpenGL 4.3
	glConfig.computeShaderAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_compute_shader, r_arb_compute_shader.Get() );

	{
		bool bindlessTextureEnabled = r_arb_bindless_texture.Get();

		if ( bindlessTextureEnabled )
		{
			/* Some of the mesa 24.x driver versions have a bug in their shader compiler
			related to bindless textures, which results in either glitches or the shader
			compiler crashing (when material system is enabled). It is expected to affect
			every Mesa-supported hardware (the bug is in shared NIR code). See:
			- https://gitlab.freedesktop.org/mesa/mesa/-/issues/11535
			- https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/30315
			- https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/30338
			- https://gitlab.freedesktop.org/mesa/piglit/-/merge_requests/932 */
			if ( glConfig.driverVendor == glDriverVendor_t::MESA )
			{
				const char *str1 = "Mesa 24.";

				const char *match1 = Q_stristr( glConfig.version_string, str1 );

				if ( match1 )
				{
					match1 += strlen( str1 );

					std::string str2 = "";
					std::string str3 = "";

					bool foundMesa241 = false;

					static const std::pair<std::string, std::vector<std::string>> versions[] = {
						/* Most 24.1.0-devel (after bug is introduced),
						some 24.1.6-devel (before bug is fixed),
						and all 24.1.0-rc[1-4] versions are buggy.
						It is fixed starting with 24.1.7 (backport from 24.2.1). */
						{ "1.",  { "0", "1", "2", "3", "4", "5", "6", } },
						/* Most 24.2.0-devel (before bug is fixed),
						and all 24.2.0-rc[1-4] versions are buggy.
						It is fixed starting with 24.2.1. */
						{ "2.", { "0", } },
					};

					for ( auto& vp : versions )
					{
						if ( Str::IsPrefix( vp.first, match1 ) )
						{
							const char *match2 = match1 + vp.first.length();
							str2 = vp.first;

							for ( auto& v : vp.second )
							{
								if ( Str::IsPrefix( v, match2 ) )
								{
									foundMesa241 = true;
									str3 = v;
									break;
								}
							}

							break;
						}
					}

					if ( foundMesa241 && workaround_glDriver_mesa_v241_disableBindlessTexture.Get() ) {
						logger.Notice( "^1Found buggy %s%s%s driver, disabling ARB_bindless_texture.",
							str1, str2, str3 );
						bindlessTextureEnabled = false;
					}
				}
			}

			// AMD proprietary drivers are known to have buggy bindless texture implementation.
			else if ( glConfig.hardwareVendor == glHardwareVendor_t::ATI )
			{
				// AMD proprietary driver for macOS does not implement bindless texture.
				// Other systems like FreeBSD don't have AMD proprietary drivers.

				bool foundOglp = false;
				bool foundAdrenalin = false;

				#if defined(__linux__)
					foundOglp = true;
				#elif defined(_WIN32)
					/* AMD OGLP driver for Linux shares the same vendor string than AMD Adrenalin driver
					for Windows and AMD ATI driver for macOS. When running the Windows engine binary on
					Wine we must check we're not running Windows or macOS to detect Linux OGLP. */
					if ( Sys::isRunningOnWine() )
					{
						const char* system = Sys::getWineHostSystem();

						if ( system && !strcmp( system, "Linux" ) )
						{
							foundOglp = true;
						}
					}
					else
					{
						foundAdrenalin = true;
					}
				#endif

				if ( foundOglp && workaround_glDriver_amd_oglp_disableBindlessTexture.Get() )
				{
					logger.Notice( "^1Found buggy AMD OGLP driver, disabling ARB_bindless_texture." );
					bindlessTextureEnabled = false;
				}

				if ( foundAdrenalin && workaround_glDriver_amd_adrenalin_disableBindlessTexture.Get() )
				{
					logger.Notice( "^1Found buggy AMD Adrenalin driver, disabling ARB_bindless_texture." );
					bindlessTextureEnabled = false;
				}
			}
		}

		// not required by any OpenGL version
		glConfig.bindlessTexturesAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_bindless_texture, bindlessTextureEnabled );
	}

	bool shaderDrawParametersEnabled = r_arb_shader_draw_parameters.Get();

	if ( shaderDrawParametersEnabled
		&& SILENTLY_CHECK_EXTENSION( ARB_shader_draw_parameters )
		&& glConfig.shadingLanguageVersion <= 120
		&& workaround_glExtension_glsl120_disableShaderDrawParameters.Get() )
	{
		logger.Warn( "Found ARB_shader_draw_parameters with incompatible GLSL 1.20, disabling ARB_shader_draw_parameters." );
		shaderDrawParametersEnabled = false;
	}

	// made required in OpenGL 4.6
	glConfig.shaderDrawParametersAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_shader_draw_parameters, shaderDrawParametersEnabled );

	// made required in OpenGL 4.3
	// We don't use it but the ARB_shader_storage_buffer_object spec says "OpenGL 4.3 or ARB_program_interface_query is required" and
	// Intel's driver interprets that as meaning we must explicitly load the extension for SSBOs to work?
	// But don't stop ourselves from using SSBOs if this fails.
	LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_program_interface_query, r_arb_program_interface_query.Get() );

	// made required in OpenGL 4.3
	glConfig.SSBOAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_shader_storage_buffer_object, r_arb_shader_storage_buffer_object.Get() );

	// made required in OpenGL 4.0
	glConfig.multiDrawIndirectAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_multi_draw_indirect, r_arb_multi_draw_indirect.Get() );

	// made required in OpenGL 4.2
	glConfig.shadingLanguage420PackAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_shading_language_420pack, r_arb_shading_language_420pack.Get() );

	// made required in OpenGL 4.3
	glConfig.explicitUniformLocationAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_explicit_uniform_location, r_arb_explicit_uniform_location.Get() );

	// made required in OpenGL 4.2
	glConfig.shaderImageLoadStoreAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_shader_image_load_store, r_arb_shader_image_load_store.Get() );

	// made required in OpenGL 4.2
	glConfig.shaderAtomicCountersAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_shader_atomic_counters, r_arb_shader_atomic_counters.Get() );

	// made required in OpenGL 4.6
	glConfig.shaderAtomicCounterOpsAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_shader_atomic_counter_ops, r_arb_shader_atomic_counter_ops.Get() );

	// made required in OpenGL 4.6
	glConfig.indirectParametersAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_indirect_parameters, r_arb_indirect_parameters.Get() );

	// made required in OpenGL 4.5
	glConfig.directStateAccessAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_direct_state_access, r_arb_direct_state_access.Get() );

	// made required in OpenGL 4.3
	glConfig.vertexAttribBindingAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_vertex_attrib_binding, r_arb_vertex_attrib_binding.Get() );

	glConfig.geometryCacheAvailable = glConfig.vertexAttribBindingAvailable && glConfig.directStateAccessAvailable;

	glConfig.materialSystemAvailable =
		glConfig.bindlessTexturesAvailable
		&& glConfig.computeShaderAvailable
		&& glConfig.directStateAccessAvailable
		&& glConfig.explicitUniformLocationAvailable
		&& glConfig.geometryCacheAvailable
		&& glConfig.gpuShader4Available
		&& glConfig.indirectParametersAvailable
		&& glConfig.multiDrawIndirectAvailable
		&& glConfig.shaderAtomicCountersAvailable
		&& glConfig.shaderDrawParametersAvailable
		&& glConfig.shaderImageLoadStoreAvailable
		&& glConfig.shadingLanguage420PackAvailable
		&& glConfig.SSBOAvailable
		&& glConfig.uniformBufferObjectAvailable;

	// This requires GLEW 2.2+, so skip if it's a lower version
#if defined(GLEW_KHR_shader_subgroup)
	// not required by any OpenGL version
	glConfig.shaderSubgroupAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, KHR_shader_subgroup, r_khr_shader_subgroup.Get() );

	if ( glConfig.shaderSubgroupAvailable ) {
		int subgroupFeatures;
		glGetIntegerv( GL_SUBGROUP_SUPPORTED_FEATURES_KHR, &subgroupFeatures );

		glConfig.shaderSubgroupBasicAvailable = subgroupFeatures & GL_SUBGROUP_FEATURE_BASIC_BIT_KHR;
		glConfig.shaderSubgroupVoteAvailable = subgroupFeatures & GL_SUBGROUP_FEATURE_VOTE_BIT_KHR;
		glConfig.shaderSubgroupArithmeticAvailable = subgroupFeatures & GL_SUBGROUP_FEATURE_ARITHMETIC_BIT_KHR;
		glConfig.shaderSubgroupBallotAvailable = subgroupFeatures & GL_SUBGROUP_FEATURE_BALLOT_BIT_KHR;
		glConfig.shaderSubgroupShuffleAvailable = subgroupFeatures & GL_SUBGROUP_FEATURE_SHUFFLE_BIT_KHR;
		glConfig.shaderSubgroupShuffleRelativeAvailable = subgroupFeatures & GL_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT_KHR;
		glConfig.shaderSubgroupQuadAvailable = subgroupFeatures & GL_SUBGROUP_FEATURE_QUAD_BIT_KHR;

		Log::Notice( "Supported subgroup extensions: basic: %s, vote: %s, arithmetic: %s, ballot: %s, shuffle: %s, "
			"shuffle_relative: %s, quad: %s", glConfig.shaderSubgroupBasicAvailable,
			glConfig.shaderSubgroupVoteAvailable, glConfig.shaderSubgroupArithmeticAvailable,
			glConfig.shaderSubgroupBallotAvailable, glConfig.shaderSubgroupShuffleAvailable,
			glConfig.shaderSubgroupShuffleRelativeAvailable, glConfig.shaderSubgroupQuadAvailable );
	}
#else
	glConfig.shaderSubgroupAvailable = false;

	glConfig.shaderSubgroupBasicAvailable = false;
	glConfig.shaderSubgroupVoteAvailable = false;
	glConfig.shaderSubgroupArithmeticAvailable = false;
	glConfig.shaderSubgroupBallotAvailable = false;
	glConfig.shaderSubgroupShuffleAvailable = false;
	glConfig.shaderSubgroupShuffleRelativeAvailable = false;
	glConfig.shaderSubgroupQuadAvailable = false;

	// Currently this functionality is only used by material system shaders
	if ( glConfig.materialSystemAvailable ) {
		logger.Notice( "^1Using outdated GLEW version, GL_KHR_shader_subgroup unavailable."
			"Update GLEW to 2.2+ to be able to use this extension" );
	}
#endif

	// Shader limits.

	// From GL_ARB_vertex_shader.
	glGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig.maxVertexUniforms );

	// From GL_ARB_vertex_program.
	glGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig.maxVertexAttribs );

	// From GL_ARB_uniform_buffer_object.
	if ( glConfig.uniformBufferObjectAvailable )
	{
		glGetIntegerv( GL_MAX_UNIFORM_BLOCK_SIZE, &glConfig.maxUniformBlockSize );
	}

	int reservedComponents = 36 * 10; // approximation how many uniforms we have besides the bone matrices
	glConfig.maxVertexSkinningBones = Math::Clamp( ( glConfig.maxVertexUniforms - reservedComponents ) / 16, 0, MAX_BONES );
	glConfig.vboVertexSkinningAvailable = r_vboVertexSkinning->integer && ( ( glConfig.maxVertexSkinningBones >= 12 ) ? true : false );

	GL_CheckErrors();
}

static const int R_MODE_FALLBACK = 3; // 640 * 480

/* Support code for GLimp_Init */

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
	glConfig.driverType = glDriverType_t::GLDRV_OPENGL3;

	r_sdlDriver = Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
	r_allowResize = Cvar_Get( "r_allowResize", "0", CVAR_LATCH );
	r_displayIndex = Cvar_Get( "r_displayIndex", "0", 0 );

	Cvar::Latch( workaround_glDriver_amd_adrenalin_disableBindlessTexture );
	Cvar::Latch( workaround_glDriver_amd_oglp_disableBindlessTexture );
	Cvar::Latch( workaround_glDriver_mesa_ati_rv300_useFloatVertex );
	Cvar::Latch( workaround_glDriver_mesa_ati_rv600_disableHyperZ );
	Cvar::Latch( workaround_glDriver_mesa_broadcom_vc4_useFloatVertex );
	Cvar::Latch( workaround_glDriver_mesa_forceS3tc );
	Cvar::Latch( workaround_glDriver_mesa_intel_gma3_forceFragmentShader );
	Cvar::Latch( workaround_glDriver_mesa_intel_gma3_stubOcclusionQuery );
	Cvar::Latch( workaround_glDriver_mesa_v241_disableBindlessTexture );
	Cvar::Latch( workaround_glDriver_nvidia_v340_disableTextureGather );
	Cvar::Latch( workaround_glExtension_missingArbFbo_useExtFbo );
	Cvar::Latch( workaround_glExtension_glsl120_disableShaderDrawParameters );
	Cvar::Latch( workaround_glExtension_glsl120_disableGpuShader4 );
	Cvar::Latch( workaround_glHardware_intel_useFirstProvokinVertex );

	/* Enable S3TC on Mesa even if libtxc-dxtn is not available
	The environment variables is currently always set,
	it should do nothing with other systems and drivers.

	It should also be set on Win32 when running on Wine
	on Linux anyway. */
	if ( workaround_glDriver_mesa_forceS3tc.Get() )
	{
		Sys::SetEnv( "force_s3tc_enable", "true" );
	}

	/* Enable 2.1 GL on Intel GMA Gen 3 on Linux Mesa driver.

	Mesa provides limited ARB_fragment_shader support and a stub
	for ARB_occlusion_query implementation on GMA Gen 3, making
	possible to enable OpenGL 2.1 on such hardware.

	The Mesa i915 driver for GMA Gen 3 disabled GL 2.1 on such
	hardware to force Google Chrome to use its CPU fallback
	that was faster but we don't implement such fallback.
	See https://gitlab.freedesktop.org/mesa/mesa/-/commit/a1891da7c865c80d95c450abfc0d2bc49db5f678

	Only Mesa i915 on Linux supports GL 2.1 for GMA Gen 3,
	so there is no similar tweak available for Windows and macOS.

	Mesa i915 and macOS also supports GL 2.1 on GMA Gen 4
	(while windows drivers don't) and those tweaks are not
	required as the related features are enabled by default.

	First Intel hardware range expected to have drivers
	supporting GL 2.1 on Windows is GMA Gen 5.

	Enabling those options will at least make the engine
	properly report missing extensions instead of missing
	GL version, for example the Intel GMA 3100 G33 (Gen 3)
	will report missing GL_ARB_half_float_vertex extension
	instead of missing OpenGL 2.1 version. This will make
	the engine runs on such hardware once float vertex
	is implemented.

	The GMA 3150 is known to have wider OpenGL support than
	GMA 3100, for example it has OpenGL version similar to
	GMA 4 on Windows while being a GMA 3 so the list of
	available GL extensions may be different.

	The environment variables are currently always set, they
	should do nothing with other systems and drivers. They
	should also be set when running Windows binaries running
	on Wine on Linux anyway. So we better always set them. */
	if ( workaround_glDriver_mesa_intel_gma3_forceFragmentShader.Get() )
	{
		Sys::SetEnv( "fragment_shader", "true" );
	}

	if ( workaround_glDriver_mesa_intel_gma3_stubOcclusionQuery.Get() )
	{
		Sys::SetEnv( "stub_occlusion_query", "true" );
	}

	int mode = r_mode->integer;
	bool fullscreen = r_fullscreen.Get();
	bool bordered = !r_noBorder.Get();

	// Create the window and set up the context
	rserr_t err = GLimp_StartDriverAndSetMode( mode, fullscreen, bordered );

	if ( err == rserr_t::RSERR_RESTART )
	{
		Log::Warn( "...restarting SDL Video" );
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
		err = GLimp_StartDriverAndSetMode( mode, fullscreen, bordered );
	}

	if ( err != rserr_t::RSERR_OK )
	{
		// Finally, try the default screen resolution
		if ( mode != R_MODE_FALLBACK )
		{
			logger.Notice("Setting r_mode %d failed, falling back on r_mode %d", mode, R_MODE_FALLBACK );

			err = GLimp_StartDriverAndSetMode( R_MODE_FALLBACK, false, true );
		}
	}

	if ( err != rserr_t::RSERR_OK )
	{
		// Nothing worked, give up
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
		return false;
	}

	// These values force the UI to disable driver selection
	glConfig.hardwareType = glHardwareType_t::GLHW_GENERIC;

	DetectGLVendors(
		glConfig.vendor_string,
		glConfig.version_string,
		glConfig.renderer_string,
		glConfig.hardwareVendor,
		glConfig.driverVendor );

	Log::Debug( "Detected OpenGL hardware vendor: %s", GetGLHardwareVendorName( glConfig.hardwareVendor ) );
	Log::Debug( "Detected OpenGL driver vendor: %s", GetGLDriverVendorName( glConfig.driverVendor ) );

	glConfig.glExtensionsString = std::string();

	if ( glConfig.glMajor >= 3 )
	{
		GLint numExts, i;

		// NUM_EXTENSIONS and glGetStringi( GL_EXTENSIONS, i ) were added in OpenGL 3.0
		glGetIntegerv( GL_NUM_EXTENSIONS, &numExts );

		logger.Debug( "Found %d OpenGL extensions.", numExts );

		std::string glExtensionsString = std::string();

		for ( i = 0; i < numExts; ++i )
		{
			char* s = ( char * ) glGetStringi( GL_EXTENSIONS, i );

			/* Check for errors when fetching string.

			> If an error is generated, glGetString returns 0. 
			> -- https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetString.xhtml */
			if ( s == nullptr )
			{
				logger.Warn( "Error when fetching OpenGL extension list." );
			}
			else
			{
				std::string extensionName = s;

				if ( i != 0 )
				{
					glExtensionsString += " ";
				}

				glExtensionsString += extensionName;
			}
		}

		logger.Debug( "OpenGL extensions found: %s", glExtensionsString );

		glConfig.glExtensionsString = glExtensionsString;
	}
	else
	{
		// glGetString( GL_EXTENSIONS ) was deprecated in OpenGL 3.0
		char* extensions_string = ( char * ) glGetString( GL_EXTENSIONS );

		if ( extensions_string == nullptr )
		{
			logger.Warn( "Error when fetching OpenGL extension list." );
		}
		else
		{
			std::string glExtensionsString = extensions_string;

			int numExts = std::count(glExtensionsString.begin(), glExtensionsString.end(), ' ');

			logger.Debug( "Found %d OpenGL extensions.", numExts );

			logger.Debug( "OpenGL extensions found: %s", glExtensionsString );

			glConfig.glExtensionsString = glExtensionsString;
		}
	}

	if ( glConfig.hardwareVendor == glHardwareVendor_t::ATI &&
	     std::make_pair( glConfig.glMajor, glConfig.glMinor ) < std::make_pair( 3, 2 ) )
	{
		glConfig.hardwareType = glHardwareType_t::GLHW_R300;
	}

	reportHardwareType( false );

	{ // allow overriding where the user really does know better
		Cvar::Latch( r_glForceHardware );
		glHardwareType_t hardwareType = glHardwareType_t::GLHW_UNKNOWN;

		if      ( Str::IsIEqual( r_glForceHardware.Get(), "generic" ) )
		{
			hardwareType = glHardwareType_t::GLHW_GENERIC;
		}
		else if ( Str::IsIEqual( r_glForceHardware.Get(), "r300" ) )
		{
			hardwareType = glHardwareType_t::GLHW_R300;
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

Responsible for handling cvars that change the window or OpenGL state,
should only be called by the main thread.
===============
*/
void GLimp_HandleCvars()
{
	if ( Util::optional<int> swapInterval = r_swapInterval.GetModifiedValue() )
	{
		SetSwapInterval( *swapInterval );
	}

	if ( Util::optional<bool> wantFullscreen = r_fullscreen.GetModifiedValue() )
	{
		bool needToToggle = true;
		bool fullscreen = !!( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN );

		// Is the state we want different from the current state?
		needToToggle = *wantFullscreen != fullscreen;

		if ( needToToggle )
		{
			if ( !SDL_SetWindowFullscreen( window, *wantFullscreen ) )
			{
				Log::Warn( "SDL_SetWindowFullscreen failed: %s", SDL_GetError() );
				Log::Warn( "Trying vid_restart" );
				Cmd::BufferCommandText("vid_restart");
			}
		}
	}

	if ( Util::optional<bool> noBorder = r_noBorder.GetModifiedValue() )
	{
		bool bordered = !*noBorder;
		SDL_SetWindowBordered( window, bordered );
	}

	// TODO: Update r_allowResize using SDL_SetWindowResizable when we have SDL 2.0.5
}

// Never use GLimp_LogComment_() directly, use the GLIMP_LOGCOMMENT() wrapper instead.
void GLimp_LogComment_( std::string comment )
{
	static char buf[ 4096 ];

	Q_snprintf( buf, sizeof( buf ), "%s\n", comment.c_str() );

	glDebugMessageInsertARB(
		GL_DEBUG_SOURCE_APPLICATION_ARB,
		GL_DEBUG_TYPE_OTHER_ARB,
		0,
		GL_DEBUG_SEVERITY_MEDIUM_ARB,
		strlen( buf ), buf );
}

class SetWindowOriginCmd : public Cmd::StaticCmd
{
public:
	SetWindowOriginCmd() : StaticCmd("setWindowOrigin", Cmd::CLIENT, "move the window") {}

	void Run( const Cmd::Args &args ) const
	{
		int x, y;
		if ( args.Argc() != 3
			|| !Str::ParseInt( x, args.Argv( 1 ) ) || !Str::ParseInt( y, args.Argv( 2 ) ) )
		{
			Print( "Usage: setWindowOrigin <x> <y>" );
			return;
		}

		SDL_SetWindowPosition( window, x, y );
	}
};
static SetWindowOriginCmd setWindowOriginCmdRegistration;
