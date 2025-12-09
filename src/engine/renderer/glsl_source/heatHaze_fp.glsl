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

/* heatHaze_fp.glsl */

#insert reliefMapping_fp

#define HEATHAZE_GLSL

uniform sampler2D	u_CurrentMap;

#if defined(USE_MATERIAL_SYSTEM)
	uniform float u_DeformEnable;
#endif

IN(smooth) vec2		var_TexCoords;
IN(smooth) float	var_Deform;

DECLARE_OUTPUT(vec4)

void	main()
{
	#insert material_fp

	vec4 color;

	// compute normal in tangent space from normalmap
	#if defined(r_normalMapping)
		vec3 normal = NormalInTangentSpace(var_TexCoords, u_NormalMap);
	#else // !r_normalMapping
		vec3 normal = NormalInTangentSpace(var_TexCoords);
	#endif // !r_normalMapping

	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st / r_FBufSize;

	// offset by the scaled normal and clamp it to 0.0 - 1.0
	
	#if defined(USE_MATERIAL_SYSTEM)
		// Use a global uniform for heatHaze with material system to avoid duplicating all of the shader stage data
		st += normal.xy * var_Deform * u_DeformEnable;
	#else
		st += normal.xy * var_Deform;
	#endif

	st = clamp(st, 0.0, 1.0);

	color = texture2D(u_CurrentMap, st);

	outputColor = color;
}
