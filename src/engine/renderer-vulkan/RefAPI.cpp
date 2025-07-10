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

#include "Thread/TaskList.h"

Cvar::Modified<Cvar::Cvar<bool>> r_fullscreen( "r_fullscreen", "use full-screen window", CVAR_ARCHIVE, true );

cvar_t* r_allowResize;

struct SDL_Window* window;

namespace TempAPI {
	void Shutdown( bool destroyWindow ) {
		Q_UNUSED( destroyWindow );
	}

	bool BeginRegistration( glconfig_t*, glconfig2_t* ) {
		taskList.Init();
		return true;
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
	re.RegisterModel = nullptr;

	re.RegisterSkin = nullptr;
	re.RegisterShader = nullptr;

	re.LoadWorld = nullptr;
	re.SetWorldVisData = nullptr;
	re.EndRegistration = nullptr;

	re.BeginFrame = nullptr;
	re.EndFrame = nullptr;

	re.MarkFragments = nullptr;

	re.LerpTag = nullptr;

	re.ModelBounds = nullptr;

	re.ClearScene = nullptr;
	re.AddRefEntityToScene = nullptr;

	re.AddPolyToScene = nullptr;
	re.AddPolysToScene = nullptr;
	re.LightForPoint = nullptr;

	re.AddLightToScene = nullptr;
	re.AddAdditiveLightToScene = nullptr;

	re.RenderScene = nullptr;

	re.SetColor = nullptr;
	re.SetClipRegion = nullptr;
	re.DrawStretchPic = nullptr;

	re.DrawRotatedPic = nullptr;
	re.Add2dPolys = nullptr;
	re.ScissorEnable = nullptr;
	re.ScissorSet = nullptr;
	re.DrawStretchPicGradient = nullptr;
	re.SetMatrixTransform = nullptr;
	re.ResetMatrixTransform = nullptr;

	re.Glyph = nullptr;
	re.GlyphChar = nullptr;
	re.RegisterFont = nullptr;
	re.UnregisterFont = nullptr;

	re.RemapShader = nullptr;
	re.GetEntityToken = nullptr;
	re.inPVS = nullptr;
	re.inPVVS = nullptr;
	// Q3A END

	// XreaL BEGIN
	re.TakeVideoFrame = nullptr;

	re.RegisterAnimation = nullptr;
	re.CheckSkeleton = nullptr;
	re.BuildSkeleton = nullptr;
	re.BlendSkeleton = nullptr;
	re.BoneIndex = nullptr;
	re.AnimNumFrames = nullptr;
	re.AnimFrameRate = nullptr;

	// XreaL END

	re.RegisterVisTest = nullptr;
	re.AddVisTestToScene = nullptr;
	re.CheckVisibility = nullptr;
	re.UnregisterVisTest = nullptr;

	re.SetColorGrading = nullptr;

	re.SetAltShaderTokens = nullptr;

	re.GetTextureSize = nullptr;
	re.Add2dPolysIndexed = nullptr;
	re.GenerateTexture = nullptr;
	re.ShaderNameFromHandle = nullptr;
	re.SendBotDebugDrawCommands = nullptr;

	return &re;
}