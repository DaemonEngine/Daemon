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
// GLUtils.h

#ifndef GLUTILS_H
#define GLUTILS_H

#include "common/Common.h"
#include "GL/glew.h"

#include "engine/RefAPI.h"

#include "DetectGLVendors.h"

// Note: some of these (particularly ones dealing with GL extensions) are only updated on
// an explicit vid_restart - see GLimp_InitExtensions(). Others are updated on every map change
// - see EnableAvailableFeatures().
struct GLConfig
{
	char renderer_string[MAX_STRING_CHARS];
	char vendor_string[MAX_STRING_CHARS];
	char version_string[MAX_STRING_CHARS];

	int maxTextureSize; // queried from GL

	int colorBits;

	glDriverType_t driverType;
	glHardwareType_t hardwareType;

	textureCompression_t textureCompression;

	uint32_t sdlDisplayID;

	bool smpActive; // dual processor

	bool textureCompressionRGTCAvailable;

	int glHighestMajor;
	int glHighestMinor;

	int glRequestedMajor;
	int glRequestedMinor;

	int glMajor;
	int glMinor;

	bool glCoreProfile;
	bool glForwardCompatibleContext;

	glDriverVendor_t driverVendor;
	glHardwareVendor_t hardwareVendor;

	std::string glExtensionsString;
	std::string glEnabledExtensionsString;
	std::string glMissingExtensionsString;

	int max3DTextureSize;
	int maxCubeMapTextureSize;
	int maxTextureUnits;

	char shadingLanguageVersionString[MAX_STRING_CHARS];
	int  shadingLanguageVersion;

	int maxUniformBlockSize;
	int maxVertexUniforms;
	int maxVertexAttribs;
	bool vboVertexSkinningAvailable;
	int maxVertexSkinningBones;
	int maxAluInstructions;
	int maxTexIndirections;

	bool drawBuffersAvailable;
	bool textureHalfFloatAvailable;
	bool textureFloatAvailable;
	bool textureIntegerAvailable;
	bool textureRGAvailable;
	bool computeShaderAvailable;
	bool bindlessTexturesAvailable; // do the driver/hardware support it
	bool usingBindlessTextures; // are we using them right now
	bool usingReadonlyDepth;
	bool shaderDrawParametersAvailable;
	bool SSBOAvailable;
	bool multiDrawIndirectAvailable;
	bool indirectParametersAvailable;
	bool shadingLanguage420PackAvailable;
	bool explicitUniformLocationAvailable;
	bool directStateAccessAvailable;
	bool vertexAttribBindingAvailable;
	bool shaderImageLoadStoreAvailable;
	bool shaderAtomicCountersAvailable;
	bool shaderAtomicCounterOpsAvailable;
	bool shaderSubgroupAvailable;
	bool shaderSubgroupBasicAvailable;
	bool shaderSubgroupVoteAvailable;
	bool shaderSubgroupArithmeticAvailable;
	bool shaderSubgroupBallotAvailable;
	bool shaderSubgroupShuffleAvailable;
	bool shaderSubgroupShuffleRelativeAvailable;
	bool shaderSubgroupClusteredAvailable;
	bool shaderSubgroupQuadAvailable;
	bool materialSystemAvailable; // do the driver/hardware support it
	bool usingMaterialSystem; // are we using it right now
	bool geometryCacheAvailable;
	bool pushBufferAvailable;
	bool usingGeometryCache;
	bool gpuShader4Available;
	bool gpuShader5Available;
	bool textureGatherAvailable;
	int maxDrawBuffers;

	float maxTextureAnisotropy;
	float textureAnisotropy;
	bool textureAnisotropyAvailable;

	int maxColorAttachments;

	bool getProgramBinaryAvailable;
	bool bufferStorageAvailable;
	bool uniformBufferObjectAvailable;
	bool mapBufferRangeAvailable;
	bool syncAvailable;
	bool textureBarrierAvailable;
	bool halfFloatVertexAvailable;

	bool colorGrading;
	bool realtimeLighting;
	int realtimeLightLayers;
	bool deluxeMapping;
	bool normalMapping;
	bool specularMapping;
	bool physicalMapping;
	bool reliefMapping;
	bool reflectionMappingAvailable;
	bool reflectionMapping;
	bool bloom;
	bool ssao;
	bool motionBlur;
};

extern WindowConfig windowConfig; // outside of TR since it shouldn't be cleared during ref re-init
extern GLConfig glConfig;

void GL_CheckErrors_( const char *filename, int line );

void GL_BindVAO( const GLuint id );

#define GL_CheckErrors() do { if ( !glConfig.smpActive ) GL_CheckErrors_( __FILE__, __LINE__ ); } while ( false )

#endif // GLUTILS_H
