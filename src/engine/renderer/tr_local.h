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

#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#include <GL/glew.h>

#include "common/FileSystem.h"
#include "qcommon/q_shared.h"
#include "qcommon/qfiles.h"
#include "qcommon/qcommon.h"
#include "botlib/bot_debug.h"
#include "engine/RefAPI.h"
#include "GLUtils.h"
#include "iqm.h"
#include "DetectGLVendors.h"
#include "TextureManager.h"
#include "VBO.h"

#define DYN_BUFFER_SIZE ( 4 * 1024 * 1024 )
#define DYN_BUFFER_SEGMENTS 4
#define BUFFER_OFFSET(i) (reinterpret_cast<const void*>( i ))

using i8vec4_t = int8_t[4];
using u8vec4_t = uint8_t[4];
using i16vec4_t = int16_t[4];
using u16vec4_t = uint16_t[4];
using i16vec2_t = int16_t[2];
using u16vec2_t = uint16_t[2];

// The struct has the same memory layout as a half-float
struct f16_t
{
	uint16_t bits;
};

using f16vec2_t = f16_t[2]; // half float vector
using f16vec4_t = f16_t[4]; // half float vector

// GL conversion helpers
static inline float unorm8ToFloat(byte unorm8) {
	return unorm8 * (1.0f / 255.0f);
}
static inline byte floatToUnorm8(float f) {
	// don't use Q_ftol here, as the semantics of Q_ftol
	// has been changed from round to nearest to round to 0 !
	return lrintf(f * 255.0f);
}
static inline float snorm8ToFloat(int8_t snorm8) {
	return std::max( snorm8 * (1.0f / 127.0f), -1.0f);
}
static inline int8_t floatToSnorm8(float f) {
	// don't use Q_ftol here, as the semantics of Q_ftol
	// has been changed from round to nearest to round to 0 !
	return lrintf(f * 127.0f);
}

static inline float unorm16ToFloat(uint16_t unorm16) {
	return unorm16 * (1.0f / 65535.0f);
}
static inline uint16_t floatToUnorm16(float f) {
	// don't use Q_ftol here, as the semantics of Q_ftol
	// has been changed from round to nearest to round to 0 !
	return lrintf(f * 65535.0f);
}
static inline float snorm16ToFloat(int16_t snorm16) {
	return std::max( snorm16 * (1.0f / 32767.0f), -1.0f);
}
static inline int16_t floatToSnorm16(float f) {
	// don't use Q_ftol here, as the semantics of Q_ftol
	// has been changed from round to nearest to round to 0 !
	return lrintf(f * 32767.0f);
}


static inline void floatToUnorm8( const vec4_t in, u8vec4_t out )
{
	out[ 0 ] = floatToUnorm8( in[ 0 ] );
	out[ 1 ] = floatToUnorm8( in[ 1 ] );
	out[ 2 ] = floatToUnorm8( in[ 2 ] );
	out[ 3 ] = floatToUnorm8( in[ 3 ] );
}

static inline void unorm8ToFloat( const u8vec4_t in, vec4_t out )
{
	out[ 0 ] = unorm8ToFloat( in[ 0 ] );
	out[ 1 ] = unorm8ToFloat( in[ 1 ] );
	out[ 2 ] = unorm8ToFloat( in[ 2 ] );
	out[ 3 ] = unorm8ToFloat( in[ 3 ] );
}

static inline void floatToSnorm16( const vec4_t in, i16vec4_t out )
{
	out[ 0 ] = floatToSnorm16( in[ 0 ] );
	out[ 1 ] = floatToSnorm16( in[ 1 ] );
	out[ 2 ] = floatToSnorm16( in[ 2 ] );
	out[ 3 ] = floatToSnorm16( in[ 3 ] );
}

static inline void floatToSnorm16_fast( const vec4_t in, i16vec4_t out )
{
	// Just truncate them all.
	out[ 0 ] = in[ 0 ] * 32767.0f;
	out[ 1 ] = in[ 1 ] * 32767.0f;
	out[ 2 ] = in[ 2 ] * 32767.0f;
	out[ 3 ] = in[ 3 ] * 32767.0f;
}

static inline void snorm16ToFloat( const i16vec4_t in, vec4_t out )
{
	out[ 0 ] = snorm16ToFloat( in[ 0 ] );
	out[ 1 ] = snorm16ToFloat( in[ 1 ] );
	out[ 2 ] = snorm16ToFloat( in[ 2 ] );
	out[ 3 ] = snorm16ToFloat( in[ 3 ] );
}

static inline uint32_t packUnorm4x8( const vec4_t in ) {
	return uint32_t( floatToUnorm8( in[0] ) )
		| ( uint32_t( floatToUnorm8( in[1] ) ) << 8 )
		| ( uint32_t( floatToUnorm8( in[2] ) ) << 16 )
		| ( uint32_t( floatToUnorm8( in[3] ) ) << 24 );
}

static inline f16_t floatToHalf( float in ) {
	static float scale = powf(2.0f, 15 - 127);

	uint32_t ui = Util::bit_cast<uint32_t>( in * scale );

	return { uint16_t(((ui & 0x80000000) >> 16) | ((ui & 0x0fffe000) >> 13)) };
}
static inline void floatToHalf( const vec4_t in, f16vec4_t out )
{
	out[ 0 ] = floatToHalf( in[ 0 ] );
	out[ 1 ] = floatToHalf( in[ 1 ] );
	out[ 2 ] = floatToHalf( in[ 2 ] );
	out[ 3 ] = floatToHalf( in[ 3 ] );
}
static inline float halfToFloat( f16_t in ) {
	static float scale = powf(2.0f, 127 - 15);

	uint32_t ui = (((unsigned int)in.bits & 0x8000) << 16) | (((unsigned int)in.bits & 0x7fff) << 13);
	return Util::bit_cast<float>(ui) * scale;
}
static inline void halfToFloat( const f16vec4_t in, vec4_t out )
{
	out[ 0 ] = halfToFloat( in[ 0 ] );
	out[ 1 ] = halfToFloat( in[ 1 ] );
	out[ 2 ] = halfToFloat( in[ 2 ] );
	out[ 3 ] = halfToFloat( in[ 3 ] );
}

// everything that is needed by the backend needs
// to be double buffered to allow it to run in
// parallel on a dual cpu machine
#define SMP_FRAMES            2

#define MAX_SHADERS           ( 1 << 12 )

#define MAX_SHADER_TABLES     1024
#define MAX_SHADER_STAGES     16

#define MAX_FBOS              64

#define MAX_VISCOUNTS         5
#define MAX_VIEWS             10

#define MAX_TEXTURE_MIPS      16
#define MAX_TEXTURE_LAYERS    256

// visibility tests: check if a 3D-point is visible
// results may be delayed, but for visual effect like flares this
// shouldn't matter
#define MAX_VISTESTS          256

#define MAX_MOD_KNOWN      1024
#define MAX_ANIMATIONFILES 4096

#define MAX_LIGHTMAPS      256
#define MAX_SKINS          1024

#define MAX_IN_GAME_VIDEOS 32

#define MAX_DRAWSURFS      0x10000
#define DRAWSURF_MASK      ( MAX_DRAWSURFS - 1 )

// 16x16 pixels per tile
#define TILE_SHIFT 4
#define TILE_SIZE  (1 << TILE_SHIFT)
#define TILE_SHIFT_STEP1 2
#define TILE_SIZE_STEP1  (1 << TILE_SHIFT_STEP1)

struct glFboShim_t
{
	/* Functions with same signature and similar purpose can be provided by:

	- ARB_framebuffer_object
	- EXT_framebuffer_object
	- OES_framebuffer_object

	They are not 100% equivalent, for example the ARB fbo provides more
	features than the EXT fbo, but the EXT fbo subset of the ARB fbo looks
	to be enough for us. */

	// void (*glBindFramebuffer) (GLenum target, GLuint framebuffer);
	PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
	// void (*glBindRenderbuffer) (GLenum target, GLuint renderbuffer);
	PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
	// GLenum (*glCheckFramebufferStatus) (GLenum target);
	PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
	// void (*glDeleteFramebuffers) (GLsizei n, const GLuint *framebuffers);
	PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
	// void (*glDeleteRenderbuffers) (GLsizei n, const GLuint *renderbuffers);
	PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
	// void (*glFramebufferRenderbuffer) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
	// void (*glFramebufferTexture1D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D;
	// void (*glFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
	// void (*glFramebufferTexture3D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
	PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D;
	// void (*glGenerateMipmap) (GLenum target);
	PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
	// void (*glGenFramebuffers) (GLsizei n, GLuint *framebuffers);
	PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
	// void (*glGenRenderbuffers) (GLsizei n, GLuint *renderbuffers);
	PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
	// void (*glRenderbufferStorage) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
	// void (*glBlitFramebuffer) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;

	/* Unused for now, part of GL_EXT_framebuffer_multisample:
	// void (*glRenderbufferStorageMultisample) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample; */

	/* Those three values are the same:

	- GL_FRAMEBUFFER_COMPLETE
	- GL_FRAMEBUFFER_COMPLETE_EXT
	- GL_FRAMEBUFFER_COMPLETE_OES

	The same happens with other GL_FRAMEBUFFER_* defines, we can expect the
	various names for various the extension variants to share the same values.

	So there is no need to abstract them and GL_FRAMEBUFFER_COMPLETE can be
	used in all cases. */
};

extern glFboShim_t GL_fboShim;

static inline void glFboSetArb()
{
	GL_fboShim.glBindFramebuffer =  glBindFramebuffer;
	GL_fboShim.glBindRenderbuffer = glBindRenderbuffer;
	GL_fboShim.glCheckFramebufferStatus = glCheckFramebufferStatus;
	GL_fboShim.glDeleteFramebuffers = glDeleteFramebuffers;
	GL_fboShim.glDeleteRenderbuffers = glDeleteRenderbuffers;
	GL_fboShim.glFramebufferRenderbuffer = glFramebufferRenderbuffer;
	GL_fboShim.glFramebufferTexture1D = glFramebufferTexture1D;
	GL_fboShim.glFramebufferTexture2D = glFramebufferTexture2D;
	GL_fboShim.glFramebufferTexture3D = glFramebufferTexture3D;
	GL_fboShim.glGenerateMipmap = glGenerateMipmap;
	GL_fboShim.glGenFramebuffers = glGenFramebuffers;
	GL_fboShim.glGenRenderbuffers = glGenRenderbuffers;
	GL_fboShim.glRenderbufferStorage = glRenderbufferStorage;
	GL_fboShim.glBlitFramebuffer = glBlitFramebuffer;

	/* Unused for now, part of GL_EXT_framebuffer_multisample:
	GL_fboShim.glRenderbufferStorageMultisample = glRenderbufferStorageMultisample; */
}

static inline void glFboSetExt()
{
	// EXT_framebuffer_object
	GL_fboShim.glBindFramebuffer = glBindFramebufferEXT;
	GL_fboShim.glBindRenderbuffer = glBindRenderbufferEXT;
	GL_fboShim.glCheckFramebufferStatus = glCheckFramebufferStatusEXT;
	GL_fboShim.glDeleteFramebuffers = glDeleteFramebuffersEXT;
	GL_fboShim.glDeleteRenderbuffers = glDeleteRenderbuffersEXT;
	GL_fboShim.glFramebufferRenderbuffer = glFramebufferRenderbufferEXT;
	GL_fboShim.glFramebufferTexture1D = glFramebufferTexture1DEXT;
	GL_fboShim.glFramebufferTexture2D = glFramebufferTexture2DEXT;
	GL_fboShim.glFramebufferTexture3D = glFramebufferTexture3DEXT;
	GL_fboShim.glGenerateMipmap = glGenerateMipmapEXT;
	GL_fboShim.glGenFramebuffers = glGenFramebuffersEXT;
	GL_fboShim.glGenRenderbuffers = glGenRenderbuffersEXT;
	GL_fboShim.glRenderbufferStorage = glRenderbufferStorageEXT;

	// EXT_framebuffer_blit
	GL_fboShim.glBlitFramebuffer = glBlitFramebufferEXT;

	/* Unused for now, part of GL_EXT_framebuffer_multisample:
	GL_fboShim.glRenderbufferStorageMultisample = glRenderbufferStorageMultisampleEXT; */
}

enum class lightMode_t { FULLBRIGHT, VERTEX, GRID, MAP };
enum class deluxeMode_t { NONE, GRID, MAP };

enum class showCubeProbesMode {
	DISABLED,
	GRID,
	UNIQUE
};

enum class cubeProbesAutoBuildMode {
	DISABLED,
	CACHED,
	ALWAYS
};

enum class shaderProfilerRenderSubGroupsMode {
	VS_OPAQUE,
	VS_TRANSPARENT,
	VS_ALL,
	FS_OPAQUE,
	FS_TRANSPARENT,
	FS_ALL
};

enum class ssaoMode {
	SHOW = -1,
	DISABLED,
	ENABLED,
};

	enum class renderSpeeds_t
	{
	  RSPEEDS_GENERAL = 1,
	  RSPEEDS_CULLING,
	  RSPEEDS_VIEWCLUSTER,
	  RSPEEDS_FOG,
	  RSPEEDS_CHC,
	  RSPEEDS_NEAR_FAR,
	};

	enum class glDebugModes_t
	{
		GLDEBUG_NONE,
		GLDEBUG_ERROR,
		GLDEBUG_DEPRECIATED,
		GLDEBUG_UNDEFINED,
		GLDEBUG_PORTABILITY,
		GLDEBUG_PERFORMANCE,
		GLDEBUG_OTHER,
		GLDEBUG_ALL
	};

#define REF_CUBEMAP_SIZE 32

#define REF_COLORGRADE_SLOTS   4
#define REF_COLORGRADEMAP_SIZE 16
#define REF_COLORGRADEMAP_STORE_SIZE ( REF_COLORGRADEMAP_SIZE * REF_COLORGRADEMAP_SIZE * REF_COLORGRADEMAP_SIZE )

	enum cullResult_t
	{
	  CULL_IN, // completely unclipped
	  CULL_CLIP, // clipped by one or more planes
	  CULL_OUT, // completely outside the clipping planes
	};

	struct screenRect_t
	{
		int                 coords[ 4 ];
	};

	enum frustumBits_t : int
	{
	  FRUSTUM_LEFT,
	  FRUSTUM_RIGHT,
	  FRUSTUM_BOTTOM,
	  FRUSTUM_TOP,
	  FRUSTUM_NEAR,
	  // seems intentionnal to have twice the value 5, except that, quote:
	  // There is at least one minor mistake in the usage of FRUSTUM_PLANES though.
	  // The signature of R_CalcFrustumFarCorners suggests that an array of length
	  // 5 is a good input, but it reads from the 6th element"
	  FRUSTUM_FAR = 5,
	  FRUSTUM_PLANES = 5,
	  FRUSTUM_CLIPALL = 1 | 2 | 4 | 8 | 16 //| 32
	};

	using frustum_t = cplane_t[6];

	// FIXME: it is impossible for the game to create any type besides RL_OMNI
	enum class refLightType_t
	{
	  RL_OMNI, // point light
	  RL_PROJ, // spot light
	  RL_DIRECTIONAL, // sun light

	  RL_MAX_REF_LIGHT_TYPE
	};

	struct refLight_t
	{
		refLightType_t rlType;

		vec3_t    origin;
		vec3_t    color; // should be color normalized

		// omni-directional light specific
		float     radius;

		// projective light specific
		vec3_t   projTarget;
		vec3_t   projUp;
	};

	// a structure matching the GLSL struct shaderLight in std140 layout
	struct shaderLight_t {
		vec3_t  center;
		float   radius;
		vec3_t  color;
		float   type;
		vec3_t  direction;
		float   angle;
	};

// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
	struct trRefEntity_t
	{
		// public from client game
		refEntity_t e;

		// local
		float        axisLength; // compensate for non-normalized axis

		vec3_t       localBounds[ 2 ];
		vec3_t       worldBounds[ 2 ];
	};

	struct orientationr_t
	{
		vec3_t   origin; // in world coordinates
		vec3_t   axis[ 3 ]; // orientation in world
		vec3_t   viewOrigin; // viewParms->or.origin in local coordinates
		matrix_t transformMatrix; // transforms object to world: either used by camera, model or light
		matrix_t viewMatrix; // affine inverse of transform matrix to transform other objects into this space
		matrix_t viewMatrix2; // without quake2opengl conversion
		matrix_t modelViewMatrix; // only used by models, camera viewMatrix * transformMatrix
	};

	enum
	{
		BIND_DIFFUSEMAP,
		BIND_NORMALMAP,
		BIND_HEIGHTMAP,
		BIND_MATERIALMAP,
		BIND_LIGHTMAP,
		BIND_DELUXEMAP,
		BIND_GLOWMAP,
		BIND_LIGHTGRID1,
		BIND_LIGHTGRID2,
		BIND_ENVIRONMENTMAP0,
		BIND_ENVIRONMENTMAP1,
		BIND_LIGHTTILES,
		BIND_LIGHTS,
	};

	enum
	{
	  IF_NONE,
	  IF_NOPICMIP = BIT( 0 ),
	  IF_FITSCREEN = BIT( 1 ),
	  IF_NORMALMAP = BIT( 2 ),
	  IF_RGBA16F = BIT( 3 ),
	  IF_RGBA32F = BIT( 4 ),
	  IF_TWOCOMP16F = BIT( 5 ),
	  IF_TWOCOMP32F = BIT( 6 ),
	  IF_ONECOMP16F = BIT( 7 ),
	  IF_ONECOMP32F = BIT( 8 ),
	  IF_DEPTH16 = BIT( 9 ),
	  IF_DEPTH24 = BIT( 10 ),
	  IF_DEPTH32 = BIT( 11 ),
	  IF_PACKED_DEPTH24_STENCIL8 = BIT( 12 ),
	  IF_LIGHTMAP = BIT( 13 ),
	  IF_RGBE = BIT( 15 ),
	  IF_ALPHATEST = BIT( 16 ), // FIXME: this is unused
	  IF_ALPHA = BIT( 17 ),
	  IF_SRGB = BIT( 18 ),
	  IF_BC1 = BIT( 19 ),
	  IF_BC2 = BIT( 20 ),
	  IF_BC3 = BIT( 21 ),
	  IF_BC4 = BIT( 22 ),
	  IF_BC5 = BIT( 23 ),
	  IF_RGBA32UI = BIT( 24 ),
	  IF_HOMEPATH = BIT( 25 )
	};

	enum class filterType_t
	{
	  FT_DEFAULT,
	  FT_LINEAR,
	  FT_NEAREST
	};

	enum class wrapTypeEnum_t
	{
	  WT_REPEAT,
	  WT_CLAMP, // don't repeat the texture for texture coords outside [0, 1]
	  WT_EDGE_CLAMP,
	  WT_ONE_CLAMP,
	  WT_ZERO_CLAMP, // guarantee 0,0,0,255 edge for projected textures
	  WT_ALPHA_ZERO_CLAMP // guarantee 0 alpha edge for projected textures
	};

	struct wrapType_t
	{
		wrapTypeEnum_t s, t;

		wrapType_t() = default;
		wrapType_t( wrapTypeEnum_t w ) : s(w), t(w) {}
		wrapType_t( wrapTypeEnum_t s, wrapTypeEnum_t t ) : s(s), t(t) {}

		inline wrapType_t &operator =( wrapTypeEnum_t w ) { this->s = this->t = w; return *this; }

	};

	static inline bool operator ==( const wrapType_t &a, const wrapType_t &b ) { return a.s == b.s && a.t == b.t; }
	static inline bool operator !=( const wrapType_t &a, const wrapType_t &b ) { return a.s != b.s || a.t != b.t; }

	struct imageParams_t
	{
		int bits = 0;
		filterType_t filterType;
		wrapType_t wrapType;
		int minDimension = 0;
		int maxDimension = 0;

		bool operator==(const imageParams_t &o) const
		{
			return o.bits == bits && o.filterType == filterType && o.wrapType == wrapType
				&& o.minDimension == minDimension && o.maxDimension == maxDimension;
		}
	};

	struct image_t
	{
		char name[ MAX_QPATH ];

		imageParams_t initialParams; // may not match final values

		GLenum         type;
		GLuint         texnum; // gl texture binding
		Texture        *texture;

		uint16_t width, height, numLayers; // source image
		uint16_t       uploadWidth, uploadHeight; // after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE

		int            frameUsed; // for texture usage in frame statistics

		uint32_t       internalFormat;

		uint32_t       bits;
		filterType_t   filterType;
		wrapType_t     wrapType;

		image_t *next;
	};

	inline bool IsImageCompressed(int bits) { return bits & (IF_BC1 | IF_BC2 | IF_BC3 | IF_BC4 | IF_BC5); }

	struct FBO_t
	{
		char     name[ MAX_QPATH ];

		uint32_t frameBuffer;

		int      width;
		int      height;
	};

//===============================================================================

	// Sorts commented "keyword only" will never be used unless the shader text has a `sort` keyword specifying it.
	enum class shaderSort_t
	{
	  SS_BAD,
	  SS_PORTAL, // mirrors, portals, viewscreens

	  SS_DEPTH, // depth pre-pass

	  SS_ENVIRONMENT_FOG, // sky

	  SS_OPAQUE, // opaque

	  SS_ENVIRONMENT_NOFOG, // Tr3B: moved skybox here so we can fog post process all SS_OPAQUE materials

	  SS_DECAL, // scorch marks, etc.
	  SS_SEE_THROUGH, // ladders, grates, grills that may have small blended edges in addition to alpha test
	  SS_BANNER, // keyword only
	  SS_FOG,

	  SS_UNDERWATER, // keyword only
	  SS_FAR, // keyword only
	  SS_MEDIUM, // keyword only
	  SS_CLOSE, // keyword only

	  // Must have value 15 because binary.shader contains numerical sort values between 15 and 16
	  SS_BLEND0 = 15, // regular transparency and filters

	  SS_BLEND1, // Keyword only. Generally only used for additive type effects
	  SS_ALMOST_NEAREST, // keyword only
	  SS_NEAREST, // keyword only
	  SS_POST_PROCESS, // keyword only

	  SS_NUM_SORTS
	};

	struct shaderTable_t
	{
		char                 name[ MAX_QPATH ];

		uint16_t             index;

		bool             clamp;
		bool             snap;

		float                *values;
		uint16_t             numValues;

		shaderTable_t *next;
	};

	enum class genFunc_t
	{
	  GF_NONE,

	  GF_SIN,
	  GF_SQUARE,
	  GF_TRIANGLE,
	  GF_SAWTOOTH,
	  GF_INVERSE_SAWTOOTH,
	  GF_NOISE
	};

	enum class deform_t
	{
	  DEFORM_NONE,
	  DEFORM_WAVE,
	  DEFORM_NORMALS,
	  DEFORM_BULGE,
	  DEFORM_MOVE,
	  DEFORM_ROTGROW
	};

	enum class alphaGen_t
	{
	  AGEN_IDENTITY,
	  AGEN_ENTITY,
	  AGEN_ONE_MINUS_ENTITY,
	  AGEN_VERTEX,
	  AGEN_ONE_MINUS_VERTEX,
	  AGEN_WAVEFORM,
	  AGEN_PORTAL,
	  AGEN_CONST,
	  AGEN_CUSTOM
	};

	enum class colorGen_t
	{
	  CGEN_BAD,
	  CGEN_IDENTITY_LIGHTING, // Always (1,1,1,1) in Dæmon engine, unless you set r_overbrightQ3.
	  CGEN_IDENTITY, // always (1,1,1,1)
	  CGEN_ENTITY, // grabbed from entity's modulate field
	  CGEN_ONE_MINUS_ENTITY, // grabbed from 1 - entity.modulate
	  CGEN_VERTEX, // tess.colors
	  CGEN_ONE_MINUS_VERTEX,
	  CGEN_WAVEFORM, // programmatically generated
	  CGEN_CONST, // fixed color
	  CGEN_CUSTOM_RGB, // like fixed color but generated dynamically, single arithmetic expression
	  CGEN_CUSTOM_RGBs, // multiple expressions
	};

	enum class opcode_t
	{
	  OP_BAD,
	  // logic operators
	  OP_LAND,
	  OP_LOR,
	  OP_GE,
	  OP_LE,
	  OP_LEQ,
	  OP_LNE,
	  // arithmetic operators
	  OP_ADD,
	  OP_SUB,
	  OP_DIV,
	  OP_MOD,
	  OP_MUL,
	  OP_NEG,
	  // logic operators
	  OP_LT,
	  OP_GT,
	  // embracements
	  OP_LPAREN,
	  OP_RPAREN,
	  OP_LBRACKET,
	  OP_RBRACKET,
	  // constants or variables
	  OP_NUM,
	  OP_TIME,
	  OP_PARM0,
	  OP_PARM1,
	  OP_PARM2,
	  OP_PARM3,
	  OP_PARM4,
	  OP_PARM5,
	  OP_PARM6,
	  OP_PARM7,
	  OP_PARM8,
	  OP_PARM9,
	  OP_PARM10,
	  OP_PARM11,
	  OP_GLOBAL0,
	  OP_GLOBAL1,
	  OP_GLOBAL2,
	  OP_GLOBAL3,
	  OP_GLOBAL4,
	  OP_GLOBAL5,
	  OP_GLOBAL6,
	  OP_GLOBAL7,
	  OP_FRAGMENTSHADERS,
	  OP_FRAMEBUFFEROBJECTS,
	  OP_SOUND,
	  OP_DISTANCE,
	  // table access
	  OP_TABLE
	};

	struct opstring_t
	{
		const char *s;
		opcode_t   type;
	};

	struct expOperation_t
	{
		opcode_t type;
		float    value;
	};


enum
{
	EXP_NONE,
	EXP_CLAMP = BIT( 0 ),
	EXP_SRGB = BIT( 1 ),
};

#define MAX_EXPRESSION_OPS 32
	struct expression_t
	{
		expOperation_t ops[ MAX_EXPRESSION_OPS ];
		size_t numOps;
		int bits;

		bool operator==( const expression_t& other ) {
			if ( numOps != other.numOps ) {
				return false;
			}

			for ( size_t i = 0; i < numOps; i++ ) {
				if ( ops[i].type != other.ops[i].type || ops[i].value != other.ops[i].value ) {
					return false;
				}
			}

			return true;
		}

		bool operator!=( const expression_t& other ) {
			return !( *this == other );
		}
	};

	struct waveForm_t
	{
		genFunc_t func;

		float     base;
		float     amplitude;
		float     phase;
		float     frequency;

		bool operator==( const waveForm_t& other ) {
			return func == other.func && base == other.base && amplitude == other.amplitude && phase == other.phase
				&& frequency == other.frequency;
		}

		bool operator!=( const waveForm_t& other ) {
			return !( *this == other );
		}
	};

#define TR_MAX_TEXMODS 4

	enum class texMod_t
	{
	  TMOD_NONE,
	  TMOD_TRANSFORM,
	  TMOD_TURBULENT,
	  TMOD_SCROLL,
	  TMOD_SCALE,
	  TMOD_STRETCH,
	  TMOD_ROTATE,
	  TMOD_ENTITY_TRANSLATE,

	  TMOD_SCROLL2,
	  TMOD_SCALE2,
	  TMOD_CENTERSCALE,
	  TMOD_SHEAR,
	  TMOD_ROTATE2
	};

#define MAX_SHADER_DEFORMS      3
	struct deformStage_t
	{
		deform_t   deformation; // vertex coordinate modification type

		vec3_t     moveVector;
		waveForm_t deformationWave;
		float      deformationSpread;

		float      bulgeWidth;
		float      bulgeHeight;
		float      bulgeSpeed;

		float      flareSize;
	};

	struct texModInfo_t
	{
		texMod_t type;

		// used for TMOD_TURBULENT and TMOD_STRETCH
		waveForm_t wave;

		// used for TMOD_TRANSFORM
		matrix_t matrix; // s' = s * m[0][0] + t * m[1][0] + trans[0]
		// t' = s * m[0][1] + t * m[0][1] + trans[1]

		// used for TMOD_SCALE
		float scale[ 2 ]; // s *= scale[0]
		// t *= scale[1]

		// used for TMOD_SCROLL
		float scroll[ 2 ]; // s' = s + scroll[0] * time
		// t' = t + scroll[1] * time

		// + = clockwise
		// - = counterclockwise
		float rotateSpeed;

		// used by everything else
		expression_t sExp;
		expression_t tExp;
		expression_t rExp;

		bool operator==( const texModInfo_t& other ) {
			return type == other.type && wave == other.wave && MatrixCompare( matrix, other.matrix )
				&& scale[0] == other.scale[0] && scale[1] == other.scale[1] && scroll[0] == other.scroll[0] && scroll[1] == other.scroll[1]
				&& rotateSpeed == other.rotateSpeed
				&& sExp == other.sExp && tExp == other.tExp && rExp == other.rExp;
		}

		bool operator!=( const texModInfo_t& other ) {
			return !( *this == other );
		}
	};

#define MAX_IMAGE_ANIMATIONS 32

	enum
	{
	  TB_COLORMAP,
	  TB_DIFFUSEMAP = TB_COLORMAP,
	  TB_REFLECTIONMAP = TB_COLORMAP,
	  TB_NORMALMAP,
	  TB_HEIGHTMAP,
	  TB_MATERIALMAP,
	  TB_PHYSICALMAP = TB_MATERIALMAP,
	  TB_SPECULARMAP = TB_MATERIALMAP,
	  TB_GLOWMAP,
	  MAX_TEXTURE_BUNDLES
	};

	struct textureBundle_t
	{
		uint8_t      numImages;
		float        imageAnimationSpeed;
		image_t      *image[ MAX_IMAGE_ANIMATIONS ];

		size_t numTexMods;
		texModInfo_t *texMods;

		int videoMapHandle;
		bool isVideoMap;
	};

	enum class stageType_t
	{
	  // material shader stage types
	  ST_COLORMAP, // vanilla Q3A style shader treatening
	  ST_GLOWMAP,
	  ST_DIFFUSEMAP,
	  ST_NORMALMAP,
	  ST_HEIGHTMAP,
	  ST_PHYSICALMAP,
	  ST_SPECULARMAP,
	  ST_REFLECTIONMAP, // cubeMap based reflection
	  ST_SKYBOXMAP,
	  ST_SCREENMAP, // 2d offscreen or portal rendering
	  ST_PORTALMAP,
	  ST_HEATHAZEMAP, // heatHaze post process effect
	  ST_LIQUIDMAP,
	  ST_FOGMAP,
	  ST_LIGHTMAP,
	  ST_STYLELIGHTMAP,
	  ST_STYLECOLORMAP,
	  ST_COLLAPSE_COLORMAP,
	  ST_COLLAPSE_DIFFUSEMAP,
	  ST_COLLAPSE_REFLECTIONMAP,  // color cubemap + normalmap
	};

	enum class collapseType_t
	{
	  COLLAPSE_none,
	  COLLAPSE_generic, // used before we know it's another one
	  COLLAPSE_PHONG,
	  COLLAPSE_PBR,
	  COLLAPSE_REFLECTIONMAP,
	};

	struct shader_t;
	struct shaderStage_t;
	struct Material;
	struct MaterialSurface;

	// [implicit only] enable lightmapping, front-side culling, disable (non-BSP) vertex colors, disable blending
	// TODO(0.56): move to the public RegisterShaderFlags_t interface
#define RSF_3D ( BIT( 30 ) )

#define RSF_BSP ( BIT ( 29 ) )

	using stageRenderer_t = void(*)(shaderStage_t *);
	using stageShaderBuildMarker_t = void(*)(const shaderStage_t*);
	using surfaceDataUpdater_t = void(*)(uint32_t*, shaderStage_t*, bool, bool);
	using stageShaderBinder_t = void(*)(Material*);
	using stageMaterialProcessor_t = void(*)(Material*, shaderStage_t*, MaterialSurface*);

	enum ShaderStageVariant {
		VERTEX_LIT = BIT( 0 ),
		FULLBRIGHT = BIT( 1 ),
		ALL = BIT( 2 )
	};

	using floatProcessor_t = float(*)(float);
	using colorProcessor_t = Color::Color(*)(Color::Color);

	Color::Color convertColorFromSRGB_NOP( Color::Color c );

	struct shaderStage_t
	{
		stageType_t     type;

		collapseType_t collapseType;

		bool        active;

		bool            dpMaterial;

		// Core renderer (code path for when only OpenGL Core is available, or compatible OpenGL 2).
		stageRenderer_t colorRenderer;
		stageShaderBuildMarker_t shaderBuildMarker;

		// Material renderer (code path for advanced OpenGL techniques like bindless textures).
		surfaceDataUpdater_t surfaceDataUpdater;
		stageShaderBinder_t shaderBinder;
		stageMaterialProcessor_t materialProcessor;

		colorProcessor_t convertColorFromSRGB;

		textureBundle_t bundle[ MAX_TEXTURE_BUNDLES ];

		expression_t    ifExp;

		waveForm_t      rgbWave;
		colorGen_t      rgbGen;
		expression_t    rgbExp;
		expression_t    redExp;
		expression_t    greenExp;
		expression_t    blueExp;

		waveForm_t      alphaWave;
		alphaGen_t      alphaGen;
		expression_t    alphaExp;

		expression_t    alphaTestExp;

		bool        tcGen_Environment;
		bool        tcGen_Lightmap;

		Color::Color32Bit constantColor; // for CGEN_CONST and AGEN_CONST

		uint32_t        stateBits; // GLS_xxxx mask
		uint8_t colorspaceBits; // LINEAR_xxxx mask

		bool            isCubeMap;

		int deformIndex; // FIXME: should be on the shader level, not per-stage
		bool        overrideNoPicMip; // for images that must always be full resolution
		bool fitScreen; // For images that should be scaled to fit the screen size.
		bool        overrideFilterType; // for console fonts, 2D elements, etc.
		filterType_t    filterType;
		bool        overrideWrapType;
		wrapType_t      wrapType;

		bool        highQuality;
		bool        forceHighQuality;

		bool forceVertexLighting;

		bool        hasDepthFade; // for soft particles
		float           depthFadeValue;

		expression_t    refractionIndexExp;

		expression_t    specularExponentMin;
		expression_t    specularExponentMax;

		expression_t    fresnelPowerExp;
		expression_t    fresnelScaleExp;
		expression_t    fresnelBiasExp;

		// Texture storage variants.
		bool hasHeightMapInNormalMap;

		// Available features.
		bool enableNormalMapping;
		bool enableDeluxeMapping;
		bool enableReliefMapping;
		bool enablePhysicalMapping;
		bool enableSpecularMapping;
		bool enableGlowMapping;

		// Normal map scale and format.
		int8_t normalFormat[ 3 ];
		vec3_t normalScale = { 1.0f, 1.0f, 1.0f };

		expression_t    normalIntensityExp;

		expression_t    fogDensityExp;

		expression_t    depthScaleExp;

		expression_t    deformMagnitudeExp;

		bool        noFog; // used only for shaders that have fog disabled, so we can enable it for individual stages

		bool useMaterialSystem = false;
		shader_t* shader;
		shaderStage_t* materialRemappedStage = nullptr;

		uint32_t paddedSize = 0;

		uint32_t materialOffset = 0;
		uint32_t bufferOffset = 0;
		uint32_t dynamicBufferOffset = 0;

		bool initialized = false;

		bool dynamic = false;
		bool colorDynamic = false;

		int variantOffsets[ShaderStageVariant::ALL];
		uint32_t variantOffset = 0;
	};

	enum cullType_t : int
	{
		CT_FRONT_SIDED = 0,
		CT_TWO_SIDED   = 1,
		CT_BACK_SIDED  = 2
	};

	// reverse the cull operation
#       define ReverseCull(c) Util::enum_cast<cullType_t>(2 - (c))

	enum class fogPass_t
	{
	  FP_NONE, // surface is translucent and will just be adjusted properly
	  FP_EQUAL, // surface is opaque but possibly alpha tested
	  FP_LE // surface is translucent, but still needs a fog pass (fog surface)
	};

	struct skyParms_t
	{
		float   cloudHeight;
		image_t *outerbox, *innerbox;
	};

	struct fogParms_t
	{
		vec3_t color;
		float  depthForOpaque;
	};

	struct shader_t
	{
		char         name[ MAX_QPATH ]; // game path, including extension
		int registerFlags; // RSF_

		int          index; // this shader == tr.shaders[index]
		int          sortedIndex; // this shader == tr.sortedShaders[sortedIndex]

		float        sort; // lower numbered shaders draw before higher numbered

		bool     defaultShader; // we want to return index 0 if the shader failed to
		// load for some reason, but R_FindShader should
		// still keep a name allocated for it, so if
		// something calls RE_RegisterShader again with
		// the same name, we don't try looking for it again

		int            surfaceFlags;
		int            contentFlags;

		bool       entityMergable; // merge across entites optimizable (smoke, blood)

		fogParms_t     fogParms;
		fogPass_t      fogPass; // draw a blended pass, possibly with depth test equals
		bool       noFog;

		bool       disableReliefMapping; // disable relief mapping for this material even if it's available
		float      reliefOffsetBias; // offset the heightmap top relatively to the floor
		float reliefDepthScale = 1.0f; // per-shader relief depth scale

		bool       ambientLight;
		bool       translucent;
		bool       forceOpaque;
		bool       isSky;
		skyParms_t     sky;

		float          portalRange; // distance to fog out at
		bool       isPortal;

		cullType_t     cullType; // CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
		bool       polygonOffset; // set for decals and other items that must be offset
		float          polygonOffsetValue;

		bool       noPicMip; // for images that must always be full resolution
		bool fitScreen; // For images that should be scaled to fit the screen size.
		int        imageMinDimension;   // for images that must not be loaded with smaller size
		int        imageMaxDimension;   // for images that must not be loaded with larger size
		filterType_t   filterType; // for console fonts, 2D elements, etc.
		wrapType_t     wrapType;

		// For RT_SPRITE, face opposing the view direction rather than the viewer
		bool entitySpriteFaceViewDirection;

		int		autoSpriteMode;
		bool autoSpriteWarned = false;

		bool forceLightMap;

		bool shaderRemapWarned;

		uint8_t         numDeforms;
		deformStage_t   deforms[ MAX_SHADER_DEFORMS ];

		shaderStage_t *stages;
		shaderStage_t *lastStage;

		struct shader_t *remappedShader; // current shader this one is remapped too

		struct {
			char *name;
			int  index;
		} altShader[ MAX_ALTSHADERS ]; // state-based remapping; note that index 0 is unused

		struct shader_t *depthShader;
		struct shader_t *fogShader;
		struct shader_t *next;
	};

// *INDENT-OFF*
	enum
	{
	  GLS_SRCBLEND_ZERO = ( 1 << 0 ),
	  GLS_SRCBLEND_ONE = ( 1 << 1 ),
	  GLS_SRCBLEND_DST_COLOR = ( 1 << 2 ),
	  GLS_SRCBLEND_ONE_MINUS_DST_COLOR = ( 1 << 3 ),
	  GLS_SRCBLEND_SRC_ALPHA = ( 1 << 4 ),
	  GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA = ( 1 << 5 ),
	  GLS_SRCBLEND_DST_ALPHA = ( 1 << 6 ),
	  GLS_SRCBLEND_ONE_MINUS_DST_ALPHA = ( 1 << 7 ),
	  GLS_SRCBLEND_ALPHA_SATURATE = ( 1 << 8 ),

	  GLS_SRCBLEND_BITS = GLS_SRCBLEND_ZERO
	                      | GLS_SRCBLEND_ONE
	                      | GLS_SRCBLEND_DST_COLOR
	                      | GLS_SRCBLEND_ONE_MINUS_DST_COLOR
	                      | GLS_SRCBLEND_SRC_ALPHA
	                      | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA
	                      | GLS_SRCBLEND_DST_ALPHA
	                      | GLS_SRCBLEND_ONE_MINUS_DST_ALPHA
	                      | GLS_SRCBLEND_ALPHA_SATURATE,

	  GLS_DSTBLEND_ZERO = ( 1 << 9 ),
	  GLS_DSTBLEND_ONE = ( 1 << 10 ),
	  GLS_DSTBLEND_SRC_COLOR = ( 1 << 11 ),
	  GLS_DSTBLEND_ONE_MINUS_SRC_COLOR = ( 1 << 12 ),
	  GLS_DSTBLEND_SRC_ALPHA = ( 1 << 13 ),
	  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA = ( 1 << 14 ),
	  GLS_DSTBLEND_DST_ALPHA = ( 1 << 15 ),
	  GLS_DSTBLEND_ONE_MINUS_DST_ALPHA = ( 1 << 16 ),

	  GLS_DSTBLEND_BITS = GLS_DSTBLEND_ZERO
	                      | GLS_DSTBLEND_ONE
	                      | GLS_DSTBLEND_SRC_COLOR
	                      | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR
	                      | GLS_DSTBLEND_SRC_ALPHA
	                      | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA
	                      | GLS_DSTBLEND_DST_ALPHA
	                      | GLS_DSTBLEND_ONE_MINUS_DST_ALPHA,

	  GLS_DEPTHMASK_TRUE = ( 1 << 17 ),

	  GLS_POLYMODE_LINE = ( 1 << 18 ),

	  GLS_DEPTHTEST_DISABLE = ( 1 << 19 ),

	  GLS_DEPTHFUNC_LESS = ( 1 << 20 ),
	  GLS_DEPTHFUNC_EQUAL = ( 1 << 21 ),
	  GLS_DEPTHFUNC_ALWAYS = ( 1 << 22 ),

	  GLS_DEPTHFUNC_BITS = GLS_DEPTHFUNC_LESS
	                       | GLS_DEPTHFUNC_EQUAL
	                       | GLS_DEPTHFUNC_ALWAYS,

	  GLS_ATEST_NONE   = ( 0 << 23 ),
	  GLS_ATEST_GT_0   = ( 1 << 23 ),
	  GLS_ATEST_LT_128 = ( 2 << 23 ),
	  GLS_ATEST_GE_128 = ( 3 << 23 ),
	  GLS_ATEST_LT_ENT = ( 4 << 23 ),
	  GLS_ATEST_GT_ENT = ( 5 << 23 ),

	  GLS_ATEST_BITS   = ( 7 << 23 ),

	  GLS_REDMASK_FALSE = ( 1 << 26 ),
	  GLS_GREENMASK_FALSE = ( 1 << 27 ),
	  GLS_BLUEMASK_FALSE = ( 1 << 28 ),
	  GLS_ALPHAMASK_FALSE = ( 1 << 29 ),

	  GLS_COLORMASK_BITS = GLS_REDMASK_FALSE
	                       | GLS_GREENMASK_FALSE
	                       | GLS_BLUEMASK_FALSE
	                       | GLS_ALPHAMASK_FALSE,

	  GLS_DEFAULT = GLS_DEPTHMASK_TRUE
	};

	enum {
		LINEAR_RGBGEN = ( 1 << 0 ),
		LINEAR_COLORMAP = ( 1 << 1 ),
		LINEAR_SPECULARMAP = ( 1 << 2 ),
	};

// *INDENT-ON*
	struct ShaderProgramDescriptor;

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
	struct trRefdef_t
	{
		int           x, y, width, height;
		float         fov_x, fov_y;
		vec3_t        vieworg;
		vec3_t        viewaxis[ 3 ]; // transformation matrix
		vec3_t        blurVec;

		int           time; // time in milliseconds for shader effects and other time dependent rendering issues
		int           rdflags; // RDF_NOWORLDMODEL, etc

		// 1 bits will prevent the associated area from rendering at all
		byte     areamask[ MAX_MAP_AREA_BYTES ];
		bool areamaskModified; // true if areamask changed since last scene

		float    floatTime; // tr.refdef.time / 1000.0

		int                     numEntities;
		trRefEntity_t           *entities;

		int                     numLights;
		refLight_t              *lights;

		int                     numPolys;
		struct srfPoly_t        *polys;

		int                     numDrawSurfs;
		struct drawSurf_t       *drawSurfs;

		byte                    *pixelTarget; //set this to Non Null to copy to a buffer after scene rendering
		int                     pixelTargetWidth;
		int                     pixelTargetHeight;

		int                    numVisTests;
		struct visTestResult_t *visTests;
	};

//=================================================================================

// skins allow models to be retextured without modifying the model file
	struct skinSurface_t
	{
		char     name[ MAX_QPATH ];
		shader_t *shader;
	};

	struct skin_t
	{
		char          name[ MAX_QPATH ]; // game path, including extension
		int           numSurfaces;
		int           numModels;
		skinSurface_t *surfaces[ MD3_MAX_SURFACES ];
	};

//----(SA) end

//=================================================================================

	struct fog_t
	{
		int        originalBrushNumber;
		vec3_t     bounds[ 2 ];

		Color::Color color; // in packed byte format
		float      tcScale; // texture coordinate vector scales
		fogParms_t fogParms;

		// for clipping distance in fog when outside
		bool hasSurface;
		float    surface[ 4 ];
	};

	struct viewParms_t
	{
		orientationr_t orientation;
		orientationr_t world;

		uint viewID = 0;

		vec3_t         pvsOrigin; // may be different than or.origin for portals

		int            portalLevel; // number of portals this view is through
		bool hasNestedViews = false;
		int            mirrorLevel;
		bool           isMirror; // the portal is a mirror, invert the face culling

		int            frameSceneNum; // copied from tr.frameSceneNum
		int            frameCount; // copied from tr.frameCount
		int            viewCount; // copied from tr.viewCount

		frustum_t      portalFrustum; // view space frustum bounding the portal surface, clip anything behind near plane for proper rendering. FAR plane is unused
		int            scissorX, scissorY, scissorWidth, scissorHeight;
		int            viewportX, viewportY, viewportWidth, viewportHeight;
		vec4_t         viewportVerts[ 4 ]; // for immediate 2D quad rendering
		vec4_t         gradingWeights;

		float          fovX, fovY;
		matrix_t       projectionMatrix;
		matrix_t       projectionMatrixNonPortal; // For skybox rendering in portals
		matrix_t       unprojectionMatrix; // transform pixel window space -> world space

		frustum_t      frustum;

		vec3_t               visBounds[ 2 ];
		float                zNear;
		float                zFar;

		int                  numDrawSurfs;
		struct drawSurf_t    *drawSurfs;
		int                  firstDrawSurf[ Util::ordinal(shaderSort_t::SS_NUM_SORTS) + 1 ];
	};

	/*
	==============================================================================

	SURFACES

	==============================================================================
	*/

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
	enum class surfaceType_t
	{
	  SF_MIN = -1, // partially ensures that sizeof(surfaceType_t) == sizeof(int)

	  SF_BAD,
	  SF_SKIP, // ignore

	  SF_FACE,
	  SF_GRID,
	  SF_TRIANGLES,

	  SF_POLY,

	  SF_MDV,
	  SF_MD5,
	  SF_IQM,

	  SF_ENTITY, // beams, rails, lightning, etc that can be determined by entity

	  SF_VBO_MESH,
	  SF_VBO_MD5MESH,
	  SF_VBO_MDVMESH,

	  SF_NUM_SURFACE_TYPES,

	  SF_MAX = 0x7fffffff // partially (together, fully) ensures that sizeof(surfaceType_t) == sizeof(int)
	};


	// drawSurf components are packed into a single 64bit integer for
	// efficient sorting
	// sort by:
	// 1. shaderNum
	// 2. lightmapNum
	// 3. entityNum
	// 4. index

	static const uint64_t SORT_INDEX_BITS = 20;
	static const uint64_t SORT_ENTITYNUM_BITS = 13;
	static const uint64_t SORT_LIGHTMAP_BITS = 9;
	static const uint64_t SORT_SHADER_BITS = 16;
	static const uint64_t SORT_UNUSED_BITS = 6;

	static_assert( SORT_SHADER_BITS +
		SORT_LIGHTMAP_BITS +
		SORT_ENTITYNUM_BITS +
		SORT_INDEX_BITS +
		SORT_UNUSED_BITS == 64, "invalid number of drawSurface sort bits" );

	static const uint64_t SORT_ENTITYNUM_SHIFT = SORT_INDEX_BITS;
	static const uint64_t SORT_LIGHTMAP_SHIFT = SORT_ENTITYNUM_BITS + SORT_ENTITYNUM_SHIFT;
	static const uint64_t SORT_SHADER_SHIFT = SORT_LIGHTMAP_BITS + SORT_LIGHTMAP_SHIFT;

#define MASKBITS( b ) ( 1 << (b) ) - 1
	static const uint32_t SORT_INDEX_MASK = MASKBITS( SORT_INDEX_BITS );
	static const uint32_t SORT_ENTITYNUM_MASK = MASKBITS( SORT_ENTITYNUM_BITS );
	static const uint32_t SORT_LIGHTMAP_MASK = MASKBITS( SORT_LIGHTMAP_BITS );
	static const uint32_t SORT_SHADER_MASK = MASKBITS( SORT_SHADER_BITS );

	static_assert( SORT_INDEX_MASK >= MAX_DRAWSURFS - 1, "not enough index bits" );

	// need space for tr.worldEntity, in addition to MAX_REF_ENTITIES
	static_assert( SORT_ENTITYNUM_MASK >= MAX_REF_ENTITIES, "not enough entity bits" );

	// need space for -1 lightmap (no lightmap) in addition to MAX_LIGHTMAPS
	static_assert( SORT_LIGHTMAP_MASK >= MAX_LIGHTMAPS, "not enough lightmap bits" );

	static_assert( SORT_SHADER_MASK >= MAX_SHADERS - 1, "not enough qshader bits" );

	struct drawSurf_t
	{
		trRefEntity_t *entity;
		surfaceType_t *surface; // any of surface*_t
		shader_t      *shader;
		uint64_t      sort;
		bool          bspSurface;
		int fog;
		int portalNum = -1;

		inline int index() const {
			return int( ( sort & SORT_INDEX_MASK ) );
		}
		inline int entityNum() const {
			return int( ( sort >> SORT_ENTITYNUM_SHIFT ) & SORT_ENTITYNUM_MASK ) - 1;
		}
		inline int lightmapNum() const {
			return int( ( sort >> SORT_LIGHTMAP_SHIFT ) & SORT_LIGHTMAP_MASK ) - 1;
		}
		inline int shaderNum() const {
			return int( sort >> SORT_SHADER_SHIFT );
		}

		inline void setSort( int shaderNum, int lightmapNum, int entityNum, int index ) {
			entityNum = entityNum + 1; //world entity is -1
			lightmapNum = lightmapNum + 1; //no lightmap is -1
			sort = uint64_t( index & SORT_INDEX_MASK ) |
				( uint64_t( entityNum & SORT_ENTITYNUM_MASK ) << SORT_ENTITYNUM_SHIFT ) |
				( uint64_t( lightmapNum & SORT_LIGHTMAP_MASK ) << SORT_LIGHTMAP_SHIFT ) |
				( uint64_t( shaderNum & SORT_SHADER_MASK ) << SORT_SHADER_SHIFT );
		}
	};

#define MAX_PATCH_SIZE  64 // max dimensions of a patch mesh in map file
#define MAX_GRID_SIZE   65 // max dimensions of a grid mesh in memory

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
	struct srfPoly_t
	{
		surfaceType_t surfaceType;
		qhandle_t     hShader;
		int16_t       numVerts;
		int16_t       fogIndex;
		polyVert_t    *verts;
	};

	struct srfVert_t
	{
		vec3_t xyz;

		// HACK: st and lightmap must be adjacent for R_CreateWorldVBO
		vec2_t st;
		vec2_t lightmap;

		vec3_t normal;
		i16vec4_t qtangent;
		Color::Color32Bit lightColor;
	};

	struct srfTriangle_t
	{
		int      indexes[ 3 ];
	};

// ydnar: plain map drawsurfaces must match this header
	struct srfGeneric_t {
		// v Valid in: SF_FACE, SF_GRID, SF_TRIANGLES, SF_VBO_MESH
		surfaceType_t surfaceType;

		// Culling information
		vec3_t bounds[2];
		vec3_t origin;
		float radius;

		// Static render data
		VBO_t* vbo;
		IBO_t* ibo;

		// BSP VBO offset
		int firstIndex;

		int numVerts;

		int numTriangles;
		srfTriangle_t* triangles;
		// ^ Valid in: SF_FACE, SF_GRID, SF_TRIANGLES, SF_VBO_MESH

		cplane_t plane; // Valid in: SF_FACE, SF_TRIANGLES

		srfVert_t* verts; // Valid in: SF_FACE, SF_GRID, SF_TRIANGLES
	};

	struct srfGridMesh_t : srfGeneric_t {
		// lod information, which may be different
		// than the culling information to allow for
		// groups of curves that LOD as a unit
		vec3_t lodOrigin;
		float lodRadius;
		int lodFixed;
		int lodStitched;

		// triangle definitions
		int width, height;
		float* widthLodError;
		float* heightLodError;
	};

	struct srfVBOMesh_t : srfGeneric_t {
		int lightmapNum; // FIXME get rid of this by merging all lightmaps at level load
	};

	struct srfVBOMD5Mesh_t
	{
		surfaceType_t     surfaceType;

		struct md5Model_t *md5Model;

		shader_t   *shader; // FIXME move this to somewhere else

		int               skinIndex;

		int               numBoneRemap;
		int               boneRemap[ MAX_BONES ];
		int               boneRemapInverse[ MAX_BONES ];

		// backEnd stats
		int numIndexes;
		int numVerts;

		// static render data
		VBO_t *vbo;
		IBO_t *ibo;
	};

	struct srfVBOMDVMesh_t
	{
		surfaceType_t       surfaceType;

		struct mdvModel_t   *mdvModel;

		struct mdvSurface_t *mdvSurface;

		// backEnd stats
		int numIndexes;
		int numVerts;

		// static render data
		VBO_t *vbo;
		IBO_t *ibo;
	};

	extern void ( *rb_surfaceTable[Util::ordinal(surfaceType_t::SF_NUM_SURFACE_TYPES)] )(void * );

	void ValidateVertex( srfVert_t* vertex, int vertexID, shader_t* shader );

	/*
	==============================================================================
	BRUSH MODELS - in memory representation
	==============================================================================
	*/
	struct bspSurface_t
	{
		int             viewCount; // if == tr.viewCount, already added

		int scratch1, scratch2;

		struct shader_t *shader;

		int16_t         lightmapNum; // -1 = no lightmap
		int16_t         fogIndex;
		int portalNum;

		bool renderable = false;
		bool BSPModel = false;
		bool skyBrush = false;

		surfaceType_t   *data; // any of srf*_t
	};

#define CONTENTS_NODE -1
	struct bspNode_t
	{
		// common with leaf and node
		int              contents; // -1 for nodes, to differentiate from leafs
		int              visCounts[ MAX_VISCOUNTS ]; // node needs to be traversed if current

		vec3_t           mins, maxs; // for bounding box culling
		bspNode_t *parent;

		// node specific
		cplane_t         *plane;
		bspNode_t *children[ 2 ];

		// leaf specific
		int          cluster;
		int          area;

		int          firstMarkSurface;
		int          numMarkSurfaces;
	};

	struct bspModel_t
	{
		vec3_t       bounds[ 2 ]; // for culling

		uint32_t     numSurfaces;
		bspSurface_t *firstSurface;
	};

	// The ambient and directional colors are packed into four bytes, the color[3] is the
	// average of the ambient and directional colors and the ambientPart factor is the
	// proportion of ambient light in the total light
	struct bspGridPoint1_t
	{
		byte  color[3];
		byte  ambientPart;
	};
	struct bspGridPoint2_t
	{
		byte  direction[3];
		byte  unused;
	};

	struct AABB {
		vec3_t origin;
		vec3_t mins;
		vec3_t maxs;
	};

	struct world_t
	{
		char          name[ MAX_QPATH ]; // ie: maps/tim_dm2.bsp
		char          baseName[ MAX_QPATH ]; // ie: tim_dm2

		uint32_t      dataSize;

		int           numShaders;
		dshader_t     *shaders;

		int           numModels;
		bspModel_t    *models;

		int           numplanes;
		cplane_t      *planes;

		int           numnodes; // includes leafs
		int           numDecisionNodes;
		bspNode_t     *nodes;

		int numPortals;
		AABB *portals;

		VBO_t         *vbo;
		IBO_t         *ibo;

		int                numSurfaces;
		bspSurface_t       *surfaces;

		int                numMarkSurfaces;
		bspSurface_t       **markSurfaces;

		bspSurface_t       **viewSurfaces;

		int                numMergedSurfaces;
		bspSurface_t       *mergedSurfaces;

		int                numFogs;
		fog_t              *fogs;

		int                globalFog; // Arnout: index of global fog

		vec3_t             lightGridOrigin;
		vec3_t             lightGridSize;
		vec3_t             lightGridInverseSize;
		vec3_t             lightGridGLOrigin;
		vec3_t             lightGridGLScale;
		int                lightGridBounds[ 3 ];
		bspGridPoint1_t    *lightGridData1;
		bspGridPoint2_t    *lightGridData2;
		int                numLightGridPoints;

		int                numClusters;

		int                clusterBytes;
		const byte         *vis; // may be passed in by CM_LoadMap to save space
		byte       *visvis; // clusters visible from visible clusters
		byte               *novis; // clusterBytes of 0xff

		char     *entityString;
		const char     *entityParsePoint;

		bool hasSkyboxPortal;
	};

#define REFLECTION_CUBEMAP_VERSION 0

	/*
	==============================================================================
	MDV MODELS - meta format for vertex animation models like .md2, .md3, .mdc
	==============================================================================
	*/
	struct mdvFrame_t
	{
		float bounds[ 2 ][ 3 ];
		float localOrigin[ 3 ];
		float radius;
	};

	struct mdvTag_t
	{
		float origin[ 3 ];
		float axis[ 3 ][ 3 ];
	};

	struct mdvTagName_t
	{
		char name[ MAX_QPATH ]; // tag name
	};

	struct mdvXyz_t
	{
		vec3_t xyz;
	};

	struct mdvNormal_t
	{
		vec3_t normal;
	};

	struct mdvSt_t
	{
		float st[ 2 ];
	};

	struct mdvSurface_t
	{
		surfaceType_t     surfaceType;

		char              name[ MAX_QPATH ]; // polyset name

		shader_t          *shader;

		int               numVerts;
		mdvXyz_t          *verts;
		mdvNormal_t       *normals;
		mdvSt_t           *st;

		int               numTriangles;
		srfTriangle_t     *triangles;

		struct mdvModel_t *model;
	};

	struct mdvModel_t
	{
		int             numFrames;
		mdvFrame_t      *frames;

		int             numTags;
		mdvTag_t        *tags;
		mdvTagName_t    *tagNames;

		int             numSurfaces;
		mdvSurface_t    *surfaces;

		int             numVBOSurfaces;
		srfVBOMDVMesh_t **vboSurfaces;

		int             numSkins;
	};

	/*
	==============================================================================
	MD5 MODELS - in memory representation
	==============================================================================
	*/
#define MD5_IDENTSTRING "MD5Version"
#define MD5_VERSION     10

	struct md5Weight_t
	{
		uint8_t boneIndex; // these are indexes into the boneReferences,
		float   boneWeight; // not the global per-frame bone list
		vec3_t  offset;
	};

	// align for sse skinning
	struct alignas(16) md5Vertex_t
	{
		vec4_t      position;
		vec4_t      tangent;
		vec4_t      binormal;
		vec4_t      normal;

		vec2_t texCoords;
		uint32_t    firstWeight;
		uint32_t    numWeights;

		uint32_t    boneIndexes[ MAX_WEIGHTS ];
		float       boneWeights[ MAX_WEIGHTS ];
	};

	struct md5Surface_t
	{
		surfaceType_t surfaceType;

		char              shader[ MAX_QPATH ];
		int               shaderIndex; // for in-game use

		uint32_t          numVerts;
		md5Vertex_t       *verts;

		uint32_t          numTriangles;
		srfTriangle_t     *triangles;

		uint32_t          numWeights;
		md5Weight_t       *weights;

		struct md5Model_t *model;
	};

	struct md5Bone_t
	{
		char     name[ MAX_QPATH ];
		int8_t   parentIndex; // parent index (-1 if root)
		vec3_t   origin;
		quat_t   rotation;

		// Precompute transform like IQM.
		transform_t joint;
	};

	struct md5Model_t
	{
		uint16_t        numBones;
		md5Bone_t       *bones;

		uint16_t        numSurfaces;
		md5Surface_t    *surfaces;

		uint16_t        numVBOSurfaces;
		srfVBOMD5Mesh_t **vboSurfaces;

		vec3_t          bounds[ 2 ];
		float		internalScale;
	};

	enum class animType_t
	{
	  AT_BAD,
	  AT_MD5,
	  AT_IQM,
	};

	enum
	{
	  COMPONENT_BIT_TX = 1 << 0,
	  COMPONENT_BIT_TY = 1 << 1,
	  COMPONENT_BIT_TZ = 1 << 2,
	  COMPONENT_BIT_QX = 1 << 3,
	  COMPONENT_BIT_QY = 1 << 4,
	  COMPONENT_BIT_QZ = 1 << 5
	};

	struct md5Channel_t
	{
		char     name[ MAX_QPATH ];
		int8_t   parentIndex;

		uint8_t  componentsBits; // e.g. (COMPONENT_BIT_TX | COMPONENT_BIT_TY | COMPONENT_BIT_TZ)
		uint16_t componentsOffset;

		vec3_t   baseOrigin;
		quat_t   baseQuat;
	};

	struct md5Frame_t
	{
		vec3_t bounds[ 2 ]; // bounds of all surfaces of all LODs for this frame
		float  *components; // numAnimatedComponents many
	};

	struct md5Animation_t
	{
		uint16_t     numFrames;
		md5Frame_t   *frames;

		uint8_t      numChannels; // same as numBones in model
		md5Channel_t *channels;

		int16_t      frameRate;

		uint32_t     numAnimatedComponents;
	};

	//======================================================================
	// inter-quake-model format
	//======================================================================
	using iqmHeader_t = struct iqmheader;
	using iqmMesh_t = struct iqmmesh;
	using iqmTriangle_t = struct iqmtriangle;
	using iqmAdjacency_t = struct iqmadjacency;
	using iqmJoint_t = struct iqmjoint;
	using iqmPose_t = struct iqmpose;
	using iqmAnim_t = struct iqmanim;
	using iqmVertexArray_t = struct iqmvertexarray;
	using iqmBounds_t = struct iqmbounds;

	struct IQModel_t {
		int             num_vertexes;
		int             num_triangles;
		int             num_frames;
		int             num_surfaces;
		int             num_joints;
		int             num_anims;

		struct srfIQModel_t     *surfaces;
		struct IQAnim_t         *anims;

		vec3_t          bounds[2];
		float		internalScale;

		// vertex data
		float           *positions;
		float           *normals;
		float           *tangents;
		float           *bitangents;
		float           *texcoords;
		byte            *blendIndexes;
		byte            *blendWeights;
		byte            *colors;
		int             *triangles;

		// skeleton data
		int             *jointParents;
		transform_t     *joints;
		char            *jointNames;
	};

	struct IQAnim_t {
		int             num_frames;
		int             num_joints;
		int             framerate;
		int             flags;

		// skeleton data
		int             *jointParents;
		transform_t     *poses;
		float           *bounds;
		char            *name;
		char            *jointNames;
	};

	// inter-quake-model surface
	struct srfIQModel_t {
		surfaceType_t   surfaceType;
		char            *name;
		shader_t        *shader;
		IQModel_t       *data;
		int             first_vertex, num_vertexes;
		int             first_triangle, num_triangles;

		VBO_t *vbo;
		IBO_t *ibo;
	};

	struct skelAnimation_t
	{
		char           name[ MAX_QPATH ]; // game path, including extension
		animType_t     type;
		int            index; // anim = tr.animations[anim->index]

		union {
			md5Animation_t *md5;
			IQAnim_t       *iqm;
		};
	};

	struct skelTriangle_t
	{
		int         indexes[ 3 ];
		md5Vertex_t *vertexes[ 3 ];
		bool    referenced;
	};

//======================================================================

	enum class modtype_t
	{
	  MOD_BAD,
	  MOD_BSP,
	  MOD_MESH,
	  MOD_MD5,
	  MOD_IQM
	};

	struct model_t
	{
		char        name[ MAX_QPATH ];
		modtype_t   type;
		int         index; // model = tr.models[model->index]

		int         dataSize; // just for listing purposes
		union {
			bspModel_t  *bsp; // only if type == MOD_BSP
			mdvModel_t  *mdv[ MD3_MAX_LODS ]; // only if type == MOD_MESH
			md5Model_t  *md5; // only if type == MOD_MD5
			IQModel_t   *iqm; // only if type == MOD_IQM
		};

		int         numLods;
	};

	void               R_ModelInit();
	model_t            *R_GetModelByHandle( qhandle_t hModel );

	int                RE_LerpTagET( orientation_t *tag, const refEntity_t *refent, const char *tagNameIn, int startIndex );

	int                RE_BoneIndex( qhandle_t hModel, const char *boneName );

	void               R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

//====================================================
	extern refimport_t ri;

	extern int gl_filter_min, gl_filter_max;

	struct frontEndCounters_t
	{
		int c_box_cull_in, c_box_cull_clip, c_box_cull_out;
		int c_plane_cull_in, c_plane_cull_out;

		int c_sphere_cull_mdv_in, c_sphere_cull_mdv_clip, c_sphere_cull_mdv_out;
		int c_box_cull_mdv_in, c_box_cull_mdv_clip, c_box_cull_mdv_out;
		int c_box_cull_md5_in, c_box_cull_md5_clip, c_box_cull_md5_out;

		int c_nodes;
		int c_leafs;
	};

#define FOG_TABLE_SIZE  256
#define FUNCTABLE_SIZE  1024
#define FUNCTABLE_SIZE2 10
#define FUNCTABLE_MASK  ( FUNCTABLE_SIZE - 1 )

#define MAX_GLSTACK     5

// the renderer front end should never modify glState_t
	struct glstate_t
	{
		int    blendSrc, blendDst;
		float  clearColorRed, clearColorGreen, clearColorBlue, clearColorAlpha;
		double clearDepth;
		int    clearStencil;
		int    colorMaskRed, colorMaskGreen, colorMaskBlue, colorMaskAlpha;
		int    depthFunc;
		int    depthMask;
		int    drawBuffer;
		int    frontFace;
		int    polygonFace, polygonMode;
		int    scissorX, scissorY, scissorWidth, scissorHeight;
		int    viewportX, viewportY, viewportWidth, viewportHeight;
		float  polygonOffsetFactor, polygonOffsetUnits;
		vec2_t tileStep;

		int    currenttmu;

		int stackIndex;
		matrix_t        modelViewMatrix[ MAX_GLSTACK ];
		matrix_t        projectionMatrix[ MAX_GLSTACK ];
		matrix_t        modelViewProjectionMatrix[ MAX_GLSTACK ];

		bool        finishCalled;
		cullType_t      faceCulling;
		uint32_t        glStateBits;
		uint32_t        glStateBitsMask; // GLS_ bits set to 1 will not be changed in GL_State
		uint32_t        vertexAttribsState;
		uint32_t        vertexAttribPointersSet;
		float           vertexAttribsInterpolation; // 0 = no interpolation, 1 = final position
		uint32_t        vertexAttribsNewFrame; // offset for VBO vertex animations
		uint32_t        vertexAttribsOldFrame; // offset for VBO vertex animations
		ShaderProgramDescriptor* currentProgram;
		FBO_t           *currentFBO;
		VBO_t           *currentVBO;
		IBO_t           *currentIBO;
		image_t         *colorgradeSlots[ REF_COLORGRADE_SLOTS ];
	};

	struct backEndCounters_t
	{
		int   c_views;
		int   c_portals;
		int   c_batches;
		int   c_surfaces;
		int   c_vertexes;
		int   c_indexes;
		int   c_drawElements;
		int   c_vboVertexBuffers;
		int   c_vboIndexBuffers;
		int   c_vboVertexes;
		int   c_vboIndexes;

		int   c_fogSurfaces;
		int   c_fogBatches;

		int   c_multiDrawElements;
		int   c_multiDrawPrimitives;
		int   c_multiVboIndexes;

		int   msec; // total msec for backend run
	};

	// for the backend to keep track of vis tests
	struct visTestQueries_t
	{
		GLuint   hQuery, hQueryRef;
		bool running;
	};

// all state modified by the back end is separated
// from the front end state
	struct backEndState_t
	{
		int               smpFrame;
		trRefdef_t        refdef;
		viewParms_t       viewParms;
		orientationr_t    orientation;
		backEndCounters_t pc;
		visTestQueries_t  visTestQueries[ MAX_VISTESTS ];
		bool          isHyperspace;
		trRefEntity_t     *currentEntity;
		bool dirtyDepthBuffer;
		bool postDepthLightTileRendered = false;

		bool          projection2D; // if true, drawstretchpic doesn't need to change modes
		Color::Color32Bit color2D;
		trRefEntity_t     entity2D; // currentEntity will point at this when doing 2D rendering
		int               currentMainFBO;
		GLuint currentVAO;
		GLuint defaultVAO;
	};

	struct visTest_t
	{
		vec3_t   position;
		float    depthAdjust;
		float    area;
		bool registered;
		float    lastResult;
	};

	struct visTestResult_t
	{
		qhandle_t visTestHandle;
		vec3_t    position;
		float     depthAdjust; // move position this distance to camera
		float     area; // size of the quad used to test vis

		bool  discardExisting; // true if the currently running vis test should be discarded

		float     lastResult; // updated by backend
	};

	struct cubemapProbe_t
	{
		vec3_t  origin;
		image_t *cubemap;
	};

	template<class T>
	class Grid {
		public:
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		uint32_t size;
		const bool clampToEdge;

		Grid( const bool newClampToEdge ) :
			clampToEdge( newClampToEdge ) {
		}

		Grid( const uint32_t newWidth, const uint32_t newHeight, const uint32_t newDepth, const bool newClampToEdge ) :
			width( newWidth ),
			height( newHeight ),
			depth( newDepth ),
			size( width * height * depth ),
			clampToEdge( newClampToEdge ) {
			grid = ( T* ) ri.Hunk_Alloc( size * sizeof( T ), ha_pref::h_low );
		}

		~Grid() {
		}

		T& operator()( uint32_t x, uint32_t  y, uint32_t z ) {
			if ( clampToEdge ) {
				x = Math::Clamp( x, 0u, width - 1 );
				y = Math::Clamp( y, 0u, height - 1 );
				z = Math::Clamp( z, 0u, depth - 1 );
			}

			DAEMON_ASSERT_GE( x, 0u );
			DAEMON_ASSERT_GE( y, 0u );
			DAEMON_ASSERT_GE( z, 0u );

			DAEMON_ASSERT_LT( x, width );
			DAEMON_ASSERT_LT( y, height );
			DAEMON_ASSERT_LT( z, depth );

			return grid[z * width * height + y * width + x];
		}

		T operator()( uint32_t x, uint32_t  y, uint32_t z ) const {
			if ( clampToEdge ) {
				x = Math::Clamp( x, 0u, width - 1 );
				y = Math::Clamp( y, 0u, height - 1 );
				z = Math::Clamp( z, 0u, depth - 1 );
			}

			return *grid[z * width * height + y * width + x];
		}

		T& operator()( uint32_t index ) {
			if ( clampToEdge ) {
				index = Math::Clamp( index, 0u, size - 1 );
			}

			DAEMON_ASSERT_GE( index, 0U );

			DAEMON_ASSERT_LT( index, size );

			return grid[index];
		}

		T operator()( uint32_t index ) const {
			if ( clampToEdge ) {
				index = Math::Clamp( index, 0u, size - 1 );
			}

			return *grid[index];
		}

		void SetSize( const uint32_t newWidth, const uint32_t newHeight, const uint32_t newDepth ) {
			width = newWidth;
			height = newHeight;
			depth = newDepth;
			size = width * height * depth;

			grid = ( T* ) ri.Hunk_Alloc( size * sizeof( T ), ha_pref::h_low );
		}

		void Clear() {
			memset( grid, 0, size * sizeof( T ) );
		}

		struct Iterator {
			T* ptr;

			Iterator( T* newPtr ) :
				ptr( newPtr ) {
			}

			T& operator*() const {
				return *ptr;
			}

			T* operator->() {
				return ptr;
			}

			ptrdiff_t operator-( const Iterator& other ) const {
				return ptr - other.ptr;
			}

			Iterator& operator++() {
				ptr++;
				return *this;
			}

			Iterator operator++( int ) {
				Iterator current = *this;
				++( *this );
				return current;
			}

			friend bool operator==( const Iterator& lhs, const Iterator& rhs ) {
				return lhs.ptr == rhs.ptr;
			}

			friend bool operator!=( const Iterator& lhs, const Iterator& rhs ) {
				return lhs.ptr != rhs.ptr;
			}
		};

		Iterator begin() {
			return Iterator{ &grid[0] };
		}

		Iterator end() {
			return Iterator{ &grid[size] };
		}

		void IndexToCoords( uint32_t index, uint32_t* x, uint32_t* y, uint32_t* z ) {
			*z = index / ( width * height );
			index %= width * height;
			*y = index / width;
			*x = index % width;
		}

		void IteratorToCoords( Iterator it, uint32_t* x, uint32_t* y, uint32_t* z ) {
			const uint32_t index = it.ptr - begin().ptr;
			IndexToCoords( index, x, y, z );
		}

		private:
		T* grid;
	};

	class GLShader;

	struct scissorState_t
	{
		bool status;
		int       x;
		int       y;
		int       w;
		int       h;
	};

	/*
	** trGlobals_t
	**
	** Most renderer globals are defined here.
	** backend functions should never modify any of these fields,
	** but may read fields that aren't dynamically modified
	** by the frontend.
	*/
	struct trGlobals_t
	{
		bool registered; // cleared at shutdown, set at beginRegistration

		int      visIndex;
		int      visClusters[ MAX_VISCOUNTS ];
		int      visCounts[ MAX_VISCOUNTS ]; // incremented every time a new vis cluster is entered

		int      frameCount; // incremented every frame
		int      sceneCount; // incremented every scene
		int      viewCount; // incremented every view (twice a scene if portaled)
		int      viewCountNoReset; // incremented when doing something that visits surfaces and sets their viewCount
		// and every R_MarkFragments call

		int        smpFrame; // toggles from 0 to 1 every endFrame

		int        frameSceneNum; // zeroed at RE_BeginFrame

		bool   worldMapLoaded;
		bool   worldLightMapping;
		bool   worldDeluxeMapping;
		bool   worldHDR_RGBE;
		bool worldLinearizeTexture;
		bool worldLinearizeLightMap;

		floatProcessor_t convertFloatFromSRGB;
		colorProcessor_t convertColorFromSRGB;

		lightMode_t lightMode;
		lightMode_t worldLight;
		lightMode_t modelLight;
		deluxeMode_t worldDeluxe;
		deluxeMode_t modelDeluxe;

		bool worldLoaded;
		world_t    *world;
		std::string loadingMap;

		TextureManager textureManager;

		const byte *externalVisData; // from RE_SetWorldVisData, shared with CM_Load

		// Maximum reported is 192, see https://opengl.gpuinfo.org/displaycapability.php?name=GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
		std::vector<int> currenttextures;

		image_t    *defaultImage;
		image_t    *cinematicImage[ MAX_IN_GAME_VIDEOS ];
		image_t    *whiteImage; // full of 0xff
		image_t    *blackImage; // 0x0 color channels, 0xff alpha channel
		image_t    *flatImage; // use this as default normalmap
		image_t    *blackCubeImage;
		image_t    *whiteCubeImage;

		image_t    *contrastRenderFBOImage;
		image_t    *bloomRenderFBOImage[ 2 ];
		image_t    *currentRenderImage[ 2 ];
		image_t    *currentDepthImage;
		image_t    *depthSamplerImage;
		image_t    *currentRenderImageMSAA;
		image_t    *currentDepthImageMSAA;
		image_t    *depthtile1RenderImage;
		image_t    *depthtile2RenderImage;
		image_t    *lighttileRenderImage;
		image_t    *portalRenderImage;

		// external images
		image_t *colorGradeImage;
		GLuint   colorGradePBO;

		// framebuffer objects
		FBO_t *mainFBO[ 2 ];
		FBO_t *readonlyDepthFBO;
		FBO_t *msaaFBO;
		FBO_t *depthtile1FBO;
		FBO_t *depthtile2FBO;
		FBO_t *lighttileFBO;
		FBO_t *portalRenderFBO; // used with liquidMapping shader
		FBO_t *contrastRenderFBO;
		FBO_t *bloomRenderFBO[ 2 ];

		// vertex buffer objects
		VBO_t *lighttileVBO;

		// internal shaders
		shader_t *defaultShader;
		shader_t *fogEqualShader;
		shader_t *fogLEShader;

		std::vector<image_t *> lightmaps;
		std::vector<image_t *> deluxemaps;

		vec3_t ambientLight;
		bool ambientLightSet = false;

		image_t   *lightGrid1Image;
		image_t   *lightGrid2Image;

		// render entities
		trRefEntity_t *currentEntity;
		trRefEntity_t worldEntity; // point currentEntity at this when rendering world
		model_t       *currentModel;

		// -----------------------------------------

		viewParms_t    viewParms;

		// Brightness scaling: roughly speaking, a lightmap value of x will be interpreted as
		// min(x * pow(2, mapOverBrightBits), pow(2, overbrightBits))
		// (but when a component hits the max allowed value, others are scaled down to keep the "same color")
		int mapOverBrightBits;
		// min(r_overbrightBits.Get(), mapOverBrightBits)
		int overbrightBits;
		// pow(2, overbrightBits), unless r_overbrightQ3 is on
		float mapLightFactor;
		// 1/pow(2, overbrightBits) if r_overbrightQ3 is on
		float identityLight;

		orientationr_t orientation; // for current entity

		trRefdef_t     refdef;

		// Generic shapes
		drawSurf_t* genericQuad;
		drawSurf_t* genericTriangle;

		bool           hasSkybox;
		bool           drawingSky = false;
		drawSurf_t     *skybox;

		vec3_t         sunLight; // from the sky shader for this level
		vec3_t         sunDirection;

		frontEndCounters_t pc;
		int                frontEndMsec; // not in pc due to clearing issue

		bool skipSubgroupProfiler = false;
		bool skipVBO = false;

		vec4_t             clipRegion; // 2D clipping region

		//
		// put large tables at the end, so most elements will be
		// within the +/32K indexed range on risc processors
		//
		model_t         *models[ MAX_MOD_KNOWN ];
		int             numModels;

		int             numAnimations;
		skelAnimation_t *animations[ MAX_ANIMATIONFILES ];

		std::vector<image_t *> images;

		unsigned numGeneratedTextures;

		int             numFBOs;
		FBO_t           *fbos[ MAX_FBOS ];

		GLuint          dlightUBO;

		std::vector<VBO_t *> vbos;
		std::vector<IBO_t *> ibos;

		byte *cubeTemp[ 6 ]; // 6 textures for cubemap storage
		std::vector<cubemapProbe_t> cubeProbes; // all cubemaps in a linear growing list
		Grid<uint32_t> cubeProbeGrid{ true };
		uint32_t cubeProbeSpacing;

		// shader indexes from other modules will be looked up in tr.shaders[]
		// shader indexes from drawsurfs will be looked up in sortedShaders[]
		// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
		int           numShaders;
		shader_t      *shaders[ MAX_SHADERS ];
		shader_t      *sortedShaders[ MAX_SHADERS ];

		int           numSkins;
		skin_t        *skins[ MAX_SKINS ];

		int           numTables;
		shaderTable_t *shaderTables[ MAX_SHADER_TABLES ];

		int           numVisTests;
		visTest_t     visTests[ MAX_VISTESTS ];

		float         sinTable[ FUNCTABLE_SIZE ];
		float         squareTable[ FUNCTABLE_SIZE ];
		float         triangleTable[ FUNCTABLE_SIZE ];
		float         sawToothTable[ FUNCTABLE_SIZE ];
		float         inverseSawToothTable[ FUNCTABLE_SIZE ];

		scissorState_t scissor;
	};

	extern const matrix_t quakeToOpenGLMatrix;
	extern const matrix_t openGLToQuakeMatrix;
	extern const matrix_t flipZMatrix;

	extern backEndState_t backEnd;
	extern trGlobals_t    tr;

	extern glstate_t      glState; // outside of TR since it shouldn't be cleared during ref re-init

//
// cvars
//
	extern Cvar::Cvar<int> r_rendererAPI;

	extern cvar_t *r_glMajorVersion; // override GL version autodetect (for testing)
	extern cvar_t *r_glMinorVersion;
	extern cvar_t *r_glProfile;
	extern Cvar::Cvar<bool> r_glDebugProfile;
	extern Cvar::Range<Cvar::Cvar<int>> r_glDebugMode;
	extern cvar_t *r_glAllowSoftware;
	extern cvar_t *r_glExtendedValidation;

	extern cvar_t *r_ignore; // used for debugging anything

	extern Cvar::Cvar<bool> r_dpBlend;

	extern cvar_t *r_znear; // near Z clip plane
	extern cvar_t *r_zfar;

	extern cvar_t *r_colorbits; // number of desired color bits, only relevant for fullscreen

	extern cvar_t *r_lodBias; // push/pull LOD transitions
	extern cvar_t *r_lodScale;

	extern cvar_t *r_wolfFog;
	extern cvar_t *r_noFog;

	extern Cvar::Range<Cvar::Cvar<float>> r_forceAmbient;
	extern Cvar::Cvar<float> r_ambientScale;

	extern Cvar::Cvar<bool> r_drawSky; // Controls whether sky should be drawn or cleared.
	extern Cvar::Cvar<bool> r_realtimeLighting;
	extern Cvar::Range<Cvar::Cvar<int>> r_realtimeLightLayers;
	extern cvar_t *r_precomputedLighting;
	extern Cvar::Cvar<int> r_overbrightDefaultExponent;
	extern Cvar::Range<Cvar::Cvar<int>> r_overbrightBits;
	extern Cvar::Cvar<bool> r_overbrightQ3;
	extern Cvar::Cvar<bool> r_overbrightIgnoreMapSettings;
	extern Cvar::Range<Cvar::Cvar<int>> r_lightMode;
	extern Cvar::Cvar<bool> r_colorGrading;
	extern Cvar::Cvar<bool> r_preferBindlessTextures;
	extern Cvar::Cvar<bool> r_materialSystem;
	extern Cvar::Cvar<bool> r_gpuFrustumCulling;
	extern Cvar::Cvar<bool> r_gpuOcclusionCulling;
	extern Cvar::Cvar<bool> r_materialSystemSkip;
	extern cvar_t *r_lightStyles;
	extern cvar_t *r_exportTextures;
	extern cvar_t *r_heatHaze;
	extern cvar_t *r_noMarksOnTrisurfs;
	extern Cvar::Range<Cvar::Cvar<int>> r_lazyShaders; // 0: build all shaders on program start 1: delay shader build until first map load 2: delay shader build until needed

	extern cvar_t *r_norefresh; // bypasses the ref rendering
	extern cvar_t *r_drawentities; // disable/enable entity rendering
	extern cvar_t *r_drawworld; // disable/enable world rendering
	extern cvar_t *r_drawpolies; // disable/enable world rendering
	extern cvar_t *r_speeds; // various levels of information display
	extern cvar_t *r_novis; // disable/enable usage of PVS
	extern cvar_t *r_nocull;
	extern cvar_t *r_facePlaneCull; // enables culling of planar surfaces with back side test
	extern cvar_t *r_nocurves;

	extern cvar_t *r_mode; // video mode
	extern cvar_t *r_gamma;

	extern Cvar::Cvar<bool> r_toneMapping;
	extern Cvar::Cvar<float> r_toneMappingExposure;
	extern Cvar::Range<Cvar::Cvar<float>> r_toneMappingContrast;
	extern Cvar::Range<Cvar::Cvar<float>> r_toneMappingHighlightsCompressionSpeed;
	extern Cvar::Range<Cvar::Cvar<float>> r_toneMappingHDRMax;
	extern Cvar::Range<Cvar::Cvar<float>> r_toneMappingDarkAreaPointHDR;
	extern Cvar::Range<Cvar::Cvar<float>> r_toneMappingDarkAreaPointLDR;

	extern cvar_t *r_nobind; // turns off binding to appropriate textures
	extern cvar_t *r_singleShader; // make most world faces use default shader
	extern cvar_t *r_picMip; // controls picmip values
	extern cvar_t *r_imageMaxDimension;
	extern cvar_t *r_ignoreMaterialMinDimension;
	extern cvar_t *r_ignoreMaterialMaxDimension;
	extern cvar_t *r_replaceMaterialMinDimensionIfPresentWithMaxDimension;
	extern Cvar::Range<Cvar::Cvar<int>> r_imageFitScreen;
	extern cvar_t *r_finish;
	extern cvar_t *r_drawBuffer;
	extern Cvar::Modified<Cvar::Cvar<std::string>> r_textureMode;
	extern cvar_t *r_offsetFactor;
	extern cvar_t *r_offsetUnits;
	extern Cvar::Cvar<bool> r_depthShaders;

	extern cvar_t *r_physicalMapping;
	extern cvar_t *r_specularExponentMin;
	extern cvar_t *r_specularExponentMax;
	extern cvar_t *r_specularScale;
	extern cvar_t *r_specularMapping;
	extern cvar_t *r_deluxeMapping;
	extern cvar_t *r_normalScale;
	extern cvar_t *r_normalMapping;
	extern cvar_t *r_highQualityNormalMapping;
	extern cvar_t *r_liquidMapping;
	extern cvar_t *r_reliefDepthScale;
	extern cvar_t *r_reliefMapping;
	extern cvar_t *r_glowMapping;
	extern Cvar::Cvar<bool> r_reflectionMapping;
	extern Cvar::Range<Cvar::Cvar<int>> r_autoBuildCubeMaps;
	extern Cvar::Range<Cvar::Cvar<int>> r_cubeProbeSize;
	extern Cvar::Range<Cvar::Cvar<int>> r_cubeProbeSpacing;

	extern cvar_t *r_halfLambertLighting;
	extern cvar_t *r_rimLighting;
	extern cvar_t *r_rimExponent;

	extern Cvar::Cvar<bool> r_highPrecisionRendering;
	extern Cvar::Cvar<bool> r_accurateSRGB;

	extern Cvar::Range<Cvar::Cvar<int>> r_shadows;

	extern cvar_t *r_lockpvs;
	extern cvar_t *r_noportals;
	extern cvar_t *r_max_portal_levels;

	extern cvar_t *r_subdivisions;
	extern cvar_t *r_stitchCurves;

	extern cvar_t *r_smp;
	extern cvar_t *r_showSmp;
	extern cvar_t *r_skipBackEnd;

	extern cvar_t *r_checkGLErrors;
	extern Cvar::Cvar<bool> r_logFile;

	extern cvar_t *r_debugSurface;

	extern cvar_t *r_showImages;
	extern cvar_t *r_debugSort;

	extern cvar_t *r_printShaders;

	extern cvar_t *r_maxPolys;
	extern cvar_t *r_maxPolyVerts;

	extern cvar_t *r_showTris; // enables wireframe rendering of the world
	extern cvar_t *r_showSky; // forces sky in front of all surfaces
	extern cvar_t *r_showSkeleton;
	extern cvar_t *r_showLightGrid;
	extern cvar_t *r_showLightTiles;
	extern cvar_t *r_showBatches;
	extern Cvar::Cvar<bool> r_showVertexColors;
	extern cvar_t *r_showLightMaps; // render lightmaps only
	extern cvar_t *r_showDeluxeMaps;
	extern cvar_t *r_showNormalMaps;
	extern cvar_t *r_showMaterialMaps;
	extern Cvar::Cvar<bool> r_showReflectionMaps;
	extern Cvar::Range<Cvar::Cvar<int>> r_showCubeProbes;
	extern Cvar::Cvar<int> r_showCubeProbeFace;
	extern cvar_t *r_showBspNodes;
	extern Cvar::Range<Cvar::Cvar<int>> r_showGlobalMaterials;
	extern Cvar::Cvar<bool> r_materialDebug;

	extern Cvar::Cvar<int> r_forceRendererTime;

	extern Cvar::Cvar<bool> r_profilerRenderSubGroups;
	extern Cvar::Range<Cvar::Cvar<int>> r_profilerRenderSubGroupsMode;
	extern Cvar::Cvar<int> r_profilerRenderSubGroupsStage;

	extern cvar_t *r_vboFaces;
	extern cvar_t *r_vboCurves;
	extern cvar_t *r_vboTriangles;
	extern Cvar::Cvar<bool> r_vboModels;
	extern cvar_t *r_vboVertexSkinning;

	extern cvar_t *r_mergeLeafSurfaces;

	extern Cvar::Cvar<bool> r_bloom;
	extern Cvar::Cvar<float> r_bloomBlur;
	extern Cvar::Cvar<int> r_bloomPasses;
	extern cvar_t *r_FXAA;
	extern Cvar::Range<Cvar::Cvar<int>> r_ssao;
	extern Cvar::Range<Cvar::Cvar<int>> r_msaa;

	extern cvar_t *r_evsmPostProcess;

//====================================================================

/* r_checkGLErrors values:
- -1: check for GL errors on debug build only
- 0: never check for GL errors
- 1: always check for GL errors */
inline bool checkGLErrors()
{
#ifdef DEBUG_BUILD
	return r_checkGLErrors->integer != 0;
#else
	return r_checkGLErrors->integer > 0;
#endif
}

#define IMAGE_FILE_HASH_SIZE 4096
	extern image_t *r_imageHashTable[ IMAGE_FILE_HASH_SIZE ];

	unsigned int   GenerateImageHashValue( const char *fname );

	float          R_NoiseGet4f( float x, float y, float z, float t );
	void           R_NoiseInit();

	int            PortalOffScreenOrOutOfRange( const drawSurf_t* drawSurf, screenRect_t& surfRect );
	bool           R_MirrorViewBySurface( drawSurf_t* drawSurf );
	void           R_RenderView( viewParms_t *parms );
	void           R_RenderPostProcess();

	void           R_AddMDVSurfaces( trRefEntity_t *e );

	void           R_AddPolygonSurfaces();

	void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int lightmapNum, int fogNum, bool bspSurface = false, int portalNum = -1 );

	void           R_LocalNormalToWorld( const vec3_t local, vec3_t world );
	void           R_LocalPointToWorld( const vec3_t local, vec3_t world );

	cullResult_t   R_CullBox( vec3_t worldBounds[ 2 ] );
	cullResult_t   R_CullLocalBox( vec3_t bounds[ 2 ] );
	cullResult_t   R_CullLocalPointAndRadius( vec3_t origin, float radius );
	cullResult_t   R_CullPointAndRadius( vec3_t origin, float radius );

	int            R_FogWorldBox( vec3_t bounds[ 2 ] );

	void           R_SetupEntityWorldBounds( trRefEntity_t *ent );

	void           R_RotateEntityForViewParms( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *orien );

	void           R_SetupFrustum2( frustum_t frustum, const matrix_t modelViewProjectionMatrix );
	template<size_t frustumSize>
	inline
	typename std::enable_if<(frustumSize >= frustumBits_t::FRUSTUM_NEAR + 1)>::type
	R_CalcFrustumNearCorners( const plane_t(&frustum)[frustumSize], vec3_t (&corners)[ 4 ] ) {
		extern void R_CalcFrustumNearCornersUnsafe( const plane_t frustum[frustumBits_t::FRUSTUM_NEAR + 1], vec3_t (&corners)[ 4 ] );
		R_CalcFrustumNearCornersUnsafe( frustum, corners );
	}
	template<size_t frustumSize>
	inline
	typename std::enable_if<(frustumSize >= frustumBits_t::FRUSTUM_FAR + 1)>::type
	R_CalcFrustumFarCorners( const plane_t(&frustum)[frustumSize], vec3_t (&corners)[ 4 ] ) {
		extern void R_CalcFrustumFarCornersUnsafe( const plane_t frustum[frustumBits_t::FRUSTUM_FAR + 1], vec3_t (&corners)[ 4 ] );
		R_CalcFrustumFarCornersUnsafe( frustum, corners );
	}

	/* Tangent/normal vector calculation functions */
	void R_CalcFaceNormal( vec3_t normal, const vec3_t v0,
			       const vec3_t v1, const vec3_t v2 );

	void R_CalcTangents( vec3_t tangent, vec3_t binormal,
			     const vec3_t v0, const vec3_t v1, const vec3_t v2,
			     const vec2_t t0, const vec2_t t1, const vec2_t t2 );

	void R_CalcTangents( vec3_t tangent, vec3_t binormal,
			     const vec3_t v0, const vec3_t v1, const vec3_t v2,
			     const f16vec2_t t0, const f16vec2_t t1, const f16vec2_t t2 );

	/*
	 * QTangent representation of tangentspace:
	 *
	 *   A unit quaternion can represent any member of the special
	 *   orthogonal subgroup of 3x3 matrices, i.e. orthogonal matrices
	 *   with determinant 1.  In the general case the TBN vectors may not
	 *   be orthogonal, so they have to be orthogonalized first.
	 *
	 *   Even then the determinant may be -1, so we can only encode the
	 *   TBN we get by flipping the T vector.  Fortunately every
	 *   quaternion and its negative represent the same rotation, so we
	 *   can remember if the matrix has been flipped in the sign of the W
	 *   component.
	 *
	 *   This is not possible if W == 0, i.e. it is a 180 degree rotation,
	 *   so in that case we set W to 1 or -1. This introduces another
	 *   minimal error.
	 */
	void R_TBNtoQtangents( const vec3_t tangent, const vec3_t binormal,
		const vec3_t normal, i16vec4_t qtangent, bool fast = false );

	inline void R_TBNtoQtangentsFast( const vec3_t tangent, const vec3_t binormal,
		const vec3_t normal, i16vec4_t qtangent )
	{
		R_TBNtoQtangents( tangent, binormal, normal, qtangent, true );
	}

	void R_QtangentsToTBN( const i16vec4_t qtangent, vec3_t tangent,
			       vec3_t binormal, vec3_t normal );
	void R_QtangentsToNormal( const i16vec4_t qtangent, vec3_t normal );

// Tr3B - visualisation tools to help debugging the renderer frontend
	void     DebugDrawVertex(const vec3_t pos, unsigned int color, const vec2_t uv);
	void     DebugDrawBegin( debugDrawMode_t mode, float size );
	void     DebugDrawDepthMask(bool state);
	void     DebugDrawEnd();
	void RE_SendBotDebugDrawCommands( std::vector<char> commands );
	/*
	====================================================================

	OpenGL WRAPPERS, tr_backend.cpp

	====================================================================
	*/
	void GL_Bind( image_t *image );
	void GL_Unbind( image_t *image );
	GLuint64 BindAnimatedImage( int unit, const textureBundle_t *bundle );
	void GL_BindProgram( ShaderProgramDescriptor* program );
	GLuint64 GL_BindToTMU( int unit, image_t *image );
	void GL_BindNullProgram();

	void TransitionMainToMSAA( const GLbitfield mask );
	void TransitionMSAAToMain( const GLbitfield mask );
	void BindMSAAOrMainFBO();

	void GL_SetDefaultState();
	void GL_SelectTexture( int unit );
	void GL_TextureMode( const char *string );

	void GL_BlendFunc( GLenum sfactor, GLenum dfactor );
	void GL_ClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
	void GL_ClearDepth( GLclampd depth );
	void GL_ClearStencil( GLint s );
	void GL_ColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
	void GL_DepthFunc( GLenum func );
	void GL_DepthMask( GLboolean flag );
	void GL_DrawBuffer( GLenum mode );
	void GL_FrontFace( GLenum mode );
	void GL_LoadModelViewMatrix( const matrix_t m );
	void GL_LoadProjectionMatrix( const matrix_t m );
	void GL_PushMatrix();
	void GL_PopMatrix();
	void GL_PolygonMode( GLenum face, GLenum mode );
	void GL_Scissor( GLint x, GLint y, GLsizei width, GLsizei height );
	void GL_Viewport( GLint x, GLint y, GLsizei width, GLsizei height );
	void GL_PolygonOffset( float factor, float units );

	void GL_State( uint32_t stateVector );
	void GL_VertexAttribsState( uint32_t stateBits, const bool settingUpVAO = false );
	void GL_Cull( cullType_t cullType );
void GL_TexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *data, bool isSRGB );
void GL_TexImage2DMultisample( GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, bool fixedSampleLocations, bool isSRGB );
void GL_TexImage3D( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *data, bool isSRGB );
void GL_CompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data, bool isSRGB );
void GL_CompressedTexSubImage3D( GLenum target, GLint level, GLint xOffset, GLint yOffset, GLint zOffset, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLsizei size, const void *data, bool isSRGB );
	void R_ShutdownBackend();
	void RB_PrepareForSamplingDepthMap();

	/*
	====================================================================



	====================================================================
	*/

	void      RE_UploadCinematic( int cols, int rows, const byte *data, int client, bool dirty );

	void      RE_BeginFrame();
	bool  RE_BeginRegistration( WindowConfig* windowCfg );
	void      RE_LoadWorldMap( const char *mapname );
	void      RE_SetWorldVisData( const byte *vis );
	qhandle_t RE_RegisterModel( const char *name );
	qhandle_t RE_RegisterSkin( const char *name );
	void      RE_Shutdown( bool destroyWindow );

	void R_ProcessLightmap( byte *bytes, int width, int height, int bits ); // Arnout

	model_t    *R_AllocModel();

	bool   R_Init();

	void AssertCvarRange( cvar_t *cv, float minVal, float maxVal, bool shouldBeIntegral );

	bool   R_GetModeInfo( int *width, int *height, int mode );

	void       R_InitSkins();
	skin_t     *R_GetSkinByHandle( qhandle_t hSkin );

	/*
	====================================================================

	IMAGES, tr_image.c

	====================================================================
	*/
	void    R_InitImages();
	void    R_ShutdownImages();

	bool R_HasImageLoader( const char *baseName );
	image_t *R_FindImageFile( const char *name, imageParams_t &imageParams );
	image_t *R_FindCubeImage( const char *name, imageParams_t &imageParams );

	image_t *R_CreateImage( const char *name, const byte **pic, int width, int height, int numMips, const imageParams_t &imageParams,
		const uint32_t samples = 0, const bool fixedSampleLocations = true );

	image_t *R_CreateCubeImage( const char *name, const byte *pic[ 6 ], int width, int height, const imageParams_t &imageParams );
	image_t *R_Create3DImage( const char *name, const byte *pic, int width, int height, int depth, const imageParams_t &imageParams );

	image_t *R_CreateGlyph( const char *name, const byte *pic, int width, int height );
	qhandle_t RE_GenerateTexture( const byte *pic, int width, int height );

	image_t *R_AllocImage( const char *name, bool linkIntoHashTable );
	void R_UploadImage( const char *name, const byte **dataArray, int numLayers, int numMips, image_t *image, const imageParams_t &imageParams,
		const uint32_t samples = 0, const bool fixedSampleLocations = true );

	void    RE_GetTextureSize( int textureID, int *width, int *height );

	void    RE_SetColorGrading( int slot, qhandle_t hShader );

	/*
	====================================================================

	SHADERS, tr_shader.c

	====================================================================
	*/
	qhandle_t RE_RegisterShader( const char *name, int flags );
	qhandle_t RE_RegisterShaderFromImage( const char *name, image_t *image );

	shader_t  *R_FindShader( const char *name, int flags );
	shader_t  *R_GetShaderByHandle( qhandle_t hShader );
	const char *RE_GetShaderNameFromHandle( qhandle_t shader );
	void      R_InitShaders();
	void      R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );

	/*
	====================================================================

	IMPLEMENTATION SPECIFIC FUNCTIONS

	====================================================================
	*/

	bool GLimp_Init();
	void     GLimp_Shutdown();
	void     GLimp_EndFrame();
	void     GLimp_HandleCvars();

	bool GLimp_SpawnRenderThread( void ( *function )() );
	void     GLimp_ShutdownRenderThread();
	void     *GLimp_RendererSleep();
	void     GLimp_FrontEndSleep();
	void     GLimp_SyncRenderThread();
	void     GLimp_WakeRenderer( void *data );

inline bool GLimp_isLogging() {
	return r_logFile.Get() && GLEW_ARB_debug_output;
}

// Never use GLimp_LogComment_() directly, use the GLIMP_LOGCOMMENT() wrapper instead.
void GLimp_LogComment_( std::string comment );

#define GLIMP_LOGCOMMENT( ... ) { \
	if ( GLimp_isLogging() ) \
	{ \
		GLimp_LogComment_( Str::Format( __VA_ARGS__ ) ); \
	} \
}

	/*
	====================================================================

	TESSELATOR/SHADER DECLARATIONS

	====================================================================
	*/

	struct stageVars_t
	{
		Color::Color color;
		matrix_t texMatrices[ MAX_TEXTURE_BUNDLES ];
	};

#define MAX_MULTIDRAW_PRIMITIVES 1000

	struct shaderVertex_t {
		vec3_t    xyz;
		Color::Color32Bit color;
		i16vec4_t qtangents;

		// the lightmap coords (index 2-3) may be used in some weird cases like an autosprite map surface
		vec4_t texCoords;
	};

	struct glRingbuffer_t {
		// the BO is logically split into DYN_BUFFER_SEGMENTS
		// segments, the active segment is the one the CPU may write
		// into, while the GPU may read from the inactive segments.
		void           *baseAddr;
		GLsizei        elementSize;
		GLsizei        segmentElements;
		int            activeSegment;
		// all syncs except the active segment's should be
		// always defined and waitable, the active segment's
		// sync is always undefined
		GLsync         syncs[ DYN_BUFFER_SEGMENTS ];
	};

	struct shaderCommands_t
	{
		// For drawing which is not based on static VBOs, the data is written here.
		shaderVertex_t *verts;   // at least SHADER_MAX_VERTEXES accessible
		glIndex_t      *indexes; // at least SHADER_MAX_INDEXES accessible

		// When writing into the buffers above, these are used to track how much was written.
		// For some static VBO/IBO-based drawing, these can be used to request a single data range.
		uint32_t    numIndexes;
		uint32_t    numVertexes;

		// Must be set by the stage iterator function if needed. These are *not*
		// automatically cleared by the likes of Tess_End.
		stageVars_t svars;

		shader_t    *surfaceShader;

		// some drawing parameters from drawSurf_t
		int16_t     lightmapNum;
		int16_t     fogNum;
		bool        bspSurface;

		// Signals that ATTR_QTANGENT will not be needed, so functions that generate vertexes
		// may skip writing out shaderVertex_t::qtangents
		bool skipTangents;

		// Used for static VBO/IBO-based drawing, if multiple ranges of data from the
		// buffers may be requested.
		int         multiDrawPrimitives;
		glIndex_t    *multiDrawIndexes[ MAX_MULTIDRAW_PRIMITIVES ];
		int         multiDrawCounts[ MAX_MULTIDRAW_PRIMITIVES ];
		uint        multiDrawOffsets[ MAX_MULTIDRAW_PRIMITIVES ];

		// enabled when a skeletal model VBO is used
		bool    vboVertexSkinning;
		int         numBones;
		transform_t bones[ MAX_BONES ];

		// enabled when an MD3 VBO is used
		bool    vboVertexAnimation;

		// This can be thought of a "flush" function for the vertex buffer.
		// Which function depends on backend mode and also the shader.
		void ( *stageIteratorFunc )();

		shaderStage_t *surfaceStages;    // surfaceShader->stages
		shaderStage_t *surfaceLastStage; // surfaceShader->lastStage

		// preallocated host buffers for verts and indexes
		// NOT mapped to any VBO
		shaderVertex_t *vertsBuffer;
		glIndex_t      *indexesBuffer;

		uint32_t       vertsWritten, vertexBase;
		uint32_t       indexesWritten, indexBase;

		// The "default", "dynamic" VBO/IBO used for data that is generated and sent to the
		// GPU on a per-frame basis
		VBO_t       *vbo;
		IBO_t       *ibo;

		glRingbuffer_t  vertexRB;
		glRingbuffer_t  indexRB;
	};

	alignas(16) extern shaderCommands_t tess;

	void                    GLSL_InitGPUShaders();
	void                    GLSL_InitWorldShaders();
	void                    GLSL_ShutdownGPUShaders();
	void                    GLSL_FinishGPUShaders();

// *INDENT-OFF*
	void Tess_Begin( void ( *stageIteratorFunc )(),
	                 shader_t *surfaceShader,
	                 bool skipTangents,
	                 int lightmapNum,
	                 int fogNum,
	                 bool bspSurface = false );

// *INDENT-ON*
	void Tess_Clear();
	void Tess_End();
	void Tess_EndBegin();
	void Tess_DrawElements();
	void Tess_DrawArrays( GLenum elementType );
	void Tess_CheckOverflow( int verts, int indexes );

	void SetNormalScale( const shaderStage_t* pStage, vec3_t normalScale );
	void Tess_ComputeColor( shaderStage_t *pStage );
	void Tess_ComputeTexMatrices( shaderStage_t* pStage );

	void Tess_StageIteratorDummy();
	void Tess_StageIteratorDebug();
	void Tess_StageIteratorColor();
	void Tess_StageIteratorPortal();
	void Tess_StageIteratorSky();

	void Tess_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, const Color::Color& color );
	void Tess_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, const Color::Color& color, float s1, float t1, float s2, float t2 );

	void Tess_AddQuadStampExt2( vec4_t quadVerts[ 4 ], const Color::Color& color, float s1, float t1, float s2, float t2 );
	void Tess_AddQuadStamp2( vec4_t quadVerts[ 4 ], const Color::Color& color );
	void Tess_AddQuadStamp2WithNormals( vec4_t quadVerts[ 4 ], const Color::Color& color );

	/*
	Add a polyhedron that is composed of four triangular faces

	@param tetraVerts[0..2] are the ground vertices, tetraVerts[3] is the pyramid offset
	*/
	void Tess_AddTetrahedron( vec4_t tetraVerts[ 4 ], const Color::Color& color );

	void Tess_AddCube( const vec3_t position, const vec3_t minSize, const vec3_t maxSize, const Color::Color& color );
	void Tess_AddCubeWithNormals( const vec3_t position, const vec3_t minSize, const vec3_t maxSize, const Color::Color& color );

	class u_ModelViewProjectionMatrix;
	void Tess_InstantScreenSpaceQuad();
	void Tess_InstantQuad( u_ModelViewProjectionMatrix &shader, float x, float y, float width, float height );

	void Tess_MapVBOs( bool forceCPU );
	void Tess_UpdateVBOs();

	void RB_ShowImages();

	void ProcessShaderNONE( const shaderStage_t* );
	void ProcessShaderNOP( const shaderStage_t* );
	void ProcessShaderGeneric3D( const shaderStage_t* pStage );
	void ProcessShaderLightMapping( const shaderStage_t* pStage );
	void ProcessShaderReflection( const shaderStage_t* pStage );
	void ProcessShaderHeatHaze( const shaderStage_t* );
	void ProcessShaderLiquid( const shaderStage_t* pStage );
	void ProcessShaderFog( const shaderStage_t* );

	void Render_NONE( shaderStage_t *pStage );
	void Render_NOP( shaderStage_t *pStage );
	void Render_generic( shaderStage_t *pStage );
	void Render_generic3D( shaderStage_t *pStage );
	void Render_lightMapping( shaderStage_t *pStage );
	void Render_reflection_CB( shaderStage_t *pStage );
	void Render_skybox( shaderStage_t *pStage );
	void Render_screen( shaderStage_t *pStage );
	void Render_portal( shaderStage_t *pStage );
	void Render_heatHaze( shaderStage_t *pStage );
	void Render_liquid( shaderStage_t *pStage );
	void Render_fog( shaderStage_t* pStage );

	/*
	============================================================

	WORLD MAP, tr_world.c

	============================================================
	*/

	void     R_AddBSPModelSurfaces( trRefEntity_t *e );
	void     R_AddWorldSurfaces();
	bspNode_t* R_PointInLeaf( const vec3_t p );
	const byte* R_ClusterPVS( int cluster );
	bool R_inPVS( const vec3_t p1, const vec3_t p2 );
	bool R_inPVVS( const vec3_t p1, const vec3_t p2 );

	/*
	============================================================

	LIGHTS, tr_light.c

	============================================================
	*/

	float R_InterpolateLightGrid( world_t *w, int from[3], int to[3],
				      float *factors[3], vec3_t ambientLight,
				      vec3_t directedLight, vec3_t lightDir );
	int      R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );

	/*
	============================================================

	CURVE TESSELATION, tr_curve.c

	============================================================
	*/

	srfGridMesh_t *R_SubdividePatchToGrid( int width, int height, srfVert_t points[ MAX_PATCH_SIZE *MAX_PATCH_SIZE ] );
	srfGridMesh_t *R_GridInsertColumn( srfGridMesh_t *grid, int column, int row, vec3_t point, float loderror );
	srfGridMesh_t *R_GridInsertRow( srfGridMesh_t *grid, int row, int column, vec3_t point, float loderror );
	void          R_FreeSurfaceGridMesh( srfGridMesh_t *grid );

	/*
	============================================================

	MARKERS, POLYGON PROJECTION ON WORLD POLYGONS, tr_marks.c

	============================================================
	*/

	int R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
	                     int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

	/*
	============================================================

	FRAME BUFFER OBJECTS, tr_fbo.c

	============================================================
	*/
	bool R_CheckFBO( const FBO_t *fbo );

	FBO_t    *R_CreateFBO( const char *name, int width, int height );

	void     R_AttachFBOTexture1D( int texId, int attachmentIndex );
	void     R_AttachFBOTexture2D( int target, int texId, int attachmentIndex );
	void     R_AttachFBOTexture3D( int texId, int attachmentIndex, int zOffset );

	void     R_BindFBO( FBO_t *fbo );
	void     R_BindFBO( GLenum target, FBO_t *fbo );
	void     R_BindNullFBO();

	void     R_InitFBOs();
	void     R_ShutdownFBOs();

	/*
	============================================================

	SCENE GENERATION, tr_scene.c

	============================================================
	*/

	void R_ToggleSmpFrame();

	void RE_ClearScene();
	void RE_AddRefEntityToScene( const refEntity_t *ent );

	void RE_AddPolyToSceneET( qhandle_t hShader, int numVerts, const polyVert_t *verts );
	void RE_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );

	void RE_AddDynamicLightToScene( const vec3_t org, float radius, float r, float g, float b, int flags );

	void RE_RenderScene( const refdef_t *fd );

	qhandle_t RE_RegisterVisTest();
	void RE_AddVisTestToScene( qhandle_t hTest, const vec3_t pos,
				   float depthAdjust, float area );
	float RE_CheckVisibility( qhandle_t hTest );
	void RE_UnregisterVisTest( qhandle_t hTest );
	void R_UpdateVisTests();
	void R_InitVisTests();
	void R_ShutdownVisTests();
	/*
	=============================================================

	ANIMATED MODELS

	=============================================================
	*/

	void      R_InitAnimations();

	qhandle_t RE_RegisterAnimation( const char *name );
	qhandle_t RE_RegisterAnimationIQM( const char *name, IQAnim_t *data );

	skelAnimation_t *R_GetAnimationByHandle( qhandle_t hAnim );

	void            R_AddMD5Surfaces( trRefEntity_t *ent );

	void		R_AddIQMSurfaces( trRefEntity_t *ent );

	int             RE_CheckSkeleton( refSkeleton_t *skel, qhandle_t hModel, qhandle_t hAnim );
	int             RE_BuildSkeleton( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac,
	                                  bool clearOrigin );
	int             RE_BlendSkeleton( refSkeleton_t *skel, const refSkeleton_t *blend, float frac );
	int             RE_AnimNumFrames( qhandle_t hAnim );
	int             RE_AnimFrameRate( qhandle_t hAnim );

	/*
	=============================================================

	ANIMATED MODELS WOLFENSTEIN

	=============================================================
	*/

	void     R_TransformWorldToClip( const vec3_t src, const float *cameraViewMatrix,
	                                 const float *projectionMatrix, vec4_t eye, vec4_t dst );
	void     R_TransformModelToClip( const vec3_t src, const float *modelViewMatrix,
	                                 const float *projectionMatrix, vec4_t eye, vec4_t dst );
	void     R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window );
	float    R_ProjectRadius( float r, vec3_t location );

	void     Tess_AutospriteDeform( int mode );

	float    RB_EvalWaveForm( const waveForm_t *wf );
	float    RB_EvalWaveFormClamped( const waveForm_t *wf );
	float    RB_EvalExpression( const expression_t *exp, float defaultValue );

	void     RB_CalcTexMatrix( const textureBundle_t *bundle, matrix_t matrix );

	/*
	=============================================================

	IN-GAME VIDEO PLAYBACK ON A SURFACE (tr_video.cpp)

	=============================================================
	*/

	using videoHandle_t = int; // 0 is valid; -1 is the invalid handle

	videoHandle_t CIN_PlayCinematic( std::string name );
	bool CIN_RunCinematic( videoHandle_t handle );
	void CIN_UploadCinematic( videoHandle_t handle );
	void CIN_CloseAllVideos();

	/*
	=============================================================

	RENDERER BACK END FUNCTIONS

	=============================================================
	*/

	void RB_RenderThread();
	void RB_ExecuteRenderCommands( const void *data );

	/*
	=============================================================

	RENDERER BACK END COMMAND QUEUE

	=============================================================
	*/

#define MAX_RENDER_COMMANDS ( 0x40000 * 8 ) // Tr3B: was 0x40000

	struct renderCommandList_t
	{
		byte cmds[ MAX_RENDER_COMMANDS ];
		int  used;
	};

	struct RenderCommand {
		virtual ~RenderCommand() = default;

		// returns address of next command or nullptr
		virtual const RenderCommand *ExecuteSelf() const = 0;
	};

	struct SetColorCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		Color::Color color;
	};
	struct SetColorGradingCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		image_t *image;
		int     slot;
	};
	struct DrawBufferCommand : public RenderCommand	{
		const RenderCommand *ExecuteSelf() const override;

		int buffer;
	};
	struct SwapBuffersCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;
	};
	struct StretchPicCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		shader_t *shader;
		float    x, y;
		float    w, h;
		float    s1, t1;
		float    s2, t2;
	};
	struct RotatedPicCommand : public StretchPicCommand {
		const RenderCommand *ExecuteSelf() const override;

		float    angle;
	};
	struct GradientPicCommand : public StretchPicCommand {
		const RenderCommand *ExecuteSelf() const override;

		Color::Color32Bit gradientColor; // color values 0-255
		int               gradientType;
	};
	struct Poly2dCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		polyVert_t *verts;
		int        numverts;
		shader_t   *shader;
	};
	struct Poly2dIndexedCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		polyVert_t *verts;
		int        numverts;
		int        *indexes;
		int        numIndexes;
		shader_t   *shader;
		int         translation[2];
	};
	struct ScissorSetCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		int       x;
		int       y;
		int       w;
		int       h;
	};
	struct SetMatrixTransformCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		matrix_t matrix;
	};
	struct ResetMatrixTransformCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;
	};
	struct DrawViewCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		trRefdef_t  refdef;
		viewParms_t viewParms;
		bool        depthPass;
	};
	struct SetupLightsCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		trRefdef_t  refdef;
	};
	enum class ssFormat_t
	{
	  SSF_TGA,
	  SSF_JPEG,
	  SSF_PNG
	};
	struct ScreenshotCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		int        x;
		int        y;
		int        width;
		int        height;
		char       fileName[MAX_OSPATH];
		ssFormat_t format;
	};
	struct VideoFrameCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		int      width;
		int      height;
		byte     *captureBuffer;
		byte     *encodeBuffer;
		bool motionJpeg;
	};
	struct RenderPostProcessCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		trRefdef_t      refdef;
		viewParms_t     viewParms;
	};
	struct ClearBufferCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		trRefdef_t      refdef;
		viewParms_t     viewParms;
	};
	struct PreparePortalCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		trRefdef_t      refdef;
		viewParms_t     viewParms;
		drawSurf_t     *surface;
	};
	struct FinalisePortalCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;

		trRefdef_t      refdef;
		viewParms_t     viewParms;
		drawSurf_t     *surface;
	};
	struct EndOfListCommand : public RenderCommand {
		const RenderCommand *ExecuteSelf() const override;
	};

// all of the information needed by the back end must be
// contained in a backEndData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine
	struct backEndData_t
	{
		drawSurf_t          drawSurfs[ MAX_DRAWSURFS ];

		refLight_t          lights[ MAX_REF_LIGHTS ];
		trRefEntity_t       entities[ MAX_REF_ENTITIES ];

		srfPoly_t           *polys; //[MAX_POLYS];
		polyVert_t          *polyVerts; //[MAX_POLYVERTS];
		int                 *polyIndexes; //[MAX_POLYVERTS];

		// the backend communicates to the frontend through visTestResult_t
		int                 numVisTests;
		visTestResult_t     visTests[ MAX_VISTESTS ];

		bspNode_t			**traversalList;
		int                 traversalLength;

		renderCommandList_t commands;
	};

	extern backEndData_t                *backEndData[ SMP_FRAMES ]; // the second one may not be allocated

	extern volatile bool            renderThreadActive;

	void *                              R_GetCommandBuffer( size_t bytes );
	template <class T> T *              R_GetRenderCommand() { return new (R_GetCommandBuffer(sizeof(T))) T; }
	void                                RB_ExecuteRenderCommands( const void *data );

	void                                R_SyncRenderThread();

	void                                R_AddSetupLightsCmd();
	void                                R_AddDrawViewCmd( bool depthPass );
	void                                R_AddClearBufferCmd();
	void                                R_AddPreparePortalCmd( drawSurf_t *surf );
	void                                R_AddFinalisePortalCmd( drawSurf_t *surf );
	void                                R_AddPostProcessCmd();

	void                                RE_SetColor( const Color::Color& rgba );
	void                                RE_SetClipRegion( const float *region );
	void                                RE_StretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
	void                                RE_RotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );  // NERVE - SMF
	void                                RE_StretchPicGradient( float x, float y, float w, float h,
	    float s1, float t1, float s2, float t2, qhandle_t hShader, const Color::Color& gradientColor,
	    int gradientType );
	void                                RE_2DPolyies( polyVert_t *verts, int numverts, qhandle_t hShader );
	void                                RE_2DPolyiesIndexed( polyVert_t *verts, int numverts, int *indexes, int numindexes, int trans_x, int trans_y, qhandle_t hShader );
	void                                RE_ScissorEnable( bool enable );
	void                                RE_ScissorSet( int x, int y, int w, int h );
	void                                RE_SetMatrixTransform( const matrix_t matrix );
	void                                RE_ResetMatrixTransform();

	void                                RE_BeginFrame();
	void                                RE_EndFrame( int *frontEndMsec, int *backEndMsec );

	void                                LoadTGA( const char *name, byte **pic, int *width, int *height, int *numLayers, int *numMips, int *bits, byte alphaByte );

	void                                LoadJPG( const char *filename, unsigned char **pic, int *width, int *height, int *numLayers, int *numMips, int *bits, byte alphaByte );
	void                                SaveJPG( const char *filename, int quality, int image_width, int image_height, unsigned char *image_buffer );
	int                                 SaveJPGToBuffer( byte *buffer, size_t bufferSize, int quality, int image_width, int image_height, byte *image_buffer );

	void                                LoadPNG( const char *name, byte **pic, int *width, int *height, int *numLayers, int *numMips, int *bits, byte alphaByte );
	void                                SavePNG( const char *name, const byte *pic, int width, int height, int numBytes, bool flip );

	void                                LoadWEBP( const char *name, byte **pic, int *width, int *height, int *numLayers, int *numMips, int *bits, byte alphaByte );
	void                                LoadDDS( const char *name, byte **pic, int *width, int *height, int *numLayers, int *numMips, int *bits, byte alphaByte);
	void                                LoadCRN( const char *name, byte **pic, int *width, int *height, int *numLayers, int *numMips, int *bits, byte alphaByte);
	void                                LoadKTX( const char *name, byte **pic, int *width, int *height, int *numLayers, int *numMips, int *bits, byte alphaByte);
	void                                SaveImageKTX( const char *name, image_t *img );


// video stuff
	void       RE_TakeVideoFrame( int width, int height, byte *captureBuffer, byte *encodeBuffer, bool motionJpeg );

// cubemap reflections stuff
	void R_BuildCubeMaps();
	void R_GetNearestCubeMaps( const vec3_t position, cubemapProbe_t** cubeProbes, vec4_t trilerp,
		const uint8_t samples, vec3_t *gridPoints = nullptr );

// font stuff
	void       R_InitFreeType();
	void       R_DoneFreeType();
	fontInfo_t* RE_RegisterFont( const char *fontName, int pointSize );
	void       RE_UnregisterFont( fontInfo_t *font );
	void       RE_GlyphChar(fontInfo_t *font, int ch, glyphInfo_t *glyph);

	void       R_SetAltShaderTokens( const char * );

#endif // TR_LOCAL_H
