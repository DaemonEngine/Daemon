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

#include "../Memory/CoreThreadMemory.h"

#include "../QueuesConfig.h"
#include "../Semaphore.h"

#include "ExecCmd.h"

Semaphore& GetInstantCmdBuf( const QueueType queueType, VkCommandBuffer* cmd ) {
	InstantCmdPool* instantCmd;

	switch ( queueType ) {
		case GRAPHICS:
		default:
			instantCmd = &GMEM.instantGraphicsCmd;
			break;
		case COMPUTE:
			instantCmd = &GMEM.instantComputeCmd;
			break;
		case TRANSFER:
			instantCmd = &GMEM.instantTransferCmd;
			break;
		case SPARSE:
			instantCmd = &GMEM.instantSparseCmd;
			break;
	}

	uint32 id = FindLZeroBit( instantCmd->allocState );

	while ( true ) {
		if ( id < maxInstantCmdBuffers ) {
			*cmd = instantCmd->cmds[id];
			break;
		}

		bool success = false;
		for ( Semaphore& semaphore : instantCmd->signalSemaphores ) {
			if ( semaphore.Wait( 0 ) ) {
				id   = &semaphore - instantCmd->signalSemaphores;
				*cmd = instantCmd->cmds[id];

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

	SetBit( &instantCmd->allocState, id );

	return instantCmd->signalSemaphores[id];
}

Semaphore& ExecCmd( const QueueType queueType, CmdFunction func ) {
	VkCommandBuffer cmd;

	Semaphore& semaphore = GetInstantCmdBuf( queueType, &cmd );

	VkCommandBufferBeginInfo cmdInfo {
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer( cmd, &cmdInfo );

	func( cmd );

	vkEndCommandBuffer( cmd );

	VkQueue queue;

	switch ( queueType ) {
		case GRAPHICS:
			queue = graphicsQueue.queues[0];
			break;
		case COMPUTE:
			queue = computeQueue.queues[0];
			break;
		case TRANSFER:
			queue = transferQueue.queues[0];
			break;
		case SPARSE:
			queue = sparseQueue.queues[0];
			break;
	}

	VkCommandBufferSubmitInfo instantCmdSubmitInfo {
		.commandBuffer = cmd
	};

	semaphore++;
	VkSemaphoreSubmitInfo instantSemaphoreInfo = semaphore.GenSubmitInfo( VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT );

	VkSubmitInfo2 instantSubmitInfo {
		.commandBufferInfoCount   = 1,
		.pCommandBufferInfos      = &instantCmdSubmitInfo,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos    = &instantSemaphoreInfo
	};

	vkQueueSubmit2( queue, 1, &instantSubmitInfo, nullptr );

	return semaphore;
}