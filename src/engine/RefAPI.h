/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2006-2010 Robert Beckebans

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
// RefAPI.h

#ifndef REF_API_H
#define REF_API_H

#include <string>

#include "renderer/tr_types.h"

#define REF_API_VERSION 10

extern Cvar::Modified<Cvar::Cvar<bool>> r_fullscreen;

// font support
struct glyphInfo_t
{
	int       height; // number of scan lines
	int       top; // top of glyph in buffer
	int       bottom; // bottom of glyph in buffer
	int       pitch; // width for copying
	int       xSkip; // x adjustment
	int       imageWidth; // width of actual image
	int       imageHeight; // height of actual image
	float     s; // x offset in image where glyph starts
	float     t; // y offset in image where glyph starts
	float     s2;
	float     t2;
	qhandle_t glyph; // handle to the shader with the glyph
	char      shaderName[ 32 ];
};

struct fontInfo_t
{
	void         *face, *faceData;
	glyphInfo_t  *glyphBlock[0x110000 / 256]; // glyphBlock_t
	int           pointSize;
	char          name[ MAX_QPATH ];
};

//
// these are the functions exported by the refresh module
//
struct refexport_t {
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
	bool( *BeginRegistration )( WindowConfig* windowCfg );
	qhandle_t( *RegisterModel )( const char* name );
	//qhandle_t   (*RegisterModelAllLODs) (const char *name);
	qhandle_t( *RegisterSkin )( const char* name );
	qhandle_t( *RegisterShader )( const char* name, int flags );
	fontInfo_t* ( *RegisterFont )( const char* fontName, int pointSize );
	void   ( *UnregisterFont )( fontInfo_t* font );
	void   ( *GlyphChar )( fontInfo_t* font, int ch, glyphInfo_t* glyph );

	void ( *LoadWorld )( const char* name );

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void ( *SetWorldVisData )( const byte* vis );

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void ( *EndRegistration )( );

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void ( *ClearScene )( );
	void ( *AddRefEntityToScene )( const refEntity_t* re );
	void ( *SyncRefEntities )( const std::vector<EntityUpdate>& ents );
	std::vector<LerpTagSync>( *SyncLerpTags )( const std::vector<LerpTagUpdate>& lerpTags );

	int ( *LightForPoint )( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );

	void ( *AddPolyToScene )( qhandle_t hShader, int numVerts, const polyVert_t* verts );
	void ( *AddPolysToScene )( qhandle_t hShader, int numVerts, const polyVert_t* verts, int numPolys );

	void ( *AddLightToScene )( const vec3_t org, float radius, float r, float g, float b, int flags );

	void ( *AddAdditiveLightToScene )( const vec3_t org, float intensity, float r, float g, float b );

	void ( *RenderScene )( const refdef_t* fd );

	void ( *SetColor )( const Color::Color& rgba );             // nullptr = 1,1,1,1
	void ( *SetClipRegion )( const float* region );
	void ( *DrawStretchPic )( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );             // 0 = white
	void ( *DrawRotatedPic )( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );             // NERVE - SMF
	void ( *DrawStretchPicGradient )( float x, float y, float w, float h, float s1, float t1, float s2, float t2,
		qhandle_t hShader, const Color::Color& gradientColor, int gradientType );
	void ( *Add2dPolys )( polyVert_t* polys, int numverts, qhandle_t hShader );

	void ( *BeginFrame )( );

	// if the pointers are not nullptr, timing info will be returned
	void ( *EndFrame )( int* frontEndMsec, int* backEndMsec );

	int ( *MarkFragments )( int numPoints, const vec3_t* points, const vec3_t projection,
		int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t* fragmentBuffer );

	void ( *ModelBounds )( qhandle_t model, vec3_t mins, vec3_t maxs );

	void ( *RemapShader )( const char* oldShader, const char* newShader, const char* offsetTime );

	bool( *inPVS )( const vec3_t p1, const vec3_t p2 );
	bool( *inPVVS )( const vec3_t p1, const vec3_t p2 );

	// XreaL BEGIN
	void ( *TakeVideoFrame )( int h, int w, byte* captureBuffer, byte* encodeBuffer, bool motionJpeg );

	// RB: alternative skeletal animation system
	qhandle_t( *RegisterAnimation )( const char* name );
	int ( *CheckSkeleton )( refSkeleton_t* skel, qhandle_t model, qhandle_t anim );
	int ( *BuildSkeleton )( refSkeleton_t* skel, qhandle_t anim, int startFrame, int endFrame, float frac,
		bool clearOrigin );
	int ( *BlendSkeleton )( refSkeleton_t* skel, const refSkeleton_t* blend, float frac );
	int ( *BoneIndex )( qhandle_t hModel, const char* boneName );
	int ( *AnimNumFrames )( qhandle_t hAnim );
	int ( *AnimFrameRate )( qhandle_t hAnim );

	// XreaL END

	// VisTest API
	qhandle_t( *RegisterVisTest ) ( );
	void      ( *AddVisTestToScene ) ( qhandle_t hTest, const vec3_t pos,
		float depthAdjust, float area );
	float     ( *CheckVisibility ) ( qhandle_t hTest );
	void      ( *UnregisterVisTest ) ( qhandle_t hTest );

	// color grading
	void      ( *SetColorGrading ) ( int slot, qhandle_t hShader );

	void ( *ScissorEnable ) ( bool enable );
	void ( *ScissorSet ) ( int x, int y, int w, int h );

	void ( *SetAltShaderTokens ) ( const char* );

	void ( *GetTextureSize )( int textureID, int* width, int* height );
	void ( *Add2dPolysIndexed )( polyVert_t* polys, int numverts, int* indexes, int numindexes, int trans_x, int trans_y, qhandle_t shader );
	qhandle_t( *GenerateTexture )( const byte* pic, int width, int height );
	const char* ( *ShaderNameFromHandle )( qhandle_t shader );
	void ( *SendBotDebugDrawCommands )( std::vector<char> commands );
	void ( *SetMatrixTransform )( const matrix_t matrix );
	void ( *ResetMatrixTransform )( );
};

//
// these are the functions imported by the refresh module
//
struct refimport_t {
	// milliseconds should only be used for profiling, never
	// for anything game related.  Get time from the refdef
	int ( *Milliseconds )( );

	int ( *RealTime )( qtime_t* qtime );

	// stack based memory allocation for per-level things that
	// won't be freed
	void* ( *Hunk_Alloc )( int size, ha_pref pref );
	void* ( *Hunk_AllocateTempMemory )( int size );
	void ( *Hunk_FreeTempMemory )( void* block );

	// a -1 return means the file does not exist
	// nullptr can be passed for buf to just determine existence
	int ( *FS_ReadFile )( const char* name, void** buf );
	void ( *FS_FreeFile )( void* buf );
	char** ( *FS_ListFiles )( const char* name, const char* extension, int* numfilesfound );
	void ( *FS_FreeFileList )( char** filelist );
	void ( *FS_WriteFile )( const char* qpath, const void* buffer, int size );
	int ( *FS_Seek )( fileHandle_t f, long offset, fsOrigin_t origin );
	int ( *FS_FTell )( fileHandle_t f );
	int ( *FS_Read )( void* buffer, int len, fileHandle_t f );
	int ( *FS_FCloseFile )( fileHandle_t f );
	int ( *FS_FOpenFileRead )( const char* qpath, fileHandle_t* file );

	// XreaL BEGIN
	bool( *CL_VideoRecording )( );
	void ( *CL_WriteAVIVideoFrame )( const byte* buffer, int size );
	// XreaL END

	// input event handling
	void ( *IN_Init )( void* windowData );
	void ( *IN_Shutdown )( );
	void ( *IN_Restart )( );
};

// this is the only function actually exported at the linker level
// If the module can't init to a valid rendering state, nullptr will be
// returned.

// RB: changed to GetRefAPI_t
using GetRefAPI_t = refexport_t * ( * )( int apiVersion, refimport_t* rimp );

#endif // REF_API_H
