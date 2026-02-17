/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2026 Daemon Developers
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
// ExecCmd.cpp

#include "../../Math/Bit.h"

#include "../Vulkan.h"

#include "../Memory/CoreThreadMemory.h"

#include "../Semaphore.h"

#include "ExecCmd.h"

static void GetExecCmdBuf( const QueueType queueType, VkCommandBuffer* cmd, Semaphore** signalSemaphore ) {
	ExecCmdPool* execCmd;

	switch ( queueType ) {
		case GRAPHICS:
		default:
			execCmd = &GMEM.execGraphicsCmd;
			break;
		case COMPUTE:
			execCmd = &GMEM.execComputeCmd;
			break;
		case TRANSFER:
			execCmd = &GMEM.execTransferCmd;
			break;
		case SPARSE:
			execCmd = &GMEM.execSparseCmd;
			break;
	}

	uint32 id = FindLZeroBit( execCmd->allocState );

	while ( true ) {
		if ( id < maxExecCmdBuffers ) {
			*cmd = execCmd->cmds[id];
			break;
		}

		bool success = false;
		for ( Semaphore& semaphore : execCmd->signalSemaphores ) {
			if ( semaphore.Wait( 0 ) ) {
				id   = &semaphore - execCmd->signalSemaphores;
				*cmd = execCmd->cmds[id];

				vkResetCommandBuffer( *cmd, 0 );

				success = true;
				break;
			}
		}

		if ( success ) {
			break;
		}

		std::this_thread::yield();
	}

	*signalSemaphore = &execCmd->signalSemaphores[id];

	SetBit( &execCmd->allocState, id );
}

ExecCmd::ExecCmd( const QueueType newQueueType, VkCommandBuffer* newCmd ) {
	queueType = newQueueType;
	GetExecCmdBuf( queueType, &cmd, &GMEM.execSemaphore );
	*newCmd   = cmd;

	VkCommandBufferBeginInfo cmdInfo {
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer( cmd, &cmdInfo );
}

ExecCmd::~ExecCmd() {
	vkEndCommandBuffer( cmd );

	VkQueue queue;

	switch ( queueType ) {
		case GRAPHICS:
			queue = graphicsQueue.queue;
			break;
		case COMPUTE:
			queue = computeQueue.queue;
			break;
		case TRANSFER:
			queue = transferQueue.queue;
			break;
		case SPARSE:
			queue = sparseQueue.queue;
			break;
	}

	VkCommandBufferSubmitInfo execCmdSubmitInfo {
		.commandBuffer = cmd
	};

	( *GMEM.execSemaphore )++;
	VkSemaphoreSubmitInfo signalSemaphoreInfo = GMEM.execSemaphore->GenSubmitInfo( VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT );

	VkSubmitInfo2 execSubmitInfo {
		.commandBufferInfoCount   = 1,
		.pCommandBufferInfos      = &execCmdSubmitInfo,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos    = &signalSemaphoreInfo
	};

	vkQueueSubmit2( queue, 1, &execSubmitInfo, nullptr );
}