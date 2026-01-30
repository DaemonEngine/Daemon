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
// CoreThreadMemory.cpp

#include "../../Math/Bit.h"

#include "../QueuesConfig.h"

#include "CoreThreadMemory.h"

void InitCmdPools() {
	struct QueuePool {
		QueueConfig* cfg;
		VkCommandPool* cmdPool;
	};

	QueuePool queues[] {
		{ &queuesConfig.graphicsQueue, &GMEM.graphicsCmdPool },
		{ &queuesConfig.computeQueue,  &GMEM.computeCmdPool  },
		{ &queuesConfig.transferQueue, &GMEM.transferCmdPool },
		{ &queuesConfig.sparseQueue,   &GMEM.sparseCmdPool   },
	};

	for ( QueuePool& queuePool : queues ) {
		if ( queuePool.cfg->unique ) {
			VkCommandPoolCreateInfo cmdPoolInfo {
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = queuePool.cfg->id
			};

			vkCreateCommandPool( device, &cmdPoolInfo, nullptr, queuePool.cmdPool );
		}
	}
}

void InitInstantCmdPools() {
	struct QueueCmdPool {
		QueueConfig*    cfg;
		InstantCmdPool* cmdPool;
	};

	QueueCmdPool instantQueues[] {
		{ &queuesConfig.graphicsQueue, &GMEM.instantGraphicsCmd, },
		{ &queuesConfig.computeQueue,  &GMEM.instantComputeCmd,  },
		{ &queuesConfig.transferQueue, &GMEM.instantTransferCmd, },
		{ &queuesConfig.sparseQueue,   &GMEM.instantSparseCmd,   },
	};

	for ( QueueCmdPool& queuePool : instantQueues ) {
		if ( queuePool.cfg->unique ) {
			VkCommandPoolCreateInfo cmdPoolInfo {
				.flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
				.queueFamilyIndex   = queuePool.cfg->id
			};

			vkCreateCommandPool( device, &cmdPoolInfo, nullptr, &queuePool.cmdPool->cmdPool );

			VkCommandBufferAllocateInfo cmdInfo {
				.commandPool        = queuePool.cmdPool->cmdPool,
				.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = maxInstantCmdBuffers
			};

			vkAllocateCommandBuffers( device, &cmdInfo, queuePool.cmdPool->cmds );

			for ( Semaphore& semaphore : queuePool.cmdPool->signalSemaphores ) {
				semaphore.Init();
			}
		}
	}
}

void FreeCmdPools() {
	vkDestroyCommandPool( device, GMEM.graphicsCmdPool, nullptr );
	vkDestroyCommandPool( device, GMEM.computeCmdPool,  nullptr );
	vkDestroyCommandPool( device, GMEM.transferCmdPool, nullptr );
	vkDestroyCommandPool( device, GMEM.sparseCmdPool,   nullptr );

	vkDestroyCommandPool( device, GMEM.instantGraphicsCmd.cmdPool, nullptr );
	vkDestroyCommandPool( device, GMEM.instantComputeCmd.cmdPool,  nullptr );
	vkDestroyCommandPool( device, GMEM.instantTransferCmd.cmdPool, nullptr );
	vkDestroyCommandPool( device, GMEM.instantSparseCmd.cmdPool,   nullptr );
}

AlignedAtomicUint64 cmdBufferStates[MAX_THREADS];
thread_local uint64 cmdBufferAllocState;

VkCommandBuffer     cmdBuffers[MAX_THREADS][maxThreadCmdBuffers];
VkFence             cmdBufferFences[MAX_THREADS][maxThreadCmdBuffers];

thread_local GraphicsCoreMemory GMEM;