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

inline size_t GetLightMapNum( const shaderCommands_t* tess )
{
	return tess->lightmapNum;
}

inline size_t GetLightMapNum( const MaterialSurface* surface )
{
	return surface->lightMapNum;
}

template<typename Obj> bool HasLightMap( Obj* obj )
{
	return static_cast<size_t>( GetLightMapNum( obj ) ) < tr.lightmaps.size();
}

template<typename Obj> image_t* GetLightMap( Obj* obj )
{
	if ( HasLightMap( obj ) )
	{
		return tr.lightmaps[ GetLightMapNum( obj ) ];
	}

	return tr.whiteImage;
}

template<typename Obj> bool HasDeluxeMap( Obj* obj )
{
	return static_cast<size_t>( GetLightMapNum( obj ) ) < tr.deluxemaps.size();
}

template<typename Obj> image_t* GetDeluxeMap( Obj* obj )
{
	if ( HasDeluxeMap( obj ) )
	{
		return tr.deluxemaps[ GetLightMapNum( obj ) ];
	}

	return tr.blackImage;
}

inline shader_t* GetSurfaceShader( shaderCommands_t* tess )
{
	return tess->surfaceShader;
}

inline shader_t* GetSurfaceShader( shader_t* shader )
{
	return shader;
}

template<typename Obj> static bool hasExplicitelyDisabledLightMap( Obj* obj )
{
	return GetSurfaceShader( obj )->surfaceFlags & SURF_NOLIGHTMAP;
}

inline shaderStage_t* GetSurfaceLastStage( shaderCommands_t* tess )
{
	return tess->surfaceLastStage;
}

inline shaderStage_t* GetSurfaceLastStage( shader_t* shader )
{
	return shader->lastStage;
}

inline shaderStage_t* GetSurfaceStages( shaderCommands_t* tess )
{
	return tess->surfaceStages;
}

inline shaderStage_t* GetSurfaceStages( shader_t* shader )
{
	return shader->stages;
}

template<typename Obj> bool isExplicitelyVertexLitSurface( Obj* obj )
{
	shaderStage_t* lastStage = GetSurfaceLastStage( obj );
	shaderStage_t* stages = GetSurfaceStages( obj );
	return lastStage != stages && stages[0].rgbGen == colorGen_t::CGEN_VERTEX;
}

template<typename Obj> void SetLightDeluxeMode(
	const Obj *obj, const shaderStage_t *stage,
	lightMode_t& lightMode, deluxeMode_t& deluxeMode )
{
	lightMode = lightMode_t::FULLBRIGHT;
	deluxeMode = deluxeMode_t::NONE;

	if ( stage->forceVertexLighting )
	{
		lightMode = lightMode_t::VERTEX;
		deluxeMode = deluxeMode_t::NONE;
	}
	else if ( hasExplicitelyDisabledLightMap( stage->shader ) && !isExplicitelyVertexLitSurface( stage->shader ) )
	{
		// Use fullbright on “surfaceparm nolightmap” materials.
	}
	else if ( stage->type == stageType_t::ST_COLLAPSE_COLORMAP )
	{
		/* Use fullbright for collapsed stages without lightmaps,
		for example:

		  {
		    map textures/texture_d
		    heightMap textures/texture_h
		  }

		This is doable for some complex multi-stage materials. */
	}
	else if( stage->type == stageType_t::ST_LIQUIDMAP )
	{
		lightMode = tr.modelLight;
		deluxeMode = tr.modelDeluxe;
	}
	else if ( obj->bspSurface )
	{
		lightMode = tr.worldLight;
		deluxeMode = tr.worldDeluxe;

		if ( lightMode == lightMode_t::MAP )
		{
			if ( !HasLightMap( obj ) )
			{
				lightMode = lightMode_t::VERTEX;
				deluxeMode = deluxeMode_t::NONE;
			}
		}
	}
	else
	{
		lightMode = tr.modelLight;
		deluxeMode = tr.modelDeluxe;
	}
}

template<typename Obj> image_t* SetLightMap( Obj* obj, lightMode_t lightMode )
{
	switch ( lightMode ) {
		case lightMode_t::VERTEX:
			break;

		case lightMode_t::GRID:
			return tr.lightGrid1Image;
			break;

		case lightMode_t::MAP:
			return GetLightMap( obj );
			break;

		default:
			break;
	}

	return tr.whiteImage;
}

template<typename Obj> image_t* SetDeluxeMap( Obj* obj, deluxeMode_t deluxeMode )
{
	switch ( deluxeMode ) {
		case deluxeMode_t::MAP:
			return GetDeluxeMap( obj );
			break;

		case deluxeMode_t::GRID:
			return tr.lightGrid2Image;
			break;

		default:
			break;
	}

	return tr.blackImage;
}

inline colorGen_t SetRgbGen( const shaderStage_t *pStage )
{
	switch ( pStage->rgbGen )
	{
		case colorGen_t::CGEN_VERTEX:
		case colorGen_t::CGEN_ONE_MINUS_VERTEX:
			return pStage->rgbGen;
			break;

		default:
			break;
	}

	return colorGen_t::CGEN_CONST;
}

inline alphaGen_t SetAlphaGen( const shaderStage_t *pStage )
{
	switch ( pStage->alphaGen )
	{
		case alphaGen_t::AGEN_VERTEX:
		case alphaGen_t::AGEN_ONE_MINUS_VERTEX:
			return pStage->alphaGen;
			break;

		default:
			break;
	}

	return alphaGen_t::AGEN_CONST;
}

inline void SetVertexLightingSettings( lightMode_t lightMode, colorGen_t& rgbGen )
{
	if ( lightMode == lightMode_t::VERTEX )
	{
		// Do not rewrite pStage->rgbGen.
		rgbGen = colorGen_t::CGEN_VERTEX;
		tess.svars.color.SetRed( 0.0f );
		tess.svars.color.SetGreen( 0.0f );
		tess.svars.color.SetBlue( 0.0f );
	}
}

inline uint GetShaderProfilerRenderSubGroupsMode( const uint32_t stateBits ) {
	const bool isOpaque = !( stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) );
	const bool modeOpaque = r_profilerRenderSubGroupsMode.Get() == Util::ordinal( shaderProfilerRenderSubGroupsMode::VS_OPAQUE )
		|| r_profilerRenderSubGroupsMode.Get() == Util::ordinal( shaderProfilerRenderSubGroupsMode::FS_OPAQUE );

	if ( r_profilerRenderSubGroupsMode.Get() == Util::ordinal( shaderProfilerRenderSubGroupsMode::VS_ALL )
		|| r_profilerRenderSubGroupsMode.Get() == Util::ordinal( shaderProfilerRenderSubGroupsMode::FS_ALL )
		|| isOpaque == modeOpaque ) {

		switch ( r_profilerRenderSubGroupsMode.Get() ) {
			case Util::ordinal( shaderProfilerRenderSubGroupsMode::VS_OPAQUE ):
			case Util::ordinal( shaderProfilerRenderSubGroupsMode::VS_TRANSPARENT ):
			case Util::ordinal( shaderProfilerRenderSubGroupsMode::VS_ALL ):
				return 1;
			case Util::ordinal( shaderProfilerRenderSubGroupsMode::FS_OPAQUE ):
			case Util::ordinal( shaderProfilerRenderSubGroupsMode::FS_TRANSPARENT ):
			case Util::ordinal( shaderProfilerRenderSubGroupsMode::FS_ALL ):
				return 2;
			default:
				break;
		}
	}

	return 0;
}
