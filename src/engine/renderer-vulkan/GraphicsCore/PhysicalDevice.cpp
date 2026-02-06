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

	EngineConfig bestCFG   = GetEngineConfigForDevice( *bestDevice );
	bestCFG.capabilityPack = GetHighestSuppportedCapabilityPack( bestCFG );

	PrintDeviceInfo( bestCFG );

	for ( const VkPhysicalDevice& device : devices ) {
		if ( &device == bestDevice ) {
			continue;
		}

		EngineConfig cfg   = GetEngineConfigForDevice( device );
		cfg.capabilityPack = GetHighestSuppportedCapabilityPack( cfg );

		PrintDeviceInfo( cfg );

		if ( cfg.capabilityPack == CapabilityPackType::NONE ) {
			Log::Notice( "%s doesn't support the minimal capability pack", cfg.deviceName );
			continue;
		}

		if ( cfg.deviceType < bestCFG.deviceType ) {
			bestDevice = &device;
			bestCFG    = cfg;
			Log::Notice( "Selecting %s because it has a better GPU type", cfg.deviceName );
			continue;
		}

		if ( cfg.capabilityPack > bestCFG.capabilityPack ) {
			bestDevice = &device;
			bestCFG    = cfg;

			Log::Notice( "Selecting %s because it supports a higher capability pack", cfg.deviceName );
			continue;
		}
	}

	if ( r_vkDevice.Get() != -1 ) {
		if ( r_vkDevice.Get() < devices.size ) {
			bestDevice             = &devices[r_vkDevice.Get()];
			bestCFG                = GetEngineConfigForDevice( *bestDevice );
			bestCFG.capabilityPack = GetHighestSuppportedCapabilityPack( bestCFG );

			Log::Notice( "Selecting %s because device was overridden with r_vkDevice = %i", bestCFG.deviceName, r_vkDevice.Get() );
		} else {
			Log::Warn( "r_vkDevice out of range, using default instead" );
		}
	}

	if ( bestCFG.capabilityPack == CapabilityPackType::NONE ) {
		Err( "No available Vulkan devices support the minimal capability pack" );
		return false;
	}

	*config    =  bestCFG;

	*deviceOut = *bestDevice;

	return true;
}

void CreateDevice( const VkPhysicalDevice& physicalDevice, EngineConfig& config, QueuesConfig& queuesConfig,
	const char* const* requiredExtensions, const uint32 extensionCount,
	VkDevice* device ) {
	VkPhysicalDeviceVulkan11Features                     features11                 {};
	VkPhysicalDeviceVulkan12Features                     features12                 { .pNext = &features11 };
	VkPhysicalDeviceVulkan13Features                     features13                 { .pNext = &features12 };
	VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR  featuresComputeDerivatives { .pNext = &features13 };
	VkPhysicalDeviceCopyMemoryIndirectFeaturesKHR        featuresIndirectMemoryCopy { .pNext = &featuresComputeDerivatives };
	VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT   featuresPipelinelibrary    { .pNext = &featuresIndirectMemoryCopy };
	VkPhysicalDeviceHostImageCopyFeatures                featuresHostImageCopy      { .pNext = &featuresPipelinelibrary };
	VkPhysicalDeviceMaintenance4FeaturesKHR              featuresMaintenance4       { .pNext = &featuresHostImageCopy };
	VkPhysicalDeviceMaintenance5FeaturesKHR              featuresMaintenance5       { .pNext = &featuresMaintenance4 };
	VkPhysicalDeviceMaintenance6FeaturesKHR              featuresMaintenance6       { .pNext = &featuresMaintenance5 };
	VkPhysicalDeviceMaintenance7FeaturesKHR              featuresMaintenance7       { .pNext = &featuresMaintenance6 };
	VkPhysicalDeviceMaintenance8FeaturesKHR              featuresMaintenance8       { .pNext = &featuresMaintenance7 };
	VkPhysicalDeviceMaintenance9FeaturesKHR              featuresMaintenance9       { .pNext = &featuresMaintenance8 };
	// VkPhysicalDeviceMaintenance10FeaturesKHR             featuresMaintenance10      { .pNext = &featuresHostImageCopy };
	VkPhysicalDeviceMeshShaderFeaturesEXT                featuresMeshShader         { .pNext = &featuresMaintenance9 };
	VkPhysicalDevicePipelineBinaryFeaturesKHR            featuresPipelineBinary     { .pNext = &featuresMeshShader };
	VkPhysicalDeviceShaderClockFeaturesKHR               featuresShaderClock        { .pNext = &featuresPipelineBinary };
	VkPhysicalDeviceShaderQuadControlFeaturesKHR         featuresQuadControl        { .pNext = &featuresShaderClock };
	// VkPhysicalDeviceShaderSubgroupPartitionedFeaturesEXT featuresSubgroupPart       { .pNext = &featuresQuadControl };
	VkPhysicalDeviceShaderUntypedPointersFeaturesKHR     featuresUntypedPtr         { .pNext = &featuresQuadControl };
	VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR     featuresSwapChainMaint     { .pNext = &featuresUntypedPtr };
	VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR       featuresUnifiedImgLayouts  { .pNext = &featuresSwapChainMaint };
	VkPhysicalDevicePipelineRobustnessProperties         featuresPipelineRobustness { .pNext = &featuresUnifiedImgLayouts };

	VkPhysicalDeviceFeatures2                            features                   { .pNext = &featuresMaintenance9 };

	vkGetPhysicalDeviceFeatures2( physicalDevice, &features );

	DynamicArray<VkDeviceQueueCreateInfo> queueInfos;
	queueInfos.Resize( queuesConfig.count );
	queueInfos.Init();

	for ( uint32 i = 0; i < queuesConfig.count; i++ ) {
		DynamicArray<float> priorities;
		priorities.Resize( queuesConfig[i].queueCount );

		for ( float* value = priorities.memory; value < priorities.memory + priorities.size; value++ ) {
			*value = 1.0f;
		}

		VkDeviceQueueCreateInfo& queueInfo = queueInfos[i];

		queueInfo.queueFamilyIndex = i;
		queueInfo.queueCount       = queuesConfig[i].queueCount;
		queueInfo.pQueuePriorities = priorities.memory;
	}

	features.features.robustBufferAccess   = false;

	features11.multiview                   = false;
	features11.multiviewGeometryShader     = false;
	features11.multiviewTessellationShader = false;
	features11.protectedMemory             = false;
	features11.samplerYcbcrConversion      = false;

	features12.shaderInputAttachmentArrayDynamicIndexing          = false;
	features12.shaderUniformTexelBufferArrayDynamicIndexing       = false;
	features12.shaderStorageTexelBufferArrayDynamicIndexing       = false;
	features12.shaderUniformBufferArrayNonUniformIndexing         = false;
	features12.shaderStorageBufferArrayNonUniformIndexing         = false;
	features12.shaderInputAttachmentArrayNonUniformIndexing       = false;
	features12.shaderUniformTexelBufferArrayNonUniformIndexing    = false;
	features12.shaderStorageTexelBufferArrayNonUniformIndexing    = false;
	features12.descriptorBindingUniformBufferUpdateAfterBind      = false;
	features12.descriptorBindingStorageBufferUpdateAfterBind      = false;
	features12.descriptorBindingUniformTexelBufferUpdateAfterBind = false;
	features12.descriptorBindingStorageTexelBufferUpdateAfterBind = false;
	features12.imagelessFramebuffer                               = false;
	features12.separateDepthStencilLayouts                        = false;
	features12.hostQueryReset                                     = false;
	features12.bufferDeviceAddressCaptureReplay                   = false;
	features12.bufferDeviceAddressMultiDevice                     = false;

	features13.robustImageAccess                                  = false;
	features13.inlineUniformBlock                                 = false;
	features13.descriptorBindingInlineUniformBlockUpdateAfterBind = false;
	features13.privateData                                        = false;

	VkDeviceCreateInfo info {
		.pNext                   = &features,
		.queueCreateInfoCount    = queuesConfig.count,
		.pQueueCreateInfos       = queueInfos.memory,
		.enabledExtensionCount   = extensionCount,
		.ppEnabledExtensionNames = requiredExtensions
	};

	VkResult res = vkCreateDevice( physicalDevice, &info, nullptr, device );
	Q_UNUSED( res );
	Q_UNUSED( config );
}
