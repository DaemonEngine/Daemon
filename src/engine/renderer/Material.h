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

struct DrawCommand {
	GLIndirectBuffer::GLIndirectCommand cmd;
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

	uint32_t staticCommandOffset = 0;

	uint32_t id = 0;
	bool useSync = false;
	uint32_t syncMaterial = 0; // Must not be drawn before the material with this id

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

struct drawSurfBoundingSphere {
	vec3_t origin;
	float radius;

	uint32_t drawSurfID;
};

class MaterialSystem {
	public:
	bool generatedWorldCommandBuffer = false;
	bool skipDrawCommands;
	bool generatingWorldCommandBuffer = false;
	vec3_t worldViewBounds[2] = {};

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

	bool frameStart = true;

	void AddTexture( Texture* texture );
	void AddDrawCommand( const uint32_t materialID, const uint32_t materialPackID, const uint32_t materialsSSBOOffset,
						 const GLuint count, const GLuint firstIndex );

	void AddPortalSurfaces();
	void RenderMaterials( const shaderSort_t fromSort, const shaderSort_t toSort );
	void UpdateDynamicSurfaces();

	void AddStageTextures( drawSurf_t* drawSurf, shaderStage_t* pStage, Material* material );
	void GenerateWorldMaterials();
	void GenerateWorldMaterialsBuffer();
	void GenerateWorldCommandBuffer();
	void GeneratePortalBoundingSpheres();

	void AddAllWorldSurfaces();

	void Free();

	private:
	DrawCommand cmd;
	std::vector<drawSurf_t> dynamicDrawSurfs;
	uint32_t dynamicDrawSurfsOffset = 0;
	uint32_t dynamicDrawSurfsSize = 0;

	void RenderMaterial( Material& material );
};

extern GLSSBO materialsSSBO;
extern GLIndirectBuffer commandBuffer;
extern MaterialSystem materialSystem;

#endif // MATERIAL_H
