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
// tr_init.c -- functions that are not called every frame
#include "tr_local.h"

	float       displayAspect = 0.0f;

	static void GfxInfo_f();

	/*
	** InitOpenGL
	**
	** This function is responsible for initializing a valid OpenGL subsystem.  This
	** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
	** setting variables, checking GL constants, and reporting the gfx system config
	** to the user.
	*/
	static bool InitOpenGL()
	{
		char renderer_buffer[ 1024 ];

		//
		// initialize OS specific portions of the renderer
		//
		// GLimp_Init directly or indirectly references the following cvars:
		//      - r_fullscreen
		//      - r_mode
		//      - r_(color|depth|stencil)bits
		//

		if ( glConfig.vidWidth == 0 )
		{
			GLint temp;

			if ( !GLimp_Init() )
			{
				return false;
			}

			if( glConfig2.glCoreProfile ) {
				glGenVertexArrays( 1, &backEnd.currentVAO );
				glBindVertexArray( backEnd.currentVAO );
			}

			glState.tileStep[ 0 ] = TILE_SIZE * ( 1.0f / glConfig.vidWidth );
			glState.tileStep[ 1 ] = TILE_SIZE * ( 1.0f / glConfig.vidHeight );

			GL_CheckErrors();

			strcpy( renderer_buffer, glConfig.renderer_string );
			Q_strlwr( renderer_buffer );

			// OpenGL driver constants
			glGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
			glConfig.maxTextureSize = temp;

			// stubbed or broken drivers may have reported 0...
			if ( glConfig.maxTextureSize <= 0 )
			{
				glConfig.maxTextureSize = 0;
			}

			// handle any OpenGL/GLSL brokenness here...
			// nothing at present

#if defined( GLSL_COMPILE_STARTUP_ONLY )
			GLSL_InitGPUShaders();
#endif
			glConfig.smpActive = false;

			if ( r_smp->integer )
			{
				Log::Notice("Trying SMP acceleration..." );

				if ( GLimp_SpawnRenderThread( RB_RenderThread ) )
				{
					Log::Notice("...succeeded." );
					glConfig.smpActive = true;
				}
				else
				{
					Log::Notice("...failed." );
				}
			}
		}

		GL_CheckErrors();

		// set default state
		GL_SetDefaultState();
		GL_CheckErrors();

		return true;
	}

	/*
	** R_GetModeInfo
	*/
	struct vidmode_t
	{
		const char *description;
		int        width, height;
		float      pixelAspect; // pixel width / height
	};

	static const vidmode_t r_vidModes[] =
	{
		{ " 320x240",           320,  240, 1 },
		{ " 400x300",           400,  300, 1 },
		{ " 512x384",           512,  384, 1 },
		{ " 640x480",           640,  480, 1 },
		{ " 800x600",           800,  600, 1 },
		{ " 960x720",           960,  720, 1 },
		{ "1024x768",          1024,  768, 1 },
		{ "1152x864",          1152,  864, 1 },
		{ "1280x720  (16:9)",  1280,  720, 1 },
		{ "1280x768  (16:10)", 1280,  768, 1 },
		{ "1280x800  (16:10)", 1280,  800, 1 },
		{ "1280x1024",         1280, 1024, 1 },
		{ "1360x768  (16:9)",  1360,  768,  1 },
		{ "1440x900  (16:10)", 1440,  900, 1 },
		{ "1680x1050 (16:10)", 1680, 1050, 1 },
		{ "1600x1200",         1600, 1200, 1 },
		{ "1920x1080 (16:9)",  1920, 1080, 1 },
		{ "1920x1200 (16:10)", 1920, 1200, 1 },
		{ "2048x1536",         2048, 1536, 1 },
		{ "2560x1600 (16:10)", 2560, 1600, 1 },
	};
	static const int s_numVidModes = ARRAY_LEN( r_vidModes );

	bool R_GetModeInfo( int *width, int *height, int mode )
	{
		const vidmode_t *vm;

		if ( mode < -2 )
		{
			return false;
		}

		if ( mode >= s_numVidModes )
		{
			return false;
		}

		if( mode == -2)
		{
			// Must set width and height to display size before calling this function!
		}
		else if ( mode == -1 )
		{
			*width = r_customwidth->integer;
			*height = r_customheight->integer;
		}
		else
		{
			vm = &r_vidModes[ mode ];

			*width = vm->width;
			*height = vm->height;
		}

		return true;
	}

	/*
	** R_ModeList_f
	*/
	static void R_ModeList_f()
	{
		int i;

		for ( i = 0; i < s_numVidModes; i++ )
		{
			Log::Notice("Mode %-2d: %s", i, r_vidModes[ i ].description );
		}
	}

	/*
	==================
	RB_ReadPixels

	Reads an image but takes care of alignment issues for reading RGB images.
	Prepends the specified number of (uninitialized) bytes to the buffer.

	The returned buffer must be freed with ri.Hunk_FreeTempMemory().
	==================
	*/
	static byte *RB_ReadPixels( int x, int y, int width, int height, size_t offset )
	{
		GLint packAlign;
		int   lineLen, paddedLineLen;
		byte  *buffer, *pixels;
		int   i;

		glGetIntegerv( GL_PACK_ALIGNMENT, &packAlign );

		lineLen = width * 3;
		paddedLineLen = PAD( lineLen, packAlign );

		// Allocate a few more bytes so that we can choose an alignment we like
		buffer = ( byte * ) ri.Hunk_AllocateTempMemory( offset + paddedLineLen * height + packAlign - 1 );

		pixels = ( byte * ) PADP( buffer + offset, packAlign );
		glReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels );

		// Drop alignment and line padding bytes
		for ( i = 0; i < height; ++i )
		{
			memmove( buffer + offset + i * lineLen, pixels + i * paddedLineLen, lineLen );
		}

		return buffer;
	}

	/*
	==================
	RB_TakeScreenshot
	==================
	*/
	static void RB_TakeScreenshot( int x, int y, int width, int height, const char *fileName )
	{
		byte *buffer;
		int  dataSize;
		byte *end, *p;

		// with 18 bytes for the TGA file header
		buffer = RB_ReadPixels( x, y, width, height, 18 );
		Com_Memset( buffer, 0, 18 );

		buffer[ 2 ] = 2; // uncompressed type
		buffer[ 12 ] = width & 255;
		buffer[ 13 ] = width >> 8;
		buffer[ 14 ] = height & 255;
		buffer[ 15 ] = height >> 8;
		buffer[ 16 ] = 24; // pixel size

		dataSize = 3 * width * height;

		// swap RGB to BGR
		end = buffer + 18 + dataSize;

		for ( p = buffer + 18; p < end; p += 3 )
		{
			byte temp = p[ 0 ];
			p[ 0 ] = p[ 2 ];
			p[ 2 ] = temp;
		}

		ri.FS_WriteFile( fileName, buffer, 18 + dataSize );

		ri.Hunk_FreeTempMemory( buffer );
	}

	/*
	==================
	RB_TakeScreenshotJPEG
	==================
	*/
	static void RB_TakeScreenshotJPEG( int x, int y, int width, int height, const char *fileName )
	{
		byte *buffer = RB_ReadPixels( x, y, width, height, 0 );

		SaveJPG( fileName, 90, width, height, buffer );
		ri.Hunk_FreeTempMemory( buffer );
	}

	/*
	==================
	RB_TakeScreenshotPNG
	==================
	*/
	static void RB_TakeScreenshotPNG( int x, int y, int width, int height, const char *fileName )
	{
		byte *buffer = RB_ReadPixels( x, y, width, height, 0 );

		SavePNG( fileName, buffer, width, height, 3, false );
		ri.Hunk_FreeTempMemory( buffer );
	}

	/*
	==================
	RB_TakeScreenshotCmd
	==================
	*/
	const RenderCommand *ScreenshotCommand::ExecuteSelf( ) const
	{
		switch ( format )
		{
			case ssFormat_t::SSF_TGA:
				RB_TakeScreenshot( x, y, width, height, fileName );
				break;

			case ssFormat_t::SSF_JPEG:
				RB_TakeScreenshotJPEG( x, y, width, height, fileName );
				break;

			case ssFormat_t::SSF_PNG:
				RB_TakeScreenshotPNG( x, y, width, height, fileName );
				break;
		}

		return this + 1;
	}

	/*
	==================
	R_TakeScreenshot
	==================
	*/
	static bool R_TakeScreenshot( Str::StringRef path, ssFormat_t format )
	{
		ScreenshotCommand *cmd = R_GetRenderCommand<ScreenshotCommand>();

		if ( !cmd )
		{
			return false;
		}

		cmd->x = 0;
		cmd->y = 0;
		cmd->width = glConfig.vidWidth;
		cmd->height = glConfig.vidHeight;
		Q_strncpyz(cmd->fileName, path.c_str(), sizeof(cmd->fileName));
		cmd->format = format;

		return true;
	}

namespace {
class ScreenshotCmd : public Cmd::StaticCmd {
	const ssFormat_t format;
	const std::string fileExtension;
public:
	ScreenshotCmd(std::string cmdName, ssFormat_t format, std::string ext)
		: StaticCmd(cmdName, Cmd::RENDERER, Str::Format("take a screenshot in %s format", ext)),
		format(format), fileExtension(ext) {}

	void Run(const Cmd::Args& args) const override {
		if (!tr.registered) {
			Print("ScreenshotCmd: renderer not initialized");
			return;
		}
		if (args.Argc() > 2) {
			PrintUsage(args, "[name]");
			return;
		}

		std::string fileName;
		if ( args.Argc() == 2 )
		{
			fileName = Str::Format( "screenshots/" PRODUCT_NAME_LOWER "-%s.%s", args.Argv(1), fileExtension );
		}
		else
		{
			qtime_t t;
			ri.RealTime( &t );

			// scan for a free filename
			int lastNumber;
			for ( lastNumber = 0; lastNumber <= 999; lastNumber++ )
			{
				fileName = Str::Format( "screenshots/" PRODUCT_NAME_LOWER "_%04d-%02d-%02d_%02d%02d%02d_%03d.%s",
					                    1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, lastNumber, fileExtension );

				if ( !ri.FS_FileExists( fileName.c_str() ) )
				{
					break; // file doesn't exist
				}
			}

			if ( lastNumber == 1000 )
			{
				Print("ScreenshotCmd: Couldn't create a file" );
				return;
			}
		}

		if (R_TakeScreenshot(fileName, format))
		{
			Print("Wrote %s", fileName);
		}
	}
};
ScreenshotCmd screenshotTGARegistration("screenshot", ssFormat_t::SSF_TGA, "tga");
ScreenshotCmd screenshotJPEGRegistration("screenshotJPEG", ssFormat_t::SSF_JPEG, "jpg");
ScreenshotCmd screenshotPNGRegistration("screenshotPNG", ssFormat_t::SSF_PNG, "png");
} // namespace

//============================================================================

	/*
	==================
	RB_TakeVideoFrameCmd
	==================
	*/
	const RenderCommand *VideoFrameCommand::ExecuteSelf( ) const
	{
		GLint                     packAlign;
		int                       lineLen, captureLineLen;
		byte                      *pixels;
		int                       i;
		int                       outputSize;
		int                       j;
		int                       aviLineLen;

		// RB: it is possible to we still have a videoFrameCommand_t but we already stopped
		// video recording
		if ( ri.CL_VideoRecording() )
		{
			// take care of alignment issues for reading RGB images..

			glGetIntegerv( GL_PACK_ALIGNMENT, &packAlign );

			lineLen = width * 3;
			captureLineLen = PAD( lineLen, packAlign );

			pixels = ( byte * ) PADP( captureBuffer, packAlign );
			glReadPixels( 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels );

			if ( motionJpeg )
			{
				// Drop alignment and line padding bytes
				for ( i = 0; i < height; ++i )
				{
					memmove( captureBuffer + i * lineLen, pixels + i * captureLineLen, lineLen );
				}

				outputSize = SaveJPGToBuffer( encodeBuffer, 3 * width * height, 90, width, height, captureBuffer );
				ri.CL_WriteAVIVideoFrame( encodeBuffer, outputSize );
			}
			else
			{
				aviLineLen = PAD( lineLen, AVI_LINE_PADDING );

				for ( i = 0; i < height; ++i )
				{
					for ( j = 0; j < lineLen; j += 3 )
					{
						encodeBuffer[ i * aviLineLen + j + 0 ] = pixels[ i * captureLineLen + j + 2 ];
						encodeBuffer[ i * aviLineLen + j + 1 ] = pixels[ i * captureLineLen + j + 1 ];
						encodeBuffer[ i * aviLineLen + j + 2 ] = pixels[ i * captureLineLen + j + 0 ];
					}

					while ( j < aviLineLen )
					{
						encodeBuffer[ i * aviLineLen + j++ ] = 0;
					}
				}

				ri.CL_WriteAVIVideoFrame( encodeBuffer, aviLineLen * height );
			}
		}

		return this + 1;
	}

//============================================================================

	/*
	** GL_SetDefaultState
	*/
	void GL_SetDefaultState()
	{
		int i;

		GLimp_LogComment( "--- GL_SetDefaultState ---\n" );

		GL_ClearDepth( 1.0f );

		if ( glConfig.stencilBits >= 4 )
		{
			GL_ClearStencil( 0 );
		}

		GL_FrontFace( GL_CCW );
		GL_CullFace( GL_FRONT );

		glState.faceCulling = CT_TWO_SIDED;
		glDisable( GL_CULL_FACE );

		GL_CheckErrors();

		glVertexAttrib4f( ATTR_INDEX_COLOR, 1, 1, 1, 1 );

		GL_CheckErrors();

		// initialize downstream texture units if we're running
		// in a multitexture environment
		if ( glConfig.driverType == glDriverType_t::GLDRV_OPENGL3 )
		{
			for ( i = 31; i >= 0; i-- )
			{
				GL_SelectTexture( i );
				GL_TextureMode( r_textureMode->string );
			}
		}

		GL_CheckErrors();

		GL_DepthFunc( GL_LEQUAL );

		// make sure our GL state vector is set correctly
		glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;
		glState.vertexAttribsState = 0;
		glState.vertexAttribPointersSet = 0;

		GL_BindProgram( nullptr );

		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
		glState.currentVBO = nullptr;
		glState.currentIBO = nullptr;

		GL_CheckErrors();

		// the vertex array is always enabled, but the color and texture
		// arrays are enabled and disabled around the compiled vertex array call
		glEnableVertexAttribArray( ATTR_INDEX_POSITION );

		/*
		   OpenGL 3.0 spec: E.1. PROFILES AND DEPRECATED FEATURES OF OPENGL 3.0 405
		   Calling VertexAttribPointer when no buffer object or no
		   vertex array object is bound will generate an INVALID OPERATION error,
		   as will calling any array drawing command when no vertex array object is
		   bound.
		 */

		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		glBindRenderbuffer( GL_RENDERBUFFER, 0 );
		glState.currentFBO = nullptr;

		GL_PolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		GL_DepthMask( GL_TRUE );
		glDisable( GL_DEPTH_TEST );
		glEnable( GL_SCISSOR_TEST );
		glDisable( GL_BLEND );

		glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
		glClearDepth( 1.0 );

		glDrawBuffer( GL_BACK );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

		GL_CheckErrors();

		glState.stackIndex = 0;

		for ( i = 0; i < MAX_GLSTACK; i++ )
		{
			MatrixIdentity( glState.modelViewMatrix[ i ] );
			MatrixIdentity( glState.projectionMatrix[ i ] );
			MatrixIdentity( glState.modelViewProjectionMatrix[ i ] );
		}
	}

	/*
	================
	GfxInfo_f
	================
	*/
	void GfxInfo_f()
	{
		static const char fsstrings[][16] =
		{
			"windowed",
			"fullscreen"
		};

		Log::Notice("GL_VENDOR: %s", glConfig.vendor_string );
		Log::Notice("GL_RENDERER: %s", glConfig.renderer_string );
		Log::Notice("GL_VERSION: %s", glConfig.version_string );
		Log::Debug("GL_EXTENSIONS: %s", glConfig.extensions_string );
		Log::Debug("GL_MAX_TEXTURE_SIZE: %d", glConfig.maxTextureSize );

		Log::Notice("GL_SHADING_LANGUAGE_VERSION: %s", glConfig2.shadingLanguageVersionString );

		Log::Notice("GL_MAX_VERTEX_UNIFORM_COMPONENTS %d", glConfig2.maxVertexUniforms );
		Log::Debug("GL_MAX_VERTEX_ATTRIBS %d", glConfig2.maxVertexAttribs );

		if ( glConfig2.occlusionQueryAvailable )
		{
			Log::Debug("%d occlusion query bits", glConfig2.occlusionQueryBits );
		}

		if ( glConfig2.drawBuffersAvailable )
		{
			Log::Debug("GL_MAX_DRAW_BUFFERS: %d", glConfig2.maxDrawBuffers );
		}

		if ( glConfig2.textureAnisotropyAvailable )
		{
			Log::Debug("GL_TEXTURE_MAX_ANISOTROPY_EXT: %f", glConfig2.maxTextureAnisotropy );
		}

		Log::Debug("GL_MAX_RENDERBUFFER_SIZE: %d", glConfig2.maxRenderbufferSize );
		Log::Debug("GL_MAX_COLOR_ATTACHMENTS: %d", glConfig2.maxColorAttachments );

		Log::Debug("\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)", glConfig.colorBits,
		           glConfig.depthBits, glConfig.stencilBits );
		Log::Debug("MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight,
		           fsstrings[ r_fullscreen->integer == 1 ] );

		if ( glConfig.displayFrequency )
		{
			Log::Debug("%d", glConfig.displayFrequency );
		}
		else
		{
			Log::Debug("N/A" );
		}

		Log::Debug("texturemode: %s", r_textureMode->string );
		Log::Debug("picmip: %d", r_picMip->integer );
		Log::Debug("imageMaxDimension: %d", r_imageMaxDimension->integer );
		Log::Debug("ignoreMaterialMinDimension: %d", r_ignoreMaterialMinDimension->integer );
		Log::Debug("ignoreMaterialMaxDimension: %d", r_ignoreMaterialMaxDimension->integer );
		Log::Debug("replaceMaterialMinDimensionIfPresentWithMaxDimension: %d", r_replaceMaterialMinDimensionIfPresentWithMaxDimension->integer );

		if ( glConfig.driverType == glDriverType_t::GLDRV_OPENGL3 )
		{
			int contextFlags, profile;

			Log::Notice("%sUsing OpenGL 3.x context", Color::ToString( Color::Green ) );

			// check if we have a core-profile
			glGetIntegerv( GL_CONTEXT_PROFILE_MASK, &profile );

			if ( profile == GL_CONTEXT_CORE_PROFILE_BIT )
			{
				Log::Debug("%sHaving a core profile", Color::ToString( Color::Green ) );
			}
			else
			{
				Log::Debug("%sHaving a compatibility profile", Color::ToString( Color::Red ) );
			}

			// check if context is forward compatible
			glGetIntegerv( GL_CONTEXT_FLAGS, &contextFlags );

			if ( contextFlags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT )
			{
				Log::Debug("%sContext is forward compatible", Color::ToString( Color::Green ) );
			}
			else
			{
				Log::Debug("%sContext is NOT forward compatible", Color::ToString( Color::Red  ));
			}
		}

		if ( glConfig.hardwareType == glHardwareType_t::GLHW_R300 )
		{
			Log::Debug("HACK: ATI R300 approximations" );
		}

		if ( glConfig.textureCompression != textureCompression_t::TC_NONE )
		{
			Log::Debug("Using S3TC (DXTC) texture compression" );
		}

		if ( glConfig2.vboVertexSkinningAvailable )
		{
			Log::Notice("Using GPU vertex skinning with max %i bones in a single pass", glConfig2.maxVertexSkinningBones );
		}

		if ( glConfig.smpActive )
		{
			Log::Debug("Using dual processor acceleration" );
		}

		if ( r_finish->integer )
		{
			Log::Debug("Forcing glFinish" );
		}
	}

	static void GLSL_restart_f()
	{
		// make sure the render thread is stopped
		R_SyncRenderThread();

		GLSL_ShutdownGPUShaders();
		GLSL_InitGPUShaders();
	}

	/*
	===============
	R_Register
	===============
	*/
	void R_Register()
	{
		shadowMapResolutions[ 0 ] = r_shadowMapSizeUltra->integer;
		shadowMapResolutions[ 1 ] = r_shadowMapSizeVeryHigh->integer;
		shadowMapResolutions[ 2 ] = r_shadowMapSizeHigh->integer;
		shadowMapResolutions[ 3 ] = r_shadowMapSizeMedium->integer;
		shadowMapResolutions[ 4 ] = r_shadowMapSizeLow->integer;

		sunShadowMapResolutions[ 0 ] = r_shadowMapSizeSunUltra->integer;
		sunShadowMapResolutions[ 1 ] = r_shadowMapSizeSunVeryHigh->integer;
		sunShadowMapResolutions[ 2 ] = r_shadowMapSizeSunHigh->integer;
		sunShadowMapResolutions[ 3 ] = r_shadowMapSizeSunMedium->integer;
		sunShadowMapResolutions[ 4 ] = r_shadowMapSizeSunLow->integer;

		// make sure all the commands added here are also removed in R_Shutdown
		ri.Cmd_AddCommand( "imagelist", R_ImageList_f );
		ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
		ri.Cmd_AddCommand( "shaderexp", R_ShaderExp_f );
		ri.Cmd_AddCommand( "skinlist", R_SkinList_f );
		ri.Cmd_AddCommand( "modellist", R_Modellist_f );
		ri.Cmd_AddCommand( "modelist", R_ModeList_f );
		ri.Cmd_AddCommand( "animationlist", R_AnimationList_f );
		ri.Cmd_AddCommand( "fbolist", R_FBOList_f );
		ri.Cmd_AddCommand( "vbolist", R_VBOList_f );
		ri.Cmd_AddCommand( "gfxinfo", GfxInfo_f );
		ri.Cmd_AddCommand( "buildcubemaps", R_BuildCubeMaps );

		ri.Cmd_AddCommand( "glsl_restart", GLSL_restart_f );
	}

	/*
	===============
	R_Init
	===============
	*/
	bool R_Init()
	{
		int i;

		Log::Debug("----- R_Init -----" );

		// clear all our internal state
		tr.~trGlobals_t();
		new(&tr) trGlobals_t{};

		backEnd.~backEndState_t();
		new(&backEnd) backEndState_t{};

		tess.~shaderCommands_t();
		new(&tess) shaderCommands_t{};

		if ( ( intptr_t ) tess.verts & 15 )
		{
			Log::Warn( "tess.verts not 16 byte aligned" );
		}

		// init function tables
		for ( i = 0; i < FUNCTABLE_SIZE; i++ )
		{
			tr.sinTable[ i ] = sin( DEG2RAD( i * 360.0f / ( ( float )( FUNCTABLE_SIZE - 1 ) ) ) );
			tr.squareTable[ i ] = ( i < FUNCTABLE_SIZE / 2 ) ? 1.0f : -1.0f;
			tr.sawToothTable[ i ] = ( float ) i / FUNCTABLE_SIZE;
			tr.inverseSawToothTable[ i ] = 1.0f - tr.sawToothTable[ i ];

			if ( i < FUNCTABLE_SIZE / 2 )
			{
				if ( i < FUNCTABLE_SIZE / 4 )
				{
					tr.triangleTable[ i ] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
				}
				else
				{
					tr.triangleTable[ i ] = 1.0f - tr.triangleTable[ i - FUNCTABLE_SIZE / 4 ];
				}
			}
			else
			{
				tr.triangleTable[ i ] = -tr.triangleTable[ i - FUNCTABLE_SIZE / 2 ];
			}
		}

		R_InitFogTable();

		R_NoiseInit();

		R_Register();

		if ( !InitOpenGL() )
		{
			return false;
		}

#if !defined( GLSL_COMPILE_STARTUP_ONLY )
		GLSL_InitGPUShaders();
#endif

		backEndData[ 0 ] = ( backEndData_t * ) ri.Hunk_Alloc( sizeof( *backEndData[ 0 ] ), ha_pref::h_low );
		backEndData[ 0 ]->polys = ( srfPoly_t * ) ri.Hunk_Alloc( r_maxPolys->integer * sizeof( srfPoly_t ), ha_pref::h_low );
		backEndData[ 0 ]->polyVerts = ( polyVert_t * ) ri.Hunk_Alloc( r_maxPolyVerts->integer * sizeof( polyVert_t ), ha_pref::h_low );
		backEndData[ 0 ]->polyIndexes = ( int * ) ri.Hunk_Alloc( r_maxPolyVerts->integer * sizeof( int ), ha_pref::h_low );

		if ( r_smp->integer )
		{
			backEndData[ 1 ] = ( backEndData_t * ) ri.Hunk_Alloc( sizeof( *backEndData[ 1 ] ), ha_pref::h_low );
			backEndData[ 1 ]->polys = ( srfPoly_t * ) ri.Hunk_Alloc( r_maxPolys->integer * sizeof( srfPoly_t ), ha_pref::h_low );
			backEndData[ 1 ]->polyVerts = ( polyVert_t * ) ri.Hunk_Alloc( r_maxPolyVerts->integer * sizeof( polyVert_t ), ha_pref::h_low );
			backEndData[ 1 ]->polyIndexes = ( int * ) ri.Hunk_Alloc( r_maxPolyVerts->integer * sizeof( int ), ha_pref::h_low );
		}
		else
		{
			backEndData[ 1 ] = nullptr;
		}

		R_ToggleSmpFrame();

		R_InitImages();

		R_InitFBOs();

		R_InitVBOs();

		R_InitShaders();

		R_InitSkins();

		R_ModelInit();

		R_InitAnimations();

		R_InitFreeType();

		if ( glConfig2.textureAnisotropyAvailable )
		{
			AssertCvarRange( r_ext_texture_filter_anisotropic, 0, glConfig2.maxTextureAnisotropy, false );
		}

		R_InitVisTests();

		GL_CheckErrors();

		// print info
		GfxInfo_f();
		GL_CheckErrors();

		Log::Debug("----- finished R_Init -----" );

		return true;
	}

	/*
	===============
	RE_Shutdown
	===============
	*/
	void RE_Shutdown( bool destroyWindow )
	{
		Log::Debug("RE_Shutdown( destroyWindow = %i )", destroyWindow );

		ri.Cmd_RemoveCommand( "modellist" );
		ri.Cmd_RemoveCommand( "imagelist" );
		ri.Cmd_RemoveCommand( "shaderlist" );
		ri.Cmd_RemoveCommand( "shaderexp" );
		ri.Cmd_RemoveCommand( "skinlist" );
		ri.Cmd_RemoveCommand( "gfxinfo" );
		ri.Cmd_RemoveCommand( "modelist" );
		ri.Cmd_RemoveCommand( "shaderstate" );
		ri.Cmd_RemoveCommand( "animationlist" );
		ri.Cmd_RemoveCommand( "fbolist" );
		ri.Cmd_RemoveCommand( "vbolist" );
		ri.Cmd_RemoveCommand( "generatemtr" );
		ri.Cmd_RemoveCommand( "buildcubemaps" );

		ri.Cmd_RemoveCommand( "glsl_restart" );

		if ( tr.registered )
		{
			R_SyncRenderThread();

			R_ShutdownBackend();
			R_ShutdownImages();
			R_ShutdownVBOs();
			R_ShutdownFBOs();
			R_ShutdownVisTests();

#if !defined( GLSL_COMPILE_STARTUP_ONLY )
			GLSL_ShutdownGPUShaders();
#endif
		}

		R_DoneFreeType();

		// shut down platform specific OpenGL stuff
		if ( destroyWindow )
		{
#if defined( GLSL_COMPILE_STARTUP_ONLY )
			GLSL_ShutdownGPUShaders();
#endif
			if( glConfig2.glCoreProfile ) {
				glBindVertexArray( 0 );
				glDeleteVertexArrays( 1, &backEnd.currentVAO );
			}

			GLimp_Shutdown();
			ri.Tag_Free();
		}

		tr.registered = false;
	}

	/*
	=============
	RE_EndRegistration

	Touch all images to make sure they are resident
	=============
	*/
	void RE_EndRegistration()
	{
		R_SyncRenderThread();
		if ( r_lazyShaders->integer == 1 )
		{
			GLSL_FinishGPUShaders();
		}
	}

	/*
	=====================
	GetRefAPI
	=====================
	*/
	refexport_t *GetRefAPI( int apiVersion, refimport_t *rimp )
	{
		static refexport_t re;

		ri = *rimp;

		Log::Debug("GetRefAPI()" );

		Com_Memset( &re, 0, sizeof( re ) );

		if ( apiVersion != REF_API_VERSION )
		{
			Log::Notice("Mismatched REF_API_VERSION: expected %i, got %i", REF_API_VERSION, apiVersion );
			return nullptr;
		}

		// the RE_ functions are Renderer Entry points

		// Q3A BEGIN
		re.Shutdown = RE_Shutdown;

		re.BeginRegistration = RE_BeginRegistration;
		re.RegisterModel = RE_RegisterModel;

		re.RegisterSkin = RE_RegisterSkin;
		re.RegisterShader = RE_RegisterShader;

		re.LoadWorld = RE_LoadWorldMap;
		re.SetWorldVisData = RE_SetWorldVisData;
		re.EndRegistration = RE_EndRegistration;

		re.BeginFrame = RE_BeginFrame;
		re.EndFrame = RE_EndFrame;

		re.MarkFragments = R_MarkFragments;

		re.LerpTag = RE_LerpTagET;

		re.ModelBounds = R_ModelBounds;

		re.ClearScene = RE_ClearScene;
		re.AddRefEntityToScene = RE_AddRefEntityToScene;

		re.AddPolyToScene = RE_AddPolyToSceneET;
		re.AddPolysToScene = RE_AddPolysToScene;
		re.LightForPoint = R_LightForPoint;

		re.AddLightToScene = RE_AddDynamicLightToSceneET;
		re.AddAdditiveLightToScene = RE_AddDynamicLightToSceneQ3A;

		re.RenderScene = RE_RenderScene;

		re.SetColor = RE_SetColor;
		re.SetClipRegion = RE_SetClipRegion;
		re.DrawStretchPic = RE_StretchPic;

		re.DrawRotatedPic = RE_RotatedPic;
		re.Add2dPolys = RE_2DPolyies;
		re.ScissorEnable = RE_ScissorEnable;
		re.ScissorSet = RE_ScissorSet;
		re.DrawStretchPicGradient = RE_StretchPicGradient;

		re.Glyph = RE_Glyph;
		re.GlyphChar = RE_GlyphChar;
		re.RegisterFont = RE_RegisterFont;
		re.UnregisterFont = RE_UnregisterFont;

		re.RemapShader = R_RemapShader;
		re.GetEntityToken = R_GetEntityToken;
		re.inPVS = R_inPVS;
		re.inPVVS = R_inPVVS;
		// Q3A END

		// ET BEGIN
		re.ProjectDecal = RE_ProjectDecal;
		re.ClearDecals = RE_ClearDecals;

		re.LoadDynamicShader = RE_LoadDynamicShader;
		re.Finish = RE_Finish;
		// ET END

		// XreaL BEGIN
		re.TakeVideoFrame = RE_TakeVideoFrame;

		re.AddRefLightToScene = RE_AddRefLightToScene;

		re.RegisterAnimation = RE_RegisterAnimation;
		re.CheckSkeleton = RE_CheckSkeleton;
		re.BuildSkeleton = RE_BuildSkeleton;
		re.BlendSkeleton = RE_BlendSkeleton;
		re.BoneIndex = RE_BoneIndex;
		re.AnimNumFrames = RE_AnimNumFrames;
		re.AnimFrameRate = RE_AnimFrameRate;

		// XreaL END

		re.RegisterVisTest = RE_RegisterVisTest;
		re.AddVisTestToScene = RE_AddVisTestToScene;
		re.CheckVisibility = RE_CheckVisibility;
		re.UnregisterVisTest = RE_UnregisterVisTest;

		re.SetColorGrading = RE_SetColorGrading;

		re.SetAltShaderTokens = R_SetAltShaderTokens;

		re.GetTextureSize = RE_GetTextureSize;
		re.Add2dPolysIndexed = RE_2DPolyiesIndexed;
		re.GenerateTexture = RE_GenerateTexture;
		re.ShaderNameFromHandle = RE_GetShaderNameFromHandle;
		re.SendBotDebugDrawCommands = RE_SendBotDebugDrawCommands;

		return &re;
	}
