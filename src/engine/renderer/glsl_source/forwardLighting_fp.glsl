/*
===========================================================================
Copyright (C) 2007-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* forwardLighting_fp.glsl */

// computeSpecularity is the only thing used from this file
#insert computeLight_fp

#insert reliefMapping_fp

/* swizzle one- and two-component textures to RG */
#if defined(HAVE_ARB_texture_rg)
#  define SWIZ1 r
#  define SWIZ2 rg
#else
#  define SWIZ1 a
#  define SWIZ2 ar
#endif

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_MaterialMap;
uniform sampler2D	u_AttenuationMapXY;
uniform sampler2D	u_AttenuationMapZ;

uniform sampler2D	u_RandomMap;	// random normals

uniform vec3		u_ViewOrigin;

#if defined(LIGHT_DIRECTIONAL)
uniform vec3		u_LightDir;
#else
uniform vec3		u_LightOrigin;
#endif
uniform vec3		u_LightColor;
uniform float		u_LightRadius;
uniform float       u_LightScale;
uniform float		u_AlphaThreshold;

uniform mat4		u_ViewMatrix;

IN(smooth) vec3		var_Position;
IN(smooth) vec2		var_TexCoords;
IN(smooth) vec4		var_TexAttenuation;
IN(smooth) vec4		var_Tangent;
IN(smooth) vec4		var_Binormal;
IN(smooth) vec4		var_Normal;
IN(smooth) vec4		var_Color;

DECLARE_OUTPUT(vec4)

float Rand(vec2 co)
{
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec3 RandomVec3(vec2 uv)
{
	vec3 dir;

#if 1
	float r = Rand(uv);
	float angle = 2.0 * M_PI * r;// / 360.0;

	dir = normalize(vec3(cos(angle), sin(angle), r));
#else
	// dir = texture2D(u_NoiseMap, gl_FragCoord.st / r_FBufSize).rgb;
	dir = normalize(2.0 * (texture2D(u_RandomMap, uv).xyz - 0.5));
#endif

	return dir;
}

/*
float log_conv(float x0, float X, float y0, float Y)
{
    return (X + log(x0 + (y0 * exp(Y - X))));
}
*/

void	main()
{
#if 0
	// create random noise vector
	vec3 rand = RandomVec3(gl_FragCoord.st / r_FBufSize);

	outputColor = vec4(rand * 0.5 + 0.5, 1.0);
	return;
#endif

	// compute light direction in world space
#if defined(LIGHT_DIRECTIONAL)
	vec3 lightDir = u_LightDir;
#else
	vec3 lightDir = normalize(u_LightOrigin - var_Position);
#endif

	vec2 texCoords = var_TexCoords;

	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);

	// compute view direction in world space
	vec3 viewDir = normalize(u_ViewOrigin - var_Position.xyz);

#if defined(USE_RELIEF_MAPPING)
	// compute texcoords offset from heightmap
	#if defined(USE_HEIGHTMAP_IN_NORMALMAP)
		vec2 texOffset = ReliefTexOffset(texCoords, viewDir, tangentToWorldMatrix, u_NormalMap);
	#else
		vec2 texOffset = ReliefTexOffset(texCoords, viewDir, tangentToWorldMatrix, u_HeightMap);
	#endif

	texCoords += texOffset;
#endif // USE_RELIEF_MAPPING

	// compute half angle in world space
	vec3 H = normalize(lightDir + viewDir);

	// compute normal in world space from normal map
	vec3 normal = NormalInWorldSpace(texCoords, tangentToWorldMatrix, u_NormalMap);

	// compute the light term
	float NL = clamp(dot(normal, lightDir), 0.0, 1.0);

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texCoords);
	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}
	diffuse.rgb *= u_LightColor * NL;

#if !defined(USE_PHYSICAL_MAPPING)
#if defined(r_specularMapping)
	// compute the specular term
	vec4 materialColor = texture2D(u_MaterialMap, texCoords);
	float NdotH = clamp(dot(normal, H), 0.0, 1.0);
	vec3 specular = computeSpecularity(u_LightColor, materialColor, NdotH);
#endif // r_specularMapping
#endif // !USE_PHYSICAL_MAPPING

	// compute light attenuation
#if defined(LIGHT_PROJ)
	vec3 attenuationXY = texture2DProj(u_AttenuationMapXY, var_TexAttenuation.xyw).rgb;
	vec3 attenuationZ  = texture2D(u_AttenuationMapZ, vec2(var_TexAttenuation.z + 0.5, 0.0)).rgb; // FIXME

#elif defined(LIGHT_DIRECTIONAL)
	vec3 attenuationXY = vec3(1.0);
	vec3 attenuationZ  = vec3(1.0);

#else
	vec3 attenuationXY = texture2D(u_AttenuationMapXY, var_TexAttenuation.xy).rgb;
	vec3 attenuationZ  = texture2D(u_AttenuationMapZ, vec2(var_TexAttenuation.z, 0)).rgb;
#endif

	// compute final color
	vec4 color = diffuse;

#if !defined(USE_PHYSICAL_MAPPING)
#if defined(r_specularMapping)
	color.rgb += specular;
#endif // r_specularMapping
#endif // !USE_PHYSICAL_MAPPING

#if !defined(LIGHT_DIRECTIONAL)
	color.rgb *= attenuationXY;
	color.rgb *= attenuationZ;
#endif
	color.rgb *= abs(u_LightScale);

	color.rgb *= var_Color.rgb;

	if( u_LightScale < 0.0 ) {
		color.rgb = vec3( clamp(dot(color.rgb, vec3( 0.3333 ) ), 0.3, 0.7 ) );
	}

	outputColor = color;

#if 0
#if defined(USE_RELIEF_MAPPING)
	outputColor = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
#else
	outputColor = vec4(vec3(0.0, 0.0, 1.0), diffuse.a);
#endif
#endif

#if 0
#if defined(USE_VERTEX_SKINNING)
	outputColor = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
#elif defined(USE_VERTEX_ANIMATION)
	outputColor = vec4(vec3(0.0, 0.0, 1.0), diffuse.a);
#else
	outputColor = vec4(vec3(0.0, 1.0, 0.0), diffuse.a);
#endif
#endif
}
