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

/* lightMapping_fp.glsl */
uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_SpecularMap;
uniform sampler2D	u_GlowMap;
uniform sampler2D	u_LightMap;
uniform sampler2D	u_DeluxeMap;
uniform float		u_AlphaThreshold;
uniform vec3		u_ViewOrigin;

IN(smooth) vec3		var_Position;
IN(smooth) vec2		var_TexCoords;
IN(smooth) vec2		var_TexLight;

IN(smooth) vec3		var_Tangent;
IN(smooth) vec3		var_Binormal;
IN(smooth) vec3		var_Normal;

IN(smooth) vec4		var_Color;

DECLARE_OUTPUT(vec4)

#if defined(r_floorLight)
// returns a pseudo-boolean (1 or 0) vec3 given the component
// is smaller to the given float or not
// for example isSmaller(vec3(1, 2, 3), 2) returns vec3(1, 0, 0)
vec3 isSmaller(vec3 v, float max)
{
	return floor(min(v, max) / max) * -1 + 1;
}

// shift and rescale a vec3 from any [min1, max1] range to [min2 - max2] range
// for example it can be used to rescale [0, 12] to [6, 32]
vec3 shiftRange(vec3 v, float min1, float max1, float min2, float max2)
{
	v -= min1;
	v *= (max2 - min2) / (max1 - min1);
	v += min2;
	return v;
}
#endif // r_floorLight

void	main()
{
	// compute view direction in world space
	vec3 viewDir = normalize(u_ViewOrigin - var_Position);

	vec2 texCoords = var_TexCoords;

	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);

#if defined(USE_PARALLAX_MAPPING)
	// compute texcoords offset from heightmap
	vec2 texOffset = ParallaxTexOffset(texCoords, viewDir, tangentToWorldMatrix);

	texCoords += texOffset;
#endif // USE_PARALLAX_MAPPING

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texCoords);

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	// compute the specular term
	vec4 specular = texture2D(u_SpecularMap, texCoords);

	// compute normal in world space from normalmap
	vec3 N = NormalInWorldSpace(texCoords, tangentToWorldMatrix);

	// compute light color from world space lightmap
	vec3 lightColor = texture2D(u_LightMap, var_TexLight).xyz;
	
#if defined(r_floorLight)
	// rescale [min1, max1] to [min2, max2]
	// values were chosen the empirical way by testing against some maps on a
	// calibrated screen with 100% sRGB coverage, the tested maps were:
	// parpax, spacetracks, antares, vega, arachnid2, hangar28
	// those values are for non-sRGB lightmaps
	// TODO: find or compute related values for sRGB lightmaps if implemented
	float min1 = 0.0;
	float max1 = 0.18;
	float min2 = 0.06;
	float max2 = max1;
	// isSmaller() produces a pseudo boolean vec3
	// shiftRange() translates the given range
	// shiftRange() - lightColor gives the value of the number to add to lightColor
	// this number being 0 if isSmaller() produces a 0
	lightColor += isSmaller(lightColor, max1) * (shiftRange(lightColor, min1, max1, min2, max2) - lightColor);
#endif // r_floorLight

	vec4 color = vec4( 0.0, 0.0, 0.0, diffuse.a );

#if defined(USE_DELUXE_MAPPING)
	// compute light direction in world space
	vec4 deluxe = texture2D(u_DeluxeMap, var_TexLight);
#else // !USE_DELUXE_MAPPING
	// normal/deluxe mapping is disabled
	color.xyz += lightColor.xyz * diffuse.xyz;
#endif // USE_DELUXE_MAPPING

#if defined(USE_DELUXE_MAPPING)
	vec3 L = 2.0 * deluxe.xyz - 1.0;
	L = normalize(L);

	// divide by cosine term to restore original light color
	lightColor /= clamp(dot(normalize(var_Normal), L), 0.004, 1.0);

	// compute final color
	computeLight( L, N, viewDir, lightColor, diffuse, specular, color );
#endif // USE_DELUXE_MAPPING

	computeDLights( var_Position, N, viewDir, diffuse, specular, color );

#if defined(r_glowMapping)
	color.rgb += texture2D(u_GlowMap, texCoords).rgb;
#endif // r_glowMapping

	outputColor = color;

#if defined(r_showLightMaps)
	outputColor = texture2D(u_LightMap, var_TexLight);
#elif defined(r_showDeluxeMaps)
	outputColor = texture2D(u_DeluxeMap, var_TexLight);
#endif
}
