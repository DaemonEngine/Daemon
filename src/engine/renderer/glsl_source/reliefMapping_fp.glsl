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
// reliefMapping_fp.glsl - Relief mapping helper functions

#if defined(r_normalMapping) || defined(USE_PARALLAX_MAPPING)
uniform sampler2D	u_NormalMap;
uniform int u_HeightMapInNormalMap;
#endif // r_normalMapping || USE_PARALLAX_MAPPING

#if defined(r_normalMapping)
uniform vec3 u_NormalScale;
#endif // r_normalMapping

#if defined(USE_PARALLAX_MAPPING)
uniform float       u_ParallaxDepthScale;
uniform float       u_ParallaxOffsetBias;
#endif // USE_PARALLAX_MAPPING

// compute normal in tangent space
vec3 NormalInTangentSpace(vec2 texNormal)
{
	vec3 normal;

#if defined(r_normalMapping)
	if (u_HeightMapInNormalMap == 0)
	{
		// the Capcom trick abusing alpha channel of DXT1/5 formats to encode normal map
		// https://github.com/DaemonEngine/Daemon/issues/183#issuecomment-473691252
		//
		// the algorithm also works with normal maps in rgb format without alpha channel
		// but we still must be sure there is no height map in alpha channel hence the test
		//
		// crunch -dxn seems to produce such files, since alpha channel is abused such format
		// is unsuitable to embed height map, then height map must be distributed as loose file
		normal = texture2D(u_NormalMap, texNormal).rga;
		normal.x *= normal.z;
		normal.xy = 2.0 * normal.xy - 1.0;
		normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
	}
	else
	{
		// alpha channel contains the height map so do not try to restore the normal map from it
		normal = texture2D(u_NormalMap, texNormal).rgb;
		normal = 2.0 * normal - 1.0;
	}

	// HACK: 0 normal Z channel can't be good
	if (u_NormalScale.z != 0)
	{
		normal *= u_NormalScale;
	}

#if defined(r_NormalScale)
	normal.z *= r_NormalScale;
#endif
#else // !r_normalMapping
	normal = vec3(0.5, 0.5, 1.0);
#endif // !r_normalMapping

	return normal;
}

// compute normal in worldspace from normalmap
vec3 NormalInWorldSpace(vec2 texNormal, mat3 tangentToWorldMatrix)
{
	// compute normal in tangent space from normalmap
	vec3 normal = NormalInTangentSpace(texNormal);
	// transform normal into world space
	return normalize(tangentToWorldMatrix * normal);
}

#if defined(USE_PARALLAX_MAPPING)
// compute texcoords offset from heightmap
// most of the code doing somewhat the same is likely to be named
// RayIntersectDisplaceMap in other id tech3-based engines
// so please keep the comment above to enable cross-tree look-up
vec2 ParallaxTexOffset(vec2 rayStartTexCoords, vec3 viewDir, mat3 tangentToWorldMatrix)
{
	// compute view direction in tangent space
	vec3 tangentViewDir = normalize(viewDir * tangentToWorldMatrix);

	vec2 displacement = tangentViewDir.xy * -u_ParallaxDepthScale / tangentViewDir.z;

	const int linearSearchSteps = 16;
	const int binarySearchSteps = 6;

	float depthStep = 1.0 / float(linearSearchSteps);
	float topDepth = 1.0 - u_ParallaxOffsetBias;

	// current size of search window
	float currentSize = depthStep;

	// current depth position
	float currentDepth = 0.0;

	// best match found (starts with last position 1.0)
	float bestDepth = 1.0;

	// search front to back for first point inside object
	for(int i = 0; i < linearSearchSteps - 1; ++i)
	{
		currentDepth += currentSize;

		float heightMapDepth = topDepth - texture2D(u_NormalMap, rayStartTexCoords + displacement * currentDepth).a;

		if(bestDepth > 0.996) // if no depth found yet
		{
			if(currentDepth >= heightMapDepth)
			{
				bestDepth = currentDepth;
			}
		}
	}

	currentDepth = bestDepth;

	// recurse around first point (depth) for closest match
	for(int i = 0; i < binarySearchSteps; ++i)
	{
		currentSize *= 0.5;

		float heightMapDepth = topDepth - texture2D(u_NormalMap, rayStartTexCoords + displacement * currentDepth).a;

		if(currentDepth >= heightMapDepth)
		{
			bestDepth = currentDepth;
			currentDepth -= 2.0 * currentSize;
		}

		currentDepth += currentSize;
	}

	return bestDepth * displacement;
}
#endif // USE_PARALLAX_MAPPING
