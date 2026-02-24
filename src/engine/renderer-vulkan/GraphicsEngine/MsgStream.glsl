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
} push;

void PushMsg( const uint id, const uint msg ) {
	push.msgStreamWrite.msgStream[id + 1] = msg;
}

void PushMsg( const uint id, const float msg ) {
	push.msgStreamWrite.msgStream[id + 1] = floatBitsToUint( msg );
}

void PushMsg( const uint id, const bool msg ) {
	push.msgStreamWrite.msgStream[id + 1] = msg ? 1 : 0;
}

Image2D rgba16f swapchain 1.0f nomips testImg;
Image2D rgba16f swapchain 1.0f testImg2;
Image2D rgba16f 1 1 testImg3;
Image3D rgba16f 1 1 5 testImg4;
ImageCube rgba16f 1 1 5 testImg5;

void main() {
	const uint globalGroupID      = GLOBAL_GROUP_ID;
	const uint globalInvocationID = GLOBAL_INVOCATION_ID;

	if ( globalInvocationID >= 64 ) {
		return;
	}

	const uint initMsg = push.msgStreamRead.msgStream[0];

	uint msgCount = 0;

	if ( initMsg == ENGINE_INIT && globalInvocationID < imageCount ) {
		const uint msgOffset = subgroupExclusiveAdd( 9 );
		
		PushMsg( msgOffset,     CORE_ALLOC_IMAGE );
		PushMsg( msgOffset + 1, imageConfigs[globalInvocationID].id );
		PushMsg( msgOffset + 2, imageConfigs[globalInvocationID].format );
		PushMsg( msgOffset + 3, imageConfigs[globalInvocationID].relativeSize );
		PushMsg( msgOffset + 4, imageConfigs[globalInvocationID].width );
		PushMsg( msgOffset + 5, imageConfigs[globalInvocationID].height );
		PushMsg( msgOffset + 6, imageConfigs[globalInvocationID].depth );
		PushMsg( msgOffset + 7, imageConfigs[globalInvocationID].useMips );
		PushMsg( msgOffset + 8, imageConfigs[globalInvocationID].cube );

		msgCount++;
	}

	const uint totalMsgs = subgroupAdd( msgCount );

	if ( subgroupElect() ) {
		push.msgStreamWrite.msgStream[0] = totalMsgs;
	}
}