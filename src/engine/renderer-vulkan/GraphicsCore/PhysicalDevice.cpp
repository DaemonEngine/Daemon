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
// PhysicalDevice.cpp

#include "../Math/NumberTypes.h"
#include "../Memory/Array.h"
#include "../Memory/DynamicArray.h"
#include "../Error.h"

#include "GraphicsCoreCVars.h"

#include "EngineConfig.h"
#include "QueuesConfig.h"
#include "CapabilityPack.h"

#include "PhysicalDevice.h"

static void PrintDeviceInfo( const EngineConfig& config ) {
	static const char* deviceTypes[] = {
		"Discrete",
		"Integrated",
		"Virtual",
		"CPU"
	};

	Log::Notice( "%s, type: %s, driver name: %s, driver info: %s, maximum supported capability pack: %s, Vulkan: %u.%u.%u",
		config.deviceName, deviceTypes[config.deviceType], config.driverName, config.driverInfo,
		CapabilityPackType::typeStrings[config.capabilityPack],
		config.conformanceVersion.major, config.conformanceVersion.minor, config.conformanceVersion.patch );
}

bool SelectPhysicalDevice( const DynamicArray<VkPhysicalDevice>& devices, EngineConfig* config, VkPhysicalDevice* deviceOut ) {
	if ( !devices.size ) {
		Err( "No Vulkan devices found" );
		return false;
	}

	Log::Notice( "Found Vulkan devices:" );

	const VkPhysicalDevice* bestDevice = &devices[0];
	EngineConfig bestCFG = GetEngineConfigForDevice( *bestDevice );
	bestCFG.capabilityPack = GetHighestSuppportedCapabilityPack( bestCFG );

	PrintDeviceInfo( bestCFG );

	for ( const VkPhysicalDevice& device : devices ) {
		if ( &device == bestDevice ) {
			continue;
		}

		EngineConfig cfg = GetEngineConfigForDevice( device );
		cfg.capabilityPack = GetHighestSuppportedCapabilityPack( cfg );

		PrintDeviceInfo( cfg );

		if ( cfg.capabilityPack == CapabilityPackType::NONE ) {
			Log::Notice( "%s doesn't support the minimal capability pack", cfg.deviceName );
			continue;
		}

		if ( cfg.deviceType < bestCFG.deviceType ) {
			bestDevice = &device;
			bestCFG = cfg;
			Log::Notice( "Selecting %s because it has a better GPU type", cfg.deviceName );
			continue;
		}

		if ( cfg.capabilityPack > bestCFG.capabilityPack ) {
			bestDevice = &device;
			bestCFG = cfg;
			Log::Notice( "Selecting %s because it supports a higher capability pack", cfg.deviceName );
			continue;
		}
	}

	if ( r_vkDevice.Get() != -1 ) {
		if ( r_vkDevice.Get() < devices.size ) {
			bestDevice = &devices[r_vkDevice.Get()];
			bestCFG = GetEngineConfigForDevice( *bestDevice );
			bestCFG.capabilityPack = GetHighestSuppportedCapabilityPack( bestCFG );
		} else {
			Log::Warn( "r_vkDevice out of range, using default instead" );
		}
	}

	if ( bestCFG.capabilityPack == CapabilityPackType::NONE ) {
		Err( "No available Vulkan devices support the minimal capability pack" );
		return false;
	}

	*config = bestCFG;

	*deviceOut = *bestDevice;

	return true;
}

void CreateDevice( const VkPhysicalDevice& physicalDevice, EngineConfig& config, QueuesConfig& queuesConfig,
	const char* const* requiredExtensions, const uint32 extensionCount,
	VkDevice* device ) {
	VkPhysicalDeviceVulkan12Features features12 {};
	VkPhysicalDeviceVulkan13Features features13 { .pNext = &features12 };
	VkPhysicalDeviceFeatures2        features   { .pNext = &features13 };

	vkGetPhysicalDeviceFeatures2( physicalDevice, &features );

	DynamicArray<VkDeviceQueueCreateInfo> queueInfos;
	queueInfos.Resize( queuesConfig.count );

	for ( uint32 i = 0; i < queuesConfig.count; i++ ) {
		DynamicArray<float> priorities;
		priorities.Resize( queuesConfig[i].queues );

		for ( float* value = priorities.memory; value < priorities.memory + priorities.size; value++ ) {
			*value = 1.0f;
		}

		VkDeviceQueueCreateInfo& queueInfo = queueInfos[i];

		queueInfo.queueFamilyIndex = i;
		queueInfo.queueCount = queuesConfig[i].queues;
		queueInfo.pQueuePriorities = priorities.memory;
	}

	VkDeviceCreateInfo info {
		.pNext = &features,
		.queueCreateInfoCount = queuesConfig.count,
		.pQueueCreateInfos = queueInfos.memory,
		.enabledExtensionCount = extensionCount,
		.ppEnabledExtensionNames = requiredExtensions
	};

	VkResult res = vkCreateDevice( physicalDevice, &info, nullptr, device );
	Q_UNUSED( res );
	Q_UNUSED( config );
}
