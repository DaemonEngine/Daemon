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

#if defined(r_normalMapping) || defined(USE_HEIGHTMAP_IN_NORMALMAP)
uniform sampler2D	u_NormalMap;
#endif // r_normalMapping || USE_HEIGHTMAP_IN_NORMALMAP

#if defined(r_normalMapping)
uniform vec3        u_NormalScale;
#endif // r_normalMapping

#if (defined(USE_PARALLAX_MAPPING) && !defined(USE_HEIGHTMAP_IN_NORMALMAP)) || defined(USE_NORMALMAP_FROM_HEIGHTMAP)
uniform sampler2D	u_HeightMap;
#endif // (USE_PARALLAX_MAPPING && !USE_HEIGHTMAP_IN_NORMALMAP) || USE_NORMALMAP_FROM_HEIGHTMAP

#if defined(USE_PARALLAX_MAPPING)
uniform float       u_ParallaxDepthScale;
uniform float       u_ParallaxOffsetBias;
#endif // USE_PARALLAX_MAPPING

#if defined(USE_NORMALMAP_FROM_HEIGHTMAP)
vec3 NormalFromHeightMap(vec2 texNormal)
{
	vec3 normal;

	/* Major inspiration was this post by jollyjeffers:
	https://www.gamedev.net/forums/topic/475213-generate-normal-map-from-heightmap-algorithm/#post_4117038

	and this Wikipedia article:
	https://en.wikipedia.org/wiki/Sobel_operator

	Various people recommends doing abs() on sample read in case of float image format:
	http://www.catalinzima.com/2008/01/converting-displacement-maps-into-normal-maps/
	https://community.khronos.org/t/heightmap-to-normalmap/58862

	Or report issues by not doing it:
	https://gamedev.stackexchange.com/questions/165575/calculating-normal-map-from-height-map-using-sobel-operator

	Other people say texture2D clamps it but it's safer to do this.
	*/

	// Get texel size
	vec2 texelSize = 1.0 / vec2(textureSize(u_HeightMap, 0));

#if defined(r_sobelFiltering)
	/* Useful things to know:
	- It's required to get height samples (S) surrounding the current texel (T) to do a sobel filter:

	  S S S
	  S T S
	  S S S

	- Sobel X kernel is:

	  [ 1  0  -1 ]
	  [ 2  0  -2 ]
	  [ 1  0  -1 ]

	  See https://en.wikipedia.org/wiki/Sobel_operator#Formulation

	- Sobel Y kernel is:

	  [  1  2  1 ]
	  [  0  0  0 ]
	  [ -1 -2 -1 ]

	  Which is the rotated sobel X kernel.

	- Tangent space normals are +Z.
	*/

	ivec3 offsets;
	mat3 xCoords;
	mat3 yCoords;
	mat3 heights;
	mat3 sobel;

	// Components will be computed by accumulating values.
	normal = vec3(0.0, 0.0, 0.0);

	// Set offsets:
	offsets = ivec3(-1, 0, 1);

	// Set sobel X kernel (beware, line is column with this notation):
	sobel[0] = vec3( 1.0,  2.0,  1.0);
	sobel[1] = vec3( 0.0,  0.0,  0.0);
	sobel[2] = vec3(-1.0, -2.0, -1.0);

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (i != 1 || j != 1)
			{
				// Compute coordinates:
				xCoords[i][j] = texNormal.x + texelSize.x * offsets[i];
				yCoords[i][j] = texNormal.y + texelSize.y * offsets[j];

				// Get surrounding samples:
				heights[i][j] = abs(texture2D(u_HeightMap, vec2(xCoords[i][j], yCoords[i][j])).r);

				if (i != 1)
				{
					// Sobel computation for X component (use the X sobel kernel):
					normal.x += heights[i][j] * sobel[i][j];
				}

				if (j != 1)
				{
					// Sobel computation for Y component (use the rotated X sobel kernel as Y one):
					normal.y += heights[i][j] * sobel[j][i];
				}
			}
		}
	}

	/* Reconstruct Z component while making sure to not take the square root
	of a negative number. That may occur because of compression artifacts.

	Xonotic texture known to produce black normalmap artifacts when doing
	Z reconstruction from X and Y computed from jpg heightmap:
	textures/map_glowplant/sand_bump
	*/

	normal.z = sqrt(max(0.0, 1.0 - dot(normal.xy, normal.xy)));

	normal.xy = 2.0 * normal.xy;
#else // !r_sobelFiltering
	/* Useful thing to know:
	- Only three samples are required to determine two vectors
	  they would be used to generate the normal at this texel:

	  T S
	  S

	The texel itself is used as sample.
	*/

	vec2 hOffsets;
	vec2 vOffsets;
	vec2 hCoords;
	vec2 vCoords;
	vec3 heights;
	vec3 xVector;
	vec3 yVector;

	// Set horizontal and vertical offsets:
	hOffsets = vec2(texelSize.x, 0.0);
	vOffsets = vec2(0.0, texelSize.y);

	// Compute coordinates:
	hCoords = vec2(texNormal.x + hOffsets.x, texNormal.y + hOffsets.y);
	vCoords = vec2(texNormal.x + vOffsets.x, texNormal.y + vOffsets.y);

	// Get samples:
	heights.x = texture2D(u_HeightMap, texNormal).r; // No offset.
	heights.y = texture2D(u_HeightMap, hCoords).r;
	heights.z = texture2D(u_HeightMap, vCoords).r;

	xVector = vec3(hOffsets.x, vOffsets.x, heights.y - heights.x);
	yVector = vec3(hOffsets.y, vOffsets.y, heights.z - heights.x);

	normal = cross(xVector, yVector);
#endif // !r_sobelFiltering

	return normal;
}
#endif // USE_NORMALMAP_FROM_HEIGHTMAP

// compute normal in tangent space
vec3 NormalInTangentSpace(vec2 texNormal)
{
	vec3 normal;

#if defined(r_normalMapping)
#if defined(USE_NORMALMAP_FROM_HEIGHTMAP)
	normal = NormalFromHeightMap(texNormal);
#elif defined(USE_HEIGHTMAP_IN_NORMALMAP)
	// alpha channel contains the height map so do not try to reconstruct normal map from it
	normal = texture2D(u_NormalMap, texNormal).rgb;
	normal = 2.0 * normal - 1.0;

	// HACK: the GLSL code is currently assuming
	// DirectX normal map format (+X -Y +Z)
	// but engine is assuming the OpenGL way (+X +Y +Z)
	normal.y *= -1;
#else // !USE_HEIGHTMAP_IN_NORMALMAP
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
	// In a perfect world this code must be enough:
	// normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
	//
	// Unvanquished texture known to trigger black normalmap artifacts
	// when doing Z reconstruction:
	//   textures/shared_pk02_src/rock01_n
	//
	// Although the normal vector is supposed to have a length of 1,
	// dot(normal.xy, normal.xy) may be greater than 1 due to compression
	// artifacts: values as large as 1.27 have been observed with crunch -dxn.
	// https://github.com/DaemonEngine/Daemon/pull/260#issuecomment-571010935
	//
	// This might happen with other formats too. So we must take care not to
	// take the square root of a negative number here.
	normal.z = sqrt(max(0, 1.0 - dot(normal.xy, normal.xy)));

	// HACK: the GLSL code is currently assuming
	// DirectX normal map format (+X -Y +Z)
	// but engine is assuming the OpenGL way (+X +Y +Z)
	normal.y *= -1;
#endif // !USE_HEIGHTMAP_IN_NORMALMAP
	/* Disable normal map scaling when normal Z scale is set to zero.

	This happens when r_normalScale is set to zero because
	u_NormalScale.z is premultiplied with r_normalScale. User can
	disable normal map scaling by setting r_normalScale to zero.

	Normal Z component equal to zero would be wrong anyway.
	*/
	if (u_NormalScale.z != 0)
	{
		normal *= u_NormalScale;
	}
#else // !r_normalMapping
	// Flat normal map is {0.5, 0.5, 1.0} in [ 0.0, 1.0]
	// which is stored as {0.0, 0.0, 1.0} in [-1.0, 1.0].
	normal = vec3(0.0, 0.0, 1.0);
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

#if defined(USE_HEIGHTMAP_IN_NORMALMAP)
		float depth = texture2D(u_NormalMap, rayStartTexCoords + displacement * currentDepth).a;
#else // !USE_HEIGHTMAP_IN_NORMALMAP
		float depth = texture2D(u_HeightMap, rayStartTexCoords + displacement * currentDepth).g;
#endif // !USE_HEIGHTMAP_IN_NORMALMAP

		float heightMapDepth = topDepth - depth;

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

#if defined(USE_HEIGHTMAP_IN_NORMALMAP)
		float depth = texture2D(u_NormalMap, rayStartTexCoords + displacement * currentDepth).a;
#else // !USE_HEIGHTMAP_IN_NORMALMAP
		float depth = texture2D(u_HeightMap, rayStartTexCoords + displacement * currentDepth).g;
#endif // !USE_HEIGHTMAP_IN_NORMALMAP

		float heightMapDepth = topDepth - depth;

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
