/*
===========================================================================
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/* ssao_fp.glsl */

#define DEPTHMAP_GLSL

uniform sampler2D u_DepthMap;
uniform vec3 u_UnprojectionParams;

DECLARE_OUTPUT(vec4)

uniform vec3 u_zFar;

const vec2 pixelScale = 1 / r_FBufSize;
const float haloCutoff = 15.0;

float depthToZ( float depth ) {
	return u_UnprojectionParams.x / ( u_UnprojectionParams.y * depth - u_UnprojectionParams.z );
}
vec4 depthToZ( vec4 depth ) {
	return u_UnprojectionParams.x / ( u_UnprojectionParams.y * depth - u_UnprojectionParams.z );
}

vec4 rangeTest( vec4 diff1, vec4 diff2, vec4 deltaX, vec4 deltaY ) {
	vec4 mask = step( 0.0, diff1 + diff2 )
		* step( -haloCutoff, -diff1 )
		* step( -haloCutoff, -diff2 );
	vec4 deltaXY = deltaX * deltaX + deltaY * deltaY;
	vec4 scale = inversesqrt( diff1 * diff1 + deltaXY );
	scale *= inversesqrt( diff2 * diff2 + deltaXY );
	vec4 cosAngle = scale * ( diff1 * diff2 - deltaXY );
	return mask * ( 0.5 * cosAngle + 0.5 );
}

// Based on AMD HDAO, adapted for lack of normals
void computeOcclusionForQuad( in vec2 centerTC, in float centerDepth,
			      in vec2 quadOffset, in sampler2D u_DepthMap,
			      inout vec4 occlusion, inout vec4 total ) {
	vec2 tc1 = centerTC + quadOffset * pixelScale;
	vec2 tc2 = centerTC - quadOffset * pixelScale;

	vec4 x = vec4( -0.5, 0.5, 0.5, -0.5 ) + quadOffset.x;
	vec4 y = vec4( 0.5, 0.5, -0.5, -0.5 ) + quadOffset.y;
	vec4 weights = inversesqrt( x * x + y * y );

	// depths1 and depth2 pixels are swizzled to be in opposite position
	vec4 depths1 = depthToZ( textureGather( u_DepthMap, tc1 ) ).xyzw;
	vec4 depths2 = depthToZ( textureGather( u_DepthMap, tc2 ) ).zwxy;

	depths1 = centerDepth - depths1;
	depths2 = centerDepth - depths2;

	total += weights;
	occlusion += weights * rangeTest( depths1, depths2,
					  centerDepth * x * u_zFar.x,
					  centerDepth * y * u_zFar.y );
}

vec4 offsets[] = vec4[6](
	vec4( 0.5,  1.5, 1.5, -0.5 ),
	vec4( 0.5,  4.5, 3.5,  2.5 ),
	vec4( 4.5, -0.5, 2.5, -3.5 ),
	vec4( 0.5,  7.5, 3.5,  5.5 ),
	vec4( 6.5,  2.5, 7.5, -0.5 ),
	vec4( 5.5, -3.5, 2.5, -6.5 )
);

void	main()
{
	#insert material_fp

	vec2 st = gl_FragCoord.st * pixelScale;
	vec4 occlusion = vec4( 0.0 );
	vec4 total = vec4( 0.0 );

	float center = texture2D( u_DepthMap, st ).r;

	if( center >= 1.0 ) {
		// no SSAO on the skybox !
		discard;
		return;
	}
	center = depthToZ( center );
	float spread = max( 1.0, 100.0 / center );

	for( int i = 0; i < offsets.length(); i++ ) {
		vec2 of;
		int checker = int( gl_FragCoord.x - gl_FragCoord.y ) & 1;
    	if ( checker != 0 ) {
			of = offsets[i].xy;
		} else {
			of = offsets[i].zw;
		}
		computeOcclusionForQuad( st, center, spread * of, u_DepthMap,
					 occlusion, total );
	}

	float summedOcclusion = dot( occlusion, vec4( 1.0 ) );
	float summedTotal = dot( total, vec4( 1.0 ) );

	if ( summedTotal > 0.0 ) {
		summedOcclusion /= summedTotal;
	}
	outputColor = vec4( clamp( 1.0 - summedOcclusion, 0.0, 1.0 ) );
}
