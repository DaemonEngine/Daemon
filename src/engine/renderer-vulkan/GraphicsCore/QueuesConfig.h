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
// QueuesConfig.h

#ifndef QUEUES_CONFIG_H
#define QUEUES_CONFIG_H

#include "Vulkan.h"

#include "../Math/NumberTypes.h"
#include "../Memory/IteratorSeq.h"

enum QueueType {
	GRAPHICS = VK_QUEUE_GRAPHICS_BIT,
	COMPUTE  = VK_QUEUE_COMPUTE_BIT,
	TRANSFER = VK_QUEUE_TRANSFER_BIT,
	SPARSE   = VK_QUEUE_SPARSE_BINDING_BIT
};

struct QueueConfig {
	uint32 id;
	bool unique;

	QueueType type;
	uint32 queues = 0;
	uint32 timestampValidBits;
	VkExtent3D minImageTransferGranularity;
};

struct QueuesConfig {
	QueueConfig graphicsQueue;
	QueueConfig computeQueue;
	QueueConfig transferQueue;
	QueueConfig sparseQueue;

	uint32 count;
	QueueConfig queues[8];

	constexpr QueueConfig& operator[]( const uint32 index ) {
		return queues[index];
	}

	constexpr IteratorSeq<QueueConfig> begin() {
		return IteratorSeq<QueueConfig>{ &queues[0] };
	}

	constexpr IteratorSeq<QueueConfig> end() {
		return IteratorSeq<QueueConfig>{ &queues[count] };
	}
};

QueuesConfig GetQueuesConfigForDevice( const VkPhysicalDevice& device );

#endif // QUEUES_CONFIG_H