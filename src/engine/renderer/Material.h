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

static constexpr uint32_t MAX_DRAWCOMMAND_TEXTURES = 64;

/* Similar to GLIndirectBuffer::GLIndirectCommand, but we always set instanceCount to 1 and baseVertex to 0,
 so no need to waste memory on those */
struct IndirectCompactCommand {
	GLuint count;
	GLuint firstIndex;
	GLuint baseInstance;
};

struct DrawCommand {
	IndirectCompactCommand cmd;
	uint32_t materialsSSBOOffset = 0;
	uint32_t textureCount = 0;
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
	uint32_t materialsSSBOOffset = 0;
	uint32_t staticMaterialsSSBOOffset = 0;
	uint32_t dynamicMaterialsSSBOOffset = 0;
	uint32_t totalDrawSurfCount = 0;
	uint32_t totalStaticDrawSurfCount = 0;
	uint32_t totalDynamicDrawSurfCount = 0;
	uint32_t currentDrawSurfCount = 0;
	uint32_t currentStaticDrawSurfCount = 0;
	uint32_t currentDynamicDrawSurfCount = 0;

	uint32_t globalID = 0;
	uint32_t surfaceCommandBatchOffset = 0;
	uint32_t surfaceCommandBatchCount = 0;
	uint32_t surfaceCommandBatchPadding = 0;

	uint32_t id = 0;
	bool useSync = false;
	uint32_t syncMaterial = 0; // Must not be drawn before the material with this id

	uint32_t stateBits = 0;
	stageShaderBinder_t shaderBinder;
	GLuint program = 0;
	GLShader* shader;

	int deformIndex;
	bool tcGenEnvironment;
	bool tcGen_Lightmap;
	bool hasDepthFade;
	bool alphaTest;

	bool bspSurface;
	bool enableDeluxeMapping;
	bool enableGridLighting;
	bool enableGridDeluxeMapping;
	bool hasHeightMapInNormalMap;
	bool enableReliefMapping;
	bool enableNormalMapping;
	bool enableSpecularMapping;
	bool enablePhysicalMapping;

	cullType_t cullType;

	uint32_t sort;

	bool usePolygonOffset = false;

	VBO_t* vbo;
	IBO_t* ibo;

	std::vector<drawSurf_t*> drawSurfs;
	std::vector<DrawCommand> drawCommands;
	bool texturesResident = false;
	std::vector<Texture*> textures;

	bool operator==( const Material& other ) {
		return program == other.program && stateBits == other.stateBits && vbo == other.vbo && ibo == other.ibo
			&& cullType == other.cullType && usePolygonOffset == other.usePolygonOffset;
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

enum class MaterialDebugMode {
	NONE,
	DEPTH,
	OPAQUE,
	OPAQUE_TRANSPARENT
};

struct PortalSurface {
	vec3_t origin;
	float radius;

	uint32_t drawSurfID;
	float distance;
	vec2_t padding;
};

struct PortalView {
	uint32_t count;
	drawSurf_t* drawSurf;
	uint32_t views[MAX_VIEWS];
};

extern PortalView portalStack[MAX_VIEWS];

#define MAX_SURFACE_COMMANDS 16
#define MAX_COMMAND_COUNTERS 64
#define SURFACE_COMMANDS_PER_BATCH 64

#define MAX_SURFACE_COMMAND_BATCHES 4096

#define BOUNDING_SPHERE_SIZE 4

#define INDIRECT_COMMAND_SIZE 5
#define SURFACE_COMMAND_SIZE 4
#define SURFACE_COMMAND_BATCH_SIZE 2
#define PORTAL_SURFACE_SIZE 8

#define MAX_FRAMES 2
#define MAX_VIEWFRAMES MAX_VIEWS * MAX_FRAMES // Buffer 2 frames for each view

struct ViewFrame {
	uint32_t viewID = 0;
	uint32_t portalViews[MAX_VIEWS];
	uint32_t viewCount;
	vec3_t origin;
	frustum_t frustum;
	uint32_t portalSurfaceID;
};

struct Frame {
	uint32_t viewCount = 0;
	ViewFrame viewFrames[MAX_VIEWS];
};

struct BoundingSphere {
	vec3_t origin;
	float radius;
};

struct SurfaceDescriptor {
	BoundingSphere boundingSphere;
	uint32_t surfaceCommandIDs[MAX_SURFACE_COMMANDS] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
};

struct SurfaceCommand {
	uint32_t enabled; // uint because bool in GLSL is always 4 bytes
	IndirectCompactCommand drawCommand;
};

struct SurfaceCommandBatch {
	uint32_t materialIDs[2] { 0, 0 };
};

class MaterialSystem {
	public:
	bool generatedWorldCommandBuffer = false;
	bool generatingWorldCommandBuffer = false;
	vec3_t worldViewBounds[2] = {};

	uint8_t maxStages = 0;
	uint32_t descriptorSize;

	std::vector<DrawCommand> drawCommands;

	std::vector<drawSurf_t*> portalSurfacesTmp;
	std::vector<drawSurf_t> portalSurfaces;
	std::vector<drawSurf_t> autospriteSurfaces;
	std::vector<PortalSurface> portalBounds;
	uint32_t totalPortals;
	std::vector<shader_t*> skyShaders;

	std::vector<Material*> renderedMaterials;

	/* MaterialPack is an abstraction to match a range of materials with the 3 different calls to RB_RenderDrawSurfaces()
	with 3 different shaderSort_t ranges in RB_RenderView(). The 4th one that uses a different surface filter (DRAWSURFACES_NEAR_ENTITIES)
	is ignored because it's never used for BSP surfaces. */
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

	void AddTexture( Texture* texture );
	void AddDrawCommand( const uint32_t materialID, const uint32_t materialPackID, const uint32_t materialsSSBOOffset,
						 const GLuint count, const GLuint firstIndex );

	void AddPortalSurfaces();
	void AddAutospriteSurfaces();
	void RenderMaterials( const shaderSort_t fromSort, const shaderSort_t toSort, const uint32_t viewID );
	void UpdateDynamicSurfaces();

	void QueueSurfaceCull( const uint32_t viewID, const vec3_t origin, const frustum_t* frustum );
	void DepthReduction();
	void CullSurfaces();
	
	void StartFrame();
	void EndFrame();

	void GenerateDepthImages( const int width, const int height, imageParams_t imageParms );

	void AddStageTextures( drawSurf_t* drawSurf, shaderStage_t* pStage, Material* material );
	void ProcessStage( drawSurf_t* drawSurf, shaderStage_t* pStage, shader_t* shader, uint32_t* packIDs, uint32_t& stage,
		uint32_t& previousMaterialID );
	void GenerateWorldMaterials();
	void GenerateWorldMaterialsBuffer();
	void GenerateWorldCommandBuffer();
	void GeneratePortalBoundingSpheres();

	void AddAllWorldSurfaces();

	void Free();

	private:
	bool PVSLocked = false;
	frustum_t lockedFrustums[MAX_VIEWS];
	vec3_t lockedOrigins[MAX_VIEWS];
	image_t* lockedDepthImage;
	matrix_t lockedViewMatrix;

	uint32_t viewCount;

	image_t* depthImage;
	int depthImageLevels;

	DrawCommand cmd;
	uint32_t lastCommandID;
	uint32_t totalDrawSurfs;
	uint32_t totalBatchCount = 0;

	uint32_t surfaceCommandsCount = 0;
	uint32_t surfaceDescriptorsCount = 0;

	std::vector<drawSurf_t> dynamicDrawSurfs;
	uint32_t dynamicDrawSurfsOffset = 0;
	uint32_t dynamicDrawSurfsSize = 0;

	Frame frames[MAX_FRAMES];
	uint32_t currentFrame = 0;
	uint32_t nextFrame = 1;

	bool AddPortalSurface( uint32_t viewID, PortalSurface* portalSurfs );

	void RenderIndirect( const Material& material, const uint32_t viewID );
	void RenderMaterial( Material& material, const uint32_t viewID );
	void UpdateFrameData();
};

extern GLSSBO materialsSSBO; // Global

extern GLSSBO surfaceDescriptorsSSBO; // Global
extern GLSSBO surfaceCommandsSSBO; // Per viewframe, GPU updated
extern GLBuffer culledCommandsBuffer; // Per viewframe
extern GLUBO surfaceBatchesUBO; // Global
extern GLBuffer atomicCommandCountersBuffer; // Per viewframe
extern GLSSBO portalSurfacesSSBO; // Per viewframe

extern GLSSBO debugSSBO; // Global

extern MaterialSystem materialSystem;

void UpdateSurfaceDataNONE( uint32_t*, Material&, drawSurf_t*, const uint32_t );
void UpdateSurfaceDataNOP( uint32_t*, Material&, drawSurf_t*, const uint32_t );
void UpdateSurfaceDataGeneric3D( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage );
void UpdateSurfaceDataLightMapping( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage );
void UpdateSurfaceDataReflection( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage );
void UpdateSurfaceDataSkybox( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage );
void UpdateSurfaceDataScreen( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage );
void UpdateSurfaceDataHeatHaze( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage );
void UpdateSurfaceDataLiquid( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage );

void BindShaderNONE( Material* );
void BindShaderNOP( Material* );
void BindShaderGeneric3D( Material* material );
void BindShaderLightMapping( Material* material );
void BindShaderReflection( Material* material );
void BindShaderSkybox( Material* material );
void BindShaderScreen( Material* material );
void BindShaderHeatHaze( Material* material );
void BindShaderLiquid( Material* material );

void ProcessMaterialNONE( Material*, shaderStage_t*, drawSurf_t* );
void ProcessMaterialNOP( Material*, shaderStage_t*, drawSurf_t* );
void ProcessMaterialGeneric3D( Material* material, shaderStage_t* pStage, drawSurf_t* drawSurf );
void ProcessMaterialLightMapping( Material* material, shaderStage_t* pStage, drawSurf_t* drawSurf );
void ProcessMaterialReflection( Material* material, shaderStage_t* pStage, drawSurf_t* /* drawSurf */ );
void ProcessMaterialSkybox( Material* material, shaderStage_t* pStage, drawSurf_t* /* drawSurf */ );
void ProcessMaterialScreen( Material* material, shaderStage_t* pStage, drawSurf_t* /* drawSurf */ );
void ProcessMaterialHeatHaze( Material* material, shaderStage_t* pStage, drawSurf_t* drawSurf );
void ProcessMaterialLiquid( Material* material, shaderStage_t* pStage, drawSurf_t* /* drawSurf */ );

#endif // MATERIAL_H
