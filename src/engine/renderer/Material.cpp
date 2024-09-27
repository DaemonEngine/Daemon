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
// Material.cpp

#include "tr_local.h"
#include "Material.h"
#include "ShadeCommon.h"

GLSSBO materialsSSBO( "materials", 0, GL_MAP_WRITE_BIT, GL_MAP_INVALIDATE_RANGE_BIT );
GLSSBO surfaceDescriptorsSSBO( "surfaceDescriptors", 1, GL_MAP_WRITE_BIT, GL_MAP_INVALIDATE_RANGE_BIT );
GLSSBO surfaceCommandsSSBO( "surfaceCommands", 2, GL_MAP_WRITE_BIT, GL_MAP_FLUSH_EXPLICIT_BIT );
GLBuffer culledCommandsBuffer( "culledCommands", 3, GL_MAP_WRITE_BIT, GL_MAP_FLUSH_EXPLICIT_BIT );
GLUBO surfaceBatchesUBO( "surfaceBatches", 0, GL_MAP_WRITE_BIT, GL_MAP_INVALIDATE_RANGE_BIT );
GLBuffer atomicCommandCountersBuffer( "atomicCommandCounters", 4, GL_MAP_WRITE_BIT, GL_MAP_FLUSH_EXPLICIT_BIT );
GLSSBO portalSurfacesSSBO( "portalSurfaces", 5, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT, 0 );

PortalView portalStack[MAX_VIEWS];

MaterialSystem materialSystem;

static void ComputeDynamics( shaderStage_t* pStage ) {
	// TODO: Move color and texMatrices stuff to a compute shader
	switch ( pStage->rgbGen ) {
		case colorGen_t::CGEN_IDENTITY:
		case colorGen_t::CGEN_ONE_MINUS_VERTEX:
		default:
		case colorGen_t::CGEN_IDENTITY_LIGHTING:
			/* Historically CGEN_IDENTITY_LIGHTING was done this way:

			  tess.svars.color = Color::White * tr.identityLight;

			But tr.identityLight is always 1.0f in Dæmon engine
			as the as the overbright bit implementation is fully
			software. */
		case colorGen_t::CGEN_VERTEX:
		case colorGen_t::CGEN_CONST:
		case colorGen_t::CGEN_ENTITY:
		case colorGen_t::CGEN_ONE_MINUS_ENTITY:
		{
			// TODO: Move this to some entity buffer once this is extended past BSP surfaces
			if ( backEnd.currentEntity ) {
				//
			} else {
				//
			}
			pStage->colorDynamic = false;

			break;
		}

		case colorGen_t::CGEN_WAVEFORM:
		case colorGen_t::CGEN_CUSTOM_RGB:
		case colorGen_t::CGEN_CUSTOM_RGBs:
		{
			pStage->colorDynamic = true;
			break;
		}
	}

	switch ( pStage->alphaGen ) {
		default:
		case alphaGen_t::AGEN_IDENTITY:
		case alphaGen_t::AGEN_ONE_MINUS_VERTEX:
		case alphaGen_t::AGEN_VERTEX:
		case alphaGen_t::AGEN_CONST: {
		case alphaGen_t::AGEN_ENTITY:
		case alphaGen_t::AGEN_ONE_MINUS_ENTITY:
			// TODO: Move this to some entity buffer once this is extended past BSP surfaces
			/* if ( backEnd.currentEntity ) {
			} else {
			} */
			pStage->colorDynamic = false;
			break;
		}

		case alphaGen_t::AGEN_WAVEFORM:
		case alphaGen_t::AGEN_CUSTOM:
		{
			pStage->colorDynamic = true;
			break;
		}
	}

	for ( textureBundle_t& bundle : pStage->bundle ) {
		for ( size_t i = 0; i < bundle.numTexMods; i++ ) {
			switch ( bundle.texMods[i].type ) {
				case texMod_t::TMOD_NONE:
				case texMod_t::TMOD_SCALE:
				case texMod_t::TMOD_TRANSFORM:
					break;

				case texMod_t::TMOD_TURBULENT:
				case texMod_t::TMOD_ENTITY_TRANSLATE:
				case texMod_t::TMOD_SCROLL:
				{
					pStage->texMatricesDynamic = true;
					break;
				}

				case texMod_t::TMOD_STRETCH:
				{
					if( bundle.texMods->wave.func != genFunc_t::GF_NONE ) {
						pStage->texMatricesDynamic = true;
					}
					break;
				}

				case texMod_t::TMOD_ROTATE:
				{
					pStage->texMatricesDynamic = true;
					break;
				}

				case texMod_t::TMOD_SCROLL2:
				case texMod_t::TMOD_SCALE2:
				case texMod_t::TMOD_CENTERSCALE:
				case texMod_t::TMOD_SHEAR:
				{
					if ( bundle.texMods[i].sExp.numOps || bundle.texMods[i].tExp.numOps ) {
						pStage->texMatricesDynamic = true;
					}
					break;
				}

				case texMod_t::TMOD_ROTATE2:
				{
					if( bundle.texMods[i].rExp.numOps ) {
						pStage->texMatricesDynamic = true;
					}
					break;
				}

				default:
					break;
			}
		}
	}

	// TODO: Move this to a different buffer?
	for ( const textureBundle_t& bundle : pStage->bundle ) {
		if ( bundle.isVideoMap || bundle.numImages > 1 ) {
			pStage->texturesDynamic = true;
			break;
		}
	}

	// Can we move this to a compute shader too?
	// Doesn't seem to be used much if at all, so probably not worth the effort to do that
	pStage->dynamic = pStage->dynamic || pStage->ifExp.numOps;
	pStage->dynamic = pStage->dynamic || pStage->alphaExp.numOps || pStage->alphaTestExp.numOps;
	pStage->dynamic = pStage->dynamic || pStage->rgbExp.numOps || pStage->redExp.numOps || pStage->greenExp.numOps || pStage->blueExp.numOps;
	pStage->dynamic = pStage->dynamic || pStage->deformMagnitudeExp.numOps;
	pStage->dynamic = pStage->dynamic || pStage->depthScaleExp.numOps || pStage->etaExp.numOps || pStage->etaDeltaExp.numOps
		|| pStage->fogDensityExp.numOps || pStage->fresnelBiasExp.numOps || pStage->fresnelPowerExp.numOps
		|| pStage->fresnelScaleExp.numOps || pStage->normalIntensityExp.numOps || pStage->refractionIndexExp.numOps;

	pStage->dynamic = pStage->dynamic || pStage->colorDynamic || pStage->texMatricesDynamic || pStage->texturesDynamic;
}

// UpdateSurface*() functions will actually write the uniform values to the SSBO
// Mirrors parts of the Render_*() functions in tr_shade.cpp

void UpdateSurfaceDataNONE( uint32_t*, Material&, drawSurf_t*, const uint32_t ) {
	ASSERT_UNREACHABLE();
}

void UpdateSurfaceDataNOP( uint32_t*, Material&, drawSurf_t*, const uint32_t ) {
}

void UpdateSurfaceDataGeneric3D( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
	shader_t* shader = drawSurf->shader;
	shaderStage_t* pStage = &shader->stages[stage];

	const uint32_t paddedOffset = drawSurf->materialsSSBOOffset[stage] * material.shader->GetPaddedSize();
	materials += paddedOffset;

	bool updated = !drawSurf->initialized[stage] || pStage->colorDynamic || pStage->texMatricesDynamic || pStage->dynamic;
	if ( !updated ) {
		return;
	}
	drawSurf->initialized[stage] = true;

	gl_genericShaderMaterial->BindProgram( material.deformIndex );

	gl_genericShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_genericShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );

	// u_AlphaThreshold
	gl_genericShaderMaterial->SetUniform_AlphaTest( pStage->stateBits );

	// u_InverseLightFactor
	float inverseLightFactor = pStage->cancelOverBright ? tr.mapInverseLightFactor : 1.0f;
	gl_genericShaderMaterial->SetUniform_InverseLightFactor( inverseLightFactor );

	// u_ColorModulate
	colorGen_t rgbGen = SetRgbGen( pStage );
	alphaGen_t alphaGen = SetAlphaGen( pStage );

	gl_genericShaderMaterial->SetUniform_ColorModulate( rgbGen, alphaGen );

	Tess_ComputeColor( pStage );
	gl_genericShaderMaterial->SetUniform_Color( tess.svars.color );

	Tess_ComputeTexMatrices( pStage );
	gl_genericShaderMaterial->SetUniform_TextureMatrix( tess.svars.texMatrices[TB_COLORMAP] );

	// u_DeformGen
	gl_genericShaderMaterial->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	// bind u_ColorMap
	if ( pStage->type == stageType_t::ST_STYLELIGHTMAP ) {
		gl_genericShaderMaterial->SetUniform_ColorMapBindless(
			GL_BindToTMU( 0, GetLightMap( drawSurf ) )
		);
	} else {
		gl_genericShaderMaterial->SetUniform_ColorMapBindless( BindAnimatedImage( 0, &pStage->bundle[TB_COLORMAP] ) );
	}

	bool needDepthMap = pStage->hasDepthFade || shader->autoSpriteMode;
	if ( needDepthMap ) {
		gl_genericShaderMaterial->SetUniform_DepthMapBindless( GL_BindToTMU( 1, tr.currentDepthImage ) );
	}

	bool hasDepthFade = pStage->hasDepthFade && !shader->autoSpriteMode;
	if ( hasDepthFade ) {
		gl_genericShaderMaterial->SetUniform_DepthScale( pStage->depthFadeValue );
	}

	gl_genericShaderMaterial->SetUniform_VertexInterpolation( false );

	gl_genericShaderMaterial->WriteUniformsToBuffer( materials );
}

void UpdateSurfaceDataLightMapping( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
	shader_t* shader = drawSurf->shader;
	shaderStage_t* pStage = &shader->stages[stage];

	const uint32_t paddedOffset = drawSurf->materialsSSBOOffset[stage] * material.shader->GetPaddedSize();
	materials += paddedOffset;

	bool updated = !drawSurf->initialized[stage] || pStage->colorDynamic || pStage->texMatricesDynamic || pStage->dynamic;
	if ( !updated ) {
		return;
	}
	drawSurf->initialized[stage] = true;

	gl_lightMappingShaderMaterial->BindProgram( material.deformIndex );

	gl_lightMappingShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );

	lightMode_t lightMode;
	deluxeMode_t deluxeMode;
	SetLightDeluxeMode( drawSurf, pStage->type, lightMode, deluxeMode );

	// u_Map, u_DeluxeMap
	image_t* lightmap = SetLightMap( drawSurf, lightMode );
	image_t* deluxemap = SetDeluxeMap( drawSurf, deluxeMode );

	// u_ColorModulate
	colorGen_t rgbGen = SetRgbGen( pStage );
	alphaGen_t alphaGen = SetAlphaGen( pStage );

	SetVertexLightingSettings( lightMode, rgbGen );

	bool enableGridLighting = ( lightMode == lightMode_t::GRID );
	bool enableGridDeluxeMapping = ( deluxeMode == deluxeMode_t::GRID );

	// TODO: Update this when this is extended to MDV support
	gl_lightMappingShaderMaterial->SetUniform_VertexInterpolation( false );

	if ( glConfig2.realtimeLighting ) {
		gl_lightMappingShaderMaterial->SetUniformBlock_Lights( tr.dlightUBO );

		// bind u_LightTiles
		if ( r_realtimeLightingRenderer.Get() == Util::ordinal( realtimeLightingRenderer_t::TILED ) ) {
			gl_lightMappingShaderMaterial->SetUniform_LightTilesIntBindless(
				GL_BindToTMU( BIND_LIGHTTILES, tr.lighttileRenderImage )
			);
		}
	}

	// u_DeformGen
	gl_lightMappingShaderMaterial->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	// u_InverseLightFactor
	/* HACK: use sign to know if there is a light or not, and
	then if it will receive overbright multiplication or not. */
	bool cancelOverBright = pStage->cancelOverBright || lightMode == lightMode_t::FULLBRIGHT;
	float inverseLightFactor = cancelOverBright ? tr.mapInverseLightFactor : -tr.mapInverseLightFactor;
	gl_lightMappingShaderMaterial->SetUniform_InverseLightFactor( inverseLightFactor );

	// u_ColorModulate
	gl_lightMappingShaderMaterial->SetUniform_ColorModulate( rgbGen, alphaGen );

	// u_Color
	Tess_ComputeColor( pStage );
	gl_lightMappingShaderMaterial->SetUniform_Color( tess.svars.color );

	// u_AlphaThreshold
	gl_lightMappingShaderMaterial->SetUniform_AlphaTest( pStage->stateBits );

	// bind u_HeightMap
	if ( pStage->enableReliefMapping ) {
		float depthScale = RB_EvalExpression( &pStage->depthScaleExp, r_reliefDepthScale->value );
		depthScale *= shader->reliefDepthScale;

		gl_lightMappingShaderMaterial->SetUniform_ReliefDepthScale( depthScale );
		gl_lightMappingShaderMaterial->SetUniform_ReliefOffsetBias( shader->reliefOffsetBias );

		// FIXME: if there is both, embedded heightmap in normalmap is used instead of standalone heightmap
		if ( !pStage->hasHeightMapInNormalMap ) {
			gl_lightMappingShaderMaterial->SetUniform_HeightMapBindless(
				GL_BindToTMU( BIND_HEIGHTMAP, pStage->bundle[TB_HEIGHTMAP].image[0] )
			);
		}
	}

	// bind u_DiffuseMap
	gl_lightMappingShaderMaterial->SetUniform_DiffuseMapBindless(
		GL_BindToTMU( BIND_DIFFUSEMAP, pStage->bundle[TB_DIFFUSEMAP].image[0] )
	);

	if ( pStage->type != stageType_t::ST_LIGHTMAP ) {
		Tess_ComputeTexMatrices( pStage );
		gl_lightMappingShaderMaterial->SetUniform_TextureMatrix( tess.svars.texMatrices[TB_DIFFUSEMAP] );
	}

	// bind u_NormalMap
	if ( !!r_normalMapping->integer || pStage->hasHeightMapInNormalMap ) {
		gl_lightMappingShaderMaterial->SetUniform_NormalMapBindless(
			GL_BindToTMU( BIND_NORMALMAP, pStage->bundle[TB_NORMALMAP].image[0] )
		);
	}

	// bind u_NormalScale
	if ( pStage->enableNormalMapping ) {
		vec3_t normalScale;
		SetNormalScale( pStage, normalScale );

		gl_lightMappingShaderMaterial->SetUniform_NormalScale( normalScale );
	}

	// bind u_MaterialMap
	if ( pStage->enableSpecularMapping || pStage->enablePhysicalMapping ) {
		gl_lightMappingShaderMaterial->SetUniform_MaterialMapBindless(
			GL_BindToTMU( BIND_MATERIALMAP, pStage->bundle[TB_MATERIALMAP].image[0] )
		);
	}

	if ( pStage->enableSpecularMapping ) {
		float specExpMin = RB_EvalExpression( &pStage->specularExponentMin, r_specularExponentMin->value );
		float specExpMax = RB_EvalExpression( &pStage->specularExponentMax, r_specularExponentMax->value );

		gl_lightMappingShaderMaterial->SetUniform_SpecularExponent( specExpMin, specExpMax );
	}

	// TODO: Move this to a per-entity buffer
	// specular reflection
	if ( tr.cubeHashTable != nullptr ) {
		cubemapProbe_t* cubeProbeNearest;
		cubemapProbe_t* cubeProbeSecondNearest;

		image_t* cubeMap0 = nullptr;
		image_t* cubeMap1 = nullptr;

		float interpolation = 0.0;

		bool isWorldEntity = backEnd.currentEntity == &tr.worldEntity;

		if ( backEnd.currentEntity && !isWorldEntity ) {
			R_FindTwoNearestCubeMaps( backEnd.currentEntity->e.origin, &cubeProbeNearest, &cubeProbeSecondNearest );
		} else {
			// FIXME position
			R_FindTwoNearestCubeMaps( backEnd.viewParms.orientation.origin, &cubeProbeNearest, &cubeProbeSecondNearest );
		}

		if ( cubeProbeNearest == nullptr && cubeProbeSecondNearest == nullptr ) {
			GLimp_LogComment( "cubeProbeNearest && cubeProbeSecondNearest == NULL\n" );

			cubeMap0 = tr.whiteCubeImage;
			cubeMap1 = tr.whiteCubeImage;
		} else if ( cubeProbeNearest == nullptr ) {
			GLimp_LogComment( "cubeProbeNearest == NULL\n" );

			cubeMap0 = cubeProbeSecondNearest->cubemap;
		} else if ( cubeProbeSecondNearest == nullptr ) {
			GLimp_LogComment( "cubeProbeSecondNearest == NULL\n" );

			cubeMap0 = cubeProbeNearest->cubemap;
		} else {
			float cubeProbeNearestDistance, cubeProbeSecondNearestDistance;

			if ( backEnd.currentEntity && !isWorldEntity ) {
				cubeProbeNearestDistance = Distance( backEnd.currentEntity->e.origin, cubeProbeNearest->origin );
				cubeProbeSecondNearestDistance = Distance( backEnd.currentEntity->e.origin, cubeProbeSecondNearest->origin );
			} else {
				// FIXME position
				cubeProbeNearestDistance = Distance( backEnd.viewParms.orientation.origin, cubeProbeNearest->origin );
				cubeProbeSecondNearestDistance = Distance( backEnd.viewParms.orientation.origin, cubeProbeSecondNearest->origin );
			}

			interpolation = cubeProbeNearestDistance / ( cubeProbeNearestDistance + cubeProbeSecondNearestDistance );

			if ( r_logFile->integer ) {
				GLimp_LogComment( va( "cubeProbeNearestDistance = %f, cubeProbeSecondNearestDistance = %f, interpolation = %f\n",
					cubeProbeNearestDistance, cubeProbeSecondNearestDistance, interpolation ) );
			}

			cubeMap0 = cubeProbeNearest->cubemap;
			cubeMap1 = cubeProbeSecondNearest->cubemap;
		}

		/* TODO: Check why it is required to test for this, why
		cubeProbeNearest->cubemap and cubeProbeSecondNearest->cubemap
		can be nullptr while cubeProbeNearest and cubeProbeSecondNearest
		are not. Maybe this is only required while cubemaps are building. */
		if ( cubeMap0 == nullptr ) {
			cubeMap0 = tr.whiteCubeImage;
		}

		if ( cubeMap1 == nullptr ) {
			cubeMap1 = tr.whiteCubeImage;
		}

		// bind u_EnvironmentMap0
		gl_lightMappingShaderMaterial->SetUniform_EnvironmentMap0Bindless(
			GL_BindToTMU( BIND_ENVIRONMENTMAP0, cubeMap0 )
		);

		// bind u_EnvironmentMap1
		gl_lightMappingShaderMaterial->SetUniform_EnvironmentMap1Bindless(
			GL_BindToTMU( BIND_ENVIRONMENTMAP1, cubeMap1 )
		);

		// bind u_EnvironmentInterpolation
		gl_lightMappingShaderMaterial->SetUniform_EnvironmentInterpolation( interpolation );

		updated = true;
	}

	// bind u_LightMap
	if ( !enableGridLighting ) {
		gl_lightMappingShaderMaterial->SetUniform_LightMapBindless(
			GL_BindToTMU( BIND_LIGHTMAP, lightmap )
		);
	} else {
		gl_lightMappingShaderMaterial->SetUniform_LightGrid1Bindless( GL_BindToTMU( BIND_LIGHTMAP, lightmap ) );
	}

	// bind u_DeluxeMap
	if ( !enableGridDeluxeMapping ) {
		gl_lightMappingShaderMaterial->SetUniform_DeluxeMapBindless(
			GL_BindToTMU( BIND_DELUXEMAP, deluxemap )
		);
	} else {
		gl_lightMappingShaderMaterial->SetUniform_LightGrid2Bindless( GL_BindToTMU( BIND_DELUXEMAP, deluxemap ) );
	}

	// bind u_GlowMap
	if ( !!r_glowMapping->integer ) {
		gl_lightMappingShaderMaterial->SetUniform_GlowMapBindless(
			GL_BindToTMU( BIND_GLOWMAP, pStage->bundle[TB_GLOWMAP].image[0] )
		);
	}

	gl_lightMappingShaderMaterial->WriteUniformsToBuffer( materials );
}

void UpdateSurfaceDataReflection( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
	shader_t* shader = drawSurf->shader;
	shaderStage_t* pStage = &shader->stages[stage];

	const uint32_t paddedOffset = drawSurf->materialsSSBOOffset[stage] * material.shader->GetPaddedSize();
	materials += paddedOffset;

	bool updated = !drawSurf->initialized[stage] || pStage->colorDynamic || pStage->texMatricesDynamic || pStage->dynamic;
	if ( !updated ) {
		return;
	}
	drawSurf->initialized[stage] = true;

	gl_reflectionShaderMaterial->SetUniform_VertexInterpolation( false );

	// bind u_NormalMap
	gl_reflectionShaderMaterial->SetUniform_NormalMapBindless(
		GL_BindToTMU( 1, pStage->bundle[TB_NORMALMAP].image[0] )
	);

	// bind u_ColorMap
	if ( backEnd.currentEntity && ( backEnd.currentEntity != &tr.worldEntity ) ) {
		GL_BindNearestCubeMap( gl_reflectionShaderMaterial->GetUniformLocation_ColorMapCube(), backEnd.currentEntity->e.origin );
	} else {
		GL_BindNearestCubeMap( gl_reflectionShaderMaterial->GetUniformLocation_ColorMapCube(), backEnd.viewParms.orientation.origin );
	}

	if ( pStage->enableNormalMapping ) {
		vec3_t normalScale;
		SetNormalScale( pStage, normalScale );

		gl_reflectionShaderMaterial->SetUniform_NormalScale( normalScale );
	}

	// bind u_HeightMap u_depthScale u_reliefOffsetBias
	if ( pStage->enableReliefMapping ) {
		float depthScale = RB_EvalExpression( &pStage->depthScaleExp, r_reliefDepthScale->value );
		float reliefDepthScale = shader->reliefDepthScale;
		depthScale *= reliefDepthScale == 0 ? 1 : reliefDepthScale;
		gl_reflectionShaderMaterial->SetUniform_ReliefDepthScale( depthScale );
		gl_reflectionShaderMaterial->SetUniform_ReliefOffsetBias( shader->reliefOffsetBias );

		// FIXME: if there is both, embedded heightmap in normalmap is used instead of standalone heightmap
		if ( !pStage->hasHeightMapInNormalMap ) {
			gl_reflectionShaderMaterial->SetUniform_HeightMapBindless(
				GL_BindToTMU( 15, pStage->bundle[TB_HEIGHTMAP].image[0] )
			);
		}
	}

	gl_reflectionShaderMaterial->WriteUniformsToBuffer( materials );
}

void UpdateSurfaceDataSkybox( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
	shader_t* shader = drawSurf->shader;
	shaderStage_t* pStage = &shader->stages[stage];

	const uint32_t paddedOffset = drawSurf->materialsSSBOOffset[stage] * material.shader->GetPaddedSize();
	materials += paddedOffset;

	bool updated = !drawSurf->initialized[stage] || pStage->colorDynamic || pStage->texMatricesDynamic || pStage->dynamic;
	if ( !updated ) {
		return;
	}
	drawSurf->initialized[stage] = true;

	gl_skyboxShaderMaterial->BindProgram( material.deformIndex );

	// bind u_ColorMap
	gl_skyboxShaderMaterial->SetUniform_ColorMapCubeBindless(
		GL_BindToTMU( 0, pStage->bundle[TB_COLORMAP].image[0] )
	);

	// u_AlphaThreshold
	gl_skyboxShaderMaterial->SetUniform_AlphaTest( GLS_ATEST_NONE );

	// u_InverseLightFactor
	gl_skyboxShaderMaterial->SetUniform_InverseLightFactor( tr.mapInverseLightFactor );

	gl_skyboxShaderMaterial->WriteUniformsToBuffer( materials );
}

void UpdateSurfaceDataScreen( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
	shader_t* shader = drawSurf->shader;
	shaderStage_t* pStage = &shader->stages[stage];

	const uint32_t paddedOffset = drawSurf->materialsSSBOOffset[stage] * material.shader->GetPaddedSize();
	materials += paddedOffset;

	bool updated = !drawSurf->initialized[stage] || pStage->colorDynamic || pStage->texMatricesDynamic || pStage->dynamic;
	if ( !updated ) {
		return;
	}
	drawSurf->initialized[stage] = true;

	gl_screenShaderMaterial->BindProgram( pStage->deformIndex );

	// bind u_CurrentMap
	gl_screenShaderMaterial->SetUniform_CurrentMapBindless( BindAnimatedImage( 0, &drawSurf->shader->stages[stage].bundle[TB_COLORMAP] ) );

	gl_screenShaderMaterial->WriteUniformsToBuffer( materials );
}

void UpdateSurfaceDataHeatHaze( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
	shader_t* shader = drawSurf->shader;
	shaderStage_t* pStage = &shader->stages[stage];

	const uint32_t paddedOffset = drawSurf->materialsSSBOOffset[stage] * material.shader->GetPaddedSize();
	materials += paddedOffset;

	bool updated = !drawSurf->initialized[stage] || pStage->colorDynamic || pStage->texMatricesDynamic || pStage->dynamic;
	if ( !updated ) {
		return;
	}
	drawSurf->initialized[stage] = true;

	// bind u_NormalMap
	gl_heatHazeShaderMaterial->SetUniform_NormalMapBindless(
		GL_BindToTMU( 0, pStage->bundle[TB_NORMALMAP].image[0] )
	);

	float deformMagnitude = RB_EvalExpression( &pStage->deformMagnitudeExp, 1.0 );
	gl_heatHazeShaderMaterial->SetUniform_DeformMagnitude( deformMagnitude );

	if ( pStage->enableNormalMapping ) {
		vec3_t normalScale;
		SetNormalScale( pStage, normalScale );

		// bind u_NormalScale
		gl_heatHazeShaderMaterial->SetUniform_NormalScale( normalScale );
	}

	// bind u_CurrentMap
	gl_heatHazeShaderMaterial->SetUniform_CurrentMapBindless(
		GL_BindToTMU( 1, tr.currentRenderImage[backEnd.currentMainFBO] )
	);

	gl_heatHazeShaderMaterial->WriteUniformsToBuffer( materials );
}

void UpdateSurfaceDataLiquid( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
	shader_t* shader = drawSurf->shader;
	shaderStage_t* pStage = &shader->stages[stage];

	const uint32_t paddedOffset = drawSurf->materialsSSBOOffset[stage] * material.shader->GetPaddedSize();
	materials += paddedOffset;

	bool updated = !drawSurf->initialized[stage] || pStage->colorDynamic || pStage->texMatricesDynamic || pStage->dynamic;
	if ( !updated ) {
		return;
	}
	drawSurf->initialized[stage] = true;

	float fogDensity = RB_EvalExpression( &pStage->fogDensityExp, 0.001 );
	vec4_t fogColor;
	Tess_ComputeColor( pStage );
	VectorCopy( tess.svars.color.ToArray(), fogColor );

	gl_liquidShaderMaterial->SetUniform_RefractionIndex( RB_EvalExpression( &pStage->refractionIndexExp, 1.0 ) );
	gl_liquidShaderMaterial->SetUniform_FresnelPower( RB_EvalExpression( &pStage->fresnelPowerExp, 2.0 ) );
	gl_liquidShaderMaterial->SetUniform_FresnelScale( RB_EvalExpression( &pStage->fresnelScaleExp, 1.0 ) );
	gl_liquidShaderMaterial->SetUniform_FresnelBias( RB_EvalExpression( &pStage->fresnelBiasExp, 0.05 ) );
	gl_liquidShaderMaterial->SetUniform_FogDensity( fogDensity );
	gl_liquidShaderMaterial->SetUniform_FogColor( fogColor );

	gl_liquidShaderMaterial->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );
	gl_liquidShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_liquidShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );

	// NOTE: specular component is computed by shader.
	// FIXME: physical mapping is not implemented.
	if ( pStage->enableSpecularMapping ) {
		float specMin = RB_EvalExpression( &pStage->specularExponentMin, r_specularExponentMin->value );
		float specMax = RB_EvalExpression( &pStage->specularExponentMax, r_specularExponentMax->value );
		gl_liquidShaderMaterial->SetUniform_SpecularExponent( specMin, specMax );
	}

	// bind u_CurrentMap
	gl_liquidShaderMaterial->SetUniform_CurrentMapBindless( GL_BindToTMU( 0, tr.currentRenderImage[backEnd.currentMainFBO] ) );

	// bind u_PortalMap
	gl_liquidShaderMaterial->SetUniform_PortalMapBindless( GL_BindToTMU( 1, tr.portalRenderImage ) );

	// depth texture
	gl_liquidShaderMaterial->SetUniform_DepthMapBindless( GL_BindToTMU( 2, tr.currentDepthImage ) );

	// bind u_HeightMap u_depthScale u_reliefOffsetBias
	if ( pStage->enableReliefMapping ) {
		float depthScale;
		float reliefDepthScale;

		depthScale = RB_EvalExpression( &pStage->depthScaleExp, r_reliefDepthScale->value );
		reliefDepthScale = tess.surfaceShader->reliefDepthScale;
		depthScale *= reliefDepthScale == 0 ? 1 : reliefDepthScale;
		gl_liquidShaderMaterial->SetUniform_ReliefDepthScale( depthScale );
		gl_liquidShaderMaterial->SetUniform_ReliefOffsetBias( tess.surfaceShader->reliefOffsetBias );

		// FIXME: if there is both, embedded heightmap in normalmap is used instead of standalone heightmap
		if ( !pStage->hasHeightMapInNormalMap ) {
			gl_liquidShaderMaterial->SetUniform_HeightMapBindless( GL_BindToTMU( 15, pStage->bundle[TB_HEIGHTMAP].image[0] ) );
		}
	}

	// bind u_NormalMap
	gl_liquidShaderMaterial->SetUniform_NormalMapBindless( GL_BindToTMU( 3, pStage->bundle[TB_NORMALMAP].image[0] ) );

	// bind u_NormalScale
	if ( pStage->enableNormalMapping ) {
		vec3_t normalScale;
		// FIXME: NormalIntensity default was 0.5
		SetNormalScale( pStage, normalScale );

		gl_liquidShaderMaterial->SetUniform_NormalScale( normalScale );
	}

	gl_liquidShaderMaterial->WriteUniformsToBuffer( materials );
}

/*
* Buffer layout:
* // Static surfaces data:
* // Material0
* // Surface/stage0_0:
* uniform0_0
* uniform0_1
* ..
* uniform0_x
* optional_struct_padding
* // Surface/stage0_1:
* ..
* // Surface/stage0_y:
* uniform0_0
* uniform0_1
* ..
* uniform0_x
* optional_struct_padding
* optional_material1_padding
* // Material1
* // Surface/stage1_0:
* ..
* // Surface/stage1_y:
* ..
* ..
* // Materialz:
* ..
* ..
* // Dynamic surfaces data:
* // Same as the static layout
*/
// Buffer is separated into static and dynamic parts so we can just update the whole dynamic range at once
// This will generate the actual buffer with per-stage values AFTER materials are generated
void MaterialSystem::GenerateWorldMaterialsBuffer() {
	Log::Debug( "Generating materials buffer" );

	uint32_t offset = 0;

	materialsSSBO.BindBuffer();

	// Compute data size for static surfaces
	for ( MaterialPack& pack : materialPacks ) {
		for ( Material& material : pack.materials ) {
			// Any new material in the buffer must start on an offset that is an integer multiple of
			// the padded size of the material struct
			const uint32_t paddedSize = material.shader->GetPaddedSize();
			const uint32_t padding = ( offset % paddedSize == 0 ) ? 0 : paddedSize - ( offset % paddedSize );

			offset += padding;
			material.staticMaterialsSSBOOffset = offset;
			offset += paddedSize * material.totalStaticDrawSurfCount;
		}
	}

	dynamicDrawSurfsOffset = offset;

	// Compute data size for dynamic surfaces
	for ( MaterialPack& pack : materialPacks ) {
		for ( Material& material : pack.materials ) {
			// Any new material in the buffer must start on an offset that is an integer multiple of
			// the padded size of the material struct
			const uint32_t paddedSize = material.shader->GetPaddedSize();
			const uint32_t padding = ( offset % paddedSize == 0 ) ? 0 : paddedSize - ( offset % paddedSize );

			offset += padding;
			material.dynamicMaterialsSSBOOffset = offset;
			offset += paddedSize * material.totalDynamicDrawSurfCount;
		}
	}

	dynamicDrawSurfsSize = offset - dynamicDrawSurfsOffset;

	// 4 bytes per component
	glBufferData( GL_SHADER_STORAGE_BUFFER, offset * sizeof( uint32_t ), nullptr, GL_DYNAMIC_DRAW );
	uint32_t* materialsData = materialsSSBO.MapBufferRange( offset );
	memset( materialsData, 0, offset * sizeof( uint32_t ) );

	for ( uint32_t materialPackID = 0; materialPackID < 4; materialPackID++ ) {
		for ( Material& material : materialPacks[materialPackID].materials ) {

			for ( drawSurf_t* drawSurf : material.drawSurfs ) {
				bool hasDynamicStages = false;

				uint32_t stage = 0;
				for ( shaderStage_t* pStage = drawSurf->shader->stages; pStage < drawSurf->shader->lastStage; pStage++ ) {
					if ( drawSurf->materialIDs[stage] != material.id || drawSurf->materialPackIDs[stage] != materialPackID ) {
						stage++;
						continue;
					}
					
					uint32_t SSBOOffset = 0;
					uint32_t drawSurfCount = 0;
					if ( pStage->dynamic ) {
						SSBOOffset = material.dynamicMaterialsSSBOOffset;
						drawSurfCount = material.currentDynamicDrawSurfCount;
						material.currentDynamicDrawSurfCount++;
					} else {
						SSBOOffset = material.staticMaterialsSSBOOffset;
						drawSurfCount = material.currentStaticDrawSurfCount;
						material.currentStaticDrawSurfCount++;
					}

					drawSurf->materialsSSBOOffset[stage] = ( SSBOOffset + drawSurfCount * material.shader->GetPaddedSize() ) /
						material.shader->GetPaddedSize();

					if ( pStage->dynamic ) {
						hasDynamicStages = true;
					}

					AddStageTextures( drawSurf, pStage, &material );

					pStage->surfaceDataUpdater( materialsData, material, drawSurf, stage );

					tess.currentDrawSurf = drawSurf;

					tess.currentSSBOOffset = tess.currentDrawSurf->materialsSSBOOffset[stage];
					tess.materialID = tess.currentDrawSurf->materialIDs[stage];
					tess.materialPackID = tess.currentDrawSurf->materialPackIDs[stage];

					Tess_Begin( Tess_StageIteratorDummy, nullptr, nullptr, false, -1, 0 );
					rb_surfaceTable[Util::ordinal( *drawSurf->surface )]( drawSurf->surface );
					pStage->colorRenderer( pStage );
					Tess_Clear();

					drawSurf->drawCommandIDs[stage] = lastCommandID;

					if ( pStage->dynamic ) {
						drawSurf->materialsSSBOOffset[stage] = ( SSBOOffset - dynamicDrawSurfsOffset + drawSurfCount *
							material.shader->GetPaddedSize() ) / material.shader->GetPaddedSize();
					}

					stage++;
				}

				if ( hasDynamicStages ) {
					// We need a copy here because the memory pointed to by drawSurf will change later
					// We'll probably need a separate buffer for entities other than world entity + ensure we don't store a drawSurf with
					// invalid pointers
					dynamicDrawSurfs.emplace_back( *drawSurf );
				}
			}
		}
	}

	materialsSSBO.UnmapBuffer();
}

// This generates the buffer GLIndirect commands
void MaterialSystem::GenerateWorldCommandBuffer() {
	Log::Debug( "Generating world command buffer" );

	totalBatchCount = 0;

	uint32_t batchOffset = 0;
	uint32_t globalID = 0;
	for ( MaterialPack& pack : materialPacks ) {
		for ( Material& material : pack.materials ) {
			material.surfaceCommandBatchOffset = batchOffset;

			const uint32_t cmdCount = material.drawCommands.size();
			const uint32_t batchCount = cmdCount % SURFACE_COMMANDS_PER_BATCH == 0 ? cmdCount / SURFACE_COMMANDS_PER_BATCH
				: cmdCount / SURFACE_COMMANDS_PER_BATCH + 1;

			material.surfaceCommandBatchOffset = batchOffset;
			material.surfaceCommandBatchCount = batchCount;

			batchOffset += batchCount;
			material.globalID = globalID;

			totalBatchCount += batchCount;
			globalID++;
		}
	}

	Log::Debug( "Total batch count: %u", totalBatchCount );

	drawSurf_t* drawSurf;

	surfaceDescriptorsSSBO.BindBuffer();
	surfaceDescriptorsCount = totalDrawSurfs;
	descriptorSize = BOUNDING_SPHERE_SIZE + maxStages;
	glBufferData( GL_SHADER_STORAGE_BUFFER, surfaceDescriptorsCount * descriptorSize * sizeof( uint32_t ),
				  nullptr, GL_STATIC_DRAW );
	uint32_t* surfaceDescriptors = surfaceDescriptorsSSBO.MapBufferRange( surfaceDescriptorsCount * descriptorSize );

	surfaceCommandsCount = totalBatchCount * SURFACE_COMMANDS_PER_BATCH;

	surfaceCommandsSSBO.BindBuffer();
	surfaceCommandsSSBO.BufferStorage( surfaceCommandsCount * SURFACE_COMMAND_SIZE * MAX_VIEWFRAMES, 1, nullptr );
	surfaceCommandsSSBO.MapAll();
	SurfaceCommand* surfaceCommands = ( SurfaceCommand* ) surfaceCommandsSSBO.GetData();
	memset( surfaceCommands, 0, surfaceCommandsCount * sizeof( SurfaceCommand ) * MAX_VIEWFRAMES );

	culledCommandsBuffer.BindBuffer( GL_SHADER_STORAGE_BUFFER );
	culledCommandsBuffer.BufferStorage( GL_SHADER_STORAGE_BUFFER,
		surfaceCommandsCount * INDIRECT_COMMAND_SIZE * MAX_VIEWFRAMES, 1, nullptr );
	culledCommandsBuffer.MapAll( GL_SHADER_STORAGE_BUFFER );
	GLIndirectBuffer::GLIndirectCommand* culledCommands = ( GLIndirectBuffer::GLIndirectCommand* ) culledCommandsBuffer.GetData();
	memset( culledCommands, 0, surfaceCommandsCount * sizeof( GLIndirectBuffer::GLIndirectCommand ) * MAX_VIEWFRAMES );
	culledCommandsBuffer.FlushAll( GL_SHADER_STORAGE_BUFFER );

	surfaceBatchesUBO.BindBuffer();
	glBufferData( GL_UNIFORM_BUFFER, MAX_SURFACE_COMMAND_BATCHES * sizeof( SurfaceCommandBatch ), nullptr, GL_STATIC_DRAW );
	SurfaceCommandBatch* surfaceCommandBatches =
		( SurfaceCommandBatch* ) surfaceBatchesUBO.MapBufferRange( MAX_SURFACE_COMMAND_BATCHES * SURFACE_COMMAND_BATCH_SIZE );

	// memset( (void*) surfaceCommandBatches, 0, MAX_SURFACE_COMMAND_BATCHES * sizeof( SurfaceCommandBatch ) );
	// Fuck off gcc
	for ( int i = 0; i < MAX_SURFACE_COMMAND_BATCHES; i++ ) {
		surfaceCommandBatches[i] = {};
	}

	uint32_t id = 0;
	uint32_t matID = 0;
	for ( MaterialPack& pack : materialPacks ) {
		for ( Material& mat : pack.materials ) {
			for ( uint32_t i = 0; i < mat.surfaceCommandBatchCount; i++ ) {
				surfaceCommandBatches[id].materialIDs[0] = matID;
				surfaceCommandBatches[id].materialIDs[1] = mat.surfaceCommandBatchOffset;
				id++;
			}
			matID++;
		}
	}

	atomicCommandCountersBuffer.BindBuffer( GL_ATOMIC_COUNTER_BUFFER );
	atomicCommandCountersBuffer.BufferStorage( GL_ATOMIC_COUNTER_BUFFER,
		MAX_COMMAND_COUNTERS * MAX_VIEWS, MAX_FRAMES, nullptr );
	atomicCommandCountersBuffer.MapAll( GL_ATOMIC_COUNTER_BUFFER );
	uint32_t* atomicCommandCounters = (uint32_t*) atomicCommandCountersBuffer.GetData();
	memset( atomicCommandCounters, 0, MAX_COMMAND_COUNTERS * MAX_VIEWFRAMES * sizeof(uint32_t) );

	for ( int i = 0; i < tr.refdef.numDrawSurfs; i++ ) {
		drawSurf = &tr.refdef.drawSurfs[i];
		if ( drawSurf->entity != &tr.worldEntity ) {
			continue;
		}

		shader_t* shader = drawSurf->shader;
		if ( !shader ) {
			continue;
		}

		shader = shader->remappedShader ? shader->remappedShader : shader;
		if ( shader->isPortal ) {
			continue;
		}

		// Don't add SF_SKIP surfaces
		if ( *drawSurf->surface == surfaceType_t::SF_SKIP ) {
			continue;
		}

		// Depth prepass surfaces are added as stages to the main surface instead
		if ( drawSurf->materialSystemSkip ) {
			continue;
		}

		SurfaceDescriptor surface;
		VectorCopy( ( ( srfGeneric_t* ) drawSurf->surface )->origin, surface.boundingSphere.origin );
		surface.boundingSphere.radius = ( ( srfGeneric_t* ) drawSurf->surface )->radius;

		const bool depthPrePass = drawSurf->depthSurface != nullptr;

		if ( depthPrePass ) {
			const drawSurf_t* depthDrawSurf = drawSurf->depthSurface;
			const Material* material = &materialPacks[depthDrawSurf->materialPackIDs[0]].materials[depthDrawSurf->materialIDs[0]];
			uint cmdID = material->surfaceCommandBatchOffset * SURFACE_COMMANDS_PER_BATCH + depthDrawSurf->drawCommandIDs[0];
			// Add 1 because cmd 0 == no-command
			surface.surfaceCommandIDs[0] = cmdID + 1;

			SurfaceCommand surfaceCommand;
			surfaceCommand.enabled = 0;
			surfaceCommand.drawCommand = material->drawCommands[depthDrawSurf->drawCommandIDs[0]].cmd;
			surfaceCommands[cmdID] = surfaceCommand;
		}

		uint32_t stage = 0;
		for ( shaderStage_t* pStage = drawSurf->shader->stages; pStage < drawSurf->shader->lastStage; pStage++ ) {
			const Material* material = &materialPacks[drawSurf->materialPackIDs[stage]].materials[drawSurf->materialIDs[stage]];
			uint32_t cmdID = material->surfaceCommandBatchOffset * SURFACE_COMMANDS_PER_BATCH + drawSurf->drawCommandIDs[stage];
			// Add 1 because cmd 0 == no-command
			surface.surfaceCommandIDs[stage + ( depthPrePass ? 1 : 0 )] = cmdID + 1;

			SurfaceCommand surfaceCommand;
			surfaceCommand.enabled = 0;
			surfaceCommand.drawCommand = material->drawCommands[drawSurf->drawCommandIDs[stage]].cmd;
			surfaceCommands[cmdID] = surfaceCommand;

			stage++;
		}
		memcpy( surfaceDescriptors, &surface, descriptorSize * sizeof( uint32_t ) );
		surfaceDescriptors += descriptorSize;
	}

	for ( int i = 0; i < MAX_VIEWFRAMES; i++ ) {
		memcpy( surfaceCommands + surfaceCommandsCount * i, surfaceCommands, surfaceCommandsCount * sizeof( SurfaceCommand ) );
	}

	surfaceDescriptorsSSBO.BindBuffer();
	surfaceDescriptorsSSBO.UnmapBuffer();

	surfaceCommandsSSBO.BindBuffer();
	surfaceCommandsSSBO.UnmapBuffer();

	culledCommandsBuffer.BindBuffer( GL_SHADER_STORAGE_BUFFER );
	culledCommandsBuffer.UnmapBuffer();

	atomicCommandCountersBuffer.BindBuffer( GL_ATOMIC_COUNTER_BUFFER);
	atomicCommandCountersBuffer.UnmapBuffer();

	surfaceBatchesUBO.BindBuffer();
	surfaceBatchesUBO.UnmapBuffer();

	GL_CheckErrors();
}

void MaterialSystem::GenerateDepthImages( const int width, const int height, imageParams_t imageParms ) {
	int size = std::max( width, height );
	imageParms.bits ^= ( IF_NOPICMIP | IF_PACKED_DEPTH24_STENCIL8 );
	imageParms.bits |= IF_ONECOMP32F;

	depthImageLevels = 0;
	while ( size > 0 ) {
		depthImageLevels++;
		size >>= 1; // mipmaps round down
	}

	depthImage = R_CreateImage( "_depthImage", nullptr, width, height, depthImageLevels, imageParms );
	GL_Bind( depthImage );
	int mipmapWidth = width;
	int mipmapHeight = height;
	for ( int j = 0; j < depthImageLevels; j++ ) {
		glTexImage2D( GL_TEXTURE_2D, j, GL_R32F, mipmapWidth, mipmapHeight, 0, GL_RED, GL_FLOAT, nullptr );
		mipmapWidth = mipmapWidth > 1 ? mipmapWidth >> 1 : 1;
		mipmapHeight = mipmapHeight > 1 ? mipmapHeight >> 1 : 1;
	}
}

void BindShaderNONE( Material* ) {
	ASSERT_UNREACHABLE();
}

void BindShaderNOP( Material* ) {
}

void BindShaderGeneric3D( Material* material ) {
	// Select shader permutation.
	gl_genericShaderMaterial->SetVertexAnimation( material->vertexAnimation );
	gl_genericShaderMaterial->SetTCGenEnvironment( material->tcGenEnvironment );
	gl_genericShaderMaterial->SetTCGenLightmap( material->tcGen_Lightmap );
	gl_genericShaderMaterial->SetDepthFade( material->hasDepthFade );
	gl_genericShaderMaterial->SetVertexSprite( material->vertexSprite );

	// Bind shader program.
	gl_genericShaderMaterial->BindProgram( material->deformIndex );

	// Set shader uniforms.
	if ( material->tcGenEnvironment || material->vertexSprite ) {
		gl_genericShaderMaterial->SetUniform_ViewOrigin( backEnd.orientation.viewOrigin );
		gl_genericShaderMaterial->SetUniform_ViewUp( backEnd.orientation.axis[2] );
	}

	gl_genericShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_genericShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
}

void BindShaderLightMapping( Material* material ) {
	// Select shader permutation.
	gl_lightMappingShaderMaterial->SetVertexAnimation( material->vertexAnimation );
	gl_lightMappingShaderMaterial->SetBspSurface( material->bspSurface );
	gl_lightMappingShaderMaterial->SetDeluxeMapping( material->enableDeluxeMapping );
	gl_lightMappingShaderMaterial->SetGridLighting( material->enableGridLighting );
	gl_lightMappingShaderMaterial->SetGridDeluxeMapping( material->enableGridDeluxeMapping );
	gl_lightMappingShaderMaterial->SetHeightMapInNormalMap( material->hasHeightMapInNormalMap );
	gl_lightMappingShaderMaterial->SetReliefMapping( material->enableReliefMapping );
	gl_lightMappingShaderMaterial->SetReflectiveSpecular( material->enableNormalMapping && tr.cubeHashTable != nullptr );
	gl_lightMappingShaderMaterial->SetPhysicalShading( material->enablePhysicalMapping );

	// Bind shader program.
	gl_lightMappingShaderMaterial->BindProgram( material->deformIndex );

	// Set shader uniforms.
	if ( tr.world ) {
		gl_lightMappingShaderMaterial->SetUniform_LightGridOrigin( tr.world->lightGridGLOrigin );
		gl_lightMappingShaderMaterial->SetUniform_LightGridScale( tr.world->lightGridGLScale );
	}
	// FIXME: else

	gl_lightMappingShaderMaterial->SetUniform_ViewOrigin( backEnd.orientation.viewOrigin );
	gl_lightMappingShaderMaterial->SetUniform_numLights( backEnd.refdef.numLights );
	gl_lightMappingShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_lightMappingShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
}

void BindShaderReflection( Material* material ) {
	// Select shader permutation.
	gl_reflectionShaderMaterial->SetHeightMapInNormalMap( material->hasHeightMapInNormalMap );
	gl_reflectionShaderMaterial->SetReliefMapping( material->enableReliefMapping );
	gl_reflectionShaderMaterial->SetVertexAnimation( material->vertexAnimation );

	// Bind shader program.
	gl_reflectionShaderMaterial->BindProgram( material->deformIndex );

	// Set shader uniforms.
	gl_reflectionShaderMaterial->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );
	gl_reflectionShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_reflectionShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
}

void BindShaderSkybox( Material* material ) {
	// Bind shader program.
	gl_skyboxShaderMaterial->BindProgram( material->deformIndex );

	// Set shader uniforms.
	gl_skyboxShaderMaterial->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );
	gl_skyboxShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_skyboxShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
}

void BindShaderScreen( Material* material ) {
	// Bind shader program.
	gl_screenShaderMaterial->BindProgram( material->deformIndex );

	// Set shader uniforms.
	gl_screenShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
}

void BindShaderHeatHaze( Material* material ) {
	// Select shader permutation.
	gl_heatHazeShaderMaterial->SetVertexAnimation( material->vertexAnimation );
	gl_heatHazeShaderMaterial->SetVertexSprite( material->vertexSprite );

	// Bind shader program.
	gl_heatHazeShaderMaterial->BindProgram( material->deformIndex );

	// Set shader uniforms.
	if ( material->vertexSprite ) {
		gl_heatHazeShaderMaterial->SetUniform_ViewOrigin( backEnd.orientation.viewOrigin );
		gl_heatHazeShaderMaterial->SetUniform_ViewUp( backEnd.orientation.axis[2] );
	}

	gl_heatHazeShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_heatHazeShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
}

void BindShaderLiquid( Material* material ) {
	// Select shader permutation.
	gl_liquidShaderMaterial->SetHeightMapInNormalMap( material->hasHeightMapInNormalMap );
	gl_liquidShaderMaterial->SetReliefMapping( material->enableReliefMapping );

	// Bind shader program.
	gl_liquidShaderMaterial->BindProgram( material->deformIndex );

	// Set shader uniforms.
	gl_liquidShaderMaterial->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );
	gl_liquidShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_liquidShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
}

void ProcessMaterialNONE( Material*, shaderStage_t*, drawSurf_t* ) {
	ASSERT_UNREACHABLE();
}

void ProcessMaterialNOP( Material*, shaderStage_t*, drawSurf_t* ) {
}

// ProcessMaterial*() are essentially same as BindShader*(), but only set the GL program id to the material,
// without actually binding it
void ProcessMaterialGeneric3D( Material* material, shaderStage_t* pStage, drawSurf_t* drawSurf ) {
	shader_t* shader = drawSurf->shader;

	material->shader = gl_genericShaderMaterial;

	material->vertexAnimation = false;
	material->tcGenEnvironment = pStage->tcGen_Environment;
	material->tcGen_Lightmap = pStage->tcGen_Lightmap;
	material->vertexSprite = shader->autoSpriteMode != 0;
	material->deformIndex = pStage->deformIndex;

	gl_genericShaderMaterial->SetVertexAnimation( false );

	gl_genericShaderMaterial->SetTCGenEnvironment( pStage->tcGen_Environment );
	gl_genericShaderMaterial->SetTCGenLightmap( pStage->tcGen_Lightmap );

	bool hasDepthFade = pStage->hasDepthFade && !shader->autoSpriteMode;
	material->hasDepthFade = hasDepthFade;
	gl_genericShaderMaterial->SetDepthFade( hasDepthFade );
	gl_genericShaderMaterial->SetVertexSprite( shader->autoSpriteMode != 0 );

	material->program = gl_genericShaderMaterial->GetProgram( pStage->deformIndex );
}

void ProcessMaterialLightMapping( Material* material, shaderStage_t* pStage, drawSurf_t* drawSurf ) {
	material->shader = gl_lightMappingShaderMaterial;

	material->vertexAnimation = false;
	material->bspSurface = false;

	gl_lightMappingShaderMaterial->SetVertexAnimation( false );
	gl_lightMappingShaderMaterial->SetBspSurface( drawSurf->bspSurface );

	lightMode_t lightMode;
	deluxeMode_t deluxeMode;
	SetLightDeluxeMode( drawSurf, pStage->type, lightMode, deluxeMode );

	bool enableDeluxeMapping = ( deluxeMode == deluxeMode_t::MAP );
	bool enableGridLighting = ( lightMode == lightMode_t::GRID );
	bool enableGridDeluxeMapping = ( deluxeMode == deluxeMode_t::GRID );

	DAEMON_ASSERT( !( enableDeluxeMapping && enableGridDeluxeMapping ) );

	material->enableDeluxeMapping = enableDeluxeMapping;
	material->enableGridLighting = enableGridLighting;
	material->enableGridDeluxeMapping = enableGridDeluxeMapping;
	material->hasHeightMapInNormalMap = pStage->hasHeightMapInNormalMap;
	material->enableReliefMapping = pStage->enableReliefMapping;
	material->enableNormalMapping = pStage->enableNormalMapping && tr.cubeHashTable != nullptr;
	material->enablePhysicalMapping = pStage->enablePhysicalMapping;
	material->deformIndex = pStage->deformIndex;

	gl_lightMappingShaderMaterial->SetDeluxeMapping( enableDeluxeMapping );

	gl_lightMappingShaderMaterial->SetGridLighting( enableGridLighting );

	gl_lightMappingShaderMaterial->SetGridDeluxeMapping( enableGridDeluxeMapping );

	gl_lightMappingShaderMaterial->SetHeightMapInNormalMap( pStage->hasHeightMapInNormalMap );

	gl_lightMappingShaderMaterial->SetReliefMapping( pStage->enableReliefMapping );

	gl_lightMappingShaderMaterial->SetReflectiveSpecular( pStage->enableNormalMapping && tr.cubeHashTable != nullptr );

	gl_lightMappingShaderMaterial->SetPhysicalShading( pStage->enablePhysicalMapping );

	material->program = gl_lightMappingShaderMaterial->GetProgram( pStage->deformIndex );
}

void ProcessMaterialReflection( Material* material, shaderStage_t* pStage, drawSurf_t* /* drawSurf */ ) {
	material->shader = gl_reflectionShaderMaterial;

	material->hasHeightMapInNormalMap = pStage->hasHeightMapInNormalMap;
	material->enableReliefMapping = pStage->enableReliefMapping;
	material->vertexAnimation = false;
	material->deformIndex = pStage->deformIndex;

	gl_reflectionShaderMaterial->SetHeightMapInNormalMap( pStage->hasHeightMapInNormalMap );

	gl_reflectionShaderMaterial->SetReliefMapping( pStage->enableReliefMapping );

	gl_reflectionShaderMaterial->SetVertexAnimation( false );

	material->program = gl_reflectionShaderMaterial->GetProgram( pStage->deformIndex );
}

void ProcessMaterialSkybox( Material* material, shaderStage_t* pStage, drawSurf_t* /* drawSurf */ ) {
	material->shader = gl_skyboxShaderMaterial;

	material->deformIndex = pStage->deformIndex;

	material->program = gl_skyboxShaderMaterial->GetProgram( pStage->deformIndex );
}

void ProcessMaterialScreen( Material* material, shaderStage_t* pStage, drawSurf_t* /* drawSurf */ ) {
	material->shader = gl_screenShaderMaterial;

	material->deformIndex = pStage->deformIndex;

	material->program = gl_screenShaderMaterial->GetProgram( pStage->deformIndex );
}

void ProcessMaterialHeatHaze( Material* material, shaderStage_t* pStage, drawSurf_t* drawSurf ) {
	shader_t* shader = drawSurf->shader;

	material->shader = gl_heatHazeShaderMaterial;

	material->vertexAnimation = false;
	material->deformIndex = pStage->deformIndex;

	gl_heatHazeShaderMaterial->SetVertexAnimation( false );
	if ( shader->autoSpriteMode ) {
		gl_heatHazeShaderMaterial->SetVertexSprite( true );
	} else {
		gl_heatHazeShaderMaterial->SetVertexSprite( false );
	}

	material->program = gl_heatHazeShaderMaterial->GetProgram( pStage->deformIndex );
}

void ProcessMaterialLiquid( Material* material, shaderStage_t* pStage, drawSurf_t* /* drawSurf */ ) {
	material->shader = gl_liquidShaderMaterial;

	material->hasHeightMapInNormalMap = pStage->hasHeightMapInNormalMap;
	material->enableReliefMapping = pStage->enableReliefMapping;
	material->deformIndex = pStage->deformIndex;

	gl_liquidShaderMaterial->SetHeightMapInNormalMap( pStage->hasHeightMapInNormalMap );

	gl_liquidShaderMaterial->SetReliefMapping( pStage->enableReliefMapping );

	material->program = gl_liquidShaderMaterial->GetProgram( pStage->deformIndex );
}

void MaterialSystem::ProcessStage( drawSurf_t* drawSurf, shaderStage_t* pStage, shader_t* shader, uint32_t* packIDs, uint32_t& stage,
	uint32_t& previousMaterialID ) {
	Material material;

	uint32_t materialPack = 0;
	if ( shader->sort == Util::ordinal( shaderSort_t::SS_DEPTH ) ) {
		materialPack = 0;
	} else if ( shader->sort >= Util::ordinal( shaderSort_t::SS_ENVIRONMENT_FOG )
		&& shader->sort <= Util::ordinal( shaderSort_t::SS_OPAQUE ) ) {
		materialPack = 1;
	} else {
		materialPack = 2;
	}
	if ( shader->isSky ) {
		materialPack = 3;
	}
	uint32_t id = packIDs[materialPack];

	// In surfaces with multiple stages each consecutive stage must be drawn after the previous stage,
	// except if an opaque stage follows a transparent stage etc.
	if ( stage > 0 ) {
		material.useSync = true;
		material.syncMaterial = previousMaterialID;
	}

	material.stateBits = pStage->stateBits;
	// GLS_ATEST_BITS don't matter here as they don't change GL state
	material.stateBits &= GLS_DEPTHFUNC_BITS | GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE
		| GLS_COLORMASK_BITS | GLS_DEPTHMASK_TRUE;
	material.shaderBinder = pStage->shaderBinder;
	material.cullType = shader->cullType;
	material.usePolygonOffset = shader->polygonOffset;

	material.vbo = glState.currentVBO;
	material.ibo = glState.currentIBO;

	ComputeDynamics( pStage );

	if ( pStage->texturesDynamic ) {
		drawSurf->texturesDynamic[stage] = true;
	}

	if ( shader->isSky ) {
		if ( std::find( skyShaders.begin(), skyShaders.end(), shader ) == skyShaders.end() ) {
			skyShaders.emplace_back( shader );
		}

		material.skyShader = shader;
	}

	pStage->materialProcessor( &material, pStage, drawSurf );

	std::vector<Material>& materials = materialPacks[materialPack].materials;
	std::vector<Material>::iterator currentSearchIt = materials.begin();
	std::vector<Material>::iterator materialIt;
	// Look for this material in the ones we already have
	while ( true ) {
		materialIt = std::find( currentSearchIt, materials.end(), material );
		if ( materialIt == materials.end() ) {
			break;
		}
		if ( material.useSync && materialIt->id < material.syncMaterial ) {
			currentSearchIt = materialIt + 1;
		} else {
			break;
		}
	}

	// Add it at the back if not found
	if ( materialIt == materials.end() ) {
		material.id = id;
		previousMaterialID = id;
		materials.emplace_back( material );
		id++;
	} else {
		previousMaterialID = materialIt->id;
	}

	pStage->useMaterialSystem = true;
	materials[previousMaterialID].totalDrawSurfCount++;
	if ( pStage->dynamic ) {
		materials[previousMaterialID].totalDynamicDrawSurfCount++;
	} else {
		materials[previousMaterialID].totalStaticDrawSurfCount++;
	}

	if ( std::find( materials[previousMaterialID].drawSurfs.begin(), materials[previousMaterialID].drawSurfs.end(), drawSurf )
		== materials[previousMaterialID].drawSurfs.end() ) {
		materials[previousMaterialID].drawSurfs.emplace_back( drawSurf );
	}

	drawSurf->materialIDs[stage] = previousMaterialID;
	drawSurf->materialPackIDs[stage] = materialPack;

	packIDs[materialPack] = id;

	stage++;
}

/* This will only generate the materials themselves
*  A material represents a distinct global OpenGL state (e. g. blend function, depth test, depth write etc.)
*  Materials can have a dependency on other materials to make sure that consecutive stages are rendered in the proper order */
void MaterialSystem::GenerateWorldMaterials() {
	const int current_r_nocull = r_nocull->integer;
	const int current_r_drawworld = r_drawworld->integer;
	r_nocull->integer = 1;
	r_drawworld->integer = 1;
	generatingWorldCommandBuffer = true;

	Log::Debug( "Generating world materials" );

	R_SyncRenderThread();

	++tr.viewCountNoReset;
	R_AddWorldSurfaces();

	Log::Notice( "World bounds: min: %f %f %f max: %f %f %f", tr.viewParms.visBounds[0][0], tr.viewParms.visBounds[0][1],
		tr.viewParms.visBounds[0][2], tr.viewParms.visBounds[1][0], tr.viewParms.visBounds[1][1], tr.viewParms.visBounds[1][2] );
	VectorCopy( tr.viewParms.visBounds[0], worldViewBounds[0] );
	VectorCopy( tr.viewParms.visBounds[1], worldViewBounds[1] );

	backEnd.currentEntity = &tr.worldEntity;

	drawSurf_t* drawSurf;
	totalDrawSurfs = 0;
	uint32_t packIDs[4] = { 0, 0, 0, 0 };

	for ( int i = 0; i < tr.refdef.numDrawSurfs; i++ ) {
		drawSurf = &tr.refdef.drawSurfs[i];
		if ( drawSurf->entity != &tr.worldEntity ) {
			continue;
		}

		shader_t* shader = drawSurf->shader;
		if ( !shader ) {
			continue;
		}

		shader = shader->remappedShader ? shader->remappedShader : shader;
		if ( shader->isPortal ) {
			continue;
		}

		// Don't add SF_SKIP surfaces
		if ( *drawSurf->surface == surfaceType_t::SF_SKIP ) {
			continue;
		}

		// The verts aren't used; it's only to get the VBO/IBO.
		Tess_Begin( Tess_StageIteratorDummy, shader, nullptr, true, -1, 0 );
		rb_surfaceTable[Util::ordinal( *( drawSurf->surface ) )]( drawSurf->surface );
		Tess_Clear();

		// Only add the main surface for surfaces with depth pre-pass to the total count
		if ( !drawSurf->materialSystemSkip ) {
			totalDrawSurfs++;
		}

		uint32_t stage = 0;
		uint32_t previousMaterialID = 0;
		for ( shaderStage_t* pStage = drawSurf->shader->stages; pStage < drawSurf->shader->lastStage; pStage++ ) {
			ProcessStage( drawSurf, pStage, shader, packIDs, stage, previousMaterialID );
		}
	}

	GenerateWorldMaterialsBuffer();

	uint32_t totalCount = 0;
	for ( MaterialPack& pack : materialPacks ) {
		totalCount += pack.materials.size();
	}
	Log::Notice( "Generated %u materials from %u surfaces", totalCount, tr.refdef.numDrawSurfs );
	/* for ( const MaterialPack& materialPack : materialPacks ) {
		Log::Notice( "materialPack sort: %i %i", Util::ordinal( materialPack.fromSort ), Util::ordinal( materialPack.toSort ) );
		for ( const Material& material : materialPack.materials ) {
			Log::Notice( "id: %u, useSync: %b, sync: %u, program: %i, stateBits: %u, totalDrawSurfCount: %u, shader: %s, vbo: %s, ibo: %s"
				", staticDrawSurfs: %u, dynamicDrawSurfs: %u, culling: %i",
				material.id, material.useSync, material.syncMaterial, material.program, material.stateBits, material.totalDrawSurfCount,
				material.shader->GetName(), material.vbo->name, material.ibo->name, material.currentStaticDrawSurfCount,
				material.currentDynamicDrawSurfCount, material.cullType );
		}
	} */

	r_nocull->integer = current_r_nocull;
	r_drawworld->integer = current_r_drawworld;
	AddAllWorldSurfaces();

	GeneratePortalBoundingSpheres();

	generatedWorldCommandBuffer = true;
}

void MaterialSystem::AddAllWorldSurfaces() {
	GenerateWorldCommandBuffer();

	generatingWorldCommandBuffer = false;
}

void MaterialSystem::AddStageTextures( drawSurf_t* drawSurf, shaderStage_t* pStage, Material* material ) {
	for ( const textureBundle_t& bundle : pStage->bundle ) {
		if ( bundle.isVideoMap ) {
			material->AddTexture( tr.cinematicImage[bundle.videoMapHandle]->texture );
			continue;
		}

		for ( image_t* image : bundle.image ) {
			if ( image ) {
				material->AddTexture( image->texture );
			}
		}
	}

	// Add lightmap and deluxemap for this surface to the material as well

	lightMode_t lightMode;
	deluxeMode_t deluxeMode;
	SetLightDeluxeMode( drawSurf, pStage->type, lightMode, deluxeMode );

	// u_Map, u_DeluxeMap
	image_t* lightmap = SetLightMap( drawSurf, lightMode );
	image_t* deluxemap = SetDeluxeMap( drawSurf, deluxeMode );

	material->AddTexture( lightmap->texture );
	material->AddTexture( deluxemap->texture );

	if ( glConfig2.realtimeLighting ) {
		if ( r_realtimeLightingRenderer.Get() == Util::ordinal( realtimeLightingRenderer_t::TILED ) ) {
			material->AddTexture( tr.lighttileRenderImage->texture );
		}
	}
}

// Dynamic surfaces are those whose values in the SSBO can be updated
void MaterialSystem::UpdateDynamicSurfaces() {
	if ( dynamicDrawSurfsSize == 0 ) {
		return;
	}

	materialsSSBO.BindBuffer();
	uint32_t* materialsData = materialsSSBO.MapBufferRange( dynamicDrawSurfsOffset, dynamicDrawSurfsSize );
	// Shader uniforms are set to 0 if they're not specified, so make sure we do that here too
	memset( materialsData, 0, 4 * dynamicDrawSurfsSize );
	for ( drawSurf_t& drawSurf : dynamicDrawSurfs ) {
		uint32_t stage = 0;
		for ( shaderStage_t* pStage = drawSurf.shader->stages; pStage < drawSurf.shader->lastStage; pStage++ ) {
			Material& material = materialPacks[drawSurf.materialPackIDs[stage]].materials[drawSurf.materialIDs[stage]];

			pStage->surfaceDataUpdater( materialsData, material, &drawSurf, stage );

			stage++;
		}
	}
	materialsSSBO.UnmapBuffer();
}

void MaterialSystem::UpdateFrameData() {
	atomicCommandCountersBuffer.BindBufferBase( GL_SHADER_STORAGE_BUFFER );
	gl_clearSurfacesShader->BindProgram( 0 );
	gl_clearSurfacesShader->SetUniform_Frame( nextFrame );
	gl_clearSurfacesShader->DispatchCompute( MAX_VIEWS, 1, 1 );
	atomicCommandCountersBuffer.UnBindBufferBase( GL_SHADER_STORAGE_BUFFER );

	GL_CheckErrors();
}

void MaterialSystem::QueueSurfaceCull( const uint32_t viewID, const vec3_t origin, const frustum_t* frustum ) {
	VectorCopy( origin, frames[nextFrame].viewFrames[viewID].origin );
	memcpy( frames[nextFrame].viewFrames[viewID].frustum, frustum, sizeof( frustum_t ) );
	frames[nextFrame].viewCount++;
}

void MaterialSystem::DepthReduction() {
	if ( r_lockpvs->integer ) {
		if ( !PVSLocked ) {
			lockedDepthImage = depthImage;
		}

		return;
	}

	int width = depthImage->width;
	int height = depthImage->height;

	gl_depthReductionShader->BindProgram( 0 );

	uint32_t globalWorkgroupX = ( width + 7 ) / 8;
	uint32_t globalWorkgroupY = ( height + 7 ) / 8;

	GL_Bind( tr.currentDepthImage );
	glBindImageTexture( 2, depthImage->texnum, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F );

	gl_depthReductionShader->SetUniform_InitialDepthLevel( true );
	gl_depthReductionShader->SetUniform_ViewWidth( width );
	gl_depthReductionShader->SetUniform_ViewHeight( height );
	gl_depthReductionShader->DispatchCompute( globalWorkgroupX, globalWorkgroupY, 1 );

	for ( int i = 0; i < depthImageLevels; i++ ) {
		width = width > 1 ? width >> 1 : 1;
		height = height > 1 ? height >> 1 : 1;

		globalWorkgroupX = ( width + 7 ) / 8;
		globalWorkgroupY = ( height + 7 ) / 8;

		glBindImageTexture( 1, depthImage->texnum, i, GL_FALSE, 0, GL_READ_ONLY, GL_R32F );
		glBindImageTexture( 2, depthImage->texnum, i + 1, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F );

		gl_depthReductionShader->SetUniform_InitialDepthLevel( false );
		gl_depthReductionShader->SetUniform_ViewWidth( width );
		gl_depthReductionShader->SetUniform_ViewHeight( height );
		gl_depthReductionShader->DispatchCompute( globalWorkgroupX, globalWorkgroupY, 1 );

		glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );
	}
}

void MaterialSystem::CullSurfaces() {
	if ( r_gpuOcclusionCulling.Get() ) {
		DepthReduction();
	}

	surfaceDescriptorsSSBO.BindBufferBase();
	surfaceCommandsSSBO.BindBufferBase();
	culledCommandsBuffer.BindBufferBase( GL_SHADER_STORAGE_BUFFER );
	surfaceBatchesUBO.BindBufferBase();
	atomicCommandCountersBuffer.BindBufferBase( GL_ATOMIC_COUNTER_BUFFER );

	if ( totalPortals > 0 ) {
		portalSurfacesSSBO.BindBufferBase();
	}

	GL_CheckErrors();

	for ( uint32_t view = 0; view < frames[nextFrame].viewCount; view++ ) {
		vec3_t origin;
		frustum_t* frustum = &frames[nextFrame].viewFrames[view].frustum;

		vec4_t frustumPlanes[6];
		for ( int i = 0; i < 6; i++ ) {
			VectorCopy( PVSLocked ? lockedFrustums[view][i].normal : frustum[0][i].normal, frustumPlanes[i] );
			frustumPlanes[i][3] = PVSLocked ? lockedFrustums[view][i].dist : frustum[0][i].dist;
		}
		matrix_t viewMatrix;
		if ( PVSLocked ) {
			VectorCopy( lockedOrigins[view], origin );
			MatrixCopy( lockedViewMatrix, viewMatrix );
		} else {
			VectorCopy( frames[nextFrame].viewFrames[view].origin, origin );
			MatrixCopy( backEnd.viewParms.world.modelViewMatrix, viewMatrix );
		}

		gl_cullShader->BindProgram( 0 );
		uint32_t globalWorkGroupX = totalDrawSurfs % MAX_COMMAND_COUNTERS == 0 ?
			totalDrawSurfs / MAX_COMMAND_COUNTERS : totalDrawSurfs / MAX_COMMAND_COUNTERS + 1;
		GL_Bind( depthImage );
		gl_cullShader->SetUniform_Frame( nextFrame );
		gl_cullShader->SetUniform_ViewID( view );
		gl_cullShader->SetUniform_TotalDrawSurfs( totalDrawSurfs );
		gl_cullShader->SetUniform_UseFrustumCulling( r_gpuFrustumCulling.Get() );
		gl_cullShader->SetUniform_UseOcclusionCulling( r_gpuOcclusionCulling.Get() );
		gl_cullShader->SetUniform_CameraPosition( origin );
		gl_cullShader->SetUniform_ModelViewMatrix( viewMatrix );
		gl_cullShader->SetUniform_FirstPortalGroup( globalWorkGroupX );
		gl_cullShader->SetUniform_TotalPortals( totalPortals );
		gl_cullShader->SetUniform_ViewWidth( depthImage->width );
		gl_cullShader->SetUniform_ViewHeight( depthImage->height );
		gl_cullShader->SetUniform_SurfaceCommandsOffset( surfaceCommandsCount * ( MAX_VIEWS * nextFrame + view ) );
		gl_cullShader->SetUniform_P00( glState.projectionMatrix[glState.stackIndex][0] );
		gl_cullShader->SetUniform_P11( glState.projectionMatrix[glState.stackIndex][5] );

		if ( totalPortals > 0 ) {
			globalWorkGroupX += totalPortals % 64 == 0 ?
				totalPortals / 64 : totalPortals / 64 + 1;
		}

		if ( PVSLocked ) {
			if ( r_lockpvs->integer == 0 ) {
				PVSLocked = false;
			}
		}
		if ( r_lockpvs->integer == 1 && !PVSLocked ) {
			PVSLocked = true;
			for ( int i = 0; i < 6; i++ ) {
				VectorCopy( frustum[0][i].normal, lockedFrustums[view][i].normal );
				lockedFrustums[view][i].dist = frustum[0][i].dist;
			}
			VectorCopy( origin, lockedOrigins[view] );
			MatrixCopy( viewMatrix, lockedViewMatrix );
		}

		gl_cullShader->SetUniform_Frustum( frustumPlanes );

		gl_cullShader->DispatchCompute( globalWorkGroupX, 1, 1 );

		gl_processSurfacesShader->BindProgram( 0 );
		gl_processSurfacesShader->SetUniform_Frame( nextFrame );
		gl_processSurfacesShader->SetUniform_ViewID( view );
		gl_processSurfacesShader->SetUniform_SurfaceCommandsOffset( surfaceCommandsCount * ( MAX_VIEWS * nextFrame + view ) );

		glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT );
		gl_processSurfacesShader->DispatchCompute( totalBatchCount, 1, 1 );
	}

	surfaceDescriptorsSSBO.UnBindBufferBase();
	surfaceCommandsSSBO.UnBindBufferBase();
	culledCommandsBuffer.UnBindBufferBase( GL_SHADER_STORAGE_BUFFER );
	surfaceBatchesUBO.UnBindBufferBase();
	atomicCommandCountersBuffer.UnBindBufferBase( GL_ATOMIC_COUNTER_BUFFER );

	if ( totalPortals > 0 ) {
		portalSurfacesSSBO.UnBindBufferBase();
	}

	GL_CheckErrors();
}

void MaterialSystem::StartFrame() {
	if ( !generatedWorldCommandBuffer ) {
		return;
	}
	frames[nextFrame].viewCount = 0;

	// renderedMaterials.clear();
	// UpdateDynamicSurfaces();
	// UpdateFrameData();
}

void MaterialSystem::EndFrame() {
	if ( !generatedWorldCommandBuffer ) {
		return;
	}

	currentFrame = nextFrame;
	nextFrame++;
	if ( nextFrame >= MAX_FRAMES ) {
		nextFrame = 0;
	}
}

void MaterialSystem::GeneratePortalBoundingSpheres() {
	Log::Debug( "Generating portal bounding spheres" );

	totalPortals = portalSurfacesTmp.size();

	if ( totalPortals == 0 ) {
		return;
	}

	// FIXME: This only requires distance, origin and radius can be moved to surfaceDescriptors SSBO,
	// drawSurfID is not needed as it's the same as the index in portalSurfacesSSBO
	PortalSurface* portalSurfs = new PortalSurface[totalPortals * sizeof( PortalSurface ) * MAX_VIEWFRAMES];

	uint32_t index = 0;
	for ( drawSurf_t* drawSurf : portalSurfacesTmp ) {
		Tess_MapVBOs( /*forceCPU=*/ true );
		Tess_Begin( Tess_StageIteratorDummy, nullptr, nullptr, true, -1, 0 );
		rb_surfaceTable[Util::ordinal( *( drawSurf->surface ) )]( drawSurf->surface );
		const int numVerts = tess.numVertexes;
		vec3_t portalCenter{ 0.0, 0.0, 0.0 };
		for ( int vertIndex = 0; vertIndex < numVerts; vertIndex++ ) {
			VectorAdd( portalCenter, tess.verts[vertIndex].xyz, portalCenter );
		}
		VectorScale( portalCenter, 1.0 / numVerts, portalCenter );

		float furthestDistance = 0.0;
		for ( int vertIndex = 0; vertIndex < numVerts; vertIndex++ ) {
			const float distance = Distance( portalCenter, tess.verts[vertIndex].xyz );
			furthestDistance = distance > furthestDistance ? distance : furthestDistance;
		}

		Tess_Clear();

		portalSurfaces.emplace_back( *drawSurf );
		PortalSurface sphere;
		VectorCopy( portalCenter, sphere.origin );
		sphere.radius = furthestDistance;
		sphere.drawSurfID = portalSurfaces.size() - 1;
		sphere.distance = -1;

		portalBounds.emplace_back( sphere );
		for ( uint32_t i = 0; i < MAX_FRAMES; i++ ) {
			for ( uint32_t j = 0; j < MAX_VIEWS; j++ ) {
				portalSurfs[index + ( i * MAX_VIEWS + j ) * totalPortals] = sphere;
			}
		}
		index++;
	}

	portalSurfacesSSBO.BindBuffer();
	portalSurfacesSSBO.BufferStorage( totalPortals * PORTAL_SURFACE_SIZE * MAX_VIEWS, 2, portalSurfs );
	portalSurfacesSSBO.MapAll();
	portalSurfacesSSBO.UnBindBuffer();

	portalSurfacesTmp.clear();
}

void MaterialSystem::Free() {
	generatedWorldCommandBuffer = false;

	dynamicDrawSurfs.clear();
	portalSurfaces.clear();
	portalSurfacesTmp.clear();
	portalBounds.clear();
	skyShaders.clear();
	renderedMaterials.clear();

	R_SyncRenderThread();

	surfaceCommandsSSBO.UnmapBuffer();
	culledCommandsBuffer.UnmapBuffer();
	atomicCommandCountersBuffer.UnmapBuffer();

	if ( totalPortals > 0 ) {
		portalSurfacesSSBO.UnmapBuffer();

		for ( PortalView& portalView : portalStack ) {
			portalView.drawSurf = nullptr;
			memset( portalView.views, 0, MAX_VIEWS * sizeof( uint32_t ) );
			portalView.count = 0;
		}
	}

	currentFrame = 0;
	nextFrame = 1;
	maxStages = 0;

	renderSkyBrushDepth = false;

	for ( MaterialPack& pack : materialPacks ) {
		for ( Material& material : pack.materials ) {
			material.drawCommands.clear();
			material.drawSurfs.clear();
		}
		pack.materials.clear();
	}
}

// This gets the information for the surface vertex/index data through Tess
void MaterialSystem::AddDrawCommand( const uint32_t materialID, const uint32_t materialPackID, const uint32_t materialsSSBOOffset,
									 const GLuint count, const GLuint firstIndex ) {
	cmd.cmd.count = count;
	cmd.cmd.firstIndex = firstIndex;
	cmd.cmd.baseInstance = materialsSSBOOffset;
	cmd.materialsSSBOOffset = materialsSSBOOffset;

	materialPacks[materialPackID].materials[materialID].drawCommands.emplace_back( cmd );
	lastCommandID = materialPacks[materialPackID].materials[materialID].drawCommands.size() - 1;
	cmd.textureCount = 0;
}

void MaterialSystem::AddTexture( Texture* texture ) {
	if ( cmd.textureCount > MAX_DRAWCOMMAND_TEXTURES ) {
		Sys::Drop( "Exceeded max DrawCommand textures" );
	}
	cmd.textures[cmd.textureCount] = texture;
	cmd.textureCount++;
}

bool MaterialSystem::AddPortalSurface( uint32_t viewID, PortalSurface* portalSurfs ) {
	uint32_t portalViews[MAX_VIEWS] {};
	uint32_t count = 0;

	frames[nextFrame].viewFrames[viewID].viewCount = 0;
	portalStack[viewID].count = 0;

	PortalSurface* tmpSurfs = new PortalSurface[totalPortals];
	memcpy( tmpSurfs, portalSurfs + viewID * totalPortals, totalPortals * sizeof( PortalSurface ) );
	std::sort( tmpSurfs, tmpSurfs + totalPortals,
		[]( const PortalSurface& lhs, const PortalSurface& rhs ) {
			return lhs.distance < rhs.distance;
		} );

	for ( uint32_t i = 0; i < totalPortals; i++ ) {
		PortalSurface* portalSurface = &tmpSurfs[i];
		if ( portalSurface->distance == -1 ) { // -1 is set if the surface is culled
			continue;
		}
		
		uint32_t portalViewID = viewCount + 1;
		// This check has to be done first so we can correctly determine when we get to MAX_VIEWS - 1 amount of views
		screenRect_t surfRect;
		bool offScreenOrOutOfRange = 0 != PortalOffScreenOrOutOfRange(
			&portalSurfaces[ portalSurface->drawSurfID ], surfRect );
		Tess_Clear();
		if ( offScreenOrOutOfRange ) {
			continue;
		}

		if ( portalViewID == MAX_VIEWS ) {
			continue;
		}

		portalViews[count] = portalViewID;
		frames[nextFrame].viewFrames[portalViewID].portalSurfaceID = portalSurface->drawSurfID;
		frames[nextFrame].viewFrames[viewID].viewCount++;

		portalStack[viewID].views[count] = portalViewID;
		portalStack[portalViewID].drawSurf = &portalSurfaces[portalSurface->drawSurfID];
		portalStack[viewID].count++;

		count++;
		viewCount++;

		if ( count == MAX_VIEWS || viewCount == MAX_VIEWS ) {
			return false;
		}

		for ( uint32_t j = 0; j < frames[currentFrame].viewFrames[viewID].viewCount; j++ ) {
			uint32_t subView = frames[currentFrame].viewFrames[viewID].portalViews[j];
			if ( subView != 0 && portalSurface->drawSurfID == frames[currentFrame].viewFrames[subView].portalSurfaceID ) {
				if ( !AddPortalSurface( subView, portalSurfs ) ) {
					return false;
				}

				portalViewID = subView;
				break;
			}
		}
	}

	memcpy( frames[nextFrame].viewFrames[viewID].portalViews, portalViews, MAX_VIEWS * sizeof( uint32_t ) );

	return true;
}

void MaterialSystem::AddPortalSurfaces() {
	if ( totalPortals == 0 ) {
		return;
	}

	if ( r_lockpvs->integer ) {
		return;
	}

	portalSurfacesSSBO.BindBufferBase();
	PortalSurface* portalSurfs = ( PortalSurface* ) portalSurfacesSSBO.GetCurrentAreaData();
	viewCount = 0;
	// This will recursively find potentially visible portals in each view based on the data read back from the GPU
	// It only fills up an array up to MAX_VIEWS, the actual views are still added in R_MirrowViewBySurface()
	AddPortalSurface( 0, portalSurfs );
	portalSurfacesSSBO.AreaIncr();
}

void MaterialSystem::RenderMaterials( const shaderSort_t fromSort, const shaderSort_t toSort, const uint32_t viewID ) {
	if ( !r_drawworld->integer ) {
		return;
	}

	if ( frameStart ) {
		renderedMaterials.clear();
		UpdateDynamicSurfaces();
		UpdateFrameData();
		// StartFrame();

		// Make sure compute dispatches from the last frame finished writing to memory
		glMemoryBarrier( GL_COMMAND_BARRIER_BIT );
		frameStart = false;
	}

	materialsSSBO.BindBufferBase();

	for ( MaterialPack& materialPack : materialPacks ) {
		if ( materialPack.fromSort >= fromSort && materialPack.toSort <= toSort ) {
			for ( Material& material : materialPack.materials ) {
				RenderMaterial( material, viewID );
				renderedMaterials.emplace_back( &material );
			}
		}
	}

	// Draw the skybox here because we skipped R_AddWorldSurfaces()
	const bool environmentFogDraw = ( fromSort <= shaderSort_t::SS_ENVIRONMENT_FOG ) && ( toSort >= shaderSort_t::SS_ENVIRONMENT_FOG );
	const bool environmentNoFogDraw = ( fromSort <= shaderSort_t::SS_ENVIRONMENT_NOFOG ) && toSort >= ( shaderSort_t::SS_ENVIRONMENT_NOFOG );
	if ( tr.hasSkybox && ( environmentFogDraw || environmentNoFogDraw ) ) {
		const bool noFogPass = toSort >= shaderSort_t::SS_ENVIRONMENT_NOFOG;
		for ( shader_t* skyShader : skyShaders ) {
			if ( skyShader->noFog != noFogPass && !renderSkyBrushDepth ) {
				continue;
			}

			// Use stencil buffer to avoid rendering stuff over the skybox that would normally be culled by the BSP
			if ( backEnd.viewParms.portalLevel == 0 ) {
				glEnable( GL_STENCIL_TEST );
				glStencilMask( 0xff );
			}

			glStencilFunc( GL_EQUAL, backEnd.viewParms.portalLevel, 0xff );
			glStencilOp( GL_KEEP, GL_KEEP, GL_INCR );

			glState.glStateBitsMask = 0;

			for ( Material& material : materialPacks[3].materials ) {
				if ( material.skyShader == skyShader ) {
					RenderMaterial( material, viewID );
					renderedMaterials.emplace_back( &material );
				}
			}

			// Set depth to 1.0 on skybrushes
			glStencilFunc( GL_EQUAL, backEnd.viewParms.portalLevel + 1, 0xff );
			glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

			GL_State( GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_ALWAYS );
			glState.glStateBitsMask = GLS_COLORMASK_BITS | GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_ALWAYS;
			glDepthRange( 1.0, 1.0 );

			for ( Material& material : materialPacks[3].materials ) {
				if ( material.skyShader == skyShader ) {
					RenderMaterial( material, viewID );
					renderedMaterials.emplace_back( &material );
				}
			}

			// Actually render the skybox
			if ( !renderSkyBrushDepth ) {
				tr.drawingSky = true;
				Tess_Begin( Tess_StageIteratorSky, skyShader, nullptr, false, -1, 0, false );
				Tess_End();
			}

			// Decrease the stencil bits back on skybrushes
			glStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

			// Set depth back to the skybrushes depth
			glState.glStateBitsMask = 0;
			GL_State( GLS_COLORMASK_BITS | GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_ALWAYS );
			glState.glStateBitsMask = GLS_COLORMASK_BITS | GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_ALWAYS;

			/* SSAO excludes fragments with depth 1.0, so we need to change the depth on skybrushes back to 1.0
			if SSAO is enabled and it's an SS_ENVIRONMENT_FOG pass */
			if ( r_ssao->integer ) {
				if ( environmentFogDraw && !renderSkyBrushDepth ) {
					glDepthRange( 0.0, 1.0 );
					renderSkyBrushDepth = true;
				} else {
					glDepthRange( 1.0, 1.0 );
				}
			} else {
				glDepthRange( 0.0, 1.0 );
			}

			for ( Material& material : materialPacks[3].materials ) {
				if ( material.skyShader == skyShader ) {
					RenderMaterial( material, viewID );
					renderedMaterials.emplace_back( &material );
				}
			}

			glStencilFunc( GL_EQUAL, backEnd.viewParms.portalLevel, 0xff );
			glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

			glState.glStateBitsMask = 0;
			GL_State( GLS_DEFAULT );
			glDepthRange( 0.0, 1.0 );

			if ( backEnd.viewParms.portalLevel == 0 ) {
				glDisable( GL_STENCIL_TEST );
			}
		}
	}

	if ( environmentNoFogDraw && renderSkyBrushDepth ) {
		renderSkyBrushDepth = false;
	}
}

void MaterialSystem::RenderMaterial( Material& material, const uint32_t viewID ) {
	backEnd.currentEntity = &tr.worldEntity;

	GL_State( material.stateBits );
	if ( material.usePolygonOffset ) {
		glEnable( GL_POLYGON_OFFSET_FILL );
		GL_PolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	} else {
		glDisable( GL_POLYGON_OFFSET_FILL );
	}
	GL_Cull( material.cullType );

	backEnd.orientation = backEnd.viewParms.world;
	GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

	material.shaderBinder( &material );

	R_BindVBO( material.vbo );
	R_BindIBO( material.ibo );
	material.shader->SetRequiredVertexPointers();

	if ( !material.texturesResident ) {
		for ( Texture* texture : material.textures ) {
			if ( !texture->IsResident() ) {
				texture->MakeResident();

				bool resident = glIsTextureHandleResidentARB( texture->bindlessTextureHandle );

				if ( resident ) {
					continue;
				}

				for ( Material* mat : renderedMaterials ) {
					Log::Warn( "Making material %u textures non-resident (%u)", mat->id, mat->textures.size() );
					for ( Texture* tex : mat->textures ) {
						if ( tex->IsResident() ) {
							tex->MakeNonResident();
						}
					}
					mat->texturesResident = false;
				}

				texture->MakeResident();

				resident = glIsTextureHandleResidentARB( texture->bindlessTextureHandle );

				if( !resident ) {
					Log::Warn( "Not enough texture space! Some textures may be missing" );
					break;
				}
			}
		}
	}
	material.texturesResident = true;

	culledCommandsBuffer.BindBuffer( GL_DRAW_INDIRECT_BUFFER );

	atomicCommandCountersBuffer.BindBuffer( GL_PARAMETER_BUFFER_ARB );

	glMultiDrawElementsIndirectCountARB( GL_TRIANGLES, GL_UNSIGNED_INT,
		BUFFER_OFFSET( material.surfaceCommandBatchOffset * SURFACE_COMMANDS_PER_BATCH * sizeof( GLIndirectBuffer::GLIndirectCommand )
					   + ( surfaceCommandsCount * ( MAX_VIEWS * currentFrame + viewID )
					   * sizeof( GLIndirectBuffer::GLIndirectCommand ) ) ),
		material.globalID * sizeof( uint32_t )
		+ ( MAX_COMMAND_COUNTERS * ( MAX_VIEWS * currentFrame + viewID ) ) * sizeof( uint32_t ),
		material.drawCommands.size(), 0 );

	if ( r_showTris->integer
		&& ( material.stateBits & GLS_DEPTHMASK_TRUE ) == 0
		&& material.shaderBinder == &BindShaderLightMapping )
	{
		gl_lightMappingShaderMaterial->SetUniform_ShowTris( 1 );
		GL_State( GLS_DEPTHTEST_DISABLE );
		glMultiDrawElementsIndirectCountARB( GL_LINES, GL_UNSIGNED_INT,
			BUFFER_OFFSET( material.surfaceCommandBatchOffset * SURFACE_COMMANDS_PER_BATCH * sizeof( GLIndirectBuffer::GLIndirectCommand )
			+ ( surfaceCommandsCount * ( MAX_VIEWS * currentFrame + viewID )
			* sizeof( GLIndirectBuffer::GLIndirectCommand ) ) ),
			material.globalID * sizeof( uint32_t )
			+ ( MAX_COMMAND_COUNTERS * ( MAX_VIEWS * currentFrame + viewID ) ) * sizeof( uint32_t ),
			material.drawCommands.size(), 0 );
		gl_lightMappingShaderMaterial->SetUniform_ShowTris( 0 );
	}

	culledCommandsBuffer.UnBindBuffer( GL_DRAW_INDIRECT_BUFFER );

	atomicCommandCountersBuffer.UnBindBuffer( GL_PARAMETER_BUFFER_ARB );

	if ( material.usePolygonOffset ) {
		glDisable( GL_POLYGON_OFFSET_FILL );
	}
}
