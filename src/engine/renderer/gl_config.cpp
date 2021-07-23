#include "common/Common.h"
#include "tr_local.h"

Log::Logger glConfigLogger("glconfig", "", Log::Level::NOTICE);
#define logger glConfigLogger

glconfig_t  glConfig;
glconfig2_t glConfig2;

glstate_t   glState;

cvar_t      *r_glMajorVersion;
cvar_t      *r_glMinorVersion;
cvar_t      *r_glProfile;
cvar_t      *r_glDebugProfile;
cvar_t      *r_glDebugMode;
cvar_t      *r_glAllowSoftware;

cvar_t      *r_verbose;
cvar_t      *r_ignore;

cvar_t      *r_znear;
cvar_t      *r_zfar;

cvar_t      *r_smp;
cvar_t      *r_showSmp;
cvar_t      *r_skipBackEnd;
cvar_t      *r_skipLightBuffer;

cvar_t      *r_measureOverdraw;

cvar_t      *r_fastsky;
cvar_t      *r_drawSun;

cvar_t      *r_lodBias;
cvar_t      *r_lodScale;
cvar_t      *r_lodTest;

cvar_t      *r_norefresh;
cvar_t      *r_drawentities;
cvar_t      *r_drawworld;
cvar_t      *r_drawpolies;
cvar_t      *r_speeds;
cvar_t      *r_novis;
cvar_t      *r_nocull;
cvar_t      *r_facePlaneCull;
cvar_t      *r_showcluster;
cvar_t      *r_nocurves;
cvar_t      *r_lightScissors;
cvar_t      *r_noLightVisCull;
cvar_t      *r_noInteractionSort;
cvar_t      *r_dynamicLight;
cvar_t      *r_staticLight;
cvar_t      *r_dynamicLightCastShadows;
cvar_t      *r_precomputedLighting;
cvar_t      *r_vertexLighting;
cvar_t      *r_exportTextures;
cvar_t      *r_heatHaze;
cvar_t      *r_noMarksOnTrisurfs;
cvar_t      *r_lazyShaders;

cvar_t      *r_ext_occlusion_query;
cvar_t      *r_ext_draw_buffers;
cvar_t      *r_ext_vertex_array_object;
cvar_t      *r_ext_half_float_pixel;
cvar_t      *r_ext_texture_float;
cvar_t      *r_ext_texture_integer;
cvar_t      *r_ext_texture_rg;
cvar_t      *r_ext_texture_filter_anisotropic;
cvar_t      *r_ext_gpu_shader4;
cvar_t      *r_arb_buffer_storage;
cvar_t      *r_arb_map_buffer_range;
cvar_t      *r_arb_sync;
cvar_t      *r_arb_uniform_buffer_object;
cvar_t      *r_arb_texture_gather;
cvar_t      *r_arb_gpu_shader5;

cvar_t      *r_checkGLErrors;
cvar_t      *r_logFile;

cvar_t      *r_stencilbits;
cvar_t      *r_depthbits;
cvar_t      *r_colorbits;
cvar_t      *r_alphabits;
cvar_t      *r_ext_multisample;

cvar_t      *r_drawBuffer;
cvar_t      *r_shadows;
cvar_t      *r_softShadows;
cvar_t      *r_softShadowsPP;
cvar_t      *r_shadowBlur;

cvar_t      *r_shadowMapQuality;
cvar_t      *r_shadowMapSizeUltra;
cvar_t      *r_shadowMapSizeVeryHigh;
cvar_t      *r_shadowMapSizeHigh;
cvar_t      *r_shadowMapSizeMedium;
cvar_t      *r_shadowMapSizeLow;

cvar_t      *r_shadowMapSizeSunUltra;
cvar_t      *r_shadowMapSizeSunVeryHigh;
cvar_t      *r_shadowMapSizeSunHigh;
cvar_t      *r_shadowMapSizeSunMedium;
cvar_t      *r_shadowMapSizeSunLow;

cvar_t      *r_shadowOffsetFactor;
cvar_t      *r_shadowOffsetUnits;
cvar_t      *r_shadowLodBias;
cvar_t      *r_shadowLodScale;
cvar_t      *r_noShadowPyramids;
cvar_t      *r_cullShadowPyramidFaces;
cvar_t      *r_cullShadowPyramidCurves;
cvar_t      *r_cullShadowPyramidTriangles;
cvar_t      *r_debugShadowMaps;
cvar_t      *r_noShadowFrustums;
cvar_t      *r_noLightFrustums;
cvar_t      *r_shadowMapLinearFilter;
cvar_t      *r_lightBleedReduction;
cvar_t      *r_overDarkeningFactor;
cvar_t      *r_shadowMapDepthScale;
cvar_t      *r_parallelShadowSplits;
cvar_t      *r_parallelShadowSplitWeight;

cvar_t      *r_mode;
cvar_t      *r_nobind;
cvar_t      *r_singleShader;
cvar_t      *r_colorMipLevels;
cvar_t      *r_picMip;
cvar_t      *r_imageMaxDimension;
cvar_t      *r_ignoreMaterialMinDimension;
cvar_t      *r_ignoreMaterialMaxDimension;
cvar_t      *r_replaceMaterialMinDimensionIfPresentWithMaxDimension;
cvar_t      *r_finish;
cvar_t      *r_clear;
cvar_t      *r_swapInterval;
cvar_t      *r_textureMode;
cvar_t      *r_offsetFactor;
cvar_t      *r_offsetUnits;

cvar_t      *r_physicalMapping;
cvar_t      *r_specularExponentMin;
cvar_t      *r_specularExponentMax;
cvar_t      *r_specularScale;
cvar_t      *r_specularMapping;
cvar_t      *r_deluxeMapping;
cvar_t      *r_normalScale;
cvar_t      *r_normalMapping;
cvar_t      *r_liquidMapping;
cvar_t      *r_highQualityNormalMapping;
cvar_t      *r_reliefDepthScale;
cvar_t      *r_reliefMapping;
cvar_t      *r_glowMapping;
cvar_t      *r_reflectionMapping;

cvar_t      *r_wrapAroundLighting;
cvar_t      *r_halfLambertLighting;
cvar_t      *r_rimLighting;
cvar_t      *r_rimExponent;
cvar_t      *r_gamma;
cvar_t      *r_lockpvs;
cvar_t      *r_noportals;
cvar_t      *r_portalOnly;
cvar_t      *r_portalSky;
cvar_t      *r_max_portal_levels;

cvar_t      *r_subdivisions;
cvar_t      *r_stitchCurves;

cvar_t      *r_noBorder;
cvar_t      *r_fullscreen;

cvar_t      *r_customwidth;
cvar_t      *r_customheight;

cvar_t      *r_debugSurface;
cvar_t      *r_simpleMipMaps;

cvar_t      *r_showImages;

cvar_t      *r_forceFog;
cvar_t      *r_wolfFog;
cvar_t      *r_noFog;

cvar_t      *r_forceAmbient;
cvar_t      *r_ambientScale;
cvar_t      *r_lightScale;
cvar_t      *r_debugSort;
cvar_t      *r_printShaders;

cvar_t      *r_maxPolys;
cvar_t      *r_maxPolyVerts;

cvar_t      *r_showTris;
cvar_t      *r_showSky;
cvar_t      *r_showShadowVolumes;
cvar_t      *r_showShadowLod;
cvar_t      *r_showShadowMaps;
cvar_t      *r_showSkeleton;
cvar_t      *r_showEntityTransforms;
cvar_t      *r_showLightTransforms;
cvar_t      *r_showLightInteractions;
cvar_t      *r_showLightScissors;
cvar_t      *r_showLightBatches;
cvar_t      *r_showLightGrid;
cvar_t      *r_showLightTiles;
cvar_t      *r_showBatches;
cvar_t      *r_showLightMaps;
cvar_t      *r_showDeluxeMaps;
cvar_t      *r_showNormalMaps;
cvar_t      *r_showMaterialMaps;
cvar_t      *r_showAreaPortals;
cvar_t      *r_showCubeProbes;
cvar_t      *r_showBspNodes;
cvar_t      *r_showParallelShadowSplits;
cvar_t      *r_showDecalProjectors;

cvar_t      *r_vboFaces;
cvar_t      *r_vboCurves;
cvar_t      *r_vboTriangles;
cvar_t      *r_vboShadows;
cvar_t      *r_vboLighting;
cvar_t      *r_vboModels;
cvar_t      *r_vboVertexSkinning;
cvar_t      *r_vboDeformVertexes;

cvar_t      *r_mergeLeafSurfaces;

cvar_t      *r_bloom;
cvar_t      *r_bloomBlur;
cvar_t      *r_bloomPasses;
cvar_t      *r_FXAA;
cvar_t      *r_ssao;

cvar_t      *r_evsmPostProcess;

cvar_t      *r_fontScale;

void AssertCvarRange( cvar_t *cv, float minVal, float maxVal, bool shouldBeIntegral )
{
	if ( shouldBeIntegral )
	{
		if ( cv->value != static_cast<float>(cv->integer) )
		{
			Log::Warn("cvar '%s' must be integral (%f)", cv->name, cv->value );
			Cvar_Set( cv->name, va( "%d", cv->integer ) );
		}
	}

	if ( cv->value < minVal )
	{
		Log::Warn("cvar '%s' out of range (%g < %g)", cv->name, cv->value, minVal );
		Cvar_Set( cv->name, va( "%f", minVal ) );
	}
	else if ( cv->value > maxVal )
	{
		Log::Warn("cvar '%s' out of range (%g > %g)", cv->name, cv->value, maxVal );
		Cvar_Set( cv->name, va( "%f", maxVal ) );
	}
}

/*
===============
GLimp_InitExtensions
===============
*/

/* ExtFlag_CORE means the extension is known to be an OpenGL 3 core extension.
The code considers the extension is available even if the extension is not listed
if the driver pretends to support OpenGL Core 3 and we know this extension is part
of OpenGL Core 3. */

enum {
	ExtFlag_NONE,
	ExtFlag_REQUIRED = BIT( 1 ),
	ExtFlag_CORE = BIT( 2 ),
};

static bool LoadExt( int flags, bool hasExt, const char* name, bool test = true )
{
	if ( hasExt || ( flags & ExtFlag_CORE && glConfig2.glCoreProfile) )
	{
		if ( test )
		{
			logger.WithoutSuppression().Notice( "...using GL_%s", name );
			return true;
		}
		else
		{
			// Required extension can't be made optional
			ASSERT( !( flags & ExtFlag_REQUIRED ) );

			logger.WithoutSuppression().Notice( "...ignoring GL_%s", name );
		}
	}
	else
	{
		if ( flags & ExtFlag_REQUIRED )
		{
			Sys::Error( "Required extension GL_%s is missing", name );
		}
		else
		{
			logger.WithoutSuppression().Notice( "...GL_%s not found", name );
		}
	}
	return false;
}

#define LOAD_EXTENSION(flags, ext) LoadExt(flags, GLEW_##ext, #ext)

#define LOAD_EXTENSION_WITH_TEST(flags, ext, test) LoadExt(flags, GLEW_##ext, #ext, test)

static GLenum debugTypes[] =
{
	0,
	GL_DEBUG_TYPE_ERROR_ARB,
	GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB,
	GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB,
	GL_DEBUG_TYPE_PORTABILITY_ARB,
	GL_DEBUG_TYPE_PERFORMANCE_ARB,
	GL_DEBUG_TYPE_OTHER_ARB
};

#ifdef _WIN32
#define DEBUG_CALLBACK_CALL APIENTRY
#else
#define DEBUG_CALLBACK_CALL
#endif
static void DEBUG_CALLBACK_CALL GLimp_DebugCallback( GLenum, GLenum type, GLuint,
                                       GLenum severity, GLsizei, const GLchar *message, const void* )
{
	const char *debugTypeName;
	const char *debugSeverity;

	if ( r_glDebugMode->integer <= Util::ordinal(glDebugModes_t::GLDEBUG_NONE))
	{
		return;
	}

	if ( r_glDebugMode->integer < Util::ordinal(glDebugModes_t::GLDEBUG_ALL))
	{
		if ( debugTypes[ r_glDebugMode->integer ] != type )
		{
			return;
		}
	}

	switch ( type )
	{
		case GL_DEBUG_TYPE_ERROR_ARB:
			debugTypeName = "DEBUG_TYPE_ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
			debugTypeName = "DEBUG_TYPE_DEPRECATED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
			debugTypeName = "DEBUG_TYPE_UNDEFINED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB:
			debugTypeName = "DEBUG_TYPE_PORTABILITY";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB:
			debugTypeName = "DEBUG_TYPE_PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_OTHER_ARB:
			debugTypeName = "DEBUG_TYPE_OTHER";
			break;
		default:
			debugTypeName = "DEBUG_TYPE_UNKNOWN";
			break;
	}

	switch ( severity )
	{
		case GL_DEBUG_SEVERITY_HIGH_ARB:
			debugSeverity = "high";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM_ARB:
			debugSeverity = "med";
			break;
		case GL_DEBUG_SEVERITY_LOW_ARB:
			debugSeverity = "low";
			break;
		default:
			debugSeverity = "none";
			break;
	}

	logger.Warn("%s: severity: %s msg: %s", debugTypeName, debugSeverity, message );
}

#ifdef USE_OSMESA
#define LOAD_EXTENSION_WITH_TEST(...) false
#define LOAD_EXTENSION(...) false
#endif

void GLimp_InitExtensions()
{
	logger.Notice("Initializing OpenGL extensions" );

	if ( LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_debug_output, r_glDebugProfile->value ) )
	{
		glDebugMessageCallbackARB( (GLDEBUGPROCARB)GLimp_DebugCallback, nullptr );
		glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
	}

	// Shader limits
	glGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig2.maxVertexUniforms );
	glGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig2.maxVertexAttribs );

	int reservedComponents = 36 * 10; // approximation how many uniforms we have besides the bone matrices
	glConfig2.maxVertexSkinningBones = Math::Clamp( ( glConfig2.maxVertexUniforms - reservedComponents ) / 16, 0, MAX_BONES );
	glConfig2.vboVertexSkinningAvailable = r_vboVertexSkinning->integer && ( ( glConfig2.maxVertexSkinningBones >= 12 ) ? true : false );

	// GLSL

	Q_strncpyz( glConfig2.shadingLanguageVersionString, ( char * ) glGetString( GL_SHADING_LANGUAGE_VERSION_ARB ),
				sizeof( glConfig2.shadingLanguageVersionString ) );
	int majorVersion, minorVersion;
	if ( sscanf( glConfig2.shadingLanguageVersionString, "%i.%i", &majorVersion, &minorVersion ) != 2 )
	{
		logger.Warn("unrecognized shading language version string format" );
	}
	glConfig2.shadingLanguageVersion = majorVersion * 100 + minorVersion;

	logger.Notice("...found shading language version %i", glConfig2.shadingLanguageVersion );

	// Texture formats and compression
	glGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig2.maxCubeMapTextureSize );

	// made required in OpenGL 3.0
	glConfig2.textureHalfFloatAvailable =  LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_half_float_pixel, r_ext_half_float_pixel->value );

	// made required in OpenGL 3.0
	glConfig2.textureFloatAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_texture_float, r_ext_texture_float->value );

	// made required in OpenGL 3.0
	glConfig2.gpuShader4Available = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, EXT_gpu_shader4, r_ext_gpu_shader4->value );

	// made required in OpenGL 3.0
	// GL_EXT_texture_integer can be used in shaders only if GL_EXT_gpu_shader4 is also available
	glConfig2.textureIntegerAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, EXT_texture_integer, r_ext_texture_integer->value )
	  && glConfig2.gpuShader4Available;

	// made required in OpenGL 3.0
	glConfig2.textureRGAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_texture_rg, r_ext_texture_rg->value );

	{
		/* GT218-based GPU with Nvidia 340.108 driver advertising
		ARB_texture_gather extension is know to fail to compile
		the depthtile1 GLSL shader.

		See https://github.com/DaemonEngine/Daemon/issues/368

		Unfortunately this workaround may also disable the feature for
		all GPUs using this driver even if we don't know if some of them
		are not affected by the bug while advertising this extension, but
		there is no known easy way to detect GT218-based cards. Not all cards
		using 340 driver supports this extension anyway, like the G92 one.

		We can assume cards not using the 340 driver are not GT218 ones and
		are not affected.

		Usually, those GT218 cards are not powerful enough for dynamic
		lighting so it is likely this feature would be disabled to
		get acceptable framerate on this hardware anyway, making the
		need for such extension and the related shader code useless. */
		bool foundNvidia340 = ( Q_stristr( glConfig.vendor_string, "NVIDIA Corporation" ) && Q_stristr( glConfig.version_string, "NVIDIA 340." ) );

		if ( foundNvidia340 )
		{
			// No need for WithoutSuppression for something which can only be printed once per renderer restart.
			logger.Notice("...found buggy Nvidia 340 driver");
		}

		// made required in OpenGL 4.0
		glConfig2.textureGatherAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_texture_gather, r_arb_texture_gather->value && !foundNvidia340 );
	}

	// made required in OpenGL 1.3
	glConfig.textureCompression = textureCompression_t::TC_NONE;
	if( LOAD_EXTENSION( ExtFlag_NONE, EXT_texture_compression_s3tc ) )
	{
		glConfig.textureCompression = textureCompression_t::TC_S3TC;
	}

	// made required in OpenGL 3.0
	glConfig2.textureCompressionRGTCAvailable = LOAD_EXTENSION( ExtFlag_CORE, ARB_texture_compression_rgtc );

	// Texture - others
	glConfig2.textureAnisotropyAvailable = false;
	if ( LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, EXT_texture_filter_anisotropic, r_ext_texture_filter_anisotropic->value ) )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig2.maxTextureAnisotropy );
		glConfig2.textureAnisotropyAvailable = true;
	}

	// VAO and VBO
	// made required in OpenGL 3.0

	LOAD_EXTENSION( ExtFlag_REQUIRED | ExtFlag_CORE, ARB_half_float_vertex );

	// made required in OpenGL 3.0
	LOAD_EXTENSION( ExtFlag_REQUIRED | ExtFlag_CORE, ARB_framebuffer_object );

	// FBO
	glGetIntegerv( GL_MAX_RENDERBUFFER_SIZE, &glConfig2.maxRenderbufferSize );
	glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &glConfig2.maxColorAttachments );

	// made required in OpenGL 1.5
	glConfig2.occlusionQueryAvailable = false;
	glConfig2.occlusionQueryBits = 0;
	if ( r_ext_occlusion_query->integer != 0 )
	{
		glConfig2.occlusionQueryAvailable = true;
		glGetQueryiv( GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &glConfig2.occlusionQueryBits );
	}

	// made required in OpenGL 2.0
	glConfig2.drawBuffersAvailable = false;
	if ( r_ext_draw_buffers->integer != 0 )
	{
		glGetIntegerv( GL_MAX_DRAW_BUFFERS, &glConfig2.maxDrawBuffers );
		glConfig2.drawBuffersAvailable = true;
	}

	{
		int formats = 0;

		glGetIntegerv( GL_NUM_PROGRAM_BINARY_FORMATS, &formats );

		if ( formats == 0 )
		{
			// No need for WithoutSuppression for something which can only be printed once per renderer restart.
			logger.Notice("...no program binary formats");
		}

		glConfig2.getProgramBinaryAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_get_program_binary, formats > 0 );
	}

	glConfig2.bufferStorageAvailable = false;
	glConfig2.bufferStorageAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_NONE, ARB_buffer_storage, r_arb_buffer_storage->integer > 0 );

	// made required since OpenGL 3.1
	glConfig2.uniformBufferObjectAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_uniform_buffer_object, r_arb_uniform_buffer_object->value );

	// made required in OpenGL 3.0
	glConfig2.mapBufferRangeAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_map_buffer_range, r_arb_map_buffer_range->value );

	// made required in OpenGL 3.2
	glConfig2.syncAvailable = LOAD_EXTENSION_WITH_TEST( ExtFlag_CORE, ARB_sync, r_arb_sync->value );

	GL_CheckErrors();
}

void GLimp_LogComment( const char *comment )
{
	static char buf[ 4096 ];

	if ( r_logFile->integer && GLEW_ARB_debug_output )
	{
		// copy string and ensure it has a trailing '\0'
		Q_strncpyz( buf, comment, sizeof( buf ) );

		glDebugMessageInsertARB( GL_DEBUG_SOURCE_APPLICATION_ARB,
					 GL_DEBUG_TYPE_OTHER_ARB,
					 0,
					 GL_DEBUG_SEVERITY_MEDIUM_ARB,
					 strlen( buf ), buf );
	}
}

/*
==================
GL_CheckErrors

Must not be called while the backend rendering thread is running
==================
*/
void GL_CheckErrors_( const char *fileName, int line )
{
	int  err;
	char s[ 128 ];

	if ( r_checkGLErrors->integer )
	{
		return;
	}

	while ( ( err = glGetError() ) != GL_NO_ERROR )
	{
		switch ( err )
		{
			case GL_INVALID_ENUM:
				strcpy( s, "GL_INVALID_ENUM" );
				break;

			case GL_INVALID_VALUE:
				strcpy( s, "GL_INVALID_VALUE" );
				break;

			case GL_INVALID_OPERATION:
				strcpy( s, "GL_INVALID_OPERATION" );
				break;

			case GL_STACK_OVERFLOW:
				strcpy( s, "GL_STACK_OVERFLOW" );
				break;

			case GL_STACK_UNDERFLOW:
				strcpy( s, "GL_STACK_UNDERFLOW" );
				break;

			case GL_OUT_OF_MEMORY:
				strcpy( s, "GL_OUT_OF_MEMORY" );
				break;

			case GL_TABLE_TOO_LARGE:
				strcpy( s, "GL_TABLE_TOO_LARGE" );
				break;

			case GL_INVALID_FRAMEBUFFER_OPERATION:
				strcpy( s, "GL_INVALID_FRAMEBUFFER_OPERATION" );
				break;

			default:
				Com_sprintf( s, sizeof( s ), "0x%X", err );
				break;
		}
		// Pre-format the string so that each callsite counts separately for log suppression
		std::string error = Str::Format("OpenGL error %s detected at %s:%d", s, fileName, line);
		Log::Warn(error);
	}
}

void R_RegisterCvars()
{
	// OpenGL context selection
	r_glMajorVersion = Cvar_Get( "r_glMajorVersion", "", CVAR_LATCH );
	r_glMinorVersion = Cvar_Get( "r_glMinorVersion", "", CVAR_LATCH );
	r_glProfile = Cvar_Get( "r_glProfile", "", CVAR_LATCH );
	r_glDebugProfile = Cvar_Get( "r_glDebugProfile", "", CVAR_LATCH );
	r_glDebugMode = Cvar_Get( "r_glDebugMode", "0", CVAR_CHEAT );
	r_glAllowSoftware = Cvar_Get( "r_glAllowSoftware", "0", CVAR_LATCH );

	// latched and archived variables
	r_ext_occlusion_query = Cvar_Get( "r_ext_occlusion_query", "1", CVAR_CHEAT | CVAR_LATCH );
	r_ext_draw_buffers = Cvar_Get( "r_ext_draw_buffers", "1", CVAR_CHEAT | CVAR_LATCH );
	r_ext_vertex_array_object = Cvar_Get( "r_ext_vertex_array_object", "1", CVAR_CHEAT | CVAR_LATCH );
	r_ext_half_float_pixel = Cvar_Get( "r_ext_half_float_pixel", "1", CVAR_CHEAT | CVAR_LATCH );
	r_ext_texture_float = Cvar_Get( "r_ext_texture_float", "1", CVAR_CHEAT | CVAR_LATCH );
	r_ext_texture_integer = Cvar_Get( "r_ext_texture_integer", "1", CVAR_CHEAT | CVAR_LATCH );
	r_ext_texture_rg = Cvar_Get( "r_ext_texture_rg", "1", CVAR_CHEAT | CVAR_LATCH );
	r_ext_texture_filter_anisotropic = Cvar_Get( "r_ext_texture_filter_anisotropic", "4",  CVAR_LATCH | CVAR_ARCHIVE );
	r_ext_gpu_shader4 = Cvar_Get( "r_ext_gpu_shader4", "1", CVAR_CHEAT | CVAR_LATCH );
	r_arb_buffer_storage = Cvar_Get( "r_arb_buffer_storage", "1", CVAR_CHEAT | CVAR_LATCH );
	r_arb_map_buffer_range = Cvar_Get( "r_arb_map_buffer_range", "1", CVAR_CHEAT | CVAR_LATCH );
	r_arb_sync = Cvar_Get( "r_arb_sync", "1", CVAR_CHEAT | CVAR_LATCH );
	r_arb_uniform_buffer_object = Cvar_Get( "r_arb_uniform_buffer_object", "1", CVAR_CHEAT | CVAR_LATCH );
	r_arb_texture_gather = Cvar_Get( "r_arb_texture_gather", "1", CVAR_CHEAT | CVAR_LATCH );
	r_arb_gpu_shader5 = Cvar_Get( "r_arb_gpu_shader5", "1", CVAR_CHEAT | CVAR_LATCH );

	r_picMip = Cvar_Get( "r_picMip", "0",  CVAR_LATCH | CVAR_ARCHIVE );
	r_imageMaxDimension = Cvar_Get( "r_imageMaxDimension", "0",  CVAR_LATCH | CVAR_ARCHIVE );
	r_ignoreMaterialMinDimension = Cvar_Get( "r_ignoreMaterialMinDimension", "0",  CVAR_LATCH | CVAR_ARCHIVE );
	r_ignoreMaterialMaxDimension = Cvar_Get( "r_ignoreMaterialMaxDimension", "0",  CVAR_LATCH | CVAR_ARCHIVE );
	r_replaceMaterialMinDimensionIfPresentWithMaxDimension
		= Cvar_Get( "r_replaceMaterialMinDimensionIfPresentWithMaxDimension", "0",  CVAR_LATCH | CVAR_ARCHIVE );
	r_colorMipLevels = Cvar_Get( "r_colorMipLevels", "0", CVAR_LATCH );
	r_colorbits = Cvar_Get( "r_colorbits", "0",  CVAR_LATCH );
	r_alphabits = Cvar_Get( "r_alphabits", "0",  CVAR_LATCH );
	r_stencilbits = Cvar_Get( "r_stencilbits", "8",  CVAR_LATCH );
	r_depthbits = Cvar_Get( "r_depthbits", "0",  CVAR_LATCH );
	r_ext_multisample = Cvar_Get( "r_ext_multisample", "0",  CVAR_LATCH | CVAR_ARCHIVE );
	r_mode = Cvar_Get( "r_mode", "-2", CVAR_LATCH | CVAR_ARCHIVE );
	r_noBorder = Cvar_Get( "r_noBorder", "0", CVAR_ARCHIVE );
	r_fullscreen = Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE );
	r_customwidth = Cvar_Get( "r_customwidth", "1600", CVAR_LATCH | CVAR_ARCHIVE );
	r_customheight = Cvar_Get( "r_customheight", "1024", CVAR_LATCH | CVAR_ARCHIVE );
	r_simpleMipMaps = Cvar_Get( "r_simpleMipMaps", "0", CVAR_LATCH );
	r_subdivisions = Cvar_Get( "r_subdivisions", "4", CVAR_LATCH );
	r_dynamicLightCastShadows = Cvar_Get( "r_dynamicLightCastShadows", "1", 0 );
	r_precomputedLighting = Cvar_Get( "r_precomputedLighting", "1", CVAR_LATCH );
	r_vertexLighting = Cvar_Get( "r_vertexLighting", "0", CVAR_LATCH | CVAR_ARCHIVE );
	r_exportTextures = Cvar_Get( "r_exportTextures", "0", 0 );
	r_heatHaze = Cvar_Get( "r_heatHaze", "1", CVAR_LATCH | CVAR_ARCHIVE );
	r_noMarksOnTrisurfs = Cvar_Get( "r_noMarksOnTrisurfs", "1", CVAR_CHEAT );
	r_lazyShaders = Cvar_Get( "r_lazyShaders", "0", 0 );

	r_forceFog = Cvar_Get( "r_forceFog", "0", CVAR_CHEAT /* | CVAR_LATCH */ );
	AssertCvarRange( r_forceFog, 0.0f, 1.0f, false );
	r_wolfFog = Cvar_Get( "r_wolfFog", "1", CVAR_CHEAT );
	r_noFog = Cvar_Get( "r_noFog", "0", CVAR_CHEAT );

	r_forceAmbient = Cvar_Get( "r_forceAmbient", "0.125",  CVAR_LATCH );
	AssertCvarRange( r_forceAmbient, 0.0f, 0.3f, false );

	r_smp = Cvar_Get( "r_smp", "0",  CVAR_LATCH );

	// temporary latched variables that can only change over a restart
	r_singleShader = Cvar_Get( "r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH );
	r_stitchCurves = Cvar_Get( "r_stitchCurves", "1", CVAR_CHEAT | CVAR_LATCH );
	r_debugShadowMaps = Cvar_Get( "r_debugShadowMaps", "0", CVAR_CHEAT | CVAR_LATCH );
	r_shadowMapLinearFilter = Cvar_Get( "r_shadowMapLinearFilter", "1", CVAR_CHEAT | CVAR_LATCH );
	r_lightBleedReduction = Cvar_Get( "r_lightBleedReduction", "0", CVAR_CHEAT | CVAR_LATCH );
	r_overDarkeningFactor = Cvar_Get( "r_overDarkeningFactor", "30.0", CVAR_CHEAT | CVAR_LATCH );
	r_shadowMapDepthScale = Cvar_Get( "r_shadowMapDepthScale", "1.41", CVAR_CHEAT | CVAR_LATCH );

	r_parallelShadowSplitWeight = Cvar_Get( "r_parallelShadowSplitWeight", "0.9", CVAR_CHEAT );
	r_parallelShadowSplits = Cvar_Get( "r_parallelShadowSplits", "2", CVAR_CHEAT | CVAR_LATCH );
	AssertCvarRange( r_parallelShadowSplits, 0, MAX_SHADOWMAPS - 1, true );

	// archived variables that can change at any time
	r_lodBias = Cvar_Get( "r_lodBias", "0", 0 );
	r_znear = Cvar_Get( "r_znear", "3", CVAR_CHEAT );
	r_zfar = Cvar_Get( "r_zfar", "0", CVAR_CHEAT );
	r_checkGLErrors = Cvar_Get( "r_ignoreGLErrors", "0", 0 );
	r_fastsky = Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_drawSun = Cvar_Get( "r_drawSun", "0", 0 );
	r_finish = Cvar_Get( "r_finish", "0", CVAR_CHEAT );
	r_textureMode = Cvar_Get( "r_textureMode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	r_swapInterval = Cvar_Get( "r_swapInterval", "0", CVAR_ARCHIVE );
	r_gamma = Cvar_Get( "r_gamma", "1.0", CVAR_ARCHIVE );
	r_facePlaneCull = Cvar_Get( "r_facePlaneCull", "1", 0 );

	r_ambientScale = Cvar_Get( "r_ambientScale", "1.0", CVAR_CHEAT | CVAR_LATCH );
	r_lightScale = Cvar_Get( "r_lightScale", "2", CVAR_CHEAT );

	r_vboFaces = Cvar_Get( "r_vboFaces", "1", CVAR_CHEAT );
	r_vboCurves = Cvar_Get( "r_vboCurves", "1", CVAR_CHEAT );
	r_vboTriangles = Cvar_Get( "r_vboTriangles", "1", CVAR_CHEAT );
	r_vboShadows = Cvar_Get( "r_vboShadows", "1", CVAR_CHEAT );
	r_vboLighting = Cvar_Get( "r_vboLighting", "1", CVAR_CHEAT );
	r_vboModels = Cvar_Get( "r_vboModels", "1", 0 );
	r_vboVertexSkinning = Cvar_Get( "r_vboVertexSkinning", "1",  CVAR_LATCH );
	r_vboDeformVertexes = Cvar_Get( "r_vboDeformVertexes", "1",  CVAR_LATCH );

	r_mergeLeafSurfaces = Cvar_Get( "r_mergeLeafSurfaces", "1",  CVAR_LATCH );

	r_evsmPostProcess = Cvar_Get( "r_evsmPostProcess", "0",  CVAR_LATCH );

	r_printShaders = Cvar_Get( "r_printShaders", "0", 0 );

	r_bloom = Cvar_Get( "r_bloom", "0", CVAR_LATCH | CVAR_ARCHIVE );
	r_bloomBlur = Cvar_Get( "r_bloomBlur", "1.0", CVAR_CHEAT );
	r_bloomPasses = Cvar_Get( "r_bloomPasses", "2", CVAR_CHEAT );
	r_FXAA = Cvar_Get( "r_FXAA", "0", CVAR_LATCH | CVAR_ARCHIVE );
	r_ssao = Cvar_Get( "r_ssao", "0", CVAR_LATCH | CVAR_ARCHIVE );

	// temporary variables that can change at any time
	r_showImages = Cvar_Get( "r_showImages", "0", CVAR_TEMP );

	r_debugSort = Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );

	r_nocurves = Cvar_Get( "r_nocurves", "0", CVAR_CHEAT );
	r_lightScissors = Cvar_Get( "r_lightScissors", "1", CVAR_ARCHIVE );
	AssertCvarRange( r_lightScissors, 0, 2, true );

	r_noLightVisCull = Cvar_Get( "r_noLightVisCull", "0", CVAR_CHEAT );
	r_noInteractionSort = Cvar_Get( "r_noInteractionSort", "0", CVAR_CHEAT );
	r_dynamicLight = Cvar_Get( "r_dynamicLight", "2", CVAR_LATCH | CVAR_ARCHIVE );

	r_staticLight = Cvar_Get( "r_staticLight", "2", CVAR_ARCHIVE );
	r_drawworld = Cvar_Get( "r_drawworld", "1", CVAR_CHEAT );
	r_portalOnly = Cvar_Get( "r_portalOnly", "0", CVAR_CHEAT );
	r_portalSky = Cvar_Get( "cg_skybox", "1", 0 );
	r_max_portal_levels = Cvar_Get( "r_max_portal_levels", "5", 0 );

	r_showSmp = Cvar_Get( "r_showSmp", "0", CVAR_CHEAT );
	r_skipBackEnd = Cvar_Get( "r_skipBackEnd", "0", CVAR_CHEAT );
	r_skipLightBuffer = Cvar_Get( "r_skipLightBuffer", "0", CVAR_CHEAT );

	r_measureOverdraw = Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
	r_lodScale = Cvar_Get( "r_lodScale", "5", CVAR_CHEAT );
	r_lodTest = Cvar_Get( "r_lodTest", "0.5", CVAR_CHEAT );
	r_norefresh = Cvar_Get( "r_norefresh", "0", CVAR_CHEAT );
	r_drawentities = Cvar_Get( "r_drawentities", "1", CVAR_CHEAT );
	r_drawpolies = Cvar_Get( "r_drawpolies", "1", CVAR_CHEAT );
	r_ignore = Cvar_Get( "r_ignore", "1", CVAR_CHEAT );
	r_nocull = Cvar_Get( "r_nocull", "0", CVAR_CHEAT );
	r_novis = Cvar_Get( "r_novis", "0", CVAR_CHEAT );
	r_showcluster = Cvar_Get( "r_showcluster", "0", CVAR_CHEAT );
	r_speeds = Cvar_Get( "r_speeds", "0", 0 );
	r_verbose = Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
	r_logFile = Cvar_Get( "r_logFile", "0", CVAR_CHEAT );
	r_debugSurface = Cvar_Get( "r_debugSurface", "0", CVAR_CHEAT );
	r_nobind = Cvar_Get( "r_nobind", "0", CVAR_CHEAT );
	r_clear = Cvar_Get( "r_clear", "1", 0 );
	r_offsetFactor = Cvar_Get( "r_offsetFactor", "-1", CVAR_CHEAT );
	r_offsetUnits = Cvar_Get( "r_offsetUnits", "-2", CVAR_CHEAT );

	r_physicalMapping = Cvar_Get( "r_physicalMapping", "1", CVAR_LATCH | CVAR_ARCHIVE );
	r_specularExponentMin = Cvar_Get( "r_specularExponentMin", "0", CVAR_CHEAT );
	r_specularExponentMax = Cvar_Get( "r_specularExponentMax", "16", CVAR_CHEAT );
	r_specularScale = Cvar_Get( "r_specularScale", "1.0", CVAR_CHEAT | CVAR_LATCH );
	r_specularMapping = Cvar_Get( "r_specularMapping", "1", CVAR_LATCH | CVAR_ARCHIVE );
	r_deluxeMapping = Cvar_Get( "r_deluxeMapping", "1", CVAR_LATCH | CVAR_ARCHIVE );
	r_normalScale = Cvar_Get( "r_normalScale", "1.0", CVAR_ARCHIVE );
	r_normalMapping = Cvar_Get( "r_normalMapping", "1", CVAR_LATCH | CVAR_ARCHIVE );
	r_highQualityNormalMapping = Cvar_Get( "r_highQualityNormalMapping", "0",  CVAR_LATCH );
	r_liquidMapping = Cvar_Get( "r_liquidMapping", "0", CVAR_LATCH | CVAR_ARCHIVE );
	r_reliefDepthScale = Cvar_Get( "r_reliefDepthScale", "0.03", CVAR_CHEAT );
	r_reliefMapping = Cvar_Get( "r_reliefMapping", "0", CVAR_LATCH | CVAR_ARCHIVE );
	r_glowMapping = Cvar_Get( "r_glowMapping", "1", CVAR_LATCH );
	r_reflectionMapping = Cvar_Get( "r_reflectionMapping", "0", CVAR_LATCH | CVAR_ARCHIVE );

	r_wrapAroundLighting = Cvar_Get( "r_wrapAroundLighting", "0.7", CVAR_CHEAT | CVAR_LATCH );
	r_halfLambertLighting = Cvar_Get( "r_halfLambertLighting", "1", CVAR_LATCH | CVAR_ARCHIVE );
	r_rimLighting = Cvar_Get( "r_rimLighting", "0",  CVAR_LATCH | CVAR_ARCHIVE );
	r_rimExponent = Cvar_Get( "r_rimExponent", "3", CVAR_CHEAT | CVAR_LATCH );
	AssertCvarRange( r_rimExponent, 0.5, 8.0, false );

	r_drawBuffer = Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
	r_lockpvs = Cvar_Get( "r_lockpvs", "0", CVAR_CHEAT );
	r_noportals = Cvar_Get( "r_noportals", "0", CVAR_CHEAT );

	r_shadows = Cvar_Get( "cg_shadows", "1",  CVAR_LATCH );
	AssertCvarRange( r_shadows, 0, Util::ordinal(shadowingMode_t::SHADOWING_EVSM32), true );

	r_softShadows = Cvar_Get( "r_softShadows", "0",  CVAR_LATCH );
	AssertCvarRange( r_softShadows, 0, 6, true );

	r_softShadowsPP = Cvar_Get( "r_softShadowsPP", "0",  CVAR_LATCH );

	r_shadowBlur = Cvar_Get( "r_shadowBlur", "2",  CVAR_LATCH );

	r_shadowMapQuality = Cvar_Get( "r_shadowMapQuality", "3",  CVAR_LATCH );
	AssertCvarRange( r_shadowMapQuality, 0, 4, true );

	r_shadowMapSizeUltra = Cvar_Get( "r_shadowMapSizeUltra", "1024",  CVAR_LATCH );
	AssertCvarRange( r_shadowMapSizeUltra, 32, 2048, true );

	r_shadowMapSizeVeryHigh = Cvar_Get( "r_shadowMapSizeVeryHigh", "512",  CVAR_LATCH );
	AssertCvarRange( r_shadowMapSizeVeryHigh, 32, 2048, true );

	r_shadowMapSizeHigh = Cvar_Get( "r_shadowMapSizeHigh", "256",  CVAR_LATCH );
	AssertCvarRange( r_shadowMapSizeHigh, 32, 2048, true );

	r_shadowMapSizeMedium = Cvar_Get( "r_shadowMapSizeMedium", "128",  CVAR_LATCH );
	AssertCvarRange( r_shadowMapSizeMedium, 32, 2048, true );

	r_shadowMapSizeLow = Cvar_Get( "r_shadowMapSizeLow", "64",  CVAR_LATCH );
	AssertCvarRange( r_shadowMapSizeLow, 32, 2048, true );

	r_shadowMapSizeSunUltra = Cvar_Get( "r_shadowMapSizeSunUltra", "1024",  CVAR_LATCH );
	AssertCvarRange( r_shadowMapSizeSunUltra, 32, 2048, true );

	r_shadowMapSizeSunVeryHigh = Cvar_Get( "r_shadowMapSizeSunVeryHigh", "1024",  CVAR_LATCH );
	AssertCvarRange( r_shadowMapSizeSunVeryHigh, 512, 2048, true );

	r_shadowMapSizeSunHigh = Cvar_Get( "r_shadowMapSizeSunHigh", "1024",  CVAR_LATCH );
	AssertCvarRange( r_shadowMapSizeSunHigh, 512, 2048, true );

	r_shadowMapSizeSunMedium = Cvar_Get( "r_shadowMapSizeSunMedium", "1024",  CVAR_LATCH );
	AssertCvarRange( r_shadowMapSizeSunMedium, 512, 2048, true );

	r_shadowMapSizeSunLow = Cvar_Get( "r_shadowMapSizeSunLow", "1024",  CVAR_LATCH );
	AssertCvarRange( r_shadowMapSizeSunLow, 512, 2048, true );

	r_shadowOffsetFactor = Cvar_Get( "r_shadowOffsetFactor", "0", CVAR_CHEAT );
	r_shadowOffsetUnits = Cvar_Get( "r_shadowOffsetUnits", "0", CVAR_CHEAT );
	r_shadowLodBias = Cvar_Get( "r_shadowLodBias", "0", CVAR_CHEAT );
	r_shadowLodScale = Cvar_Get( "r_shadowLodScale", "0.8", CVAR_CHEAT );
	r_noShadowPyramids = Cvar_Get( "r_noShadowPyramids", "0", CVAR_CHEAT );
	r_cullShadowPyramidFaces = Cvar_Get( "r_cullShadowPyramidFaces", "0", CVAR_CHEAT );
	r_cullShadowPyramidCurves = Cvar_Get( "r_cullShadowPyramidCurves", "1", CVAR_CHEAT );
	r_cullShadowPyramidTriangles = Cvar_Get( "r_cullShadowPyramidTriangles", "1", CVAR_CHEAT );
	r_noShadowFrustums = Cvar_Get( "r_noShadowFrustums", "0", CVAR_CHEAT );
	r_noLightFrustums = Cvar_Get( "r_noLightFrustums", "1", CVAR_CHEAT );

	r_maxPolys = Cvar_Get( "r_maxpolys", "10000", CVAR_LATCH );  // 600 in vanilla Q3A
	AssertCvarRange( r_maxPolys, 600, 30000, true );

	r_maxPolyVerts = Cvar_Get( "r_maxpolyverts", "100000", CVAR_LATCH );  // 3000 in vanilla Q3A
	AssertCvarRange( r_maxPolyVerts, 3000, 200000, true );

	r_showTris = Cvar_Get( "r_showTris", "0", CVAR_CHEAT );
	r_showSky = Cvar_Get( "r_showSky", "0", CVAR_CHEAT );
	r_showShadowVolumes = Cvar_Get( "r_showShadowVolumes", "0", CVAR_CHEAT );
	r_showShadowLod = Cvar_Get( "r_showShadowLod", "0", CVAR_CHEAT );
	r_showShadowMaps = Cvar_Get( "r_showShadowMaps", "0", CVAR_CHEAT );
	r_showSkeleton = Cvar_Get( "r_showSkeleton", "0", CVAR_CHEAT );
	r_showEntityTransforms = Cvar_Get( "r_showEntityTransforms", "0", CVAR_CHEAT );
	r_showLightTransforms = Cvar_Get( "r_showLightTransforms", "0", CVAR_CHEAT );
	r_showLightInteractions = Cvar_Get( "r_showLightInteractions", "0", CVAR_CHEAT );
	r_showLightScissors = Cvar_Get( "r_showLightScissors", "0", CVAR_CHEAT );
	r_showLightBatches = Cvar_Get( "r_showLightBatches", "0", CVAR_CHEAT );
	r_showLightGrid = Cvar_Get( "r_showLightGrid", "0", CVAR_CHEAT );
	r_showLightTiles = Cvar_Get("r_showLightTiles", "0", CVAR_CHEAT | CVAR_LATCH );
	r_showBatches = Cvar_Get( "r_showBatches", "0", CVAR_CHEAT );
	r_showLightMaps = Cvar_Get( "r_showLightMaps", "0", CVAR_CHEAT | CVAR_LATCH );
	r_showDeluxeMaps = Cvar_Get( "r_showDeluxeMaps", "0", CVAR_CHEAT | CVAR_LATCH );
	r_showNormalMaps = Cvar_Get( "r_showNormalMaps", "0", CVAR_CHEAT | CVAR_LATCH );
	r_showMaterialMaps = Cvar_Get( "r_showMaterialMaps", "0", CVAR_CHEAT | CVAR_LATCH );
	r_showAreaPortals = Cvar_Get( "r_showAreaPortals", "0", CVAR_CHEAT );
	r_showCubeProbes = Cvar_Get( "r_showCubeProbes", "0", CVAR_CHEAT );
	r_showBspNodes = Cvar_Get( "r_showBspNodes", "0", CVAR_CHEAT );
	r_showParallelShadowSplits = Cvar_Get( "r_showParallelShadowSplits", "0", CVAR_CHEAT | CVAR_LATCH );
	r_showDecalProjectors = Cvar_Get( "r_showDecalProjectors", "0", CVAR_CHEAT );

	r_fontScale = Cvar_Get( "r_fontScale", "36",  CVAR_LATCH );
}
