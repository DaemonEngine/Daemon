/*
===========================================================================
Copyright (C) 2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* fogGlobal_fp.glsl */

#insert common

uniform sampler2D	u_ColorMap; // fog texture
uniform sampler2D	u_DepthMap;

uniform colorPack u_Color;

uniform vec4		u_FogDistanceVector;
uniform mat4		u_UnprojectMatrix;

#if __VERSION__ > 120
out vec4 outputColor;
#else
#define outputColor gl_FragColor
#endif

void	main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st / r_FBufSize;

	// reconstruct vertex position in world space
	float depth = texture2D(u_DepthMap, st).r;
	vec4 P = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
	P.xyz /= P.w;

	// calculate the length in fog (t is always 0 if eye is in fog)
	st.s = dot(P.xyz, u_FogDistanceVector.xyz) + u_FogDistanceVector.w;
	// st.s = vertexDistanceToCamera;
	st.t = 1.0;

	vec4 color = texture2D(u_ColorMap, st);

	outputColor = UnpackColor( u_Color ) * color;
}
