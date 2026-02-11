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
// GraphicsCoreCVars.cpp

#include "../Version.h"

#include "CapabilityPack.h"

#include "SwapChain.h"

#include "Memory/EngineAllocator.h"

#include "GraphicsCoreCVars.h"

Cvar::Cvar<int>              r_rendererApi( "r_rendererAPI", "Renderer API: 0: OpenGL, 1: Vulkan", Cvar::ROM, 1 );

Cvar::Cvar<std::string>      r_vkVersion( "r_vkVersion", "Daemon-vulkan version", Cvar::ROM, DAEMON_VULKAN_VERSION.FormatVersion() );

Cvar::Range<Cvar::Cvar<int>> r_vkCapabilityPack( "r_vkCapabilityPack", "CapabilityPack override",
	Cvar::NONE, CapabilityPackType::MINIMAL, CapabilityPackType::MINIMAL, CapabilityPackType::EXPERIMENTAL );

Cvar::Cvar<int>              r_vkDevice( "r_vkDevice", "Use specific GPU (-1: auto)", Cvar::NONE, -1 );

Cvar::Cvar<int>              r_displayIndex( "r_displayIndex", "Display index to create the window in", Cvar::NONE, 0 );

Cvar::Range<Cvar::Cvar<int>> r_vkPresentMode( "r_vkPresentMode",
	"Presentation mode: 0 - immediate, 1 - vsync on last presented frame, 2 - vsync on first presented frame before scanout, "
	"3 - relaxed vsync on first presented frame before scanout, 4 - vsync on the closest frame to scanout",
	Cvar::NONE, PresentMode::IMMEDIATE, PresentMode::IMMEDIATE, PresentMode::SCANOUT_LATEST );

Cvar::Range<Cvar::Cvar<int>> r_mode( "r_mode",
	"Window mode: -2: use display size, -1: use r_customWidth / r_customHeight",
	Cvar::NONE, -2, -2, -1 );

Cvar::Cvar<int>              r_customWidth(  "r_customWidth",  "Window width when using r_mode -1",  Cvar::NONE, 1920 );
Cvar::Cvar<int>              r_customHeight( "r_customHeight", "Window height when using r_mode -1", Cvar::NONE, 1080 );

// Cvar::Cvar<bool>             r_fullscreen( "r_fullscreen", "Fullscreen", Cvar::ARCHIVE, true );
Cvar::Cvar<bool>             r_noBorder( "r_noBorder", "Borderless window", Cvar::ARCHIVE, false );
// Cvar::Cvar<bool>             r_allowResize( "r_allowResize", "Resizable window", Cvar::ARCHIVE, false );

Cvar::Range<Cvar::Cvar<int>> r_vkGraphicsMaxMemory( "r_vkGraphicsMaxMemory",
	"Set memory allocation size for graphics engine (in mb); requires r_vkGraphicsMaxMemoryAuto",
	Cvar::NONE, 0, 0, 1 );

Cvar::Cvar<bool>             r_vkGraphicsMaxMemoryAuto( "r_vkGraphicsMaxMemoryAuto",
	"Automatically select memory allocation size based on available memory; use r_vkResourceSystemMaxMemory to set size manually",
	Cvar::NONE, true );

Cvar::Range<Cvar::Cvar<int>> r_vkVSync( "r_vkVSync",
	"Screen presentation synchronisation mode: 0: sync to screen refresh rate, adjust scanout:present ratio automatically, "
	"1 - 8: set the scanout:present ratio to this value",
	Cvar::NONE, 0, 0, 8 );

Cvar::Range<Cvar::Cvar<int>> r_vkExecutionGraphRate( "r_vkExecutionGraphRate",
	"The general rate at which ExecutionGraphs are executed",
	Cvar::NONE, 0, -1, 100000 );