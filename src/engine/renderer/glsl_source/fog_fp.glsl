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

/* fog_fp.glsl */

#insert common
#insert fogEquation_fp

#define DEPTHMAP_GLSL

IN(smooth) vec3 var_Position;
IN(flat) vec4 var_FogSurface;

#ifdef OUTSIDE_FOG
#define NUM_PLANES 5
IN(flat) vec4 var_FogPlanes[NUM_PLANES];
#endif

uniform sampler2D	u_DepthMap;

uniform colorPack u_Color;

uniform vec3 u_ViewOrigin;
uniform vec2 u_FogGradient;
uniform mat4		u_UnprojectMatrix;

DECLARE_OUTPUT(vec4)

#ifdef OUTSIDE_FOG
// Trace against the inner sides of the fog brush
float FogDistance(vec3 start, vec3 dir)
{
	vec4 start4 = vec4(-start, 1.0);
	float minDist = 1.0e20;
	for (int i = 0; i < NUM_PLANES; i++)
	{
		float dist = dot(start4, var_FogPlanes[i]) / dot(dir, var_FogPlanes[i].xyz) ;
		if (dist >= 0.0)
		{
			minDist = min(minDist, dist);
		}
	}
	return minDist < 1.0e20 ? minDist : 0.0;
}
#endif

void	main()
{
	#insert material_fp

	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st / r_FBufSize;

	// reconstruct vertex position in world space
	float depth = texture2D(u_DepthMap, st).r;
	vec4 P = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
	P.xyz /= P.w;

	#ifdef OUTSIDE_FOG
		vec3 startPoint = var_Position;
		vec3 viewDir = normalize(var_Position - u_ViewOrigin);
		float fogBoundaryDist = FogDistance(var_Position, viewDir);
	#else
		vec3 startPoint = u_ViewOrigin;
		float fogBoundaryDist = distance(u_ViewOrigin, var_Position);
	#endif

	float depthDist = distance(startPoint, P.xyz);

	vec3 endPoint;
	float distInFog;
	if ( depthDist < fogBoundaryDist )
	{
		endPoint = P.xyz;
		distInFog = depthDist;
	}
	else
	{
		#ifdef OUTSIDE_FOG
			endPoint = var_Position + fogBoundaryDist * viewDir;
		#else
			endPoint = var_Position;
		#endif

		distInFog = fogBoundaryDist;
	}

	float t0 = dot(var_FogSurface.xyz, startPoint) + var_FogSurface.w;
	float t1 = dot(var_FogSurface.xyz, endPoint) + var_FogSurface.w;

	float s = distInFog * GetFogGradientModifier(u_FogGradient.y, t0, t1) * u_FogGradient.x;

	vec4 color = vec4(1, 1, 1, GetFogAlpha(s));

	outputColor = UnpackColor( u_Color ) * color;
}
