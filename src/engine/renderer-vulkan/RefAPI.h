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
// RefAPI.h

#ifndef REFAPI_H
#define REFAPI_H

#include "common/Common.h"

#define REF_API_VERSION 10

using bool8_t = uint8_t;

enum class textureCompression_t {
	TC_NONE,
	TC_S3TC,
	TC_EXT_COMP_S3TC
};

// Keep the list in sdl_glimp.c:reportDriverType in sync with this
enum class glDriverType_t {
	GLDRV_UNKNOWN = -1,
	GLDRV_ICD, // driver is integrated with window system
	// WARNING: there are tests that check for
	// > GLDRV_ICD for minidriverness, so this
	// should always be the lowest value in this
	// enum set
	GLDRV_STANDALONE, // driver is a non-3Dfx standalone driver

	// XreaL BEGIN
	GLDRV_OPENGL3, // new driver system
	// XreaL END
};

// Keep the list in sdl_glimp.c:reportHardwareType in sync with this
enum class glHardwareType_t {
	GLHW_UNKNOWN = -1,
	GLHW_GENERIC, // where everthing works the way it should

	// XreaL BEGIN
	GLHW_R300, // pre-GL3 ATI hack
	// XreaL END
};

struct glconfig_t {
	char                 renderer_string[MAX_STRING_CHARS];
	char                 vendor_string[MAX_STRING_CHARS];
	char                 version_string[MAX_STRING_CHARS];

	int                  maxTextureSize; // queried from GL

	int colorBits;

	glDriverType_t       driverType;
	glHardwareType_t     hardwareType;

	textureCompression_t textureCompression;

	int displayIndex;
	float displayAspect;
	int displayWidth, displayHeight; // the entire monitor (the one indicated by displayIndex)
	int vidWidth, vidHeight; // what the game is using

	bool8_t smpActive; // dual processor
};

struct glconfig2_t {};

struct refBone_t {
#if defined( REFBONE_NAMES )
	char   name[64];
#endif
	short  parentIndex; // parent index (-1 if root)
	transform_t t;
};

enum class refSkeletonType_t {
	SK_INVALID,
	SK_RELATIVE,
	SK_ABSOLUTE
};

#define MAX_BONES 256

struct alignas( 16 ) refSkeleton_t {
	refSkeletonType_t type; // skeleton has been reset

	unsigned short numBones;

	vec3_t            bounds[2]; // bounds of all applied animations
	vec_t             scale;

	refBone_t         bones[MAX_BONES];
};

// XreaL END

enum class refEntityType_t {
	RT_MODEL,

	// A square 3D polygon at the specified `origin` with size `radius` with its face
	// oriented toward the viewer. By default the square's edges will be aligned so they are
	// drawn parallel to the edges of the screen; this can be changed by setting `rotation`.
	RT_SPRITE,

	RT_PORTALSURFACE, // doesn't draw anything, just info for portals

	RT_MAX_REF_ENTITY_TYPE
};

struct refEntity_t {
	refEntityType_t reType;
	int             renderfx;

	qhandle_t       hModel; // opaque type outside refresh

	// most recent data
	vec3_t    lightingOrigin; // so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)

	vec3_t    axis[3]; // rotation vectors
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
	float shaderTexCoord[2]; // texture coordinates used by tcMod entity modifiers
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

struct refdef_t {
	int    x, y, width, height;
	float  fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3]; // transformation matrix
	vec3_t blurVec;       // motion blur direction

	int    time; // time in milliseconds for shader effects and other time dependent rendering issues
	int    rdflags; // RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];

	vec4_t  gradingWeights;
};

struct polyVert_t {
	vec3_t xyz;
	float st[2];
	byte modulate[4];
};

struct refexport_t
{
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = false,
	// which will keep the screen from flashing to the desktop.
	void ( *Shutdown )( bool destroyWindow );

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// BeginRegistration makes any existing media pointers invalid
	// and returns the current gl configuration, including screen width
	// and height, which can be used by the client to intelligently
	// size display elements. Returns false if the renderer couldn't
	// be initialized.
	bool( *BeginRegistration )( glconfig_t *config, glconfig2_t *glconfig2 );
	qhandle_t ( *RegisterModel )( const char *name );
	//qhandle_t   (*RegisterModelAllLODs) (const char *name);
	qhandle_t ( *RegisterSkin )( const char *name );
	qhandle_t ( *RegisterShader )( const char *name, int flags );
	fontInfo_t* ( *RegisterFont )( const char *fontName, int pointSize );
	void   ( *UnregisterFont )( fontInfo_t *font );
	void   ( *Glyph )( fontInfo_t *font, const char *str, glyphInfo_t *glyph );
	void   ( *GlyphChar )( fontInfo_t *font, int ch, glyphInfo_t *glyph );

	void ( *LoadWorld )( const char *name );

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void ( *SetWorldVisData )( const byte *vis );

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void ( *EndRegistration )();

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void ( *ClearScene )();
	void ( *AddRefEntityToScene )( const refEntity_t *re );

	int ( *LightForPoint )( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );

	void ( *AddPolyToScene )( qhandle_t hShader, int numVerts, const polyVert_t *verts );
	void ( *AddPolysToScene )( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );

	void ( *AddLightToScene )( const vec3_t org, float radius, float intensity, float r, float g, float b,
	                           qhandle_t hShader, int flags );

	void ( *AddAdditiveLightToScene )( const vec3_t org, float intensity, float r, float g, float b );

	void ( *RenderScene )( const refdef_t *fd );

	void ( *SetColor )( const Color::Color& rgba );             // nullptr = 1,1,1,1
	void ( *SetClipRegion )( const float *region );
	void ( *DrawStretchPic )( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );             // 0 = white
	void ( *DrawRotatedPic )( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );             // NERVE - SMF
	void ( *DrawStretchPicGradient )( float x, float y, float w, float h, float s1, float t1, float s2, float t2,
	                                  qhandle_t hShader, const Color::Color& gradientColor, int gradientType );
	void ( *Add2dPolys )( polyVert_t *polys, int numverts, qhandle_t hShader );

	void ( *BeginFrame )();

	// if the pointers are not nullptr, timing info will be returned
	void ( *EndFrame )( int *frontEndMsec, int *backEndMsec );

	int ( *MarkFragments )( int numPoints, const vec3_t *points, const vec3_t projection,
	                        int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

	int ( *LerpTag )( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
	void ( *ModelBounds )( qhandle_t model, vec3_t mins, vec3_t maxs );

	void ( *RemapShader )( const char *oldShader, const char *newShader, const char *offsetTime );

	bool( *GetEntityToken )( char *buffer, int size );

	bool( *inPVS )( const vec3_t p1, const vec3_t p2 );
	bool( *inPVVS )( const vec3_t p1, const vec3_t p2 );

	// XreaL BEGIN
	void ( *TakeVideoFrame )( int h, int w, byte *captureBuffer, byte *encodeBuffer, bool motionJpeg );

	// RB: alternative skeletal animation system
	qhandle_t ( *RegisterAnimation )( const char *name );
	int ( *CheckSkeleton )( refSkeleton_t *skel, qhandle_t model, qhandle_t anim );
	int ( *BuildSkeleton )( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac,
	                        bool clearOrigin );
	int ( *BlendSkeleton )( refSkeleton_t *skel, const refSkeleton_t *blend, float frac );
	int ( *BoneIndex )( qhandle_t hModel, const char *boneName );
	int ( *AnimNumFrames )( qhandle_t hAnim );
	int ( *AnimFrameRate )( qhandle_t hAnim );

	// XreaL END

	// VisTest API
	qhandle_t ( *RegisterVisTest ) ();
	void      ( *AddVisTestToScene ) ( qhandle_t hTest, const vec3_t pos,
					   float depthAdjust, float area );
	float     ( *CheckVisibility ) ( qhandle_t hTest );
	void      ( *UnregisterVisTest ) ( qhandle_t hTest );

	// color grading
	void      ( *SetColorGrading ) ( int slot, qhandle_t hShader );

	void ( *ScissorEnable ) ( bool enable );
	void ( *ScissorSet ) ( int x, int y, int w, int h );

	void ( *SetAltShaderTokens ) ( const char * );

	void ( *GetTextureSize )( int textureID, int *width, int *height );
	void ( *Add2dPolysIndexed )( polyVert_t *polys, int numverts, int *indexes, int numindexes, int trans_x, int trans_y, qhandle_t shader );
	qhandle_t ( *GenerateTexture )( const byte *pic, int width, int height );
	const char *( *ShaderNameFromHandle )( qhandle_t shader );
	void ( *SendBotDebugDrawCommands )( std::vector<char> commands );
	void ( *SetMatrixTransform )( const matrix_t matrix );
	void ( *ResetMatrixTransform )();
};

//
// these are the functions imported by the refresh module
//
struct refimport_t
{
	// milliseconds should only be used for profiling, never
	// for anything game related.  Get time from the refdef
	int ( *Milliseconds )();

	int ( *RealTime )( qtime_t *qtime );

	// stack based memory allocation for per-level things that
	// won't be freed
	void            *( *Hunk_Alloc )( int size, ha_pref pref );
	void            *( *Hunk_AllocateTempMemory )( int size );
	void ( *Hunk_FreeTempMemory )( void *block );

	// a -1 return means the file does not exist
	// nullptr can be passed for buf to just determine existence
	int ( *FS_ReadFile )( const char *name, void **buf );
	void ( *FS_FreeFile )( void *buf );
	char           **( *FS_ListFiles )( const char *name, const char *extension, int *numfilesfound );
	void ( *FS_FreeFileList )( char **filelist );
	void ( *FS_WriteFile )( const char *qpath, const void *buffer, int size );
	int ( *FS_Seek )( fileHandle_t f, long offset, fsOrigin_t origin );
	int ( *FS_FTell )( fileHandle_t f );
	int ( *FS_Read )( void *buffer, int len, fileHandle_t f );
	int ( *FS_FCloseFile )( fileHandle_t f );
	int ( *FS_FOpenFileRead )( const char *qpath, fileHandle_t *file );

	// XreaL BEGIN
	bool( *CL_VideoRecording )();
	void ( *CL_WriteAVIVideoFrame )( const byte *buffer, int size );
	// XreaL END

	// input event handling
	void ( *IN_Init )( void *windowData );
	void ( *IN_Shutdown )();
	void ( *IN_Restart )();
};

#endif // REFAPI_H
