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
// Instance.cpp

// #define VK_NO_PROTOTYPES

#include <SDL3/SDL_vulkan.h>

#include "../Math/NumberTypes.h"
#include "../Memory/DynamicArray.h"

#include "Vulkan.h"
#include "../VulkanLoader/VulkanLoadFunctions.h"

#include "CapabilityPack.h"
#include "EngineConfig.h"
#include "QueuesConfig.h"
#include "PhysicalDevice.h"
#include "Queue.h"

#include "GraphicsCoreStore.h"

#include "Instance.h"

Instance coreInstance;

void Instance::Init( const char* engineName, const char* appName ) {
	VkApplicationInfo appInfo{
		.pApplicationName = appName,
		.applicationVersion = 0,
		.pEngineName = engineName,
		.engineVersion = 0,
		.apiVersion = VK_MAKE_API_VERSION( 0, 1, 3, 0 )
	};

	uint32 count;
	const char* const* sdlext = SDL_Vulkan_GetInstanceExtensions( &count );

	for ( int i = 0; i < count; i++ ) {
		Log::Notice( sdlext[i] );
	}

	VulkanLoaderInit();

	uint32 extCount;
	vkEnumerateInstanceExtensionProperties( nullptr, &extCount, nullptr );

	DynamicArray<VkExtensionProperties> availableExtensions;
	availableExtensions.Resize( extCount );

	vkEnumerateInstanceExtensionProperties( nullptr, &extCount, availableExtensions.memory );

	DynamicArray<const char*> extensions;
	extensions.Resize( count + capabilityPackMinimal.requiredInstanceExtensions.size );
	memcpy( extensions.memory, sdlext, count * sizeof( const char* ) );
	memcpy( extensions.memory + count, capabilityPackMinimal.requiredInstanceExtensions.memory,
		capabilityPackMinimal.requiredInstanceExtensions.size * sizeof( const char* ) );

	VkInstanceCreateInfo createInfo {
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = count, // ( uint32 ) extensions.elements,
		.ppEnabledExtensionNames = sdlext // extensions.memory
	};

	VkResult res = vkCreateInstance( &createInfo, nullptr, &instance );

	VulkanLoadInstanceFunctions( instance );

	uint32 deviceCount;
	vkEnumeratePhysicalDevices( instance, &deviceCount, nullptr );

	DynamicArray<VkPhysicalDevice> availableDevices;
	availableDevices.Resize( deviceCount );

	vkEnumeratePhysicalDevices( instance, &deviceCount, availableDevices.memory );

	VkPhysicalDeviceFeatures2 f {};
	VkPhysicalDeviceProperties2 p {};

	vkGetPhysicalDeviceFeatures2( availableDevices[1], &f );
	vkGetPhysicalDeviceProperties2( availableDevices[1], &p );

	if ( !SelectPhysicalDevice( availableDevices, &engineConfig, &physicalDevice ) ) {
		return;
	}

	QueuesConfig queuesConfig = GetQueuesConfigForDevice( physicalDevice );

	CreateDevice( physicalDevice, engineConfig, queuesConfig,
		capabilityPackMinimal.requiredExtensions.memory, capabilityPackMinimal.requiredExtensions.size, &device );

	VulkanLoadDeviceFunctions( device, instance );

	graphicsQueue.Init( device, queuesConfig.graphicsQueue.id, queuesConfig.graphicsQueue.queues );

	if( queuesConfig.computeQueue.queues ) {
		computeQueue.Init( device, queuesConfig.computeQueue.id, queuesConfig.computeQueue.queues );
	}

	if ( queuesConfig.transferQueue.queues ) {
		transferQueue.Init( device, queuesConfig.transferQueue.id, queuesConfig.transferQueue.queues );
	}

	if ( queuesConfig.sparseQueue.queues ) {
		sparseQueue.Init( device, queuesConfig.sparseQueue.id, queuesConfig.sparseQueue.queues );
	}
}
