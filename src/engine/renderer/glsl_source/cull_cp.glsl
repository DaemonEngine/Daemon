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

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

// layout(rg16f, binding = 0) uniform image2D depthImage;
layout(binding = 0) uniform sampler2D depthImage;

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

layout(std430, binding = 5) writeonly restrict buffer debugSSBO {
    vec4 debugSurfaces[];
};

struct Plane {
    vec3 normal;
    float distance;
};

uniform uint u_TotalDrawSurfs;
uniform uint u_SurfaceCommandsOffset;
uniform bool u_UseFrustumCulling;
uniform vec3 u_CameraPosition;
uniform mat4 u_ModelViewMatrix;
uniform vec4 u_Frustum[6]; // xyz - normal, w - distance
uniform uint u_ViewWidth;
uniform uint u_ViewHeight;
uniform float u_P00;
uniform float u_P11;

bool ProjectSphere( in vec3 center, in float radius, in float zNear, in float P00, in float P11, inout vec4 boundingBox, in uint debugID )
{
	/* if ( center.z < radius + zNear ) {
		return false;
    } */

    if ( -center.z - radius < zNear ) {
		return false;
    }

	vec3 cr = center * radius;
	float czr2 = center.z * center.z - radius * radius;

	float vx = sqrt( center.x * center.x + czr2 );
	float minx = center.x * center.x + czr2 >= 0.0 ? ( vx * center.x - cr.z ) / ( vx * center.z + cr.x ) : -1.0;
	float maxx = center.x * center.x + czr2 >= 0.0 ? ( vx * center.x + cr.z ) / ( vx * center.z - cr.x ) : 1.0;

	float vy = sqrt( center.y * center.y + czr2 );
	float miny = center.y * center.y + czr2 >= 0.0 ? ( vy * center.y - cr.z ) / ( vy * center.z + cr.y ) : -1.0;
	float maxy = center.y * center.y + czr2 >= 0.0 ? ( vy * center.y + cr.z ) / ( vy * center.z - cr.y ) : 1.0;

	boundingBox = vec4( minx * P00, miny * P11, maxx * P00, maxy * P11 );
	boundingBox = boundingBox.xwzy * vec4( 0.5f, -0.5f, 0.5f, -0.5f ) + vec4( 0.5, 0.5, 0.5, 0.5 ); // clip space -> uv space
    
    debugSurfaces[debugID * 5 + 2] = vec4( minx, maxx, miny, maxy );

	return true;
}

void UpdateBoundingBoxRoot( in float nx, in vec3 axisSphere, in float axisFOV, inout vec2 axisBox ) {
    float nz = ( axisSphere.z - nx * axisSphere.x ) / axisSphere.y;
    float pz = ( axisSphere.x * axisSphere.x + axisSphere.y * axisSphere.y - axisSphere.z * axisSphere.z )
               / ( axisSphere.y - ( nz / nx ) * axisSphere.x );
    if( pz > 0.0 ) {
        float c = -nz * axisFOV / nx;
        axisBox.x = nx > 0.0 ? max( axisBox.x, c ) : axisBox.x;
        axisBox.y = nx <= 0.0 ? min( axisBox.y, c ) : axisBox.y;
    }
}

void UpdateBoundingBox( in vec3 axisSphere, in float axisFOV, inout vec2 axisBox, in uint debugID ) {
    float radiusSquared = axisSphere.z * axisSphere.z;
    float centerZ = axisSphere.x * axisSphere.x + axisSphere.y * axisSphere.y;
    float distanceZ = axisSphere.x * axisSphere.x * radiusSquared - centerZ * ( radiusSquared - axisSphere.y * axisSphere.y );
    debugSurfaces[debugID].xyz = vec3( radiusSquared, centerZ, distanceZ );
    if( distanceZ > 0.0 ) {
        float a = axisSphere.z * axisSphere.x;
        float b = sqrt( distanceZ );
        float nx0 = ( a + b ) / centerZ;
        float nx1 = ( a - b ) / centerZ;
        debugSurfaces[debugID].z = a;
        debugSurfaces[debugID + 1] = vec4( b, nx0, nx1, 0.0 );
        UpdateBoundingBoxRoot( nx0, axisSphere, axisFOV, axisBox );
        UpdateBoundingBoxRoot( nx1, axisSphere, axisFOV, axisBox );
    }
}

bool CullSurface( in BoundingSphere boundingSphere ) {
    bool culled = false;

    for( int i = 0; i < 5; i++ ) { // Skip far plane for now because we always have it set to { 0, 0, 0, 0 } for some reason
        const float distance = dot( u_Frustum[i].xyz, boundingSphere.center ) - u_Frustum[i].w;

        if( distance < -boundingSphere.radius ) {
            culled = true && u_UseFrustumCulling;
        }
    }
    
    const uint debugID = gl_GlobalInvocationID.z * gl_NumWorkGroups.x * gl_WorkGroupSize.x * gl_NumWorkGroups.y * gl_WorkGroupSize.y
                            + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x
                            + gl_GlobalInvocationID.x;
    vec4 boundingBox = vec4( -1.0, 1.0, -1.0, 1.0 );
    const vec3 viewSpaceCenter = vec3( vec4( boundingSphere.center - u_CameraPosition, 1.0 ) * u_ModelViewMatrix );
    
    /* vec4 AABB;
    const vec3 viewSpaceCenter = boundingSphere.center - u_CameraPosition; */
    
    if( !culled && ProjectSphere( viewSpaceCenter, boundingSphere.radius, 3.0, u_P00, u_P11, boundingBox, debugID ) ) {
        const float width = clamp( ( boundingBox.z - boundingBox.x ) * float( u_ViewWidth ), 0.0, float( u_ViewWidth ) );
		const float height = clamp( ( boundingBox.w - boundingBox.y ) * float( u_ViewHeight ), 0.0, float( u_ViewHeight ) );

        const float level = floor( log2( max( width, height ) ) );
        // float level = 0.0;
        const int levelInt = int( level );

        // const float surfaceDepth = textureLod( depthImage, ( boundingBox.xy + boundingBox.zw ) * 0.5, level ).r;
        // const vec4 depthValues = textureGather( depthImage, ( boundingBox.xy + boundingBox.zw ) * 0.5)
        vec2 surfaceCoords = vec2( ( u_ViewWidth >> levelInt ) - 1, ( u_ViewHeight >> levelInt ) - 1 );
        surfaceCoords *= ( boundingBox.xy + boundingBox.zw ) * 0.5;
        const ivec2 surfaceCoordsFloor = ivec2( clamp( surfaceCoords.x, 0.0, float( u_ViewWidth ) ), clamp( surfaceCoords.y, 0.0, float( u_ViewHeight ) ) );
        /* ivec4 depthCoords = ivec4( surfaceCoordsFloor.xy,
                                   surfaceCoordsFloor.x + ( ( ( surfaceCoords.x - surfaceCoordsFloor.x >= 0.5 ) || surfaceCoords.x == 0 ) && surfaceCoords.x != u_ViewWidth -1 ? 1 : -1 ),
                                   surfaceCoordsFloor.y + ( ( ( surfaceCoords.y - surfaceCoordsFloor.y >= 0.5 ) || surfaceCoords.y == 0 ) && surfaceCoords.y != u_ViewHeight -1 ? 1 : -1 ) ); */
        ivec4 depthCoords = ivec4( surfaceCoordsFloor.xy,
                                   surfaceCoordsFloor.x + ( surfaceCoords.x - surfaceCoordsFloor.x >= 0.5 ? 1 : -1 ),
                                   surfaceCoordsFloor.y + ( surfaceCoords.y - surfaceCoordsFloor.y >= 0.5 ? 1 : -1 ) );
        depthCoords.x = clamp( depthCoords.x, 0, int( u_ViewWidth ) - 1 );
        depthCoords.y = clamp( depthCoords.y, 0, int( u_ViewHeight ) - 1 );
        depthCoords.z = clamp( depthCoords.z, 0, int( u_ViewWidth ) - 1 );
        depthCoords.w = clamp( depthCoords.w, 0, int( u_ViewHeight ) - 1 );

        vec4 depthValues;
        /* depthValues.x = texelFetch( depthImage, surfaceCoordsFloor, levelInt ).r;
        depthValues.y = texelFetch( depthImage,
                        ivec2(
                            clamp( surfaceCoordsFloor.x + ( surfaceCoords.x - surfaceCoordsFloor.x >= 0.5 ? 1 : -1 ), 0, ( u_ViewWidth >> levelInt ) - 1 ),
                            surfaceCoordsFloor.y ),
                        levelInt ).r;
        depthValues.z = texelFetch( depthImage,
                        ivec2( surfaceCoordsFloor.x,
                            clamp( surfaceCoordsFloor.y + ( surfaceCoords.y - surfaceCoordsFloor.y >= 0.5 ? 1 : -1 ), 0, ( u_ViewHeight >> levelInt ) - 1 )
                        ),
                        levelInt ).r;
        depthValues.w = texelFetch( depthImage,
                        ivec2(
                            clamp( surfaceCoordsFloor.x + ( surfaceCoords.x - surfaceCoordsFloor.x >= 0.5 ? 1 : -1 ), 0, ( u_ViewWidth >> levelInt ) - 1 ),
                            clamp( surfaceCoordsFloor.y + ( surfaceCoords.y - surfaceCoordsFloor.y >= 0.5 ? 1 : -1 ), 0, ( u_ViewHeight >> levelInt ) - 1 )
                        ),
                        levelInt ).r; */
        
        depthValues.x = texelFetch( depthImage, depthCoords.xy, levelInt ).r;
        depthValues.y = texelFetch( depthImage, depthCoords.zy, levelInt ).r;
        depthValues.z = texelFetch( depthImage, depthCoords.xw, levelInt ).r;
        depthValues.w = texelFetch( depthImage, depthCoords.zw, levelInt ).r;

        const float surfaceDepth = max( max( max( depthValues.x, depthValues.y ), depthValues.z ), depthValues.w );

        culled = ( 1 + 3.0 / ( viewSpaceCenter.z + boundingSphere.radius ) ) > surfaceDepth;
        debugSurfaces[debugID * 5] = vec4( viewSpaceCenter, float( culled ) );
        debugSurfaces[debugID * 5 + 1] = boundingBox.xzyw;
        debugSurfaces[debugID * 5 + 3] = vec4( width, height, level, surfaceDepth );
        debugSurfaces[debugID * 5 + 4].x = 1 + 3.0 / ( viewSpaceCenter.z + boundingSphere.radius );
        // debugSurfaces[debugID * 5 + 4].yz = vec2( clamp( surfaceCoordsFloor.x + ( surfaceCoords.x - surfaceCoordsFloor.x >= 0.5 ? 1 : -1 ), 0, ( u_ViewWidth >> levelInt ) - 1 ),
        //                                           clamp( surfaceCoordsFloor.y + ( surfaceCoords.y - surfaceCoordsFloor.y >= 0.5 ? 1 : -1 ), 0, ( u_ViewHeight >> levelInt ) - 1 ) );
        debugSurfaces[debugID * 5 + 4].yz = vec2( depthCoords.xy );
        // culled = ( boundingBox.x == boundingBox.z );
    }

    /* if( !culled ) { // boundingSphere.center - u_CameraPosition;
        vec2 minBox = vec2( -1.0, -1.0 );
        vec2 maxBox = vec2( 1.0, 1.0 );
        vec4 mmBox = vec4( -1.0, -1.0, 1.0, 1.0 );
        // UpdateBoundingBox( vec3( boundingSphere.xz, boundingSphere.radius ), u_P00, boundingBox.xy );
        // UpdateBoundingBox( vec3( boundingSphere.yz, boundingSphere.radius ), u_P11, boundingBox.zw );
        UpdateBoundingBox( vec3( viewSpaceCenter.xz, boundingSphere.radius ), u_P00, mmBox.xz, debugID * 5 + 1 );
        UpdateBoundingBox( vec3( viewSpaceCenter.yz, boundingSphere.radius ), u_P11, mmBox.yw, debugID * 5 + 3 );
        // boundingBox = mmBox * 0.5 + vec4( 0.5, 0.5, 0.5, 0.5 );
        boundingBox = mmBox.xwzy * 0.5 + vec4( 0.5, -0.5, 0.5, -0.5 );

        const float width = ( boundingBox.z - boundingBox.x ) * float( u_ViewWidth );
		const float height = ( boundingBox.w - boundingBox.y ) * float( u_ViewHeight );

        const float level = floor( log2( max( width, height ) ) );
        // float level = 0.0;

        const float surfaceDepth = textureLod( depthImage, ( boundingBox.xy + boundingBox.zw ) * 0.5, level ).r;
        
        culled = 3.0 / ( viewSpaceCenter.z - boundingSphere.radius ) < surfaceDepth;
        // culled = surfaceDepth >= 0.998;
        // culled = width >= 960;
        // culled = level > 9.0;
        // culled = ( boundingBox.x == boundingBox.z );
        // culled = !( viewSpaceCenter.x >= -1.0 );
        // culled = ( boundingBox.w - boundingBox.y ) > 1;
        debugSurfaces[debugID * 5] = vec4( viewSpaceCenter, 1.0 ); // mmBox;
    } */

    return culled;
}

void ProcessSurfaceCommands( const in SurfaceDescriptor surface, const in bool enabled ) {
    for( uint i = 0; i < MAX_SURFACE_COMMANDS; i++ ) {
        const uint commandID = surface.surfaceCommandIDs[i];
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
