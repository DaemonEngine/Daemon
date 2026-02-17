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
// Queue.h

#include "../Memory/Array.h"

#include "Vulkan.h"

#include "GraphicsCoreStore.h"

#include "Queue.h"

static void InitQueue( Queue& queue, const bool downloadQueue ) {
	if ( !downloadQueue && queue.queue || downloadQueue && queue.queueDownload ) {
		return;
	}

	VkDeviceQueueInfo2 info {
		.queueFamilyIndex = queue.id,
		.queueIndex       = downloadQueue ? 1u : 0u
	};

	vkGetDeviceQueue2( device, &info, downloadQueue ? &queue.queueDownload : &queue.queue );
}

void InitQueueConfigs( const VkPhysicalDevice& device ) {
	VkQueueFamilyProperties2 propertiesArray[8] {};
	uint32 count;
	vkGetPhysicalDeviceQueueFamilyProperties2( device, &count, propertiesArray );

	for ( uint32 i = 0; i < count; i++ ) {
		VkQueueFamilyProperties& coreProperties = propertiesArray[i].queueFamilyProperties;

		Queue queue {
			.id                          = i,
			.type                        = ( QueueType ) coreProperties.queueFlags,
			.queueCount                  = coreProperties.queueCount,
			.timestampValidBits          = coreProperties.timestampValidBits,
			.minImageTransferGranularity = coreProperties.minImageTransferGranularity
		};

		if (         queue.type & GRAPHICS  ) {
			graphicsQueue        = queue;
			graphicsQueue.unique = true;
		} else if ( ( queue.type & COMPUTE  ) && !computeQueue.queueCount ) {
			computeQueue         = queue;
			computeQueue.unique  = true;
		} else if ( ( queue.type & TRANSFER ) && !transferQueue.queueCount ) {
			transferQueue        = queue;
			transferQueue.unique = true;
		} else if ( ( queue.type & SPARSE   ) && !sparseQueue.queueCount ) {
			sparseQueue          = queue;
			sparseQueue.unique   = true;
		}
	}

	if ( !computeQueue.queueCount ) {
		computeQueue         = graphicsQueue;
		computeQueue.unique  = false;
	}

	if ( !transferQueue.queueCount ) {
		transferQueue        = graphicsQueue;
		transferQueue.unique = false;
	}

	if ( !sparseQueue.queueCount ) {
		sparseQueue          = graphicsQueue;
		sparseQueue.unique   = false;
	}
}

void InitQueues() {
	InitQueue( graphicsQueue, false );
	InitQueue( computeQueue,  false );
	InitQueue( transferQueue, false );
	InitQueue( transferQueue, true  );
	InitQueue( sparseQueue,   false );
}

Array<uint32, 4> GetConcurrentQueues( uint32* count ) {
	Array<uint32, 4> queues { graphicsQueue.id };
	uint32 i = 1;

	if ( computeQueue.unique ) {
		queues[i] = computeQueue.id;
		i++;
	}

	if ( transferQueue.unique ) {
		queues[i] = transferQueue.id;
		i++;
	}

	if ( sparseQueue.unique ) {
		queues[i] = sparseQueue.id;
		i++;
	}

	*count = i;

	return queues;
}