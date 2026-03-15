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

#include <stdlib.h>

#include <SDL3/SDL_vulkan.h>

#include "../Math/NumberTypes.h"
#include "../Memory/DynamicArray.h"

#include "../Error.h"

#include "../Version.h"

#include "../VulkanLoader/VulkanLoadFunctions.h"

#include "Vulkan.h"

#include "GraphicsCoreStore.h"

#include "CapabilityPack.h"
#include "DebugMsg.h"
#include "PhysicalDevice.h"
#include "SwapChain.h"

#include "Queue.h"

#include "Instance.h"

void Instance::Init( const char* engineName, const char* appName ) {
	VkApplicationInfo appInfo {
		.pApplicationName   = appName,
		.applicationVersion = 0,
		.pEngineName        = engineName,
		.engineVersion      = DAEMON_VULKAN_VERSION.major | ( DAEMON_VULKAN_VERSION.minor << 10 ) | ( DAEMON_VULKAN_VERSION.patch << 21 ),
		.apiVersion         = VK_MAKE_API_VERSION( 0, 1, 4, 0 )
	};

	uint32 count;
	const char* const* sdlext = SDL_Vulkan_GetInstanceExtensions( &count );

	for ( uint32 i = 0; i < count; i++ ) {
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

	uint32 layerCount;
	vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

	DynamicArray<VkLayerProperties> layers;
	layers.Resize( layerCount );

	vkEnumerateInstanceLayerProperties( &layerCount, layers.memory );

	/* Go fuck your-implicit-layer-self - steam, epic games & whoever the fuck else thinks they can add implicit layers
	and are not Khronos/IHV/driver/debugger/profiler/LunarG */

	const char* allowedLayers = "VK_LOADER_LAYERS_ALLOW=VK_LAYER_KHRONOS_*, VK_LAYER_NV_*, VK_LAYER_AMD_*, VK_LAYER_LUNARG_*";

	#ifdef _MSC_VER
		_putenv( "VK_LOADER_LAYERS_DISABLE=~all~" );
		_putenv( allowedLayers );
	#else
		putenv( "VK_LOADER_LAYERS_DISABLE=~all~" );
		putenv( allowedLayers );
	#endif

	VkInstanceCreateInfo createInfo {
		.pApplicationInfo        = &appInfo,
		.enabledLayerCount       = 0,
		.ppEnabledLayerNames     = nullptr,
		.enabledExtensionCount   = ( uint32 ) extensions.size,
		.ppEnabledExtensionNames = extensions.memory
	};

	VkResult res = vkCreateInstance( &createInfo, nullptr, &instance );
	Q_UNUSED( res );

	VulkanLoadInstanceFunctions( instance );

	InitDebugMsg();

	uint32 deviceCount;
	vkEnumeratePhysicalDevices( instance, &deviceCount, nullptr );

	DynamicArray<VkPhysicalDevice> availableDevices;
	availableDevices.Resize( deviceCount );

	vkEnumeratePhysicalDevices( instance, &deviceCount, availableDevices.memory );

	if ( !SelectPhysicalDevice( availableDevices, &engineConfig, &physicalDevice ) ) {
		return;
	}

	InitQueueConfigs();

	CreateDevice( engineConfig, &device );

	VulkanLoadDeviceFunctions( device );

	InitQueues();

	InitFormatConfigs();

	mainSwapChain.Init( instance );
}