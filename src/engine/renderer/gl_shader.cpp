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

// We currently write GLBinaryHeader to a file and memcpy all over it.
// Make sure it's a pod, so we don't put a std::string in it or something
// and try to memcpy over that or binary write an std::string to a file.
static_assert(std::is_pod<GLBinaryHeader>::value, "Value must be a pod while code in this cpp file reads and writes this object to file as binary.");

// set via command line args only since this allows arbitrary code execution
static Cvar::Cvar<std::string> shaderpath(
	"shaderpath", "path to load GLSL source files at runtime", Cvar::INIT | Cvar::TEMPORARY, "");

static Cvar::Cvar<bool> r_glslCache(
	"r_glslCache", "cache compiled GLSL shader binaries in the homepath", Cvar::NONE, true);

extern std::unordered_map<std::string, std::string> shadermap;
// shaderKind's value will be determined later based on command line setting or absence of.
ShaderKind shaderKind = ShaderKind::Unknown;

// *INDENT-OFF*

GLShader_generic2D                       *gl_generic2DShader = nullptr;
GLShader_generic                         *gl_genericShader = nullptr;
GLShader_genericMaterial                 *gl_genericShaderMaterial = nullptr;
GLShader_cull                            *gl_cullShader = nullptr;
GLShader_depthReduction                  *gl_depthReductionShader = nullptr;
GLShader_clearSurfaces                   *gl_clearSurfacesShader = nullptr;
GLShader_processSurfaces                 *gl_processSurfacesShader = nullptr;
GLShader_lightMapping                    *gl_lightMappingShader = nullptr;
GLShader_lightMappingMaterial            *gl_lightMappingShaderMaterial = nullptr;
GLShader_forwardLighting_omniXYZ         *gl_forwardLightingShader_omniXYZ = nullptr;
GLShader_forwardLighting_projXYZ         *gl_forwardLightingShader_projXYZ = nullptr;
GLShader_forwardLighting_directionalSun  *gl_forwardLightingShader_directionalSun = nullptr;
GLShader_shadowFill                      *gl_shadowFillShader = nullptr;
GLShader_reflection                      *gl_reflectionShader = nullptr;
GLShader_reflectionMaterial              *gl_reflectionShaderMaterial = nullptr;
GLShader_skybox                          *gl_skyboxShader = nullptr;
GLShader_skyboxMaterial                  *gl_skyboxShaderMaterial = nullptr;
GLShader_fogQuake3                       *gl_fogQuake3Shader = nullptr;
GLShader_fogQuake3Material               *gl_fogQuake3ShaderMaterial = nullptr;
GLShader_fogGlobal                       *gl_fogGlobalShader = nullptr;
GLShader_heatHaze                        *gl_heatHazeShader = nullptr;
GLShader_heatHazeMaterial                *gl_heatHazeShaderMaterial = nullptr;
GLShader_screen                          *gl_screenShader = nullptr;
GLShader_screenMaterial                  *gl_screenShaderMaterial = nullptr;
GLShader_portal                          *gl_portalShader = nullptr;
GLShader_contrast                        *gl_contrastShader = nullptr;
GLShader_cameraEffects                   *gl_cameraEffectsShader = nullptr;
GLShader_blurX                           *gl_blurXShader = nullptr;
GLShader_blurY                           *gl_blurYShader = nullptr;
GLShader_debugShadowMap                  *gl_debugShadowMapShader = nullptr;
GLShader_liquid                          *gl_liquidShader = nullptr;
GLShader_liquidMaterial                  *gl_liquidShaderMaterial = nullptr;
GLShader_motionblur                      *gl_motionblurShader = nullptr;
GLShader_ssao                            *gl_ssaoShader = nullptr;
GLShader_depthtile1                      *gl_depthtile1Shader = nullptr;
GLShader_depthtile2                      *gl_depthtile2Shader = nullptr;
GLShader_lighttile                       *gl_lighttileShader = nullptr;
GLShader_fxaa                            *gl_fxaaShader = nullptr;
GLShaderManager                           gl_shaderManager;

namespace // Implementation details
{
	NORETURN inline void ThrowShaderError(Str::StringRef msg)
	{
		throw ShaderException(msg.c_str());
	}

	const char* GetInternalShader(Str::StringRef filename)
	{
		auto it = shadermap.find(filename);
		if (it != shadermap.end())
			return it->second.c_str();
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

			// Alert the user when a file does not match it's built-in version.
			// There should be no differences in normal conditions.
			// When testing shader file changes this is an expected message
			// and helps the tester track which files have changed and need
			// to be recommitted to git.
			// If one is not making shader files changes this message
			// indicates there is a mismatch between disk changes and builtins
			// which the application is out of sync with it's files
			// and he translation script needs to be run.
			auto textPtr = GetInternalShader(filename);
			std::string internalShaderText;
			if (textPtr != nullptr)
				internalShaderText = textPtr;

			// Note to the user any differences that might exist between
			// what's on disk and what's compiled into the program in shaders.cpp.
			// The developer should be aware of any differences why they exist but
			// they might be expected or unexpected.
			// If the developer made changes they might want to be reminded of what
			// they have changed while they are working.
			// But it also might be that the developer hasn't made any changes but
			// the compiled code is shaders.cpp is just out of sync with the shader
			// files and that buildshaders.sh might need to be run to re-sync.
			// This message alerts user to either situation and they can decide
			// what's going on from seeing that.
			// We normalize the text by removing CL/LF's so they aren't considered
			// a difference as Windows or the Version Control System can put them in
			// and another OS might read them back and consider that a difference
			// to what's in shader.cpp or vice vesa.
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

void GLShaderManager::freeAll()
{
	_shaders.clear();

	for ( GLint sh : _deformShaders )
		glDeleteShader( sh );

	_deformShaders.clear();
	_deformShaderLookup.clear();

	while ( !_shaderBuildQueue.empty() )
	{
		_shaderBuildQueue.pop();
	}

	_totalBuildTime = 0;
}

void GLShaderManager::UpdateShaderProgramUniformLocations( GLShader *shader, shaderProgram_t *shaderProgram ) const
{
	size_t uniformSize = shader->_uniformStorageSize;
	size_t numUniforms = shader->_uniforms.size();
	size_t numUniformBlocks = shader->_uniformBlocks.size();

	// create buffer for storing uniform locations
	shaderProgram->uniformLocations = ( GLint * ) Z_Malloc( sizeof( GLint ) * numUniforms );

	// create buffer for uniform firewall
	shaderProgram->uniformFirewall = ( byte * ) Z_Malloc( uniformSize );

	// update uniforms
	for (GLUniform *uniform : shader->_uniforms)
	{
		uniform->UpdateShaderProgramUniformLocation( shaderProgram );
	}

	if( glConfig2.uniformBufferObjectAvailable ) {
		// create buffer for storing uniform block indexes
		shaderProgram->uniformBlockIndexes = ( GLuint * ) Z_Malloc( sizeof( GLuint ) * numUniformBlocks );

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

// Epsilon for float is 5.96e-08, so exponential notation with 8 decimal places should give exact values.

static inline void AddDefine( std::string& defines, const std::string& define, float value )
{
	defines += Str::Format("#ifndef %s\n#define %s %.8e\n#endif\n", define, define, value);
}

static inline void AddDefine( std::string& defines, const std::string& define, float v1, float v2 )
{
	defines += Str::Format("#ifndef %s\n#define %s vec2(%.8e, %.8e)\n#endif\n", define, define, v1, v2);
}

static inline void AddDefine( std::string& defines, const std::string& define )
{
	defines += Str::Format("#ifndef %s\n#define %s\n#endif\n", define, define);
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
	{ glConfig2.gpuShader4Available, 130, "EXT_gpu_shader4" },
	{ glConfig2.gpuShader5Available, 400, "ARB_gpu_shader5" },
	{ glConfig2.textureGatherAvailable, 400, "ARB_texture_gather" },
	{ glConfig2.textureIntegerAvailable, 0, "EXT_texture_integer" },
	{ glConfig2.textureRGAvailable, 0, "ARB_texture_rg" },
	{ glConfig2.uniformBufferObjectAvailable, 140, "ARB_uniform_buffer_object" },
	{ glConfig2.bindlessTexturesAvailable, -1, "ARB_bindless_texture" },
	/* ARB_shader_draw_parameters set to -1, because we might get a 4.6 GL context,
	where the core variables have different names. */
	{ glConfig2.shaderDrawParametersAvailable, -1, "ARB_shader_draw_parameters" },
	{ glConfig2.SSBOAvailable, 430, "ARB_shader_storage_buffer_object" },
};

// Compute version declaration, this has to be separate from other shader stages,
// because some things are unique to vertex/fragment or compute shaders.
static const std::vector<addedExtension_t> computeAddedExtensions = {
	{ glConfig2.computeShaderAvailable, 430, "ARB_compute_shader" },
	{ glConfig2.gpuShader4Available, 130, "EXT_gpu_shader4" },
	{ glConfig2.gpuShader5Available, 400, "ARB_gpu_shader5" },
	{ glConfig2.uniformBufferObjectAvailable, 140, "ARB_uniform_buffer_object" },
	{ glConfig2.SSBOAvailable, 430, "ARB_shader_storage_buffer_object" },
	{ glConfig2.shadingLanguage420PackAvailable, 420, "ARB_shading_language_420pack" },
	{ glConfig2.explicitUniformLocationAvailable, 430, "ARB_explicit_uniform_location" },
	{ glConfig2.shaderImageLoadStoreAvailable, 420, "ARB_shader_image_load_store" },
	{ glConfig2.shaderAtomicCountersAvailable, 420, "ARB_shader_atomic_counters" },
	/* ARB_shader_atomic_counter_ops set to -1,
	because we might get a 4.6 GL context, where the core functions have different names. */
	{ glConfig2.shaderAtomicCounterOpsAvailable, -1, "ARB_shader_atomic_counter_ops" },
	{ glConfig2.bindlessTexturesAvailable, -1, "ARB_bindless_texture" },
	/* Even though these are part of the GL_KHR_shader_subgroup extension, we need to enable
	the individual extensions for each feature.
	GL_KHR_shader_subgroup itself can't be used in the shader. */
	{ glConfig2.shaderSubgroupBasicAvailable, -1, "KHR_shader_subgroup_basic" },
	{ glConfig2.shaderSubgroupVoteAvailable, -1, "KHR_shader_subgroup_vote" },
	{ glConfig2.shaderSubgroupArithmeticAvailable, -1, "KHR_shader_subgroup_arithmetic" },
	{ glConfig2.shaderSubgroupBallotAvailable, -1, "KHR_shader_subgroup_ballot" },
	{ glConfig2.shaderSubgroupShuffleAvailable, -1, "KHR_shader_subgroup_shuffle" },
	{ glConfig2.shaderSubgroupShuffleRelativeAvailable, -1, "KHR_shader_subgroup_shuffle_relative" },
	{ glConfig2.shaderSubgroupQuadAvailable, -1, "KHR_shader_subgroup_quad" },
};

static void addExtension( std::string& str, bool available, int minGlslVersion, const std::string& name ) {
	if ( !available ) {
		return;
	}

	if ( ( glConfig2.shadingLanguageVersion < minGlslVersion ) || ( minGlslVersion == -1 ) ) {
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

static void AddConst( std::string& str, const std::string& name, float v1, float v2 )
{
	str += Str::Format("const vec2 %s = vec2(%.8e, %.8e);\n", name, v1, v2);
}

static std::string GenVersionDeclaration( const std::vector<addedExtension_t> &addedExtensions ) {
	// Declare version.
	std::string str = Str::Format( "#version %d %s\n",
		glConfig2.shadingLanguageVersion,
		glConfig2.shadingLanguageVersion >= 150 ? ( glConfig2.glCoreProfile ? "core" : "compatibility" ) : "" );

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
	if( glConfig2.shadingLanguageVersion <= 120 ) {
		str += "float smoothstep(float edge0, float edge1, float x) { float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0); return t * t * (3.0 - 2.0 * t); }\n";
	}

	return str;
}

static std::string GenVertexHeader() {
	std::string str;

	// Vertex shader compatibility defines
	if( glConfig2.shadingLanguageVersion > 120 ) {
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

	if ( glConfig2.shaderDrawParametersAvailable ) {
		str += "OUT(flat) int in_drawID;\n";
		str += "OUT(flat) int in_baseInstance;\n";
		str += "#define drawID gl_DrawIDARB\n";
		str += "#define baseInstance gl_BaseInstanceARB\n\n";
	}

	return str;
}

static std::string GenFragmentHeader() {
	std::string str;

	// Fragment shader compatibility defines
	if( glConfig2.shadingLanguageVersion > 120 ) {
		str =   "#define IN(mode) mode in\n"
			"#define DECLARE_OUTPUT(type) out type outputColor;\n"
			"#define textureCube texture\n"
			"#define texture2D texture\n"
			"#define texture2DProj textureProj\n"
			"#define texture3D texture\n";
	} else if( glConfig2.gpuShader4Available) {
		str =   "#define IN(mode) varying\n"
			"#define DECLARE_OUTPUT(type) varying out type outputColor;\n";
	} else {
		str =   "#define IN(mode) varying\n"
			"#define outputColor gl_FragColor\n"
			"#define DECLARE_OUTPUT(type) /* empty*/\n";
	}

	if ( glConfig2.bindlessTexturesAvailable ) {
		str += "layout(bindless_sampler) uniform;\n";
	}

	if ( glConfig2.shaderDrawParametersAvailable ) {
		str += "IN(flat) int in_drawID;\n";
		str += "IN(flat) int in_baseInstance;\n";
		str += "#define drawID in_drawID\n";
		str += "#define baseInstance in_baseInstance\n\n";
	}

	return str;
}

static std::string GenComputeHeader() {
	std::string str;

	// Compute shader compatibility defines
	AddDefine( str, "MAX_VIEWS", MAX_VIEWS );
	AddDefine( str, "MAX_FRAMES", MAX_FRAMES );
	AddDefine( str, "MAX_VIEWFRAMES", MAX_VIEWFRAMES );
	AddDefine( str, "MAX_SURFACE_COMMAND_BATCHES", MAX_SURFACE_COMMAND_BATCHES );
	AddDefine( str, "MAX_COMMAND_COUNTERS", MAX_COMMAND_COUNTERS );

	if ( glConfig2.bindlessTexturesAvailable ) {
		str += "layout(bindless_image) uniform;\n";
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

	if ( glConfig2.shadowMapping )
	{
		switch( glConfig2.shadowingMode )
		{
			case shadowingMode_t::SHADOWING_ESM16:
			case shadowingMode_t::SHADOWING_ESM32:
				AddDefine( str, "ESM", 1 );
				break;
			case shadowingMode_t::SHADOWING_VSM16:
			case shadowingMode_t::SHADOWING_VSM32:
				AddDefine( str, "VSM", 1 );

				if ( glConfig.hardwareType == glHardwareType_t::GLHW_R300 )
				{
					AddDefine( str, "VSM_CLAMP", 1 );
				}
				break;
			case shadowingMode_t::SHADOWING_EVSM32:
				AddDefine( str, "EVSM", 1 );

				// The exponents for the EVSM techniques should be less than ln(FLT_MAX/FILTER_SIZE)/2 {ln(FLT_MAX/1)/2 ~44.3}
				//         42.9 is the maximum possible value for FILTER_SIZE=15
				//         42.0 is the truncated value that we pass into the sample
				AddConst( str, "r_EVSMExponents", 42.0f, 42.0f );

				if ( r_evsmPostProcess->integer )
				{
					AddDefine( str, "r_EVSMPostProcess", 1 );
				}
				break;
			default:
				DAEMON_ASSERT( false );
				break;
		}

		switch( glConfig2.shadowingMode )
		{
			case shadowingMode_t::SHADOWING_ESM16:
			case shadowingMode_t::SHADOWING_ESM32:
				break;
			case shadowingMode_t::SHADOWING_VSM16:
				AddConst( str, "VSM_EPSILON", 0.0001f );
				break;
			case shadowingMode_t::SHADOWING_VSM32:
				// GLHW_R300 should not be GLDRV_OPENGL3 anyway.
				if ( glConfig.driverType != glDriverType_t::GLDRV_OPENGL3
					|| glConfig.hardwareType == glHardwareType_t::GLHW_R300 )
				{
					AddConst( str, "VSM_EPSILON", 0.0001f );
				}
				else
				{
					AddConst( str, "VSM_EPSILON", 0.000001f );
				}
				break;
			case shadowingMode_t::SHADOWING_EVSM32:
				// This may be wrong, but the code did that before it was rewritten.
				AddConst( str, "VSM_EPSILON", 0.0001f );
				break;
			default:
				DAEMON_ASSERT( false );
				break;
		}

		if ( r_lightBleedReduction->value )
			AddConst( str, "r_lightBleedReduction", r_lightBleedReduction->value );

		if ( r_overDarkeningFactor->value )
			AddConst( str, "r_overDarkeningFactor", r_overDarkeningFactor->value );

		if ( r_shadowMapDepthScale->value )
			AddConst( str, "r_shadowMapDepthScale", r_shadowMapDepthScale->value );

		if ( r_debugShadowMaps->integer )
			AddDefine( str, "r_debugShadowMaps", r_debugShadowMaps->integer );

		if ( r_softShadows->integer == 6 )
			AddDefine( str, "PCSS", 1 );
		else if ( r_softShadows->integer )
			AddConst( str, "r_PCFSamples", r_softShadows->value + 1.0f );

		if ( r_parallelShadowSplits->integer )
			AddDefine( str, Str::Format( "r_parallelShadowSplits_%d", r_parallelShadowSplits->integer ) );

		if ( r_showParallelShadowSplits->integer )
			AddDefine( str, "r_showParallelShadowSplits", 1 );
	}

	if ( r_highPrecisionRendering.Get() )
	{
		AddDefine( str, "r_highPrecisionRendering", 1 );
	}

	if ( glConfig2.realtimeLighting )
	{
		AddDefine( str, "r_realtimeLighting", 1 );
		AddDefine( str, "r_realtimeLightingRenderer", r_realtimeLightingRenderer.Get() );
	}

	if ( r_precomputedLighting->integer )
		AddDefine( str, "r_precomputedLighting", 1 );

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
	else if ( r_showVertexColors.Get() )
	{
		AddDefine( str, "r_showVertexColors", 1 );
	}

	if( r_showCubeProbes.Get() )
	{
		AddDefine( str, "r_showCubeProbes", 1 );
	}

	if ( glConfig2.vboVertexSkinningAvailable )
	{
		AddDefine( str, "r_vertexSkinning", 1 );
		AddConst( str, "MAX_GLSL_BONES", glConfig2.maxVertexSkinningBones );
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

	if ( r_showLightTiles->integer )
	{
		AddDefine( str, "r_showLightTiles", 1 );
	}

	if ( glConfig2.normalMapping )
	{
		AddDefine( str, "r_normalMapping", 1 );
	}

	if ( r_liquidMapping->integer )
	{
		AddDefine( str, "r_liquidMapping", 1 );
	}

	if ( glConfig2.specularMapping )
	{
		AddDefine( str, "r_specularMapping", 1 );
	}

	if ( glConfig2.physicalMapping )
	{
		AddDefine( str, "r_physicalMapping", 1 );
	}

	if ( r_glowMapping->integer )
	{
		AddDefine( str, "r_glowMapping", 1 );
	}

	AddDefine( str, "r_zNear", r_znear->value );

	return str;
}

void GLShaderManager::InitDriverInfo()
{
	std::string driverInfo = std::string(glConfig.renderer_string) + glConfig.version_string;
	_driverVersionHash = Com_BlockChecksum(driverInfo.c_str(), static_cast<int>(driverInfo.size()));
	_shaderBinaryCacheInvalidated = false;
}

void GLShaderManager::GenerateBuiltinHeaders() {
	GLVersionDeclaration = GLHeader("GLVersionDeclaration", GenFragmentVertexVersionDeclaration(), this);
	GLComputeVersionDeclaration = GLHeader( "GLComputeVersionDeclaration", GenComputeVersionDeclaration(), this );
	GLCompatHeader = GLHeader("GLCompatHeader", GenCompatHeader(), this);
	GLVertexHeader = GLHeader("GLVertexHeader", GenVertexHeader(), this);
	GLFragmentHeader = GLHeader("GLFragmentHeader", GenFragmentHeader(), this);
	GLComputeHeader = GLHeader( "GLComputeHeader", GenComputeHeader(), this );
	GLWorldHeader = GLHeader( "GLWorldHeader", GenWorldHeader(), this );
	GLEngineConstants = GLHeader("GLEngineConstants", GenEngineConstants(), this);
}

void GLShaderManager::GenerateWorldHeaders() {
	GLWorldHeader = GLHeader( "GLWorldHeader", GenWorldHeader(), this );
}

std::string GLShaderManager::BuildDeformShaderText( const std::string& steps )
{
	std::string shaderText;

	shaderText = steps + "\n";

	// We added a lot of stuff but if we do something bad
	// in the GLSL shaders then we want the proper line
	// so we have to reset the line counting.
	shaderText += "#line 0\n";
	shaderText += GetShaderText("deformVertexes_vp.glsl");
	return shaderText;
}

int GLShaderManager::getDeformShaderIndex( deformStage_t *deforms, int numDeforms )
{
	std::string steps = BuildDeformSteps( deforms, numDeforms );
	int index = _deformShaderLookup[ steps ] - 1;

	if( index < 0 )
	{
		// compile new deform shader
		std::string shaderText = GLShaderManager::BuildDeformShaderText( steps );
		_deformShaders.push_back(CompileShader( "deformVertexes",
							shaderText,
							{ &GLVersionDeclaration,
							  &GLVertexHeader },
							GL_VERTEX_SHADER ) );
		index = _deformShaders.size();
		_deformShaderLookup[ steps ] = index--;
	}

	return index;
}

std::string     GLShaderManager::BuildGPUShaderText( Str::StringRef mainShaderName,
	GLenum shaderType ) const {
	char        filename[MAX_QPATH];

	GL_CheckErrors();

	// load main() program
	switch ( shaderType ) {
		case GL_VERTEX_SHADER:
			Com_sprintf( filename, sizeof( filename ), "%s_vp.glsl", mainShaderName.c_str() );
			break;
		case GL_FRAGMENT_SHADER:
			Com_sprintf( filename, sizeof( filename ), "%s_fp.glsl", mainShaderName.c_str() );
			break;
		case GL_COMPUTE_SHADER:
			Com_sprintf( filename, sizeof( filename ), "%s_cp.glsl", mainShaderName.c_str() );
			break;
		default:
			break;
	}

	std::string out;
	out.reserve( 1024 ); // Might help, just an estimate.

	AddDefine( out, "r_AmbientScale", r_ambientScale->value );
	AddDefine( out, "r_SpecularScale", r_specularScale->value );
	AddDefine( out, "r_zNear", r_znear->value );

	AddDefine( out, "M_PI", static_cast< float >( M_PI ) );
	AddDefine( out, "MAX_SHADOWMAPS", MAX_SHADOWMAPS );
	AddDefine( out, "MAX_REF_LIGHTS", MAX_REF_LIGHTS );
	AddDefine( out, "TILE_SIZE", TILE_SIZE );

	AddDefine( out, "r_FBufSize", glConfig.vidWidth, glConfig.vidHeight );

	AddDefine( out, "r_tileStep", glState.tileStep[0], glState.tileStep[1] );

	std::string mainShaderText = GetShaderText( filename );
	std::istringstream shaderTextStream( mainShaderText );

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

static bool IsUnusedPermutation( const char *compileMacros )
{
	const char* token;
	while ( *( token = COM_ParseExt2( &compileMacros, false ) ) )
	{
		if ( strcmp( token, "USE_DELUXE_MAPPING" ) == 0 )
		{
			if ( !glConfig2.deluxeMapping ) return true;
		}
		else if ( strcmp( token, "USE_GRID_DELUXE_MAPPING" ) == 0 )
		{
			if ( !glConfig2.deluxeMapping ) return true;
		}
		else if ( strcmp( token, "USE_PHYSICAL_MAPPING" ) == 0 )
		{
			if ( !glConfig2.physicalMapping ) return true;
		}
		else if ( strcmp( token, "USE_REFLECTIVE_SPECULAR" ) == 0 )
		{
			/* FIXME: add to the following test: && r_physicalMapping->integer == 0
			when reflective specular is implemented for physical mapping too
			see https://github.com/DaemonEngine/Daemon/issues/355 */
			if ( !glConfig2.specularMapping ) return true;
		}
		else if ( strcmp( token, "USE_RELIEF_MAPPING" ) == 0 )
		{
			if ( !glConfig2.reliefMapping ) return true;
		}
		else if ( strcmp( token, "USE_HEIGHTMAP_IN_NORMALMAP" ) == 0 )
		{
			if ( !glConfig2.reliefMapping && !glConfig2.normalMapping ) return true;
		}
	}

	return false;
}

void GLShaderManager::buildPermutation( GLShader *shader, int macroIndex, int deformIndex )
{
	std::string compileMacros;
	int  startTime = ri.Milliseconds();
	int  endTime;
	size_t i = macroIndex + ( deformIndex << shader->_compileMacros.size() );

	// program already exists
	if ( i < shader->_shaderPrograms.size() &&
	     shader->_shaderPrograms[ i ].program )
	{
		return;
	}

	if( shader->GetCompileMacrosString( macroIndex, compileMacros ) )
	{
		shader->BuildShaderCompileMacros( compileMacros );

		if ( IsUnusedPermutation( compileMacros.c_str() ) )
			return;

		if( i >= shader->_shaderPrograms.size() )
			shader->_shaderPrograms.resize( (deformIndex + 1) << shader->_compileMacros.size() );

		shaderProgram_t *shaderProgram = &shader->_shaderPrograms[ i ];
		shaderProgram->attribs = shader->_vertexAttribsRequired; // | _vertexAttribsOptional;

		if( deformIndex > 0 )
		{
			shaderProgram_t *baseShader = &shader->_shaderPrograms[ macroIndex ];
			if( ( !baseShader->VS && shader->_hasVertexShader ) || ( !baseShader->FS && shader->_hasFragmentShader ) )
				CompileGPUShaders( shader, baseShader, compileMacros );

			shaderProgram->program = glCreateProgram();
			if ( shader->_hasVertexShader ) {
				glAttachShader( shaderProgram->program, baseShader->VS );
				glAttachShader( shaderProgram->program, _deformShaders[deformIndex] );
			}
			if ( shader->_hasFragmentShader ) {
				glAttachShader( shaderProgram->program, baseShader->FS );
			}

			BindAttribLocations( shaderProgram->program );
			LinkProgram( shaderProgram->program );
		}
		else if ( !LoadShaderBinary( shader, i ) )
		{
			CompileAndLinkGPUShaderProgram(	shader, shaderProgram, compileMacros, deformIndex );
			SaveShaderBinary( shader, i );
		}

		UpdateShaderProgramUniformLocations( shader, shaderProgram );
		GL_BindProgram( shaderProgram );
		shader->SetShaderProgramUniforms( shaderProgram );
		GL_BindProgram( nullptr );

		GL_CheckErrors();

		endTime = ri.Milliseconds();
		_totalBuildTime += ( endTime - startTime );
	}
}

void GLShaderManager::buildAll()
{
	while ( !_shaderBuildQueue.empty() )
	{
		GLShader& shader = *_shaderBuildQueue.front();

		std::string shaderName = shader.GetMainShaderName();

		size_t numPermutations = static_cast<size_t>(1) << shader.GetNumOfCompiledMacros();
		size_t i;

		for( i = 0; i < numPermutations; i++ )
		{
			buildPermutation( &shader, i, 0 );
		}

		_shaderBuildQueue.pop();
	}

	Log::Notice( "glsl shaders took %d msec to build", _totalBuildTime );
}

void GLShaderManager::InitShader( GLShader* shader ) {
	shader->_shaderPrograms = std::vector<shaderProgram_t>( static_cast< size_t >( 1 ) << shader->_compileMacros.size() );

	shader->PostProcessUniforms();

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

	if ( shader->_hasVertexShader ) {
		shader->_vertexShaderText = BuildGPUShaderText( shader->GetMainShaderName(), GL_VERTEX_SHADER );
	}
	if ( shader->_hasFragmentShader ) {
		shader->_fragmentShaderText = BuildGPUShaderText( shader->GetMainShaderName(), GL_FRAGMENT_SHADER );
	}
	if ( shader->_hasComputeShader ) {
		shader->_computeShaderText = BuildGPUShaderText( shader->GetMainShaderName(), GL_COMPUTE_SHADER );
	}

	if ( glConfig2.materialSystemAvailable && shader->_useMaterialSystem ) {
		shader->_vertexShaderText = ShaderPostProcess( shader, shader->_vertexShaderText );
		shader->_fragmentShaderText = ShaderPostProcess( shader, shader->_fragmentShaderText );
	}

	std::string combinedShaderText;
	if ( shader->_hasVertexShader || shader->_hasFragmentShader ) {
		combinedShaderText =
			GLVersionDeclaration.getText()
			+ GLCompatHeader.getText()
			+ GLEngineConstants.getText()
			+ GLVertexHeader.getText()
			+ GLFragmentHeader.getText();
	} else if ( shader->_hasComputeShader ) {
		combinedShaderText =
			GLComputeVersionDeclaration.getText()
			+ GLCompatHeader.getText()
			+ GLComputeHeader.getText()
			+ GLWorldHeader.getText()
			+ GLEngineConstants.getText();
	}

	if ( shader->_hasVertexShader ) {
		combinedShaderText += shader->_vertexShaderText;
	}
	if ( shader->_hasFragmentShader ) {
		combinedShaderText += shader->_fragmentShaderText;
	}
	if ( shader->_hasComputeShader ) {
		combinedShaderText += shader->_computeShaderText;
	}

	shader->_checkSum = Com_BlockChecksum( combinedShaderText.c_str(), combinedShaderText.length() );
}

bool GLShaderManager::LoadShaderBinary( GLShader *shader, size_t programNum )
{
#ifdef GL_ARB_get_program_binary
	GLint          success;
	const byte    *binaryptr;
	GLBinaryHeader shaderHeader;

	if ( !r_glslCache.Get() )
		return false;

	if (!GetShaderPath().empty())
		return false;

	// don't even try if the necessary functions aren't available
	if( !glConfig2.getProgramBinaryAvailable )
		return false;

	if (_shaderBinaryCacheInvalidated)
		return false;

	std::error_code err;

	std::string shaderFilename = Str::Format("glsl/%s/%s_%u.bin", shader->GetName(), shader->GetName(), (unsigned int)programNum);
	FS::File shaderFile = FS::HomePath::OpenRead(shaderFilename, err);
	if (err)
		return false;

	std::string shaderData = shaderFile.ReadAll(err);
	if (err)
		return false;

	if (shaderData.size() < sizeof(shaderHeader))
		return false;

	binaryptr = reinterpret_cast<const byte*>(shaderData.data());

	// get the shader header from the file
	memcpy( &shaderHeader, binaryptr, sizeof( shaderHeader ) );
	binaryptr += sizeof( shaderHeader );

	// check if the header struct is the correct format
	// and the binary was produced by the same gl driver
	if (shaderHeader.version != GL_SHADER_VERSION || shaderHeader.driverVersionHash != _driverVersionHash)
	{
		// These two fields should be the same for all shaders. So if there is a mismatch,
		// don't bother opening any of the remaining files.
		Log::Notice("Invalidating shader binary cache");
		_shaderBinaryCacheInvalidated = true;
		return false;
	}

	// make sure this shader uses the same number of macros
	if ( shaderHeader.numMacros != shader->GetNumOfCompiledMacros() )
		return false;

	// make sure this shader uses the same macros
	for ( unsigned int i = 0; i < shaderHeader.numMacros; i++ )
	{
		if ( shader->_compileMacros[ i ]->GetType() != shaderHeader.macros[ i ] )
			return false;
	}

	// make sure the checksums for the source code match
	if ( shaderHeader.checkSum != shader->_checkSum )
		return false;

	if ( shaderHeader.binaryLength != shaderData.size() - sizeof( shaderHeader ) )
	{
		Log::Warn( "Shader cache %s has wrong size", shaderFilename );
		return false;
	}

	// load the shader
	shaderProgram_t *shaderProgram = &shader->_shaderPrograms[ programNum ];
	shaderProgram->program = glCreateProgram();
	glProgramBinary( shaderProgram->program, shaderHeader.binaryFormat, binaryptr, shaderHeader.binaryLength );
	glGetProgramiv( shaderProgram->program, GL_LINK_STATUS, &success );

	if ( !success )
		return false;

	return true;
#else
	return false;
#endif
}
void GLShaderManager::SaveShaderBinary( GLShader *shader, size_t programNum )
{
#ifdef GL_ARB_get_program_binary
	GLint                 binaryLength;
	GLuint                binarySize = 0;
	byte                  *binary;
	byte                  *binaryptr;
	GLBinaryHeader        shaderHeader{}; // Zero init.
	shaderProgram_t       *shaderProgram;

	if ( !r_glslCache.Get() )
		return;

	if (!GetShaderPath().empty())
		return;

	// don't even try if the necessary functions aren't available
	if( !glConfig2.getProgramBinaryAvailable )
	{
		return;
	}

	shaderProgram = &shader->_shaderPrograms[ programNum ];

	// find output size
	binarySize += sizeof( shaderHeader );
	glGetProgramiv( shaderProgram->program, GL_PROGRAM_BINARY_LENGTH, &binaryLength );

	// The binary length may be 0 if there is an error.
	if ( binaryLength <= 0 )
	{
		return;
	}

	binarySize += binaryLength;

	binaryptr = binary = ( byte* )ri.Hunk_AllocateTempMemory( binarySize );

	// reserve space for the header
	binaryptr += sizeof( shaderHeader );

	// get the program binary and write it to the buffer
	glGetProgramBinary( shaderProgram->program, binaryLength, nullptr, &shaderHeader.binaryFormat, binaryptr );

	// set the header
	shaderHeader.version = GL_SHADER_VERSION;
	shaderHeader.numMacros = shader->_compileMacros.size();

	for ( unsigned int i = 0; i < shaderHeader.numMacros; i++ )
	{
		shaderHeader.macros[ i ] = shader->_compileMacros[ i ]->GetType();
	}

	shaderHeader.binaryLength = binaryLength;
	shaderHeader.checkSum = shader->_checkSum;
	shaderHeader.driverVersionHash = _driverVersionHash;

	// write the header to the buffer
	memcpy(binary, &shaderHeader, sizeof( shaderHeader ) );

	auto fileName = Str::Format("glsl/%s/%s_%u.bin", shader->GetName(), shader->GetName(), (unsigned int)programNum);
	ri.FS_WriteFile(fileName.c_str(), binary, binarySize);

	ri.Hunk_FreeTempMemory( binary );
#endif
}

void GLShaderManager::CompileGPUShaders( GLShader *shader, shaderProgram_t *program,
					 const std::string &compileMacros )
{
	// permutation macros
	std::string macrosString;

	const char* compileMacrosP = compileMacros.c_str();
	while ( true )
	{
		const char *token = COM_ParseExt2( &compileMacrosP, false );

		if ( !token[ 0 ] )
		{
			break;
		}

		macrosString += Str::Format( "#ifndef %s\n#define %s 1\n#endif\n", token, token );
	}

	Log::Debug( "building %s shader permutation with macro: %s",
		shader->GetMainShaderName(),
		compileMacros.empty() ? "none" : compileMacros );

	// add them
	std::string vertexShaderTextWithMacros = macrosString + shader->_vertexShaderText;
	std::string fragmentShaderTextWithMacros = macrosString + shader->_fragmentShaderText;
	std::string computeShaderTextWithMacros = macrosString + shader->_computeShaderText;
	if( shader->_hasVertexShader ) {
		program->VS = CompileShader( shader->GetName(),
						 vertexShaderTextWithMacros,
						 { &GLVersionDeclaration,
						   &GLVertexHeader,
						   &GLCompatHeader,
						   &GLEngineConstants },
						 GL_VERTEX_SHADER );
	}
	if ( shader->_hasFragmentShader ) {
		program->FS = CompileShader( shader->GetName(),
						 fragmentShaderTextWithMacros,
						 { &GLVersionDeclaration,
						   &GLFragmentHeader,
						   &GLCompatHeader,
						   &GLEngineConstants },
						 GL_FRAGMENT_SHADER );
	}
	if ( shader->_hasComputeShader ) {
		program->CS = CompileShader( shader->GetName(),
						 computeShaderTextWithMacros,
						 { &GLComputeVersionDeclaration,
						   &GLComputeHeader,
						   &GLWorldHeader,
						   &GLCompatHeader,
						   &GLEngineConstants },
						 GL_COMPUTE_SHADER );
	}
}

void GLShaderManager::CompileAndLinkGPUShaderProgram( GLShader *shader, shaderProgram_t *program,
						      Str::StringRef compileMacros, int deformIndex )
{
	GLShaderManager::CompileGPUShaders( shader, program, compileMacros );

	program->program = glCreateProgram();
	if ( shader->_hasVertexShader ) {
		glAttachShader( program->program, program->VS );
		glAttachShader( program->program, _deformShaders[ deformIndex ] );
	}
	if ( shader->_hasFragmentShader ) {
		glAttachShader( program->program, program->FS );
	}
	if ( shader->_hasComputeShader ) {
		glAttachShader( program->program, program->CS );
	}

	BindAttribLocations( program->program );
	LinkProgram( program->program );
}

// This will generate all the extra code for material system shaders
std::string GLShaderManager::ShaderPostProcess( GLShader *shader, const std::string& shaderText ) {
	std::string newShaderText;
	std::string materialStruct = "\nstruct Material {\n";
	std::string materialBlock = "layout(std430, binding = 0) readonly buffer materialsSSBO {\n"
									 "  Material materials[];\n"
									 "};\n\n";
	std::string materialDefines;

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

	for( GLUniform* uniform : shader->_uniforms ) {
		if ( uniform->IsGlobal() ) {
			continue;
		}

		if ( uniform->IsTexture() ) {
			materialStruct += "  uvec2 ";
			materialStruct += uniform->GetName();
		} else {
			materialStruct += "  " + uniform->GetType() + " " + uniform->GetName();
		}

		if ( uniform->GetComponentSize() ) {
			materialStruct += "[ " + std::to_string( uniform->GetComponentSize() ) + " ]";
		}
		materialStruct += ";\n";

		// vec3 is aligned to 4 components, so just pad it with int
		// TODO: Try to move 1 component uniforms here to avoid wasting memory
		if ( uniform->GetSTD430Size() == 3 ) {
			materialStruct += "  int ";
			materialStruct += uniform->GetName();
			materialStruct += "_padding;\n";
		}

		materialDefines += "#define ";
		materialDefines += uniform->GetName();

		if ( uniform->IsTexture() ) { // Driver bug: AMD compiler crashes when using the SSBO value directly
			materialDefines += "_initial uvec2("; // We'll need this to create sampler objects later
		}

		materialDefines += " materials[baseInstance].";
		materialDefines += uniform->GetName();
		
		if ( uniform->IsTexture() ) {
			materialDefines += " )";
		}

		materialDefines += "\n";
	}

	// Array of structs is aligned to the largest member of the struct
	for ( uint i = 0; i < shader->padding; i++ ) {
		materialStruct += "  int material_padding" + std::to_string( i );
		materialStruct += ";\n";
	}

	materialStruct += "};\n\n";
	materialDefines += "\n";

	std::istringstream shaderTextStream( shaderText );
	std::string shaderMain;

	std::string line;
	
	/* Remove local uniform declarations, but avoid removing uniform / storage blocks;
	*  their values will be sourced from a buffer instead
	*  Global uniforms (like u_ViewUp and u_ViewOrigin) will still be set as regular uniforms */
	while( std::getline( shaderTextStream, line, '\n' ) ) {
		if( !( line.find( "uniform" ) == std::string::npos || line.find( ";" ) == std::string::npos ) ) {
			continue;
		}
		shaderMain += line + "\n";
	}

	for ( GLUniform* uniform : shader->_uniforms ) {
		if ( uniform->IsGlobal() ) {
			materialDefines += "uniform " + uniform->GetType() + " " + uniform->GetName();
			if ( uniform->GetComponentSize() ) {
				materialDefines += "[ " + std::to_string( uniform->GetComponentSize() ) + " ]";
			}
			materialDefines += ";\n";
		}
	}

	materialDefines += "\n";

	newShaderText = "#define USE_MATERIAL_SYSTEM\n" + materialStruct + materialBlock + materialDefines + shaderMain;
	return newShaderText;
}

GLuint GLShaderManager::CompileShader( Str::StringRef programName,
				       Str::StringRef shaderText,
				       std::initializer_list<const GLHeader *> headers,
				       GLenum shaderType ) const
{
	GLuint shader = glCreateShader( shaderType );
	std::vector<const GLchar*> texts(headers.size() + 1);
	std::vector<GLint> lengths(headers.size() + 1);
	int i;

	i = 0;
	for(const GLHeader *hdr : headers) {
	  texts[i++] = hdr->getText().data();
	}
	texts[i++] = shaderText.data();

	i = 0;
	for(const GLHeader *hdr : headers) {
	  lengths[i++] = (GLint)hdr->getText().size();
	}
	lengths[i++] = (GLint)shaderText.size();

	GL_CheckErrors();

	glShaderSource( shader, i, texts.data(), lengths.data() );

	// compile shader
	glCompileShader( shader );

	GL_CheckErrors();

	// check if shader compiled
	GLint compiled;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );

	if ( !compiled )
	{
		PrintShaderSource( programName, shader );
		PrintInfoLog( shader );
		switch ( shaderType ) {
			case GL_VERTEX_SHADER:
				ThrowShaderError( Str::Format( "Couldn't compile vertex shader: %s", programName ) );
			case GL_FRAGMENT_SHADER:
				ThrowShaderError( Str::Format( "Couldn't compile fragment shader: %s", programName ) );
			case GL_COMPUTE_SHADER:
				ThrowShaderError( Str::Format( "Couldn't compile compute shader: %s", programName ) );
			default:
				break;
		}
	}

	return shader;
}

void GLShaderManager::PrintShaderSource( Str::StringRef programName, GLuint object ) const
{
	char        *dump;
	int         maxLength = 0;

	glGetShaderiv( object, GL_SHADER_SOURCE_LENGTH, &maxLength );

	dump = ( char * ) ri.Hunk_AllocateTempMemory( maxLength );

	glGetShaderSource( object, maxLength, &maxLength, dump );

	std::string buffer;
	std::string delim("\n");
	std::string src(dump);

	ri.Hunk_FreeTempMemory( dump );

	int lineNumber = 0;
	size_t pos = 0;

	while ( ( pos = src.find( delim ) ) != std::string::npos ) {
		std::string line = src.substr( 0, pos );
		if ( Str::IsPrefix( "#line ", line ) )
		{
			size_t lineNumEnd = line.find( ' ', 6 );
			Str::ParseInt( lineNumber, line.substr( 6, lineNumEnd - 6 ) );
		}

		std::string number = std::to_string( lineNumber );

		int p = 4 - number.length();
		p = p < 0 ? 0 : p;
		number.insert( number.begin(), p, ' ' );

		buffer.append( number );
		buffer.append( ": " );
		buffer.append( line );
		buffer.append( delim );

		src.erase( 0, pos + delim.length() );

		lineNumber++;
	}

	Log::Warn("Source for shader program %s:\n%s", programName, buffer.c_str());
}

void GLShaderManager::PrintInfoLog( GLuint object) const
{
	char        *msg;
	int         maxLength = 0;
	std::string msgText;

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
		return;
	}

	msg = ( char * ) ri.Hunk_AllocateTempMemory( maxLength );

	if ( glIsShader( object ) )
	{
		glGetShaderInfoLog( object, maxLength, &maxLength, msg );
		msgText = "Compile log:";
	}
	else if ( glIsProgram( object ) )
	{
		glGetProgramInfoLog( object, maxLength, &maxLength, msg );
		msgText = "Link log:";
	}
	if (maxLength > 0)
		msgText += '\n';
	msgText += msg;
	Log::Warn(msgText);

	ri.Hunk_FreeTempMemory( msg );
}

void GLShaderManager::LinkProgram( GLuint program ) const
{
	GLint linked;

#ifdef GL_ARB_get_program_binary
	// Apparently, this is necessary to get the binary program via glGetProgramBinary
	if( glConfig2.getProgramBinaryAvailable )
	{
		glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE );
	}
#endif
	glLinkProgram( program );

	glGetProgramiv( program, GL_LINK_STATUS, &linked );

	if ( !linked )
	{
		PrintInfoLog( program );
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
		if ( ( permutation & macro->GetBit() ) != 0 && (macro->GetType() == USE_PHYSICAL_MAPPING || macro->GetType() == USE_VERTEX_SPRITE) )
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
		if ( ( permutation & macro->GetBit() ) != 0 && (macro->GetType() == USE_VERTEX_ANIMATION || macro->GetType() == USE_VERTEX_SPRITE) )
		{
			//Log::Notice("conflicting macro! canceling '%s' vs. '%s'", GetName(), macro->GetName());
			return true;
		}
	}

	return false;
}

bool GLCompileMacro_USE_VERTEX_SKINNING::MissesRequiredMacros( size_t /*permutation*/, const std::vector< GLCompileMacro * > &/*macros*/ ) const
{
	return !glConfig2.vboVertexSkinningAvailable;
}

bool GLCompileMacro_USE_VERTEX_ANIMATION::HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ( ( permutation & macro->GetBit() ) != 0 && (macro->GetType() == USE_VERTEX_SKINNING || macro->GetType() == USE_VERTEX_SPRITE) )
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

bool GLCompileMacro_USE_VERTEX_SPRITE::HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ( ( permutation & macro->GetBit() ) != 0 && (macro->GetType() == USE_VERTEX_SKINNING || macro->GetType() == USE_VERTEX_ANIMATION || macro->GetType() == USE_DEPTH_FADE))
		{
			//Log::Notice("conflicting macro! canceling '%s' vs. '%s'", GetName(), macro->GetName());
			return true;
		}
	}

	return false;
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

bool GLCompileMacro_USE_DEPTH_FADE::HasConflictingMacros(size_t permutation, const std::vector<GLCompileMacro*> &macros) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ((permutation & macro->GetBit()) != 0 && (macro->GetType() == USE_VERTEX_SPRITE))
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

GLuint GLUniform::GetSTD430Size() const {
	return _std430Size;
}

GLuint GLUniform::GetSTD430Alignment() const {
	return _std430Alignment;
}

uint32_t* GLUniform::WriteToBuffer( uint32_t* buffer ) {
	return buffer;
}

void GLShader::RegisterUniform( GLUniform* uniform ) {
	_uniforms.push_back( uniform );
	textureCount += uniform->IsTexture();
}

GLint GLShader::GetUniformLocation( const GLchar *uniformName ) const {
	shaderProgram_t* p = GetProgram();
	return glGetUniformLocation( p->program, uniformName );
}

// Compute std430 size/alignment and sort uniforms from highest to lowest alignment
void GLShader::PostProcessUniforms() {
	if ( !_useMaterialSystem ) {
		return;
	}

	std::vector<GLUniform*> tmp;

	std::vector<GLUniform*> globalUniforms;
	for ( GLUniform* uniform : _uniforms ) {
		if ( uniform->IsGlobal() ) {
			globalUniforms.emplace_back( uniform );
		}
	}
	for ( GLUniform* uniform : globalUniforms ) {
		_uniforms.erase( std::remove( _uniforms.begin(), _uniforms.end(), uniform ), _uniforms.end() );
	}

	// Sort uniforms from highest to lowest alignment so we don't need to pad uniforms (other than vec3s)
	const uint numUniforms = _uniforms.size();
	GLuint structAlignment = 0;
	GLuint structSize = 0;
	while ( tmp.size() < numUniforms ) {
		// Higher-alignment uniforms first to avoid wasting memory
		GLuint highestAlignment = 0;
		int highestUniform = 0;
		for( uint i = 0; i < _uniforms.size(); i++ ) {
			if ( _uniforms[i]->GetSTD430Alignment() > highestAlignment ) {
				highestAlignment = _uniforms[i]->GetSTD430Alignment();
				highestUniform = i;
			}
			if ( highestAlignment > structAlignment ) {
				structAlignment = highestAlignment;
			}
			if ( highestAlignment == 4 ) {
				break; // 4-component is the highest alignment in std430
			}
		}
			
		const GLuint size = _uniforms[highestUniform]->GetSTD430Size();
		if ( _uniforms[highestUniform]->GetComponentSize() != 0 ) {
			structSize += ( size == 3 ) ? 4 * _uniforms[highestUniform]->GetComponentSize() :
												size * _uniforms[highestUniform]->GetComponentSize();
		} else {
			structSize += ( size == 3 ) ? 4 : size;
		}

		tmp.emplace_back( _uniforms[highestUniform] );
		_uniforms.erase( _uniforms.begin() + highestUniform );
	}
	_uniforms = tmp;

	padding = ( structAlignment - ( structSize % structAlignment ) ) % structAlignment;
	std430Size = structSize;
	for ( GLUniform* uniform : globalUniforms ) {
		_uniforms.emplace_back( uniform );
	}
}

bool GLShader::GetCompileMacrosString( size_t permutation, std::string &compileMacrosOut ) const
{
	compileMacrosOut.clear();

	for (const GLCompileMacro* macro : _compileMacros)
	{
		if ( permutation & macro->GetBit() )
		{
			if ( macro->HasConflictingMacros( permutation, _compileMacros ) )
			{
				//Log::Notice("conflicting macro! canceling '%s'", macro->GetName());
				return false;
			}

			if ( macro->MissesRequiredMacros( permutation, _compileMacros ) )
				return false;

			compileMacrosOut += macro->GetName();
			compileMacrosOut += " ";
		}
	}

	return true;
}

int GLShader::SelectProgram()
{
	int    index = 0;

	size_t numMacros = _compileMacros.size();

	for ( size_t i = 0; i < numMacros; i++ )
	{
		if ( _activeMacros & BIT( i ) )
		{
			index += BIT( i );
		}
	}

	return index;
}

GLuint GLShader::GetProgram( int deformIndex ) {
	int macroIndex = SelectProgram();
	size_t index = macroIndex + ( size_t( deformIndex ) << _compileMacros.size() );

	// program may not be loaded yet because the shader manager hasn't yet gotten to it
	// so try to load it now
	if ( index >= _shaderPrograms.size() || !_shaderPrograms[index].program ) {
		_shaderManager->buildPermutation( this, macroIndex, deformIndex );
	}

	// program is still not loaded
	if ( index >= _shaderPrograms.size() || !_shaderPrograms[index].program ) {
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

	return _shaderPrograms[index].program;
}

void GLShader::BindProgram( int deformIndex )
{
	int macroIndex = SelectProgram();
	size_t index = macroIndex + ( size_t(deformIndex) << _compileMacros.size() );

	// program may not be loaded yet because the shader manager hasn't yet gotten to it
	// so try to load it now
	if ( index >= _shaderPrograms.size() || !_shaderPrograms[ index ].program )
	{
		_shaderManager->buildPermutation( this, macroIndex, deformIndex );
	}

	// program is still not loaded
	if ( index >= _shaderPrograms.size() || !_shaderPrograms[ index ].program )
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

	_currentProgram = &_shaderPrograms[ index ];

	if ( r_logFile->integer )
	{
		std::string macros;

		this->GetCompileMacrosString( index, macros );

		auto msg = Str::Format("--- GL_BindProgram( name = '%s', macros = '%s' ) ---\n", this->GetName(), macros);
		GLimp_LogComment(msg.c_str());
	}

	GL_BindProgram( _currentProgram );
}

void GLShader::DispatchCompute( const GLuint globalWorkgroupX, const GLuint globalWorkgroupY, const GLuint globalWorkgroupZ ) {
	ASSERT_EQ( _currentProgram, glState.currentProgram );
	ASSERT( _hasComputeShader );
	glDispatchCompute( globalWorkgroupX, globalWorkgroupY, globalWorkgroupZ );
}

void GLShader::DispatchComputeIndirect( const GLintptr indirectBuffer ) {
	ASSERT_EQ( _currentProgram, glState.currentProgram );
	ASSERT( _hasComputeShader );
	glDispatchComputeIndirect( indirectBuffer );
}

void GLShader::SetRequiredVertexPointers( bool vertexSprite )
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

	if ( vertexSprite )
	{
		attribs &= ~ATTR_QTANGENT;
		attribs |= ATTR_ORIENTATION;
	}

	GL_VertexAttribsState( attribs );
}

void GLShader::WriteUniformsToBuffer( uint32_t* buffer ) {
	uint32_t* bufPtr = buffer;
	for ( GLUniform* uniform : _uniforms ) {
		if ( !uniform->IsGlobal() ) {
			bufPtr = uniform->WriteToBuffer( bufPtr );
		}
	}
}

GLShader_generic2D::GLShader_generic2D( GLShaderManager *manager ) :
	GLShader( "generic2D", "generic", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_ColorMap( this ),
	u_DepthMap( this ),
	u_TextureMatrix( this ),
	u_AlphaThreshold( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_DEPTH_FADE( this )
{
}

void GLShader_generic2D::BuildShaderCompileMacros( std::string& compileMacros )
{
	compileMacros += "GENERIC_2D ";
}

void GLShader_generic2D::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 1 );
}

GLShader_generic::GLShader_generic( GLShaderManager *manager ) :
	GLShader( "generic", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_ColorMap( this ),
	u_DepthMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_ViewUp( this ),
	u_AlphaThreshold( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_InverseLightFactor( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_VERTEX_SPRITE( this ),
	GLCompileMacro_USE_TCGEN_ENVIRONMENT( this ),
	GLCompileMacro_USE_TCGEN_LIGHTMAP( this ),
	GLCompileMacro_USE_DEPTH_FADE( this )
{
}

void GLShader_generic::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 1 );
}

GLShader_genericMaterial::GLShader_genericMaterial( GLShaderManager* manager ) :
	GLShader( "genericMaterial", "generic", true, ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_ColorMap( this ),
	u_DepthMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_ViewUp( this ),
	u_AlphaThreshold( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_InverseLightFactor( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	// u_Bones( this ),
	u_VertexInterpolation( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	// GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_VERTEX_SPRITE( this ),
	GLCompileMacro_USE_TCGEN_ENVIRONMENT( this ),
	GLCompileMacro_USE_TCGEN_LIGHTMAP( this ),
	GLCompileMacro_USE_DEPTH_FADE( this ) {
}

void GLShader_genericMaterial::SetShaderProgramUniforms( shaderProgram_t* shaderProgram ) {
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 1 );
}

GLShader_lightMapping::GLShader_lightMapping( GLShaderManager *manager ) :
	GLShader( "lightMapping",
	ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT | ATTR_COLOR, manager ),
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
	u_LightTilesInt( this ),
	u_LightsTexture( this ),
	u_TextureMatrix( this ),
	u_SpecularExponent( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_AlphaThreshold( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_InverseLightFactor( this ),
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

void GLShader_lightMapping::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" ), BIND_DIFFUSEMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), BIND_NORMALMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_HeightMap" ), BIND_HEIGHTMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_MaterialMap" ), BIND_MATERIALMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightMap" ), BIND_LIGHTMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid1" ), BIND_LIGHTMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DeluxeMap" ), BIND_DELUXEMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid2" ), BIND_DELUXEMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_GlowMap" ), BIND_GLOWMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_EnvironmentMap0" ), BIND_ENVIRONMENTMAP0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_EnvironmentMap1" ), BIND_ENVIRONMENTMAP1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightTiles" ), BIND_LIGHTTILES );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightTilesInt" ), BIND_LIGHTTILES );
	if( !glConfig2.uniformBufferObjectAvailable ) {
		glUniform1i( glGetUniformLocation( shaderProgram->program, "u_Lights" ), BIND_LIGHTS );
	}
}

GLShader_lightMappingMaterial::GLShader_lightMappingMaterial( GLShaderManager* manager ) :
	GLShader( "lightMappingMaterial", "lightMapping", true,
		ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT | ATTR_COLOR, manager ),
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
	u_LightTilesInt( this ),
	u_TextureMatrix( this ),
	u_SpecularExponent( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_AlphaThreshold( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_InverseLightFactor( this ),
	// u_Bones( this ),
	u_VertexInterpolation( this ),
	u_ReliefDepthScale( this ),
	u_ReliefOffsetBias( this ),
	u_NormalScale( this ),
	u_EnvironmentInterpolation( this ),
	u_LightGridOrigin( this ),
	u_LightGridScale( this ),
	u_numLights( this ),
	u_Lights( this ),
	u_ShowTris( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_BSP_SURFACE( this ),
	// GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DELUXE_MAPPING( this ),
	GLCompileMacro_USE_GRID_LIGHTING( this ),
	GLCompileMacro_USE_GRID_DELUXE_MAPPING( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this ),
	GLCompileMacro_USE_REFLECTIVE_SPECULAR( this ),
	GLCompileMacro_USE_PHYSICAL_MAPPING( this ) {
}

void GLShader_lightMappingMaterial::SetShaderProgramUniforms( shaderProgram_t* shaderProgram ) {
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" ), BIND_DIFFUSEMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), BIND_NORMALMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_HeightMap" ), BIND_HEIGHTMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_MaterialMap" ), BIND_MATERIALMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightMap" ), BIND_LIGHTMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DeluxeMap" ), BIND_DELUXEMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_GlowMap" ), BIND_GLOWMAP );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_EnvironmentMap0" ), BIND_ENVIRONMENTMAP0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_EnvironmentMap1" ), BIND_ENVIRONMENTMAP1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightTiles" ), BIND_LIGHTTILES );
	if ( !glConfig2.uniformBufferObjectAvailable ) {
		glUniform1i( glGetUniformLocation( shaderProgram->program, "u_Lights" ), BIND_LIGHTS );
	}
}

GLShader_forwardLighting_omniXYZ::GLShader_forwardLighting_omniXYZ( GLShaderManager *manager ):
	GLShader("forwardLighting_omniXYZ", "forwardLighting", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager),
	u_DiffuseMap( this ),
	u_NormalMap( this ),
	u_MaterialMap( this ),
	u_AttenuationMapXY( this ),
	u_AttenuationMapZ( this ),
	u_ShadowMap( this ),
	u_ShadowClipMap( this ),
	u_RandomMap( this ),
	u_HeightMap( this ),
	u_TextureMatrix( this ),
	u_SpecularExponent( this ),
	u_AlphaThreshold( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_ViewOrigin( this ),
	u_LightOrigin( this ),
	u_LightColor( this ),
	u_InverseLightFactor( this ),
	u_LightRadius( this ),
	u_LightScale( this ),
	u_LightAttenuationMatrix( this ),
	u_ShadowTexelSize( this ),
	u_ShadowBlur( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_ReliefDepthScale( this ),
	u_ReliefOffsetBias( this ),
	u_NormalScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )
{
}

void GLShader_forwardLighting_omniXYZ::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_MaterialMap" ), 2 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_AttenuationMapXY" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_AttenuationMapZ" ), 4 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap" ), 5 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_RandomMap" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap" ), 7 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_HeightMap" ), 15 );
}

GLShader_forwardLighting_projXYZ::GLShader_forwardLighting_projXYZ( GLShaderManager *manager ):
	GLShader("forwardLighting_projXYZ", "forwardLighting", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager),
	u_DiffuseMap( this ),
	u_NormalMap( this ),
	u_MaterialMap( this ),
	u_AttenuationMapXY( this ),
	u_AttenuationMapZ( this ),
	u_ShadowMap0( this ),
	u_ShadowClipMap0( this ),
	u_RandomMap( this ),
	u_HeightMap( this ),
	u_TextureMatrix( this ),
	u_SpecularExponent( this ),
	u_AlphaThreshold( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_ViewOrigin( this ),
	u_LightOrigin( this ),
	u_LightColor( this ),
	u_InverseLightFactor( this ),
	u_LightRadius( this ),
	u_LightScale( this ),
	u_LightAttenuationMatrix( this ),
	u_ShadowTexelSize( this ),
	u_ShadowBlur( this ),
	u_ShadowMatrix( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_ReliefDepthScale( this ),
	u_ReliefOffsetBias( this ),
	u_NormalScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )
{
}

void GLShader_forwardLighting_projXYZ::BuildShaderCompileMacros( std::string& compileMacros )
{
	compileMacros += "LIGHT_PROJ ";
}

void GLShader_forwardLighting_projXYZ::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_MaterialMap" ), 2 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_AttenuationMapXY" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_AttenuationMapZ" ), 4 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap0" ), 5 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_RandomMap" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap0" ), 7 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_HeightMap" ), 15 );
}

GLShader_forwardLighting_directionalSun::GLShader_forwardLighting_directionalSun( GLShaderManager *manager ):
	GLShader("forwardLighting_directionalSun", "forwardLighting", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager),
	u_DiffuseMap( this ),
	u_NormalMap( this ),
	u_MaterialMap( this ),
	u_ShadowMap0( this ),
	u_ShadowMap1( this ),
	u_ShadowMap2( this ),
	u_ShadowMap3( this ),
	u_ShadowMap4( this ),
	u_ShadowClipMap0( this ),
	u_ShadowClipMap1( this ),
	u_ShadowClipMap2( this ),
	u_ShadowClipMap3( this ),
	u_ShadowClipMap4( this ),
	u_HeightMap( this ),
	u_TextureMatrix( this ),
	u_SpecularExponent( this ),
	u_AlphaThreshold( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_ViewOrigin( this ),
	u_LightDir( this ),
	u_LightColor( this ),
	u_InverseLightFactor( this ),
	u_LightRadius( this ),
	u_LightScale( this ),
	u_LightAttenuationMatrix( this ),
	u_ShadowTexelSize( this ),
	u_ShadowBlur( this ),
	u_ShadowMatrix( this ),
	u_ShadowParallelSplitDistances( this ),
	u_ModelMatrix( this ),
	u_ViewMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_ReliefDepthScale( this ),
	u_ReliefOffsetBias( this ),
	u_NormalScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )
{
}

void GLShader_forwardLighting_directionalSun::BuildShaderCompileMacros( std::string& compileMacros )
{
	compileMacros += "LIGHT_DIRECTIONAL ";
}

void GLShader_forwardLighting_directionalSun::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_MaterialMap" ), 2 );
	//glUniform1i(glGetUniformLocation( shaderProgram->program, "u_AttenuationMapXY" ), 3);
	//glUniform1i(glGetUniformLocation( shaderProgram->program, "u_AttenuationMapZ" ), 4);
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap0" ), 5 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap1" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap2" ), 7 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap3" ), 8 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap4" ), 9 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap0" ), 10 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap1" ), 11 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap2" ), 12 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap3" ), 13 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap4" ), 14 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_HeightMap" ), 15 );
}

GLShader_shadowFill::GLShader_shadowFill( GLShaderManager *manager ) :
	GLShader( "shadowFill", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_ColorMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_AlphaThreshold( this ),
	u_LightOrigin( this ),
	u_LightRadius( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Color( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_LIGHT_DIRECTIONAL( this )
{
}

void GLShader_shadowFill::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_reflection::GLShader_reflection( GLShaderManager *manager ):
	GLShader("reflection", "reflection_CB", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
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
	u_InverseLightFactor( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this )
{
}

void GLShader_reflection::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_HeightMap" ), 15 );
}

GLShader_reflectionMaterial::GLShader_reflectionMaterial( GLShaderManager* manager ) :
	GLShader( "reflectionMaterial", "reflection_CB", true, ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_ColorMapCube( this ),
	u_NormalMap( this ),
	u_HeightMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	// u_Bones( this ),
	u_ReliefDepthScale( this ),
	u_ReliefOffsetBias( this ),
	u_NormalScale( this ),
	u_VertexInterpolation( this ),
	u_CameraPosition( this ),
	u_InverseLightFactor( this ),
	GLDeformStage( this ),
	// GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this ) {
}

void GLShader_reflectionMaterial::SetShaderProgramUniforms( shaderProgram_t* shaderProgram ) {
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_HeightMap" ), 15 );
}

GLShader_skybox::GLShader_skybox( GLShaderManager *manager ) :
	GLShader( "skybox", ATTR_POSITION, manager ),
	u_ColorMapCube( this ),
	u_CloudMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_CloudHeight( this ),
	u_UseCloudMap( this ),
	u_AlphaThreshold( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_InverseLightFactor( this ),
	GLDeformStage( this )
{
}

void GLShader_skybox::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMapCube" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CloudMap" ), 1 );
}

GLShader_skyboxMaterial::GLShader_skyboxMaterial( GLShaderManager* manager ) :
	GLShader( "skyboxMaterial", "skybox", true, ATTR_POSITION, manager ),
	u_ColorMapCube( this ),
	u_CloudMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_CloudHeight( this ),
	u_UseCloudMap( this ),
	u_AlphaThreshold( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_InverseLightFactor( this ),
	GLDeformStage( this ) {
}

void GLShader_skyboxMaterial::SetShaderProgramUniforms( shaderProgram_t* shaderProgram ) {
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CloudMap" ), 1 );
}

GLShader_fogQuake3::GLShader_fogQuake3( GLShaderManager *manager ) :
	GLShader( "fogQuake3", ATTR_POSITION | ATTR_QTANGENT, manager ),
	u_ColorMap( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_InverseLightFactor( this ),
	u_Color( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_FogDistanceVector( this ),
	u_FogDepthVector( this ),
	u_FogEyeT( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this )
{
}

void GLShader_fogQuake3::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_fogQuake3Material::GLShader_fogQuake3Material( GLShaderManager* manager ) :
	GLShader( "fogQuake3Material", "fogQuake3", true, ATTR_POSITION | ATTR_QTANGENT, manager ),
	u_ColorMap( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_InverseLightFactor( this ),
	u_Color( this ),
	// u_Bones( this ),
	u_VertexInterpolation( this ),
	u_FogDistanceVector( this ),
	u_FogDepthVector( this ),
	u_FogEyeT( this ),
	GLDeformStage( this ),
	// GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ) {
}

void GLShader_fogQuake3Material::SetShaderProgramUniforms( shaderProgram_t* shaderProgram ) {
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_fogGlobal::GLShader_fogGlobal( GLShaderManager *manager ) :
	GLShader( "fogGlobal", ATTR_POSITION, manager ),
	u_ColorMap( this ),
	u_DepthMap( this ),
	u_ViewOrigin( this ),
	u_ViewMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_UnprojectMatrix( this ),
	u_InverseLightFactor( this ),
	u_Color( this ),
	u_FogDistanceVector( this ),
	u_FogDepthVector( this )
{
}

void GLShader_fogGlobal::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 1 );
}

GLShader_heatHaze::GLShader_heatHaze( GLShaderManager *manager ) :
	GLShader( "heatHaze", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_CurrentMap( this ),
	u_NormalMap( this ),
	u_HeightMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_ViewUp( this ),
	u_DeformMagnitude( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ModelViewMatrixTranspose( this ),
	u_ProjectionMatrixTranspose( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_Bones( this ),
	u_NormalScale( this ),
	u_VertexInterpolation( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_VERTEX_SPRITE( this )
{
}

void GLShader_heatHaze::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_HeightMap" ), 15 );
}

GLShader_heatHazeMaterial::GLShader_heatHazeMaterial( GLShaderManager* manager ) :
	GLShader( "heatHazeMaterial", "heatHaze", true, ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_CurrentMap( this ),
	u_NormalMap( this ),
	u_HeightMap( this ),
	u_TextureMatrix( this ),
	u_ViewOrigin( this ),
	u_ViewUp( this ),
	u_DeformMagnitude( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ModelViewMatrixTranspose( this ),
	u_ProjectionMatrixTranspose( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	// u_Bones( this ),
	u_NormalScale( this ),
	u_VertexInterpolation( this ),
	GLDeformStage( this ),
	// GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_VERTEX_SPRITE( this ) {
}

void GLShader_heatHazeMaterial::SetShaderProgramUniforms( shaderProgram_t* shaderProgram ) {
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_HeightMap" ), 15 );
}

GLShader_screen::GLShader_screen( GLShaderManager *manager ) :
	GLShader( "screen", ATTR_POSITION, manager ),
	u_CurrentMap( this ),
	u_ModelViewProjectionMatrix( this )
{
}

void GLShader_screen::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
}

GLShader_screenMaterial::GLShader_screenMaterial( GLShaderManager* manager ) :
	GLShader( "screenMaterial", "screen", true, ATTR_POSITION, manager ),
	u_CurrentMap( this ),
	u_ModelViewProjectionMatrix( this ) {
}

void GLShader_screenMaterial::SetShaderProgramUniforms( shaderProgram_t* shaderProgram ) {
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
}

GLShader_portal::GLShader_portal( GLShaderManager *manager ) :
	GLShader( "portal", ATTR_POSITION, manager ),
	u_CurrentMap( this ),
	u_ModelViewMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_InversePortalRange( this )
{
}

void GLShader_portal::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
}

GLShader_contrast::GLShader_contrast( GLShaderManager *manager ) :
	GLShader( "contrast", ATTR_POSITION, manager ),
	u_ColorMap( this ),
	u_ModelViewProjectionMatrix( this ),
	u_InverseLightFactor( this )
{
}

void GLShader_contrast::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_cameraEffects::GLShader_cameraEffects( GLShaderManager *manager ) :
	GLShader( "cameraEffects", ATTR_POSITION | ATTR_TEXCOORD, manager ),
	u_ColorMap3D( this ),
	u_CurrentMap( this ),
	u_ColorModulate( this ),
	u_TextureMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_LightFactor( this ),
	u_DeformMagnitude( this ),
	u_InverseGamma( this )
{
}

void GLShader_cameraEffects::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap3D" ), 3 );
}

GLShader_blurX::GLShader_blurX( GLShaderManager *manager ) :
	GLShader( "blurX", ATTR_POSITION, manager ),
	u_ColorMap( this ),
	u_ModelViewProjectionMatrix( this ),
	u_DeformMagnitude( this ),
	u_TexScale( this )
{
}

void GLShader_blurX::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_blurY::GLShader_blurY( GLShaderManager *manager ) :
	GLShader( "blurY", ATTR_POSITION, manager ),
	u_ColorMap( this ),
	u_ModelViewProjectionMatrix( this ),
	u_DeformMagnitude( this ),
	u_TexScale( this )
{
}

void GLShader_blurY::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_debugShadowMap::GLShader_debugShadowMap( GLShaderManager *manager ) :
	GLShader( "debugShadowMap", ATTR_POSITION, manager ),
	u_CurrentMap( this ),
	u_ModelViewProjectionMatrix( this )
{
}

void GLShader_debugShadowMap::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
}

GLShader_liquid::GLShader_liquid( GLShaderManager *manager ) :
	GLShader( "liquid", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
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
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this )
{
}

void GLShader_liquid::SetShaderProgramUniforms( shaderProgram_t* shaderProgram ) {
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_PortalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 2 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid1" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid2" ), 7 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_HeightMap" ), 15 );
}

GLShader_liquidMaterial::GLShader_liquidMaterial( GLShaderManager* manager ) :
	GLShader( "liquidMaterial", "liquid", true, ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
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
	u_LightTilesInt( this ),
	u_SpecularExponent( this ),
	u_LightGridOrigin( this ),
	u_LightGridScale( this ),
	GLCompileMacro_USE_HEIGHTMAP_IN_NORMALMAP( this ),
	GLCompileMacro_USE_RELIEF_MAPPING( this ) {
}

void GLShader_liquidMaterial::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_PortalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 2 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid1" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid2" ), 7 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_HeightMap" ), 15 );
}

GLShader_motionblur::GLShader_motionblur( GLShaderManager *manager ) :
	GLShader( "motionblur", ATTR_POSITION, manager ),
	u_ColorMap( this ),
	u_DepthMap( this ),
	u_ModelViewProjectionMatrix( this ),
	u_blurVec( this )
{
}

void GLShader_motionblur::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 1 );
}

GLShader_ssao::GLShader_ssao( GLShaderManager *manager ) :
	GLShader( "ssao", ATTR_POSITION, manager ),
	u_DepthMap( this ),
	u_ModelViewProjectionMatrix( this ),
	u_UnprojectionParams( this ),
	u_zFar( this )
{
}

void GLShader_ssao::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 0 );
}

GLShader_depthtile1::GLShader_depthtile1( GLShaderManager *manager ) :
	GLShader( "depthtile1", ATTR_POSITION, manager ),
	u_DepthMap( this ),
	u_ModelViewProjectionMatrix( this ),
	u_zFar( this )
{
}

void GLShader_depthtile1::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 0 );
}

GLShader_depthtile2::GLShader_depthtile2( GLShaderManager *manager ) :
	GLShader( "depthtile2", ATTR_POSITION, manager ),
	u_DepthMap( this ),
	u_ModelViewProjectionMatrix( this )
{
}

void GLShader_depthtile2::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 0 );
}

GLShader_lighttile::GLShader_lighttile( GLShaderManager *manager ) :
	GLShader( "lighttile", ATTR_POSITION | ATTR_TEXCOORD, manager ),
	u_DepthMap( this ),
	u_Lights( this ),
	u_LightsTexture( this ),
	u_numLights( this ),
	u_lightLayer( this ),
	u_ModelMatrix( this ),
	u_zFar( this )
{
}

void GLShader_lighttile::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 0 );

	if( !glConfig2.uniformBufferObjectAvailable ) {
		glUniform1i( glGetUniformLocation( shaderProgram->program, "u_Lights" ), 1 );
	}
}

GLShader_fxaa::GLShader_fxaa( GLShaderManager *manager ) :
	GLShader( "fxaa", ATTR_POSITION, manager ),
	u_ColorMap( this ),
	u_ModelViewProjectionMatrix( this )
{
}

void GLShader_fxaa::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_cull::GLShader_cull( GLShaderManager* manager ) :
	GLShader( "cull", ATTR_POSITION, manager, false, false, true ),
	u_Frame( this ),
	u_ViewID( this ),
	u_TotalDrawSurfs( this ),
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

GLShader_depthReduction::GLShader_depthReduction( GLShaderManager* manager ) :
	GLShader( "depthReduction", ATTR_POSITION, manager, false, false, true ),
	u_ViewWidth( this ),
	u_ViewHeight( this ),
	u_InitialDepthLevel( this ) {
}

void GLShader_depthReduction::SetShaderProgramUniforms( shaderProgram_t* shaderProgram ) {
	glUniform1i( glGetUniformLocation( shaderProgram->program, "depthTextureInitial" ), 0 );
}

GLShader_clearSurfaces::GLShader_clearSurfaces( GLShaderManager* manager ) :
	GLShader( "clearSurfaces", ATTR_POSITION, manager, false, false, true ),
	u_Frame( this ) {
}

GLShader_processSurfaces::GLShader_processSurfaces( GLShaderManager* manager ) :
	GLShader( "processSurfaces", ATTR_POSITION, manager, false, false, true ),
	u_Frame( this ),
	u_ViewID( this ),
	u_SurfaceCommandsOffset( this ) {
}
