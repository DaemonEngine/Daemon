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

#include "gl_shader.h"

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

extern GLStagingBuffer stagingBuffer;

#endif // GLMEMORY_H
