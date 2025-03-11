/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2013-2016 Daemon Developers
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

/* lighttile_fp.glsl */

IN(smooth) vec2 vPosition;

struct Light {
	vec3 center;
	float radius;
	vec3 color;
	float type;
	vec3 direction;
	float angle;
};

layout(std140) uniform u_Lights {
	Light lights[MAX_REF_LIGHTS];
};

Light GetLight( in uint idx ) {
	return lights[idx];
}

uniform int u_numLights;
uniform mat4 u_ModelMatrix;
uniform sampler2D u_DepthMap;
uniform int u_lightLayer;
uniform vec3 u_zFar;

const int lightsPerLayer = 16;

#define idxs_t uvec4

DECLARE_OUTPUT(uvec4)

// 8 bits per light ID
void pushIdxs( in uint idx, in uint count, inout uvec4 idxs ) {
	idxs[count / 4] <<= 8;
	idxs[count / 4] |= idx & 0xFFu;
}

#define exportIdxs( x ) outputColor = ( x )

void lightOutsidePlane( in vec4 plane, inout vec3 center, inout float radius ) {
	float dist = dot( plane, vec4( center, 1.0 ) );
	if( dist >= radius ) {
		radius = 0.0; // light completely outside plane
		return;
	}

	if( dist >= 0.0 ) {
		// light is outside plane, but intersects the volume
		center -= dist * plane.xyz;
		radius = sqrt( radius * radius - dist * dist );
	}
}

vec3 ProjToView( vec2 inp ) {
	return u_zFar * vec3( inp, -1 );
}

void main() {
	vec2 minmax = texture2D( u_DepthMap, 0.5 * vPosition + 0.5 ).xy;

	float minx = vPosition.x - r_tileStep.x;
	float maxx = vPosition.x + r_tileStep.x;
	float miny = vPosition.y - r_tileStep.y;
	float maxy = vPosition.y + r_tileStep.y;

	vec3 bottomleft = ProjToView( vec2( minx, miny ) );
	vec3 bottomright = ProjToView( vec2( maxx, miny ) );
	vec3 topright = ProjToView( vec2( maxx, maxy ) );
	vec3 topleft = ProjToView( vec2( minx, maxy ) );

	vec4 plane1 = vec4( normalize( cross( bottomleft, bottomright ) ), 0 );
	vec4 plane2 = vec4( normalize( cross( bottomright, topright ) ), 0 );
	vec4 plane3 = vec4( normalize( cross( topright, topleft ) ), 0 );
	vec4 plane4 = vec4( normalize( cross( topleft, bottomleft ) ), 0 );

	vec4 plane5 = vec4( 0.0, 0.0,  1.0,  minmax.y );
	vec4 plane6 = vec4( 0.0, 0.0, -1.0, -minmax.x );

	idxs_t idxs = uvec4( 0, 0, 0, 0 );

	uint lightCount = 0;

	/* Dynamic lights are put into 4 layers of a 3D texture. Since checking if we already added some light is infeasible,
	only process 1 / 4 of different lights for each layer, extra lights going into the last layer. This can fail to add some lights
	if 1 / 4 of all lights is more than the amount of lights that each layer can hold (16). To fix this, we'd need to either do this on CPU
	or use compute shaders with atomics so we can have a variable amount of lights for each tile. */
	for( uint i = u_lightLayer; i < u_numLights; i += NUM_LIGHT_LAYERS ) {
		Light l = GetLight( i );
		vec3 center = ( u_ModelMatrix * vec4( l.center, 1.0 ) ).xyz;
		float radius = max( 2.0 * l.radius, 2.0 * 32.0 ); // Avoid artifacts with weak light sources

		// todo: better checks for spotlights
		lightOutsidePlane( plane1, center, radius );
		lightOutsidePlane( plane2, center, radius );
		lightOutsidePlane( plane3, center, radius );
		lightOutsidePlane( plane4, center, radius );
		lightOutsidePlane( plane5, center, radius );
		lightOutsidePlane( plane6, center, radius );

		if( radius > 0.0 ) {
			/* Light IDs are stored relative to the layer
			Add 1 because 0 means there's no light */
			pushIdxs( ( i / NUM_LIGHT_LAYERS ) + 1, lightCount, idxs );
			lightCount++;

			if( lightCount == lightsPerLayer ) {
				break;
			}
		}
	}

	exportIdxs( idxs );
}
