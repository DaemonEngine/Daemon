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

/* cull_cp.glsl */

// Keep this to 64 because we don't want extra shared mem etc. to be allocated, and to minimize wasted lanes
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

struct BoundingSphere {
    vec3 center;
    float radius;
};

struct SurfaceDescriptor {
    BoundingSphere boundingSphere;
    uint surfaceCommandIDs[MAX_SURFACE_COMMANDS];
};

struct GLIndirectCommand {
	uint count;
	uint instanceCount;
	uint firstIndex;
	int baseVertex;
	uint baseInstance;
};

struct SurfaceCommand {
    bool enabled;
    GLIndirectCommand drawCommand;
};

layout(std430, binding = 1) readonly restrict buffer surfaceDescriptorsSSBO {
    SurfaceDescriptor surfaces[];
};

layout(std430, binding = 2) writeonly restrict buffer surfaceCommandsSSBO {
    SurfaceCommand surfaceCommands[];
};

struct Plane {
    vec3 normal;
    float distance;
};

uniform uint u_TotalDrawSurfs;
uniform uint u_SurfaceCommandsOffset;
uniform vec4 u_Frustum[6]; // xyz - normal, w - distance

bool CullSurface( in BoundingSphere boundingSphere ) {
    // Skip far plane because we always make it encompass the whole map in the current direction
    // This might need to be changed later for shadowmaps since lights could have some far plane set
    for( int i = 0; i < 5; i++ ) {
        const float distance = dot( u_Frustum[i].xyz, boundingSphere.center ) - u_Frustum[i].w;

        if( distance < -boundingSphere.radius ) {
            return true;
        }
    }
    return false;
}

void ProcessSurfaceCommands( const in SurfaceDescriptor surface, const in bool enabled ) {
    for( uint i = 0; i < MAX_SURFACE_COMMANDS; i++ ) {
        const uint commandID = surface.surfaceCommandIDs[i];
        if( commandID == 0 ) { // Reserved for no-command
            return;
        }
        surfaceCommands[commandID + u_SurfaceCommandsOffset].enabled = enabled;
    }
}

void main() {
    const uint globalInvocationID = gl_GlobalInvocationID.z * gl_NumWorkGroups.x * gl_WorkGroupSize.x * gl_NumWorkGroups.y * gl_WorkGroupSize.y
                             + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x
                             + gl_GlobalInvocationID.x;
    if( globalInvocationID >= u_TotalDrawSurfs ) {
        return;
    }
    SurfaceDescriptor surface = surfaces[globalInvocationID];
    bool culled = CullSurface( surface.boundingSphere );

    ProcessSurfaceCommands( surface, !culled );
}
