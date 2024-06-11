/*
===========================================================================
Copyright (C) 2007-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_vbo.c
#include "tr_local.h"
#include "Material.h"

// "templates" for VBO vertex data layouts

// interleaved position and qtangents per frame/vertex in part 1
struct fmtVertexAnim1 {
	i16vec4_t position;
	i16vec4_t qtangents;
};
const GLsizei sizeVertexAnim1 = sizeof( struct fmtVertexAnim1 );
// interleaved texcoords and colour in part 2
struct fmtVertexAnim2 {
	f16vec2_t texcoord;
	Color::Color32Bit colour;
};
const GLsizei sizeVertexAnim2 = sizeof( struct fmtVertexAnim2 );

// interleaved data: position, colour, qtangent, texcoord
// -> struct shaderVertex_t in tr_local.h
const GLsizei sizeShaderVertex = sizeof( shaderVertex_t );

static uint32_t R_DeriveAttrBits( const vboData_t &data )
{
	uint32_t stateBits = 0;

	if ( data.xyz )
	{
		stateBits |= ATTR_POSITION;
	}

	if ( data.qtangent )
	{
		stateBits |= ATTR_QTANGENT;
	}

	if ( data.color )
	{
		stateBits |= ATTR_COLOR;
	}

	if ( data.st )
	{
		stateBits |= ATTR_TEXCOORD;
	}

	if ( data.numFrames )
	{
		if ( data.xyz )
		{
			stateBits |= ATTR_POSITION2;
		}

		if ( data.qtangent )
		{
			stateBits |= ATTR_QTANGENT2;
		}
	}

	return stateBits;
}

static void R_SetAttributeLayoutsVertexAnimation( VBO_t *vbo )
{
	// part 1 is repeated for every frame
	GLsizei sizePart1 = sizeVertexAnim1 * vbo->vertexesNum * vbo->framesNum;
	GLsizei sizePart2 = sizeVertexAnim2 * vbo->vertexesNum;

	vbo->attribs[ ATTR_INDEX_POSITION ].numComponents = 4;
	vbo->attribs[ ATTR_INDEX_POSITION ].componentType = GL_SHORT;
	vbo->attribs[ ATTR_INDEX_POSITION ].normalize     = GL_TRUE;
	vbo->attribs[ ATTR_INDEX_POSITION ].ofs           = offsetof( struct fmtVertexAnim1, position );
	vbo->attribs[ ATTR_INDEX_POSITION ].stride        = sizeVertexAnim1;
	vbo->attribs[ ATTR_INDEX_POSITION ].frameOffset   = sizeVertexAnim1 * vbo->vertexesNum;

	vbo->attribs[ ATTR_INDEX_QTANGENT ].numComponents = 4;
	vbo->attribs[ ATTR_INDEX_QTANGENT ].componentType = GL_SHORT;
	vbo->attribs[ ATTR_INDEX_QTANGENT ].normalize     = GL_TRUE;
	vbo->attribs[ ATTR_INDEX_QTANGENT ].ofs          = offsetof( struct fmtVertexAnim1, qtangents );
	vbo->attribs[ ATTR_INDEX_QTANGENT ].stride       = sizeVertexAnim1;
	vbo->attribs[ ATTR_INDEX_QTANGENT ].frameOffset  = sizeVertexAnim1 * vbo->vertexesNum;

	// these use the same layout
	vbo->attribs[ ATTR_INDEX_POSITION2 ] = vbo->attribs[ ATTR_INDEX_POSITION ];
	vbo->attribs[ ATTR_INDEX_QTANGENT2 ] = vbo->attribs[ ATTR_INDEX_QTANGENT ];

	vbo->attribs[ ATTR_INDEX_TEXCOORD ].numComponents = 2;
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].componentType = GL_HALF_FLOAT;
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].normalize     = GL_FALSE;
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].ofs          = sizePart1 + offsetof( struct fmtVertexAnim2, texcoord );
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].stride       = sizeVertexAnim2;
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].frameOffset  = 0;

	vbo->attribs[ ATTR_INDEX_COLOR ].numComponents   = 4;
	vbo->attribs[ ATTR_INDEX_COLOR ].componentType   = GL_UNSIGNED_BYTE;
	vbo->attribs[ ATTR_INDEX_COLOR ].normalize       = GL_TRUE;
	vbo->attribs[ ATTR_INDEX_COLOR ].ofs             = sizePart1 + offsetof( struct fmtVertexAnim2, colour );
	vbo->attribs[ ATTR_INDEX_COLOR ].stride          = sizeVertexAnim2;
	vbo->attribs[ ATTR_INDEX_COLOR ].frameOffset     = 0;

	// total size
	vbo->vertexesSize = sizePart1 + sizePart2;
}

static void R_SetAttributeLayoutsStatic( VBO_t *vbo )
{
	vbo->attribs[ ATTR_INDEX_POSITION ].numComponents = 3;
	vbo->attribs[ ATTR_INDEX_POSITION ].componentType = GL_FLOAT;
	vbo->attribs[ ATTR_INDEX_POSITION ].normalize     = GL_FALSE;
	vbo->attribs[ ATTR_INDEX_POSITION ].ofs           = offsetof( shaderVertex_t, xyz );
	vbo->attribs[ ATTR_INDEX_POSITION ].stride        = sizeShaderVertex;
	vbo->attribs[ ATTR_INDEX_POSITION ].frameOffset   = 0;

	vbo->attribs[ ATTR_INDEX_COLOR ].numComponents   = 4;
	vbo->attribs[ ATTR_INDEX_COLOR ].componentType   = GL_UNSIGNED_BYTE;
	vbo->attribs[ ATTR_INDEX_COLOR ].normalize       = GL_TRUE;
	vbo->attribs[ ATTR_INDEX_COLOR ].ofs             = offsetof( shaderVertex_t, color );
	vbo->attribs[ ATTR_INDEX_COLOR ].stride          = sizeShaderVertex;
	vbo->attribs[ ATTR_INDEX_COLOR ].frameOffset     = 0;

	vbo->attribs[ ATTR_INDEX_QTANGENT ].numComponents = 4;
	vbo->attribs[ ATTR_INDEX_QTANGENT ].componentType = GL_SHORT;
	vbo->attribs[ ATTR_INDEX_QTANGENT ].normalize     = GL_TRUE;
	vbo->attribs[ ATTR_INDEX_QTANGENT ].ofs           = offsetof( shaderVertex_t, qtangents );
	vbo->attribs[ ATTR_INDEX_QTANGENT ].stride        = sizeShaderVertex;
	vbo->attribs[ ATTR_INDEX_QTANGENT ].frameOffset   = 0;

	vbo->attribs[ ATTR_INDEX_TEXCOORD ].numComponents = 4;
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].componentType = GL_HALF_FLOAT;
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].normalize     = GL_FALSE;
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].ofs           = offsetof( shaderVertex_t, texCoords );
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].stride        = sizeShaderVertex;
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].frameOffset   = 0;

	// total size
	vbo->vertexesSize = sizeShaderVertex * vbo->vertexesNum;
}

static void R_SetAttributeLayoutsXYST( VBO_t *vbo )
{
	vbo->attribs[ ATTR_INDEX_POSITION ].numComponents = 2;
	vbo->attribs[ ATTR_INDEX_POSITION ].componentType = GL_FLOAT;
	vbo->attribs[ ATTR_INDEX_POSITION ].normalize     = GL_FALSE;
	vbo->attribs[ ATTR_INDEX_POSITION ].ofs           = 0;
	vbo->attribs[ ATTR_INDEX_POSITION ].stride        = sizeof( vec4_t );
	vbo->attribs[ ATTR_INDEX_POSITION ].frameOffset   = 0;

	vbo->attribs[ ATTR_INDEX_TEXCOORD ].numComponents = 2;
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].componentType = GL_FLOAT;
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].normalize     = GL_FALSE;
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].ofs           = sizeof( vec2_t );
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].stride        = sizeof( vec4_t );
	vbo->attribs[ ATTR_INDEX_TEXCOORD ].frameOffset   = 0;

	// total size
	vbo->vertexesSize = sizeof( vec4_t ) * vbo->vertexesNum;
}

static void R_SetVBOAttributeLayouts( VBO_t *vbo )
{
	if ( vbo->layout == vboLayout_t::VBO_LAYOUT_VERTEX_ANIMATION )
	{
		R_SetAttributeLayoutsVertexAnimation( vbo );
	}
	else if ( vbo->layout == vboLayout_t::VBO_LAYOUT_STATIC )
	{
		R_SetAttributeLayoutsStatic( vbo );
	}
	else if ( vbo->layout == vboLayout_t::VBO_LAYOUT_XYST )
	{
		R_SetAttributeLayoutsXYST( vbo );
	}
	else
	{
		Sys::Drop("%sUnknown attribute layout for vbo: %s\n",
		          Color::ToString( Color::Yellow ), vbo->name );
	}
}

static uint32_t ComponentSize( GLenum type )
{
	switch ( type )
	{
	case GL_UNSIGNED_BYTE:
		return 1;

	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_HALF_FLOAT:
		return 2;

	case GL_FLOAT:
		return 4;
	}

	Sys::Error( "VBO ComponentSize: unknown type %d", type );
}

static void R_CopyVertexData( VBO_t *vbo, byte *outData, vboData_t inData )
{
	uint32_t v;

	if ( vbo->layout == vboLayout_t::VBO_LAYOUT_VERTEX_ANIMATION )
	{
		struct fmtVertexAnim1 *ptr = ( struct fmtVertexAnim1 * )outData;

		for ( v = 0; v < vbo->framesNum * vbo->vertexesNum; v++ )
		{

			if ( ( vbo->attribBits & ATTR_POSITION ) )
			{
				vec4_t tmp;
				VectorScale( inData.xyz[ v ], 1.0f / 512.0f, tmp);
				tmp[ 3 ] = 1.0f; // unused

				floatToSnorm16( tmp, ptr[ v ].position );
			}

			if ( ( vbo->attribBits & ATTR_QTANGENT ) )
			{
				Vector4Copy( inData.qtangent[ v ], ptr[ v ].qtangents );
			}
		}
	}

	for ( v = 0; v < vbo->vertexesNum; v++ )
	{
		if ( vbo->layout == vboLayout_t::VBO_LAYOUT_XYST ) {
			vec2_t *ptr = ( vec2_t * )outData;
			if ( ( vbo->attribBits & ATTR_POSITION ) )
			{
				Vector2Copy( inData.xyz[ v ], ptr[ 2 * v ] );
			}

			if ( ( vbo->attribBits & ATTR_TEXCOORD ) )
			{
				Vector2Copy( inData.stf[ v ], ptr[ 2 * v + 1 ] );
			}
		} else if ( vbo->layout == vboLayout_t::VBO_LAYOUT_VERTEX_ANIMATION ) {
			struct fmtVertexAnim2 *ptr = ( struct fmtVertexAnim2 * )( outData + ( vbo->framesNum * vbo->vertexesNum ) * sizeVertexAnim1 );

			if ( ( vbo->attribBits & ATTR_TEXCOORD ) )
			{
				Vector2Copy( inData.st[ v ], ptr[ v ].texcoord );
			}

			if ( ( vbo->attribBits & ATTR_COLOR ) )
			{
				ptr[ v ].colour = Color::Adapt( inData.color[ v ] );
			}
		} else {
			Sys::Drop( "R_CopyVertexData: unsupported VBO layout" );
		}
	}

}

#if defined( GL_ARB_buffer_storage ) && defined( GL_ARB_sync )
/*
============
R_InitRingbuffer
============
*/
static void R_InitRingbuffer( GLenum target, GLsizei elementSize,
			      GLsizei segmentElements, glRingbuffer_t *rb ) {
	GLsizei totalSize = elementSize * segmentElements * DYN_BUFFER_SEGMENTS;
	int i;

	glBufferStorage( target, totalSize, nullptr,
			 GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT );
	rb->baseAddr = glMapBufferRange( target, 0, totalSize,
					 GL_MAP_WRITE_BIT |
					 GL_MAP_PERSISTENT_BIT |
					 GL_MAP_FLUSH_EXPLICIT_BIT );
	rb->elementSize = elementSize;
	rb->segmentElements = segmentElements;
	rb->activeSegment = 0;
	for( i = 1; i < DYN_BUFFER_SEGMENTS; i++ ) {
		rb->syncs[ i ] = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	}
}

/*
============
R_RotateRingbuffer
============
*/
static GLsizei R_RotateRingbuffer( glRingbuffer_t *rb ) {
	rb->syncs[ rb->activeSegment ] = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );

	rb->activeSegment++;
	if( rb->activeSegment >= DYN_BUFFER_SEGMENTS )
		rb->activeSegment = 0;

	// wait until next segment is ready in 1 sec intervals
	while( glClientWaitSync( rb->syncs[ rb->activeSegment ], GL_SYNC_FLUSH_COMMANDS_BIT,
				 10000000 ) == GL_TIMEOUT_EXPIRED ) {
		Log::Warn("long wait for GL buffer" );
	};
	glDeleteSync( rb->syncs[ rb->activeSegment ] );

	return rb->activeSegment * rb->segmentElements;
}

/*
============
R_ShutdownRingbuffer
============
*/
static void R_ShutdownRingbuffer( GLenum target, glRingbuffer_t *rb ) {
	int i;

	glUnmapBuffer( target );
	rb->baseAddr = nullptr;

	for( i = 0; i < DYN_BUFFER_SEGMENTS; i++ ) {
		if( i == rb->activeSegment )
			continue;

		glDeleteSync( rb->syncs[ i ] );
	}
}
#endif

VBO_t *R_CreateDynamicVBO( const char *name, int numVertexes, uint32_t stateBits, vboLayout_t layout )
{
	VBO_t *vbo;

	if ( !numVertexes )
	{
		return nullptr;
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	vbo = (VBO_t*) ri.Hunk_Alloc( sizeof( *vbo ), ha_pref::h_low );
	*vbo = {};

	tr.vbos.push_back( vbo );

	Q_strncpyz( vbo->name, name, sizeof( vbo->name ) );

	vbo->layout = layout;
	vbo->framesNum = 0;
	vbo->vertexesNum = numVertexes;
	vbo->attribBits = stateBits;
	vbo->usage = GL_DYNAMIC_DRAW;

	R_SetVBOAttributeLayouts( vbo );

	glGenBuffers( 1, &vbo->vertexesVBO );

	R_BindVBO( vbo );

#if defined( GL_ARB_buffer_storage ) && defined( GL_ARB_sync )
	if( glConfig2.mapBufferRangeAvailable && glConfig2.bufferStorageAvailable &&
	    glConfig2.syncAvailable ) {
		R_InitRingbuffer( GL_ARRAY_BUFFER, sizeof( shaderVertex_t ),
				  numVertexes, &tess.vertexRB );
	} else
#endif
	{
		glBufferData( GL_ARRAY_BUFFER, vbo->vertexesSize, nullptr, vbo->usage );
	}
	R_BindNullVBO();

	GL_CheckErrors();

	return vbo;
}

static void CopyVertexAttribute(
	const vboAttributeLayout_t &attrib, const vertexAttributeSpec_t &spec,
	uint32_t count, byte *interleavedData )
{
	if ( count == 0 )
	{
		return; // some loops below are 'do/while'-like
	}

	const size_t inStride = spec.stride;
	const size_t outStride = attrib.stride;

	byte *out = interleavedData + attrib.ofs;
	const byte *in = reinterpret_cast<const byte *>( spec.begin );

	if ( attrib.componentType == spec.componentInputType )
	{
		uint32_t size = attrib.numComponents * ComponentSize( attrib.componentType );

		for ( uint32_t v = count; ; )
		{
			memcpy( out, in, size );

			if ( --v == 0 ) break;
			in += inStride;
			out += outStride;
		}
	}
	else if ( spec.componentInputType == GL_FLOAT && attrib.componentType == GL_HALF_FLOAT )
	{
		for ( uint32_t v = count; ; )
		{
			const float *single = reinterpret_cast<const float *>( in );
			f16_t *half = reinterpret_cast<f16_t *>( out );
			for ( uint32_t n = spec.numComponents; n--; )
			{
				*half++ = floatToHalf( *single++ );
			}

			if ( --v == 0 ) break;
			in += inStride;
			out += outStride;
		}
	}
	else if ( spec.componentInputType == GL_HALF_FLOAT && attrib.componentType == GL_FLOAT )
	{
		for ( uint32_t v = count; ; )
		{
			const f16_t *half = reinterpret_cast<const f16_t *>( in );
			float *single = reinterpret_cast<float *>( out );
			for ( uint32_t n = spec.numComponents; n--; )
			{
				*single++ = halfToFloat( *half++ );
			}

			if ( --v == 0 ) break;
			in += inStride;
			out += outStride;
		}
	}
	else if ( spec.componentInputType == GL_FLOAT && attrib.componentType == GL_SHORT
	          && spec.attrOptions & ATTR_OPTION_NORMALIZE )
	{
		for ( uint32_t v = count; ; )
		{
			const float *single = reinterpret_cast<const float *>( in );
			int16_t *snorm = reinterpret_cast<int16_t *>( out );
			for ( uint32_t n = spec.numComponents; n--; )
			{
				*snorm++ = floatToSnorm16( *single++ );
			}

			if ( --v == 0 ) break;
			in += inStride;
			out += outStride;
		}
	}
	else if ( spec.componentInputType == GL_FLOAT && attrib.componentType == GL_UNSIGNED_SHORT
	          && spec.attrOptions & ATTR_OPTION_NORMALIZE )
	{
		for ( uint32_t v = count; ; )
		{
			const float *single = reinterpret_cast<const float *>( in );
			uint16_t *unorm = reinterpret_cast<uint16_t *>( out );
			for ( uint32_t n = spec.numComponents; n--; )
			{
				*unorm++ = floatToUnorm16( *single++ );
			}

			if ( --v == 0 ) break;
			in += inStride;
			out += outStride;
		}
	}
	else
	{
		Sys::Error( "Unsupported GL type conversion (%d to %d)",
		            spec.componentInputType, attrib.componentType );
	}
}

VBO_t *R_CreateStaticVBO(
	Str::StringRef name,
	const vertexAttributeSpec_t *attrBegin, const vertexAttributeSpec_t *attrEnd,
	uint32_t numVerts )
{
	// make sure the render thread is stopped
	R_SyncRenderThread();

	VBO_t *vbo = (VBO_t*) ri.Hunk_Alloc( sizeof( *vbo ), ha_pref::h_low );
	*vbo = {};
	tr.vbos.push_back( vbo );

	Q_strncpyz( vbo->name, name.c_str(), sizeof(vbo->name));
	vbo->vertexesNum = numVerts;
	vbo->usage = GL_STATIC_DRAW;

	glGenBuffers( 1, &vbo->vertexesVBO );
	R_BindVBO( vbo );

	uint32_t ofs = 0;

	for ( const vertexAttributeSpec_t *spec = attrBegin; spec != attrEnd; ++spec )
	{
		vboAttributeLayout_t &attrib = vbo->attribs[ spec->attrIndex ];
		ASSERT_EQ( attrib.numComponents, 0 );
		ASSERT_NQ( spec->numComponents, 0U );
		attrib.componentType = spec->componentStorageType;
		if ( attrib.componentType == GL_HALF_FLOAT && !glConfig2.halfFloatVertexAvailable )
		{
			attrib.componentType = GL_FLOAT;
		}
		attrib.numComponents = spec->numComponents;
		attrib.ofs = ofs;
		attrib.normalize = spec->attrOptions & ATTR_OPTION_NORMALIZE ? GL_TRUE : GL_FALSE;

		ofs += attrib.numComponents * ComponentSize( attrib.componentType );
		ofs = ( ofs + 3 ) & ~3;
	}

	for ( int i = 0; i < ATTR_INDEX_MAX; i++ )
	{
		if ( vbo->attribs[ i ].numComponents )
		{
			vbo->attribs[ i ].stride = ofs;
			vbo->attribBits |= 1 << i;
		}
	}

	vbo->vertexesSize = numVerts * ofs;

	// TODO: does it really need to be interleaved?
	byte *interleavedData = (byte *)ri.Hunk_AllocateTempMemory( vbo->vertexesSize );

	for ( const vertexAttributeSpec_t *spec = attrBegin; spec != attrEnd; ++spec )
	{
		CopyVertexAttribute( vbo->attribs[ spec->attrIndex ], *spec, numVerts, interleavedData );
	}

#ifdef GL_ARB_buffer_storage
	if( glConfig2.bufferStorageAvailable ) {
		glBufferStorage( GL_ARRAY_BUFFER, vbo->vertexesSize, interleavedData, 0 );
	} else
#endif
	{
		glBufferData( GL_ARRAY_BUFFER, vbo->vertexesSize, interleavedData, vbo->usage );
	}

	ri.Hunk_FreeTempMemory( interleavedData );

	R_BindNullVBO();
	GL_CheckErrors();

	return vbo;
}

/*
============
R_CreateVBO
============
*/
VBO_t *R_CreateStaticVBO( const char *name, vboData_t data, vboLayout_t layout )
{
	// make sure the render thread is stopped
	R_SyncRenderThread();

	VBO_t *vbo = (VBO_t*) ri.Hunk_Alloc( sizeof( *vbo ), ha_pref::h_low );
	*vbo = {};

	tr.vbos.push_back( vbo );

	Q_strncpyz( vbo->name, name, sizeof( vbo->name ) );

	vbo->layout = layout;
	vbo->vertexesNum = data.numVerts;
	vbo->framesNum = data.numFrames;
	vbo->attribBits = R_DeriveAttrBits( data );
	vbo->usage = GL_STATIC_DRAW;

	R_SetVBOAttributeLayouts( vbo );

	glGenBuffers( 1, &vbo->vertexesVBO );
	R_BindVBO( vbo );

	byte *outData;
#ifdef GL_ARB_buffer_storage
	if( glConfig2.bufferStorageAvailable ) {
		outData = (byte *)ri.Hunk_AllocateTempMemory( vbo->vertexesSize );
		R_CopyVertexData( vbo, outData, data );
		glBufferStorage( GL_ARRAY_BUFFER, vbo->vertexesSize,
				 outData, 0 );
		ri.Hunk_FreeTempMemory( outData );
	} else
#endif
	{
		glBufferData( GL_ARRAY_BUFFER, vbo->vertexesSize, nullptr, vbo->usage );
		outData = (byte *)glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );
		R_CopyVertexData( vbo, outData, data );
		glUnmapBuffer( GL_ARRAY_BUFFER );
	}

	R_BindNullVBO();

	GL_CheckErrors();

	return vbo;
}

/*
============
R_CreateVBO2
============
*/
VBO_t *R_CreateStaticVBO2( const char *name, int numVertexes, shaderVertex_t *verts, unsigned int stateBits )
{
	VBO_t  *vbo;

	if ( !numVertexes )
	{
		return nullptr;
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	vbo = ( VBO_t * ) ri.Hunk_Alloc( sizeof( *vbo ), ha_pref::h_low );
	*vbo = {};

	tr.vbos.push_back( vbo );

	Q_strncpyz( vbo->name, name, sizeof( vbo->name ) );

	vbo->layout = vboLayout_t::VBO_LAYOUT_STATIC;
	vbo->framesNum = 0;
	vbo->vertexesNum = numVertexes;
	vbo->attribBits = stateBits;
	vbo->usage = GL_STATIC_DRAW;

	R_SetVBOAttributeLayouts( vbo );
	
	glGenBuffers( 1, &vbo->vertexesVBO );
	R_BindVBO( vbo );

#ifdef GL_ARB_buffer_storage
	if( glConfig2.bufferStorageAvailable ) {
		glBufferStorage( GL_ARRAY_BUFFER, vbo->vertexesSize,
				 verts, 0 );
	} else
#endif
	{
		glBufferData( GL_ARRAY_BUFFER, vbo->vertexesSize,
			      verts, vbo->usage );
	}

	R_BindNullVBO();
	GL_CheckErrors();

	return vbo;
}


/*
============
R_CreateIBO
============
*/
IBO_t *R_CreateDynamicIBO( const char *name, int numIndexes )
{
	IBO_t *ibo;

	// make sure the render thread is stopped
	R_SyncRenderThread();

	ibo = (IBO_t*) ri.Hunk_Alloc( sizeof( *ibo ), ha_pref::h_low );
	tr.ibos.push_back( ibo );

	Q_strncpyz( ibo->name, name, sizeof( ibo->name ) );

	ibo->indexesSize = numIndexes * sizeof( glIndex_t );
	ibo->indexesNum = numIndexes;

	glGenBuffers( 1, &ibo->indexesVBO );

	R_BindIBO( ibo );
#if defined( GL_ARB_buffer_storage ) && defined( GL_ARB_sync )
	if( glConfig2.mapBufferRangeAvailable && glConfig2.bufferStorageAvailable &&
	    glConfig2.syncAvailable ) {
		R_InitRingbuffer( GL_ELEMENT_ARRAY_BUFFER, sizeof( glIndex_t ),
				  numIndexes, &tess.indexRB );
	} else
#endif
	{
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, ibo->indexesSize, nullptr, GL_DYNAMIC_DRAW );
	}
	R_BindNullIBO();

	GL_CheckErrors();

	return ibo;
}

/*
============
R_CreateIBO2
============
*/
IBO_t *R_CreateStaticIBO( const char *name, glIndex_t *indexes, int numIndexes )
{
	IBO_t         *ibo;

	if ( !numIndexes )
	{
		return nullptr;
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	ibo = ( IBO_t * ) ri.Hunk_Alloc( sizeof( *ibo ), ha_pref::h_low );
	tr.ibos.push_back( ibo );

	Q_strncpyz( ibo->name, name, sizeof( ibo->name ) );

	ibo->indexesSize = numIndexes * sizeof( glIndex_t );
	ibo->indexesNum = numIndexes;

	glGenBuffers( 1, &ibo->indexesVBO );

	R_BindIBO( ibo );

#ifdef GL_ARB_buffer_storage
	if( glConfig2.bufferStorageAvailable ) {
		glBufferStorage( GL_ELEMENT_ARRAY_BUFFER, ibo->indexesSize, indexes, 0 );
	} else
#endif
	{
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, ibo->indexesSize, indexes, GL_STATIC_DRAW );
	}
	R_BindNullIBO();

	GL_CheckErrors();

	return ibo;
}

IBO_t *R_CreateStaticIBO2( const char *name, int numTriangles, glIndex_t *indexes )
{
	IBO_t         *ibo;

	if ( !numTriangles )
	{
		return nullptr;
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	ibo = ( IBO_t * ) ri.Hunk_Alloc( sizeof( *ibo ), ha_pref::h_low );
	tr.ibos.push_back( ibo );

	Q_strncpyz( ibo->name, name, sizeof( ibo->name ) );
	ibo->indexesNum = numTriangles * 3;
	ibo->indexesSize = ibo->indexesNum * sizeof( glIndex_t );

	glGenBuffers( 1, &ibo->indexesVBO );
	R_BindIBO( ibo );

#ifdef GL_ARB_buffer_storage
	if( glConfig2.bufferStorageAvailable ) {
		glBufferStorage( GL_ELEMENT_ARRAY_BUFFER, ibo->indexesSize,
				 indexes, 0 );
	} else
#endif
	{
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, ibo->indexesSize,
			      indexes, GL_STATIC_DRAW );
	}
	R_BindNullIBO();

	return ibo;
}

/*
============
R_BindVBO
============
*/
void R_BindVBO( VBO_t *vbo )
{
	if ( !vbo )
	{
		Sys::Drop( "R_BindNullVBO: NULL vbo" );
	}

	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment( va( "--- R_BindVBO( %s ) ---\n", vbo->name ) );
	}

	if ( glState.currentVBO != vbo )
	{
		glState.currentVBO = vbo;
		glState.vertexAttribPointersSet = 0;

		glState.vertexAttribsInterpolation = -1;
		glState.vertexAttribsOldFrame = 0;
		glState.vertexAttribsNewFrame = 0;

		glBindBuffer( GL_ARRAY_BUFFER, vbo->vertexesVBO );

		backEnd.pc.c_vboVertexBuffers++;
	}
}

/*
============
R_BindNullVBO
============
*/
void R_BindNullVBO()
{
	GLimp_LogComment( "--- R_BindNullVBO ---\n" );

	if ( glState.currentVBO )
	{
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		glState.currentVBO = nullptr;
	}

	GL_CheckErrors();
}

/*
============
R_BindIBO
============
*/
void R_BindIBO( IBO_t *ibo )
{
	if ( !ibo )
	{
		Sys::Drop( "R_BindIBO: NULL ibo" );
	}

	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment( va( "--- R_BindIBO( %s ) ---\n", ibo->name ) );
	}

	if ( glState.currentIBO != ibo )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo->indexesVBO );

		glState.currentIBO = ibo;

		backEnd.pc.c_vboIndexBuffers++;
	}
}

/*
============
R_BindNullIBO
============
*/
void R_BindNullIBO()
{
	GLimp_LogComment( "--- R_BindNullIBO ---\n" );

	if ( glState.currentIBO )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
		glState.currentIBO = nullptr;
		glState.vertexAttribPointersSet = 0;
	}
}

static void R_InitGenericVBOs() {
	// Min and max coordinates of the quad
	static const vec3_t min = { 0.0f, 0.0f, 0.0f };
	static const vec3_t max = { 1.0f, 1.0f, 0.0f };
	/*
		Quad is a static mesh with 4 vertices and 2 triangles

		 z
		 ^
		 |     1------2
		 |   y |      |
		 |  /  |      |
		 | /   |      |
		 |/    0------3
		 0---------->x
		   Verts:
		   0: 0.0 0.0 0.0
		   1: 0.0 1.0 0.0
		   2: 1.0 1.0 0.0
		   3: 1.0 0.0 0.0
		   Surfs:
		   0: 0 2 1 / 0 3 2
	*/

	drawSurf_t* genericQuad;
	genericQuad = ( drawSurf_t* ) ri.Hunk_Alloc( sizeof( *genericQuad ), ha_pref::h_low );
	genericQuad->entity = &tr.worldEntity;
	srfVBOMesh_t* surface;
	surface = ( srfVBOMesh_t* ) ri.Hunk_Alloc( sizeof( *surface ), ha_pref::h_low );
	surface->surfaceType = surfaceType_t::SF_VBO_MESH;
	surface->numVerts = 4;
	surface->numIndexes = 6;
	surface->firstIndex = 0;
	vec3_t v0 = { min[0], min[1], min[2] };
	vec3_t v1 = { min[0], max[1], min[2] };
	vec3_t v2 = { max[0], max[1], min[2] };
	vec3_t v3 = { max[0], min[1], min[2] };

	shaderVertex_t verts[4];
	VectorCopy( v0, verts[0].xyz );
	VectorCopy( v1, verts[1].xyz );
	VectorCopy( v2, verts[2].xyz );
	VectorCopy( v3, verts[3].xyz );
	
	for ( int i = 0; i < 4; i++ ) {
		verts[i].color = Color::White;
		verts[i].texCoords[0] = floatToHalf( i < 2 ? 0.0f : 1.0f );
		verts[i].texCoords[1] = floatToHalf( i > 0 && i < 3 ? 1.0f : 0.0f );
	}
	surface->vbo = R_CreateStaticVBO2( "generic_VBO", surface->numVerts, verts, ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR );

	glIndex_t indexes[6] = { 0, 2, 1,  0, 3, 2 }; // Front

	surface->ibo = R_CreateStaticIBO( "generic_IBO", indexes, surface->numIndexes );
	genericQuad->surface = ( surfaceType_t* ) surface;

	tr.genericQuad = genericQuad;
}

static void R_InitTileVBO()
{
	if ( !glConfig2.realtimeLighting )
	{
		return;
	}

	if ( r_realtimeLightingRenderer.Get() != Util::ordinal( realtimeLightingRenderer_t::TILED ) )
	{
		/* This computation is part of the tiled dynamic lighting renderer,
		it's better to not run it and save CPU cycles when such effects
		are disabled.

		There is no need to create vertex buffers that are only used by the
		tiled dynamic lighting renderer when this feature is disabled. */

		return;
	}

	R_SyncRenderThread();
	int       x, y, w, h;

	w = tr.depthtile2RenderImage->width;
	h = tr.depthtile2RenderImage->height;

	vboData_t data{};
	data.numVerts = w * h;

	data.xyz = ( vec3_t * ) ri.Hunk_AllocateTempMemory( data.numVerts * sizeof( *data.xyz ) );
	data.stf = ( vec2_t * ) ri.Hunk_AllocateTempMemory( data.numVerts * sizeof( *data.stf ) );

	for (y = 0; y < h; y++ ) {
		for (x = 0; x < w; x++ ) {
			VectorSet( data.xyz[ y * w + x ],
				   (2 * x - w + 1) * (1.0f / w),
				   (2 * y - h + 1) * (1.0f / h),
				   0.0f );
			Vector2Set( data.stf[ y * w + x ],
				    2 * x * glState.tileStep[ 0 ] + glState.tileStep[ 0 ] - 1.0f,
				    2 * y * glState.tileStep[ 1 ] + glState.tileStep[ 1 ] - 1.0f );
		}
	}

	tr.lighttileVBO = R_CreateStaticVBO( "lighttile_VBO", data, vboLayout_t::VBO_LAYOUT_XYST );

	ri.Hunk_FreeTempMemory( data.stf );
	ri.Hunk_FreeTempMemory( data.xyz );
}

const int vertexCapacity = DYN_BUFFER_SIZE / sizeof( shaderVertex_t );
const int indexCapacity = DYN_BUFFER_SIZE / sizeof( glIndex_t );

static void R_InitLightUBO()
{
	if ( !glConfig2.realtimeLighting )
	{
		return;
	}

	if( glConfig2.uniformBufferObjectAvailable ) {
		glGenBuffers( 1, &tr.dlightUBO );
		glBindBuffer( GL_UNIFORM_BUFFER, tr.dlightUBO );
		glBufferData( GL_UNIFORM_BUFFER, MAX_REF_LIGHTS * sizeof( shaderLight_t ), nullptr, GL_DYNAMIC_DRAW );
		glBindBuffer( GL_UNIFORM_BUFFER, 0 );
	} else {
		glGenBuffers( 1, &tr.dlightUBO );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, tr.dlightUBO );
		glBufferData( GL_PIXEL_UNPACK_BUFFER, MAX_REF_LIGHTS * sizeof( shaderLight_t ), nullptr, GL_DYNAMIC_DRAW );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
	}
}

static void R_InitMaterialBuffers() {
	if( glConfig2.materialSystemAvailable ) {
		materialsSSBO.GenBuffer();

		surfaceDescriptorsSSBO.GenBuffer();
		surfaceCommandsSSBO.GenBuffer();
		culledCommandsBuffer.GenBuffer();
		surfaceBatchesUBO.GenBuffer();
		atomicCommandCountersBuffer.GenBuffer();
		
		portalSurfacesSSBO.GenBuffer();

		if ( r_materialDebug.Get() ) {
			debugSSBO.GenBuffer();
		}
	}
}

/*
============
R_InitVBOs
============
*/
void R_InitVBOs()
{
	uint32_t attribs = ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT | ATTR_COLOR;

	Log::Debug("------- R_InitVBOs -------" );

	tr.vbos.reserve( 100 );
	tr.ibos.reserve( 100 );

	tess.vertsBuffer = ( shaderVertex_t * ) Com_Allocate_Aligned( 64, SHADER_MAX_VERTEXES * sizeof( shaderVertex_t ) );
	tess.indexesBuffer = ( glIndex_t * ) Com_Allocate_Aligned( 64, SHADER_MAX_INDEXES * sizeof( glIndex_t ) );

	if( glConfig2.mapBufferRangeAvailable ) {
		tess.vbo = R_CreateDynamicVBO( "tessVertexArray_VBO", vertexCapacity, attribs, vboLayout_t::VBO_LAYOUT_STATIC );

		tess.ibo = R_CreateDynamicIBO( "tessVertexArray_IBO", indexCapacity );
		tess.vertsWritten = tess.indexesWritten = 0;
	} else {
		// use glBufferSubData to update VBO
		tess.vbo = R_CreateDynamicVBO( "tessVertexArray_VBO", SHADER_MAX_VERTEXES, attribs, vboLayout_t::VBO_LAYOUT_STATIC );

		tess.ibo = R_CreateDynamicIBO( "tessVertexArray_IBO", SHADER_MAX_INDEXES );
	}

	R_InitGenericVBOs();

	R_InitTileVBO();

	// allocate a PBO for color grade map transfers
	glGenBuffers( 1, &tr.colorGradePBO );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, tr.colorGradePBO );
	glBufferData( GL_PIXEL_PACK_BUFFER,
		      REF_COLORGRADEMAP_STORE_SIZE * sizeof(u8vec4_t),
		      nullptr, GL_STREAM_COPY );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

	R_InitLightUBO();

	R_InitMaterialBuffers();

	GL_CheckErrors();
}

/*
============
R_ShutdownVBOs
============
*/
void R_ShutdownVBOs()
{
	Log::Debug("------- R_ShutdownVBOs -------" );

	if( !glConfig2.mapBufferRangeAvailable ) {
		// nothing
	}
#if defined( GL_ARB_buffer_storage ) && defined( GL_ARB_sync )
	else if( glConfig2.bufferStorageAvailable &&
		 glConfig2.syncAvailable ) {
		R_BindVBO( tess.vbo );
		R_ShutdownRingbuffer( GL_ARRAY_BUFFER, &tess.vertexRB );
		R_BindIBO( tess.ibo );
		R_ShutdownRingbuffer( GL_ELEMENT_ARRAY_BUFFER, &tess.indexRB );
	}
#endif
	else {
		if( tess.verts != nullptr && tess.verts != tess.vertsBuffer ) {
			R_BindVBO( tess.vbo );
			glUnmapBuffer( GL_ARRAY_BUFFER );
		}

		if( tess.indexes != nullptr && tess.indexes != tess.indexesBuffer ) {
			R_BindIBO( tess.ibo );
			glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );
		}
	}

	R_BindNullVBO();
	R_BindNullIBO();

	glDeleteBuffers( 1, &tr.colorGradePBO );

	for ( VBO_t *vbo : tr.vbos )
	{
		if ( vbo->vertexesVBO )
		{
			glDeleteBuffers( 1, &vbo->vertexesVBO );
		}
	}

	for ( IBO_t *ibo : tr.ibos )
	{
		if ( ibo->indexesVBO )
		{
			glDeleteBuffers( 1, &ibo->indexesVBO );
		}
	}

	tr.vbos.clear();
	tr.ibos.clear();

	Com_Free_Aligned( tess.vertsBuffer );
	Com_Free_Aligned( tess.indexesBuffer );

	if( glConfig2.uniformBufferObjectAvailable ) {
		glDeleteBuffers( 1, &tr.dlightUBO );
		tr.dlightUBO = 0;
	}

	if ( glConfig2.materialSystemAvailable ) {
		materialsSSBO.DelBuffer();

		surfaceDescriptorsSSBO.DelBuffer();
		surfaceCommandsSSBO.DelBuffer();
		culledCommandsBuffer.DelBuffer();
		surfaceBatchesUBO.DelBuffer();
		atomicCommandCountersBuffer.DelBuffer();

		portalSurfacesSSBO.DelBuffer();

		if ( r_materialDebug.Get() ) {
			debugSSBO.DelBuffer();
		}
	}

	tess.verts = tess.vertsBuffer = nullptr;
	tess.indexes = tess.indexesBuffer = nullptr;
}

/*
==============
Tess_MapVBOs

Map the default VBOs
Must be called before writing to tess.verts or tess.indexes
==============
*/
void Tess_MapVBOs( bool forceCPU ) {
	if( forceCPU || !glConfig2.mapBufferRangeAvailable ) {
		// use host buffers
		tess.verts = tess.vertsBuffer;
		tess.indexes = tess.indexesBuffer;

		return;
	}

	if( tess.verts == nullptr ) {
		R_BindVBO( tess.vbo );

#if defined( GL_ARB_buffer_storage ) && defined( GL_ARB_sync )
		if( glConfig2.bufferStorageAvailable &&
		    glConfig2.syncAvailable ) {
			GLsizei segmentEnd = (tess.vertexRB.activeSegment + 1) * tess.vertexRB.segmentElements;
			if( tess.vertsWritten + SHADER_MAX_VERTEXES > (unsigned) segmentEnd ) {
				tess.vertsWritten = R_RotateRingbuffer( &tess.vertexRB );
			}
			tess.verts = ( shaderVertex_t * )tess.vertexRB.baseAddr + tess.vertsWritten;
		} else
#endif
		{
			if( vertexCapacity - tess.vertsWritten < SHADER_MAX_VERTEXES ) {
				// buffer is full, allocate a new one
				glBufferData( GL_ARRAY_BUFFER, vertexCapacity * sizeof( shaderVertex_t ), nullptr, GL_DYNAMIC_DRAW );
				tess.vertsWritten = 0;
			}
			tess.verts = ( shaderVertex_t *) glMapBufferRange( 
				GL_ARRAY_BUFFER, tess.vertsWritten * sizeof( shaderVertex_t ),
				SHADER_MAX_VERTEXES * sizeof( shaderVertex_t ),
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT |
				GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT );
		}
	}

	if( tess.indexes == nullptr ) {
		R_BindIBO( tess.ibo );

#if defined( GL_ARB_buffer_storage ) && defined( GL_ARB_sync )
		if( glConfig2.bufferStorageAvailable &&
		    glConfig2.syncAvailable ) {
			GLsizei segmentEnd = (tess.indexRB.activeSegment + 1) * tess.indexRB.segmentElements;
			if( tess.indexesWritten + SHADER_MAX_INDEXES > (unsigned) segmentEnd ) {
				tess.indexesWritten = R_RotateRingbuffer( &tess.indexRB );
			}
			tess.indexes = ( glIndex_t * )tess.indexRB.baseAddr + tess.indexesWritten;
		} else
#endif
		{
			if( indexCapacity - tess.indexesWritten < SHADER_MAX_INDEXES ) {
				// buffer is full, allocate a new one
				glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexCapacity * sizeof( glIndex_t ), nullptr, GL_DYNAMIC_DRAW );
				tess.indexesWritten = 0;
			}
			tess.indexes = ( glIndex_t *) glMapBufferRange( 
				GL_ELEMENT_ARRAY_BUFFER, tess.indexesWritten * sizeof( glIndex_t ),
				SHADER_MAX_INDEXES * sizeof( glIndex_t ),
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT |
				GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT );
		}
	}
}

/*
==============
Tess_UpdateVBOs

Tr3B: update the default VBO to replace the client side vertex arrays
==============
*/
void Tess_UpdateVBOs()
{
	GLimp_LogComment( "--- Tess_UpdateVBOs( ) ---\n" );

	GL_CheckErrors();

	// update the default VBO
	if ( tess.numVertexes > 0 && tess.numVertexes <= SHADER_MAX_VERTEXES )
	{
		GLsizei size = tess.numVertexes * sizeof( shaderVertex_t );

		GL_CheckErrors();

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "glBufferSubData( vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
		}

		if( !glConfig2.mapBufferRangeAvailable ) {
			R_BindVBO( tess.vbo );
			glBufferSubData( GL_ARRAY_BUFFER, 0, size, tess.verts );
		} else {
			R_BindVBO( tess.vbo );
			if( glConfig2.bufferStorageAvailable &&
			    glConfig2.syncAvailable ) {
				GLsizei offset = tess.vertexBase * sizeof( shaderVertex_t );

				glFlushMappedBufferRange( GL_ARRAY_BUFFER,
							  offset, size );
			} else {
				glFlushMappedBufferRange( GL_ARRAY_BUFFER,
							  0, size );
				glUnmapBuffer( GL_ARRAY_BUFFER );
			}
			tess.vertexBase = tess.vertsWritten;
			tess.vertsWritten += tess.numVertexes;

			tess.verts = nullptr;
		}
	}

	GL_CheckErrors();

	// update the default IBO
	if ( tess.numIndexes > 0 && tess.numIndexes <= SHADER_MAX_INDEXES )
	{
		GLsizei size = tess.numIndexes * sizeof( glIndex_t );

		if( !glConfig2.mapBufferRangeAvailable ) {
			R_BindIBO( tess.ibo );
			glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, 0, size,
					 tess.indexes );
		} else {
			R_BindIBO( tess.ibo );

			if( glConfig2.bufferStorageAvailable &&
			    glConfig2.syncAvailable ) {
				GLsizei offset = tess.indexBase * sizeof( glIndex_t );

				glFlushMappedBufferRange( GL_ELEMENT_ARRAY_BUFFER,
							  offset, size );
			} else {
				glFlushMappedBufferRange( GL_ELEMENT_ARRAY_BUFFER,
							  0, size );
				glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );
			}
			tess.indexBase = tess.indexesWritten;
			tess.indexesWritten += tess.numIndexes;

			tess.indexes = nullptr;
		}
	}

	GL_CheckErrors();
}

/*
============
R_ListVBOs_f
============
*/
void R_ListVBOs_f()
{
	int   vertexesSize = 0;
	int   indexesSize = 0;

	Log::Notice(" size          name" );
	Log::Notice("----------------------------------------------------------" );

	for ( VBO_t *vbo : tr.vbos )
	{
		Log::Notice("%d.%02d MB %s", vbo->vertexesSize / ( 1024 * 1024 ),
		           ( vbo->vertexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ), vbo->name );

		vertexesSize += vbo->vertexesSize;
	}

	for ( IBO_t *ibo : tr.ibos)
	{
		Log::Notice("%d.%02d MB %s", ibo->indexesSize / ( 1024 * 1024 ),
		           ( ibo->indexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ), ibo->name );

		indexesSize += ibo->indexesSize;
	}

	Log::Notice(" %i total VBOs", tr.vbos.size() );
	Log::Notice(" %d.%02d MB total vertices memory", vertexesSize / ( 1024 * 1024 ),
	           ( vertexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );

	Log::Notice(" %i total IBOs", tr.ibos.size() );
	Log::Notice(" %d.%02d MB total triangle indices memory", indexesSize / ( 1024 * 1024 ),
	           ( indexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
}
