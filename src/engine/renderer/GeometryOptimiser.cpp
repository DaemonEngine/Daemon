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

bspSurface_t** OptimiseMapGeometryCore( world_t* world, int &numSurfaces ) {
	// mark matching surfaces
	for ( int i = 0; i < world->numnodes - world->numDecisionNodes; i++ ) {
		bspNode_t *leaf = world->nodes + world->numDecisionNodes + i;

		for ( int j = 0; j < leaf->numMarkSurfaces; j++ ) {
			bspSurface_t* surf1 = world->markSurfaces[leaf->firstMarkSurface + j];

			if ( surf1->viewCount != -1 ) {
				continue;
			}

			if ( *surf1->data != surfaceType_t::SF_GRID && *surf1->data != surfaceType_t::SF_TRIANGLES
				&& *surf1->data != surfaceType_t::SF_FACE ) {
				continue;
			}

			shader_t* shader1 = surf1->shader;

			if ( shader1->isPortal || shader1->autoSpriteMode != 0 ) {
				continue;
			}

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

				if ( *surf2->data != surfaceType_t::SF_GRID && *surf2->data != surfaceType_t::SF_TRIANGLES
					&& *surf2->data != surfaceType_t::SF_FACE ) {
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

	bspSurface_t** coreSurfaces = ( bspSurface_t** ) ri.Hunk_AllocateTempMemory( sizeof( bspSurface_t* ) * numSurfaces );

	numSurfaces = 0;
	for ( int k = 0; k < world->numSurfaces; k++ ) {
		bspSurface_t* surface = &world->surfaces[k];

		if ( surface->shader->isPortal ) {
			// HACK: don't use VBO because when adding a portal we have to read back the verts CPU-side
			continue;
		}

		if ( surface->shader->autoSpriteMode != 0 ) {
			// don't use VBO because verts are rewritten each time based on view origin
			continue;
		}

		if ( *surface->data == surfaceType_t::SF_FACE || *surface->data == surfaceType_t::SF_GRID
			|| *surface->data == surfaceType_t::SF_TRIANGLES ) {
			coreSurfaces[numSurfaces++] = surface;
		}
	}

	qsort( coreSurfaces, numSurfaces, sizeof( bspSurface_t* ), LeafSurfaceCompare );

	return coreSurfaces;
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

static void ProcessMaterialSurface( MaterialSurface& surface, std::vector<MaterialSurface>& materialSurfaces,
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
}

std::vector<MaterialSurface> OptimiseMapGeometryMaterial( world_t* world, int numSurfaces ) {
	std::vector<MaterialSurface> materialSurfaces;
	materialSurfaces.reserve( numSurfaces );

	std::vector<MaterialSurface> processedMaterialSurfaces;
	processedMaterialSurfaces.reserve( numSurfaces );

	int surfaceIndex = 0;
	for ( int k = 0; k < world->numSurfaces; k++ ) {
		bspSurface_t* surface = &world->surfaces[k];

		if ( surface->shader->isPortal ) {
			continue;
		}

		if ( surface->shader->autoSpriteMode ) {
			continue;
		}

		if ( *surface->data == surfaceType_t::SF_FACE || *surface->data == surfaceType_t::SF_GRID
			|| *surface->data == surfaceType_t::SF_TRIANGLES ) {
			MaterialSurface srf {};

			srf.shader = surface->shader;
			srf.bspSurface = true;
			srf.fog = surface->fogIndex;

			switch ( *surface->data ) {
				case surfaceType_t::SF_FACE:
					srf.firstIndex = ( ( srfSurfaceFace_t* ) surface->data )->firstIndex;
					srf.count = ( ( srfSurfaceFace_t* ) surface->data )->numTriangles;
					srf.verts = ( ( srfSurfaceFace_t* ) surface->data )->verts;
					srf.tris = ( ( srfSurfaceFace_t* ) surface->data )->triangles;
					break;
				case surfaceType_t::SF_GRID:
					srf.firstIndex = ( ( srfGridMesh_t* ) surface->data )->firstIndex;
					srf.count = ( ( srfGridMesh_t* ) surface->data )->numTriangles;
					srf.verts = ( ( srfGridMesh_t* ) surface->data )->verts;
					srf.tris = ( ( srfGridMesh_t* ) surface->data )->triangles;
					break;
				case surfaceType_t::SF_TRIANGLES:
					srf.firstIndex = ( ( srfTriangles_t* ) surface->data )->firstIndex;
					srf.count = ( ( srfTriangles_t* ) surface->data )->numTriangles;
					srf.verts = ( ( srfTriangles_t* ) surface->data )->verts;
					srf.tris = ( ( srfTriangles_t* ) surface->data )->triangles;
					break;
				default:
					break;
			}

			materialSurfaces.emplace_back( srf );
			surfaceIndex++;
		}
	}

	while ( materialSurfaces.size() ) {
		ProcessMaterialSurface( materialSurfaces.front(), materialSurfaces, processedMaterialSurfaces, verts, indices );
	}

	// qsort( materialSurfaces, numSurfaces, sizeof( bspSurface_t* ), LeafSurfaceCompare );

	return materialSurfaces;
}
