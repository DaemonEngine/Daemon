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
// BufferBind.h

#ifndef BUFFERBIND_H
#define BUFFERBIND_H

namespace BufferBind {
	enum : uint32_t {
		// UBO
		MATERIALS = 0,
		TEX_DATA = 1,
		LIGHTMAP_DATA = 2,
		LIGHTS = 3,

		SURFACE_BATCHES = 4,

		// SSBO
		SURFACE_DESCRIPTORS = 0,
		SURFACE_COMMANDS = 1,
		CULLED_COMMANDS = 2,
		PORTAL_SURFACES = 4,

		GEOMETRY_CACHE_INPUT_VBO = 5,
		GEOMETRY_CACHE_VBO = 6,
		GEOMETRY_CACHE_IBO = 7,

		COMMAND_COUNTERS_STORAGE = 9,
		TEX_DATA_STORAGE = 11,

		DEBUG = 10,
		
		// Atomic
		COMMAND_COUNTERS_ATOMIC = 0
	};
};

#endif // BUFFERBIND_H
