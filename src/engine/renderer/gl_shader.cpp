/*
===========================================================================
Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// gl_shader.cpp -- GLSL shader handling

#include <common/FileSystem.h>
#include "gl_shader.h"
#include "Material.h"
#include "DaemonEmbeddedFiles/EngineShaders.h"

// We currently write GLBinaryHeader to a file and memcpy all over it.
// Make sure it's a pod, so we don't put a std::string in it or something
// and try to memcpy over that or binary write an std::string to a file.
static_assert(IsPod<GLBinaryHeader>, "Value must be a pod while code in this cpp file reads and writes this object to file as binary.");

// set via command line args only since this allows arbitrary code execution
static Cvar::Cvar<std::string> shaderpath(
	"shaderpath", "path to load GLSL source files at runtime", Cvar::INIT | Cvar::TEMPORARY, "");

static Cvar::Cvar<bool> r_glslCache(
	"r_glslCache", "cache compiled GLSL shader binaries in the homepath", Cvar::NONE, true);

static Cvar::Cvar<bool> r_logUnmarkedGLSLBuilds(
	"r_logUnmarkedGLSLBuilds", "Log building information for GLSL shaders that are built after the map is loaded",
	Cvar::NONE, true );

// shaderKind's value will be determined later based on command line setting or absence of.
ShaderKind shaderKind = ShaderKind::Unknown;

// *INDENT-OFF*

GLShader_cull                            *gl_cullShader = nullptr;
GLShader_depthReduction                  *gl_depthReductionShader = nullptr;
GLShader_clearSurfaces                   *gl_clearSurfacesShader = nullptr;
GLShader_processSurfaces                 *gl_processSurfacesShader = nullptr;

GLShader_blur                            *gl_blurShader = nullptr;
GLShader_cameraEffects                   *gl_cameraEffectsShader = nullptr;
GLShader_contrast                        *gl_contrastShader = nullptr;
GLShader_fogGlobal                       *gl_fogGlobalShader = nullptr;
GLShader_fxaa                            *gl_fxaaShader = nullptr;
GLShader_motionblur                      *gl_motionblurShader = nullptr;
GLShader_ssao                            *gl_ssaoShader = nullptr;

GLShader_depthtile1                      *gl_depthtile1Shader = nullptr;
GLShader_depthtile2                      *gl_depthtile2Shader = nullptr;
GLShader_lighttile                       *gl_lighttileShader = nullptr;

GLShader_generic                         *gl_genericShader = nullptr;
GLShader_genericMaterial                 *gl_genericShaderMaterial = nullptr;
GLShader_lightMapping                    *gl_lightMappingShader = nullptr;
GLShader_lightMappingMaterial            *gl_lightMappingShaderMaterial = nullptr;
GLShader_fogQuake3                       *gl_fogQuake3Shader = nullptr;
GLShader_fogQuake3Material               *gl_fogQuake3ShaderMaterial = nullptr;
GLShader_heatHaze                        *gl_heatHazeShader = nullptr;
GLShader_heatHazeMaterial                *gl_heatHazeShaderMaterial = nullptr;
GLShader_liquid                          *gl_liquidShader = nullptr;
GLShader_liquidMaterial                  *gl_liquidShaderMaterial = nullptr;
GLShader_portal                          *gl_portalShader = nullptr;
GLShader_reflection                      *gl_reflectionShader = nullptr;
GLShader_reflectionMaterial              *gl_reflectionShaderMaterial = nullptr;
GLShader_screen                          *gl_screenShader = nullptr;
GLShader_screenMaterial                  *gl_screenShaderMaterial = nullptr;
GLShader_skybox                          *gl_skyboxShader = nullptr;
GLShader_skyboxMaterial                  *gl_skyboxShaderMaterial = nullptr;
GlobalUBOProxy                           *globalUBOProxy = nullptr;
GLShaderManager                           gl_shaderManager;

namespace // Implementation details
{
	NORETURN inline void ThrowShaderError(Str::StringRef msg)
	{
		throw ShaderException(msg.c_str());
	}

	const char* GetInternalShader(Str::StringRef filename)
	{
		auto it = EngineShaders::FileMap.find(filename);
		if (it != EngineShaders::FileMap.end())
			return it->second.data;
		return nullptr;
	}

	void CRLFToLF(std::string& source)
	{
		size_t sourcePos = 0;
		size_t keepPos = 0;

		auto keep = [&](size_t keepLength)
		{
			if (sourcePos > 0)
				std::copy(source.begin() + sourcePos, source.begin() + sourcePos + keepLength,
					source.begin() + keepPos);
			keepPos += keepLength;
		};

		for (;;)
		{
			size_t targetPos = source.find("\r\n", sourcePos);
			// If we don't find a line break, shuffle what's left
			// into place and we're done.
			if (targetPos == std::string::npos)
			{
				size_t remainingLength = source.length() - sourcePos;
				keep(remainingLength);
				break;
			}
			// If we do find a line break, shuffle what's before it into place
			// except for the '\r\n'. But then tack on a '\n',
			// resulting in effectively just losing the '\r' in the sequence.
			size_t keepLength = (targetPos - sourcePos);
			keep(keepLength);
			source[keepPos] = '\n';
			++keepPos;
			sourcePos = targetPos + 2;
		}
		source.resize(keepPos);
	}

	// CR/LF's can wind up in the raw files because of how
	// version control system works and how Windows works.
	// Remove them so we are always comparing apples with apples.
	void NormalizeShaderText( std::string& text )
	{
		// A windows user changing the shader file can put
		// Windows can put CRLF's in the file. Make them LF's.
		CRLFToLF(text);
	}

	std::string GetShaderFilename(Str::StringRef filename)
	{
		std::string shaderBase = GetShaderPath();
		if (shaderBase.empty())
			return shaderBase;
		std::string shaderFileName = FS::Path::Build(shaderBase, filename);
		return shaderFileName;
	}

	std::string GetShaderText(Str::StringRef filename)
	{
		// Shader type should be set during initialisation.
		if (shaderKind == ShaderKind::BuiltIn)
		{
			// Look for the shader internally. If not found, look for it externally.
			// If found neither internally or externally of if empty, then Error.
			auto text_ptr = GetInternalShader(filename);
			if (text_ptr == nullptr)
				ThrowShaderError(Str::Format("No shader found for shader: %s", filename));
			return text_ptr;
		}
		else if (shaderKind == ShaderKind::External)
		{
			std::string shaderText;
			std::string shaderFilename = GetShaderFilename(filename);

			Log::Notice("Loading shader '%s'", shaderFilename);

			std::error_code err;

			FS::File shaderFile = FS::RawPath::OpenRead(shaderFilename, err);
			if (err)
				ThrowShaderError(Str::Format("Cannot load shader from file %s: %s", shaderFilename, err.message()));

			shaderText = shaderFile.ReadAll(err);
			if (err)
				ThrowShaderError(Str::Format("Failed to read shader from file %s: %s", shaderFilename, err.message()));

			auto textPtr = GetInternalShader(filename);
			std::string internalShaderText;
			if (textPtr != nullptr)
				internalShaderText = textPtr;

			// Alert the user when a file does not match its built-in version.
			// When testing shader file changes this is an expected message
			// and helps the tester track which files have changed.
			// We normalize the text by removing CR/LF's so they aren't considered
			// a difference as Windows or the Version Control System can put them in
			// and another OS might read them back and consider that a difference
			// to what's in shaders.cpp or vice versa.
			NormalizeShaderText(internalShaderText);
			NormalizeShaderText(shaderText);
			if (internalShaderText != shaderText)
				Log::Warn("Note shader file differs from built-in shader: %s", shaderFilename);

			if (shaderText.empty())
				ThrowShaderError(Str::Format("Shader from file is empty: %s", shaderFilename));

			return shaderText;
		}
		ThrowShaderError("Internal error. ShaderKind not set.");
	}
}

std::string GetShaderPath()
{
	return shaderpath.Get();
}

GLShaderManager::~GLShaderManager()
= default;

void GLShaderManager::FreeAll() {
	_shaders.clear();

	deformShaderCount = 0;
	_deformShaderLookup.clear();

	for ( const ShaderProgramDescriptor& program : shaderProgramDescriptors ) {
		if ( program.id ) {
			glDeleteProgram( program.id );
		}

		if ( program.uniformLocations ) {
			Z_Free( program.uniformLocations );
		}

		if ( program.uniformBlockIndexes ) {
			Z_Free( program.uniformBlockIndexes );
		}

		if ( program.uniformFirewall ) {
			Z_Free( program.uniformFirewall );
		}
	}

	shaderProgramDescriptors.clear();

	for (  const ShaderDescriptor& shader : shaderDescriptors ) {
		if ( shader.id ) {
			glDeleteShader( shader.id );
		}
	}

	shaderDescriptors.clear();

	while ( !_shaderBuildQueue.empty() )
	{
		_shaderBuildQueue.pop();
	}
}

void GLShaderManager::UpdateShaderProgramUniformLocations( GLShader* shader, ShaderProgramDescriptor* shaderProgram ) const {
	size_t uniformSize = shader->_uniformStorageSize;
	size_t numUniforms = shader->_uniforms.size();
	size_t numUniformBlocks = shader->_uniformBlocks.size();

	// create buffer for storing uniform locations
	shaderProgram->uniformLocations = ( GLint* ) Z_Malloc( sizeof( GLint ) * numUniforms );

	// create buffer for uniform firewall
	shaderProgram->uniformFirewall = ( byte* ) Z_Malloc( uniformSize );

	// update uniforms
	for (GLUniform *uniform : shader->_uniforms)
	{
		uniform->UpdateShaderProgramUniformLocation( shaderProgram );
	}

	if( glConfig.uniformBufferObjectAvailable && !glConfig.shadingLanguage420PackAvailable ) {
		// create buffer for storing uniform block indexes
		shaderProgram->uniformBlockIndexes = ( GLuint* ) Z_Malloc( sizeof( GLuint ) * numUniformBlocks );

		// update uniform blocks
		for (GLUniformBlock *uniformBlock : shader->_uniformBlocks)
		{
			uniformBlock->UpdateShaderProgramUniformBlockIndex( shaderProgram );
		}
	}
}

static inline void AddDefine( std::string& defines, const std::string& define, int value )
{
	defines += Str::Format("#ifndef %s\n#define %s %d\n#endif\n", define, define, value);
}

static inline void AddDefine( std::string& defines, const std::string& define, uint32_t value ) {
	defines += Str::Format( "#ifndef %s\n#define %s %d\n#endif\n", define, define, value );
}

// Epsilon for float is 5.96e-08, so exponential notation with 8 decimal places should give exact values.

static inline void AddDefine( std::string& defines, const std::string& define, float value )
{
	defines += Str::Format("#ifndef %s\n#define %s %.8e\n#endif\n", define, define, value);
}

static inline void AddDefine( std::string& defines, const std::string& define, float v1, float v2 )
{
	defines += Str::Format("#ifndef %s\n#define %s vec2(%.8e, %.8e)\n#endif\n", define, define, v1, v2);
}

// Has to match enum genFunc_t in tr_local.h
static const char *const genFuncNames[] = {
	  "DSTEP_NONE",
	  "DSTEP_SIN",
	  "DSTEP_SQUARE",
	  "DSTEP_TRIANGLE",
	  "DSTEP_SAWTOOTH",
	  "DSTEP_INV_SAWTOOTH",
	  "DSTEP_NOISE"
};

static std::string BuildDeformSteps( deformStage_t *deforms, int numDeforms )
{
	std::string steps;

	steps.reserve(256); // Might help a little.

	steps += "#define DEFORM_STEPS ";
	for( int step = 0; step < numDeforms; step++ )
	{
		const deformStage_t &ds = deforms[ step ];

		switch ( ds.deformation )
		{
		case deform_t::DEFORM_WAVE:
			steps += "DSTEP_LOAD_POS(1.0, 1.0, 1.0) ";
			steps += Str::Format("%s(%f, %f, %f) ",
				    genFuncNames[ Util::ordinal(ds.deformationWave.func) ],
				    ds.deformationWave.phase,
				    ds.deformationSpread,
				    ds.deformationWave.frequency );
			steps += "DSTEP_LOAD_NORM(1.0, 1.0, 1.0) ";
			steps += Str::Format("DSTEP_MODIFY_POS(%f, %f, 1.0) ",
				    ds.deformationWave.base,
				    ds.deformationWave.amplitude );
			break;

		case deform_t::DEFORM_BULGE:
			steps += "DSTEP_LOAD_TC(1.0, 0.0, 0.0) ";
			steps += Str::Format("DSTEP_SIN(0.0, %f, %f) ",
				    ds.bulgeWidth,
				    ds.bulgeSpeed * 0.001f );
			steps += "DSTEP_LOAD_NORM(1.0, 1.0, 1.0) ";
			steps += Str::Format("DSTEP_MODIFY_POS(0.0, %f, 1.0) ",
				    ds.bulgeHeight );
			break;

		case deform_t::DEFORM_MOVE:
			steps += Str::Format("%s(%f, 0.0, %f) ",
				    genFuncNames[ Util::ordinal(ds.deformationWave.func) ],
				    ds.deformationWave.phase,
				    ds.deformationWave.frequency );
			steps += Str::Format("DSTEP_LOAD_VEC(%f, %f, %f) ",
				    ds.moveVector[ 0 ],
				    ds.moveVector[ 1 ],
				    ds.moveVector[ 2 ] );
			steps += Str::Format("DSTEP_MODIFY_POS(%f, %f, 1.0) ",
				    ds.deformationWave.base,
				    ds.deformationWave.amplitude );
			break;

		case deform_t::DEFORM_NORMALS:
			steps += "DSTEP_LOAD_POS(1.0, 1.0, 1.0) ";
			steps += Str::Format("DSTEP_NOISE(0.0, 0.0, %f) ",
				    ds.deformationWave.frequency );
			steps += Str::Format("DSTEP_MODIFY_NORM(0.0, %f, 1.0) ",
				    0.98f * ds.deformationWave.amplitude );
			break;

		case deform_t::DEFORM_ROTGROW:
			steps += "DSTEP_LOAD_POS(1.0, 1.0, 1.0) ";
			steps += Str::Format("DSTEP_ROTGROW(%f, %f, %f) ",
				    ds.moveVector[0],
				    ds.moveVector[1],
				    ds.moveVector[2] );
			steps += "DSTEP_LOAD_COLOR(1.0, 1.0, 1.0) ";
			steps += "DSTEP_MODIFY_COLOR(-1.0, 1.0, 0.0) ";
			break;

		default:
			break;
		}
	}

	return steps;
}

struct addedExtension_t {
	bool &available;
	int minGlslVersion;
	std::string name;
};

// Fragment and vertex version declaration.
static const std::vector<addedExtension_t> fragmentVertexAddedExtensions = {
	{ glConfig.gpuShader4Available, 130, "EXT_gpu_shader4" },
	{ glConfig.gpuShader5Available, 400, "ARB_gpu_shader5" },
	{ glConfig.shadingLanguage420PackAvailable, 420, "ARB_shading_language_420pack" },
	{ glConfig.textureFloatAvailable, 0, "ARB_texture_float" },
	{ glConfig.textureGatherAvailable, 400, "ARB_texture_gather" },
	{ glConfig.textureIntegerAvailable, 0, "EXT_texture_integer" },
	{ glConfig.textureRGAvailable, 0, "ARB_texture_rg" },
	{ glConfig.uniformBufferObjectAvailable, 140, "ARB_uniform_buffer_object" },
	{ glConfig.usingBindlessTextures, -1, "ARB_bindless_texture" },
	/* ARB_shader_draw_parameters set to -1, because we might get a 4.6 GL context,
	where the core variables have different names. */
	{ glConfig.shaderDrawParametersAvailable, -1, "ARB_shader_draw_parameters" },
	{ glConfig.SSBOAvailable, 430, "ARB_shader_storage_buffer_object" },
	/* Even though these are part of the GL_KHR_shader_subgroup extension, we need to enable
	the individual extensions for each feature.
	GL_KHR_shader_subgroup itself can't be used in the shader. */
	{ glConfig.shaderSubgroupBasicAvailable, -1, "KHR_shader_subgroup_basic" },
	{ glConfig.shaderSubgroupVoteAvailable, -1, "KHR_shader_subgroup_vote" },
	{ glConfig.shaderSubgroupArithmeticAvailable, -1, "KHR_shader_subgroup_arithmetic" },
	{ glConfig.shaderSubgroupBallotAvailable, -1, "KHR_shader_subgroup_ballot" },
	{ glConfig.shaderSubgroupShuffleAvailable, -1, "KHR_shader_subgroup_shuffle" },
	{ glConfig.shaderSubgroupShuffleRelativeAvailable, -1, "KHR_shader_subgroup_shuffle_relative" },
	{ glConfig.shaderSubgroupQuadAvailable, -1, "KHR_shader_subgroup_quad" },
};

// Compute version declaration, this has to be separate from other shader stages,
// because some things are unique to vertex/fragment or compute shaders.
static const std::vector<addedExtension_t> computeAddedExtensions = {
	{ glConfig.computeShaderAvailable, 430, "ARB_compute_shader" },
	{ glConfig.gpuShader4Available, 130, "EXT_gpu_shader4" },
	{ glConfig.gpuShader5Available, 400, "ARB_gpu_shader5" },
	{ glConfig.uniformBufferObjectAvailable, 140, "ARB_uniform_buffer_object" },
	{ glConfig.SSBOAvailable, 430, "ARB_shader_storage_buffer_object" },
	{ glConfig.shadingLanguage420PackAvailable, 420, "ARB_shading_language_420pack" },
	{ glConfig.explicitUniformLocationAvailable, 430, "ARB_explicit_uniform_location" },
	{ glConfig.shaderImageLoadStoreAvailable, 420, "ARB_shader_image_load_store" },
	{ glConfig.shaderAtomicCountersAvailable, 420, "ARB_shader_atomic_counters" },
	/* ARB_shader_atomic_counter_ops set to -1,
	because we might get a 4.6 GL context, where the core functions have different names. */
	{ glConfig.shaderAtomicCounterOpsAvailable, -1, "ARB_shader_atomic_counter_ops" },
	{ glConfig.usingBindlessTextures, -1, "ARB_bindless_texture" },
	/* Even though these are part of the GL_KHR_shader_subgroup extension, we need to enable
	the individual extensions for each feature.
	GL_KHR_shader_subgroup itself can't be used in the shader. */
	{ glConfig.shaderSubgroupBasicAvailable, -1, "KHR_shader_subgroup_basic" },
	{ glConfig.shaderSubgroupVoteAvailable, -1, "KHR_shader_subgroup_vote" },
	{ glConfig.shaderSubgroupArithmeticAvailable, -1, "KHR_shader_subgroup_arithmetic" },
	{ glConfig.shaderSubgroupBallotAvailable, -1, "KHR_shader_subgroup_ballot" },
	{ glConfig.shaderSubgroupShuffleAvailable, -1, "KHR_shader_subgroup_shuffle" },
	{ glConfig.shaderSubgroupShuffleRelativeAvailable, -1, "KHR_shader_subgroup_shuffle_relative" },
	{ glConfig.shaderSubgroupQuadAvailable, -1, "KHR_shader_subgroup_quad" },
};

static void addExtension( std::string& str, bool available, int minGlslVersion, const std::string& name ) {
	if ( !available ) {
		return;
	}

	if ( ( glConfig.shadingLanguageVersion < minGlslVersion ) || ( minGlslVersion == -1 ) ) {
		str += Str::Format( "#extension GL_%s : require\n", name );
	}

	str += Str::Format( "#define HAVE_%s 1\n", name );
}

static void AddConst( std::string& str, const std::string& name, int value )
{
	str += Str::Format("const int %s = %d;\n", name, value);
}

static void AddConst( std::string& str, const std::string& name, float value )
{
	str += Str::Format("const float %s = %.8e;\n", name, value);
}

#if 0
static void AddConst( std::string& str, const std::string& name, float v1, float v2 )
{
	str += Str::Format("const vec2 %s = vec2(%.8e, %.8e);\n", name, v1, v2);
}
#endif

static std::string GenVersionDeclaration( const std::vector<addedExtension_t> &addedExtensions ) {
	// Declare version.
	std::string str = Str::Format( "#version %d %s\n\n",
		glConfig.shadingLanguageVersion,
		glConfig.shadingLanguageVersion >= 150 ? ( glConfig.glCoreProfile ? "core" : "compatibility" ) : "" );

	// Add supported GLSL extensions.
	for ( const auto& addedExtension : addedExtensions ) {
		addExtension( str, addedExtension.available, addedExtension.minGlslVersion, addedExtension.name );
	}

	return str;
}

static std::string GenFragmentVertexVersionDeclaration() {
	return GenVersionDeclaration( fragmentVertexAddedExtensions );
}

static std::string GenComputeVersionDeclaration() {
	return GenVersionDeclaration( computeAddedExtensions );
}

static std::string GenCompatHeader() {
	std::string str;

	// definition of functions missing in early GLSL
	if( glConfig.shadingLanguageVersion <= 120 ) {
		str += "float smoothstep(float edge0, float edge1, float x) { float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0); return t * t * (3.0 - 2.0 * t); }\n";
	}

	if ( !glConfig.gpuShader5Available && glConfig.gpuShader4Available )
	{
		str +=
R"(vec4 unpackUnorm4x8( uint value )
{
	uint x = value & 0xFFu;
	uint y = ( value >> 8u ) & 0xFFu;
	uint z = ( value >> 16u ) & 0xFFu;
	uint w = ( value >> 24u ) & 0xFFu;

	return vec4( x, y, z, w ) / 255.0f;
}
)";
	}

	/* Driver bug: Adrenaline/OGLP drivers fail to recognise the ARB function versions when they return a 4.6 context
	and the shaders get #version 460 core, so add #define's for them here */
	if ( glConfig.driverVendor == glDriverVendor_t::ATI && glConfig.shaderAtomicCounterOpsAvailable ) {
		str += "#define atomicCounterAddARB atomicCounterAdd\n";
		str += "#define atomicCounterSubtractARB atomicCounterSubtract\n";
		str += "#define atomicCounterMinARB atomicCounterMin\n";
		str += "#define atomicCounterMaxARB atomicCounterMax\n";
		str += "#define atomicCounterAndARB atomicCounterAnd\n";
		str += "#define atomicCounterOrARB atomicCounterOr\n";
		str += "#define atomicCounterXorARB atomicCounterXor\n";
		str += "#define atomicCounterExchangeARB atomicCounterExchange\n";
		str += "#define atomicCounterAndARB atomicCounterAnd\n";
	}

	return str;
}

static std::string GenVertexHeader() {
	std::string str;

	// Vertex shader compatibility defines
	if( glConfig.shadingLanguageVersion > 120 ) {
		str =   "#define IN in\n"
			"#define OUT(mode) mode out\n"
			"#define textureCube texture\n"
			"#define texture2D texture\n"
			"#define texture2DProj textureProj\n"
			"#define texture3D texture\n";
	} else {
		str =   "#define IN attribute\n"
			"#define OUT(mode) varying\n";
	}

	if ( glConfig.shaderDrawParametersAvailable ) {
		str += "OUT(flat) int in_drawID;\n";
		str += "OUT(flat) int in_baseInstance;\n";
		str += "#define drawID gl_DrawIDARB\n";
		str += "#define baseInstance gl_BaseInstanceARB\n\n";
	}

	if ( glConfig.usingMaterialSystem ) {
		AddDefine( str, "BIND_MATERIALS", BufferBind::MATERIALS );
		AddDefine( str, "BIND_TEX_DATA", BufferBind::TEX_DATA );
		AddDefine( str, "BIND_TEX_DATA_STORAGE", BufferBind::TEX_DATA_STORAGE );
		AddDefine( str, "BIND_LIGHTMAP_DATA", BufferBind::LIGHTMAP_DATA );
	}

	return str;
}

static std::string GenFragmentHeader() {
	std::string str;

	// Fragment shader compatibility defines
	if( glConfig.shadingLanguageVersion > 120 ) {
		str =   "#define IN(mode) mode in\n"
			"#define DECLARE_OUTPUT(type) out type outputColor;\n"
			"#define textureCube texture\n"
			"#define texture2D texture\n"
			"#define texture2DProj textureProj\n"
			"#define texture3D texture\n";
	} else if( glConfig.gpuShader4Available) {
		str =   "#define IN(mode) varying\n"
			"#define DECLARE_OUTPUT(type) varying out type outputColor;\n";
	} else {
		str =   "#define IN(mode) varying\n"
			"#define outputColor gl_FragColor\n"
			"#define DECLARE_OUTPUT(type) /* empty*/\n";
	}

	if ( glConfig.usingBindlessTextures ) {
		str += "layout(bindless_sampler) uniform;\n";
	}

	if ( glConfig.shaderDrawParametersAvailable ) {
		str += "IN(flat) int in_drawID;\n";
		str += "IN(flat) int in_baseInstance;\n";
		str += "#define drawID in_drawID\n";
		str += "#define baseInstance in_baseInstance\n\n";
	}

	if ( glConfig.shadingLanguage420PackAvailable ) {
		AddDefine( str, "BIND_LIGHTS", BufferBind::LIGHTS );
	}

	if ( glConfig.usingMaterialSystem ) {
		AddDefine( str, "BIND_MATERIALS", BufferBind::MATERIALS );
		AddDefine( str, "BIND_TEX_DATA", BufferBind::TEX_DATA );
		AddDefine( str, "BIND_TEX_DATA_STORAGE", BufferBind::TEX_DATA_STORAGE );
		AddDefine( str, "BIND_LIGHTMAP_DATA", BufferBind::LIGHTMAP_DATA );
	}

	if ( glConfig.pushBufferAvailable ) {
		AddDefine( str, "USE_PUSH_BUFFER", 1 );
	}

	return str;
}

static std::string GenComputeHeader() {
	std::string str;

	// Compute shader compatibility defines
	if ( glConfig.usingMaterialSystem ) {
		AddDefine( str, "MAX_VIEWS", MAX_VIEWS );
		AddDefine( str, "MAX_FRAMES", MAX_FRAMES );
		AddDefine( str, "MAX_VIEWFRAMES", MAX_VIEWFRAMES );
		AddDefine( str, "MAX_SURFACE_COMMAND_BATCHES", MAX_SURFACE_COMMAND_BATCHES );
		AddDefine( str, "MAX_COMMAND_COUNTERS", MAX_COMMAND_COUNTERS );

		AddDefine( str, "BIND_SURFACE_DESCRIPTORS", BufferBind::SURFACE_DESCRIPTORS );
		AddDefine( str, "BIND_SURFACE_COMMANDS", BufferBind::SURFACE_COMMANDS );
		AddDefine( str, "BIND_CULLED_COMMANDS", BufferBind::CULLED_COMMANDS );
		AddDefine( str, "BIND_SURFACE_BATCHES", BufferBind::SURFACE_BATCHES );
		AddDefine( str, "BIND_COMMAND_COUNTERS_ATOMIC", BufferBind::COMMAND_COUNTERS_ATOMIC );
		AddDefine( str, "BIND_COMMAND_COUNTERS_STORAGE", BufferBind::COMMAND_COUNTERS_STORAGE );
		AddDefine( str, "BIND_PORTAL_SURFACES", BufferBind::PORTAL_SURFACES );

		AddDefine( str, "BIND_DEBUG", BufferBind::DEBUG );
	}

	if ( glConfig.pushBufferAvailable ) {
		AddDefine( str, "USE_PUSH_BUFFER", 1 );
	}

	return str;
}

static std::string GenWorldHeader() {
	std::string str;

	// Shader compatibility defines that use map data for compile-time values
	AddDefine( str, "MAX_SURFACE_COMMANDS", materialSystem.maxStages );

	return str;
}

static std::string GenEngineConstants() {
	// Engine constants
	std::string str;

	AddDefine( str, "r_AmbientScale", r_ambientScale.Get() );
	AddDefine( str, "r_SpecularScale", r_specularScale->value );
	AddDefine( str, "r_zNear", r_znear->value );

	AddDefine( str, "M_PI", static_cast< float >( M_PI ) );
	AddDefine( str, "MAX_REF_LIGHTS", MAX_REF_LIGHTS );
	AddDefine( str, "NUM_LIGHT_LAYERS", glConfig.realtimeLightLayers );
	AddDefine( str, "TILE_SIZE", TILE_SIZE );

	AddDefine( str, "r_FBufSize", windowConfig.vidWidth, windowConfig.vidHeight );

	AddDefine( str, "r_tileStep", glState.tileStep[0], glState.tileStep[1] );

	if ( glConfig.realtimeLighting )
	{
		AddDefine( str, "r_realtimeLighting", 1 );
	}

	if ( r_showNormalMaps->integer )
	{
		AddDefine( str, "r_showNormalMaps", 1 );
	}
	else if ( r_showMaterialMaps->integer )
	{
		AddDefine( str, "r_showMaterialMaps", 1 );
	}
	else if ( r_showLightMaps->integer )
	{
		AddDefine( str, "r_showLightMaps", 1 );
	}
	else if ( r_showDeluxeMaps->integer )
	{
		AddDefine( str, "r_showDeluxeMaps", 1 );
	}
	else if ( r_showReflectionMaps.Get() )
	{
		AddDefine( str, "r_showReflectionMaps", 1 );
	}
	else if ( r_showVertexColors.Get() )
	{
		AddDefine( str, "r_showVertexColors", 1 );
	}
	else if ( r_showGlobalMaterials.Get() != 0 )
	{
		AddDefine( str, "r_showGlobalMaterials", r_showGlobalMaterials.Get() );
	}

	if( r_showCubeProbes.Get() )
	{
		AddDefine( str, "r_showCubeProbes", 1 );
	}

	if ( r_materialDebug.Get() )
	{
		AddDefine( str, "r_materialDebug", 1 );
	}

	if ( r_profilerRenderSubGroups.Get() )
	{
		AddDefine( str, "r_profilerRenderSubGroups", 1 );
	}

	if ( glConfig.vboVertexSkinningAvailable )
	{
		AddDefine( str, "r_vertexSkinning", 1 );
		AddConst( str, "MAX_GLSL_BONES", glConfig.maxVertexSkinningBones );
	}
	else
	{
		AddConst( str, "MAX_GLSL_BONES", 4 );
	}

	if ( r_halfLambertLighting->integer )
		AddDefine( str, "r_halfLambertLighting", 1 );

	if ( r_rimLighting->integer )
	{
		AddDefine( str, "r_rimLighting", 1 );
		AddConst( str, "r_RimExponent", r_rimExponent->value );
	}

	if ( r_accurateSRGB.Get() )
	{
		AddDefine( str, "r_accurateSRGB", 1 );
	}

	if ( r_showLightTiles->integer )
	{
		AddDefine( str, "r_showLightTiles", 1 );
	}

	if ( glConfig.normalMapping )
	{
		AddDefine( str, "r_normalMapping", 1 );
	}

	if ( r_liquidMapping->integer )
	{
		AddDefine( str, "r_liquidMapping", 1 );
	}

	if ( glConfig.specularMapping )
	{
		AddDefine( str, "r_specularMapping", 1 );
	}

	if ( glConfig.physicalMapping )
	{
		AddDefine( str, "r_physicalMapping", 1 );
	}

	if ( r_glowMapping->integer )
	{
		AddDefine( str, "r_glowMapping", 1 );
	}

	if ( glConfig.colorGrading )
	{
		AddDefine( str, "r_colorGrading", 1 );
	}

	if ( r_highPrecisionRendering.Get() ) {
		AddDefine( str, "r_highPrecisionRendering", 1 );
	}

	return str;
}

void GLShaderManager::InitDriverInfo()
{
	std::string driverInfo = std::string(glConfig.renderer_string) + glConfig.version_string;
	_driverVersionHash = Com_BlockChecksum(driverInfo.c_str(), static_cast<int>(driverInfo.size()));
}

void GLShaderManager::GenerateBuiltinHeaders() {
	GLVersionDeclaration = GLHeader( "GLVersionDeclaration", GenFragmentVertexVersionDeclaration() );
	GLComputeVersionDeclaration = GLHeader( "GLComputeVersionDeclaration", GenComputeVersionDeclaration() );
	GLCompatHeader = GLHeader( "GLCompatHeader", GenCompatHeader() );
	GLVertexHeader = GLHeader( "GLVertexHeader", GenVertexHeader() );
	GLFragmentHeader = GLHeader( "GLFragmentHeader", GenFragmentHeader() );
	GLComputeHeader = GLHeader( "GLComputeHeader", GenComputeHeader() );
	GLWorldHeader = GLHeader( "GLWorldHeader", GenWorldHeader() );
	GLEngineConstants = GLHeader( "GLEngineConstants", GenEngineConstants() );
}

void GLShaderManager::GenerateWorldHeaders() {
	GLWorldHeader = GLHeader( "GLWorldHeader", GenWorldHeader() );
}

std::string GLShaderManager::GetDeformShaderName( const int index ) {
	if ( ( !tr.world && !tr.loadingMap.size() ) || !index ) {
		return Str::Format( "deformVertexes_%i", index );
	}

	if ( !tr.world ) {
		return Str::Format( "deformVertexes_%s_%i", tr.loadingMap, index );
	}

	return Str::Format( "deformVertexes_%s_%i", tr.world->baseName, index );
}

std::string GLShaderManager::BuildDeformShaderText( const std::string& steps ) {
	std::string shaderText;

	shaderText = steps + "\n";
	shaderText += GetShaderText( "deformVertexes_vp.glsl" );

	return shaderText;
}

int GLShaderManager::GetDeformShaderIndex( deformStage_t *deforms, int numDeforms ) {
	std::string steps = BuildDeformSteps( deforms, numDeforms );
	uint32_t index = _deformShaderLookup[steps];

	if( !index ) {
		std::string shaderText = GLShaderManager::BuildDeformShaderText( steps );
		index = deformShaderCount;
		FindShader( GetDeformShaderName( index ), shaderText, GL_VERTEX_SHADER,
			std::vector<GLHeader*> { &GLVersionDeclaration, &GLVertexHeader } );

		deformShaderCount++;
		_deformShaderLookup[steps] = deformShaderCount;
	} else {
		index--;
	}

	return index;
}

static bool IsUnusedPermutation( const char *compileMacros )
{
	const char* token;
	while ( *( token = COM_ParseExt2( &compileMacros, false ) ) )
	{
		if ( strcmp( token, "USE_DELUXE_MAPPING" ) == 0 )
		{
			if ( !glConfig.deluxeMapping ) return true;
		}
		else if ( strcmp( token, "USE_GRID_DELUXE_MAPPING" ) == 0 )
		{
			if ( !glConfig.deluxeMapping ) return true;
		}
		else if ( strcmp( token, "USE_PHYSICAL_MAPPING" ) == 0 )
		{
			if ( !glConfig.physicalMapping ) return true;
		}
		else if ( strcmp( token, "USE_REFLECTIVE_SPECULAR" ) == 0 )
		{
			/* FIXME: add to the following test: && r_physicalMapping->integer == 0
			when reflective specular is implemented for physical mapping too
			see https://github.com/DaemonEngine/Daemon/issues/355 */
			if ( !glConfig.specularMapping ) return true;
		}
		else if ( strcmp( token, "USE_RELIEF_MAPPING" ) == 0 )
		{
			if ( !glConfig.reliefMapping ) return true;
		}
		else if ( strcmp( token, "USE_HEIGHTMAP_IN_NORMALMAP" ) == 0 )
		{
			if ( !glConfig.reliefMapping && !glConfig.normalMapping ) return true;
		}
	}

	return false;
}

void GLShaderManager::BuildShader( ShaderDescriptor* descriptor ) {
	if ( descriptor->id ) {
		return;
	}

	const int start = Sys::Milliseconds();

	const GLchar* text[1] = { descriptor->shaderSource.data() };
	GLint length[1] = { ( GLint ) descriptor->shaderSource.size() };

	GLuint shader = glCreateShader( descriptor->type );
	GL_CheckErrors();

	glShaderSource( shader, 1, text, length );
	glCompileShader( shader );

	GL_CheckErrors();

	GLint compiled;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );

	if ( !compiled ) {
		std::string log = GetInfoLog( shader );
		std::vector<InfoLogEntry> infoLog = ParseInfoLog( log );
		PrintShaderSource( descriptor->name, shader, infoLog );

		Log::Warn( "Compile log:\n%s", log );

		switch ( descriptor->type ) {
			case GL_VERTEX_SHADER:
				ThrowShaderError( Str::Format( "Couldn't compile vertex shader: %s", descriptor->name ) );
			case GL_FRAGMENT_SHADER:
				ThrowShaderError( Str::Format( "Couldn't compile fragment shader: %s", descriptor->name ) );
			case GL_COMPUTE_SHADER:
				ThrowShaderError( Str::Format( "Couldn't compile compute shader: %s", descriptor->name ) );
			default:
				break;
		}
	}

	descriptor->id = shader;

	const int time = Sys::Milliseconds() - start;
	compileTime += time;
	compileCount++;
	Log::Debug( "Compilation: %i", time );
}

void GLShaderManager::BuildShaderProgram( ShaderProgramDescriptor* descriptor ) {
	if ( descriptor->id ) {
		return;
	}

	const int start = Sys::Milliseconds();

	GLuint program = glCreateProgram();
	GL_CheckErrors();

	for ( const GLuint& shader : descriptor->shaders ) {
		if ( shader ) {
			glAttachShader( program, shader );
		} else {
			break;
		}
	}
	GL_CheckErrors();

	BindAttribLocations( program );

	if ( glConfig.getProgramBinaryAvailable ) {
		glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE );
	}

	GLint linked;
	glLinkProgram( program );

	glGetProgramiv( program, GL_LINK_STATUS, &linked );

	if ( !linked ) {
		Log::Warn( "Link log:" );
		Log::Warn( GetInfoLog( program ) );
		ThrowShaderError( "Shader program failed to link!" );
	}

	descriptor->id = program;

	const int time = Sys::Milliseconds() - start;
	linkTime += time;
	linkCount++;
	Log::Debug( "Program creation + linking: %i", time );
}

ShaderProgramDescriptor* GLShaderManager::FindShaderProgram( std::vector<ShaderEntry>& shaders, const std::string& mainShader ) {
	std::vector<ShaderProgramDescriptor>::iterator it = std::find_if( shaderProgramDescriptors.begin(), shaderProgramDescriptors.end(),
		[&]( const ShaderProgramDescriptor& program ) {
			for ( const ShaderEntry& shader : shaders ) {
				if ( std::find( program.shaderNames, program.shaderNames + program.shaderCount, shader )
					== program.shaderNames + program.shaderCount ) {
					return false;
				}
			}

			return true;
		}
	);

	if ( it == shaderProgramDescriptors.end() ) {
		std::sort( shaders.begin(), shaders.end(),
			[]( const ShaderEntry& lhs, const ShaderEntry& rhs ) {
				return lhs.name < rhs.name;
			}
		);

		ShaderProgramDescriptor desc;
		std::string combinedShaderText;
		std::vector<ShaderDescriptor*> buildQueue;
		buildQueue.reserve( shaders.size() );

		for ( const ShaderEntry& shader : shaders ) {
			std::vector<ShaderDescriptor>::iterator shaderIt = std::find_if( shaderDescriptors.begin(), shaderDescriptors.end(),
				[&]( const ShaderDescriptor& other ) {
					return shader.type == other.type && shader.macro == other.macro && shader.name == other.name;
				}
			);

			if ( shaderIt == shaderDescriptors.end() ) {
				ThrowShaderError( Str::Format( "Shader not found: %s %u", shader.name, shader.macro ) );
			}

			buildQueue.emplace_back( &*shaderIt );

			combinedShaderText += shaderIt->shaderSource;
		}

		desc.checkSum = Com_BlockChecksum( combinedShaderText.c_str(), combinedShaderText.length() );

		if ( !LoadShaderBinary( shaders, mainShader, &desc ) ) {
			for ( ShaderDescriptor* shader : buildQueue ) {
				BuildShader( shader );
				desc.AttachShader( &*shader );
			}
			BuildShaderProgram( &desc );
			SaveShaderBinary( &desc );
		}

		shaderProgramDescriptors.emplace_back( desc );

		return &shaderProgramDescriptors[shaderProgramDescriptors.size() - 1];
	}

	return &*it;
}

bool GLShaderManager::BuildPermutation( GLShader* shader, int index, const bool buildOneShader ) {
	std::string compileMacros;
	if ( !shader->GetCompileMacrosString( index, compileMacros, GLCompileMacro::VERTEX | GLCompileMacro::FRAGMENT ) ) {
		return false;
	}

	if ( IsUnusedPermutation( compileMacros.c_str() ) ) {
		return false;
	}

	// Program already exists
	if ( index < shader->shaderPrograms.size() &&
		shader->shaderPrograms[index].id ) {
		return false;
	}

	Log::Debug( "Building %s shader permutation with macro: %s",
		shader->_name,
		compileMacros.empty() ? "none" : compileMacros );

	if ( buildOneShader ) {
		compileTime = 0;
		compileCount = 0;
		linkTime = 0;
		linkCount = 0;

		cacheLoadTime = 0;
		cacheLoadCount = 0;
		cacheSaveTime = 0;
		cacheSaveCount = 0;
	}

	const int start = Sys::Milliseconds();

	int macroIndex = index & ( ( 1 << shader->_compileMacros.size() ) - 1 );
	int deformIndex = index >> shader->_compileMacros.size();

	if ( index >= shader->shaderPrograms.size() ) {
		shader->shaderPrograms.resize( ( deformIndex + 1 ) << shader->_compileMacros.size() );
	}

	ShaderProgramDescriptor* program;

	std::vector<ShaderEntry> shaders;
	if ( shader->hasVertexShader ) {
		const uint32_t macros = shader->GetUniqueCompileMacros( macroIndex, GLCompileMacro::VERTEX );
		shaders.emplace_back( ShaderEntry{ shader->_name, macros, GL_VERTEX_SHADER } );
		shaders.emplace_back( ShaderEntry{ GetDeformShaderName( deformIndex ), 0, GL_VERTEX_SHADER } );
	}
	if ( shader->hasFragmentShader ) {
		const uint32_t macros = shader->GetUniqueCompileMacros( macroIndex, GLCompileMacro::FRAGMENT );
		shaders.emplace_back( ShaderEntry{ shader->_name, macros, GL_FRAGMENT_SHADER } );
	}
	if ( shader->hasComputeShader ) {
		shaders.emplace_back( ShaderEntry{ shader->_name, 0, GL_COMPUTE_SHADER } );
	}

	program = FindShaderProgram( shaders, shader->_name );

	UpdateShaderProgramUniformLocations( shader, program );
	GL_BindProgram( program );
	shader->SetShaderProgramUniforms( program );
	GL_BindNullProgram();

	// Copy this for a fast look-up, but the values held in program aren't supposed to change after
	shader->shaderPrograms[index] = *program;

	GL_CheckErrors();

	Log::Debug( "Built in: %i ms", Sys::Milliseconds() - start );

	if ( buildOneShader && r_logUnmarkedGLSLBuilds.Get() ) {
		Log::Notice( "Built a glsl shader program in %i ms (compile: %u in %i ms, link: %u in %i ms;"
			" cache: loaded %u in %i ms, saved %u in %i ms)",
			Sys::Milliseconds() - start,
			compileCount, compileTime, linkCount, linkTime,
			cacheLoadCount, cacheLoadTime, cacheSaveCount, cacheSaveTime );
	}

	return true;
}

void GLShaderManager::BuildAll( const bool buildOnlyMarked ) {
	int startTime = Sys::Milliseconds();
	int count = 0;
	compileTime = 0;
	compileCount = 0;
	linkTime = 0;
	linkCount = 0;

	cacheLoadTime = 0;
	cacheLoadCount = 0;
	cacheSaveTime = 0;
	cacheSaveCount = 0;

	if ( buildOnlyMarked ) {
		Log::Notice( "Building only marked GLSL shaders" );
	}

	while ( !_shaderBuildQueue.empty() ) {
		GLShader* shader = _shaderBuildQueue.front();

		if ( buildOnlyMarked ) {
			for ( size_t i = 0; i < shader->shaderProgramsToBuild.size(); i++ ) {
				if ( shader->shaderProgramsToBuild[i] ) {
					count += +BuildPermutation( shader, i, false );
				}
			}
		} else {
			size_t numPermutations = static_cast<size_t>( 1 ) << shader->GetNumOfCompiledMacros();

			for ( size_t i = 0; i < numPermutations; i++ ) {
				// doesn't include deform vertex shaders, those are built elsewhere!
				count += +BuildPermutation( shader, i, false );
			}
		}

		_shaderBuildQueue.pop();
	}

	Log::Notice( "Built %u glsl shader programs in %i ms (compile: %u in %i ms, link: %u in %i ms, init: %u in %i ms;"
		" cache: loaded %u in %i ms, saved %u in %i ms)",
		count, Sys::Milliseconds() - startTime,
		compileCount, compileTime, linkCount, linkTime, initCount, initTime,
		cacheLoadCount, cacheLoadTime, cacheSaveCount, cacheSaveTime );
}

void GLShaderManager::BindBuffers() {
	glBindBufferBase( GL_UNIFORM_BUFFER, BufferBind::LIGHTS, tr.dlightUBO );
}

std::string GLShaderManager::ProcessInserts( const std::string& shaderText ) const {
	std::string out;
	std::istringstream shaderTextStream( shaderText );

	std::string line;
	int insertCount = 0;
	int lineCount = 0;

	while ( std::getline( shaderTextStream, line, '\n' ) ) {
		++lineCount;
		const std::string::size_type position = line.find( "#insert" );
		if ( position == std::string::npos || line.find_first_not_of( " \t" ) != position ) {
			out += line + "\n";
			continue;
		}

		std::string shaderInsertPath = line.substr( position + 8, std::string::npos );

		// Inserted shader lines will start at 10000, 20000 etc. to easily tell them apart from the main shader code
		// #insert recursion is not supported
		++insertCount;
		out += "#line " + std::to_string( insertCount * 10000 ) + " // " + shaderInsertPath + ".glsl\n";

		out += GetShaderText( shaderInsertPath + ".glsl" );
		out += "#line " + std::to_string( lineCount ) + "\n";
	}

	return out;
}

ShaderDescriptor* GLShaderManager::FindShader( const std::string& name, const std::string& mainText,
	const GLenum type, const std::vector<GLHeader*>& headers,
	const uint32_t macro, const std::string& compileMacros, const bool main ) {

	ShaderDescriptor desc{ name, compileMacros, macro, type, main };
	const std::vector<ShaderDescriptor>::iterator it = std::find_if( shaderDescriptors.begin(), shaderDescriptors.end(),
		[&]( const ShaderDescriptor& other ) {
			return desc.type == other.type && desc.macro == other.macro && desc.name == other.name;
		}
	);

	if ( it != shaderDescriptors.end() ) {
		return nullptr;
	}

	std::string combinedShaderText = BuildShaderText( mainText, headers, compileMacros );
	combinedShaderText = ProcessInserts( combinedShaderText );

	desc.shaderSource = combinedShaderText;

	shaderDescriptors.emplace_back( desc );

	return &shaderDescriptors.back();
}

std::string GLShaderManager::BuildShaderText( const std::string& mainShaderText, const std::vector<GLHeader*>& headers,
	const std::string& macros ) {
	std::string combinedText;

	uint32_t count = 0;
	for ( GLHeader* header : headers ) {
		count += header->text.size();
	}

	combinedText.reserve( count );

	for ( GLHeader* header : headers ) {
		combinedText += header->text;
	}

	const char* compileMacrosP = macros.c_str();
	while ( true ) {
		const char* token = COM_ParseExt2( &compileMacrosP, false );

		if ( !token[0] ) {
			break;
		}

		combinedText += Str::Format( "#ifndef %s\n#define %s 1\n#endif\n", token, token );
	}

	combinedText += mainShaderText;

	return combinedText;
}

void GLShaderManager::InitShader( GLShader* shader ) {
	const int start = Sys::Milliseconds();

	shader->_uniformStorageSize = 0;
	for ( std::size_t i = 0; i < shader->_uniforms.size(); i++ ) {
		GLUniform* uniform = shader->_uniforms[i];
		uniform->SetLocationIndex( i );
		uniform->SetFirewallIndex( shader->_uniformStorageSize );
		shader->_uniformStorageSize += uniform->GetSize();
	}

	for ( std::size_t i = 0; i < shader->_uniformBlocks.size(); i++ ) {
		GLUniformBlock* uniformBlock = shader->_uniformBlocks[i];
		uniformBlock->SetLocationIndex( i );
	}

	struct ShaderType {
		bool enabled;
		int type;
		GLenum GLType;

		const char* postfix;
		std::string path;
		uint32_t offset;
		std::vector<GLHeader*> headers;

		std::string mainText = "";
	};

	ShaderType shaderTypes[] = {
		{ shader->hasVertexShader, GLCompileMacro::VERTEX, GL_VERTEX_SHADER, "_vp",
			shader->vertexShaderName,
			uint32_t( GLVersionDeclaration.text.size() ),
			{ &GLVersionDeclaration, &GLCompatHeader, &GLEngineConstants, &GLVertexHeader } },
		{ shader->hasFragmentShader, GLCompileMacro::FRAGMENT, GL_FRAGMENT_SHADER, "_fp",
			shader->fragmentShaderName,
			uint32_t( GLVersionDeclaration.text.size() ),
			{ &GLVersionDeclaration, &GLCompatHeader, &GLEngineConstants, &GLFragmentHeader } },
		{ shader->hasComputeShader, GLCompileMacro::COMPUTE, GL_COMPUTE_SHADER, "_cp",
			shader->computeShaderName,
			uint32_t( GLComputeVersionDeclaration.text.size() ),
			{ &GLComputeVersionDeclaration, &GLCompatHeader, &GLEngineConstants, &GLComputeHeader, &GLWorldHeader } }
	};

	char filename[MAX_QPATH];
	for ( ShaderType& shaderType : shaderTypes ) {
		if ( shaderType.enabled ) {
			Com_sprintf( filename, sizeof( filename ), "%s%s.glsl", shaderType.path.c_str(), shaderType.postfix );

			shaderType.mainText = GetShaderText( filename );
		}
	}

	for ( int i = 0; i < BIT( shader->GetNumOfCompiledMacros() ); i++ ) {
		for ( ShaderType& shaderType : shaderTypes ) {
			if ( !shaderType.enabled ) {
				continue;
			}

			std::string compileMacros;
			if ( !shader->GetCompileMacrosString( i, compileMacros, shaderType.type ) ) {
				continue;
			}

			if ( IsUnusedPermutation( compileMacros.c_str() ) ) {
				continue;
			}

			const uint32_t uniqueMacros = shader->GetUniqueCompileMacros( i, shaderType.type );

			ShaderDescriptor* desc = FindShader( shader->_name, shaderType.mainText, shaderType.GLType, shaderType.headers,
				uniqueMacros, compileMacros, true );

			if ( desc && glConfig.pushBufferAvailable ) {
				desc->shaderSource = RemoveUniformsFromShaderText( desc->shaderSource, shader->_pushUniforms );

				desc->shaderSource.insert( shaderType.offset, globalUniformBlock );
			}

			if ( desc && glConfig.usingMaterialSystem && shader->_useMaterialSystem ) {
				desc->shaderSource = ShaderPostProcess( shader, desc->shaderSource, shaderType.offset );
			}

			initCount++;
		}
	}

	initTime += Sys::Milliseconds() - start;
}

bool GLShaderManager::LoadShaderBinary( const std::vector<ShaderEntry>& shaders, const std::string& mainShader,
	ShaderProgramDescriptor* descriptor ) {
	if ( !r_glslCache.Get() ) {
		return false;
	}

	if ( !GetShaderPath().empty() ) {
		return false;
	}

	// Don't even try if the necessary functions aren't available
	if ( !glConfig.getProgramBinaryAvailable ) {
		return false;
	}

	const int start = Sys::Milliseconds();

	std::error_code err;

	std::string secondaryName;
	for ( const ShaderEntry& shader : shaders ) {
		if ( shader.name != mainShader ) {
			secondaryName += Str::Format( "%s_%u_%u", shader.name, shader.macro, shader.type );
		} else {
			secondaryName += Str::Format( "%u_%u_", shader.macro, shader.type );
		}
	}

	std::string shaderFilename = Str::Format( "glsl/%s/%s.bin", mainShader, secondaryName );
	FS::File shaderFile = FS::HomePath::OpenRead( shaderFilename, err );
	if ( err ) {
		return false;
	}

	GLint success;
	const byte *binaryptr;
	GLBinaryHeader shaderHeader;
	std::string shaderData = shaderFile.ReadAll( err );
	if ( err ) {
		return false;
	}

	if ( shaderData.size() < sizeof( shaderHeader ) ) {
		return false;
	}

	binaryptr = reinterpret_cast<const byte*>( shaderData.data() );

	// Get the shader header from the file
	memcpy( &shaderHeader, binaryptr, sizeof( shaderHeader ) );
	binaryptr += sizeof( shaderHeader );

	/* Check if the header struct is the correct format
	and the binary was produced by the same GL driver */
	if ( shaderHeader.version != GL_SHADER_VERSION || shaderHeader.driverVersionHash != _driverVersionHash ) {
		return false;
	}

	// Make sure the checksums for the source code match
	if ( shaderHeader.checkSum != descriptor->checkSum ) {
		return false;
	}

	if ( shaderHeader.binaryLength != shaderData.size() - sizeof( shaderHeader ) ) {
		Log::Warn( "Shader cache %s has wrong size", shaderFilename );
		return false;
	}

	// Load the shader program
	descriptor->id = glCreateProgram();
	glProgramBinary( descriptor->id, shaderHeader.binaryFormat, binaryptr, shaderHeader.binaryLength );
	glGetProgramiv( descriptor->id, GL_LINK_STATUS, &success );

	if ( !success ) {
		return false;
	}

	for ( const ShaderEntry& shader : shaders ) {
		descriptor->shaderNames[descriptor->shaderCount] = shader;
		descriptor->shaderCount++;
	}

	cacheLoadTime += Sys::Milliseconds() - start;
	cacheLoadCount++;

	return true;
}

void GLShaderManager::SaveShaderBinary( ShaderProgramDescriptor* descriptor ) {
	if ( !r_glslCache.Get() ) {
		return;
	}

	if ( !GetShaderPath().empty() ) {
		return;
	}

	// Don't even try if the necessary functions aren't available
	if( !glConfig.getProgramBinaryAvailable ) {
		return;
	}

	const int start = Sys::Milliseconds();

	// Find output size
	GLBinaryHeader shaderHeader{};
	GLuint binarySize = sizeof( shaderHeader );
	GLint binaryLength;
	glGetProgramiv( descriptor->id, GL_PROGRAM_BINARY_LENGTH, &binaryLength );

	// The binary length may be 0 if there is an error.
	if ( binaryLength <= 0 ) {
		return;
	}

	binarySize += binaryLength;

	byte* binary;
	byte* binaryptr;
	binaryptr = binary = ( byte* )ri.Hunk_AllocateTempMemory( binarySize );

	// Reserve space for the header
	binaryptr += sizeof( shaderHeader );

	// Get the program binary and write it to the buffer
	glGetProgramBinary( descriptor->id, binaryLength, nullptr, &shaderHeader.binaryFormat, binaryptr );

	// Set the header
	shaderHeader.version = GL_SHADER_VERSION;

	shaderHeader.binaryLength = binaryLength;
	shaderHeader.checkSum = descriptor->checkSum;
	shaderHeader.driverVersionHash = _driverVersionHash;

	// Write the header to the buffer
	memcpy( binary, &shaderHeader, sizeof( shaderHeader ) );

	std::string secondaryName;
	for ( uint32_t i = 0; i < descriptor->shaderCount; i++ ) {
		const ShaderEntry& shader = descriptor->shaderNames[i];
		if ( shader.name != descriptor->mainShader || !descriptor->hasMain ) {
			secondaryName += Str::Format( "%s_%u_%u", shader.name, shader.macro, shader.type );
		} else {
			secondaryName += Str::Format( "%u_%u_", shader.macro, shader.type );
		}
	}

	std::string name = descriptor->hasMain ? descriptor->mainShader : "unknown";
	auto fileName = Str::Format( "glsl/%s/%s.bin", name, secondaryName );
	ri.FS_WriteFile( fileName.c_str(), binary, binarySize );

	ri.Hunk_FreeTempMemory( binary );

	cacheSaveTime += Sys::Milliseconds() - start;
	cacheSaveCount++;
}

std::string GLShaderManager::RemoveUniformsFromShaderText( const std::string& shaderText, const std::vector<GLUniform*>& uniforms ) {
	std::istringstream shaderTextStream( shaderText );
	std::string shaderMain;

	std::string line;
	/* Remove local uniform declarations, but avoid removing uniform / storage blocks;
	*  their values will be sourced from a buffer instead
	*  Global uniforms (like u_ViewOrigin) will still be set as regular uniforms */
	while ( std::getline( shaderTextStream, line, '\n' ) ) {
		bool skip = false;
		if ( line.find( "uniform" ) < line.find( "//" ) && line.find( ";" ) != std::string::npos ) {
			for ( GLUniform* uniform : uniforms ) {
				const size_t pos = line.find( uniform->_name );
				if ( pos != std::string::npos && !Str::cisalpha( line[pos + uniform->_name.size()] ) ) {
					skip = true;
					break;
				}
			}
		}

		if ( skip ) {
			continue;
		}

		shaderMain += line + "\n";
	}

	return shaderMain;
}

uint32_t GLShaderManager::GenerateUniformStructDefinesText( const std::vector<GLUniform*>& uniforms,
	const std::string& definesName, const uint32_t offset, std::string& uniformStruct, std::string& uniformDefines ) {
	int pad = offset;
	for ( GLUniform* uniform : uniforms ) {
		uniformStruct += "	" + ( uniform->_isTexture ? "uvec2" : uniform->_type ) + " " + uniform->_name;

		if ( uniform->_components ) {
			uniformStruct += "[" + std::to_string( uniform->_components ) + "]";
		}
		uniformStruct += ";\n";

		for (int p = uniform->_std430Size - uniform->_std430BaseSize; p--; ) {
			uniformStruct += "\tfloat _pad" + std::to_string( ++pad ) + ";\n";
		}

		uniformDefines += "#define ";
		uniformDefines += uniform->_name;

		if ( uniform->_isTexture ) {
			uniformDefines += "_initial";
		}

		uniformDefines += " " + definesName + ".";
		uniformDefines += uniform->_name;

		uniformDefines += "\n";
	}

	uniformDefines += "\n";

	return pad;
}

void GLShaderManager::PostProcessGlobalUniforms() {
	/* Generate the struct and defines in the form of:
	* struct GlobalUniforms {
	*   type uniform0;
	*   type uniform1;
	*   ..
	*   type uniformn;
	* }
	*
	* #define uniformx globalUniforms.uniformx
	*/

	std::string uniformStruct = "\nstruct GlobalUniforms {\n";
	std::string uniformBlock = "layout(std140, binding = "
		+ std::to_string( BufferBind::GLOBAL_DATA )
		+ ") uniform globalUBO {\n"
		+ "GlobalUniforms globalUniforms;\n"
		+ "};\n\n";
	std::string uniformDefines;

	GLuint size;
	std::vector<GLUniform*>* uniforms = &( ( GLShader* ) globalUBOProxy )->_uniforms;
	std::vector<GLUniform*> constUniforms =
		ProcessUniforms( GLUniform::CONST, GLUniform::CONST, !glConfig.usingBindlessTextures, *uniforms, size );

	const uint32_t padding = GenerateUniformStructDefinesText( constUniforms, "globalUniforms", 0, uniformStruct, uniformDefines );

	pushBuffer.constUniformsSize = size;

	std::vector<GLUniform*> frameUniforms =
		ProcessUniforms( GLUniform::FRAME, GLUniform::FRAME, !glConfig.usingBindlessTextures, *uniforms, size );
	
	GenerateUniformStructDefinesText( frameUniforms, "globalUniforms", padding,  uniformStruct, uniformDefines );

	pushBuffer.frameUniformsSize = size;

	uniformStruct += "};\n\n";

	globalUniformBlock = uniformStruct + uniformBlock + uniformDefines;

	uniforms = &( ( GLShader* ) globalUBOProxy )->_pushUniforms;
	uniforms->clear();

	for ( GLUniform* uniform : constUniforms ) {
		uniforms->push_back( uniform );
	}

	for ( GLUniform* uniform : frameUniforms ) {
		uniforms->push_back( uniform );
	}
}

// This will generate all the extra code for material system shaders
std::string GLShaderManager::ShaderPostProcess( GLShader *shader, const std::string& shaderText, const uint32_t offset ) {
	if ( !shader->std140Size ) {
		return shaderText;
	}

	std::string texBuf = glConfig.maxUniformBlockSize >= MIN_MATERIAL_UBO_SIZE ?
		"layout(std140, binding = "
		+ std::to_string( BufferBind::TEX_DATA )
		+ ") uniform texDataUBO {\n"
		"	TexData texData[" + std::to_string( MAX_TEX_BUNDLES ) + "]; \n"
		"};\n\n"
		: "layout(std430, binding = "
		+ std::to_string( BufferBind::TEX_DATA )
		+ ") restrict readonly buffer texDataSSBO {\n"
		"	TexData texData[];\n"
		"};\n\n";
	// We have to store u_TextureMatrix as vec4 + vec2 because otherwise it would be aligned to a vec4 under std140
	std::string texDataBlock = "struct TexData {\n"
		                       "	vec4 u_TextureMatrix;\n"
	                           "	vec2 u_TextureMatrix2;\n"
	                           "	uvec2 u_DiffuseMap;\n"
	                           "	uvec2 u_NormalMap;\n"
	                           "	uvec2 u_HeightMap;\n"
	                           "	uvec2 u_MaterialMap;\n"
	                           "	uvec2 u_GlowMap;\n"
	                           "};\n\n"
	                           + texBuf +
		                       "#define u_TextureMatrix mat3x2( texData[( baseInstance >> 12 ) & 0xFFF].u_TextureMatrix.xy, texData[( baseInstance >> 12 ) & 0xFFF].u_TextureMatrix.zw, texData[( baseInstance >> 12 ) & 0xFFF].u_TextureMatrix2 )\n"
		                       "#define u_DiffuseMap_initial texData[( baseInstance >> 12 ) & 0xFFF].u_DiffuseMap\n"
		                       "#define u_NormalMap_initial texData[( baseInstance >> 12 ) & 0xFFF].u_NormalMap\n"
		                       "#define u_HeightMap_initial texData[( baseInstance >> 12 ) & 0xFFF].u_HeightMap\n"
		                       "#define u_MaterialMap_initial texData[( baseInstance >> 12 ) & 0xFFF].u_MaterialMap\n"
		                       "#define u_GlowMap_initial texData[( baseInstance >> 12 ) & 0xFFF].u_GlowMap\n\n"
		                       "struct LightMapData {\n"
	                           "	uvec2 u_LightMap;\n"
	                           "	uvec2 u_DeluxeMap;\n"
	                           "};\n\n"
	                           "layout(std140, binding = "
		                       + std::to_string( BufferBind::LIGHTMAP_DATA )
		                       + ") uniform lightMapDataUBO {\n"
	                           "	LightMapData lightMapData[256];\n"
	                           "};\n\n"
		                       "#define u_LightMap_initial lightMapData[( baseInstance >> 24 ) & 0xFF].u_LightMap\n"
		                       "#define u_DeluxeMap_initial lightMapData[( baseInstance >> 24 ) & 0xFF].u_DeluxeMap\n\n";

	/* Generate the struct and defines in the form of:
	* struct Material {
	*   type uniform0;
	*   type uniform1;
	*   ..
	*   type uniformn;
	* }
	* 
	* #define uniformx materials[baseInstance].uniformx
	*/

	std::string materialStruct = "\nstruct Material {\n";
	std::string materialDefines;
	GenerateUniformStructDefinesText( shader->_materialSystemUniforms,
		"materials[baseInstance & 0xFFF]", 0, materialStruct, materialDefines );

	materialStruct += "};\n\n";

	// 6 kb for materials
	const uint32_t count = ( 4096 + 2048 ) / shader->GetSTD140Size();
	std::string materialBlock = "layout(std140, binding = "
		+ std::to_string( BufferBind::MATERIALS )
		+ ") uniform materialsUBO {\n"
		"	Material materials[" + std::to_string( count ) + "]; \n"
		"};\n\n";

	std::string shaderMain = RemoveUniformsFromShaderText( shaderText, shader->_materialSystemUniforms );

	std::string newShaderText = "#define USE_MATERIAL_SYSTEM\n" + materialStruct + materialBlock + texDataBlock + materialDefines;
	shaderMain.insert( offset, newShaderText );
	return shaderMain;
}

void GLShaderManager::PrintShaderSource( Str::StringRef programName, GLuint object, std::vector<InfoLogEntry>& infoLog ) const
{
	char *dump;
	int maxLength = 0;

	glGetShaderiv( object, GL_SHADER_SOURCE_LENGTH, &maxLength );

	dump = ( char * ) ri.Hunk_AllocateTempMemory( maxLength );

	glGetShaderSource( object, maxLength, &maxLength, dump );

	std::string buffer;
	std::string delim( "\n" );
	std::string src( dump );

	ri.Hunk_FreeTempMemory( dump );

	int lineNumber = 0;
	size_t pos = 0;

	int infoLogID = -1;
	if ( infoLog.size() > 0 ) {
		infoLogID = 0;
	}

	while ( ( pos = src.find( delim ) ) != std::string::npos ) {
		std::string line = src.substr( 0, pos );
		if ( Str::IsPrefix( "#line ", line ) )
		{
			size_t lineNumEnd = line.find( ' ', 6 );
			Str::ParseInt( lineNumber, line.substr( 6, lineNumEnd - 6 ) );
		}

		std::string number = std::to_string( lineNumber );

		static const int numberWidth = 4;
		int p = numberWidth - number.length();
		p = p < 0 ? 0 : p;
		number.insert( number.begin(), p, ' ' );

		buffer.append( number );
		buffer.append( ": " );
		buffer.append( line );
		buffer.append( delim );

		while ( infoLogID != -1 && infoLog[infoLogID].line == lineNumber ) {
			if ( ( int( line.length() ) > infoLog[infoLogID].character ) && ( infoLog[infoLogID].character != -1 ) ) {
				buffer.append( numberWidth + 2, '-' );
				const size_t position = line.find_first_not_of( "\t" );

				if ( position != std::string::npos ) {
					buffer.append( position, '\t' );
					buffer.append( infoLog[infoLogID].character - position, '-' );
				} else {
					buffer.append( infoLog[infoLogID].character, '-' );
				}
				buffer.append( "^" );
				buffer.append( line.length() - infoLog[infoLogID].character - 1, '-' );

			} else if ( ( line.length() > 0 ) && ( infoLog[infoLogID].token.length() > 0 ) ) {
				size_t position = line.find_first_not_of( "\t" );
				size_t prevPosition = 0;

				buffer.append( numberWidth + 2, '-' );
				if ( position != std::string::npos ) {
					buffer.append( position, '\t' );
				} else {
					position = 0;
				}

				while ( ( position = line.find( infoLog[infoLogID].token, position ) ) && ( position != std::string::npos ) ) {
					buffer.append( position - prevPosition - 1, '-' );
					buffer.append( "^" );
					prevPosition = position;
					position++;
				}
				buffer.append( line.length() - position - 1, '-' );
			} else {
				buffer.append( numberWidth + 2 + line.length(), '^' );
			}

			buffer.append( delim );
			buffer.append( infoLog[infoLogID].error );
			buffer.append( delim );
			buffer.append( delim );

			infoLogID++;

			if ( infoLogID >= int( infoLog.size() ) ) {
				infoLogID = -1;
			}
		}

		src.erase( 0, pos + delim.length() );

		lineNumber++;
	}

	Log::Warn("Source for shader program %s:\n%s", programName, buffer.c_str());
}

std::vector<GLShaderManager::InfoLogEntry> GLShaderManager::ParseInfoLog( const std::string& infoLog ) const {
	std::vector<InfoLogEntry> out;

	std::istringstream infoLogTextStream( infoLog );
	std::string line;
	const char* digits = "0123456789";

	// Info-log is entirely implementation dependent, so different vendors will report it differently
	while ( std::getline( infoLogTextStream, line, '\n' ) ) {
		switch ( glConfig.driverVendor ) {
			case glDriverVendor_t::NVIDIA:
			{
				// Format: <num of attached shader>(<line>): <error>
				size_t lineNum1 = line.find_first_of( '(' );
				if ( lineNum1 != std::string::npos ) {
					lineNum1++;
					const size_t lineNum2 = line.find_first_not_of( digits, lineNum1 );

					if ( lineNum2 != std::string::npos ) {
						int lineNum;
						if ( !Str::ParseInt( lineNum, line.substr( lineNum1, lineNum2 - lineNum1 ) ) ) {
							break;
						}
						int characterNum = -1;
						std::string token;

						// The token producing the error tends to be at the end in ""
						const size_t characterNum2 = line.find_last_of( "\"" );
						if ( characterNum2 != std::string::npos ) {
							const std::string subLine = line.substr( lineNum2, characterNum2 - lineNum2 );
							const size_t characterNum1 = subLine.find_last_of( "\"" );

							token = subLine.substr( characterNum1 + 1 );
						}

						InfoLogEntry entry;
						entry.line = lineNum;
						entry.character = characterNum;
						entry.token = token;
						entry.error = line;
						out.push_back( entry );
					}
				}
				break;
			}
			case glDriverVendor_t::MESA:
			{
				// Format: <num of attached shader>:<line>(<character>): <error>
				size_t num1 = line.find_first_of( ':' );
				if ( num1 != std::string::npos ) {
					num1++;
					size_t num2 = line.find_first_not_of( digits, num1 );
					int lineNum;
					if ( !Str::ParseInt( lineNum, line.substr( num1, num2 - num1 ) ) ) {
						break;
					}
					int characterNum = -1;

					num1 = line.find_first_of( '(' );
					if ( num1 != std::string::npos ) {
						num1++;
						num2 = line.find_first_not_of( digits, num1 );
						if ( !Str::ParseInt( characterNum, line.substr( num1, num2 - num1 ) ) ) {
							break;
						}
					}

					InfoLogEntry entry;
					entry.line = lineNum;
					entry.character = characterNum;
					entry.error = line;
					out.push_back( entry );
				}
				break;
			}
			case glDriverVendor_t::ATI:
				// Format: ERROR: <num of attached shader>:<line>: <error>
			case glDriverVendor_t::INTEL:
				// Format: ERROR: <wtf is this number?>:<line>: <error>
			{
				// Note: Intel sometimes reports incorrect line number for errors
				size_t num1 = line.find_first_not_of( "ERROR: " );
				if ( num1 != std::string::npos ) {
					num1 = line.find_first_not_of( digits, num1 );
					if ( num1 == std::string::npos ) {
						break;
					}
					num1 = line.find_first_of( digits, num1 );
					if ( num1 == std::string::npos ) {
						break;
					}

					const size_t num2 = line.find_first_not_of( digits, num1 );
					if ( num2 == std::string::npos ) {
						break;
					}
					int lineNum;
					if ( !Str::ParseInt( lineNum, line.substr( num1, num2 - num1 ) ) ) {
						break;
					}

					InfoLogEntry entry;
					entry.line = lineNum;
					entry.character = -1;
					entry.error = line;
					out.push_back( entry );
				}
				break;
			}
			case glDriverVendor_t::UNKNOWN:
			default:
				Log::Warn( "Unable to parse shader info log errors: unknown format" );
				return out;
		}
	}

	// Messages in the log can sometimes be out of order
	std::sort( out.begin(), out.end(),
		[]( const InfoLogEntry& lhs, const InfoLogEntry& rhs ) {
			return lhs.line < rhs.line;
		} );

	return out;
}

std::string GLShaderManager::GetInfoLog( GLuint object ) const
{
	char *msg;
	int maxLength = 0;

	if ( glIsShader( object ) )
	{
		glGetShaderiv( object, GL_INFO_LOG_LENGTH, &maxLength );
	}
	else if ( glIsProgram( object ) )
	{
		glGetProgramiv( object, GL_INFO_LOG_LENGTH, &maxLength );
	}
	else
	{
		Log::Warn( "object is not a shader or program" );
		return "";
	}

	msg = ( char * ) ri.Hunk_AllocateTempMemory( maxLength );

	if ( glIsShader( object ) )
	{
		glGetShaderInfoLog( object, maxLength, &maxLength, msg );
	}
	else if ( glIsProgram( object ) )
	{
		glGetProgramInfoLog( object, maxLength, &maxLength, msg );
	}

	std::string out = msg;
	ri.Hunk_FreeTempMemory( msg );

	return out;
}

void GLShaderManager::LinkProgram( GLuint program ) const
{
	GLint linked;

#ifdef GL_ARB_get_program_binary
	// Apparently, this is necessary to get the binary program via glGetProgramBinary
	if( glConfig.getProgramBinaryAvailable )
	{
		glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE );
	}
#endif
	glLinkProgram( program );

	glGetProgramiv( program, GL_LINK_STATUS, &linked );

	if ( !linked )
	{
		Log::Warn( "Link log:" );
		Log::Warn( GetInfoLog( program ) );
		ThrowShaderError( "Shaders failed to link!" );
	}
}

void GLShaderManager::BindAttribLocations( GLuint program ) const
{
	for ( uint32_t i = 0; i < ATTR_INDEX_MAX; i++ )
	{
		glBindAttribLocation( program, i, attributeNames[ i ] );
	}
}

// reflective specular not implemented for PBR yet
bool GLCompileMacro_USE_REFLECTIVE_SPECULAR::HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ( ( permutation & macro->GetBit() ) != 0 && macro->GetType() == USE_PHYSICAL_MAPPING )
		{
			//Log::Notice("conflicting macro! canceling '%s' vs. '%s'", GetName(), macro->GetName());
			return true;
		}
	}

	return false;
}

bool GLCompileMacro_USE_VERTEX_SKINNING::HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	for (const GLCompileMacro* macro : macros)
	{
		//if(GLCompileMacro_USE_VERTEX_ANIMATION* m = dynamic_cast<GLCompileMacro_USE_VERTEX_ANIMATION*>(macro))
		if ( ( permutation & macro->GetBit() ) != 0 && macro->GetType() == USE_VERTEX_ANIMATION )
		{
			//Log::Notice("conflicting macro! canceling '%s' vs. '%s'", GetName(), macro->GetName());
			return true;
		}
	}

	return false;
}

bool GLCompileMacro_USE_VERTEX_SKINNING::MissesRequiredMacros( size_t /*permutation*/, const std::vector< GLCompileMacro * > &/*macros*/ ) const
{
	return !glConfig.vboVertexSkinningAvailable;
}

bool GLCompileMacro_USE_VERTEX_ANIMATION::HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ( ( permutation & macro->GetBit() ) != 0 && macro->GetType() == USE_VERTEX_SKINNING )
		{
			//Log::Notice("conflicting macro! canceling '%s' vs. '%s'", GetName(), macro->GetName());
			return true;
		}
	}

	return false;
}

uint32_t GLCompileMacro_USE_VERTEX_ANIMATION::GetRequiredVertexAttributes() const
{
	uint32_t attribs = ATTR_POSITION2 | ATTR_QTANGENT2;

	return attribs;
}

bool GLCompileMacro_USE_TCGEN_ENVIRONMENT::HasConflictingMacros( size_t permutation, const std::vector<GLCompileMacro*> &macros) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ((permutation & macro->GetBit()) != 0 && (macro->GetType() == USE_TCGEN_LIGHTMAP))
		{
			//Log::Notice("conflicting macro! canceling '%s' vs. '%s'", GetName(), macro->GetName());
			return true;
		}
	}

	return false;
}

bool GLCompileMacro_USE_TCGEN_LIGHTMAP::HasConflictingMacros(size_t permutation, const std::vector<GLCompileMacro*> &macros) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ((permutation & macro->GetBit()) != 0 && (macro->GetType() == USE_TCGEN_ENVIRONMENT))
		{
			//Log::Notice("conflicting macro! canceling '%s' vs. '%s'", GetName(), macro->GetName());
			return true;
		}
	}

	return false;
}

bool GLCompileMacro_USE_DELUXE_MAPPING::HasConflictingMacros(size_t permutation, const std::vector<GLCompileMacro*> &macros) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ((permutation & macro->GetBit()) != 0 && (macro->GetType() == USE_GRID_DELUXE_MAPPING || macro->GetType() == USE_GRID_LIGHTING))
		{
			return true;
		}
	}

	return false;
}

bool GLCompileMacro_USE_GRID_DELUXE_MAPPING::HasConflictingMacros(size_t permutation, const std::vector<GLCompileMacro*> &macros) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ((permutation & macro->GetBit()) != 0 && (macro->GetType() == USE_DELUXE_MAPPING || macro->GetType() == USE_BSP_SURFACE))
		{
			return true;
		}
	}

	return false;
}

bool GLCompileMacro_USE_GRID_LIGHTING::HasConflictingMacros(size_t permutation, const std::vector<GLCompileMacro*> &macros) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ((permutation & macro->GetBit()) != 0 && (macro->GetType() == USE_DELUXE_MAPPING))
		{
			return true;
		}
	}

	return false;
}

bool GLCompileMacro_USE_BSP_SURFACE::HasConflictingMacros(size_t permutation, const std::vector<GLCompileMacro*> &macros) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ((permutation & macro->GetBit()) != 0 && (macro->GetType() == USE_GRID_DELUXE_MAPPING))
		{
			return true;
		}
	}

	return false;
}

uint32_t* GLUniform::WriteToBuffer( uint32_t * ) {
	Sys::Error( "WriteToBuffer not implemented for GLUniform '%s'", _name );
}

void GLShader::RegisterUniform( GLUniform* uniform ) {
	_uniforms.push_back( uniform );
}

GLint GLShader::GetUniformLocation( const GLchar *uniformName ) const {
	ShaderProgramDescriptor* p = GetProgram();
	return glGetUniformLocation( p->id, uniformName );
}

static auto FindUniformForOffset( std::vector<GLUniform*>& uniforms, const GLuint baseOffset ) {
	for ( auto it = uniforms.begin(); it != uniforms.end(); ++it ) {
		if ( 0 == ( ( (*it)->_std430Alignment - 1 ) & baseOffset ) ) {
			return it;
		}
	}

	return uniforms.end();
}

// Compute std140 size/alignment and sort uniforms from highest to lowest alignment
// Note: using the std430 uniform size will give the wrong result for matrix types where
// the number of rows is not 4
GLuint GLShaderManager::SortUniforms( std::vector<GLUniform*>& uniforms ) {
	std::vector<GLUniform*> uniformQueue = uniforms;

	std::stable_sort( uniformQueue.begin(), uniformQueue.end(),
		[]( const GLUniform* lhs, const GLUniform* rhs ) {
			return lhs->_std430Alignment > rhs->_std430Alignment;
		}
	);

	// Sort uniforms from highest to lowest alignment so we don't need to pad uniforms (other than vec3s)
	GLuint align = 4; // mininum alignment since this will be used as an std140 array element
	GLuint structSize = 0;
	uniforms.clear();
	while ( !uniformQueue.empty() || structSize & ( align - 1 ) ) {
		auto iterNext = FindUniformForOffset( uniformQueue, structSize );
		if ( iterNext == uniformQueue.end() ) {
			// add 1 unit of padding
			ASSERT( !uniforms.back()->_components ); // array WriteToBuffer impls don't handle padding correctly
			++structSize;
			++uniforms.back()->_std430Size;
		} else {
			( *iterNext )->_std430Size = ( *iterNext )->_std430BaseSize;
			if ( ( *iterNext )->_components ) {
				ASSERT_GE( ( *iterNext )->_std430Alignment, 4u ); // these would need extra padding in a std130 array
				structSize += ( *iterNext )->_std430Size * ( *iterNext )->_components;
			} else {
				structSize += ( *iterNext )->_std430Size;
			}
			align = std::max( align, ( *iterNext )->_std430Alignment );
			uniforms.push_back( *iterNext );
			uniformQueue.erase( iterNext );
		}
	}

	return structSize;
}

std::vector<GLUniform*> GLShaderManager::ProcessUniforms( const GLUniform::UpdateType minType, const GLUniform::UpdateType maxType,
	const bool skipTextures,
	std::vector<GLUniform*>& uniforms, GLuint& structSize ) {
	std::vector<GLUniform*> tmp;

	tmp.reserve( uniforms.size() );
	for ( GLUniform* uniform : uniforms ) {
		if ( uniform->_updateType >= minType && uniform->_updateType <= maxType
			&& ( !uniform->_isTexture || !skipTextures ) ) {
			tmp.emplace_back( uniform );
		}
	}

	structSize = SortUniforms( tmp );

	return tmp;
}

void GLShader::PostProcessUniforms() {
	if ( _useMaterialSystem ) {
		_materialSystemUniforms = gl_shaderManager.ProcessUniforms( GLUniform::MATERIAL_OR_PUSH, GLUniform::MATERIAL_OR_PUSH,
			true, _uniforms, std140Size );
	}

	if ( glConfig.pushBufferAvailable && !pushSkip ) {
		GLuint unused;
		_pushUniforms = gl_shaderManager.ProcessUniforms( GLUniform::CONST, GLUniform::FRAME,
			!glConfig.usingBindlessTextures, _uniforms, unused );
	}
}

uint32_t GLShader::GetUniqueCompileMacros( size_t permutation, const int type ) const {
	uint32_t macroOut = 0;
	for ( const GLCompileMacro* macro : _compileMacros ) {
		if ( permutation & macro->GetBit() ) {
			if ( macro->HasConflictingMacros( permutation, _compileMacros ) ) {
				continue;
			}

			if ( macro->MissesRequiredMacros( permutation, _compileMacros ) ) {
				continue;
			}

			if ( !( macro->GetShaderTypes() & type ) ) {
				continue;
			}

			macroOut |= BIT( macro->GetType() );
		}
	}

	return macroOut;
}

bool GLShader::GetCompileMacrosString( size_t permutation, std::string &compileMacrosOut, const int type ) const {
	compileMacrosOut.clear();

	for ( const GLCompileMacro* macro : _compileMacros ) {
		if ( permutation & macro->GetBit() ) {
			if ( macro->HasConflictingMacros( permutation, _compileMacros ) ) {
				return false;
			}

			if ( macro->MissesRequiredMacros( permutation, _compileMacros ) ) {
				return false;
			}

			if ( !( macro->GetShaderTypes() & type ) ) {
				return false;
			}

			compileMacrosOut += macro->GetName();
			compileMacrosOut += " ";
		}
	}

	return true;
}

int GLShader::SelectProgram()
{
	int    index = 0;

	int numMacros = static_cast<int>( _compileMacros.size() );

	for ( int i = 0; i < numMacros; i++ )
	{
		if ( _activeMacros & BIT( i ) )
		{
			index += BIT( i );
		}
	}

	return index | ( _deformIndex << numMacros );
}

void GLShader::MarkProgramForBuilding() {
	int index = SelectProgram();

	if ( size_t(index) >= shaderProgramsToBuild.size() ) {
		shaderProgramsToBuild.resize( index + 1 );
	}

	shaderProgramsToBuild[index] = true;
}

GLuint GLShader::GetProgram( const bool buildOneShader ) {
	int index = SelectProgram();

	// program may not be loaded yet because the shader manager hasn't yet gotten to it
	// so try to load it now
	if ( index >= shaderPrograms.size() || !shaderPrograms[index].id ) {
		gl_shaderManager.BuildPermutation( this, index, buildOneShader );
	}

	// program is still not loaded
	if ( index >= shaderPrograms.size() || !shaderPrograms[index].id ) {
		std::string activeMacros;
		size_t      numMacros = _compileMacros.size();

		for ( size_t j = 0; j < numMacros; j++ ) {
			GLCompileMacro* macro = _compileMacros[j];

			int           bit = macro->GetBit();

			if ( IsMacroSet( bit ) ) {
				activeMacros += macro->GetName();
				activeMacros += " ";
			}
		}

		ThrowShaderError( Str::Format( "Invalid shader configuration: shader = '%s', macros = '%s'", _name, activeMacros ) );
	}

	return shaderPrograms[index].id;
}

void GLShader::BindProgram() {
	int index = SelectProgram();

	// program may not be loaded yet because the shader manager hasn't yet gotten to it
	// so try to load it now
	if ( index >= shaderPrograms.size() || !shaderPrograms[index].id )
	{
		gl_shaderManager.BuildPermutation( this, index, true );
	}

	// program is still not loaded
	if ( index >= shaderPrograms.size() || !shaderPrograms[index].id )
	{
		std::string activeMacros;

		for ( const auto& macro : _compileMacros )
		{
			if ( IsMacroSet( macro->GetBit() ) )
			{
				activeMacros += macro->GetName();
				activeMacros += " ";
			}
		}

		ThrowShaderError(Str::Format("Invalid shader configuration: shader = '%s', macros = '%s'", _name, activeMacros ));
	}

	currentProgram = &shaderPrograms[index];

	if ( GLimp_isLogging() )
	{
		std::string macros;

		GetCompileMacrosString( index, macros, GLCompileMacro::VERTEX | GLCompileMacro::FRAGMENT );

		GLIMP_LOGCOMMENT( "--- GL_BindProgram( name = '%s', macros = '%s' ) ---",
			_name, macros );
	}

	GL_BindProgram( &shaderPrograms[index] );
}

void GLShader::DispatchCompute( const GLuint globalWorkgroupX, const GLuint globalWorkgroupY, const GLuint globalWorkgroupZ ) {
	ASSERT_EQ( currentProgram, glState.currentProgram );
	ASSERT( hasComputeShader );
	glDispatchCompute( globalWorkgroupX, globalWorkgroupY, globalWorkgroupZ );
}

void GLShader::DispatchComputeIndirect( const GLintptr indirectBuffer ) {
	ASSERT_EQ( currentProgram, glState.currentProgram );
	ASSERT( hasComputeShader );
	glDispatchComputeIndirect( indirectBuffer );
}

void GLShader::SetRequiredVertexPointers()
{
	uint32_t macroVertexAttribs = 0;

	for ( const auto& macro : _compileMacros )
	{
		if ( IsMacroSet( macro->GetBit() ) )
		{
			macroVertexAttribs |= macro->GetRequiredVertexAttributes();
		}
	}

	uint32_t attribs = _vertexAttribsRequired | _vertexAttribs | macroVertexAttribs; // & ~_vertexAttribsUnsupported);
	GL_VertexAttribsState( attribs );
}

void GLShader::WriteUniformsToBuffer( uint32_t* buffer, const Mode mode, const int filter ) {
	uint32_t* bufPtr = buffer;
	std::vector<GLUniform*>* uniforms;
	switch ( mode ) {
		case MATERIAL:
			uniforms = &_materialSystemUniforms;
			break;
		case PUSH:
			uniforms = &_pushUniforms;
			break;
		default:
			ASSERT_UNREACHABLE();
	}

	for ( GLUniform* uniform : *uniforms ) {
		if ( filter == -1 || uniform->_updateType == filter ) {
			bufPtr = uniform->WriteToBuffer( bufPtr );
		}
	}
}

GLShader_generic::GLShader_generic() :
	GLShader( "generic", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT | ATTR_COLOR,
		false, "generic", "generic" ),
	u_ColorMap( this ),
	u_DepthMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_AlphaThreshold( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ColorModulateColorGen_Float( this ),
	u_ColorModulateColorGen_Uint( this ),
	u_Color_Float( this ),
	u_Color_Uint( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_DepthScale( this ),
	u_ProfilerZero( this ),
	u_ProfilerRenderSubGroups( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_TCGEN_ENVIRONMENT( this ),
	GLCompileMacro_USE_TCGEN_LIGHTMAP( this ),
	GLCompileMacro_USE_DEPTH_FADE( this )
{
}

void GLShader_generic::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_DepthMap" ), 1 );
}

GLShader_genericMaterial::GLShader_genericMaterial() :
	GLShader( "genericMaterial", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT | ATTR_COLOR,
		true, "generic", "generic" ),
	u_ColorMap( this ),
	u_DepthMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_AlphaThreshold( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ColorModulateColorGen_Uint( this ),
	u_Color_Uint( this ),
	u_DepthScale( this ),
	u_ShowTris( this ),
	u_MaterialColour( this ),
	u_ProfilerZero( this ),
	u_ProfilerRenderSubGroups( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_TCGEN_ENVIRONMENT( this ),
	GLCompileMacro_USE_TCGEN_LIGHTMAP( this ),
	GLCompileMacro_USE_DEPTH_FADE( this ) {
}

GLShader_lightMapping::GLShader_lightMapping() :
	GLShader( "lightMapping", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT | ATTR_COLOR,
		false, "lightMapping", "lightMapping" ),
	u_DiffuseMap( this ),
	u_NormalMap( this ),
	u_HeightMap( this ),
	u_MaterialMap( this ),
	u_LightMap( this ),
	u_DeluxeMap( this ),
	u_GlowMap( this ),
	u_EnvironmentMap0( this ),
	u_EnvironmentMap1( this ),
	u_LightGrid1( this ),
	u_LightGrid2( this ),
	u_LightTiles( this ),
	u_TextureMatrix( this ),
	u_SpecularExponent( this ),
	u_ColorModulateColorGen_Float( this ),
	u_ColorModulateColorGen_Uint( this ),
	u_Color_Float( this ),
	u_Color_Uint( this ),
	u_AlphaThreshold( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_ReliefDepthScale( this ),
	u_ReliefOffsetBias( this ),
	u_NormalScale( this ),
	u_EnvironmentInterpolation( this ),
	u_LightGridOrigin( this ),
	u_LightGridScale( this ),
	u_numLights( this ),
	u_Lights( this ),
	u_ProfilerZero( this ),
	u_ProfilerRenderSubGroups( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_BSP_SURFACE( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DELUXE_MAPPING( this ),
	GLCompileMacro_USE_GRID_LIGHTING( this ),
	GLCompileMacro_USE_GRID_DELUXE_MAPPING( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this ),
	GLCompileMacro_USE_REFLECTIVE_SPECULAR( this ),
	GLCompileMacro_USE_PHYSICAL_MAPPING( this )
{
}

void GLShader_lightMapping::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_DiffuseMap" ), BIND_DIFFUSEMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_NormalMap" ), BIND_NORMALMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_HeightMap" ), BIND_HEIGHTMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_MaterialMap" ), BIND_MATERIALMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_LightMap" ), BIND_LIGHTMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_LightGrid1" ), BIND_LIGHTGRID1 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_DeluxeMap" ), BIND_DELUXEMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_LightGrid2" ), BIND_LIGHTGRID2 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_GlowMap" ), BIND_GLOWMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_EnvironmentMap0" ), BIND_ENVIRONMENTMAP0 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_EnvironmentMap1" ), BIND_ENVIRONMENTMAP1 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_LightTiles" ), BIND_LIGHTTILES );
}

GLShader_lightMappingMaterial::GLShader_lightMappingMaterial() :
	GLShader( "lightMappingMaterial", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT | ATTR_COLOR,
		true, "lightMapping", "lightMapping" ),
	u_DiffuseMap( this ),
	u_NormalMap( this ),
	u_HeightMap( this ),
	u_MaterialMap( this ),
	u_LightMap( this ),
	u_DeluxeMap( this ),
	u_GlowMap( this ),
	u_EnvironmentMap0( this ),
	u_EnvironmentMap1( this ),
	u_LightGrid1( this ),
	u_LightGrid2( this ),
	u_LightTiles( this ),
	u_TextureMatrix( this ),
	u_SpecularExponent( this ),
	u_ColorModulateColorGen_Uint( this ),
	u_Color_Uint( this ),
	u_AlphaThreshold( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ReliefDepthScale( this ),
	u_ReliefOffsetBias( this ),
	u_NormalScale( this ),
	u_EnvironmentInterpolation( this ),
	u_LightGridOrigin( this ),
	u_LightGridScale( this ),
	u_numLights( this ),
	u_Lights( this ),
	u_ShowTris( this ),
	u_MaterialColour( this ),
	u_ProfilerZero( this ),
	u_ProfilerRenderSubGroups( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_BSP_SURFACE( this ),
	GLCompileMacro_USE_DELUXE_MAPPING( this ),
	GLCompileMacro_USE_GRID_LIGHTING( this ),
	GLCompileMacro_USE_GRID_DELUXE_MAPPING( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this ),
	GLCompileMacro_USE_REFLECTIVE_SPECULAR( this ),
	GLCompileMacro_USE_PHYSICAL_MAPPING( this ) {
}

GLShader_reflection::GLShader_reflection():
	GLShader( "reflection", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT,
		false, "reflection_CB", "reflection_CB" ),
	u_ColorMapCube( this ),
	u_NormalMap( this ),
	u_HeightMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Bones( this ),
	u_ReliefDepthScale( this ),
	u_ReliefOffsetBias( this ),
	u_NormalScale( this ),
	u_VertexInterpolation( this ),
	u_CameraPosition( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this )
{
}

void GLShader_reflection::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_ColorMapCube" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_HeightMap" ), 15 );
}

GLShader_reflectionMaterial::GLShader_reflectionMaterial() :
	GLShader( "reflectionMaterial", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT,
		true, "reflection_CB", "reflection_CB" ),
	u_ColorMapCube( this ),
	u_NormalMap( this ),
	u_HeightMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ReliefDepthScale( this ),
	u_ReliefOffsetBias( this ),
	u_NormalScale( this ),
	u_CameraPosition( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this ) {
}

GLShader_skybox::GLShader_skybox() :
	GLShader( "skybox", ATTR_POSITION,
		false, "skybox", "skybox" ),
	u_ColorMapCube( this ),
	u_CloudMap( this ),
	u_TextureMatrix( this ),
	u_CloudHeight( this ),
	u_UseCloudMap( this ),
	u_AlphaThreshold( this ),
	u_ModelViewProjectionMatrix( this )
{
}

void GLShader_skybox::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_ColorMapCube" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_CloudMap" ), 1 );
}

GLShader_skyboxMaterial::GLShader_skyboxMaterial() :
	GLShader( "skyboxMaterial", ATTR_POSITION,
		true, "skybox", "skybox" ),
	u_ColorMapCube( this ),
	u_CloudMap( this ),
	u_TextureMatrix( this ),
	u_CloudHeight( this ),
	u_UseCloudMap( this ),
	u_AlphaThreshold( this ),
	u_ModelViewProjectionMatrix( this )
{}

GLShader_fogQuake3::GLShader_fogQuake3() :
	GLShader( "fogQuake3", ATTR_POSITION | ATTR_QTANGENT,
		false, "fogQuake3", "fogQuake3" ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ColorGlobal_Float( this ),
	u_ColorGlobal_Uint( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_ViewOrigin( this ),
	u_FogDensity( this ),
	u_FogDepthVector( this ),
	u_FogEyeT( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this )
{
}

GLShader_fogQuake3Material::GLShader_fogQuake3Material() :
	GLShader( "fogQuake3Material", ATTR_POSITION | ATTR_QTANGENT,
		true, "fogQuake3", "fogQuake3" ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ColorGlobal_Uint( this ),
	u_ViewOrigin( this ),
	u_FogDensity( this ),
	u_FogDepthVector( this ),
	u_FogEyeT( this ),
	GLDeformStage( this ) {
}

GLShader_fogGlobal::GLShader_fogGlobal() :
	GLShader( "fogGlobal", ATTR_POSITION,
		false, "screenSpace", "fogGlobal" ),
	u_DepthMap( this ),
	u_UnprojectMatrix( this ),
	u_Color_Float( this ),
	u_Color_Uint( this ),
	u_ViewOrigin( this ),
	u_FogDensity( this )
{
}

void GLShader_fogGlobal::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_DepthMap" ), 1 );
}

GLShader_heatHaze::GLShader_heatHaze() :
	GLShader( "heatHaze", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT,
		false, "heatHaze", "heatHaze" ),
	u_CurrentMap( this ),
	u_NormalMap( this ),
	u_TextureMatrix( this ),
	u_DeformMagnitude( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ModelViewMatrixTranspose( this ),
	u_ProjectionMatrixTranspose( this ),
	u_Bones( this ),
	u_NormalScale( this ),
	u_VertexInterpolation( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this )
{
}

void GLShader_heatHaze::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_NormalMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_CurrentMap" ), 1 );
}

GLShader_heatHazeMaterial::GLShader_heatHazeMaterial() :
	GLShader( "heatHazeMaterial", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT,
		true, "heatHaze", "heatHaze" ),
	u_CurrentMap( this ),
	u_NormalMap( this ),
	u_TextureMatrix( this ),
	u_DeformEnable( this ),
	u_DeformMagnitude( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ModelViewMatrixTranspose( this ),
	u_ProjectionMatrixTranspose( this ),
	u_NormalScale( this ),
	GLDeformStage( this )
{
}

GLShader_screen::GLShader_screen() :
	GLShader( "screen", ATTR_POSITION,
		false, "screen", "screen" ),
	u_CurrentMap( this ),
	u_ModelViewProjectionMatrix( this )
{
}

void GLShader_screen::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_CurrentMap" ), 0 );
}

GLShader_screenMaterial::GLShader_screenMaterial() :
	GLShader( "screenMaterial", ATTR_POSITION,
		true, "screen", "screen" ),
	u_CurrentMap( this ),
	u_ModelViewProjectionMatrix( this ) {
}

GLShader_portal::GLShader_portal() :
	GLShader( "portal", ATTR_POSITION,
		false, "portal", "portal" ),
	u_CurrentMap( this ),
	u_ModelViewMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_InversePortalRange( this )
{
}

void GLShader_portal::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_CurrentMap" ), 0 );
}

GLShader_contrast::GLShader_contrast() :
	GLShader( "contrast", ATTR_POSITION,
		false, "screenSpace", "contrast" ),
	u_ColorMap( this ) {
}

void GLShader_contrast::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_ColorMap" ), 0 );
}

GLShader_cameraEffects::GLShader_cameraEffects() :
	GLShader( "cameraEffects", ATTR_POSITION,
		false, "screenSpace", "cameraEffects" ),
	u_ColorMap3D( this ),
	u_CurrentMap( this ),
	u_GlobalLightFactor( this ),
	u_ColorModulate( this ),
	u_SRGB( this ),
	u_Tonemap( this ),
	u_TonemapParms( this ),
	u_TonemapExposure( this ),
	u_InverseGamma( this )
{
}

void GLShader_cameraEffects::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_CurrentMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_ColorMap3D" ), 3 );
}

GLShader_blur::GLShader_blur() :
	GLShader( "blur", ATTR_POSITION,
		false, "screenSpace", "blur" ),
	u_ColorMap( this ),
	u_DeformMagnitude( this ),
	u_TexScale( this ),
	u_Horizontal( this )
{
}

void GLShader_blur::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_ColorMap" ), 0 );
}

GLShader_liquid::GLShader_liquid() :
	GLShader( "liquid", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT,
		false, "liquid", "liquid" ),
	u_CurrentMap( this ),
	u_DepthMap( this ),
	u_NormalMap( this ),
	u_PortalMap( this ),
	u_LightGrid1( this ),
	u_LightGrid2( this ),
	u_HeightMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_RefractionIndex( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_UnprojectMatrix( this ),
	u_FresnelPower( this ),
	u_FresnelScale( this ),
	u_FresnelBias( this ),
	u_ReliefDepthScale( this ),
	u_ReliefOffsetBias( this ),
	u_NormalScale( this ),
	u_FogDensity( this ),
	u_FogColor( this ),
	u_SpecularExponent( this ),
	u_LightGridOrigin( this ),
	u_LightGridScale( this ),
	GLCompileMacro_USE_GRID_DELUXE_MAPPING( this ),
	GLCompileMacro_USE_GRID_LIGHTING( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this )
{
}

void GLShader_liquid::SetShaderProgramUniforms( ShaderProgramDescriptor* shaderProgram ) {
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_CurrentMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_PortalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_DepthMap" ), 2 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_NormalMap" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_LightGrid1" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_LightGrid2" ), 7 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_HeightMap" ), 15 );
}

GLShader_liquidMaterial::GLShader_liquidMaterial() :
	GLShader( "liquidMaterial", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT,
		true, "liquid", "liquid" ),
	u_CurrentMap( this ),
	u_DepthMap( this ),
	u_NormalMap( this ),
	u_PortalMap( this ),
	u_LightGrid1( this ),
	u_LightGrid2( this ),
	u_HeightMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_RefractionIndex( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_UnprojectMatrix( this ),
	u_FresnelPower( this ),
	u_FresnelScale( this ),
	u_FresnelBias( this ),
	u_ReliefDepthScale( this ),
	u_ReliefOffsetBias( this ),
	u_NormalScale( this ),
	u_FogDensity( this ),
	u_FogColor( this ),
	u_LightTiles( this ),
	u_SpecularExponent( this ),
	u_LightGridOrigin( this ),
	u_LightGridScale( this ),
	GLCompileMacro_USE_GRID_DELUXE_MAPPING( this ),
	GLCompileMacro_USE_GRID_LIGHTING( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this ) {
}

GLShader_motionblur::GLShader_motionblur() :
	GLShader( "motionblur", ATTR_POSITION,
		false, "screenSpace", "motionblur" ),
	u_ColorMap( this ),
	u_DepthMap( this ),
	u_blurVec( this )
{
}

void GLShader_motionblur::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_DepthMap" ), 1 );
}

GLShader_ssao::GLShader_ssao() :
	GLShader( "ssao", ATTR_POSITION,
		false, "screenSpace", "ssao" ),
	u_DepthMap( this ),
	u_UnprojectionParams( this ),
	u_zFar( this )
{
}

void GLShader_ssao::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_DepthMap" ), 0 );
}

GLShader_depthtile1::GLShader_depthtile1() :
	GLShader( "depthtile1", ATTR_POSITION,
		false, "depthtile1", "depthtile1" ),
	u_DepthMap( this ),
	u_ModelViewProjectionMatrix( this ),
	u_zFar( this )
{
}

void GLShader_depthtile1::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_DepthMap" ), 0 );
}

GLShader_depthtile2::GLShader_depthtile2() :
	GLShader( "depthtile2", ATTR_POSITION,
		false, "screenSpace", "depthtile2" ),
	u_DepthTile1( this ) {
}

void GLShader_depthtile2::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_DepthTile1" ), 0 );
}

GLShader_lighttile::GLShader_lighttile() :
	GLShader( "lighttile", ATTR_POSITION | ATTR_TEXCOORD,
		false, "lighttile", "lighttile" ),
	u_DepthTile2( this ),
	u_Lights( this ),
	u_numLights( this ),
	u_lightLayer( this ),
	u_ModelMatrix( this ),
	u_zFar( this )
{
}

void GLShader_lighttile::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_DepthTile2" ), 0 );
}

GLShader_fxaa::GLShader_fxaa() :
	GLShader( "fxaa", ATTR_POSITION,
		false, "screenSpace", "fxaa" ),
	u_ColorMap( this ) {
}

void GLShader_fxaa::SetShaderProgramUniforms( ShaderProgramDescriptor *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->id, "u_ColorMap" ), 0 );
}

GLShader_cull::GLShader_cull() :
	GLShader( "cull",
		false, "cull", true ),
	u_Frame( this ),
	u_ViewID( this ),
	u_SurfaceDescriptorsCount( this ),
	u_SurfaceCommandsOffset( this ),
	u_Frustum( this ),
	u_UseFrustumCulling( this ),
	u_UseOcclusionCulling( this ),
	u_CameraPosition( this ),
	u_ModelViewMatrix( this ),
	u_FirstPortalGroup( this ),
	u_TotalPortals( this ),
	u_ViewWidth( this ),
	u_ViewHeight( this ),
	u_P00( this ),
	u_P11( this ) {
}

GLShader_depthReduction::GLShader_depthReduction() :
	GLShader( "depthReduction",
		false, "depthReduction" ),
	u_ViewWidth( this ),
	u_ViewHeight( this ),
	u_DepthMap( this ),
	u_InitialDepthLevel( this ) {
}

GLShader_clearSurfaces::GLShader_clearSurfaces() :
	GLShader( "clearSurfaces",
		false, "clearSurfaces" ),
	u_Frame( this ) {
}

GLShader_processSurfaces::GLShader_processSurfaces() :
	GLShader( "processSurfaces",
		false, "processSurfaces" ),
	u_Frame( this ),
	u_ViewID( this ),
	u_SurfaceCommandsOffset( this ) {
}

GlobalUBOProxy::GlobalUBOProxy() :
	/* HACK: A GLShader* is required to initialise uniforms,
	but we don't need the GLSL shader itself, so we won't actually build it */
	GLShader( "proxy", 0,
		false, "screenSpace", "generic", true ),
	// CONST
	u_ColorMap3D( this ),
	u_DepthMap( this ),
	u_PortalMap( this ),
	u_DepthTile1( this ),
	u_DepthTile2( this ),
	u_LightTiles( this ),
	u_LightGrid1( this ),
	u_LightGrid2( this ),
	u_LightGridOrigin( this ),
	u_LightGridScale( this ),
	u_GlobalLightFactor( this ),
	u_SRGB( this ),
	u_FirstPortalGroup( this ),
	u_TotalPortals( this ),
	u_SurfaceDescriptorsCount( this ),
	u_ProfilerZero( this ),
	// FRAME
	u_Frame( this ),
	u_UseFrustumCulling( this ),
	u_UseOcclusionCulling( this ),
	u_blurVec( this ),
	u_numLights( this ),
	u_ColorModulate( this ),
	u_InverseGamma( this ),
	u_Tonemap( this ),
	u_TonemapParms( this ),
	u_TonemapExposure( this ) {
}