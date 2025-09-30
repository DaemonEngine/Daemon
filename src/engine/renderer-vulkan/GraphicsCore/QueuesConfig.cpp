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

#include "Vulkan.h"

#include "../Memory/Array.h"

#include "QueuesConfig.h"

static const QueueConfig* GetQueueConfigForType( QueuesConfig& config, const QueueType& type ) {
	for ( const QueueConfig* cfg = config.queues; cfg < config.queues + config.count; cfg++ ) {
		if ( cfg->type & type ) {
			return cfg;
		}
	}

	return nullptr;
}

QueuesConfig GetQueuesConfigForDevice( const VkPhysicalDevice& device ) {
	QueuesConfig config { .count = 8 };

	VkQueueFamilyProperties2 propertiesArray[8] {};
	vkGetPhysicalDeviceQueueFamilyProperties2( device, &config.count, propertiesArray );

	for ( uint32 i = 0; i < config.count; i++ ) {
		QueueConfig* cfg = &config.queues[i];
		VkQueueFamilyProperties& coreProperties = propertiesArray[i].queueFamilyProperties;
		
		cfg->id = i;

		cfg->type = ( QueueType ) coreProperties.queueFlags;
		cfg->queues = coreProperties.queueCount;
		cfg->timestampValidBits = coreProperties.timestampValidBits;
		cfg->minImageTransferGranularity = coreProperties.minImageTransferGranularity;

		if (          cfg->type & GRAPHICS ) {
			config.graphicsQueue        = *cfg;
			config.graphicsQueue.unique = true;
		} else if ( ( cfg->type & COMPUTE )  && cfg->queues > config.computeQueue.queues  ) {
			config.computeQueue         = *cfg;
			config.computeQueue.unique  =  true;
		} else if ( ( cfg->type & TRANSFER ) && cfg->queues > config.transferQueue.queues ) {
			config.transferQueue        = *cfg;
			config.transferQueue.unique = true;
		} else if ( ( cfg->type & SPARSE )   && cfg->queues > config.sparseQueue.queues   ) {
			config.sparseQueue          = *cfg;
			config.sparseQueue.unique   =   true;
		}
	}

	if ( !config.computeQueue.queues ) {
		config.computeQueue         = *GetQueueConfigForType( config, COMPUTE  );
		config.computeQueue.unique  = false;
	}

	if ( !config.transferQueue.queues ) {
		config.transferQueue        = *GetQueueConfigForType( config, TRANSFER );
		config.transferQueue.unique = false;
	}

	if ( !config.sparseQueue.queues ) {
		config.sparseQueue          = *GetQueueConfigForType( config, SPARSE   );
		config.sparseQueue.unique   = false;
	}

	return config;
}