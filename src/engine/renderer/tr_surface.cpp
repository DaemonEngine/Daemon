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
// tr_surface.c
#include "tr_local.h"
#include "gl_shader.h"
#include "Material.h"

/*
==============================================================================
THIS ENTIRE FILE IS BACK END!

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
==============================================================================
*/

static transform_t bones[ MAX_BONES ];

/*
==============
Tess_EndBegin

Flush the buffered polygons and prepare to receive more with the same parameters
==============
*/
void Tess_EndBegin()
{
	Tess_End();
	Tess_Begin( tess.stageIteratorFunc, tess.surfaceShader, tess.skipTangents,
	            tess.lightmapNum, tess.fogNum, tess.bspSurface );
}

/*
==============
Tess_CheckVBOAndIBO

Bind the buffers and flush any data that came from a different buffer.
For a multidraw-using static VBO surface, it performs an analogous function to Tess_CheckOverflow,
with the assumption that only one multidraw primitive will be used.
==============
*/
static void Tess_CheckVBOAndIBO( VBO_t *vbo, IBO_t *ibo )
{
	if ( glState.currentVBO != vbo || glState.currentIBO != ibo || tess.multiDrawPrimitives >= MAX_MULTIDRAW_PRIMITIVES )
	{
		Tess_EndBegin();

		R_BindVBO( vbo );
		R_BindIBO( ibo );
	}
}

/*
==============
Tess_CheckOverflow

Used for non-VBO surfaces. Check that tess.verts and tess.indexes have sufficient room for the
data that is about to be written and ensure that the default VBO and IBO are bound. If there
is old data from a different surface or if there is too much data already, flush it.
==============
*/
void Tess_CheckOverflow( int verts, int indexes )
{
	// FIXME: need to check if a vbo is bound, otherwise we fail on startup
	if ( glState.currentVBO != nullptr && glState.currentIBO != nullptr )
	{
		Tess_CheckVBOAndIBO( tess.vbo, tess.ibo );
	}

	if ( tess.numVertexes + verts < SHADER_MAX_VERTEXES && tess.numIndexes + indexes < SHADER_MAX_INDEXES )
	{
		return;
	}

	GLIMP_LOGCOMMENT( "--- Tess_CheckOverflow(%i + %i vertices, %i + %i triangles ) ---",
		tess.numVertexes, verts,( tess.numIndexes / 3 ), indexes );

	Tess_End();

	if ( verts >= SHADER_MAX_VERTEXES )
	{
		Sys::Drop( "Tess_CheckOverflow: verts > max (%d > %d)", verts, SHADER_MAX_VERTEXES );
	}

	if ( indexes >= SHADER_MAX_INDEXES )
	{
		Sys::Drop( "Tess_CheckOverflow: indexes > max (%d > %d)", indexes, SHADER_MAX_INDEXES );
	}

	Tess_Begin( tess.stageIteratorFunc, tess.surfaceShader, tess.skipTangents,
	            tess.lightmapNum, tess.fogNum, tess.bspSurface );
}

/*
==============
Tess_SurfaceVertsAndTris

Defines ATTR_POSITION, ATTR_TEXCOORD, ATTR_COLOR, ATTR_QTANGENT
==============
*/
static void Tess_SurfaceVertsAndTris( const srfVert_t *verts, const srfTriangle_t *triangles, int numVerts, int numTriangles )
{
	int i;
	const srfTriangle_t *tri = triangles;
	const srfVert_t *vert = verts;
	const int numIndexes = numTriangles * 3;

	Tess_CheckOverflow( numVerts, numIndexes );

	for ( i = 0; i < numIndexes; i+=3, tri++ )
	{
		tess.indexes[ tess.numIndexes + i + 0 ] = tess.numVertexes + tri->indexes[ 0 ];
		tess.indexes[ tess.numIndexes + i + 1 ] = tess.numVertexes + tri->indexes[ 1 ];
		tess.indexes[ tess.numIndexes + i + 2 ] = tess.numVertexes + tri->indexes[ 2 ];
	}

	tess.numIndexes += numIndexes;

	for ( i = 0; i < numVerts; i++, vert++ )
	{
		VectorCopy( vert->xyz, tess.verts[ tess.numVertexes + i ].xyz );
		Vector4Copy( vert->qtangent, tess.verts[ tess.numVertexes + i ].qtangents );

		tess.verts[ tess.numVertexes + i ].texCoords[ 0 ] = vert->st[ 0 ];
		tess.verts[ tess.numVertexes + i ].texCoords[ 1 ] = vert->st[ 1 ];

		tess.verts[ tess.numVertexes + i ].texCoords[ 2 ] = vert->lightmap[ 0 ];
		tess.verts[ tess.numVertexes + i ].texCoords[ 3 ] = vert->lightmap[ 1 ];

		tess.verts[ tess.numVertexes + i ].color = vert->lightColor;
	}

	tess.numVertexes += numVerts;
}

static bool Tess_SurfaceVBO( VBO_t *vbo, IBO_t *ibo, int numIndexes, int firstIndex )
{
	if ( ( !vbo && !tr.skipVBO ) || !ibo )
	{
		return false;
	}

	Tess_CheckVBOAndIBO( vbo, ibo );

	//lazy merge multidraws together
	bool mergeBack = false;
	bool mergeFront = false;

	glIndex_t *firstIndexOffset = ( glIndex_t* ) BUFFER_OFFSET( firstIndex * sizeof( glIndex_t ) );

	if ( tess.multiDrawPrimitives > 0 )
	{
		int lastPrimitive = tess.multiDrawPrimitives - 1;
		glIndex_t *lastIndexOffset = firstIndexOffset + numIndexes;
		glIndex_t *prevLastIndexOffset = tess.multiDrawIndexes[ lastPrimitive ] + tess.multiDrawCounts[ lastPrimitive ];

		if ( firstIndexOffset == prevLastIndexOffset )
		{
			mergeFront = true;
		}
		else if ( lastIndexOffset == tess.multiDrawIndexes[ lastPrimitive ] )
		{
			mergeBack = true;
		}
	}

	if ( mergeFront )
	{
		tess.multiDrawCounts[ tess.multiDrawPrimitives - 1 ] += numIndexes;
	}
	else if ( mergeBack )
	{
		tess.multiDrawIndexes[ tess.multiDrawPrimitives - 1 ] = firstIndexOffset;
		tess.multiDrawOffsets[ tess.multiDrawPrimitives - 1 ] = (GLuint) firstIndex;
		tess.multiDrawCounts[ tess.multiDrawPrimitives - 1 ] += numIndexes;
	}
	else
	{
		tess.multiDrawIndexes[ tess.multiDrawPrimitives ] = firstIndexOffset;
		tess.multiDrawOffsets[ tess.multiDrawPrimitives ] = (GLuint) firstIndex;
		tess.multiDrawCounts[ tess.multiDrawPrimitives ] = numIndexes;

		tess.multiDrawPrimitives++;
	}

	return true;
}

/*
==============
Tess_AddQuadStampExt

Defines ATTR_POSITION, ATTR_QTANGENT, ATTR_COLOR, ATTR_TEXCOORD
==============
*/
void Tess_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, const Color::Color& color, float s1, float t1, float s2, float t2 )
{
	int    i;
	vec3_t normal;
	int    ndx;

	GLIMP_LOGCOMMENT( "--- Tess_AddQuadStampExt ---" );

	Tess_CheckOverflow( 4, 6 );

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes ] = ndx;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

	tess.verts[ ndx ].xyz[ 0 ] = origin[ 0 ] + left[ 0 ] + up[ 0 ];
	tess.verts[ ndx ].xyz[ 1 ] = origin[ 1 ] + left[ 1 ] + up[ 1 ];
	tess.verts[ ndx ].xyz[ 2 ] = origin[ 2 ] + left[ 2 ] + up[ 2 ];

	tess.verts[ ndx + 1 ].xyz[ 0 ] = origin[ 0 ] - left[ 0 ] + up[ 0 ];
	tess.verts[ ndx + 1 ].xyz[ 1 ] = origin[ 1 ] - left[ 1 ] + up[ 1 ];
	tess.verts[ ndx + 1 ].xyz[ 2 ] = origin[ 2 ] - left[ 2 ] + up[ 2 ];

	tess.verts[ ndx + 2 ].xyz[ 0 ] = origin[ 0 ] - left[ 0 ] - up[ 0 ];
	tess.verts[ ndx + 2 ].xyz[ 1 ] = origin[ 1 ] - left[ 1 ] - up[ 1 ];
	tess.verts[ ndx + 2 ].xyz[ 2 ] = origin[ 2 ] - left[ 2 ] - up[ 2 ];

	tess.verts[ ndx + 3 ].xyz[ 0 ] = origin[ 0 ] + left[ 0 ] - up[ 0 ];
	tess.verts[ ndx + 3 ].xyz[ 1 ] = origin[ 1 ] + left[ 1 ] - up[ 1 ];
	tess.verts[ ndx + 3 ].xyz[ 2 ] = origin[ 2 ] + left[ 2 ] - up[ 2 ];

	// constant normal all the way around
	VectorSubtract( vec3_origin, backEnd.viewParms.orientation.axis[ 0 ], normal );

	i16vec4_t qtangents;
	R_TBNtoQtangents( left, up, normal, qtangents );

	Vector4Copy( qtangents, tess.verts[ ndx     ].qtangents );
	Vector4Copy( qtangents, tess.verts[ ndx + 1 ].qtangents );
	Vector4Copy( qtangents, tess.verts[ ndx + 2 ].qtangents );
	Vector4Copy( qtangents, tess.verts[ ndx + 3 ].qtangents );

	// standard square texture coordinates
	tess.verts[ ndx ].texCoords[ 0 ] = s1;
	tess.verts[ ndx ].texCoords[ 1 ] = t1;

	tess.verts[ ndx + 1 ].texCoords[ 0 ] = s2;
	tess.verts[ ndx + 1 ].texCoords[ 1 ] = t1;

	tess.verts[ ndx + 2 ].texCoords[ 0 ] = s2;
	tess.verts[ ndx + 2 ].texCoords[ 1 ] = t2;

	tess.verts[ ndx + 3 ].texCoords[ 0 ] = s1;
	tess.verts[ ndx + 3 ].texCoords[ 1 ] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?

	Color::Color32Bit iColor = color;

	for ( i = 0; i < 4; i++ )
	{
		tess.verts[ ndx + i ].color = iColor;
	}

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

/*
==============
Tess_AddQuadStamp
==============
*/
void Tess_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, const Color::Color& color )
{
	Tess_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}

/*
==============
Tess_AddQuadStampExt2

Defines ATTR_POSITION, ATTR_COLOR, ATTR_TEXCOORD, ATTR_QTANGENT
==============
*/
void Tess_AddQuadStampExt2( vec4_t quadVerts[ 4 ], const Color::Color& color, float s1, float t1, float s2, float t2 )
{
	int    i;
	vec3_t normal, tangent, binormal;
	int    ndx;

	GLIMP_LOGCOMMENT( "--- Tess_AddQuadStampExt2 ---" );

	Tess_CheckOverflow( 4, 6 );

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes ] = ndx;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

	VectorCopy( quadVerts[ 0 ], tess.verts[ ndx + 0 ].xyz );
	VectorCopy( quadVerts[ 1 ], tess.verts[ ndx + 1 ].xyz );
	VectorCopy( quadVerts[ 2 ], tess.verts[ ndx + 2 ].xyz );
	VectorCopy( quadVerts[ 3 ], tess.verts[ ndx + 3 ].xyz );

	// constant normal all the way around
	vec2_t st[ 3 ] = { { s1, t1 }, { s2, t1 }, { s2, t2 } };
	R_CalcFaceNormal( normal, quadVerts[ 0 ], quadVerts[ 1 ], quadVerts[ 2 ] );
	R_CalcTangents( tangent, binormal,
			quadVerts[ 0 ], quadVerts[ 1 ], quadVerts[ 2 ],
			st[ 0 ], st[ 1 ], st[ 2 ] );
	//if ( !calcNormals )
	//{
	//	VectorNegate( backEnd.viewParms.orientation.axis[ 0 ], normal );
	//}

	i16vec4_t qtangents;

	R_TBNtoQtangents( tangent, binormal, normal, qtangents );

	Vector4Copy( qtangents, tess.verts[ ndx     ].qtangents );
	Vector4Copy( qtangents, tess.verts[ ndx + 1 ].qtangents );
	Vector4Copy( qtangents, tess.verts[ ndx + 2 ].qtangents );
	Vector4Copy( qtangents, tess.verts[ ndx + 3 ].qtangents );

	// standard square texture coordinates
	tess.verts[ ndx ].texCoords[ 0 ] = s1;
	tess.verts[ ndx ].texCoords[ 1 ] = t1;

	tess.verts[ ndx + 1 ].texCoords[ 0 ] = s2;
	tess.verts[ ndx + 1 ].texCoords[ 1 ] = t1;

	tess.verts[ ndx + 2 ].texCoords[ 0 ] = s2;
	tess.verts[ ndx + 2 ].texCoords[ 1 ] = t2;

	tess.verts[ ndx + 3 ].texCoords[ 0 ] = s1;
	tess.verts[ ndx + 3 ].texCoords[ 1 ] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?

	Color::Color32Bit iColor = color;
	for ( i = 0; i < 4; i++ )
	{
		tess.verts[ ndx + i ].color = iColor;
	}

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

/*
==============
Tess_AddQuadStamp2
==============
*/
void Tess_AddQuadStamp2( vec4_t quadVerts[ 4 ], const Color::Color& color )
{
	Tess_AddQuadStampExt2( quadVerts, color, 0, 0, 1, 1 );
}

void Tess_AddQuadStamp2WithNormals( vec4_t quadVerts[ 4 ], const Color::Color& color )
{
	Tess_AddQuadStampExt2( quadVerts, color, 0, 0, 1, 1 );
}

// Defines ATTR_POSITION, ATTR_COLOR
void Tess_AddTetrahedron( vec4_t tetraVerts[ 4 ], const Color::Color& colorf )
{
	int k;

	Tess_CheckOverflow( 12, 12 );

	Color::Color32Bit color = colorf;

	// ground triangle
	for ( k = 0; k < 3; k++ )
	{
		VectorCopy( tetraVerts[ k ], tess.verts[ tess.numVertexes ].xyz );
		tess.verts[ tess.numVertexes ].color = color;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;
	}

	// side triangles
	for ( k = 0; k < 3; k++ )
	{
		VectorCopy( tetraVerts[ 3 ], tess.verts[ tess.numVertexes ].xyz );  // offset
		tess.verts[ tess.numVertexes ].color = color;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;

		VectorCopy( tetraVerts[ k ], tess.verts[ tess.numVertexes ].xyz );
		tess.verts[ tess.numVertexes ].color = color;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;

		VectorCopy( tetraVerts[( k + 1 ) % 3 ], tess.verts[ tess.numVertexes ].xyz );
		tess.verts[ tess.numVertexes ].color = color;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;
	}
}

void Tess_AddCube( const vec3_t position, const vec3_t minSize, const vec3_t maxSize, const Color::Color& color )
{
	vec4_t quadVerts[ 4 ];
	vec3_t mins;
	vec3_t maxs;

	VectorAdd( position, minSize, mins );
	VectorAdd( position, maxSize, maxs );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );
}

void Tess_AddCubeWithNormals( const vec3_t position, const vec3_t minSize, const vec3_t maxSize, const Color::Color& color )
{
	vec4_t quadVerts[ 4 ];
	vec3_t mins;
	vec3_t maxs;

	VectorAdd( position, minSize, mins );
	VectorAdd( position, maxSize, maxs );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );
}

void Tess_InstantScreenSpaceQuad() {
	GLIMP_LOGCOMMENT( "--- Tess_InstantScreenSpaceQuad ---" );

	if ( glConfig.gpuShader4Available )
	{
		tr.skipVBO = true;
		Tess_Begin( Tess_StageIteratorDummy, nullptr, true, -1, 0 );
		rb_surfaceTable[Util::ordinal( *( tr.genericTriangle->surface ) )]( tr.genericTriangle->surface );
		Tess_DrawElements();
		tr.skipVBO = false;
	}
	else
	{
		Tess_Begin( Tess_StageIteratorDummy, nullptr, true, -1, 0 );
		rb_surfaceTable[Util::ordinal( *( tr.genericQuad->surface ) )]( tr.genericQuad->surface );
		GL_VertexAttribsState( ATTR_POSITION );
		Tess_DrawElements();
	}

	GL_CheckErrors();

	Tess_Clear();
}

void Tess_InstantQuad( u_ModelViewProjectionMatrix &shader, const float x, const float y, const float width, const float height )
{
	GLIMP_LOGCOMMENT( "--- Tess_InstantQuad ---" );

	Tess_Begin( Tess_StageIteratorDummy, nullptr, true, -1, 0 );

	/* We don't use x, y, width, height directly to make it compatible
	with R_InitGenericVBOs() in tr_vbo.cpp.
	See: https://github.com/DaemonEngine/Daemon/pull/1739 */
	matrix_t modelViewMatrix;
	MatrixCopy( matrixIdentity, modelViewMatrix );
	modelViewMatrix[12] = 0.5f * width + x;
	modelViewMatrix[13] = 0.5f * height + y;
	modelViewMatrix[0] = 0.5f * width;
	modelViewMatrix[5] = 0.5f * height;
	GL_LoadModelViewMatrix( modelViewMatrix );
	shader.SetUniform_ModelViewProjectionMatrix(
		glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	rb_surfaceTable[Util::ordinal( *( tr.genericQuad->surface ) )]( tr.genericQuad->surface );
	GL_VertexAttribsState( ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR );

	Tess_DrawElements();

	GL_CheckErrors();

	Tess_Clear();
}

/*
==============
Tess_SurfaceSprite
==============
*/
static const float NORMAL_EPSILON = 0.0001;

static void Tess_SurfaceSprite()
{
	vec3_t delta, left, up;
	float  radius;

	GLIMP_LOGCOMMENT( "--- Tess_SurfaceSprite ---" );

	radius = backEnd.currentEntity->e.radius;

	if ( tess.surfaceShader->autoSpriteMode != 0 && !tess.surfaceShader->autoSpriteWarned )
	{
		// This function does similarly to autosprite mode 1. Autospriting it again would be a
		// waste and would probably lose the rotation angle
		Log::Warn( "RT_SPRITE entity should NOT configure its shader (%s) as autosprite",
		           tess.surfaceShader->name );
		tess.surfaceShader->autoSpriteWarned = true;
	}

	VectorSubtract( backEnd.currentEntity->e.origin, backEnd.viewParms.orientation.origin, delta );

	if( VectorNormalize( delta ) < NORMAL_EPSILON )
		return;

	vec3_t forward;
	if ( tess.surfaceShader->entitySpriteFaceViewDirection )
	{
		// Face opposite to view direction, triggered by RSF_SPRITE.
		// Good for particles that may appear very close to the viewer and thus have extreme
		// difference between the view direction and the direction to the viewer, so
		// as to avoid cases where they appear obviously planar
		VectorCopy( backEnd.viewParms.orientation.axis[ 0 ], forward );
	}
	else
	{
		// Face toward viewer. Used by light flares
		VectorCopy( delta, forward );
	}

	CrossProduct( backEnd.viewParms.orientation.axis[ 2 ], forward, left );

	if( VectorNormalize( left ) < NORMAL_EPSILON )
		VectorSet( left, 1, 0, 0 );

	if( backEnd.currentEntity->e.rotation != 0 )
		RotatePointAroundVector( left, forward, left, backEnd.currentEntity->e.rotation );

	CrossProduct( forward, left, up );

	VectorScale( left, radius, left );
	VectorScale( up, radius, up );

	if ( backEnd.viewParms.isMirror )
		VectorSubtract( vec3_origin, left, left );

	Color::Color32Bit color = backEnd.currentEntity->e.shaderRGBA;
	color = tr.convertColorFromSRGB( color );
	Tess_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, color );
}

/*
=============
Tess_SurfacePolychain

Defines ATTR_POSITION, ATTR_TEXCOORD, ATTR_COLOR, and maybe ATTR_QTANGENT
=============
*/
static void Tess_SurfacePolychain( srfPoly_t *p )
{
	int i;

	GLIMP_LOGCOMMENT( "--- Tess_SurfacePolychain ---" );

	int numVertexes = p->numVerts;
	int numIndexes = 3 * (p->numVerts - 2);

	Tess_CheckOverflow( numVertexes, numIndexes );

	if ( tess.skipTangents )
	{
		// fan triangles into the tess array

		for (i = 0; i < p->numVerts; i++)
		{
			VectorCopy(p->verts[i].xyz, tess.verts[tess.numVertexes + i].xyz);

			Color::Color32Bit color = Color::Adapt( p->verts[ i ].modulate );
			color = tr.convertColorFromSRGB( color );
			tess.verts[tess.numVertexes + i].color = color;

			tess.verts[tess.numVertexes + i].texCoords[0] = p->verts[i].st[0];
			tess.verts[tess.numVertexes + i].texCoords[1] = p->verts[i].st[1];
		}

		// generate fan indexes into the tess array

		for (i = 0; i < p->numVerts - 2; i++)
		{
			tess.indexes[tess.numIndexes + i * 3 + 0] = tess.numVertexes;
			tess.indexes[tess.numIndexes + i * 3 + 1] = tess.numVertexes + i + 1;
			tess.indexes[tess.numIndexes + i * 3 + 2] = tess.numVertexes + i + 2;
		}
	}
	else
	{
		vec3_t      tangent, *tangents;
		vec3_t      binormal, *binormals;
		vec3_t      normal, *normals;

		tangents = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );
		binormals = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );
		normals = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );

		for ( i = 0; i < numVertexes; i++ )
		{
			VectorClear( tangents[ i ] );
			VectorClear( binormals[ i ] );
			VectorClear( normals[ i ] );
		}

		// generate fan indexes into the tess array and generate tangent vectors for the triangles
		for (i = 0; i < p->numVerts - 2; i++ )
		{
			polyVert_t* v0 = &p->verts[0];
			polyVert_t* v1 = &p->verts[i + 1];
			polyVert_t* v2 = &p->verts[i + 2];

			R_CalcFaceNormal( normal, v0->xyz, v1->xyz, v2->xyz );
			R_CalcTangents( tangent, binormal, v0->xyz, v1->xyz, v2->xyz, v0->st, v1->st, v2->st );

			VectorAdd(tangents[0], tangent, tangents[0]);
			VectorAdd(normals[0], normal, normals[0]);
			VectorAdd(binormals[0], binormal, binormals[0]);

			VectorAdd(tangents[i + 1], tangent, tangents[i + 1]);
			VectorAdd(binormals[i + 1], binormal, binormals[i + 1]);
			VectorAdd(normals[i + 1], normal, normals[i + 1]);

			VectorAdd(tangents[i + 2], tangent, tangents[i + 2]);
			VectorAdd(binormals[i + 2], binormal, binormals[i + 2]);
			VectorAdd(normals[i + 2], normal, normals[i + 2]);

			tess.indexes[tess.numIndexes + i * 3 + 0] = tess.numVertexes;
			tess.indexes[tess.numIndexes + i * 3 + 1] = tess.numVertexes + i + 1;
			tess.indexes[tess.numIndexes + i * 3 + 2] = tess.numVertexes + i + 2;
		}

		// generate qtangents
		for ( i = 0; i < numVertexes; i++ )
		{
			i16vec4_t qtangents;

			VectorNormalizeFast(normals[i]);
			R_TBNtoQtangentsFast(tangents[i], binormals[i], normals[i], qtangents);

			VectorCopy(p->verts[i].xyz, tess.verts[tess.numVertexes + i].xyz);

			Color::Color32Bit color =  Color::Adapt( p->verts[ i ].modulate );
			color = tr.convertColorFromSRGB( color );
			tess.verts[tess.numVertexes + i].color = color;

			Vector4Copy(qtangents, tess.verts[tess.numVertexes + i].qtangents);
			tess.verts[tess.numVertexes + i].texCoords[0] = p->verts[i].st[0];
			tess.verts[tess.numVertexes + i].texCoords[1] = p->verts[i].st[1];
		}

		ri.Hunk_FreeTempMemory( normals );
		ri.Hunk_FreeTempMemory( binormals );
		ri.Hunk_FreeTempMemory( tangents );
	}

	tess.numIndexes += numIndexes;
	tess.numVertexes += numVertexes;
}

/*
==============
Tess_SurfaceFace
==============
*/
static void Tess_SurfaceFace( srfGeneric_t* srf )
{
	GLIMP_LOGCOMMENT( "--- Tess_SurfaceFace ---" );

	if ( !r_vboFaces->integer || !Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numTriangles * 3, srf->firstIndex ) )
	{
		Tess_SurfaceVertsAndTris( srf->verts, srf->triangles, srf->numVerts, srf->numTriangles );
	}
}

/*
=============
Tess_SurfaceGrid
=============
*/
static void Tess_SurfaceGrid( srfGridMesh_t *srf )
{
	GLIMP_LOGCOMMENT( "--- Tess_SurfaceGrid ---" );

	if ( !r_vboCurves->integer || !Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numTriangles * 3, srf->firstIndex ) )
	{
		Tess_SurfaceVertsAndTris( srf->verts, srf->triangles, srf->numVerts, srf->numTriangles );
	}
}

/*
=============
Tess_SurfaceTriangles
=============
*/
static void Tess_SurfaceTriangles( srfGeneric_t* srf )
{
	GLIMP_LOGCOMMENT( "--- Tess_SurfaceTriangles ---" );

	if ( !r_vboTriangles->integer || !Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numTriangles * 3, srf->firstIndex ) )
	{
		Tess_SurfaceVertsAndTris( srf->verts, srf->triangles, srf->numVerts, srf->numTriangles );
	}
}

//================================================================================

/*
=============
Tess_SurfaceMDV

Defines ATTR_POSITION, ATTR_TEXCOORD, and maybe ATTR_QTANGENT
=============
*/
static void Tess_SurfaceMDV( mdvSurface_t *srf )
{
	int           i, j;
	int           numIndexes = 0;
	int           numVertexes;
	mdvXyz_t      *oldVert, *newVert;
	mdvNormal_t   *oldNormal, *newNormal;
	mdvSt_t       *st;
	srfTriangle_t *tri;
	float         backlerp;
	float         oldXyzScale, newXyzScale;

	GLIMP_LOGCOMMENT( "--- Tess_SurfaceMDV ---" );

	if ( backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame )
	{
		backlerp = 0;
	}
	else
	{
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	newXyzScale = ( 1.0f - backlerp );
	oldXyzScale = backlerp;

	Tess_CheckOverflow( srf->numVerts, srf->numTriangles * 3 );

	numIndexes = srf->numTriangles * 3;

	for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
	{
		tess.indexes[ tess.numIndexes + i * 3 + 0 ] = tess.numVertexes + tri->indexes[ 0 ];
		tess.indexes[ tess.numIndexes + i * 3 + 1 ] = tess.numVertexes + tri->indexes[ 1 ];
		tess.indexes[ tess.numIndexes + i * 3 + 2 ] = tess.numVertexes + tri->indexes[ 2 ];
	}

	newVert = srf->verts + ( backEnd.currentEntity->e.frame * srf->numVerts );
	oldVert = srf->verts + ( backEnd.currentEntity->e.oldframe * srf->numVerts );
	newNormal = srf->normals + ( backEnd.currentEntity->e.frame * srf->numVerts );
	oldNormal = srf->normals + ( backEnd.currentEntity->e.oldframe * srf->numVerts );
	st = srf->st;

	numVertexes = srf->numVerts;

	if (tess.skipTangents)
	{
		for (j = 0; j < numVertexes; j++, newVert++, oldVert++, st++)
		{
			vec3_t tmpVert;

			if (backlerp == 0)
			{
				// just copy
				VectorCopy(newVert->xyz, tmpVert);
			}
			else
			{
				// interpolate the xyz
				VectorScale(oldVert->xyz, oldXyzScale, tmpVert);
				VectorMA(tmpVert, newXyzScale, newVert->xyz, tmpVert);
			}

			tess.verts[tess.numVertexes + j].xyz[0] = tmpVert[0];
			tess.verts[tess.numVertexes + j].xyz[1] = tmpVert[1];
			tess.verts[tess.numVertexes + j].xyz[2] = tmpVert[2];

			tess.verts[tess.numVertexes + j].texCoords[0] = st->st[0];
			tess.verts[tess.numVertexes + j].texCoords[1] = st->st[1];
		}
	}
	else
	{
		// calc tangent spaces
		float       *v;
		const float *v0, *v1, *v2;
		const float *t0, *t1, *t2;
		vec3_t* xyz;
		vec3_t      tangent, *tangents;
		vec3_t      binormal, *binormals;
		vec3_t      *normals;

		xyz = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof(vec3_t) );
		tangents = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );
		binormals = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );
		normals = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );

		for ( i = 0; i < numVertexes; i++, newVert++, oldVert++, oldNormal++, newNormal++ )
		{
			VectorClear( tangents[ i ] );
			VectorClear( binormals[ i ] );

			if ( backlerp == 0 )
			{
				// just copy
				VectorCopy( newNormal->normal, normals[ i ] );
			}
			else
			{
				// interpolate the xyz
				VectorScale( oldNormal->normal, oldXyzScale, normals[ i ] );
				VectorMA( normals[ i ], newXyzScale, newNormal->normal, normals[ i ] );
				VectorNormalizeFast( normals[ i ] );
			}

			if ( backlerp == 0 )
			{
				// just copy
				VectorCopy(newVert->xyz, xyz[i]);
			}
			else
			{
				// interpolate the xyz
				VectorScale(oldVert->xyz, oldXyzScale, xyz[i]);
				VectorMA(xyz[i], newXyzScale, newVert->xyz, xyz[i]);
			}
		}

		for (i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
		{
			int* indices = tri->indexes;
			v0 = xyz[ indices[0] ];
			v1 = xyz[ indices[1] ];
			v2 = xyz[ indices[2] ];

			t0 = st[ indices[ 0 ] ].st;
			t1 = st[ indices[ 1 ] ].st;
			t2 = st[ indices[ 2 ] ].st;

			R_CalcTangents( tangent, binormal, v0, v1, v2, t0, t1, t2 );

			for ( j = 0; j < 3; j++ )
			{
				v = tangents[ indices[ j ]  ];
				VectorAdd( v, tangent, v );

				v = binormals[ indices[ j ] ];
				VectorAdd( v, binormal, v );
			}
		}

		for ( i = 0; i < numVertexes; i++ )
		{

			i16vec4_t qtangents;

			R_TBNtoQtangents( tangents[ i ], binormals[ i ],
					  normals[ i ], qtangents );

			VectorCopy(xyz[i], tess.verts[tess.numVertexes + i].xyz);
			Vector4Copy(qtangents, tess.verts[tess.numVertexes + i].qtangents);
			tess.verts[tess.numVertexes + i].texCoords[0] = st[i].st[0];
			tess.verts[tess.numVertexes + i].texCoords[1] = st[i].st[1];
		}

		ri.Hunk_FreeTempMemory( normals );
		ri.Hunk_FreeTempMemory( binormals );
		ri.Hunk_FreeTempMemory( tangents );
		ri.Hunk_FreeTempMemory( xyz );
	}

	tess.numIndexes += numIndexes;
	tess.numVertexes += numVertexes;
}

/*
==============
Tess_SurfaceMD5

Defines ATTR_POSITION, ATTR_TEXCOORD, and maybe ATTR_QTANGENT
==============
*/
static void Tess_SurfaceMD5( md5Surface_t *srf )
{
	md5Model_t *model = srf->model;

	GLIMP_LOGCOMMENT( "--- Tess_SurfaceMD5 ---" );

	int numIndexes = srf->numTriangles * 3;

	Tess_CheckOverflow( srf->numVerts, numIndexes );

	vec_t entityScale = backEnd.currentEntity->e.skeleton.scale;
	float modelScale = model->internalScale;
	transform_t *bone = bones;
	transform_t *lastBone = bones + model->numBones;

	// Convert bones back to matrices.
	if ( backEnd.currentEntity->e.skeleton.type == refSkeletonType_t::SK_ABSOLUTE )
	{
		refBone_t *entityBone = backEnd.currentEntity->e.skeleton.bones;
		md5Bone_t *modelBone = model->bones;

		for ( ; bone < lastBone; bone++,
			modelBone++, entityBone++ )
		{
			TransInverse( &modelBone->joint, bone );
			TransCombine( bone, &entityBone->t, bone );
			TransAddScale( entityScale, bone );
			TransInsScale( modelScale, bone );
		}
	}
	else if ( tess.skipTangents )
	{
		md5Bone_t *modelBone = model->bones;

		for ( ; bone < lastBone; bone++,
			modelBone++ )
		{
			*bone = modelBone->joint;
			TransInsScale( modelScale, bone );
		}
	}
	else
	{
		for ( ; bone < lastBone; bone++ )
		{
			TransInitScale( entityScale, bone );
			TransInsScale( modelScale, bone );
		}
	}

	glIndex_t *tessIndex = tess.indexes + tess.numIndexes;
	srfTriangle_t *surfaceTriangle = srf->triangles;
	srfTriangle_t *lastTriangle = surfaceTriangle + srf->numTriangles;
	md5Vertex_t *surfaceVertex = srf->verts;

	for ( ; surfaceTriangle < lastTriangle; surfaceTriangle++,
		tessIndex += 3 )
	{
		tessIndex[ 0 ] = tess.numVertexes + surfaceTriangle->indexes[ 0 ];
		tessIndex[ 1 ] = tess.numVertexes + surfaceTriangle->indexes[ 1 ];
		tessIndex[ 2 ] = tess.numVertexes + surfaceTriangle->indexes[ 2 ];
	}

	shaderVertex_t *tessVertex = tess.verts + tess.numVertexes;
	shaderVertex_t *lastVertex = tessVertex + srf->numVerts;

	// Deform the vertices by the lerped bones.
	if ( tess.skipTangents )
	{
		for ( ; tessVertex < lastVertex; tessVertex++,
			surfaceVertex++ )
		{
			vec3_t position = {};

			float *boneWeight = surfaceVertex->boneWeights;
			float *lastWeight = boneWeight + surfaceVertex->numWeights;
			uint32_t *boneIndex = surfaceVertex->boneIndexes;
			vec4_t *surfacePosition = &surfaceVertex->position;

			for ( ; boneWeight < lastWeight; boneWeight++,
				boneIndex++ )
			{
				vec3_t tmp;

				TransformPoint( &bones[ *boneIndex ], *surfacePosition, tmp );
				VectorMA( position, *boneWeight, tmp, position );
			}

			VectorCopy( position, tessVertex->xyz );

			Vector2Copy( surfaceVertex->texCoords, tessVertex->texCoords );
		}
	}
	else
	{
		for ( ; tessVertex < lastVertex; tessVertex++,
			surfaceVertex++ )
		{
			vec3_t tangent = {}, binormal = {}, normal = {}, position = {};

			float *boneWeight = surfaceVertex->boneWeights;
			float *lastWeight = boneWeight + surfaceVertex->numWeights;
			uint32_t *boneIndex = surfaceVertex->boneIndexes;
			vec4_t *surfacePosition = &surfaceVertex->position;
			vec4_t *surfaceNormal = &surfaceVertex->normal;
			vec4_t *surfaceTangent = &surfaceVertex->tangent;
			vec4_t *surfaceBinormal = &surfaceVertex->binormal;

			for ( ; boneWeight < lastWeight; boneWeight++,
				boneIndex++ )
			{
				vec3_t tmp;

				TransformPoint( &bones[ *boneIndex ], *surfacePosition, tmp );
				VectorMA( position, *boneWeight, tmp, position );

				TransformNormalVector( &bones[ *boneIndex ], *surfaceNormal, tmp );
				VectorMA( normal, *boneWeight, tmp, normal );

				TransformNormalVector( &bones[ *boneIndex ], *surfaceTangent, tmp );
				VectorMA( tangent, *boneWeight, tmp, tangent );

				TransformNormalVector( &bones[ *boneIndex ], *surfaceBinormal, tmp );
				VectorMA( binormal, *boneWeight, tmp, binormal );
			}

			VectorNormalizeFast( normal );
			VectorNormalizeFast( tangent );
			VectorNormalizeFast( binormal );
			VectorCopy( position, tessVertex->xyz );

			R_TBNtoQtangentsFast( tangent, binormal, normal, tessVertex->qtangents );

			Vector2Copy( surfaceVertex->texCoords, tessVertex->texCoords );
		}
	}

	tess.numIndexes += numIndexes;
	tess.numVertexes += srf->numVerts;
}

/*
=================
Tess_SurfaceIQM

Compute vertices for this model surface
If not using a VBO, defines ATTR_POSITION, ATTR_TEXCOORD, and maybe ATTR_QTANGENT
=================
*/
void Tess_SurfaceIQM( srfIQModel_t *surf ) {
	IQModel_t *model = surf->data;
	int offset = tess.numVertexes - surf->first_vertex;

	GLIMP_LOGCOMMENT( "--- Tess_SurfaceIQM ---" );

	int numIndexes = surf->num_triangles * 3;

	vec_t entityScale = backEnd.currentEntity->e.skeleton.scale;
	float modelScale = model->internalScale;
	transform_t *bone = bones;
	transform_t *lastBone = bones + model->num_joints;

	// Convert bones back to matrices.
	if ( backEnd.currentEntity->e.skeleton.type == refSkeletonType_t::SK_ABSOLUTE )
	{
		refBone_t *entityBone = backEnd.currentEntity->e.skeleton.bones;
		transform_t *modelJoint = model->joints;

		for ( ; bone < lastBone; bone++,
			modelJoint++, entityBone++ )
		{
			TransInverse( modelJoint, bone );
			TransCombine( bone, &entityBone->t, bone );
			TransAddScale( entityScale, bone );
			TransInsScale( modelScale, bone );
		}
	}
	else if ( tess.skipTangents )
	{
		transform_t *modelJoint = model->joints;

		for ( ; bone < lastBone; bone++,
			modelJoint++ )
		{
			*bone = *modelJoint;
			TransInsScale( modelScale, bone );
		}
	}
	else
	{
		for ( ; bone < lastBone; bone++ )
		{
			TransInitScale( entityScale, bone );
			TransInsScale( modelScale, bone );
		}
	}

	/* This must run after the bone computation code or rendering
	will be buggy, and must run before the other loops because it
	returns early and then save CPU time.

	This test is false when r_vboModels is disabled, or when
	glConfig.vboVertexSkinningAvailable is false because related
	OpenGL extensions are unsupported, or the model has too much
	bones for the hardware, or r_vboVertexSkinning is disabled.

	See https://github.com/Unvanquished/Unvanquished/issues/1207 */
	if( surf->vbo && surf->ibo )
	{
		if( model->num_joints > 0 )
		{
			std::copy_n( bones, model->num_joints, tess.bones );
			tess.numBones = model->num_joints;
		}
		else
		{
			TransInitScale( model->internalScale * backEnd.currentEntity->e.skeleton.scale, &tess.bones[ 0 ] );
			tess.numBones = 1;
		}

		R_BindVBO( surf->vbo );
		R_BindIBO( surf->ibo );

		tess.vboVertexSkinning = true;
		tess.multiDrawIndexes[ tess.multiDrawPrimitives ] = reinterpret_cast<glIndex_t*>( sizeof(glIndex_t) * 3 * surf->first_triangle );
		tess.multiDrawCounts[ tess.multiDrawPrimitives ] = numIndexes;
		tess.multiDrawPrimitives++;

		Tess_End();

		return;
	}

	Tess_CheckOverflow( surf->num_vertexes, numIndexes );

	glIndex_t *tessIndex = tess.indexes + tess.numIndexes;
	int *modelTriangle = model->triangles + 3 * surf->first_triangle;
	int *lastModelTriangle = modelTriangle + 3 * surf->num_triangles;

	for ( ; modelTriangle < lastModelTriangle; modelTriangle++,
		tessIndex++ )
	{
		*tessIndex = offset + *modelTriangle;
	}

	int firstVertex = surf->first_vertex;
	float *modelPosition = model->positions + 3 * firstVertex;
	float *modelNormal = model->normals + 3 * firstVertex;
	float *modelTangent = model->tangents + 3 * firstVertex;
	float *modelBitangent = model->bitangents + 3 * firstVertex;
	float *modelTexcoord = model->texcoords + 2 * firstVertex;
	shaderVertex_t *tessVertex = tess.verts + tess.numVertexes;
	shaderVertex_t *lastVertex = tessVertex + surf->num_vertexes;

	// Deform the vertices by the lerped bones.
	if ( model->num_joints > 0 && model->blendWeights && model->blendIndexes )
	{
		const float weightFactor = 1.0f / 255.0f;

		if ( tess.skipTangents )
		{
			byte *modelBlendIndex = model->blendIndexes + 4 * firstVertex;
			byte *modelBlendWeight = model->blendWeights + 4 * firstVertex;

			for ( ; tessVertex < lastVertex; tessVertex++,
				modelPosition += 3, modelNormal += 3,
				modelTangent += 3, modelBitangent += 3,
				modelTexcoord += 2 )
			{
				vec3_t position = {};

				byte *lastBlendIndex = modelBlendIndex + 4;

				for ( ; modelBlendIndex < lastBlendIndex; modelBlendIndex++,
					modelBlendWeight++ )
				{
					if ( *modelBlendWeight == 0 )
					{
						continue;
					}

					float weight = *modelBlendWeight * weightFactor;
					vec3_t tmp;

					TransformPoint( &bones[ *modelBlendIndex ], modelPosition, tmp );
					VectorMA( position, weight, tmp, position );
				}

				VectorCopy( position, tessVertex->xyz );

				Vector2Copy( modelTexcoord, tessVertex->texCoords );
			}
		}
		else
		{
			byte *modelBlendIndex = model->blendIndexes + 4 * firstVertex;
			byte *modelBlendWeight = model->blendWeights + 4 * firstVertex;

			for ( ; tessVertex < lastVertex; tessVertex++,
				modelPosition += 3, modelNormal += 3,
				modelTangent += 3, modelBitangent += 3,
				modelTexcoord += 2 )
			{
				vec3_t position = {}, tangent = {}, binormal = {}, normal = {};

				byte *lastBlendIndex = modelBlendIndex + 4;

				for ( ; modelBlendIndex < lastBlendIndex; modelBlendIndex++,
					modelBlendWeight++ )
				{
					if ( *modelBlendWeight == 0 )
					{
						continue;
					}

					float weight = *modelBlendWeight * weightFactor;
					vec3_t tmp;

					TransformPoint( &bones[ *modelBlendIndex ], modelPosition, tmp );
					VectorMA( position, weight, tmp, position );

					TransformNormalVector( &bones[ *modelBlendIndex ], modelNormal, tmp );
					VectorMA( normal, weight, tmp, normal );

					TransformNormalVector( &bones[ *modelBlendIndex ], modelTangent, tmp );
					VectorMA( tangent, weight, tmp, tangent );

					TransformNormalVector( &bones[ *modelBlendIndex ], modelBitangent, tmp );
					VectorMA( binormal, weight, tmp, binormal );
				}

				VectorNormalizeFast( normal );
				VectorNormalizeFast( tangent );
				VectorNormalizeFast( binormal );
				VectorCopy( position, tessVertex->xyz );

				R_TBNtoQtangentsFast( tangent, binormal, normal, tessVertex->qtangents );

				Vector2Copy( modelTexcoord, tessVertex->texCoords );
			}
		}
	}
	else
	{
		float scale = model->internalScale * backEnd.currentEntity->e.skeleton.scale;

		for ( ; tessVertex < lastVertex; tessVertex++,
			modelPosition += 3, modelNormal += 3,
			modelTangent += 3, modelBitangent += 3,
			modelTexcoord += 2 )
		{
			VectorScale( modelPosition, scale, tessVertex->xyz );

			R_TBNtoQtangentsFast( modelTangent, modelBitangent, modelNormal, tessVertex->qtangents );

			Vector2Copy( modelTexcoord, tessVertex->texCoords );
		}
	}

	tess.numIndexes  += numIndexes;
	tess.numVertexes += surf->num_vertexes;
}

//===========================================================================

/*
====================
Tess_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
static void Tess_SurfaceEntity( surfaceType_t* )
{
	GLIMP_LOGCOMMENT( "--- Tess_SurfaceEntity ---" );

	switch ( backEnd.currentEntity->e.reType )
	{
		case refEntityType_t::RT_SPRITE:
			Tess_SurfaceSprite();
			break;
		default:
			break;
	}
}

static void Tess_SurfaceBad( surfaceType_t* )
{
	GLIMP_LOGCOMMENT( "--- Tess_SurfaceBad ---" );

	Log::Notice("Bad surface tesselated." );
}

/*
==============
Tess_SurfaceVBOMesh
==============
*/
static void Tess_SurfaceVBOMesh( srfVBOMesh_t *srf )
{
	GLIMP_LOGCOMMENT( "--- Tess_SurfaceVBOMesh ---" );

	Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numTriangles * 3, srf->firstIndex );
}

/*
==============
Tess_SurfaceVBOMDVMesh
==============
*/
void Tess_SurfaceVBOMDVMesh( srfVBOMDVMesh_t *surface )
{
	refEntity_t *refEnt;

	GLIMP_LOGCOMMENT( "--- Tess_SurfaceVBOMDVMesh ---" );

	if ( !surface->vbo || !surface->ibo )
	{
		return;
	}

	Tess_EndBegin();

	R_BindVBO( surface->vbo );
	R_BindIBO( surface->ibo );

	tess.numIndexes = surface->numIndexes;
	tess.numVertexes = surface->numVerts;
	tess.vboVertexAnimation = true;

	refEnt = &backEnd.currentEntity->e;

	if ( refEnt->oldframe == refEnt->frame )
	{
		glState.vertexAttribsInterpolation = 0;
	}
	else
	{
		glState.vertexAttribsInterpolation = ( 1.0F - refEnt->backlerp );
	}

	glState.vertexAttribsOldFrame = refEnt->oldframe;
	glState.vertexAttribsNewFrame = refEnt->frame;

	Tess_End();
}

/*
==============
Tess_SurfaceVBOMD5Mesh
==============
*/
static void Tess_SurfaceVBOMD5Mesh( srfVBOMD5Mesh_t *srf )
{
	md5Model_t *model = srf->md5Model;

	GLIMP_LOGCOMMENT( "--- Tess_SurfaceVBOMD5Mesh ---" );

	if ( !srf->vbo || !srf->ibo )
	{
		return;
	}

	Tess_EndBegin();

	R_BindVBO( srf->vbo );
	R_BindIBO( srf->ibo );

	tess.numIndexes = srf->numIndexes;
	tess.numVertexes = srf->numVerts;
	tess.numBones = srf->numBoneRemap;
	tess.vboVertexSkinning = true;

	vec_t entityScale = backEnd.currentEntity->e.skeleton.scale;
	float modelScale = model->internalScale;
	transform_t *bone = tess.bones;
	transform_t *lastBone = bone + tess.numBones;

	if ( backEnd.currentEntity->e.skeleton.type == refSkeletonType_t::SK_ABSOLUTE )
	{
		int *boneRemapInverse = srf->boneRemapInverse;

		for ( ; bone < lastBone; bone++,
			boneRemapInverse++ )
		{
			refBone_t *entityBone = &backEnd.currentEntity->e.skeleton.bones[ *boneRemapInverse ];
			md5Bone_t *modelBone = &model->bones[ *boneRemapInverse ];

			TransInverse( &modelBone->joint, bone );
			TransCombine( bone, &entityBone->t, bone );
			TransAddScale( entityScale, bone );
			TransInsScale( modelScale, bone );
		}
	}
	else if ( tess.skipTangents )
	{
		md5Bone_t *modelBone = model->bones;

		for ( ; bone < lastBone; bone++,
			modelBone++ )
		{
			*bone = modelBone->joint;
			TransInsScale( modelScale, bone );
		}
	}
	else
	{
		for ( ; bone < lastBone; bone++ )
		{
			TransInitScale( entityScale, bone );
			TransInsScale( modelScale, bone );
		}
	}

	Tess_End();
}

static void Tess_SurfaceSkip( void* )
{
}

// *INDENT-OFF*
void ( *rb_surfaceTable[ Util::ordinal(surfaceType_t::SF_NUM_SURFACE_TYPES) ] )( void * ) =
{
	( void ( * )( void * ) ) Tess_SurfaceBad,  // SF_BAD,
	( void ( * )( void * ) ) Tess_SurfaceSkip,  // SF_SKIP,
	( void ( * )( void * ) ) Tess_SurfaceFace,  // SF_FACE,
	( void ( * )( void * ) ) Tess_SurfaceGrid,  // SF_GRID,
	( void ( * )( void * ) ) Tess_SurfaceTriangles,  // SF_TRIANGLES,
	( void ( * )( void * ) ) Tess_SurfacePolychain,  // SF_POLY,
	( void ( * )( void * ) ) Tess_SurfaceMDV,  // SF_MDV,
	( void ( * )( void * ) ) Tess_SurfaceMD5,  // SF_MD5,
	( void ( * )( void * ) ) Tess_SurfaceIQM,  // SF_IQM,

	( void ( * )( void * ) ) Tess_SurfaceEntity,  // SF_ENTITY
	( void ( * )( void * ) ) Tess_SurfaceVBOMesh,  // SF_VBO_MESH
	( void ( * )( void * ) ) Tess_SurfaceVBOMD5Mesh,  // SF_VBO_MD5MESH
	( void ( * )( void * ) ) Tess_SurfaceVBOMDVMesh  // SF_VBO_MDVMESH
};
