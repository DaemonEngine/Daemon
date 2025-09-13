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

/* fogQuake3_fp.glsl */

#define FOGQUAKE3_GLSL

uniform sampler2D u_FogMap;

uniform float u_FogEyeT;

IN(smooth) float var_FogPlaneDistance;
IN(smooth) vec3 var_ScaledViewerOffset;
IN(smooth) vec4		var_Color;

DECLARE_OUTPUT(vec4)

void	main()
{
	#insert material_fp

	float s = length(var_ScaledViewerOffset);

	s += 1.0 / 512.0;

	float t = step( 0, var_FogPlaneDistance );

	if ( u_FogEyeT < 0 ) // eye outside fog
	{
		// fraction of the viewer-to-vertex ray which is inside fog
		t *= var_FogPlaneDistance / ( max( 0, var_FogPlaneDistance ) - u_FogEyeT );
	}

	t = 1.0 / 32.0 + ( 30.0 / 32.0 ) * t;

	vec4 color = texture2D(u_FogMap, vec2(s, t));

	color *= var_Color;

	outputColor = color;
}
