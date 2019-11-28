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
IN(smooth) vec2 vTexCoord;

struct light {
  vec3  center;
  float radius;
  vec3  color;
  float type;
  vec3  direction;
  float angle;
};

#ifdef HAVE_ARB_uniform_buffer_object
layout(std140) uniform u_Lights {
  vec4 lightvec[ MAX_REF_LIGHTS * 3 ];
};
light GetLight(in int idx) {
  light result; vec4 component;
  idx *= 3;
  component = lightvec[idx++];
  result.center = component.xyz; result.radius = component.w;
  component = lightvec[idx++];
  result.color = component.xyz; result.type = component.w;
  component = lightvec[idx++];
  result.direction = component.xyz; result.angle = component.w;
  return result;
}
#else
uniform sampler2D u_Lights;
vec2 idxToTC( in int idx, float w, float h ) {
  return vec2( ( float(idx) + 0.5 ) * ( 1.0 / (w * h) ),
               ( float(idx) + 0.5 ) * ( 1.0 / w ) );
}
light GetLight(in int idx) {
  light result; vec4 component;
  idx *= 3;
  component = texture2D( u_Lights, idxToTC(idx++, 64.0, float( 3 * MAX_REF_LIGHTS / 64 ) ) );
  result.center = component.xyz; result.radius = component.w;
  component = texture2D( u_Lights, idxToTC(idx++, 64.0, float( 3 * MAX_REF_LIGHTS / 64 ) ) );
  result.color = component.xyz; result.type = component.w;
  component = texture2D( u_Lights, idxToTC(idx++, 64.0, float( 3 * MAX_REF_LIGHTS / 64 ) ) );
  result.direction = component.xyz; result.angle = component.w;
  return result;
}
#endif

uniform int  u_numLights;
uniform mat4 u_ModelMatrix;
uniform sampler2D u_DepthMap;
uniform int  u_lightLayer;
uniform vec3 u_zFar;

const int numLayers = MAX_REF_LIGHTS / 256;

#if defined( TEXTURE_INTEGER )
#define idxs_t uvec4
#define idx_initializer uvec4(3)
DECLARE_OUTPUT(uvec4)
void pushIdxs(in int idx, inout uvec4 idxs ) {
  uvec4 bits = uvec4( idx >> 8, idx >> 6, idx >> 4, idx >> 2 ) & uvec4( 0x03 );
  idxs = idxs << 2 | bits;
}
#define exportIdxs(x) outputColor = ( x )
#else
DECLARE_OUTPUT(vec4)
#define idxs_t vec4
#define idx_initializer vec4(3.0)
void pushIdxs(in int idx, inout vec4 idxs ) {
  vec4 bits = floor( vec4( idx ) * vec4( 1.0/256.0, 1.0/64.0, 1.0/16.0, 1.0/4.0 ) );
  bits.yzw -= 4.0 * bits.xyz;
  idxs = idxs * 4.0 + bits;
  idxs -= 256.0 * floor( idxs * (1.0/256.0) ); // discard upper bits
}
#define exportIdxs(x) outputColor = ( x ) * (1.0/255.0)
#endif

void lightOutsidePlane( in vec4 plane, inout vec3 center, inout float radius ) {
  float dist = dot( plane, vec4( center, 1.0 ) );
  if( dist >= radius ) {
    radius = 0.0; // light completely outside plane
    return;
  }

  if( dist >= 0.0 ) {
    // light is outside plane, but intersects the volume
    center = center - dist * plane.xyz;
    radius = sqrt( radius * radius - dist * dist );
  }
}

vec3 ProjToView(vec2 inp)
{
	vec3 p = u_zFar * vec3(inp, -1);
	
	return p;
}

void main() {
  vec2 minmax = texture2D( u_DepthMap, 0.5 * vPosition + 0.5 ).xy;

  float minx = vPosition.x - r_tileStep.x;
  float maxx = vPosition.x + r_tileStep.x;
  float miny = vPosition.y - r_tileStep.y;
  float maxy = vPosition.y + r_tileStep.y;

  vec3 bottomleft = ProjToView(vec2(minx, miny));
  vec3 bottomright = ProjToView(vec2(maxx, miny));
  vec3 topright = ProjToView(vec2(maxx, maxy));
  vec3 topleft = ProjToView(vec2(minx, maxy));

  vec4 plane1 = vec4(normalize(cross(bottomleft, bottomright)), 0);
  vec4 plane2 = vec4(normalize(cross(bottomright, topright)), 0);
  vec4 plane3 = vec4(normalize(cross(topright, topleft)), 0);
  vec4 plane4 = vec4(normalize(cross(topleft, bottomleft)), 0);

  vec4 plane5 = vec4( 0.0, 0.0,  1.0,  minmax.y );
  vec4 plane6 = vec4( 0.0, 0.0, -1.0, -minmax.x );

  idxs_t idxs = idx_initializer;

  for( int i = u_lightLayer; i < u_numLights; i += numLayers ) {
    light l = GetLight( i );
    vec3 center = ( u_ModelMatrix * vec4( l.center, 1.0 ) ).xyz;
    float radius = 2.0 * l.radius;

    // todo: better checks for spotlights
    lightOutsidePlane( plane1, center, radius );
    lightOutsidePlane( plane2, center, radius );
    lightOutsidePlane( plane3, center, radius );
    lightOutsidePlane( plane4, center, radius );
    lightOutsidePlane( plane5, center, radius );
    lightOutsidePlane( plane6, center, radius );

    if( radius > 0.0 ) {
      pushIdxs( i, idxs );
    }
  }

  exportIdxs( idxs );
}
