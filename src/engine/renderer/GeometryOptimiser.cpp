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

#include "ShadeCommon.h"
#include "GeometryCache.h"

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

	// sort by leaf
	if ( aa->scratch2 < bb->scratch2 ) {
		return -1;
	} else if ( aa->scratch2 > bb->scratch2 ) {
		return 1;
	}

	// sort by leaf marksurfaces index to increase the likelihood of multidraw merging in the backend
	if ( aa->scratch1 < bb->scratch1 ) {
		return -1;
	} else if ( aa->scratch1 > bb->scratch1 ) {
		return 1;
	}
	return 0;
}

void MarkShaderBuildNONE( const shaderStage_t* ) {
	ASSERT_UNREACHABLE();
}

void MarkShaderBuildNOP( const shaderStage_t* ) {
}

void MarkShaderBuildGeneric3D( const shaderStage_t* pStage ) {
	ProcessShaderGeneric3D( pStage );
	gl_genericShader->MarkProgramForBuilding();
}

void MarkShaderBuildLightMapping( const shaderStage_t* pStage ) {
	ProcessShaderLightMapping( pStage );
	gl_lightMappingShader->MarkProgramForBuilding();
}

void MarkShaderBuildReflection( const shaderStage_t* pStage ) {
	ProcessShaderReflection( pStage );
	gl_reflectionShader->MarkProgramForBuilding();
}

void MarkShaderBuildSkybox( const shaderStage_t* ) {
	gl_skyboxShader->MarkProgramForBuilding();
}

void MarkShaderBuildScreen( const shaderStage_t* ) {
	gl_screenShader->MarkProgramForBuilding();
}

void MarkShaderBuildPortal( const shaderStage_t* ) {
	gl_portalShader->MarkProgramForBuilding();
}

void MarkShaderBuildHeatHaze( const shaderStage_t* pStage ) {
	ProcessShaderHeatHaze( pStage );
	gl_heatHazeShader->MarkProgramForBuilding();
}

void MarkShaderBuildLiquid( const shaderStage_t* pStage ) {
	ProcessShaderLiquid( pStage );
	gl_liquidShader->MarkProgramForBuilding();
}

void MarkShaderBuild( shader_t* shader, const int lightMapNum, const bool bspSurface,
	const bool vertexSkinning, const bool vertexAnimation ) {
	auto tessBackup = tess;

	tess.surfaceShader = shader;
	tess.bspSurface = bspSurface;
	tess.lightmapNum = lightMapNum;

	tess.vboVertexSkinning = vertexSkinning;
	tess.vboVertexAnimation = vertexAnimation;

	for ( const shaderStage_t* pStage = shader->stages; pStage < shader->lastStage; pStage++ ) {
		pStage->shaderBuildMarker( pStage );
	}

	tess = tessBackup;
}

static void CoreResetSurfaceViewCounts( bspSurface_t** rendererSurfaces, int numSurfaces ) {
	for ( int i = 0; i < numSurfaces; i++ ) {
		bspSurface_t* surface = rendererSurfaces[i];

		surface->viewCount = -1;
		surface->scratch1 = -1;
		surface->scratch2 = 0;
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

			int lightMapNum1 = surf1->lightmapNum;
			surf1->viewCount = surf1 - world->surfaces;
			surf1->scratch1 = j;
			surf1->scratch2 = i;

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
				int lightMapNum2 = surf2->lightmapNum;
				if ( shader1 != shader2 || lightMapNum1 != lightMapNum2 ) {
					continue;
				}

				surf2->viewCount = surf1->viewCount;
				surf2->scratch1 = k;
				surf2->scratch2 = i;
				merged = true;
			}

			if ( !merged ) {
				surf1->viewCount = -1;
				surf1->scratch1 = -1;
				// don't clear the leaf number so
				// surfaces that arn't merged are placed
				// closer to other leafs in the vbo
			}
		}
	}

	qsort( rendererSurfaces, numSurfaces, sizeof( bspSurface_t* ), LeafSurfaceCompare );

	if ( r_lazyShaders.Get() == 1 ) {
		for ( int i = 0; i < numSurfaces; i++ ) {
			bspSurface_t* surface = rendererSurfaces[i];

			MarkShaderBuild( surface->shader, surface->lightmapNum, true, false, false );
		}
	}
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

static void ProcessMaterialSurface( MaterialSurface* surface, SurfaceIndexes* surfaceIdxs,
	std::vector<MaterialSurface>& processedSurfaces,
	const srfVert_t* vertexes, glIndex_t* idxs, uint32_t* numIndices ) {
	vec3_t mins;
	vec3_t maxs;
	ClearBounds( mins, maxs );

	for ( uint32_t i = 0; i < surface->count; i++ ) {
		AddPointToBounds( vertexes[surfaceIdxs->idxs[i]].xyz, mins, maxs );
	}

	const uint32_t oldFirstIndex = surface->firstIndex;
	surface->firstIndex = *numIndices;

	memcpy( idxs + ( *numIndices ), surfaceIdxs->idxs, surface->count * sizeof( glIndex_t ) );

	*numIndices += surface->count;

	SphereFromBounds( mins, maxs, surface->origin, &surface->radius );

	processedSurfaces.emplace_back( *surface );

	surface->firstIndex = oldFirstIndex;
}

static void OptimiseMaterialSurfaces( std::vector<MaterialSurface>& materialSurfaces,
	const uint32_t gridSize[3], const uint32_t cellSize[3], const vec3_t worldMins,
	std::function<bool( const MaterialSurface&, const MaterialSurface& )> materialSurfaceSort,
	const srfVert_t* vertices, const glIndex_t* indices,
	glIndex_t* idxs,
	std::vector<MaterialSurface>& processedMaterialSurfaces, uint32_t& numIndices ) {

	Grid<uint32_t> surfaceGrid( true );
	surfaceGrid.SetSize( gridSize[0], gridSize[1], gridSize[2] );
	surfaceGrid.Clear();

	Grid<SurfaceIndexes> surfaceIdxsGrid( true );
	surfaceIdxsGrid.SetSize( gridSize[0], gridSize[1], gridSize[2] );

	MaterialSurface* surface = &materialSurfaces[0];
	MaterialSurface* surface2;
	bool addedSurfaces = false;

	std::function<void()> processGrid = [&]() {
		if ( !addedSurfaces ) {
			surface = surface2;
			return;
		}

		for ( Grid<uint32_t>::Iterator it = surfaceGrid.begin(); it != surfaceGrid.end(); it++ ) {
			if ( *it ) {
				SurfaceIndexes* srfIdxs = &surfaceIdxsGrid( it - surfaceGrid.begin() );
				surface->count = *it;
				ProcessMaterialSurface( surface, srfIdxs, processedMaterialSurfaces, vertices, idxs, &numIndices );
			}
		}

		surfaceGrid.Clear();

		surface = surface2;
		addedSurfaces = false;
	};

	for ( uint32_t surfaceIndex = 0; surfaceIndex < materialSurfaces.size(); surfaceIndex++ ) {
		surface2 = &materialSurfaces[surfaceIndex];
		if ( surface2->skyBrush ) {
			continue;
		}

		if ( materialSurfaceSort( *surface, *surface2 ) ) {
			processGrid();
		}

		for ( uint32_t i = 0; i < surface2->count; i += 3 ) {
			glIndex_t tri[3] = { indices[i + surface2->firstIndex], indices[i + 1 + surface2->firstIndex],
				indices[i + 2 + surface2->firstIndex] };

			srfVert_t triVerts[3] = { vertices[tri[0]], vertices[tri[1]], vertices[tri[2]] };

			vec3_t origin;
			VectorAdd( triVerts[0].xyz, triVerts[1].xyz, origin );
			VectorAdd( origin, triVerts[2].xyz, origin );

			VectorScale( origin, 1.0f / 3.0f, origin );
			VectorSubtract( origin, worldMins, origin );

			uint32_t gridOrigin[3]{ uint32_t( origin[0] / cellSize[0] ), uint32_t( origin[1] / cellSize[1] ),
				uint32_t( origin[2] / cellSize[2] ) };

			uint32_t* count = &surfaceGrid( gridOrigin[0], gridOrigin[1], gridOrigin[2] );
			SurfaceIndexes* srfIdxs = &surfaceIdxsGrid( gridOrigin[0], gridOrigin[1], gridOrigin[2] );

			memcpy( srfIdxs->idxs + ( *count ), tri, 3 * sizeof( glIndex_t ) );

			*count += 3;

			if ( *count + 3 >= MAX_MATERIAL_SURFACE_INDEXES ) {
				const uint32_t oldCount = surface2->count;
				surface2->count = *count;

				ProcessMaterialSurface( surface2, srfIdxs, processedMaterialSurfaces, vertices, idxs, &numIndices );

				surface2->count = oldCount;
				*count = 0;
			} else {
				addedSurfaces = true;
			}
		}
	}

	processGrid();
}

std::vector<MaterialSurface> OptimiseMapGeometryMaterial( world_t* world, bspSurface_t** rendererSurfaces, int numSurfaces,
	const srfVert_t* vertices, const int numVerticesIn, const glIndex_t* indices, const int numIndicesIn ) {
	std::vector<MaterialSurface> materialSurfaces;
	std::vector<MaterialSurface> materialSurfacesExtended;
	materialSurfaces.reserve( numSurfaces );
	materialSurfacesExtended.reserve( numSurfaces );

	materialSystem.buildOneShader = false;
	vec3_t worldBounds[2] = {};
	vec3_t worldBoundsExtended[2] = {};
	ClearBounds( worldBounds[0], worldBounds[1] );
	ClearBounds( worldBoundsExtended[0], worldBoundsExtended[1] );
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
		srf.portalNum = surface->portalNum;

		srf.firstIndex = ( ( srfGeneric_t* ) surface->data )->firstIndex;
		srf.count = ( ( srfGeneric_t* ) surface->data )->numTriangles * 3;
		srf.verts = ( ( srfGeneric_t* ) surface->data )->verts;
		srf.tris = ( ( srfGeneric_t* ) surface->data )->triangles;

		VectorCopy( ( ( srfGeneric_t* ) surface->data )->origin, srf.origin );
		srf.radius = ( ( srfGeneric_t* ) surface->data )->radius;

		materialSystem.GenerateMaterial( &srf );

		static const float MAX_NORMAL_SURFACE_DISTANCE = 65536.0f * sqrtf( 2.0f );
		if ( VectorLength( ( ( srfGeneric_t* ) surface->data )->bounds[0] ) > MAX_NORMAL_SURFACE_DISTANCE
			|| VectorLength( ( ( srfGeneric_t* ) surface->data )->bounds[1] ) > MAX_NORMAL_SURFACE_DISTANCE ) {
			BoundsAdd( worldBoundsExtended[0], worldBoundsExtended[1],
				( ( srfGeneric_t* ) surface->data )->bounds[0], ( ( srfGeneric_t* ) surface->data )->bounds[1] );

			materialSurfacesExtended.emplace_back( srf );
		} else {
			BoundsAdd( worldBounds[0], worldBounds[1],
				( ( srfGeneric_t* ) surface->data )->bounds[0], ( ( srfGeneric_t* ) surface->data )->bounds[1] );

			materialSurfaces.emplace_back( srf );
		}
	}

	vec3_t fullWorldBounds[2];
	VectorCopy( worldBounds[0], fullWorldBounds[0] );
	VectorCopy( worldBounds[1], fullWorldBounds[1] );
	BoundsAdd( fullWorldBounds[0], fullWorldBounds[1], worldBoundsExtended[0], worldBoundsExtended[1] );

	materialSystem.SetWorldBounds( fullWorldBounds );

	materialSystem.buildOneShader = true;

	/* GenerateWorldMaterialsBuffer() must be called before the surface merging loop, because it will compare the UBO offsets,
	which are set by this call */
	materialSystem.GenerateWorldMaterialsBuffer();

	std::vector<MaterialSurface> processedMaterialSurfaces;
	processedMaterialSurfaces.reserve( numSurfaces );

	std::function<bool( const MaterialSurface&, const MaterialSurface& )>
	materialSurfaceSort = []( const MaterialSurface& lhs, const MaterialSurface& rhs ) {
		if ( lhs.stages < rhs.stages ) {
			return true;
		} else if ( lhs.stages > rhs.stages ) {
			return false;
		}

		for ( uint8_t stage = 0; stage < lhs.stages; stage++ ) {
			if ( lhs.materialPackIDs[stage] < rhs.materialPackIDs[stage] ) {
				return true;
			} else if ( lhs.materialPackIDs[stage] > rhs.materialPackIDs[stage] ) {
				return false;
			}

			if ( lhs.materialIDs[stage] < rhs.materialIDs[stage] ) {
				return true;
			} else if ( lhs.materialIDs[stage] > rhs.materialIDs[stage] ) {
				return false;
			}

			shaderStage_t* pStage = lhs.shaderStages[stage];
			pStage = pStage->materialRemappedStage ? pStage->materialRemappedStage : pStage;

			const uint32_t surfaceMaterialID =
				pStage->materialOffset + pStage->variantOffsets[lhs.shaderVariant[stage]];

			pStage = rhs.shaderStages[stage];
			pStage = pStage->materialRemappedStage ? pStage->materialRemappedStage : pStage;

			const uint32_t surfaceMaterialID2 =
				pStage->materialOffset + pStage->variantOffsets[rhs.shaderVariant[stage]];

			if ( surfaceMaterialID < surfaceMaterialID2 ) {
				return true;
			} else if ( surfaceMaterialID > surfaceMaterialID2 ) {
				return false;
			}

			uint32_t texData = lhs.texDataDynamic[stage]
				? ( lhs.texDataIDs[stage] + materialSystem.GetTexDataSize() ) << TEX_BUNDLE_BITS
					: lhs.texDataIDs[stage] << TEX_BUNDLE_BITS;
				texData |= ( HasLightMap( &lhs ) ? GetLightMapNum( &lhs ) : 255 ) << LIGHTMAP_BITS;

			uint32_t texData2 = rhs.texDataDynamic[stage]
				? ( rhs.texDataIDs[stage] + materialSystem.GetTexDataSize() ) << TEX_BUNDLE_BITS
					: rhs.texDataIDs[stage] << TEX_BUNDLE_BITS;
				texData2 |= ( HasLightMap( &rhs ) ? GetLightMapNum( &rhs ) : 255 ) << LIGHTMAP_BITS;

			if ( texData < texData2 ) {
				return true;
			} else if ( texData > texData2 ) {
				return false;
			}
		}

		return lhs.firstIndex < rhs.firstIndex;
	};

	glIndex_t* idxs = ( glIndex_t* ) ri.Hunk_AllocateTempMemory( numIndicesIn * sizeof( glIndex_t ) );
	uint32_t numIndices = 0;

	uint32_t gridSize[3] {};
	uint32_t cellSize[3];
	vec3_t worldSize;
	VectorSubtract( worldBounds[1], worldBounds[0], worldSize );
	for ( int i = 0; i < 3; i++ ) {
		if ( worldSize[i] < 32768 ) {
			cellSize[i] = 1024;
		} else {
			cellSize[i] = 2048;
		}

		gridSize[i] = ( worldSize[i] + cellSize[i] - 1 ) / cellSize[i];
	}

	Log::Debug( "BSP material surface grid cell size: %u %u %u", cellSize[0], cellSize[1], cellSize[2] );

	std::sort( materialSurfaces.begin(), materialSurfaces.end(), materialSurfaceSort );

	OptimiseMaterialSurfaces( materialSurfaces, gridSize, cellSize, world->nodes[0].mins,
		materialSurfaceSort,
		vertices, indices, idxs, processedMaterialSurfaces, numIndices );

	/* On some maps surfaces go outside the 64k range, used for background and sky brushes.
	We process them separately so we can use a larger cell size there, otherwise we'll run out of memory.
	The regular surfaces can continue using samller grid sizes this way,
	resulting in actually useable bounding spheres */
	if( materialSurfacesExtended.size() ) {
		VectorSubtract( worldBoundsExtended[1], worldBoundsExtended[0], worldSize );
		for ( int i = 0; i < 3; i++ ) {
			cellSize[i] = std::max( 2048.0f, worldSize[i] / 32.0f );

			gridSize[i] = ( worldSize[i] + cellSize[i] - 1 ) / cellSize[i];
		}

		Log::Debug( "BSP material surface extended grid cell size: %u %u %u", cellSize[0], cellSize[1], cellSize[2] );

		std::sort( materialSurfacesExtended.begin(), materialSurfacesExtended.end(), materialSurfaceSort );

		OptimiseMaterialSurfaces( materialSurfacesExtended, gridSize, cellSize, world->nodes[0].mins,
			materialSurfaceSort,
			vertices, indices, idxs, processedMaterialSurfaces, numIndices );
	}

	materialSystem.GeneratePortalBoundingSpheres();
	materialSystem.GenerateWorldCommandBuffer( processedMaterialSurfaces );

	materialSystem.BindBuffers();

	vertexAttributeSpec_t attrs[] {
		{ ATTR_INDEX_POSITION, GL_FLOAT, GL_FLOAT, &vertices[0].xyz, 3, sizeof( *vertices ), 0 },
		{ ATTR_INDEX_COLOR, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, &vertices[0].lightColor, 4, sizeof( *vertices ), ATTR_OPTION_NORMALIZE },
		{ ATTR_INDEX_QTANGENT, GL_SHORT, GL_SHORT, &vertices[0].qtangent, 4, sizeof( *vertices ), ATTR_OPTION_NORMALIZE },
		{ ATTR_INDEX_TEXCOORD, GL_FLOAT, GL_HALF_FLOAT, &vertices[0].st, 4, sizeof( *vertices ), 0 },
	};

	geometryCache.AddMapGeometry( numVerticesIn, numIndices, std::begin( attrs ), std::end( attrs ), idxs );

	ri.Hunk_FreeTempMemory( idxs );

	return materialSurfaces;
}
