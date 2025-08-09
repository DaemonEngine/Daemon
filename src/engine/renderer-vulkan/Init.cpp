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
// Init.cpp

#include "common/Common.h"
#include "qcommon/qcommon.h"

#include "engine/framework/System.h"

#include "Thread/ThreadMemory.h"
#include "Thread/TaskList.h"
#include "Memory/MemoryChunkSystem.h"
#include "MiscCVarStore.h"
#include "RefAPI.h"

void Init() {
	/* TLM.main = true;
	taskList.Init();

	glconfig->vidWidth = 1920;
	glconfig->vidHeight = 1080;

	r_width.Set( 1920 );
	r_height.Set( 1080 );

	window = mainSurface.window;

	IN_Init( window ); */

	// memoryChunkSystem.InitConfig( r_vkMemoryChunkConfig.Get().c_str() );
	std::string cfg = r_vkMemoryChunkConfig.Get();
	Task task { &InitMemoryChunkSystemConfig, cfg };
	taskList.AddTask( task );
}