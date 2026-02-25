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
// CoreThreadMemory.h

#ifndef CORE_THREAD_MEMORY_H
#define CORE_THREAD_MEMORY_H

#include "../../Math/NumberTypes.h"

#include "../../Sync/AlignedAtomic.h"

#include "../../Thread/ThreadCommon.h"

#include "../Decls.h"

#include "../Queue.h"
#include "../Semaphore.h"
#include "../GraphicsCoreStore.h"

static constexpr uint32 maxExecCmdBuffers = 4;

struct ExecCmdPool {
	VkCommandPool   cmdPool;
	VkCommandBuffer cmds[maxExecCmdBuffers];
	uint64          executionPhase[maxExecCmdBuffers];
	uint8           allocState;
};

struct GraphicsCoreMemory {
	VkCommandPool graphicsCmdPool;
	VkCommandPool computeCmdPool;
	VkCommandPool transferCmdPool;
	VkCommandPool sparseCmdPool;

	ExecCmdPool   execGraphicsCmd;
	ExecCmdPool   execComputeCmd;
	ExecCmdPool   execTransferCmd;
	ExecCmdPool   execSparseCmd;
};

void InitCmdPools();
void InitExecCmdPools();

void FreeCmdPools();
void FreeExecCmdPools();

constexpr uint32              maxThreadCmdBuffers = 64;

extern                 uint64 cmdBufferStates[MAX_THREADS];
extern    thread_local uint64 cmdBufferAllocState;

extern    VkCommandBuffer     cmdBuffers[MAX_THREADS][maxThreadCmdBuffers];
extern    uint64              swapchainCmdBuffers[MAX_THREADS];
extern    VkFence             cmdBufferFences[MAX_THREADS][maxThreadCmdBuffers];

extern    thread_local GraphicsCoreMemory GMEM;

#endif // CORE_THREAD_MEMORY_H