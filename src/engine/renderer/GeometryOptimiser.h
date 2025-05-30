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
// GeometryOptimiser.h

#include "tr_local.h"
#include "Material.h"

#ifndef GEOMETRY_OPTIMISER_H
#define GEOMETRY_OPTIMISER_H

static const uint32_t MAX_MATERIAL_SURFACE_TRIS = 64;
static const uint32_t MAX_MATERIAL_SURFACE_INDEXES = 3 * MAX_MATERIAL_SURFACE_TRIS;

struct MapVertHasher {
	size_t operator()( const srfVert_t& vert ) const {
		uint32_t hash = ~( Util::bit_cast<uint32_t, float>( vert.xyz[0] ) << 15 );
		hash ^= ( Util::bit_cast<uint32_t, float>( vert.xyz[0] ) >> 10 );
		hash += ( Util::bit_cast<uint32_t, float>( vert.xyz[1] ) << 3 );
		hash ^= ( Util::bit_cast<uint32_t, float>( vert.xyz[1] ) >> 6 );
		hash += ( Util::bit_cast<uint32_t, float>( vert.xyz[2] ) << 11 );
		hash ^= ( Util::bit_cast<uint32_t, float>( vert.xyz[2] ) >> 16 );

		return hash;
	}
};

static bool CompareEpsilon( float lhs, float rhs ) {
	float diff = fabsf( lhs - rhs );
	return diff <= 0.000001;
}

struct MapVertEqual {
	bool operator()( const srfVert_t& lhs, const srfVert_t& rhs ) const {
		return VectorCompare( lhs.xyz, rhs.xyz )
			&& CompareEpsilon( lhs.st[0], rhs.st[0] ) && CompareEpsilon( lhs.st[1], rhs.st[1] )
			&& CompareEpsilon( lhs.lightmap[0], rhs.lightmap[0] ) && CompareEpsilon( lhs.lightmap[1], rhs.lightmap[1] )
			&& VectorCompareEpsilon( lhs.normal, rhs.normal, 0.0001f )
			&& CompareEpsilon( lhs.qtangent[0], rhs.qtangent[0] ) && CompareEpsilon( lhs.qtangent[1], rhs.qtangent[1] )
			&& CompareEpsilon( lhs.qtangent[2], rhs.qtangent[2] ) && CompareEpsilon( lhs.qtangent[3], rhs.qtangent[3] )
			&& lhs.lightColor.Red() == rhs.lightColor.Red() && lhs.lightColor.Green() == rhs.lightColor.Green()
			&& lhs.lightColor.Blue() == rhs.lightColor.Blue() && lhs.lightColor.Alpha() == rhs.lightColor.Alpha();
	}
};

struct SurfaceIndexes {
	glIndex_t idxs[MAX_MATERIAL_SURFACE_INDEXES];
};

void MarkShaderBuildNONE( const shaderStage_t* );
void MarkShaderBuildNOP( const shaderStage_t* );
void MarkShaderBuildGeneric3D( const shaderStage_t* pStage );
void MarkShaderBuildLightMapping( const shaderStage_t* pStage );
void MarkShaderBuildReflection( const shaderStage_t* pStage );
void MarkShaderBuildSkybox( const shaderStage_t* pStage );
void MarkShaderBuildScreen( const shaderStage_t* pStage );
void MarkShaderBuildPortal( const shaderStage_t* pStage );
void MarkShaderBuildHeatHaze( const shaderStage_t* pStage );
void MarkShaderBuildLiquid( const shaderStage_t* pStage );
void MarkShaderBuildFog( const shaderStage_t* pStage );

void MarkShaderBuildIQM( const IQModel_t* model );
void MarkShaderBuildMDV( const mdvModel_t* model );
void MarkShaderBuildMD5( const md5Model_t* model );

void MarkShaderBuild( shader_t* shader, const int lightMapNum, const bool bspSurface,
	const bool vertexSkinning, const bool vertexAnimation );

void OptimiseMapGeometryCore( world_t* world, bspSurface_t** rendererSurfaces, int numSurfaces );
void MergeLeafSurfacesCore( world_t* world, bspSurface_t** rendererSurfaces, int numSurfaces );
void MergeDuplicateVertices( bspSurface_t** rendererSurfaces, int numSurfaces, srfVert_t* vertices, int numVerticesIn,
	glIndex_t* indices, int numIndicesIn, int& numVerticesOut, int& numIndicesOut );
std::vector<MaterialSurface> OptimiseMapGeometryMaterial( world_t* world, bspSurface_t** rendererSurfaces, int numSurfaces,
	const srfVert_t* vertices, const int numVerticesIn, const glIndex_t* indices, const int numIndicesIn );

#endif // GEOMETRY_OPTIMISER_H
