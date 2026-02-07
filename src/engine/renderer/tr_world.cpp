/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_world.c

#include "tr_local.h"
#include "gl_shader.h"
#include "Material.h"

static Cvar::Modified<Cvar::Cvar<bool>> r_showCluster(
	"r_showCluster", "print PVS cluster at current location", Cvar::CHEAT, false );

/*
================
R_CullSurface

Tries to back face cull surfaces before they are lighted or
added to the sorting list.

This will also allow mirrors on both sides of a model without recursion.
================
*/
static bool R_CullSurface( surfaceType_t *surface, shader_t *shader, int planeBits )
{
	srfGeneric_t *gen;
	float        d;

	// allow culling to be disabled
	if ( r_nocull->integer )
	{
		return false;
	}

	// ydnar: made surface culling generic, inline with q3map2 surface classification
	if ( *surface == surfaceType_t::SF_GRID && r_nocurves->integer )
	{
		return true;
	}

	if ( *surface != surfaceType_t::SF_FACE && *surface != surfaceType_t::SF_TRIANGLES && *surface != surfaceType_t::SF_VBO_MESH && *surface != surfaceType_t::SF_GRID )
	{
		return true;
	}

	// get generic surface
	gen = ( srfGeneric_t* ) surface;

	// plane cull
	if ( *surface == surfaceType_t::SF_FACE && r_facePlaneCull->integer )
	{
		srfGeneric_t* srf = ( srfGeneric_t* ) gen;
		d = DotProduct( tr.orientation.viewOrigin, srf->plane.normal ) - srf->plane.dist;

		// don't cull exactly on the plane, because there are levels of rounding
		// through the BSP, ICD, and hardware that may cause pixel gaps if an
		// epsilon isn't allowed here
		if ( shader->cullType == CT_FRONT_SIDED )
		{
			if ( d < -8.0f )
			{
				tr.pc.c_plane_cull_out++;
				return true;
			}
		}
		else if ( shader->cullType == CT_BACK_SIDED )
		{
			if ( d > 8.0f )
			{
				tr.pc.c_plane_cull_out++;
				return true;
			}
		}

		tr.pc.c_plane_cull_in++;
	}

	if ( planeBits )
	{
		cullResult_t cull;

		if ( tr.currentEntity != &tr.worldEntity )
		{
			cull = R_CullLocalBox( gen->bounds );
		}
		else
		{
			cull = R_CullBox( gen->bounds );
		}

		if ( cull == CULL_OUT )
		{
			tr.pc.c_box_cull_out++;
			return true;
		}
		else if ( cull == CULL_CLIP )
		{
			tr.pc.c_box_cull_clip++;
		}
		else
		{
			tr.pc.c_box_cull_in++;
		}
	}

	// must be visible
	return false;
}

/*
======================
R_AddWorldSurface
======================
*/
static bool R_AddWorldSurface( bspSurface_t *surf, int portalNum, int planeBits )
{
	if ( surf->viewCount == tr.viewCountNoReset )
	{
		return false; // already in this view
	}

	surf->viewCount = tr.viewCountNoReset;

	// try to cull before lighting or adding
	if ( R_CullSurface( surf->data, surf->shader, planeBits ) )
	{
		return true;
	}

	R_AddDrawSurf( surf->data, surf->shader, surf->lightmapNum, true, portalNum );
	return true;
}

/*
=============================================================

        BRUSH MODELS

=============================================================
*/

/*
=================
R_AddBSPModelSurfaces
=================
*/
void R_AddBSPModelSurfaces( trRefEntity_t *ent )
{
	bspModel_t *bspModel;
	model_t    *pModel;
	unsigned int i;
	vec3_t     boundsCenter;

	pModel = R_GetModelByHandle( ent->e.hModel );
	bspModel = pModel->bsp;

	// copy local bounds
	for ( i = 0; i < 3; i++ )
	{
		ent->localBounds[ 0 ][ i ] = bspModel->bounds[ 0 ][ i ];
		ent->localBounds[ 1 ][ i ] = bspModel->bounds[ 1 ][ i ];
	}

	R_SetupEntityWorldBounds(ent);

	VectorAdd( ent->worldBounds[ 0 ], ent->worldBounds[ 1 ], boundsCenter );
	VectorScale( boundsCenter, 0.5f, boundsCenter );

	if ( R_CullBox( ent->worldBounds ) == CULL_OUT )
	{
		return;
	}

	for ( i = 0; i < bspModel->numSurfaces; i++ )
	{
		bspSurface_t *surf = bspModel->firstSurface + i;
		R_AddDrawSurf( surf->data, surf->shader, surf->lightmapNum, true );
	}
}

/*
=============================================================

        WORLD MODEL

=============================================================
*/

static void R_AddLeafSurfaces( bspNode_t *node, int planeBits )
{
	int          c;
	bspSurface_t **mark;
	bspSurface_t **view;

	tr.pc.c_leafs++;

	// add to z buffer bounds
	if ( node->mins[ 0 ] < tr.viewParms.visBounds[ 0 ][ 0 ] )
	{
		tr.viewParms.visBounds[ 0 ][ 0 ] = node->mins[ 0 ];
	}

	if ( node->mins[ 1 ] < tr.viewParms.visBounds[ 0 ][ 1 ] )
	{
		tr.viewParms.visBounds[ 0 ][ 1 ] = node->mins[ 1 ];
	}

	if ( node->mins[ 2 ] < tr.viewParms.visBounds[ 0 ][ 2 ] )
	{
		tr.viewParms.visBounds[ 0 ][ 2 ] = node->mins[ 2 ];
	}

	if ( node->maxs[ 0 ] > tr.viewParms.visBounds[ 1 ][ 0 ] )
	{
		tr.viewParms.visBounds[ 1 ][ 0 ] = node->maxs[ 0 ];
	}

	if ( node->maxs[ 1 ] > tr.viewParms.visBounds[ 1 ][ 1 ] )
	{
		tr.viewParms.visBounds[ 1 ][ 1 ] = node->maxs[ 1 ];
	}

	if ( node->maxs[ 2 ] > tr.viewParms.visBounds[ 1 ][ 2 ] )
	{
		tr.viewParms.visBounds[ 1 ][ 2 ] = node->maxs[ 2 ];
	}

	// add the individual surfaces
	mark = tr.world->markSurfaces + node->firstMarkSurface;
	c = node->numMarkSurfaces;
	view = tr.world->viewSurfaces + node->firstMarkSurface;

	while ( c-- )
	{
		// the surface may have already been added if it
		// spans multiple leafs
		R_AddWorldSurface( *view, ( *mark )->portalNum, planeBits);

		( *mark )->viewCount = tr.viewCountNoReset;

		mark++;
		view++;
	}
}

/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode( bspNode_t *node, int planeBits )
{
	do
	{
		// if the node wasn't marked as potentially visible, exit
		if ( node->visCounts[ tr.visIndex ] != tr.visCounts[ tr.visIndex ] )
		{
			return;
		}

		if ( node->contents != -1 && !node->numMarkSurfaces )
		{
			// don't waste time dealing with this empty leaf
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible
		if ( !r_nocull->integer )
		{
			int i;
			int r;

			for ( i = 0; i < FRUSTUM_PLANES; i++ )
			{
				if ( planeBits & ( 1 << i ) )
				{
					r = BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[ i ] );

					if ( r == 2 )
					{
						return; // culled
					}

					if ( r == 1 )
					{
						planeBits &= ~( 1 << i );  // all descendants will also be in front
					}
				}
			}
		}

		backEndData[ tr.smpFrame ]->traversalList[ backEndData[ tr.smpFrame ]->traversalLength++ ] = node;

		if ( node->contents != -1 )
		{
			break;
		}

		float d = DotProduct(tr.viewParms.orientation.viewOrigin, node->plane->normal) - node->plane->dist;

		uint32_t side = d <= 0;

		// recurse down the children, front side first
		R_RecursiveWorldNode( node->children[ side ], planeBits );

		// tail recurse
		node = node->children[ side ^ 1 ];
	}
	while ( true );

	if ( node->numMarkSurfaces )
	{
		// ydnar: moved off to separate function
		R_AddLeafSurfaces( node, planeBits );
	}
}

/*
===============
R_PointInLeaf
===============
*/
bspNode_t *R_PointInLeaf( const vec3_t p )
{
	bspNode_t *node;
	float     d;
	cplane_t  *plane;

	if ( !tr.world )
	{
		Sys::Drop( "R_PointInLeaf: bad model" );
	}

	node = tr.world->nodes;

	while ( true )
	{
		if ( node->contents != -1 )
		{
			break;
		}

		plane = node->plane;
		d = DotProduct( p, plane->normal ) - plane->dist;

		if ( d > 0 )
		{
			node = node->children[ 0 ];
		}
		else
		{
			node = node->children[ 1 ];
		}
	}

	return node;
}

/*
==============
R_ClusterPVS
==============
*/
const byte *R_ClusterPVS( int cluster )
{
	if ( !tr.world || !tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters )
	{
		return tr.world->novis;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

/*
==============
R_ClusterPVVS
==============
*/
static const byte *R_ClusterPVVS( int cluster )
{
	if ( !tr.world || !tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters )
	{
		return tr.world->novis;
	}

	return tr.world->visvis + cluster * tr.world->clusterBytes;
}

/*
=================
R_inPVS
=================
*/
bool R_inPVS( const vec3_t p1, const vec3_t p2 )
{
	bspNode_t  *leaf;
	const byte *vis;

	leaf = R_PointInLeaf( p1 );
	vis = R_ClusterPVS( leaf->cluster );
	leaf = R_PointInLeaf( p2 );

	if ( !( vis[ leaf->cluster >> 3 ] & ( 1 << ( leaf->cluster & 7 ) ) ) )
	{
		return false;
	}

	return true;
}

/*
=================
R_inPVVS
=================
*/
bool R_inPVVS( const vec3_t p1, const vec3_t p2 )
{
	bspNode_t  *leaf;
	const byte *vis;

	leaf = R_PointInLeaf( p1 );
	vis = R_ClusterPVVS( leaf->cluster );
	leaf = R_PointInLeaf( p2 );

	if ( !( vis[ leaf->cluster >> 3 ] & ( 1 << ( leaf->cluster & 7 ) ) ) )
	{
		return false;
	}

	return true;
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
static void R_MarkLeaves()
{
	const byte *vis;
	bspNode_t  *leaf, *parent;
	int        i;
	int        cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if ( r_lockpvs->integer )
	{
		return;
	}

	// current viewcluster
	leaf = R_PointInLeaf( tr.viewParms.pvsOrigin );
	cluster = leaf->cluster;

	bool showClusterModified = !!r_showCluster.GetModifiedValue();

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again
	if( tr.refdef.areamaskModified ) {
		// remark ALL cached visClusters
		for ( i = 0; i < MAX_VISCOUNTS; i++ ) {
			tr.visClusters[ i ] = -1;
		}
		tr.visIndex = 0;
	} else {
		for ( i = 0; i < MAX_VISCOUNTS; i++ ) {
			if ( tr.visClusters[ i ] == cluster ) {
				// if r_showcluster was just turned on, remark everything
				if ( !showClusterModified )
				{
					if ( tr.visClusters[ i ] != tr.visClusters[ tr.visIndex ] && r_showCluster.Get() )
					{
						Log::Notice("found cluster:%i  area:%i  index:%i", cluster, leaf->area, i );
					}

					tr.visIndex = i;
					return;
				}
			}
		}
		tr.visIndex = ( tr.visIndex + 1 ) % MAX_VISCOUNTS;
	}

	tr.visClusters[ tr.visIndex ] = cluster;
	tr.visCounts[ tr.visIndex ]++;

	if ( r_showCluster.Get() )
	{
		Log::Notice("update cluster:%i  area:%i  index:%i", cluster, leaf->area, tr.visIndex );
	}

	if ( r_novis->integer || tr.visClusters[ tr.visIndex ] == -1 )
	{
		for ( i = 0; i < tr.world->numnodes; i++ )
		{
			if ( tr.world->nodes[ i ].contents != CONTENTS_SOLID )
			{
				tr.world->nodes[ i ].visCounts[ tr.visIndex ] = tr.visCounts[ tr.visIndex ];
			}
		}

		return;
	}

	vis = R_ClusterPVS( tr.visClusters[ tr.visIndex ] );

	for ( i = 0, leaf = tr.world->nodes; i < tr.world->numnodes; i++, leaf++ )
	{
		if ( tr.world->vis )
		{
			cluster = leaf->cluster;

			if ( cluster >= 0 && cluster < tr.world->numClusters )
			{
				// check general pvs
				if ( !( vis[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) )
				{
					continue;
				}
			}
		}

		// check if outside map
		if (leaf->area == -1) {
			// can't be visible
			continue;
		}

		// check for door connection
		if ( ( tr.refdef.areamask[ leaf->area >> 3 ] & ( 1 << ( leaf->area & 7 ) ) ) )
		{
			// not visible
			continue;
		}

		parent = leaf;

		do
		{
			if ( parent->visCounts[ tr.visIndex ] == tr.visCounts[ tr.visIndex ] )
			{
				break;
			}

			parent->visCounts[ tr.visIndex ] = tr.visCounts[ tr.visIndex ];
			parent = parent->parent;
		}
		while ( parent );
	}
}

/*
=============
R_AddWorldSurfaces
=============
*/
void R_AddWorldSurfaces()
{
	if ( !r_drawworld->integer )
	{
		return;
	}

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		return;
	}

	tr.currentEntity = &tr.worldEntity;

	// clear out the visible min/max
	ClearBounds( tr.viewParms.visBounds[ 0 ], tr.viewParms.visBounds[ 1 ] );

	// determine which leaves are in the PVS / areamask
	R_MarkLeaves();

	// clear traversal list
	backEndData[ tr.smpFrame ]->traversalLength = 0;

	// update visbounds and add surfaces that weren't cached with VBOs
	R_RecursiveWorldNode( tr.world->nodes, FRUSTUM_CLIPALL );
}
