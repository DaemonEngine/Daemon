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

#include "engine/qcommon/qcommon.h"

#include "../Shared/Timer.h"

#include "../Error.h"

#include "../Thread/TaskList.h"
#include "../Sync/Fence.h"

#include "GraphicsCoreStore.h"
#include "Instance.h"

#include "PhysicalDevice.h"

#include "EngineConfig.h"

#include "Memory/DescriptorSet.h"
#include "Memory/EngineAllocator.h"
#include "ResourceSystem.h"

#include "Init.h"

#include "SwapChain.h"
#include "Vulkan.h"

#include "Memory/CoreThreadMemory.h"
void InitGraphicsEngine() {
	instance.Init( "Daemon-vulkan", CLIENT_WINDOW_TITLE );

	std::string foundQueues = "Found queues: graphics (present: true)";
	graphicsQueue.Init( device, queuesConfig.graphicsQueue.id, queuesConfig.graphicsQueue.queueCount );

	uint32 presentSupported;
	vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, queuesConfig.graphicsQueue.id, mainSwapChain.surface, &presentSupported );

	if ( !presentSupported ) {
		Err( "Graphics queue doesn't support present" );
		return;
	}

	if ( queuesConfig.computeQueue.unique ) {
		computeQueue.Init( device, queuesConfig.computeQueue.id, queuesConfig.computeQueue.queueCount );
		vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, queuesConfig.computeQueue.id, mainSwapChain.surface, &presentSupported );
		foundQueues += Str::Format( ", async compute (present: %s)", ( bool ) presentSupported );
	}

	if ( queuesConfig.transferQueue.unique ) {
		transferQueue.Init( device, queuesConfig.transferQueue.id, queuesConfig.transferQueue.queueCount );
		foundQueues += Str::Format( ", async transfer" );
	}

	if ( queuesConfig.sparseQueue.unique ) {
		sparseQueue.Init( device, queuesConfig.sparseQueue.id, queuesConfig.sparseQueue.queueCount );
		foundQueues += Str::Format( ", async sparse binding" );
	}

	Log::Notice( foundQueues );

	AllocDescriptors( engineConfig.maxImages, engineConfig.maxStorageImages );

	engineAllocator.Init();

	InitCmdPools();

	FenceMain initExecCmdFence;
	Task      initExecCmdTask { &InitExecCmdPools, initExecCmdFence };

	taskList.AddTask( initExecCmdTask.ThreadMaskAllOthers() );

	InitExecCmdPools();

	initExecCmdFence.Wait();
}