/*
===========================================================================
Copyright (C) 2008-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* vertexLighting_DBS_world_fp.glsl */

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_SpecularMap;
uniform sampler2D	u_GlowMap;

uniform float		u_AlphaThreshold;
uniform float		u_ParallaxDepthScale;
uniform float		u_ParallaxOffsetBias;
uniform	float		u_LightWrapAround;

uniform sampler3D       u_LightGrid1;
uniform sampler3D       u_LightGrid2;
uniform vec3            u_LightGridOrigin;
uniform vec3            u_LightGridScale;

uniform vec3		u_ViewOrigin;

IN(smooth) vec3		var_Position;
IN(smooth) vec2		var_TexDiffuse;
IN(smooth) vec2		var_TexGlow;
IN(smooth) vec4		var_Color;

IN(smooth) vec2		var_TexNormal;
IN(smooth) vec2		var_TexSpecular;
IN(smooth) vec3		var_Tangent;
IN(smooth) vec3		var_Binormal;

IN(smooth) vec3		var_Normal;

DECLARE_OUTPUT(vec4)

void ReadLightGrid(in vec3 pos, out vec3 lgtDir,
		   out vec3 ambCol, out vec3 lgtCol ) {
	vec4 texel1 = texture3D(u_LightGrid1, pos);
	vec4 texel2 = texture3D(u_LightGrid2, pos);

	ambCol = texel1.xyz;
	lgtCol = texel2.xyz;

	lgtDir.x = (255.0 * texel1.w - 128.0) / 127.0;
	lgtDir.y = (255.0 * texel2.w - 128.0) / 127.0;
	lgtDir.z = 1.0 - abs( lgtDir.x ) - abs( lgtDir.y );

	vec2 signs = 2.0 * step( 0.0, lgtDir.xy ) - vec2( 1.0 );
	if( lgtDir.z < 0.0 ) {
		lgtDir.xy = signs * ( vec2( 1.0 ) - abs( lgtDir.yx ) );
	}

	lgtDir = normalize( lgtDir );
}

void	main()
{
	vec2 texDiffuse = var_TexDiffuse;
	vec2 texGlow = var_TexGlow;

	// compute view direction in world space
	vec3 viewDir = normalize(u_ViewOrigin - var_Position);

	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyx);

	vec3 L, ambCol, dirCol;
	ReadLightGrid( (var_Position - u_LightGridOrigin) * u_LightGridScale,
		       L, ambCol, dirCol);

	vec2 texNormal = var_TexNormal;
	vec2 texSpecular = var_TexSpecular;

#if defined(USE_PARALLAX_MAPPING)
	// compute texcoords offset from heightmap
	vec2 texOffset = ParallaxTexOffset(u_NormalMap, texNormal, u_ParallaxDepthScale, u_ParallaxOffsetBias, viewDir, tangentToWorldMatrix);

	texDiffuse += texOffset;
	texGlow += texOffset;
	texNormal += texOffset;
	texSpecular += texOffset;
#endif // USE_PARALLAX_MAPPING

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse) * var_Color;

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	vec4 specular = texture2D(u_SpecularMap, texSpecular);

	// compute normal in world space from normalmap
	vec3 N = NormalInWorldSpace(u_NormalMap, texNormal, tangentToWorldMatrix);

	// compute final color
	vec4 color = vec4( ambCol * diffuse.xyz, diffuse.a );
	computeLight( L, N, viewDir, dirCol, diffuse, specular, color );

	computeDLights( var_Position, N, viewDir, diffuse, specular, color );

#if defined(r_glowMapping)
	color.rgb += texture2D(u_GlowMap, texGlow).rgb;
#endif // r_glowMapping

	outputColor = color;
}
