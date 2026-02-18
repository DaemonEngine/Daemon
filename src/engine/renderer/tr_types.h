/*
===========================================================================

Daemon GPL Source Code

Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2010 Robert Beckebans

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// This file is part of the VM ABI. Changes here may cause incompatibilities.

#ifndef __TR_TYPES_H
#define __TR_TYPES_H

// bool can't be safely deserialized by memcpy
#pragma push_macro("bool")
#undef bool
#define bool DO_NOT_USE_BOOL_IN_IPC_MESSAGE_TYPES
using bool8_t = uint8_t;

// TODO(0.56): Remove all shadow stuff in here?

// XreaL BEGIN
#define MAX_REF_LIGHTS     1024
#define MAX_REF_ENTITIES   8191 // can't be increased without changing drawsurf bit packing
#define MAX_BONES          256
#define MAX_WEIGHTS        4 // GPU vertex skinning limit, never change this without rewriting many GLSL shaders
// XreaL END

#define MAX_ENTITIES       MAX_REF_ENTITIES // RB: for compatibility

// renderfx flags
#define RF_THIRD_PERSON    0x000002 // don't draw through eyes, only mirrors (player bodies, chat sprites)
#define RF_FIRST_PERSON    0x000004 // only draw through eyes (view weapon, damage blood blob)
#define RF_DEPTHHACK       0x000008 // for view weapon Z crunching
#define RF_NOSHADOW        0x000010 // don't add stencil shadows

#define RF_SWAPCULL      0x000040 // swap CT_FRONT_SIDED and CT_BACK_SIDED

// refdef flags
#define RDF_NOWORLDMODEL ( 1 << 0 ) // used for player configuration screen
#define RDF_HYPERSPACE   ( 1 << 1 ) // teleportation effect

// XreaL BEGIN
#define RDF_NOCUBEMAP    ( 1 << 3 ) // RB: don't use cubemaps
#define RDF_NOBLOOM      ( 1 << 4 ) // RB: disable bloom. useful for HUD models
// XreaL END

#define MAX_ALTSHADERS   64 // alternative shaders ('when <condition> <shader>') – selection controlled from cgame

#define GL_INDEX_TYPE GL_UNSIGNED_INT
using glIndex_t = unsigned int;

// "[implicit only]" means the flag only has effect if there is no shader text and the
// shader was auto-generated from an image.
enum RegisterShaderFlags_t {
	// nothing
	RSF_DEFAULT = BIT( 0 ),

	// [implicit only] alter filter and wrap type
	RSF_2D = BIT( 1 ),

	// load images without mipmaps
	RSF_NOMIP = BIT( 2 ),

	// mip images to the screen size when they are larger than the screen
	RSF_FITSCREEN = BIT( 3 ),

	// used to make particles/trails work with the lightGrid in GLSL
	RSF_FORCE_LIGHTMAP = BIT( 5 ),

	// when the shader is used on an entity sprite, face view direction instead of viewer
	RSF_SPRITE = BIT( 6 ),
};

struct polyVert_t
{
	vec3_t xyz;
	float  st[ 2 ];
	byte   modulate[ 4 ];
};

struct poly_t
{
	qhandle_t  hShader;
	int        numVerts;
	polyVert_t *verts;
};

enum class refEntityType_t
{
  RT_MODEL,

  // A square 3D polygon at the specified `origin` with size `radius` with its face
  // oriented toward the viewer. By default the square's edges will be aligned so they are
  // drawn parallel to the edges of the screen; this can be changed by setting `rotation`.
  RT_SPRITE,

  RT_PORTALSURFACE, // doesn't draw anything, just info for portals

  RT_MAX_REF_ENTITY_TYPE
};

// RB: having bone names for each refEntity_t takes several MiBs
// in backEndData_t so only use it for debugging and development
// enabling this will show the bone names with r_showSkeleton 1

struct refBone_t
{
#if defined( REFBONE_NAMES )
	char   name[ 64 ];
#endif
	short  parentIndex; // parent index (-1 if root)
	transform_t t;
};

enum class refSkeletonType_t
{
  SK_INVALID,
  SK_RELATIVE,
  SK_ABSOLUTE
};

struct alignas(16) refSkeleton_t
{
	refSkeletonType_t type; // skeleton has been reset

	unsigned short numBones;

	vec3_t            bounds[ 2 ]; // bounds of all applied animations
	vec_t             scale;

	refBone_t         bones[ MAX_BONES ];
};

// XreaL END

struct refEntity_t
{
	refEntityType_t reType;
	int             renderfx;

	qhandle_t       hModel; // opaque type outside refresh

	// most recent data
	vec3_t    axis[ 3 ]; // rotation vectors
	bool8_t  nonNormalizedAxes; // axis are not normalized, i.e. they have scale
	vec3_t    origin;
	int       frame;

	// previous data for frame interpolation
	vec3_t    oldorigin; // also used as MODEL_BEAM's "to"
	int       oldframe;
	float     backlerp; // 0.0 = current, 1.0 = old

	// texturing
	int       skinNum; // inline skin index
	qhandle_t customSkin; // nullptr for default skin
	qhandle_t customShader; // use one image for the entire thing

	// misc
	Color::Color32Bit shaderRGBA; // colors used by rgbgen entity shaders
	float shaderTexCoord[ 2 ]; // texture coordinates used by tcMod entity modifiers
	float shaderTime; // subtracted from refdef time to control effect start times

	// extra sprite information
	float radius;
	float rotation;

	int altShaderIndex;

	// KEEP SKELETON AT THE END OF THE STRUCTURE
	// it is to make a serialization hack for refEntity_t easier
	// by memcpying up to skeleton and then serializing skeleton
	refSkeleton_t skeleton;

};

// ================================================================================================

struct refdef_t
{
	int    x, y, width, height;
	float  fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[ 3 ]; // transformation matrix
	vec3_t blurVec;       // motion blur direction

	int    time; // time in milliseconds for shader effects and other time dependent rendering issues
	int    rdflags; // RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[ MAX_MAP_AREA_BYTES ];

	vec4_t  gradingWeights;
};

// XreaL BEGIN

// cg_shadows modes
enum class shadowingMode_t
{
  SHADOWING_NONE,
  SHADOWING_BLOB,
  SHADOWING_ESM16,
  SHADOWING_ESM32,
  SHADOWING_VSM16,
  SHADOWING_VSM32,
  SHADOWING_EVSM32,
};
// XreaL END

enum class textureCompression_t
{
  TC_NONE,
  TC_S3TC,
  TC_EXT_COMP_S3TC
};

// TODO(0.56): remove.
enum class glDriverType_t
{
  GLDRV_UNKNOWN = -1,
  GLDRV_ICD,
  GLDRV_STANDALONE,
  GLDRV_OPENGL3,
};

// Keep the list in sdl_glimp.c:reportHardwareType in sync with this
enum class glHardwareType_t
{
  GLHW_UNKNOWN = -1,
  GLHW_GENERIC, // where everthing works the way it should

// XreaL BEGIN
  GLHW_R300, // pre-GL3 ATI hack
// XreaL END
};

/**
 * Contains variables specific to the OpenGL configuration
 * being run right now. These are constant once the OpenGL
 * subsystem is initialized.
 */
struct WindowConfig {
	float displayAspect;
	int displayWidth, displayHeight; // the entire monitor (the one indicated by displayIndex)
	int vidWidth, vidHeight; // what the game is using
};

#pragma pop_macro("bool")

#endif // __TR_TYPES_H
