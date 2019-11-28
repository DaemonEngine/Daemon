/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2013 Daemon Developers
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

===========================================================================
*/

#include "client/client.h"
#include "DetourDebugDraw.h"
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "DebugDraw.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include "bot_local.h"
#include "nav.h"
#include "bot_debug.h"
#include "bot_navdraw.h"

void DebugDrawQuake::init(BotDebugInterface_t *ref)
{
	re = ref;
}

void DebugDrawQuake::depthMask(bool state)
{
	re->DebugDrawDepthMask( ( bool ) ( int ) state );
}

void DebugDrawQuake::begin(duDebugDrawPrimitives prim, float s)
{
	re->DebugDrawBegin( ( debugDrawMode_t ) prim, s );
}

void DebugDrawQuake::vertex(const float* pos, unsigned int c)
{
	vertex( pos, c, nullptr );
}

void DebugDrawQuake::vertex(const float x, const float y, const float z, unsigned int color)
{
	vec3_t vert;
	VectorSet( vert, x, y, z );
	recast2quake( vert );
	re->DebugDrawVertex( vert, color, nullptr );
}

void DebugDrawQuake::vertex(const float *pos, unsigned int color, const float* uv)
{
	vec3_t vert;
	VectorCopy( pos, vert );
	recast2quake( vert );
	re->DebugDrawVertex( vert, color, uv );
}

void DebugDrawQuake::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
	vec3_t vert;
	vec2_t uv;
	uv[0] = u;
	uv[1] = v;
	VectorSet( vert, x, y, z );
	recast2quake( vert );
	re->DebugDrawVertex( vert, color, uv );
}

void DebugDrawQuake::end()
{
	re->DebugDrawEnd();
}
