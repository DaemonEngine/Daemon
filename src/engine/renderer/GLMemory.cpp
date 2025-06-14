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
// GLMemory.cpp

#include "common/Common.h"

#include "GLMemory.h"

// 128 MB, should be enough to fit anything in BAR without going overboard
const GLsizeiptr GLStagingBuffer::SIZE = 128 * 1024 * 1024 / sizeof( uint32_t );

GLStagingBuffer stagingBuffer;

void GLBufferCopy( GLBuffer* src, GLBuffer* dst, GLintptr srcOffset, GLintptr dstOffset, GLsizeiptr size ) {
	glCopyNamedBufferSubData( src->id, dst->id,
		srcOffset * sizeof( uint32_t ), dstOffset * sizeof( uint32_t ), size * sizeof( uint32_t ) );
}

uint32_t* GLStagingBuffer::MapBuffer( const GLsizeiptr size ) {
	if ( size > SIZE ) {
		Sys::Drop( "Couldn't map GL staging buffer: size too large (%u/%u)", size, SIZE );
	}

	if ( pointer + size > SIZE ) {
		FlushAll();

		GLsync sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );

		constexpr GLuint64 SYNC_TIMEOUT = 10000000000; // 10 seconds
		if ( glClientWaitSync( sync, GL_SYNC_FLUSH_COMMANDS_BIT, SYNC_TIMEOUT ) == GL_TIMEOUT_EXPIRED ) {
			Sys::Drop( "Failed GL staging buffer copy sync" );
		}
		glDeleteSync( sync );

		pointer = 0;
		current = 0;
		last = 0;
	}

	uint32_t* ret = buffer.GetData() + pointer;
	last = pointer;
	pointer += size;

	return ret;
}

void GLStagingBuffer::FlushBuffer() {
	buffer.FlushRange( current, pointer - current );

	GL_CheckErrors();

	current = pointer;
}

void GLStagingBuffer::QueueStagingCopy( GLBuffer* dst, const GLsizeiptr dstOffset ) {
	copyQueue[currentCopy].dst = dst;
	copyQueue[currentCopy].dstOffset = dstOffset;
	copyQueue[currentCopy].stagingOffset = last;
	copyQueue[currentCopy].size = pointer - last;

	currentCopy++;

	if ( currentCopy == MAX_COPIES ) {
		FlushStagingCopyQueue();
	}
}

void GLStagingBuffer::FlushStagingCopyQueue() {
	for ( GLStagingCopy& copy : copyQueue ) {
		if ( copy.dst ) {
			GLBufferCopy( &buffer, copy.dst, copy.stagingOffset, copy.dstOffset, copy.size );
			copy.dst = nullptr;
		}
	}

	currentCopy = 0;

	GL_CheckErrors();
}

void GLStagingBuffer::FlushAll() {
	FlushBuffer();
	FlushStagingCopyQueue();
}

void GLStagingBuffer::InitGLBuffer() {
	buffer.GenBuffer();

	buffer.BufferStorage( SIZE, 1, nullptr );
	buffer.MapAll();

	GL_CheckErrors();
}

void GLStagingBuffer::FreeGLBuffer() {
	buffer.DelBuffer();
}
