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
// VertexSpecification.h

#ifndef VERTEX_SPECIFICATION_H
#define VERTEX_SPECIFICATION_H

#include "common/Common.h"
#include "GL/glew.h"

enum
{
	ATTR_INDEX_POSITION = 0,
	ATTR_INDEX_TEXCOORD, // TODO split into 2-element texcoords and 4-element tex + lm coords
	ATTR_INDEX_QTANGENT,
	ATTR_INDEX_COLOR,

	// GPU vertex skinning
	ATTR_INDEX_BONE_FACTORS,

	// GPU vertex animations
	ATTR_INDEX_POSITION2,
	ATTR_INDEX_QTANGENT2,

	ATTR_INDEX_FOG_SURFACE,

	// This occupies 5 attribute slots
	ATTR_INDEX_FOG_PLANES_0,
	ATTR_INDEX_FOG_PLANES_LAST = ATTR_INDEX_FOG_PLANES_0 + 4,

	ATTR_INDEX_MAX
};

// must match order of ATTR_INDEX enums
static const char* const attributeNames[] =
{
	"attr_Position",
	"attr_TexCoord0",
	"attr_QTangent",
	"attr_Color",
	"attr_BoneFactors",
	"attr_Position2",
	"attr_QTangent2",
	"attr_FogSurface",
	"attr_FogPlanes", nullptr, nullptr, nullptr, nullptr,
};

enum
{
  ATTR_POSITION       = BIT( ATTR_INDEX_POSITION ),
  ATTR_TEXCOORD       = BIT( ATTR_INDEX_TEXCOORD ),
  ATTR_QTANGENT       = BIT( ATTR_INDEX_QTANGENT ),
  ATTR_COLOR          = BIT( ATTR_INDEX_COLOR ),

  ATTR_BONE_FACTORS   = BIT( ATTR_INDEX_BONE_FACTORS ),

  // for .md3 interpolation
  ATTR_POSITION2      = BIT( ATTR_INDEX_POSITION2 ),
  ATTR_QTANGENT2      = BIT( ATTR_INDEX_QTANGENT2 ),

  ATTR_FOG_SURFACE    = BIT( ATTR_INDEX_FOG_SURFACE ),
  ATTR_FOG_PLANES     = BIT( ATTR_INDEX_FOG_PLANES_0 ) * ( BIT( 5 ) - 1 ),

  ATTR_INTERP_BITS = ATTR_POSITION2 | ATTR_QTANGENT2,
};

struct vboAttributeLayout_t
{
	GLint   numComponents; // how many components in a single attribute for a single vertex
	GLenum  componentType; // the input type for a single component
	GLboolean normalize; // convert signed integers to the floating point range [-1, 1], and unsigned integers to the range [0, 1]
	GLsizei stride;
	GLsizei ofs;
	GLsizei frameOffset; // for vertex animation, real offset computed as ofs + frame * frameOffset
};

enum class vboLayout_t
{
	VBO_LAYOUT_CUSTOM,
	VBO_LAYOUT_STATIC,
};

enum
{
	ATTR_OPTION_NORMALIZE = BIT( 0 ),
	ATTR_OPTION_HAS_FRAMES = BIT( 1 ),
};

struct vertexAttributeSpec_t
{
	int attrIndex;
	GLenum componentInputType;
	GLenum componentStorageType;
	const void* begin;
	uint32_t numComponents;
	uint32_t stride;
	int attrOptions;
};

uint32_t R_ComponentSize( GLenum type );

#endif // VERTEX_SPECIFICATION_H
