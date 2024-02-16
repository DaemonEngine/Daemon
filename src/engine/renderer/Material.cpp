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

#include "Material.h"

#include "tr_local.h"

GLSSBO materialsSSBO( "materials", 0 );
GLIndirectBuffer commandBuffer( "drawCommands" );
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
					if ( bundle.texMods[i].sExp.active || bundle.texMods[i].tExp.active ) {
						pStage->texMatricesDynamic = true;
					}
					break;
				}

				case texMod_t::TMOD_ROTATE2:
				{
					if( bundle.texMods[i].rExp.active ) {
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
	pStage->dynamic = pStage->dynamic || pStage->ifExp.active;
	pStage->dynamic = pStage->dynamic || pStage->alphaExp.active || pStage->alphaTestExp.active;
	pStage->dynamic = pStage->dynamic || pStage->rgbExp.active || pStage->redExp.active || pStage->greenExp.active || pStage->blueExp.active;
	pStage->dynamic = pStage->dynamic || pStage->deformMagnitudeExp.active;
	pStage->dynamic = pStage->dynamic || pStage->depthScaleExp.active || pStage->etaExp.active || pStage->etaDeltaExp.active
		|| pStage->fogDensityExp.active || pStage->fresnelBiasExp.active || pStage->fresnelPowerExp.active
		|| pStage->fresnelScaleExp.active || pStage->normalIntensityExp.active || pStage->refractionIndexExp.active;

	pStage->dynamic = pStage->dynamic || pStage->colorDynamic || pStage->texMatricesDynamic || pStage->texturesDynamic;
}

static image_t* GetLightMap( drawSurf_t* drawSurf ) {
	if ( static_cast<size_t>( drawSurf->lightmapNum() ) < tr.lightmaps.size() ) {
		return tr.lightmaps[drawSurf->lightmapNum()];
	} else {
		return tr.whiteImage;
	}
}

static image_t* GetDeluxeMap( drawSurf_t* drawSurf ) {
	if ( static_cast<size_t>( drawSurf->lightmapNum() ) < tr.deluxemaps.size() ) {
		return tr.deluxemaps[drawSurf->lightmapNum()];
	} else {
		return tr.blackImage;
	}
}

// UpdateSurface*() functions will actually write the uniform values to the SSBO
// Mirrors parts of the Render_*() functions in tr_shade.cpp

static void UpdateSurfaceDataGeneric( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
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
	// We should cancel overbrightBits if there is no light,
	// and it's not using blendFunc dst_color.
	bool blendFunc_dstColor = ( pStage->stateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_DST_COLOR;
	float inverseLightFactor = ( pStage->shaderHasNoLight && !blendFunc_dstColor ) ? tr.mapInverseLightFactor : 1.0f;
	gl_genericShaderMaterial->SetUniform_InverseLightFactor( inverseLightFactor );

	// u_ColorModulate
	colorGen_t rgbGen;
	alphaGen_t alphaGen;
	SetRgbaGen( pStage, &rgbGen, &alphaGen );
	gl_genericShaderMaterial->SetUniform_ColorModulate( rgbGen, alphaGen );

	Tess_ComputeColor( pStage );
	gl_genericShaderMaterial->SetUniform_Color( tess.svars.color );

	Tess_ComputeTexMatrices( pStage );
	gl_genericShaderMaterial->SetUniform_TextureMatrix( tess.svars.texMatrices[TB_COLORMAP] );

	// u_DeformGen
	gl_genericShaderMaterial->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	// bind u_ColorMap=
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

static void UpdateSurfaceDataLightMapping( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
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

	lightMode_t lightMode = lightMode_t::FULLBRIGHT;
	deluxeMode_t deluxeMode = deluxeMode_t::NONE;

	/* TODO: investigate what this is. It's probably a hack to detect some
	specific use case. Without knowing which use case this takes care about,
	any change in the following code may break it. Or it may be a hack we
	should drop if it is for a bug we don't have anymore. */
	bool hack = shader->lastStage != shader->stages
		&& shader->stages[0].rgbGen == colorGen_t::CGEN_VERTEX;

	if ( ( shader->surfaceFlags & SURF_NOLIGHTMAP ) && !hack ) {
		// Use fullbright on “surfaceparm nolightmap” materials.
	} else if ( pStage->type == stageType_t::ST_COLLAPSE_COLORMAP ) {
		/* Use fullbright for collapsed stages without lightmaps,
		for example:

		  {
			map textures/texture_d
			heightMap textures/texture_h
		  }

		This is doable for some complex multi-stage materials. */
	} else if ( drawSurf->bspSurface ) {
		lightMode = tr.worldLight;
		deluxeMode = tr.worldDeluxe;

		if ( lightMode == lightMode_t::MAP ) {
			bool hasLightMap = static_cast<size_t>( drawSurf->lightmapNum() ) < tr.lightmaps.size();

			if ( !hasLightMap ) {
				lightMode = lightMode_t::VERTEX;
				deluxeMode = deluxeMode_t::NONE;
			}
		}
	} else {
		lightMode = tr.modelLight;
		deluxeMode = tr.modelDeluxe;
	}

	// u_Map, u_DeluxeMap
	image_t* lightmap = tr.whiteImage;
	image_t* deluxemap = tr.whiteImage;

	// u_ColorModulate
	colorGen_t rgbGen;
	alphaGen_t alphaGen;
	SetRgbaGen( pStage, &rgbGen, &alphaGen );

	switch ( lightMode ) {
		case lightMode_t::VERTEX:
			// Do not rewrite pStage->rgbGen.
			rgbGen = colorGen_t::CGEN_VERTEX;
			tess.svars.color.SetRed( 0.0f );
			tess.svars.color.SetGreen( 0.0f );
			tess.svars.color.SetBlue( 0.0f );
			break;

		case lightMode_t::GRID:
			// Store lightGrid1 as lightmap,
			// the GLSL code will know how to deal with it.
			lightmap = tr.lightGrid1Image;
			break;

		case lightMode_t::MAP:
			lightmap = GetLightMap( drawSurf );

			break;

		default:
			break;
	}

	switch ( deluxeMode ) {
		case deluxeMode_t::MAP:
			// Deluxe mapping for world surface.
			deluxemap = GetDeluxeMap( drawSurf );
			break;

		case deluxeMode_t::GRID:
			// Deluxe mapping emulation from grid light for game models.
			// Store lightGrid2 as deluxemap,
			// the GLSL code will know how to deal with it.
			deluxemap = tr.lightGrid2Image;
			break;

		default:
			break;
	}

	bool enableGridLighting = ( lightMode == lightMode_t::GRID );
	bool enableGridDeluxeMapping = ( deluxeMode == deluxeMode_t::GRID );

	// TODO: Update this when this is extended to MDV support
	gl_lightMappingShaderMaterial->SetUniform_VertexInterpolation( false );

	if ( glConfig2.dynamicLight ) {
		gl_lightMappingShaderMaterial->SetUniformBlock_Lights( tr.dlightUBO );

		// bind u_LightTiles
		if ( r_dynamicLightRenderer.Get() == Util::ordinal( dynamicLightRenderer_t::TILED ) ) {
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
	bool blendFunc_dstColor = ( pStage->stateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_DST_COLOR;
	bool noLight = pStage->shaderHasNoLight || lightMode == lightMode_t::FULLBRIGHT;
	float inverseLightFactor = ( noLight && !blendFunc_dstColor ) ? tr.mapInverseLightFactor : -tr.mapInverseLightFactor;
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

static void UpdateSurfaceDataReflection( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
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
		GL_BindNearestCubeMap( gl_reflectionShaderMaterial->GetUniformLocation_ColorMap(), backEnd.currentEntity->e.origin );
	} else {
		GL_BindNearestCubeMap( gl_reflectionShaderMaterial->GetUniformLocation_ColorMap(), backEnd.viewParms.orientation.origin );
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

static void UpdateSurfaceDataSkybox( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
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

static void UpdateSurfaceDataScreen( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
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

static void UpdateSurfaceDataHeatHaze( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
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

static void UpdateSurfaceDataLiquid( uint32_t* materials, Material& material, drawSurf_t* drawSurf, const uint32_t stage ) {
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

	for ( uint32_t materialPackID = 0; materialPackID < 3; materialPackID++ ) {
		for ( Material& material : materialPacks[materialPackID].materials ) {

			for ( drawSurf_t* drawSurf : material.drawSurfs ) {
				bool hasDynamicStages = false;

				uint32_t stage = 0;
				for ( shaderStage_t* pStage = drawSurf->shader->stages; pStage < drawSurf->shader->lastStage; pStage++ ) {
					if ( drawSurf->materialIDs[stage] != material.id || drawSurf->materialPackIDs[stage] != materialPackID ) {
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

					switch ( pStage->type ) {
						case stageType_t::ST_COLORMAP:
							// generic2D
							UpdateSurfaceDataGeneric( materialsData, material, drawSurf, stage );
							break;
						case stageType_t::ST_STYLELIGHTMAP:
						case stageType_t::ST_STYLECOLORMAP:
							UpdateSurfaceDataGeneric( materialsData, material, drawSurf, stage );
							break;
						case stageType_t::ST_LIGHTMAP:
						case stageType_t::ST_DIFFUSEMAP:
						case stageType_t::ST_COLLAPSE_COLORMAP:
						case stageType_t::ST_COLLAPSE_DIFFUSEMAP:
							UpdateSurfaceDataLightMapping( materialsData, material, drawSurf, stage );
							break;
						case stageType_t::ST_REFLECTIONMAP:
						case stageType_t::ST_COLLAPSE_REFLECTIONMAP:
							UpdateSurfaceDataReflection( materialsData, material, drawSurf, stage );
							break;
						case stageType_t::ST_REFRACTIONMAP:
						case stageType_t::ST_DISPERSIONMAP:
							// Not implemented yet
							break;
						case stageType_t::ST_SKYBOXMAP:
							UpdateSurfaceDataSkybox( materialsData, material, drawSurf, stage );
							break;
						case stageType_t::ST_SCREENMAP:
							UpdateSurfaceDataScreen( materialsData, material, drawSurf, stage );
							break;
						case stageType_t::ST_PORTALMAP:
							// This is supposedly used for alphagen portal and portal surfaces should never get here
							ASSERT_UNREACHABLE();
							break;
						case stageType_t::ST_HEATHAZEMAP:
							UpdateSurfaceDataHeatHaze( materialsData, material, drawSurf, stage );
							break;
						case stageType_t::ST_LIQUIDMAP:
							UpdateSurfaceDataLiquid( materialsData, material, drawSurf, stage );
							break;

						default:
							break;
					}

					tess.currentDrawSurf = drawSurf;

					tess.currentSSBOOffset = tess.currentDrawSurf->materialsSSBOOffset[stage];
					tess.materialID = tess.currentDrawSurf->materialIDs[stage];
					tess.materialPackID = tess.currentDrawSurf->materialPackIDs[stage];

					tess.multiDrawPrimitives = 0;
					tess.numIndexes = 0;
					tess.numVertexes = 0;
					tess.attribsSet = 0;

					rb_surfaceTable[Util::ordinal( *drawSurf->surface )]( drawSurf->surface );

					pStage->colorRenderer( pStage );

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

	uint32_t count = 0;
	for ( const MaterialPack& pack : materialPacks ) {
		for ( const Material& material : pack.materials ) {
			count += material.drawCommands.size();
		}
	}

	if ( count == 0 ) {
		return;
	}

	Log::Debug( "CmdBuffer size: %u", count );

	commandBuffer.BindBuffer();
	glBufferData( GL_DRAW_INDIRECT_BUFFER, count * sizeof( GLIndirectBuffer::GLIndirectCommand ), nullptr, GL_STATIC_DRAW );

	GLIndirectBuffer::GLIndirectCommand* commands = commandBuffer.MapBufferRange( count );
	uint32_t offset = 0;
	for ( MaterialPack& pack : materialPacks ) {
		for ( Material& material : pack.materials ) {
			material.staticCommandOffset = offset;

			for ( const DrawCommand& drawCmd : material.drawCommands ) {
				memcpy( commands, &drawCmd.cmd, sizeof( GLIndirectBuffer::GLIndirectCommand ) );
				commands++;
				offset++;
			}
		}
	}

	commandBuffer.UnmapBuffer();
	GL_CheckErrors();
}

static void BindShaderGeneric( Material* material ) {
	gl_genericShaderMaterial->SetVertexAnimation( material->vertexAnimation );

	gl_genericShaderMaterial->SetTCGenEnvironment( material->tcGenEnvironment );
	gl_genericShaderMaterial->SetTCGenLightmap( material->tcGen_Lightmap );

	gl_genericShaderMaterial->SetDepthFade( material->hasDepthFade );
	gl_genericShaderMaterial->SetVertexSprite( material->vboVertexSprite );

	gl_genericShaderMaterial->BindProgram( material->deformIndex );
}

static void BindShaderLightMapping( Material* material ) {
	gl_lightMappingShaderMaterial->SetVertexAnimation( material->vertexAnimation );
	gl_lightMappingShaderMaterial->SetBspSurface( material->bspSurface );

	gl_lightMappingShaderMaterial->SetDeluxeMapping( material->enableDeluxeMapping );

	gl_lightMappingShaderMaterial->SetGridLighting( material->enableGridLighting );

	gl_lightMappingShaderMaterial->SetGridDeluxeMapping( material->enableGridDeluxeMapping );

	gl_lightMappingShaderMaterial->SetHeightMapInNormalMap( material->hasHeightMapInNormalMap );

	gl_lightMappingShaderMaterial->SetReliefMapping( material->enableReliefMapping );

	gl_lightMappingShaderMaterial->SetReflectiveSpecular( material->enableNormalMapping && tr.cubeHashTable != nullptr );

	gl_lightMappingShaderMaterial->SetPhysicalShading( material->enablePhysicalMapping );

	gl_lightMappingShaderMaterial->BindProgram( material->deformIndex );
}

static void BindShaderReflection( Material* material ) {
	gl_reflectionShaderMaterial->SetHeightMapInNormalMap( material->hasHeightMapInNormalMap );

	gl_reflectionShaderMaterial->SetReliefMapping( material->enableReliefMapping );

	gl_reflectionShaderMaterial->SetVertexAnimation( material->vertexAnimation );

	gl_reflectionShaderMaterial->BindProgram( material->deformIndex );
}

static void BindShaderSkybox( Material* material ) {
	gl_skyboxShaderMaterial->BindProgram( material->deformIndex );
}

static void BindShaderScreen( Material* material ) {
	gl_screenShaderMaterial->BindProgram( material->deformIndex );
}

static void BindShaderHeatHaze( Material* material ) {
	gl_heatHazeShaderMaterial->SetVertexAnimation( material->vertexAnimation );

	gl_heatHazeShaderMaterial->SetVertexSprite( material->vboVertexSprite );

	gl_heatHazeShaderMaterial->BindProgram( material->deformIndex );
}

static void BindShaderLiquid( Material* material ) {
	gl_liquidShaderMaterial->SetHeightMapInNormalMap( material->hasHeightMapInNormalMap );

	gl_liquidShaderMaterial->SetReliefMapping( material->enableReliefMapping );

	gl_liquidShaderMaterial->BindProgram( material->deformIndex );
}

// ProcessMaterial*() are essentially same as BindShader*(), but only set the GL program id to the material,
// without actually binding it
static void ProcessMaterialGeneric( Material* material, shaderStage_t* pStage, shader_t* shader ) {
	material->shader = gl_genericShaderMaterial;

	material->vertexAnimation = false;
	material->tcGenEnvironment = pStage->tcGen_Environment;
	material->tcGen_Lightmap = pStage->tcGen_Lightmap;
	material->vboVertexSprite = shader->autoSpriteMode != 0;
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

static void ProcessMaterialLightMapping( Material* material, shaderStage_t* pStage, drawSurf_t* drawSurf ) {
	material->shader = gl_lightMappingShaderMaterial;

	material->vertexAnimation = false;
	material->bspSurface = false;

	gl_lightMappingShaderMaterial->SetVertexAnimation( false );
	gl_lightMappingShaderMaterial->SetBspSurface( drawSurf->bspSurface );

	lightMode_t lightMode = lightMode_t::FULLBRIGHT;
	deluxeMode_t deluxeMode = deluxeMode_t::NONE;

	bool hack = drawSurf->shader->lastStage != drawSurf->shader->stages
		&& drawSurf->shader->stages[0].rgbGen == colorGen_t::CGEN_VERTEX;
	if ( ( tess.surfaceShader->surfaceFlags & SURF_NOLIGHTMAP ) && !hack ) {
		// Use fullbright on “surfaceparm nolightmap” materials.
	} else if ( pStage->type == stageType_t::ST_COLLAPSE_COLORMAP ) {
		/* Use fullbright for collapsed stages without lightmaps,
		for example:
		  {
			map textures/texture_d
			heightMap textures/texture_h
		  }

		This is doable for some complex multi-stage materials. */
	} else if ( drawSurf->bspSurface ) {
		lightMode = tr.worldLight;
		deluxeMode = tr.worldDeluxe;

		if ( lightMode == lightMode_t::MAP ) {
			bool hasLightMap = ( drawSurf->lightmapNum() >= 0 );

			if ( !hasLightMap ) {
				lightMode = lightMode_t::VERTEX;
				deluxeMode = deluxeMode_t::NONE;
			}
		}
	} else {
		lightMode = tr.modelLight;
		deluxeMode = tr.modelDeluxe;
	}

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

static void ProcessMaterialReflection( Material* material, shaderStage_t* pStage ) {
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

static void ProcessMaterialSkybox( Material* material, shaderStage_t* pStage ) {
	material->shader = gl_skyboxShaderMaterial;

	material->deformIndex = pStage->deformIndex;

	material->program = gl_skyboxShaderMaterial->GetProgram( pStage->deformIndex );
}

static void ProcessMaterialScreen( Material* material, shaderStage_t* pStage ) {
	material->shader = gl_screenShaderMaterial;

	material->deformIndex = pStage->deformIndex;

	material->program = gl_screenShaderMaterial->GetProgram( pStage->deformIndex );
}

static void ProcessMaterialHeatHaze( Material* material, shaderStage_t* pStage, shader_t* shader ) {
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
static void ProcessMaterialLiquid( Material* material, shaderStage_t* pStage ) {
	material->shader = gl_liquidShaderMaterial;

	material->hasHeightMapInNormalMap = pStage->hasHeightMapInNormalMap;
	material->enableReliefMapping = pStage->enableReliefMapping;
	material->deformIndex = pStage->deformIndex;

	gl_liquidShaderMaterial->SetHeightMapInNormalMap( pStage->hasHeightMapInNormalMap );

	gl_liquidShaderMaterial->SetReliefMapping( pStage->enableReliefMapping );

	material->program = gl_liquidShaderMaterial->GetProgram( pStage->deformIndex );
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

	R_AddWorldSurfaces();

	Log::Notice( "World bounds: min: %f %f %f max: %f %f %f", tr.viewParms.visBounds[0][0], tr.viewParms.visBounds[0][1],
		tr.viewParms.visBounds[0][2], tr.viewParms.visBounds[1][0], tr.viewParms.visBounds[1][1], tr.viewParms.visBounds[1][2] );
	VectorCopy( tr.viewParms.visBounds[0], worldViewBounds[0] );
	VectorCopy( tr.viewParms.visBounds[1], worldViewBounds[1] );

	backEnd.currentEntity = &tr.worldEntity;

	drawSurf_t* drawSurf;

	uint32_t id = 0;
	uint32_t previousMaterialID = 0;
	uint32_t packIDs[3] = { 0, 0, 0 };
	skipDrawCommands = true;

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
		if ( shader->isSky || shader->isPortal ) {
			continue;
		}

		// Don't add SF_SKIP surfaces
		if ( *drawSurf->surface == surfaceType_t::SF_SKIP ) {
			continue;
		}

		rb_surfaceTable[Util::ordinal( *( drawSurf->surface ) )]( drawSurf->surface );

		uint32_t stage = 0;
		for ( shaderStage_t* pStage = drawSurf->shader->stages; pStage < drawSurf->shader->lastStage; pStage++ ) {
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
			id = packIDs[materialPack];

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
			material.stageType = pStage->type;
			material.cullType = shader->cullType;
			material.usePolygonOffset = shader->polygonOffset;

			material.vbo = glState.currentVBO;
			material.ibo = glState.currentIBO;

			ComputeDynamics( pStage );

			if ( pStage->texturesDynamic ) {
				drawSurf->texturesDynamic[stage] = true;
			}

			switch ( pStage->type ) {
				case stageType_t::ST_COLORMAP:
					// generic2D also uses this, but it's for ui only, so skip that for now
					ProcessMaterialGeneric( &material, pStage, drawSurf->shader );
					break;
				case stageType_t::ST_STYLELIGHTMAP:
				case stageType_t::ST_STYLECOLORMAP:
					ProcessMaterialGeneric( &material, pStage, drawSurf->shader );
					break;
				case stageType_t::ST_LIGHTMAP:
				case stageType_t::ST_DIFFUSEMAP:
				case stageType_t::ST_COLLAPSE_COLORMAP:
				case stageType_t::ST_COLLAPSE_DIFFUSEMAP:
					ProcessMaterialLightMapping( &material, pStage, drawSurf );
					break;
				case stageType_t::ST_REFLECTIONMAP:
				case stageType_t::ST_COLLAPSE_REFLECTIONMAP:
					ProcessMaterialReflection( &material, pStage );
					break;
				case stageType_t::ST_REFRACTIONMAP:
				case stageType_t::ST_DISPERSIONMAP:
					// Not implemented yet
					break;
				case stageType_t::ST_SKYBOXMAP:
					ProcessMaterialSkybox( &material, pStage );
					break;
				case stageType_t::ST_SCREENMAP:
					ProcessMaterialScreen( &material, pStage );
					break;
				case stageType_t::ST_PORTALMAP:
					// This is supposedly used for alphagen portal and portal surfaces should never get here
					ASSERT_UNREACHABLE();
					break;
				case stageType_t::ST_HEATHAZEMAP:
					// FIXME: This requires 2 draws per surface stage rather than 1
					ProcessMaterialHeatHaze( &material, pStage, drawSurf->shader );
					break;
				case stageType_t::ST_LIQUIDMAP:
					ProcessMaterialLiquid( &material, pStage );
					break;

				default:
					break;
			}

			std::vector<Material>& materials = materialPacks[materialPack].materials;
			std::vector<Material>::iterator currentSearchIt = materials.begin();
			std::vector<Material>::iterator materialIt;
			// Look for this material in the ones we already have
			while( true ) {
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
	}
	skipDrawCommands = false;

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

	skipDrawCommands = true;
	GeneratePortalBoundingSpheres();
	skipDrawCommands = false;

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

	lightMode_t lightMode = lightMode_t::FULLBRIGHT;
	deluxeMode_t deluxeMode = deluxeMode_t::NONE;

	bool hack = drawSurf->shader->lastStage != drawSurf->shader->stages
		&& drawSurf->shader->stages[0].rgbGen == colorGen_t::CGEN_VERTEX;

	if ( ( drawSurf->shader->surfaceFlags & SURF_NOLIGHTMAP ) && !hack ) {
		// Use fullbright on “surfaceparm nolightmap” materials.
	} else if ( pStage->type == stageType_t::ST_COLLAPSE_COLORMAP ) {
		/* Use fullbright for collapsed stages without lightmaps,
		for example:

		  {
			map textures/texture_d
			heightMap textures/texture_h
		  }

		This is doable for some complex multi-stage materials. */
	} else if ( drawSurf->bspSurface ) {
		lightMode = tr.worldLight;
		deluxeMode = tr.worldDeluxe;

		if ( lightMode == lightMode_t::MAP ) {
			bool hasLightMap = static_cast< size_t >( drawSurf->lightmapNum() ) < tr.lightmaps.size();

			if ( !hasLightMap ) {
				lightMode = lightMode_t::VERTEX;
				deluxeMode = deluxeMode_t::NONE;
			}
		}
	} else {
		lightMode = tr.modelLight;
		deluxeMode = tr.modelDeluxe;
	}

	// u_Map, u_DeluxeMap
	image_t* lightmap = tr.whiteImage;
	image_t* deluxemap = tr.whiteImage;

	switch ( lightMode ) {
		case lightMode_t::VERTEX:
			break;

		case lightMode_t::GRID:
			lightmap = tr.lightGrid1Image;
			break;

		case lightMode_t::MAP:
			lightmap = GetLightMap( drawSurf );
			break;

		default:
			break;
	}

	switch ( deluxeMode ) {
		case deluxeMode_t::MAP:
			deluxemap = GetDeluxeMap( drawSurf );
			break;

		case deluxeMode_t::GRID:
			deluxemap = tr.lightGrid2Image;
			break;

		default:
			break;
	}

	material->AddTexture( lightmap->texture );
	material->AddTexture( deluxemap->texture );

	if ( glConfig2.dynamicLight ) {
		if ( r_dynamicLightRenderer.Get() == Util::ordinal( dynamicLightRenderer_t::TILED ) ) {
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

			switch ( pStage->type ) {
				case stageType_t::ST_COLORMAP:
					// generic2D also uses this, but it's for ui only, so skip that for now
					UpdateSurfaceDataGeneric( materialsData, material, &drawSurf, stage );
					break;
				case stageType_t::ST_STYLELIGHTMAP:
				case stageType_t::ST_STYLECOLORMAP:
					UpdateSurfaceDataGeneric( materialsData, material, &drawSurf, stage );
					break;
				case stageType_t::ST_LIGHTMAP:
				case stageType_t::ST_DIFFUSEMAP:
				case stageType_t::ST_COLLAPSE_COLORMAP:
				case stageType_t::ST_COLLAPSE_DIFFUSEMAP:
					UpdateSurfaceDataLightMapping( materialsData, material, &drawSurf, stage );
					break;
				case stageType_t::ST_REFLECTIONMAP:
				case stageType_t::ST_COLLAPSE_REFLECTIONMAP:
					UpdateSurfaceDataReflection( materialsData, material, &drawSurf, stage );
					break;
				case stageType_t::ST_REFRACTIONMAP:
				case stageType_t::ST_DISPERSIONMAP:
					// Not implemented yet
					break;
				case stageType_t::ST_SKYBOXMAP:
					UpdateSurfaceDataSkybox( materialsData, material, &drawSurf, stage );
					break;
				case stageType_t::ST_SCREENMAP:
					UpdateSurfaceDataScreen( materialsData, material, &drawSurf, stage );
					break;
				case stageType_t::ST_PORTALMAP:
					// This is supposedly used for alphagen portal and portal surfaces should never get here
					ASSERT_UNREACHABLE();
					break;
				case stageType_t::ST_HEATHAZEMAP:
					UpdateSurfaceDataHeatHaze( materialsData, material, &drawSurf, stage );
					break;
				case stageType_t::ST_LIQUIDMAP:
					UpdateSurfaceDataLiquid( materialsData, material, &drawSurf, stage );
					break;

				default:
					break;
			}

			stage++;
		}
	}
	materialsSSBO.UnmapBuffer();
}

void MaterialSystem::GeneratePortalBoundingSpheres() {
	Log::Debug( "Generating portal bounding spheres" );

	for ( drawSurf_t* drawSurf : portalSurfacesTmp ) {
		tess.numVertexes = 0;
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

		portalSurfaces.emplace_back( *drawSurf );
		drawSurfBoundingSphere sphere;
		VectorCopy( portalCenter, sphere.origin );
		sphere.radius = furthestDistance;
		sphere.drawSurfID = portalSurfaces.size() - 1;

		portalBounds.emplace_back( sphere );
	}

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
	// Don't add surfaces here if we're just trying to get some VBO/IBO information
	if ( skipDrawCommands ) {
		return;
	}

	cmd.cmd.count = count;
	cmd.cmd.instanceCount = 1;
	cmd.cmd.firstIndex = firstIndex;
	cmd.cmd.baseVertex = 0;
	cmd.cmd.baseInstance = materialsSSBOOffset;
	cmd.materialsSSBOOffset = materialsSSBOOffset;

	materialPacks[materialPackID].materials[materialID].drawCommands.emplace_back(cmd);
	cmd.textureCount = 0;
}

void MaterialSystem::AddTexture( Texture* texture ) {
	if ( cmd.textureCount > MAX_DRAWCOMMAND_TEXTURES ) {
		Sys::Drop( "Exceeded max DrawCommand textures" );
	}
	cmd.textures[cmd.textureCount] = texture;
	cmd.textureCount++;
}

void MaterialSystem::AddPortalSurfaces() {
	// Very inefficient
	// TODO: Mark portals in the cull shader and do a readback to only add portals that can actually be seen
	std::sort( portalBounds.begin(), portalBounds.end(),
		[]( const drawSurfBoundingSphere& lhs, const drawSurfBoundingSphere& rhs ) {
			return Distance( backEnd.viewParms.orientation.origin, lhs.origin ) - lhs.radius <
				   Distance( backEnd.viewParms.orientation.origin, rhs.origin ) - rhs.radius;
		} );
	for ( const drawSurfBoundingSphere& sphere : portalBounds ) {
		R_MirrorViewBySurface( &portalSurfaces[sphere.drawSurfID] );
	}
}

void MaterialSystem::RenderMaterials( const shaderSort_t fromSort, const shaderSort_t toSort ) {
	if ( !r_drawworld->integer ) {
		return;
	}

	if ( frameStart ) {
		renderedMaterials.clear();
		UpdateDynamicSurfaces();
		frameStart = false;
	}

	materialsSSBO.BindBufferBase();

	for ( MaterialPack& materialPack : materialPacks ) {
		if ( materialPack.fromSort >= fromSort && materialPack.toSort <= toSort ) {
			for ( Material& material : materialPack.materials ) {
				RenderMaterial( material );
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
			if ( skyShader->noFog != noFogPass ) {
				continue;
			}

			tr.drawingSky = true;
			Tess_Begin( Tess_StageIteratorSky, skyShader, nullptr, false, -1, 0, false );
			Tess_End();
		}
	}
}

void MaterialSystem::RenderMaterial( Material& material ) {
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

	switch ( material.stageType ) {
		case stageType_t::ST_COLORMAP:
		case stageType_t::ST_STYLELIGHTMAP:
		case stageType_t::ST_STYLECOLORMAP:
			BindShaderGeneric( &material );

			if ( material.tcGenEnvironment || material.vboVertexSprite ) {
				gl_genericShaderMaterial->SetUniform_ViewOrigin( backEnd.orientation.viewOrigin );
				gl_genericShaderMaterial->SetUniform_ViewUp( backEnd.orientation.axis[2] );
			}

			gl_genericShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
			gl_genericShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
			break;
		case stageType_t::ST_LIGHTMAP:
		case stageType_t::ST_DIFFUSEMAP:
		case stageType_t::ST_COLLAPSE_COLORMAP:
		case stageType_t::ST_COLLAPSE_DIFFUSEMAP:
			BindShaderLightMapping( &material );
			if ( tr.world ) {
				gl_lightMappingShaderMaterial->SetUniform_LightGridOrigin( tr.world->lightGridGLOrigin );
				gl_lightMappingShaderMaterial->SetUniform_LightGridScale( tr.world->lightGridGLScale );
			}
			// FIXME: else

			gl_lightMappingShaderMaterial->SetUniform_ViewOrigin( backEnd.orientation.viewOrigin );
			gl_lightMappingShaderMaterial->SetUniform_numLights( backEnd.refdef.numLights );
			gl_lightMappingShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
			gl_lightMappingShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
			break;
		case stageType_t::ST_LIQUIDMAP:
			BindShaderLiquid( &material );
			gl_liquidShaderMaterial->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );
			gl_liquidShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
			gl_liquidShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
			break;
		case stageType_t::ST_REFLECTIONMAP:
		case stageType_t::ST_COLLAPSE_REFLECTIONMAP:
			BindShaderReflection( &material );
			gl_reflectionShaderMaterial->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );
			gl_reflectionShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
			gl_reflectionShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
			break;
		case stageType_t::ST_REFRACTIONMAP:
		case stageType_t::ST_DISPERSIONMAP:
			// Not implemented yet
			break;
		case stageType_t::ST_SKYBOXMAP:
			BindShaderSkybox( &material );
			gl_skyboxShaderMaterial->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );
			gl_skyboxShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
			gl_skyboxShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
			break;
		case stageType_t::ST_SCREENMAP:
			BindShaderScreen( &material );
			gl_screenShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
			break;
		case stageType_t::ST_PORTALMAP:
			// This is supposedly used for alphagen portal and portal surfaces should never get here
			ASSERT_UNREACHABLE();
			break;
		case stageType_t::ST_HEATHAZEMAP:
			// FIXME: This requires 2 draws per surface stage rather than 1
			BindShaderHeatHaze( &material );

			if ( material.vboVertexSprite ) {
				gl_heatHazeShaderMaterial->SetUniform_ViewOrigin( backEnd.orientation.viewOrigin );
				gl_heatHazeShaderMaterial->SetUniform_ViewUp( backEnd.orientation.axis[2] );
			}

			gl_heatHazeShaderMaterial->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
			gl_heatHazeShaderMaterial->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[glState.stackIndex] );
			break;
		default:
			break;
	}

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

	glMultiDrawElementsIndirect( GL_TRIANGLES, GL_UNSIGNED_INT,
		BUFFER_OFFSET( material.staticCommandOffset * sizeof( GLIndirectBuffer::GLIndirectCommand ) ),
		material.drawCommands.size(), 0 );

	if ( material.usePolygonOffset ) {
		glDisable( GL_POLYGON_OFFSET_FILL );
	}
}
