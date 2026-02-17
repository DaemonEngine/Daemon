/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2025 Daemon Developers
All rights reserved.

This file is part of the Daemon BSD Source Code (Daemon Source Code).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the Daemon developers nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===========================================================================
*/
// Surface.cpp

#include <SDL3/SDL_video.h>

#include "engine/qcommon/q_shared.h"

#include "../GraphicsCore/GraphicsCoreCVars.h"

#include "Surface.h"

void Surface::Init() {
	SDL_InitSubSystem( SDL_INIT_VIDEO );

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

	int displayCount;
	SDL_DisplayID* displayIDs = SDL_GetDisplays( &displayCount );

	if ( !displayIDs ) {
		Sys::Error( "SDL_GetDisplays failed: %s", SDL_GetError() );
	}

	if ( displayCount <= 0 ) {
		Sys::Error( "SDL_GetDisplays returned 0 displays" );
	}

	const int displayID = r_displayIndex.Get() >= 0 && r_displayIndex.Get() < displayCount ? displayIDs[r_displayIndex.Get()] : 0;

	SDL_free( displayIDs );

	int width;
	int height;
	const SDL_DisplayMode* displayMode = SDL_GetDesktopDisplayMode( displayID );

	#ifdef _MSC_VER
		// hmonitor = SDL_GetPointerProperty( SDL_GetDisplayProperties( displayID ), SDL_PROP_DISPLAY_WINDOWS_HMONITOR_POINTER );
	#endif

	switch ( r_mode.Get() ) {
		case -2:
		default:
			width  = displayMode->w;
			height = displayMode->h;

			break;

		case -1:
			width  = r_customWidth.Get();
			height = r_customHeight.Get();
			break;
	}

	screenWidth  = displayMode->w;
	screenHeight = displayMode->h;

	SDL_WindowFlags flags = SDL_WINDOW_VULKAN;

	if ( r_fullscreen.Get() ) {
		flags |= SDL_WINDOW_FULLSCREEN;
	} else if ( r_noBorder.Get() ) {
		flags |= SDL_WINDOW_BORDERLESS;
	}

	/* if ( r_allowResize.Get() ) {
		flags |= SDL_WINDOW_RESIZABLE;
	} */

	window = SDL_CreateWindow( CLIENT_WINDOW_TITLE, width, height, flags );

	if ( !window ) {
		Sys::Drop( "SDL: failed to create window" );
	}
}

Surface::~Surface() {
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	SDL_DestroyWindow( window );
}
