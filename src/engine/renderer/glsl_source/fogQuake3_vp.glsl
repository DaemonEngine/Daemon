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

/* fogQuake3_vp.glsl */

#insert vertexSimple_vp
#insert vertexSkinning_vp
#insert vertexAnimation_vp

uniform float		u_Time;

uniform uint u_ColorGlobal;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform vec4		u_FogDistanceVector;
uniform vec4		u_FogDepthVector;
uniform float		u_FogEyeT;

OUT(smooth) vec3	var_Position;
OUT(smooth) vec2	var_TexCoords;
OUT(smooth) vec4	var_Color;

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time );

void main()
{
	#insert material_vp

	vec4 position;
	localBasis LB;
	vec2 texCoord, lmCoord;
	vec4 color;

	VertexFetch( position, LB, color, texCoord, lmCoord );

	color = unpackUnorm4x8( u_ColorGlobal );

	DeformVertex( position,
		      LB.normal,
		      texCoord,
		      color,
		      u_Time );

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	// calculate the length in fog
	float s = dot(position.xyz, u_FogDistanceVector.xyz) + u_FogDistanceVector.w;
	float t = dot(position.xyz, u_FogDepthVector.xyz) + u_FogDepthVector.w;

	// partially clipped fogs use the T axis
	if(u_FogEyeT < 0.0)
	{
		if(t < 1.0)
		{
			t = 1.0 / 32.0;	// point is outside, so no fogging
		}
		else
		{
			t = 1.0 / 32.0 + 30.0 / 32.0 * t / (t - u_FogEyeT);	// cut the distance at the fog plane
		}
	}
	else
	{
		if(t < 0.0)
		{
			t = 1.0 / 32.0;	// point is outside, so no fogging
		}
		else
		{
			t = 31.0 / 32.0;
		}
	}

	var_TexCoords = vec2(s, t);

	var_Color = color;
}
