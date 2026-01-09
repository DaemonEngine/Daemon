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
// GLMemory.h

#ifndef GLMEMORY_H
#define GLMEMORY_H

#include "common/Common.h"

#include <GL/glew.h>

#include "BufferBind.h"
#include "GLUtils.h"
#include "VertexSpecification.h"

class GLBuffer {
	public:
	friend class GLVAO;

	std::string name;
	const GLuint64 SYNC_TIMEOUT = 10000000000; // 10 seconds

	GLuint id = 0;

	GLBuffer( const char* newName, const GLuint newBindingPoint, const GLbitfield newFlags, const GLbitfield newMapFlags ) :
		name( newName ),
		internalTarget( 0 ),
		internalBindingPoint( newBindingPoint ),
		flags( newFlags ),
		mapFlags( newMapFlags ) {
	}

	GLBuffer( const char* newName, const GLenum newTarget, const GLuint newBindingPoint,
		const GLbitfield newFlags, const GLbitfield newMapFlags ) :
		name( newName ),
		internalTarget( newTarget ),
		internalBindingPoint( newBindingPoint ),
		flags( newFlags ),
		mapFlags( newMapFlags ) {
	}

	void BindBufferBase( GLenum target = 0, GLuint bindingPoint = 0 ) {
		target = target ? target : internalTarget;
		bindingPoint = bindingPoint ? bindingPoint : internalBindingPoint;
		glBindBufferBase( target, bindingPoint, id );
	}

	void UnBindBufferBase( GLenum target = 0, GLuint bindingPoint = 0 ) {
		target = target ? target : internalTarget;
		bindingPoint = bindingPoint ? bindingPoint : internalBindingPoint;
		glBindBufferBase( target, bindingPoint, 0 );
	}

	void BindBuffer( GLenum target = 0 ) {
		target = target ? target : internalTarget;
		glBindBuffer( target, id );
	}

	void UnBindBuffer( GLenum target = 0 ) {
		target = target ? target : internalTarget;
		glBindBuffer( target, 0 );
	}

	void BufferData( const GLsizeiptr size, const void* data, const GLenum usageFlags ) {
		glNamedBufferData( id, size * sizeof( uint32_t ), data, usageFlags );
	}

	void BufferStorage( const GLsizeiptr newAreaSize, const GLsizeiptr areaCount, const void* data ) {
		areaSize = newAreaSize;
		maxAreas = areaCount;
		glNamedBufferStorage( id, areaSize * areaCount * sizeof( uint32_t ), data, flags );
		syncs.resize( areaCount );

		GL_CheckErrors();
	}

	void AreaIncr() {
		syncs[area] = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
		area++;
		if ( area >= maxAreas ) {
			area = 0;
		}
	}

	void MapAll() {
		if ( !mapped ) {
			mapped = true;
			data = ( uint32_t* ) glMapNamedBufferRange( id, 0, areaSize * maxAreas * sizeof( uint32_t ), flags | mapFlags );
		}
	}

	uint32_t* GetCurrentAreaData() {
		if ( syncs[area] != nullptr ) {
			if ( glClientWaitSync( syncs[area], GL_SYNC_FLUSH_COMMANDS_BIT, SYNC_TIMEOUT ) == GL_TIMEOUT_EXPIRED ) {
				Sys::Drop( "Failed buffer %s area %u sync", name, area );
			}
			glDeleteSync( syncs[area] );
		}

		return data + area * areaSize;
	}

	uint32_t* GetData() {
		return data;
	}

	void FlushCurrentArea() {
		glFlushMappedNamedBufferRange( id, area * areaSize * sizeof( uint32_t ), areaSize * sizeof( uint32_t ) );
	}

	void FlushAll() {
		glFlushMappedNamedBufferRange( id, 0, maxAreas * areaSize * sizeof( uint32_t ) );
	}

	void FlushRange( const GLsizeiptr offset, const GLsizeiptr size ) {
		glFlushMappedNamedBufferRange( id, offset * sizeof( uint32_t ), size * sizeof( uint32_t ) );
	}

	uint32_t* MapBufferRange( const GLuint count ) {
		return MapBufferRange( 0, count );
	}

	uint32_t* MapBufferRange( const GLuint offset, const GLuint count ) {
		if ( !mapped ) {
			mapped = true;
			data = ( uint32_t* ) glMapNamedBufferRange( id,
				offset * sizeof( uint32_t ), count * sizeof( uint32_t ),
				flags | mapFlags );
		}

		return data;
	}

	void UnmapBuffer() {
		if ( mapped ) {
			mapped = false;
			glUnmapNamedBuffer( id );
		}
	}

	void GenBuffer() {
		glCreateBuffers( 1, &id );
	}

	void DelBuffer() {
		glDeleteBuffers( 1, &id );
		id = 0;
		mapped = false;
	}

	private:
	const GLenum internalTarget;
	const GLuint internalBindingPoint;

	bool mapped = false;
	const GLbitfield flags;
	const GLbitfield mapFlags;

	std::vector<GLsync> syncs;
	GLsizeiptr area = 0;
	GLsizeiptr areaSize = 0;
	GLsizeiptr maxAreas = 0;
	uint32_t* data;
};

// Shorthands for buffers that are only bound to one specific target
class GLSSBO : public GLBuffer {
	public:
	GLSSBO( const char* name, const GLuint bindingPoint, const GLbitfield flags, const GLbitfield mapFlags ) :
		GLBuffer( name, GL_SHADER_STORAGE_BUFFER, bindingPoint, flags, mapFlags ) {
	}
};

class GLUBO : public GLBuffer {
	public:
	GLUBO( const char* name, const GLsizeiptr bindingPoint, const GLbitfield flags, const GLbitfield mapFlags ) :
		GLBuffer( name, GL_UNIFORM_BUFFER, bindingPoint, flags, mapFlags ) {
	}
};

class GLAtomicCounterBuffer : public GLBuffer {
	public:
	GLAtomicCounterBuffer( const char* name, const GLsizeiptr bindingPoint, const GLbitfield flags, const GLbitfield mapFlags ) :
		GLBuffer( name, GL_ATOMIC_COUNTER_BUFFER, bindingPoint, flags, mapFlags ) {
	}
};

class GLVAO {
	public:
	vboAttributeLayout_t attrs[ATTR_INDEX_MAX];
	uint32_t enabledAttrs;

	GLVAO( const GLuint newVBOBindingPoint = 0 ) :
		VBOBindingPoint( newVBOBindingPoint ) {
	}

	~GLVAO() = default;

	void Bind() {
		GL_BindVAO( id );
	}

	void SetAttrs( const vertexAttributeSpec_t* attrBegin, const vertexAttributeSpec_t* attrEnd ) {
		uint32_t ofs = 0;
		for ( const vertexAttributeSpec_t* spec = attrBegin; spec < attrEnd; spec++ ) {
			vboAttributeLayout_t& attr = attrs[spec->attrIndex];
			DAEMON_ASSERT_NQ( spec->numComponents, 0U );
			attr.componentType = spec->componentStorageType;
			if ( attr.componentType == GL_HALF_FLOAT && !glConfig.halfFloatVertexAvailable ) {
				attr.componentType = GL_FLOAT;
			}
			attr.numComponents = spec->numComponents;
			attr.normalize = spec->attrOptions & ATTR_OPTION_NORMALIZE ? GL_TRUE : GL_FALSE;

			attr.ofs = ofs;
			ofs += attr.numComponents * R_ComponentSize( attr.componentType );
			ofs = ( ofs + 3 ) & ~3; // 4 is minimum alignment for any vertex attribute

			enabledAttrs |= 1 << spec->attrIndex;
		}

		stride = ofs;

		for ( const vertexAttributeSpec_t* spec = attrBegin; spec < attrEnd; spec++ ) {
			const int index = spec->attrIndex;
			vboAttributeLayout_t& attr = attrs[index];

			attr.stride = stride;

			glEnableVertexArrayAttrib( id, index );
			glVertexArrayAttribFormat( id, index, attr.numComponents, attr.componentType,
				attr.normalize, attr.ofs );
			glVertexArrayAttribBinding( id, index, VBOBindingPoint );
		}
	}

	void SetVertexBuffer( const GLBuffer buffer, const GLuint offset ) {
		glVertexArrayVertexBuffer( id, VBOBindingPoint, buffer.id, offset, stride );
	}

	void SetIndexBuffer( const GLBuffer buffer ) {
		glVertexArrayElementBuffer( id, buffer.id );
	}

	void GenVAO() {
		glGenVertexArrays( 1, &id );
	}

	void DelVAO() {
		glDeleteVertexArrays( 1, &id );
	}

	private:
	GLuint id;
	GLuint VBOBindingPoint;
	GLuint stride;
};

void GLBufferCopy( GLBuffer* src, GLBuffer* dst, GLintptr srcOffset, GLintptr dstOffset, GLsizeiptr size );

struct GLStagingCopy {
	GLBuffer* dst;
	GLsizeiptr stagingOffset;
	GLsizeiptr dstOffset;
	GLsizeiptr size;
};

class GLStagingBuffer {
	public:
	uint32_t* MapBuffer( const GLsizeiptr size );
	void FlushBuffer();
	void QueueStagingCopy( GLBuffer* dst, const GLsizeiptr dstOffset );
	void FlushStagingCopyQueue();
	void FlushAll();

	bool Active() const;

	void InitGLBuffer();
	void FreeGLBuffer();

	private:
	static const GLsizeiptr SIZE;

	GLsizeiptr pointer = 0;
	GLsizeiptr current = 0;
	GLsizeiptr last = 0;

	static const uint32_t MAX_COPIES = 16;
	GLStagingCopy copyQueue[MAX_COPIES];
	uint32_t currentCopy = 0;

	GLBuffer buffer = GLBuffer( "staging", BufferBind::STAGING, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT,
		GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_INVALIDATE_BUFFER_BIT );
};

struct PushBuffer {
	uint32_t constUniformsSize;
	uint32_t frameUniformsSize;
	uint32_t globalUBOSize;
	uint32_t* globalUBOData;

	GLUBO globalUBO = GLUBO( "globalUniforms", BufferBind::GLOBAL_DATA, 0, 0 );

	void InitGLBuffers();
	void FreeGLBuffers();

	uint32_t* MapGlobalUniformData( const int updateType );
	void PushGlobalUniforms();
};

extern GLStagingBuffer stagingBuffer;
extern PushBuffer pushBuffer;

#endif // GLMEMORY_H