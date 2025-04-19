/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2009 Peter McNeill <n27@bigpond.net.au>

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
// GeometryOptimiser.cpp

#include "common/Common.h"

#include "GeometryOptimiser.h"

static int LeafSurfaceCompare( const void* a, const void* b ) {
	bspSurface_t* aa, * bb;

	aa = *( bspSurface_t** ) a;
	bb = *( bspSurface_t** ) b;

	// shader first
	if ( aa->shader < bb->shader ) {
		return -1;
	}

	else if ( aa->shader > bb->shader ) {
		return 1;
	}

	// by lightmap
	if ( aa->lightmapNum < bb->lightmapNum ) {
		return -1;
	}

	else if ( aa->lightmapNum > bb->lightmapNum ) {
		return 1;
	}

	if ( aa->fogIndex < bb->fogIndex ) {
		return -1;
	} else if ( aa->fogIndex > bb->fogIndex ) {
		return 1;
	}

	// sort by leaf
	if ( aa->interactionBits < bb->interactionBits ) {
		return -1;
	} else if ( aa->interactionBits > bb->interactionBits ) {
		return 1;
	}

	// sort by leaf marksurfaces index to increase the likelihood of multidraw merging in the backend
	if ( aa->lightCount < bb->lightCount ) {
		return -1;
	} else if ( aa->lightCount > bb->lightCount ) {
		return 1;
	}
	return 0;
}

static void CoreResetSurfaceViewCounts( bspSurface_t** rendererSurfaces, int numSurfaces ) {
	for ( int i = 0; i < numSurfaces; i++ ) {
		bspSurface_t* surface = rendererSurfaces[i];

		surface->viewCount = -1;
		surface->lightCount = -1;
		surface->interactionBits = 0;
	}
}

void OptimiseMapGeometryCore( world_t* world, bspSurface_t** rendererSurfaces, int numSurfaces ) {
	CoreResetSurfaceViewCounts( rendererSurfaces, numSurfaces );

	// mark matching surfaces
	for ( int i = 0; i < world->numnodes - world->numDecisionNodes; i++ ) {
		bspNode_t *leaf = world->nodes + world->numDecisionNodes + i;

		for ( int j = 0; j < leaf->numMarkSurfaces; j++ ) {
			bspSurface_t* surf1 = world->markSurfaces[leaf->firstMarkSurface + j];

			if ( surf1->viewCount != -1 ) {
				continue;
			}

			if ( !surf1->renderable ) {
				continue;
			}

			shader_t* shader1 = surf1->shader;

			int fogIndex1 = surf1->fogIndex;
			int lightMapNum1 = surf1->lightmapNum;
			surf1->viewCount = surf1 - world->surfaces;
			surf1->lightCount = j;
			surf1->interactionBits = i;

			bool merged = false;
			for ( int k = j + 1; k < leaf->numMarkSurfaces; k++ ) {
				bspSurface_t* surf2 = world->markSurfaces[leaf->firstMarkSurface + k];

				if ( surf2->viewCount != -1 ) {
					continue;
				}

				if ( !surf2->renderable ) {
					continue;
				}

				shader_t* shader2 = surf2->shader;
				int fogIndex2 = surf2->fogIndex;
				int lightMapNum2 = surf2->lightmapNum;
				if ( shader1 != shader2 || fogIndex1 != fogIndex2 || lightMapNum1 != lightMapNum2 ) {
					continue;
				}

				surf2->viewCount = surf1->viewCount;
				surf2->lightCount = k;
				surf2->interactionBits = i;
				merged = true;
			}

			if ( !merged ) {
				surf1->viewCount = -1;
				surf1->lightCount = -1;
				// don't clear the leaf number so
				// surfaces that arn't merged are placed
				// closer to other leafs in the vbo
			}
		}
	}

	qsort( rendererSurfaces, numSurfaces, sizeof( bspSurface_t* ), LeafSurfaceCompare );
}

static void SphereFromBounds( vec3_t mins, vec3_t maxs, vec3_t origin, float* radius ) {
	vec3_t temp;

	VectorAdd( mins, maxs, origin );
	VectorScale( origin, 0.5, origin );
	VectorSubtract( maxs, origin, temp );
	*radius = VectorLength( temp );
}

void MergeLeafSurfacesCore( world_t* world, bspSurface_t** rendererSurfaces, int numSurfaces ) {
	if ( !r_mergeLeafSurfaces->integer ) {
		return;
	}

	// count merged/unmerged surfaces
	int numUnmergedSurfaces = 0;
	int numMergedSurfaces = 0;
	int oldViewCount = -2;

	for ( int i = 0; i < numSurfaces; i++ ) {
		bspSurface_t* surface = rendererSurfaces[i];

		if ( surface->viewCount == -1 ) {
			numUnmergedSurfaces++;
		} else if ( surface->viewCount != oldViewCount ) {
			oldViewCount = surface->viewCount;
			numMergedSurfaces++;
		}
	}

	// Allocate merged surfaces
	world->mergedSurfaces = ( bspSurface_t* ) ri.Hunk_Alloc( sizeof( bspSurface_t ) * numMergedSurfaces, ha_pref::h_low );

	// actually merge surfaces
	bspSurface_t* mergedSurf = world->mergedSurfaces;
	oldViewCount = -2;
	for ( int i = 0; i < numSurfaces; i++ ) {
		vec3_t bounds[2];
		int surfVerts = 0;
		int surfIndexes = 0;
		srfVBOMesh_t* vboSurf;
		bspSurface_t* surf1 = rendererSurfaces[i];

		// skip unmergable surfaces
		if ( surf1->viewCount == -1 ) {
			continue;
		}

		// skip surfaces that have already been merged
		if ( surf1->viewCount == oldViewCount ) {
			continue;
		}

		oldViewCount = surf1->viewCount;

		srfGeneric_t* srf1 = ( srfGeneric_t* ) surf1->data;
		int firstIndex = srf1->firstIndex;

		// count verts and indexes and add bounds for the merged surface
		ClearBounds( bounds[0], bounds[1] );
		for ( int j = i; j < numSurfaces; j++ ) {
			bspSurface_t* surf2 = rendererSurfaces[j];

			// stop merging when we hit a surface that can't be merged
			if ( surf2->viewCount != surf1->viewCount ) {
				break;
			}

			srfGeneric_t* srf2 = ( srfGeneric_t* ) surf2->data;
			surfIndexes += srf2->numTriangles * 3;
			surfVerts += srf2->numVerts;
			BoundsAdd( bounds[0], bounds[1], srf2->bounds[0], srf2->bounds[1] );
		}

		if ( !surfIndexes || !surfVerts ) {
			continue;
		}

		vboSurf = ( srfVBOMesh_t* ) ri.Hunk_Alloc( sizeof( *vboSurf ), ha_pref::h_low );
		*vboSurf = {};
		vboSurf->surfaceType = surfaceType_t::SF_VBO_MESH;

		vboSurf->numTriangles = surfIndexes / 3;
		vboSurf->numVerts = surfVerts;
		vboSurf->firstIndex = firstIndex;

		vboSurf->lightmapNum = surf1->lightmapNum;
		vboSurf->vbo = world->vbo;
		vboSurf->ibo = world->ibo;

		VectorCopy( bounds[0], vboSurf->bounds[0] );
		VectorCopy( bounds[1], vboSurf->bounds[1] );
		SphereFromBounds( vboSurf->bounds[0], vboSurf->bounds[1], vboSurf->origin, &vboSurf->radius );

		mergedSurf->data = ( surfaceType_t* ) vboSurf;
		mergedSurf->fogIndex = surf1->fogIndex;
		mergedSurf->shader = surf1->shader;
		mergedSurf->lightmapNum = surf1->lightmapNum;
		mergedSurf->viewCount = -1;

		// redirect view surfaces to this surf
		for ( int k = 0; k < world->numMarkSurfaces; k++ ) {
			bspSurface_t** view = world->viewSurfaces + k;

			if ( ( *view )->viewCount == surf1->viewCount ) {
				*view = mergedSurf;
			}
		}

		mergedSurf++;
	}

	Log::Debug( "Processed %d surfaces into %d merged, %d unmerged", numSurfaces, numMergedSurfaces, numUnmergedSurfaces );
}

/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2025 Daemon Developers
All rights reserved.

This file is part of the Daemon BSD Source Code (Daemon Source Code).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the Daemon developers nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===========================================================================
*/
void MergeDuplicateVertices( bspSurface_t** rendererSurfaces, int numSurfaces, srfVert_t* vertices, int numVerticesIn,
	glIndex_t* indices, int numIndicesIn, int& numVerticesOut, int& numIndicesOut ) {
	int start = Sys::Milliseconds();

	std::unordered_map<srfVert_t, uint32_t, MapVertHasher, MapVertEqual> verts;
	uint32_t idx = 0;
	uint32_t vertIdx = 0;

	// To shut CI up since this *is* used for an assert
	Q_UNUSED( numIndicesIn );
	for ( int i = 0; i < numSurfaces; i++ ) {
		bspSurface_t* surface = rendererSurfaces[i];
		
		srfGeneric_t* srf = ( srfGeneric_t* ) surface->data;
		srf->firstIndex = idx;
		for ( srfTriangle_t* triangle = srf->triangles; triangle < srf->triangles + srf->numTriangles; triangle++ ) {
			for ( int j = 0; j < 3; j++ ) {
				srfVert_t& vert = srf->verts[triangle->indexes[j]];
				uint32_t index = verts[vert];

				/* There were some crashes due to bad lightmap values in .bsp vertices,
				do the check again here just in case some calculation earlier, like patch mesh triangulation,
				fucks things up again */
				ValidateVertex( &vert, -1, surface->shader );

				ASSERT_LT( idx, ( uint32_t ) numIndicesIn );
				if ( !index ) {
					verts[vert] = vertIdx + 1;
					vertices[vertIdx] = vert;
					indices[idx] = vertIdx;

					ASSERT_LT( vertIdx, ( uint32_t ) numVerticesIn );

					vertIdx++;
				} else {
					indices[idx] = index - 1;
				}
				idx++;
			}
		}
	}

	numVerticesOut = vertIdx;
	numIndicesOut = idx;

	Log::Notice( "Merged %i vertices into %i in %i ms", numVerticesIn, numVerticesOut, Sys::Milliseconds() - start );
}

/* static void ProcessMaterialSurface( MaterialSurface& surface, std::vector<MaterialSurface>& materialSurfaces,
	std::vector<MaterialSurface>& processedMaterialSurfaces,
	srfVert_t* verts, glIndex_t* indices ) {
	if ( surface.count == MAX_MATERIAL_SURFACE_TRIS ) {
		materialSurfaces.emplace_back( surface );
	}

	while ( surface.count > MAX_MATERIAL_SURFACE_TRIS ) {
		MaterialSurface srf = surface;

		srf.count = MAX_MATERIAL_SURFACE_TRIS;
		surface.count -= MAX_MATERIAL_SURFACE_TRIS;
		surface.firstIndex += MAX_MATERIAL_SURFACE_TRIS;

		processedMaterialSurfaces.push_back( srf );
	}
} */

std::vector<MaterialSurface> OptimiseMapGeometryMaterial(bspSurface_t** rendererSurfaces, int numSurfaces ) {
	std::vector<MaterialSurface> materialSurfaces;
	materialSurfaces.reserve( numSurfaces );

	std::vector<MaterialSurface> processedMaterialSurfaces;
	processedMaterialSurfaces.reserve( numSurfaces );

	// std::unordered_map<TriEdge, TriIndex> triEdges;

	vec3_t worldBounds[2] = {};
	for ( int i = 0; i < numSurfaces; i++ ) {
		bspSurface_t* surface = rendererSurfaces[i];

		if ( surface->BSPModel ) {
			// Not implemented yet
			continue;
		}

		MaterialSurface srf {};

		srf.shader = surface->shader;

		srf.bspSurface = true;
		srf.skyBrush = surface->skyBrush;

		srf.lightMapNum = surface->lightmapNum;
		srf.fog = surface->fogIndex;
		srf.portalNum = surface->portalNum;

		srf.firstIndex = ( ( srfGeneric_t* ) surface->data )->firstIndex;
		srf.count = ( ( srfGeneric_t* ) surface->data )->numTriangles * 3;
		srf.verts = ( ( srfGeneric_t* ) surface->data )->verts;
		srf.tris = ( ( srfGeneric_t* ) surface->data )->triangles;

		VectorCopy( ( ( srfGeneric_t* ) surface->data )->origin, srf.origin );
		srf.radius = ( ( srfGeneric_t* ) surface->data )->radius;

		BoundsAdd( worldBounds[0], worldBounds[1],
			( ( srfGeneric_t* ) surface->data )->bounds[0], ( ( srfGeneric_t* ) surface->data )->bounds[1] );

		materialSystem.GenerateMaterial( &srf );

		materialSurfaces.emplace_back( srf );
	}

	materialSystem.GenerateWorldMaterialsBuffer();
	materialSystem.GeneratePortalBoundingSpheres();
	materialSystem.SetWorldBounds( worldBounds );
	materialSystem.GenerateWorldCommandBuffer( materialSurfaces );

	return materialSurfaces;
}
