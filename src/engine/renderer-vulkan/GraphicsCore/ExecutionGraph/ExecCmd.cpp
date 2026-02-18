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

static ExecCmdPool* GetExecCmdPoolByType( const QueueType type ) {
	switch ( type ) {
		case GRAPHICS:
			return &GMEM.execGraphicsCmd;
		case COMPUTE:
			return &GMEM.execComputeCmd;
		case TRANSFER:
			return &GMEM.execTransferCmd;
		case SPARSE:
			return &GMEM.execSparseCmd;
		default:
			ASSERT_UNREACHABLE();
	}
}

static uint32 GetExecCmdBuf( const QueueType queueType, VkCommandBuffer* cmd ) {
	ExecCmdPool* execCmd = GetExecCmdPoolByType( queueType );

	Queue& queue = GetQueueByType( queueType );

	uint32 id = FindLZeroBit( execCmd->allocState );

	while ( true ) {
		if ( id < maxExecCmdBuffers ) {
			*cmd = execCmd->cmds[id];
			break;
		}

		id = maxExecCmdBuffers;

		uint64 queueExecutionPhase = queue.executionPhase.Current();

		for ( uint32 i = 0; i < maxExecCmdBuffers; i++ ) {
			if ( execCmd->executionPhase[i] <= queueExecutionPhase ) {
				id   = i;
				*cmd = execCmd->cmds[i];

				vkResetCommandBuffer( *cmd, 0 );

				return id;
			}
		}

		if ( id == maxExecCmdBuffers ) {
			std::this_thread::yield();
		}
	}

	SetBit( &execCmd->allocState, id );

	return id;
}

ExecCmd::ExecCmd( const QueueType newQueueType, VkCommandBuffer* newCmd ) {
	queueType = newQueueType;
	cmdID     = GetExecCmdBuf( queueType, &cmd );
	*newCmd   = cmd;

	VkCommandBufferBeginInfo cmdInfo {
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer( cmd, &cmdInfo );
}

ExecCmd::~ExecCmd() {
	vkEndCommandBuffer( cmd );

	GetExecCmdPoolByType( queueType )->executionPhase[cmdID] = GetQueueByType( queueType ).Submit( cmd );
}