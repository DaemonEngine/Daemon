/*
===========================================================================
Copyright (C) 2009-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// computeLight_fp.glsl - Light computing helper functions

#define COMPUTELIGHT_GLSL

#if !defined(USE_BSP_SURFACE)
	#define USE_MODEL_SURFACE
#endif

#if !defined(USE_GRID_LIGHTING)
	#define USE_LIGHT_MAPPING
#endif

#if defined(USE_REFLECTIVE_SPECULAR)
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float u_EnvironmentInterpolation;
#endif // USE_REFLECTIVE_SPECULAR

#if defined(r_realtimeLighting) && r_realtimeLightingRenderer == 1
struct light {
  vec4  center_radius;
  vec4  color_type;
  vec4  direction_angle;
};

layout(std140) uniform u_Lights {
  light lights[ MAX_REF_LIGHTS ];
};
#define GetLight(idx, component) lights[idx].component

uniform int u_numLights;
#endif // defined(r_realtimeLighting) && r_realtimeLightingRenderer == 1

// lighting helper functions

#if defined(USE_GRID_LIGHTING) || defined(USE_GRID_DELUXE_MAPPING)
void ReadLightGrid(in vec4 texel, out vec3 ambientColor, out vec3 lightColor) {
	float ambientScale = 2.0 * texel.a;
	float directedScale = 2.0 - ambientScale;
	ambientColor = ambientScale * texel.rgb;
	lightColor = directedScale * texel.rgb;
}
#endif

#if defined(USE_DELUXE_MAPPING) || defined(USE_GRID_DELUXE_MAPPING) || (defined(r_realtimeLighting) && r_realtimeLightingRenderer == 1)
uniform vec2 u_SpecularExponent;

#if defined(USE_REFLECTIVE_SPECULAR)
void computeDeluxeLight( vec3 lightDir, vec3 normal, vec3 viewDir, vec3 lightColor,
		   vec4 diffuseColor, vec4 materialColor,
		   inout vec4 color, in samplerCube u_EnvironmentMap0, in samplerCube u_EnvironmentMap1 )
#else // !USE_REFLECTIVE_SPECULAR
void computeDeluxeLight( vec3 lightDir, vec3 normal, vec3 viewDir, vec3 lightColor,
		   vec4 diffuseColor, vec4 materialColor,
		   inout vec4 color )
#endif // !USE_REFLECTIVE_SPECULAR
{
  vec3 H = normalize( lightDir + viewDir );

#if defined(USE_PHYSICAL_MAPPING) || defined(r_specularMapping)
  float NdotH = clamp( dot( normal, H ), 0.0, 1.0 );
#endif // USE_PHYSICAL_MAPPING || r_specularMapping

  // clamp( NdotL, 0.0, 1.0 ) is done below
  float NdotL = dot( normal, lightDir );

#if !defined(USE_BSP_SURFACE) && defined(r_halfLambertLighting)
  // http://developer.valvesoftware.com/wiki/Half_Lambert
  NdotL = NdotL * 0.5 + 0.5;
  NdotL *= NdotL;
#endif

  NdotL = clamp( NdotL, 0.0, 1.0 );

#if defined(USE_PHYSICAL_MAPPING)
  // Daemon PBR packing defaults to ORM like glTF 2.0 defines
  // https://www.khronos.org/blog/art-pipeline-for-gltf
  // > ORM texture for Occlusion, Roughness, and Metallic
  // https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/schema/material.pbrMetallicRoughness.schema.json
  // > The metalness values are sampled from the B channel. The roughness values are sampled from the G channel.
  // > These values are linear. If other channels are present (R or A), they are ignored for metallic-roughness calculations.
  // https://docs.blender.org/manual/en/2.80/addons/io_scene_gltf2.html
  // > glTF stores occlusion in the red (R) channel, allowing it to optionally share the same image
  // > with the roughness and metallic channels.
  float roughness = materialColor.g;
  float metalness = materialColor.b;

  float NdotV = clamp( dot( normal, viewDir ), 0.0, 1.0);
  float VdotH = clamp( dot( viewDir, H ), 0.0, 1.0);

  float alpha = roughness * roughness;
  float k = 0.125 * (roughness + 1.0) * (roughness + 1.0);

  float D = alpha / ((NdotH * NdotH) * (alpha * alpha - 1.0) + 1.0);
  D *= D;

  float FexpNH = pow(1.0 - NdotH, 5.0);
  float FexpNV = pow(1.0 - NdotV, 5.0);
  vec3 F = mix(vec3(0.04), diffuseColor.rgb, metalness);
  F = F + (1.0 - F) * FexpNH;

  float G = NdotL / (NdotL * (1.0 - k) + k);
  G *= NdotV / (NdotV * (1.0 - k) + k);

  vec3 diffuseBRDF = NdotL * diffuseColor.rgb * (1.0 - metalness);
  vec3 specularBRDF = vec3((D * F * G) / max(4.0 * NdotL * NdotV, 0.0001f));
  color.rgb += (diffuseBRDF + specularBRDF) * lightColor.rgb * NdotL;
  color.a = mix(diffuseColor.a, 1.0, FexpNV);
#else // !USE_PHYSICAL_MAPPING

#if defined(USE_REFLECTIVE_SPECULAR)
	// not implemented for PBR yet
	vec4 envColor0 = textureCube(u_EnvironmentMap0, reflect(-viewDir, normal));
	vec4 envColor1 = textureCube(u_EnvironmentMap1, reflect(-viewDir, normal));

	materialColor.rgb *= mix(envColor0, envColor1, u_EnvironmentInterpolation).rgb;
#endif // USE_REFLECTIVE_SPECULAR

  color.rgb += lightColor.rgb * NdotL * diffuseColor.rgb;
#if defined(r_specularMapping)
  // The minimal specular exponent should preferably be nonzero to avoid the undefined pow(0, 0)
  color.rgb += lightColor.rgb * materialColor.rgb * pow( NdotH, u_SpecularExponent.x * materialColor.a + u_SpecularExponent.y) * r_SpecularScale;
#endif // r_specularMapping
#endif // !USE_PHYSICAL_MAPPING
}
#endif // defined(USE_DELUXE_MAPPING) || defined(USE_GRID_DELUXE_MAPPING) || (defined(r_realtimeLighting) && r_realtimeLightingRenderer == 1)

#if !defined(USE_DELUXE_MAPPING) && !defined(USE_GRID_DELUXE_MAPPING)
void computeLight(in vec3 lightColor, vec4 diffuseColor, inout vec4 color) {
	color.rgb += lightColor.rgb * diffuseColor.rgb;
}
#endif // !defined(USE_DELUXE_MAPPING) && !defined(USE_GRID_DELUXE_MAPPING)

#if defined(r_realtimeLighting) && r_realtimeLightingRenderer == 1
const int lightsPerLayer = 16;

#define lightTilesSampler_t usampler3D
#define lightTilesUniform u_LightTilesInt
#define idxs_t uvec4

idxs_t fetchIdxs( in vec3 coords, in lightTilesSampler_t lightTilesUniform ) {
  return texture3D( lightTilesUniform, coords );
}

int nextIdx( inout idxs_t idxs ) {
  uvec4 tmp = ( idxs & uvec4( 3 ) ) * uvec4( 0x40, 0x10, 0x04, 0x01 );
  idxs = idxs >> 2;
  return int( tmp.x + tmp.y + tmp.z + tmp.w );
}

uniform lightTilesSampler_t lightTilesUniform;

const int numLayers = MAX_REF_LIGHTS / 256;

#if defined(USE_REFLECTIVE_SPECULAR)
void computeDynamicLight( int idx, vec3 P, vec3 normal, vec3 viewDir, vec4 diffuse,
		    vec4 material, inout vec4 color, in samplerCube u_EnvironmentMap0, in samplerCube u_EnvironmentMap1 )
#else // !USE_REFLECTIVE_SPECULAR
void computeDynamicLight( int idx, vec3 P, vec3 normal, vec3 viewDir, vec4 diffuse,
		    vec4 material, inout vec4 color )
#endif // !USE_REFLECTIVE_SPECULAR
{
  vec4 center_radius = GetLight( idx, center_radius );
  vec4 color_type = GetLight( idx, color_type );
  vec3 L;
  float attenuation;

  if( color_type.w == 0.0 ) {
    // point light
    L = center_radius.xyz - P;
    // 2.57 ~= 8.0 ^ ( 1.0 / 2.2 ), adjusted after overbright changes
    float t = 1.0 + 2.57 * length(L) / center_radius.w;
    // Quadratic attenuation function instead of linear because of overbright changes
    attenuation = 1.0 / ( t * t );
    L = normalize(L);
  } else if( color_type.w == 1.0 ) {
    // spot light
    vec4 direction_angle = GetLight( idx, direction_angle );
    L = center_radius.xyz - P;
    // 2.57 ~= 8.0 ^ ( 1.0 / 2.2 ), adjusted after overbright changes
    float t = 1.0 + 2.57 * length(L) / center_radius.w;
    // Quadratic attenuation function instead of linear because of overbright changes
    attenuation = 1.0 / ( t * t );
    L = normalize( L );

    if( dot( L, direction_angle.xyz ) <= direction_angle.w ) {
      attenuation = 0.0;
    }
  } else if( color_type.w == 2.0 ) {
    // sun (directional) light
    L = GetLight( idx, direction_angle ).xyz;
    attenuation = 1.0;
  }
  #if defined(USE_REFLECTIVE_SPECULAR)
  computeDeluxeLight( L, normal, viewDir,
		attenuation * attenuation * color_type.xyz,
		diffuse, material, color, u_EnvironmentMap0, u_EnvironmentMap1 );
  #else // !USE_REFLECTIVE_SPECULAR
  computeDeluxeLight( L, normal, viewDir,
		attenuation * attenuation * color_type.xyz,
		diffuse, material, color );
  #endif // !USE_REFLECTIVE_SPECULAR
}

#if defined(USE_REFLECTIVE_SPECULAR)
void computeDynamicLights( vec3 P, vec3 normal, vec3 viewDir, vec4 diffuse, vec4 material,
	inout vec4 color, in lightTilesSampler_t lightTilesUniform,
	in samplerCube u_EnvironmentMap0, in samplerCube u_EnvironmentMap1 )
#else // !USE_REFLECTIVE_SPECULAR
void computeDynamicLights( vec3 P, vec3 normal, vec3 viewDir, vec4 diffuse, vec4 material,
	inout vec4 color, in lightTilesSampler_t lightTilesUniform )
#endif // !USE_REFLECTIVE_SPECULAR
{
  vec2 tile = floor( gl_FragCoord.xy * (1.0 / float( TILE_SIZE ) ) ) + 0.5;
  vec3 tileScale = vec3( r_tileStep, 1.0/numLayers );

#if defined(r_showLightTiles)
  float numLights = 0.0;
#endif

  for( int layer = 0; layer < numLayers; layer++ ) {
    idxs_t idxs = fetchIdxs( tileScale * vec3( tile, float( layer ) + 0.5 ), lightTilesUniform );
    for( int i = 0; i < lightsPerLayer; i++ ) {
      int idx = numLayers * nextIdx( idxs ) + layer;

      if( idx >= u_numLights )
      {
        break;
      }
      
      #if defined(USE_REFLECTIVE_SPECULAR)
        computeDynamicLight( idx, P, normal, viewDir, diffuse, material, color, u_EnvironmentMap0, u_EnvironmentMap1 );
      #else // !USE_REFLECTIVE_SPECULAR
        computeDynamicLight( idx, P, normal, viewDir, diffuse, material, color );
      #endif // !USE_REFLECTIVE_SPECULAR

#if defined(r_showLightTiles)
      numLights++;
#endif
    }
  }

#if defined(r_showLightTiles)
  if (numLights > 0.0)
  {
    color = vec4(numLights/(lightsPerLayer*numLayers), numLights/(lightsPerLayer*numLayers), numLights/(lightsPerLayer*numLayers), 1.0);
  }
#endif
}
#endif // defined(r_realtimeLighting) && r_realtimeLightingRenderer == 1
