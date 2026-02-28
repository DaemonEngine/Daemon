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

/* MsgStream.glsl */

#include "Common.glsl"

#include "MsgStreamAPI.h"

#include "Images.glsl"

#include "Buffers.glsl"

layout ( local_size_x = 64, local_size_y = 1, local_size_z = 1 ) in;

BufferRS restrict MsgStreamRead {
	uint msgStream[];
};

BufferWS restrict MsgStreamWrite {
	uint msgStream[];
};

layout ( scalar, push_constant ) uniform Push {
	MsgStreamRead  msgStreamRead;
	MsgStreamWrite msgStreamWrite;
	MsgStreamWrite buf2;
	MsgStreamWrite buf3;
	MsgStreamWrite buf4;
} push;

void PushMsg( inout uint id, const uint msg ) {
	push.msgStreamWrite.msgStream[id + 1] = msg;

	id++;
}

void PushMsg( inout uint id, const float msg ) {
	push.msgStreamWrite.msgStream[id + 1] = floatBitsToUint( msg );

	id++;
}

void PushMsg( inout uint id, const bool msg ) {
	push.msgStreamWrite.msgStream[id + 1] = msg ? 1 : 0;

	id++;
}

Image2D rgba16f swapchain 1.0f nomips testImg;
Image2D rgba16f swapchain 1.0f testImg2;
Image2D rgba16f 1 1 testImg3;
Image3D rgba16f 1 1 5 testImg4;
ImageCube rgba16f 1 1 5 testImg5;

Buffer 566 3 buf1;
Buffer 5667 30 buf2;
Buffer 5664545 0 buf3;
Buffer 5664545 buf4;
Buffer 5664545 buf5;

void main() [[maximally_reconverges]] {
	const uint globalGroupID      = GLOBAL_GROUP_ID;
	const uint globalInvocationID = GLOBAL_INVOCATION_ID;

	if ( globalInvocationID >= 64 ) {
		return;
	}

	const uint initImgMsg = push.msgStreamRead.msgStream[0];

	uint msgCount  = 0;

	uint msgID     = 0;

	if ( initImgMsg == ENGINE_INIT && globalInvocationID < imageCount ) {
		msgID     += subgroupExclusiveAdd( 9 );
		
		PushMsg( msgID, CORE_ALLOC_IMAGE );
		PushMsg( msgID, imageConfigs[globalInvocationID].id );
		PushMsg( msgID, imageConfigs[globalInvocationID].format );
		PushMsg( msgID, imageConfigs[globalInvocationID].relativeSize );
		PushMsg( msgID, imageConfigs[globalInvocationID].width );
		PushMsg( msgID, imageConfigs[globalInvocationID].height );
		PushMsg( msgID, imageConfigs[globalInvocationID].depth );
		PushMsg( msgID, imageConfigs[globalInvocationID].useMips );
		PushMsg( msgID, imageConfigs[globalInvocationID].cube );

		msgCount++;
	}

	msgID = imageCount * 9;

	if ( initImgMsg == ENGINE_INIT && globalInvocationID < bufferCount ) {
		msgID     += subgroupExclusiveAdd( 5 );
		
		PushMsg( msgID, CORE_ALLOC_BUFFER );
		PushMsg( msgID, bufferConfigs[globalInvocationID].id );
		PushMsg( msgID, bufferConfigs[globalInvocationID].relativeSize );
		PushMsg( msgID, bufferConfigs[globalInvocationID].size );
		PushMsg( msgID, bufferConfigs[globalInvocationID].usage );

		msgCount++;
	}

	msgID = imageCount * 9 + bufferCount * 5;

	if ( initImgMsg == ENGINE_INIT && globalInvocationID < SPIRVCount ) {
		msgID     += subgroupExclusiveAdd( 3 + SPIRVBufferConfigs[globalInvocationID].count );
		
		PushMsg( msgID, CORE_PUSH_BUFFER );
		PushMsg( msgID, SPIRVBufferConfigs[globalInvocationID].id );
		PushMsg( msgID, SPIRVBufferConfigs[globalInvocationID].count );

		for ( uint i = 0; i < SPIRVBufferConfigs[globalInvocationID].count; i++ ) {
			PushMsg( msgID, SPIRVBufferConfigs[globalInvocationID].buffers[i] );
		}

		msgCount++;
	}

	const uint totalMsgs = subgroupAdd( msgCount );

	if ( subgroupElect() ) {
		push.msgStreamWrite.msgStream[0] = totalMsgs;
	}
}