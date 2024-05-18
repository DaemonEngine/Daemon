/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2024 Daemon Developers
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

/* depthReduction_cp.glsl */

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform readonly image2D depthImageIn;
layout(rgba32f, binding = 1) uniform writeonly image2D depthImageOut;

uniform uint u_ViewWidth;
uniform uint u_ViewHeight;

void main() {
    const uint globalInvocationID = gl_GlobalInvocationID.z * gl_NumWorkGroups.x * gl_WorkGroupSize.x * gl_NumWorkGroups.y * gl_WorkGroupSize.y
                             + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x
                             + gl_GlobalInvocationID.x;

    const ivec2 position = ivec2( gl_GlobalInvocationID.xy );
    if( position.x >= u_ViewWidth || position.y >= u_ViewHeight ) {
        return;
    };
    /* if( globalInvocationID >= u_TotalDrawSurfs ) {
        return;
    } */

    vec4 depth[4];
    for( int x = 0; x < 2; x++ ) {
        for( int y = 0; y < 2; y++ ) {
            depth[y * 2 + x] = imageLoad( depthImageIn, position + ivec2( x, y ) );
        }
    }

    vec4 depthOut = min( depth[0], depth[1] );
    depthOut = min( depthOut, depth[2] );
    depthOut = min( depthOut, depth[3] );
    imageStore( depthImageOut, position, depthOut );
}
