/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2024 Daemon Developers
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
// Material.h

#ifndef MATERIAL_H
#define MATERIAL_H

#include <vector>

#include "gl_shader.h"
#include "tr_local.h"

static constexpr uint MAX_DRAWCOMMAND_TEXTURES = 64;

struct DrawCommand {
	GLIndirectBuffer::GLIndirectCommand cmd;
	uint materialsSSBOOffset = 0;
	uint textureCount = 0;
	Texture* textures[MAX_DRAWCOMMAND_TEXTURES];

	DrawCommand() {
	}

	DrawCommand( const DrawCommand& other ) {
		cmd = other.cmd;
		materialsSSBOOffset = other.materialsSSBOOffset;
		textureCount = other.textureCount;
		memcpy( textures, other.textures, textureCount * sizeof( Texture* ) );
	}
};

struct Material {
	uint materialsSSBOOffset = 0;
	uint staticMaterialsSSBOOffset = 0;
	uint dynamicMaterialsSSBOOffset = 0;
	uint totalDrawSurfCount = 0;
	uint totalStaticDrawSurfCount = 0;
	uint totalDynamicDrawSurfCount = 0;
	uint currentDrawSurfCount = 0;
	uint currentStaticDrawSurfCount = 0;
	uint currentDynamicDrawSurfCount = 0;

	uint globalID = 0;
	uint surfaceCommandBatchOffset = 0;
	uint surfaceCommandBatchCount = 0;
	uint surfaceCommandBatchPadding = 0;

	uint id = 0;
	bool useSync = false;
	uint syncMaterial = 0; // Must not be drawn before the material with this id

	uint32_t stateBits = 0;
	stageType_t stageType;
	GLuint program = 0;
	GLShader* shader;

	int deformIndex;
	bool vertexAnimation;
	bool tcGenEnvironment;
	bool tcGen_Lightmap;
	bool hasDepthFade;
	bool vboVertexSprite;
	bool alphaTest;

	bool bspSurface;
	bool enableDeluxeMapping;
	bool enableGridLighting;
	bool enableGridDeluxeMapping;
	bool hasHeightMapInNormalMap;
	bool enableReliefMapping;
	bool enableNormalMapping;
	bool enablePhysicalMapping;

	cullType_t cullType;

	bool usePolygonOffset = false;

	VBO_t* vbo;
	IBO_t* ibo;

	std::vector<drawSurf_t*> drawSurfs;
	std::vector<DrawCommand> drawCommands;
	bool texturesResident = false;
	std::vector<Texture*> textures;

	bool operator==( const Material& other ) {
		return program == other.program && stateBits == other.stateBits && vbo == other.vbo && ibo == other.ibo
			&& stateBits == other.stateBits && cullType == other.cullType && usePolygonOffset == other.usePolygonOffset;
	}

	void AddTexture( Texture* texture ) {
		if ( !texture->hasBindlessHandle ) {
			texture->GenBindlessHandle();
		}

		if ( std::find( textures.begin(), textures.end(), texture ) == textures.end() ) {
			textures.emplace_back( texture );
		}
	}
};

struct drawSurfBoundingSphere {
	vec3_t origin;
	float radius;

	uint drawSurfID;
};

#define MAX_SURFACE_COMMANDS 16
#define MAX_COMMAND_COUNTERS 64
#define SURFACE_COMMANDS_PER_BATCH 64

#define MAX_SURFACE_COMMAND_BATCHES 2048

#define BOUNDING_SPHERE_SIZE 4

#define INDIRECT_COMMAND_SIZE 5
#define SURFACE_COMMAND_SIZE 6
#define SURFACE_COMMAND_BATCH_SIZE 4 // Aligned to 4 components

#define MAX_FRAMES 2
#define MAX_VIEWFRAMES MAX_VIEWS * MAX_FRAMES // Buffer 2 frames for each view

struct ViewFrame {
	uint viewID = 0;
	uint portalViews[MAX_VIEWS];
	frustum_t frustum;
};

struct Frame {
	uint viewCount = 0;
	ViewFrame viewFrames[MAX_VIEWS];
	image_t* depthTexture;
	image_t* depthImage;
};

struct BoundingSphere {
	vec3_t origin;
	float radius;
};

struct SurfaceDescriptor {
	BoundingSphere boundingSphere;
	uint surfaceCommandIDs[MAX_SURFACE_COMMANDS] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
};

struct SurfaceCommand {
	uint enabled; // uint because bool in GLSL is always 4 bytes
	GLIndirectBuffer::GLIndirectCommand drawCommand;
};

struct SurfaceCommandBatch {
	uint materialIDs[4] { 0, 0, 0, 0 };
};

class MaterialSystem {
	public:
	bool generatedWorldCommandBuffer = false;
	bool skipDrawCommands;
	bool skipSurface;
	bool generatingWorldCommandBuffer = false;
	vec3_t worldViewBounds[2] = {};

	uint currentView = 0;

	uint8_t maxStages = 0;
	uint descriptorSize;

	std::vector<DrawCommand> drawCommands;

	std::vector<drawSurf_t*> portalSurfacesTmp;
	std::vector<drawSurf_t> portalSurfaces;
	std::vector<drawSurfBoundingSphere> portalBounds;
	std::vector<shader_t*> skyShaders;

	std::vector<Material*> renderedMaterials;

	struct MaterialPack {
		const shaderSort_t fromSort;
		const shaderSort_t toSort;
		std::vector<Material> materials;
		
		MaterialPack( const shaderSort_t newFromSort, const shaderSort_t newToSort ) :
		fromSort( newFromSort ),
		toSort( newToSort ) {
		}
	};

	MaterialPack materialPacks[3]{
		{ shaderSort_t::SS_DEPTH, shaderSort_t::SS_DEPTH },
		{ shaderSort_t::SS_ENVIRONMENT_FOG, shaderSort_t::SS_OPAQUE },
		{ shaderSort_t::SS_ENVIRONMENT_NOFOG, shaderSort_t::SS_POST_PROCESS }
	};

	bool frameStart = false;
	uint testTex[2];

	void AddTexture( Texture* texture );
	void AddDrawCommand( const uint materialID, const uint materialPackID, const uint materialsSSBOOffset,
						 const GLuint count, const GLuint firstIndex );

	void AddPortalSurfaces();
	void RenderMaterials( const shaderSort_t fromSort, const shaderSort_t toSort, const uint viewID );
	void UpdateDynamicSurfaces();

	void QueueSurfaceCull( const uint viewID, const frustum_t* frustum );
	void DepthReduction();
	void CullSurfaces();
	
	void StartFrame();
	void EndFrame();

	void GenerateDepthImages( const int width, const int height, imageParams_t imageParms );

	void AddStageTextures( drawSurf_t* drawSurf, shaderStage_t* pStage, Material* material );
	void ProcessStage( drawSurf_t* drawSurf, shaderStage_t* pStage, shader_t* shader, uint& id, uint* packIDs, uint& stage,
		uint& previousMaterialID );
	void GenerateWorldMaterials();
	void GenerateWorldMaterialsBuffer();
	void GenerateWorldCommandBuffer();
	void GeneratePortalBoundingSpheres();

	void AddAllWorldSurfaces();

	void Free();

	private:
	bool PVSLocked = false;
	frustum_t lockedFrustum;
	image_t* lockedDepthImage;

	int depthImageLevels;

	DrawCommand cmd;
	uint lastCommandID;
	uint totalDrawSurfs;
	uint totalBatchCount = 0;

	uint surfaceCommandsCount = 0;
	uint culledCommandsCount = 0;
	uint surfaceDescriptorsCount = 0;

	std::vector<drawSurf_t> dynamicDrawSurfs;
	uint dynamicDrawSurfsOffset = 0;
	uint dynamicDrawSurfsSize = 0;

	Frame frames[MAX_FRAMES];
	uint currentFrame = 0;
	uint nextFrame = 1;

	void RenderMaterial( Material& material, const uint viewID );
	void UpdateFrameData();
};

extern GLSSBO materialsSSBO;
extern GLSSBO surfaceDescriptorsSSBO; // Global
extern GLSSBO surfaceCommandsSSBO; // Per viewframe, GPU updated
extern GLBuffer culledCommandsBuffer; // Per viewframe
extern GLUBO surfaceBatchesUBO; // Global
extern GLBuffer atomicCommandCountersBuffer; // Per viewframe
extern MaterialSystem materialSystem;

#endif // MATERIAL_H
