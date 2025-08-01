/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2010 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_models.c -- model loading and caching
#include "tr_local.h"
#include "GLUtils.h"

bool R_AddTriangleToVBOTriangleList(
	const skelTriangle_t *tri, int *numBoneReferences, int boneReferences[ MAX_BONES ] )
{
	md5Vertex_t *v;
	int         boneIndex;
	int         numNewReferences;
	int         newReferences[ MAX_WEIGHTS * 3 ]; // a single triangle can have up to 12 new bone references !
	bool    hasWeights;

	hasWeights = false;

	numNewReferences = 0;
	memset( newReferences, -1, sizeof( newReferences ) );

	for (unsigned i = 0; i < 3; i++ )
	{
		v = tri->vertexes[ i ];

		// can the bones be referenced?
		for (unsigned j = 0; j < MAX_WEIGHTS; j++ )
		{
			if ( j < v->numWeights )
			{
				boneIndex = v->boneIndexes[ j ];
				hasWeights = true;

				// is the bone already referenced?
				if ( !boneReferences[ boneIndex ] )
				{
					// the bone isn't yet and we have to test if we can give the mesh this bone at all
					if ( ( *numBoneReferences + numNewReferences ) >= glConfig2.maxVertexSkinningBones )
					{
						return false;
					}
					else
					{
                        unsigned k;
						for ( k = 0; k < ( MAX_WEIGHTS * 3 ); k++ )
						{
							if ( newReferences[ k ] == boneIndex )
							{
								break;
							}
						}

						if ( k == ( MAX_WEIGHTS * 3 ) )
						{
							newReferences[ numNewReferences ] = boneIndex;
							numNewReferences++;
						}
					}
				}
			}
		}
	}

	// reference them!
	for (int j = 0; j < numNewReferences; j++ )
	{
		boneIndex = newReferences[ j ];

		boneReferences[ boneIndex ]++;

		*numBoneReferences = *numBoneReferences + 1;
	}

	return hasWeights;
}

// index has to be in range 0-255, weight has to be >= 0 and <= 1
static unsigned short boneFactor( int index, float weight ) {
	int scaledWeight = lrintf( weight * 255.0F );
	return (unsigned short)( ( scaledWeight << 8 ) | index );
}

srfVBOMD5Mesh_t *R_GenerateMD5VBOSurface(
	Str::StringRef surfName, const std::vector<skelTriangle_t> &vboTriangles,
	md5Model_t *md5, md5Surface_t *surf, int skinIndex, int boneReferences[ MAX_BONES ] )
{
	int             j;

	int             vertexesNum;

	int             indexesNum;
	glIndex_t       *indexes;

	srfVBOMD5Mesh_t *vboSurf;

	vertexesNum = surf->numVerts;
	indexesNum = vboTriangles.size() * 3;

	// create surface
	vboSurf = (srfVBOMD5Mesh_t*) ri.Hunk_Alloc( sizeof( *vboSurf ), ha_pref::h_low );

	vboSurf->surfaceType = surfaceType_t::SF_VBO_MD5MESH;
	vboSurf->md5Model = md5;
	vboSurf->shader = R_GetShaderByHandle( surf->shaderIndex );
	vboSurf->skinIndex = skinIndex;
	vboSurf->numIndexes = indexesNum;
	vboSurf->numVerts = vertexesNum;

	i16vec4_t *qtangents = ( i16vec4_t * ) ri.Hunk_AllocateTempMemory( sizeof( i16vec4_t ) * vertexesNum );
	u16vec4_t *boneFactors = (u16vec4_t*)ri.Hunk_AllocateTempMemory( sizeof( u16vec4_t ) * vertexesNum );
	indexes = ( glIndex_t * ) ri.Hunk_AllocateTempMemory( indexesNum * sizeof( glIndex_t ) );

	vboSurf->numBoneRemap = 0;
	memset( vboSurf->boneRemap, 0, sizeof( vboSurf->boneRemap ) );
	memset( vboSurf->boneRemapInverse, 0, sizeof( vboSurf->boneRemapInverse ) );

	for ( j = 0; j < MAX_BONES; j++ )
	{
		if ( boneReferences[ j ] > 0 )
		{
			vboSurf->boneRemap[ j ] = vboSurf->numBoneRemap;
			vboSurf->boneRemapInverse[ vboSurf->numBoneRemap ] = j;

			vboSurf->numBoneRemap++;
		}
	}

	glIndex_t *indexesOut = indexes;

	for ( const skelTriangle_t &tri : vboTriangles )
	{
		for (unsigned k = 0; k < 3; k++ )
		{
			*indexesOut++ = tri.indexes[ k ];
		}
	}

	for ( j = 0; j < vertexesNum; j++ )
	{
		R_TBNtoQtangents( surf->verts[ j ].tangent, surf->verts[ j ].binormal,
		                  surf->verts[ j ].normal, qtangents[ j ] );

		for (unsigned k = 0; k < MAX_WEIGHTS; k++ )
		{
			if ( k < surf->verts[ j ].numWeights )
			{
				uint16_t boneIndex = vboSurf->boneRemap[ surf->verts[ j ].boneIndexes[ k ] ];
				boneFactors[ j ][ k ] = boneFactor( boneIndex, surf->verts[ j ].boneWeights[ k ] );
			}
			else
			{
				boneFactors[ j ][ k ] = 0;
			}
		}
	}

	// MD5 does not have color, but shaders always require the color vertex attribute, so we have
	// to provide this 0 color.
	// TODO: optimize a vertexAttributeSpec_t with 0 stride to use a non-array vertex attribute?
	// (although that would mess up the nice 32-bit size)
	const byte dummyColor[ 4 ]{};

	vertexAttributeSpec_t attributes[] {
		{ ATTR_INDEX_BONE_FACTORS, GL_UNSIGNED_SHORT, GL_UNSIGNED_SHORT, boneFactors, 4, sizeof(u16vec4_t), 0 },
		{ ATTR_INDEX_POSITION, GL_FLOAT, GL_SHORT, &surf->verts[ 0 ].position, 3, sizeof(md5Vertex_t), ATTR_OPTION_NORMALIZE },
		{ ATTR_INDEX_QTANGENT, GL_SHORT, GL_SHORT, qtangents, 4, sizeof(i16vec4_t), ATTR_OPTION_NORMALIZE },
		{ ATTR_INDEX_TEXCOORD, GL_FLOAT, GL_HALF_FLOAT, &surf->verts[ 0 ].texCoords, 2, sizeof(md5Vertex_t), 0 },
		{ ATTR_INDEX_COLOR, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, dummyColor, 4, 0, ATTR_OPTION_NORMALIZE },
	};

	vboSurf->vbo = R_CreateStaticVBO( "MD5 surface VBO " + surfName,
	                                  std::begin( attributes ), std::end( attributes ), vertexesNum );

	vboSurf->ibo = R_CreateStaticIBO( ( "MD5 surface IBO " + surfName ).c_str(), indexes, indexesNum );

	ri.Hunk_FreeTempMemory( indexes );
	ri.Hunk_FreeTempMemory( boneFactors );
	ri.Hunk_FreeTempMemory( qtangents );

	return vboSurf;
}
