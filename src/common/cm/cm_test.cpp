/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "cm_local.h"

static Cvar::Cvar<bool> cm_noAreas(VM_STRING_PREFIX "cm_noAreas", "Ignore the areas (ie make them all connected)", Cvar::CHEAT, false);

/*
==================
CM_PointLeafnum_r

==================
*/
int CM_PointLeafnum_r( const vec3_t p, int num )
{
	float    d;
	cNode_t  *node;
	cplane_t *plane;

	while ( num >= 0 )
	{
		node = cm.nodes + num;
		plane = node->plane;

		if ( plane->type < 3 )
		{
			d = p[ plane->type ] - plane->dist;
		}
		else
		{
			d = DotProduct( plane->normal, p ) - plane->dist;
		}

		if ( d < 0 )
		{
			num = node->children[ 1 ];
		}
		else
		{
			num = node->children[ 0 ];
		}
	}

	c_pointcontents++; // optimize counter

	return -1 - num;
}

int CM_PointLeafnum( const vec3_t p )
{
	if ( !cm.numNodes )
	{
		// map not loaded
		return 0;
	}

	return CM_PointLeafnum_r( p, 0 );
}

/*
======================================================================

LEAF LISTING

======================================================================
*/

void CM_StoreLeafs( leafList_t *ll, int nodenum )
{
	int leafNum;

	leafNum = -1 - nodenum;

	// store the lastLeaf even if the list is overflowed
	if ( cm.leafs[ leafNum ].cluster != -1 )
	{
		ll->lastLeaf = leafNum;
	}

	if ( ll->count >= ll->maxcount )
	{
		ll->overflowed = true;
		return;
	}

	ll->list[ ll->count++ ] = leafNum;
}

/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
void CM_BoxLeafnums_r( leafList_t *ll, int nodenum )
{
	cplane_t *plane;
	cNode_t  *node;
	int      s;

	while (true)
	{
		if ( nodenum < 0 )
		{
			ll->storeLeafs( ll, nodenum );
			return;
		}

		node = &cm.nodes[ nodenum ];
		plane = node->plane;
		s = BoxOnPlaneSide( ll->bounds[ 0 ], ll->bounds[ 1 ], plane );

		if ( s == 1 )
		{
			nodenum = node->children[ 0 ];
		}
		else if ( s == 2 )
		{
			nodenum = node->children[ 1 ];
		}
		else
		{
			// go down both
			CM_BoxLeafnums_r( ll, node->children[ 0 ] );
			nodenum = node->children[ 1 ];
		}
	}
}

/*
==================
CM_BoxLeafnums
==================
*/
int CM_BoxLeafnums( const vec3_t mins, const vec3_t maxs, int *list, int listsize, int *lastLeaf )
{
	leafList_t ll;

	cm.checkcount++;

	VectorCopy( mins, ll.bounds[ 0 ] );
	VectorCopy( maxs, ll.bounds[ 1 ] );
	ll.count = 0;
	ll.maxcount = listsize;
	ll.list = list;
	ll.storeLeafs = CM_StoreLeafs;
	ll.lastLeaf = 0;
	ll.overflowed = false;

	CM_BoxLeafnums_r( &ll, 0 );

	*lastLeaf = ll.lastLeaf;
	return ll.count;
}

//====================================================================

/*
==================
CM_PointContents

==================
*/
int CM_PointContents( const vec3_t p, clipHandle_t model )
{
	int      leafnum;
	cLeaf_t  *leaf;
	int      contents;
	cmodel_t *clipm;

	if ( !cm.numNodes )
	{
		// map not loaded
		return 0;
	}

	if ( model )
	{
		clipm = CM_ClipHandleToModel( model );
		leaf = &clipm->leaf;
	}
	else
	{
		leafnum = CM_PointLeafnum_r( p, 0 );
		leaf = &cm.leafs[ leafnum ];
	}

// XreaL BEGIN
	if ( leaf->area == -1 )
	{
		// RB: added this optimization
		// p is in the void and we should return solid so particles can be removed from the void
		return CONTENTS_SOLID;
	}

// XreaL END

	contents = 0;

	const int *firstBrushNum = leaf->firstLeafBrush;
	const int *endBrushNum = firstBrushNum + leaf->numLeafBrushes;
	for ( const int *brushNum = firstBrushNum; brushNum < endBrushNum; brushNum++ )
	{
		const cbrush_t *b = &cm.brushes[ *brushNum ];

		// XreaL BEGIN
		if ( !CM_BoundsIntersectPoint( b->bounds[ 0 ], b->bounds[ 1 ], p ) )
		{
			continue;
		}

		// XreaL END

		// see if the point is in the brush
		const cbrushside_t *firstSide = b->sides;
		const cbrushside_t *endSide = firstSide + b->numsides;
		const cbrushside_t *side;
		for ( side = firstSide; side < endSide; side++ )
		{
			float d = DotProduct( p, side->plane->normal );

// FIXME test for Cash
//          if ( d >= side->plane->dist ) {
			if ( d > side->plane->dist )
			{
				break;
			}
		}

		if ( side == endSide )
		{
			contents |= b->contents;
		}
	}

	return contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles )
{
	vec3_t p_l;
	vec3_t temp;
	vec3_t forward, right, up;

	// subtract origin offset
	VectorSubtract( p, origin, p_l );

	// rotate start and end into the models frame of reference
	if ( model != BOX_MODEL_HANDLE && ( angles[ 0 ] || angles[ 1 ] || angles[ 2 ] ) )
	{
		AngleVectors( angles, forward, right, up );

		VectorCopy( p_l, temp );
		p_l[ 0 ] = DotProduct( temp, forward );
		p_l[ 1 ] = -DotProduct( temp, right );
		p_l[ 2 ] = DotProduct( temp, up );
	}

	return CM_PointContents( p_l, model );
}

/*
===============================================================================

PVS

===============================================================================
*/

byte           *CM_ClusterPVS( int cluster )
{
	if ( cluster < 0 || cluster >= cm.numClusters || !cm.vised )
	{
		return cm.visibility;
	}

	return cm.visibility + cluster * cm.clusterBytes;
}

/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void CM_FloodArea_r( int areaNum, int floodnum )
{
	int     i;
	cArea_t *area;
	int     *con;

	area = &cm.areas[ areaNum ];

	if ( area->floodvalid == cm.floodvalid )
	{
		if ( area->floodnum == floodnum )
		{
			return;
		}

		Sys::Drop( "FloodArea_r: reflooded" );
	}

	area->floodnum = floodnum;
	area->floodvalid = cm.floodvalid;
	con = cm.areaPortals + areaNum * cm.numAreas;

	for ( i = 0; i < cm.numAreas; i++ )
	{
		if ( con[ i ] > 0 )
		{
			CM_FloodArea_r( i, floodnum );
		}
	}
}

/*
====================
CM_FloodAreaConnections

====================
*/
void CM_FloodAreaConnections()
{
	int     i;
	cArea_t *area;
	int     floodnum;

	// all current floods are now invalid
	cm.floodvalid++;
	floodnum = 0;

	area = cm.areas; // Ridah, optimization

	for ( i = 0; i < cm.numAreas; i++, area++ )
	{
		if ( area->floodvalid == cm.floodvalid )
		{
			continue; // already flooded into
		}

		floodnum++;
		CM_FloodArea_r( i, floodnum );
	}
}

/*
====================
CM_AdjustAreaPortalState

====================
*/
void CM_AdjustAreaPortalState( int area1, int area2, bool open )
{
	if ( area1 < 0 || area2 < 0 )
	{
		return;
	}

	if ( area1 >= cm.numAreas || area2 >= cm.numAreas )
	{
		Sys::Drop( "CM_AdjustAreaPortalState: bad area number" );
	}

	if ( open )
	{
		cm.areaPortals[ area1 * cm.numAreas + area2 ]++;
		cm.areaPortals[ area2 * cm.numAreas + area1 ]++;
	}
	else if ( cm.areaPortals[ area2 * cm.numAreas + area1 ] )
	{
		// Ridah, fixes loadgame issue
		cm.areaPortals[ area1 * cm.numAreas + area2 ]--;
		cm.areaPortals[ area2 * cm.numAreas + area1 ]--;

		if ( cm.areaPortals[ area2 * cm.numAreas + area1 ] < 0 )
		{
			Sys::Drop( "CM_AdjustAreaPortalState: negative reference count" );
		}
	}

	CM_FloodAreaConnections();
}

/*
====================
CM_AreasConnected

====================
*/
bool CM_AreasConnected( int area1, int area2 )
{
	if ( cm_noAreas.Get() )
	{
		return true;
	}

	if ( area1 < 0 || area2 < 0 )
	{
		return false;
	}

	if ( area1 >= cm.numAreas || area2 >= cm.numAreas )
	{
		Sys::Drop( "area >= cm.numAreas" );
	}

	if ( cm.areas[ area1 ].floodnum == cm.areas[ area2 ].floodnum )
	{
		return true;
	}

	return false;
}

/*
=================
CM_WriteAreaBits

Writes a bit vector of all the areas
that are in the same flood as the area parameter
Returns the number of bytes needed to hold all the bits.

The bits are OR'd in, so you can CM_WriteAreaBits from multiple
viewpoints and get the union of all visible areas.

This is used to cull non-visible entities from snapshots
=================
*/
int CM_WriteAreaBits( byte *buffer, int area )
{
	int i;
	int floodnum;
	int bytes;

	bytes = ( cm.numAreas + 7 ) >> 3;

	if ( cm_noAreas.Get() || area == -1 )
	{
		// for debugging, send everything
		memset( buffer, 255, bytes );
	}
	else
	{
		floodnum = cm.areas[ area ].floodnum;

		for ( i = 0; i < cm.numAreas; i++ )
		{
			if ( cm.areas[ i ].floodnum == floodnum )
			{
				buffer[ i >> 3 ] |= 1 << ( i & 7 );
			}
		}
	}

	return bytes;
}

// XreaL BEGIN

/*
====================
CM_BoundsIntersect
====================
*/
bool CM_BoundsIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 )
{
	return ( maxs[ 0 ] >= mins2[ 0 ] - SURFACE_CLIP_EPSILON &&
	     maxs[ 1 ] >= mins2[ 1 ] - SURFACE_CLIP_EPSILON &&
	     maxs[ 2 ] >= mins2[ 2 ] - SURFACE_CLIP_EPSILON &&
	     mins[ 0 ] <= maxs2[ 0 ] + SURFACE_CLIP_EPSILON &&
	     mins[ 1 ] <= maxs2[ 1 ] + SURFACE_CLIP_EPSILON &&
	     mins[ 2 ] <= maxs2[ 2 ] + SURFACE_CLIP_EPSILON );
}

/*
====================
CM_BoundsIntersectPoint
====================
*/
bool CM_BoundsIntersectPoint( const vec3_t mins, const vec3_t maxs, const vec3_t point )
{
	return ( maxs[ 0 ] >= point[ 0 ] - SURFACE_CLIP_EPSILON &&
	     maxs[ 1 ] >= point[ 1 ] - SURFACE_CLIP_EPSILON &&
	     maxs[ 2 ] >= point[ 2 ] - SURFACE_CLIP_EPSILON &&
	     mins[ 0 ] <= point[ 0 ] + SURFACE_CLIP_EPSILON &&
	     mins[ 1 ] <= point[ 1 ] + SURFACE_CLIP_EPSILON &&
	     mins[ 2 ] <= point[ 2 ] + SURFACE_CLIP_EPSILON );
}

// XreaL END
