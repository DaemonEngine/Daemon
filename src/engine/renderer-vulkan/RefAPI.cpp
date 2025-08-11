/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2025 Daemon Developers
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
// RefAPI.cpp

#include "common/Common.h"
#include "qcommon/qcommon.h"

#include "RefAPI.h"
#include "Init.h"

#include "Thread/TaskList.h"

#include "Thread/ThreadMemory.h"

Cvar::Modified<Cvar::Cvar<bool>> r_fullscreen( "r_fullscreen", "use full-screen window", CVAR_ARCHIVE, true );

cvar_t* r_allowResize;

// struct SDL_Window* window;

#include "Surface/Surface.h"

Surface mainSurface;
SDL_Window* window;

namespace TempAPI {
	void Shutdown( bool destroyWindow ) {
		Q_UNUSED( destroyWindow );

		taskList.Shutdown();
		taskList.exitFence.Wait();
		taskList.FinishShutdown();
	}

	bool BeginRegistration( glconfig_t* glconfig, glconfig2_t* ) {
		TLM.main = true;
		taskList.Init();

		Init();

		glconfig->vidWidth = 1920;
		glconfig->vidHeight = 1080;

		window = mainSurface.window;

		IN_Init( window );
		return true;
	}

	qhandle_t RegisterModel( const char* ) {
		return 1;
	}

	qhandle_t RegisterSkin( const char* ) {
		return 1;
	}

	qhandle_t RegisterShader( const char*, int ) {
		return 1;
	}

	void LoadWorldMap( const char* ) {
	}

	void SetWorldVisData( const byte* ) {
	}

	void EndRegistration() {
	}

	void BeginFrame() {
	}

	void EndFrame( int*, int* ) {
	}

	int MarkFragments( int, const vec3_t*, const vec3_t, int, vec3_t, int, markFragment_t* ) {
		return 0;
	}

	int LerpTag( orientation_t*, const refEntity_t*, const char*, int ) {
		return 0;
	}

	void ModelBounds( qhandle_t, vec3_t, vec3_t ) {
	}

	void ClearScene() {
	}

	void AddRefEntityToScene( const refEntity_t* ) {
	}

	void AddPolyToScene( qhandle_t, int, const polyVert_t* ) {
	}

	void AddPolysToScene( qhandle_t, int, const polyVert_t*, int ) {
	}

	int LightForPoint( vec3_t, vec3_t, vec3_t, vec3_t ) {
		return 0;
	}

	void AddLightToScene( const vec3_t, float, float, float, float, float, qhandle_t, int ) {}
	void AddLightToSceneQ3A( const vec3_t, float, float, float, float ) {}

	void RenderScene( const refdef_t* ) {
	}

	void SetColor( const Color::Color& ) {
	}

	void SetClipRegion( const float* ) {
	}

	void DrawStretchPic( float, float, float, float, float, float, float, float, qhandle_t ) {
	}

	void DrawRotatedPic( float, float, float, float, float, float, float, float, qhandle_t, float ) {
	}

	void ScissorEnable( bool ) {
	}

	void ScissorSet( int, int, int, int ) {
	}

	void DrawStretchPicGradient( float, float, float, float, float, float, float, float, qhandle_t, const Color::Color&, int ) {
	}

	void Add2DPolyies( polyVert_t*, int, qhandle_t ) {
	}

	void SetMatrixTransform( const matrix_t ) {
	}

	void ResetMatrixTransform() {
	}

	void Glyph( fontInfo_t*, const char*, glyphInfo_t* glyph ) {
		glyph->height = 1;
		glyph->top = 1;
		glyph->bottom = 0;
		glyph->pitch = 1;
		glyph->xSkip = 1;
		glyph->imageWidth = 1;
		glyph->imageHeight = 1;
		glyph->s = 0.0f;
		glyph->t = 0.0f;
		glyph->s2 = 1.0f;
		glyph->t2 = 1.0f;
		glyph->glyph = 1;
		glyph->shaderName[0] = '\0';
	}

	void GlyphChar( fontInfo_t* font, int, glyphInfo_t* glyph ) {
		Glyph( font, nullptr, glyph );
	}

	fontInfo_t* RegisterFont( const char*, int ) {
		return nullptr;
	}

	void UnregisterFont( fontInfo_t* ) {
	}

	void RemapShader( const char*, const char*, const char* ) {
	}
	
	bool GetEntityToken( char*, int ) {
		return true;
	}

	bool InPVS( const vec3_t, const vec3_t ) {
		return false;
	}

	bool InPVVS( const vec3_t, const vec3_t ) {
		return false;
	}

	void TakeVideoFrame( int, int, byte*, byte*, bool ) {
	}

	int RegisterAnimation( const char* ) {
		return 1;
	}

	int CheckSkeleton( refSkeleton_t*, qhandle_t, qhandle_t ) {
		return 1;
	}

	int BuildSkeleton( refSkeleton_t* skel, qhandle_t, int, int, float, bool ) {
		skel->numBones = 0;
		return 1;
	}

	int BlendSkeleton( refSkeleton_t*, const refSkeleton_t*, float ) {
		return 1;
	}

	int BoneIndex( qhandle_t, const char* ) {
		return 0;
	}

	int AnimNumFrames( qhandle_t ) {
		return 1;
	}

	int AnimFrameRate( qhandle_t ) {
		return 1;
	}

	qhandle_t RegisterVisTest() {
		return 1;
	}

	void AddVisTestToScene( qhandle_t, const vec3_t, float, float ) {
	}

	float CheckVisibility( qhandle_t ) {
		return 0.0f;
	}

	void UnregisterVisTest( qhandle_t ) {
	}

	void SetColorGrading( int, qhandle_t ) {
	}

	void SetAltShaderTokens( const char* ) {
	}

	void GetTextureSize( int, int* width, int* height ) {
		*width = 1;
		*height = 1;
	}

	void Add2dPolysIndexed( polyVert_t*, int, int*, int, int, int, qhandle_t ) {
	}

	qhandle_t GenerateTexture( const byte*, int, int ) {
		return 1;
	}

	const char* ShaderNameFromHandle( qhandle_t ) {
		return "";
	}

	void SendBotDebugDrawCommands( std::vector<char> ) {
	}
}

refexport_t* GetRefAPI( int apiVersion, refimport_t* rimp ) {
	static refexport_t re;
	Q_UNUSED( rimp );

	Log::Debug( "GetRefAPI()" );

	re = {};

	if ( apiVersion != REF_API_VERSION )
	{
		Log::Notice( "Mismatched REF_API_VERSION: expected %i, got %i", REF_API_VERSION, apiVersion );
		return nullptr;
	}

	// the RE_ functions are Renderer Entry points

	// Q3A BEGIN
	re.Shutdown = TempAPI::Shutdown;

	re.BeginRegistration = TempAPI::BeginRegistration;
	re.RegisterModel = TempAPI::RegisterModel;

	re.RegisterSkin = TempAPI::RegisterSkin;
	re.RegisterShader = TempAPI::RegisterShader;

	re.LoadWorld = TempAPI::LoadWorldMap;
	re.SetWorldVisData = TempAPI::SetWorldVisData;
	re.EndRegistration = TempAPI::EndRegistration;

	re.BeginFrame = TempAPI::BeginFrame;
	re.EndFrame = TempAPI::EndFrame;

	re.MarkFragments = TempAPI::MarkFragments;

	re.LerpTag = TempAPI::LerpTag;

	re.ModelBounds = TempAPI::ModelBounds;

	re.ClearScene = TempAPI::ClearScene;
	re.AddRefEntityToScene = TempAPI::AddRefEntityToScene;

	re.AddPolyToScene = TempAPI::AddPolyToScene;
	re.AddPolysToScene = TempAPI::AddPolysToScene;
	re.LightForPoint = TempAPI::LightForPoint;

	re.AddLightToScene = TempAPI::AddLightToScene;
	re.AddAdditiveLightToScene = TempAPI::AddLightToSceneQ3A;

	re.RenderScene = TempAPI::RenderScene;

	re.SetColor = TempAPI::SetColor;
	re.SetClipRegion = TempAPI::SetClipRegion;
	re.DrawStretchPic = TempAPI::DrawStretchPic;

	re.DrawRotatedPic = TempAPI::DrawRotatedPic;
	re.Add2dPolys = TempAPI::Add2DPolyies;
	re.ScissorEnable = TempAPI::ScissorEnable;
	re.ScissorSet = TempAPI::ScissorSet;
	re.DrawStretchPicGradient = TempAPI::DrawStretchPicGradient;
	re.SetMatrixTransform = TempAPI::SetMatrixTransform;
	re.ResetMatrixTransform = TempAPI::ResetMatrixTransform;

	re.Glyph = TempAPI::Glyph;
	re.GlyphChar = TempAPI::GlyphChar;
	re.RegisterFont = TempAPI::RegisterFont;
	re.UnregisterFont = TempAPI::UnregisterFont;

	re.RemapShader = TempAPI::RemapShader;
	re.GetEntityToken = TempAPI::GetEntityToken;
	re.inPVS = TempAPI::InPVS;
	re.inPVVS = TempAPI::InPVVS;
	// Q3A END

	// XreaL BEGIN
	re.TakeVideoFrame = TempAPI::TakeVideoFrame;

	re.RegisterAnimation = TempAPI::RegisterAnimation;
	re.CheckSkeleton = TempAPI::CheckSkeleton;
	re.BuildSkeleton = TempAPI::BuildSkeleton;
	re.BlendSkeleton = TempAPI::BlendSkeleton;
	re.BoneIndex = TempAPI::BoneIndex;
	re.AnimNumFrames = TempAPI::AnimNumFrames;
	re.AnimFrameRate = TempAPI::AnimFrameRate;

	// XreaL END

	re.RegisterVisTest = TempAPI::RegisterVisTest;
	re.AddVisTestToScene = TempAPI::AddVisTestToScene;
	re.CheckVisibility = TempAPI::CheckVisibility;
	re.UnregisterVisTest = TempAPI::UnregisterVisTest;

	re.SetColorGrading = TempAPI::SetColorGrading;

	re.SetAltShaderTokens = TempAPI::SetAltShaderTokens;

	re.GetTextureSize = TempAPI::GetTextureSize;
	re.Add2dPolysIndexed = TempAPI::Add2dPolysIndexed;
	re.GenerateTexture = TempAPI::GenerateTexture;
	re.ShaderNameFromHandle = TempAPI::ShaderNameFromHandle;
	re.SendBotDebugDrawCommands = TempAPI::SendBotDebugDrawCommands;

	return &re;
}