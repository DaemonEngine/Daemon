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
// VBO.h

#ifndef VBO_H
#define VBO_H

#include "common/Common.h"
#include "GL/glew.h"

#include "VertexSpecification.h"
#include "tr_types.h"

struct VBO_t
{
	char name[96]; // only for debugging with /listVBOs

	uint32_t vertexesVBO;

	uint32_t vertexesSize; // total amount of memory data allocated for this vbo

	uint32_t vertexesNum;
	uint32_t framesNum; // number of frames for vertex animation

	std::array<vboAttributeLayout_t, ATTR_INDEX_MAX> attribs; // info for buffer manipulation

	vboLayout_t layout;
	uint32_t attribBits; // Which attributes it has. Mostly for detecting errors
	GLenum usage;
};

struct IBO_t
{
	char name[96]; // only for debugging with /listVBOs

	uint32_t indexesVBO;
	uint32_t indexesSize; // amount of memory data allocated for all triangles in bytes
	uint32_t indexesNum;
};

void R_CopyVertexAttribute( const vboAttributeLayout_t& attrib, const vertexAttributeSpec_t& spec,
	uint32_t count, byte* interleavedData );

VBO_t* R_CreateStaticVBO(
	Str::StringRef name, const vertexAttributeSpec_t* attrBegin, const vertexAttributeSpec_t* attrEnd,
	uint32_t numVerts, uint32_t numFrames = 0 );

IBO_t* R_CreateStaticIBO( const char* name, glIndex_t* indexes, int numIndexes );

void  R_BindVBO( VBO_t* vbo );
void  R_BindNullVBO();

void  R_BindIBO( IBO_t* ibo );
void  R_BindNullIBO();

void  R_InitVBOs();
void  R_ShutdownVBOs();

#endif // GLMEMORY_H
