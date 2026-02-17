/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_light.c
#include "tr_local.h"

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

float R_InterpolateLightGrid( world_t *w, int from[3], int to[3],
			      float *factors[3], vec3_t ambientLight,
			      vec3_t directedLight, vec3_t lightDir ) {
	float           totalFactor = 0.0f, factor;
	float           *xFactor, *yFactor, *zFactor;
	int             gridStep[ 3 ];
	int             x, y, z;
	bspGridPoint1_t *gp1;
	bspGridPoint2_t *gp2;

	VectorClear( ambientLight );
	VectorClear( directedLight );
	VectorClear( lightDir );

	gridStep[ 0 ] = 1;
	gridStep[ 1 ] = w->lightGridBounds[ 0 ];
	gridStep[ 2 ] = gridStep[ 1 ] * w->lightGridBounds[ 1 ];

	for( x = from[ 0 ], xFactor = factors[ 0 ]; x <= to[ 0 ];
	     x++, xFactor++ ) {
		if( x < 0 || x >= w->lightGridBounds[ 0 ] )
			continue;

		for( y = from[ 1 ], yFactor = factors[ 1 ]; y <= to[ 1 ];
		     y++, yFactor++ ) {
			if( y < 0 || y >= w->lightGridBounds[ 1 ] )
				continue;

			for( z = from[ 2 ], zFactor = factors[ 2 ]; z <= to[ 2 ];
			     z++, zFactor++ ) {
				if( z < 0 || z >= w->lightGridBounds[ 2 ] )
					continue;

				gp1 = w->lightGridData1 + x * gridStep[ 0 ] + y * gridStep[ 1 ] + z * gridStep[ 2 ];
				gp2 = w->lightGridData2 + x * gridStep[ 0 ] + y * gridStep[ 1 ] + z * gridStep[ 2 ];

				if ( !gp2->isSet )
				{
					continue; // ignore samples in walls
				}

				factor = *xFactor * *yFactor * *zFactor;

				totalFactor += factor;

				lightDir[ 0 ] += factor * snorm8ToFloat( gp2->direction[ 0 ] - 128 );
				lightDir[ 1 ] += factor * snorm8ToFloat( gp2->direction[ 1 ] - 128 );
				lightDir[ 2 ] += factor * snorm8ToFloat( gp2->direction[ 2 ] - 128 );

				float ambientScale = 2.0f * unorm8ToFloat( gp1->ambientPart );
				float directedScale = 2.0f - ambientScale;

				ambientLight[ 0 ] += factor * ambientScale * unorm8ToFloat( gp1->color[ 0 ] );
				ambientLight[ 1 ] += factor * ambientScale * unorm8ToFloat( gp1->color[ 1 ] );
				ambientLight[ 2 ] += factor * ambientScale * unorm8ToFloat( gp1->color[ 2 ] );
				directedLight[ 0 ] += factor * directedScale * unorm8ToFloat( gp1->color[ 0 ] );
				directedLight[ 1 ] += factor * directedScale * unorm8ToFloat( gp1->color[ 1 ] );
				directedLight[ 2 ] += factor * directedScale * unorm8ToFloat( gp1->color[ 2 ] );
			}
		}
	}

	return totalFactor;
}

/*
=================
R_LightForPoint
=================
*/
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir )
{
	int             i;
	int             from[ 3 ], to[ 3 ];
	float           frac[ 3 ][ 2 ];
	float           *factors[ 3 ] = { frac[ 0 ], frac[ 1 ], frac[ 2 ] };
	float           totalFactor;

	// bk010103 - this segfaults with -nolight maps
	if ( tr.world->lightGridData1 == nullptr ||
	     tr.world->lightGridData2 == nullptr )
	{
		return false;
	}

	for ( i = 0; i < 3; i++ )
	{
		float v;

		v = point[ i ] - tr.world->lightGridOrigin[ i ];
		v *= tr.world->lightGridInverseSize[ i ];
		from[ i ] = floor( v );
		frac[ i ][ 0 ] = v - from[ i ];

		if ( from[ i ] < 0 )
		{
			from[ i ] = 0;
			frac[ i ][ 0 ] = 0.0f;
		}
		else if ( from[ i ] >= tr.world->lightGridBounds[ i ] - 1 )
		{
			from[ i ] = tr.world->lightGridBounds[ i ] - 2;
			frac[ i ][ 0 ] = 1.0f;
		}

		to[ i ] = from[ i ] + 1;
		frac[ i ][ 1 ] = 1.0f - frac[ i ][ 0 ];
	}

	totalFactor = R_InterpolateLightGrid( tr.world, from, to, factors,
					      ambientLight, directedLight,
					      lightDir );

	if ( totalFactor > 0 && totalFactor < 0.99 )
	{
		totalFactor = 1.0f / totalFactor;

		VectorScale( lightDir, totalFactor, lightDir );
		VectorScale( ambientLight, totalFactor, ambientLight );
		VectorScale( directedLight, totalFactor, directedLight );
	}

	// compute z-component of direction
	lightDir[ 2 ] = 1.0f - fabsf( lightDir[ 0 ] ) - fabsf( lightDir[ 1 ] );
	if( lightDir[ 2 ] < 0.0f ) {
		float X = lightDir[ 0 ];
		float Y = lightDir[ 1 ];
		lightDir[ 0 ] = copysignf( 1.0f - fabsf( Y ), X );
		lightDir[ 1 ] = copysignf( 1.0f - fabsf( X ), Y );
	}

	VectorNormalize( lightDir );

	if ( ambientLight[ 0 ] < r_forceAmbient.Get() &&
	     ambientLight[ 1 ] < r_forceAmbient.Get() &&
	     ambientLight[ 2 ] < r_forceAmbient.Get() )
	{
		ambientLight[ 0 ] = r_forceAmbient.Get();
		ambientLight[ 1 ] = r_forceAmbient.Get();
		ambientLight[ 2 ] = r_forceAmbient.Get();
	}

	return true;
}

