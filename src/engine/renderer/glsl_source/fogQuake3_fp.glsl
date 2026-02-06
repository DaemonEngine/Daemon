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

#insert fogEquation_fp

#define DEPTHMAP_GLSL

uniform vec2 u_FogGradient;
uniform float u_FogEyeT;

IN(smooth) float var_FogPlaneDistance;
IN(smooth) vec3 var_ViewerOffset;
IN(smooth) vec4		var_Color;

DECLARE_OUTPUT(vec4)

void	main()
{
	#insert material_fp

	if ( var_FogPlaneDistance < -0.01 )
	{
		discard;
	}

	float distInFog = length(var_ViewerOffset);

	if ( u_FogEyeT < 0 ) // eye outside fog
	{
		// fraction of the viewer-to-vertex ray which is inside fog
		distInFog *= var_FogPlaneDistance / ( max( 0, var_FogPlaneDistance ) - u_FogEyeT );
	}

	float gradient = GetFogGradientModifier(u_FogGradient.y, u_FogEyeT, var_FogPlaneDistance);

	vec4 color = vec4(1, 1, 1, GetFogAlpha(gradient * u_FogGradient.x * distInFog));

	color *= var_Color;

	outputColor = color;
}
