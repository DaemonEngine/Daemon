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
// EngineConfig.h

#include "engine/qcommon/q_shared.h"

#include "Vulkan.h"

#include "EngineConfig.h"

EngineConfig GetEngineConfigForDevice( const VkPhysicalDevice& device ) {
	VkPhysicalDeviceVulkan11Properties properties11 {};
	VkPhysicalDeviceVulkan12Properties properties12 { .pNext = &properties11 };
	VkPhysicalDeviceVulkan13Properties properties13 { .pNext = &properties12 };
	VkPhysicalDeviceProperties2        properties   { .pNext = &properties13 };

	VkPhysicalDeviceVulkan11Features                    features11                 {};
	VkPhysicalDeviceVulkan12Features                    features12                 { .pNext = &features11 };
	VkPhysicalDeviceVulkan13Features                    features13                 { .pNext = &features12 };
	VkPhysicalDeviceCopyMemoryIndirectFeaturesKHR       featuresIndirectMemoryCopy { .pNext = &features13 };
	VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT  featuresPipelinelibrary    { .pNext = &featuresIndirectMemoryCopy };
	VkPhysicalDeviceHostImageCopyFeatures               featuresHostImageCopy      { .pNext = &featuresPipelinelibrary };

	VkPhysicalDeviceFeatures2                           features { .pNext = &featuresHostImageCopy };

	vkGetPhysicalDeviceProperties2( device, &properties );
	vkGetPhysicalDeviceFeatures2(   device, &features );

	VkPhysicalDeviceProperties&        coreProperties   = properties.properties;
	VkPhysicalDeviceLimits&            limits           = coreProperties.limits;
	VkPhysicalDeviceSparseProperties&  sparseProperties = coreProperties.sparseProperties;
	VkPhysicalDeviceFeatures&          coreFeatures     = features.features;

	EngineConfig cfg {
		.driverID                                  = properties12.driverID,
		.conformanceVersion                        = properties12.conformanceVersion,

		.driverVersion                             = coreProperties.driverVersion,
		.vendorID                                  = coreProperties.vendorID,
		.deviceID                                  = coreProperties.deviceID,

		.maxAllocationSize                         = properties11.maxMemoryAllocationSize,
		.maxBufferSize                             = properties13.maxBufferSize,

		.subgroupSupportedStages                   = properties11.subgroupSupportedStages,
		.subgroupSupportedOps                      = properties11.subgroupSupportedOperations,
		.subgroupQuadOpsAllStages                  = ( bool ) properties11.subgroupQuadOperationsInAllStages,

		.minSubgroupSize                           = properties13.minSubgroupSize,
		.maxSubgroupSize                           = properties13.maxSubgroupSize,
		.maxComputeWorkgroupSubgroups              = properties13.maxComputeWorkgroupSubgroups,
		.requiredSubgroupSizeStages                = properties13.requiredSubgroupSizeStages,

		.maxSetDescriptors                         = properties11.maxPerSetDescriptors,
		.maxTotalDynamicDescriptors                = properties12.maxUpdateAfterBindDescriptorsInAllPools,
		.robustBufferAccessDynamic                 = ( bool ) properties12.robustBufferAccessUpdateAfterBind,

		.quadDivergentImplicitLod                  = ( bool ) properties12.quadDivergentImplicitLod,

		.filterMinmaxSingleComponentFormats        = ( bool ) properties12.filterMinmaxSingleComponentFormats,
		.filterMinmaxImageComponentMapping         = ( bool ) properties12.filterMinmaxImageComponentMapping,
		
		.maxSamplers                               = properties12.maxPerStageDescriptorUpdateAfterBindSamplers,
		.maxImages                                 = properties12.maxPerStageDescriptorUpdateAfterBindSampledImages,
		.maxStorageImages                          = properties12.maxPerStageDescriptorUpdateAfterBindStorageImages,

		.maxImageSize2D                            = limits.maxImageDimension2D,
		.maxImageSize3D                            = limits.maxImageDimension3D,
		.maxImageSizeCube                          = limits.maxImageDimensionCube,
		.maxImageArrayLayers                       = limits.maxImageArrayLayers,

		.maxPushConstSize                          = limits.maxPushConstantsSize,
		.maxAllocations                            = limits.maxMemoryAllocationCount,
		.bufferImageGranularity                    = limits.bufferImageGranularity,
		.sparseAddressSpaceSize                    = limits.sparseAddressSpaceSize,

		.maxComputeSharedMemSize                   = limits.maxComputeSharedMemorySize,
		.maxComputeWorkGroupCount                  = {
			limits.maxComputeWorkGroupCount[0],
			limits.maxComputeWorkGroupCount[1],
			limits.maxComputeWorkGroupCount[2],
		},
		.maxComputeWorkGroupInvocations            = limits.maxComputeWorkGroupInvocations,
		.maxComputeWorkGroupSize                   = {
			limits.maxComputeWorkGroupSize[0],
			limits.maxComputeWorkGroupSize[1],
			limits.maxComputeWorkGroupSize[2],
		},

		.subPixelPrecisionBits                     = limits.subPixelPrecisionBits,
		.subTexelPrecisionBits                     = limits.subTexelPrecisionBits,
		.mipmapPrecisionBits                       = limits.mipmapPrecisionBits,

		.minMemoryMapAlignment                     = limits.minMemoryMapAlignment,

		.discreteQueuePriorities                   = limits.discreteQueuePriorities,

		.optimalBufferCopyOffsetAlignment          = limits.optimalBufferCopyOffsetAlignment,
		.optimalBufferCopyRowPitchAlignment        = limits.optimalBufferCopyRowPitchAlignment,
		.nonCoherentAtomSize                       = limits.nonCoherentAtomSize,

		.shaderResourceResidency                   = ( bool ) coreFeatures.shaderResourceResidency,
		.shaderResourceMinLod                      = ( bool ) coreFeatures.shaderResourceMinLod,

		.depthBounds                               = ( bool ) coreFeatures.depthBounds,

		.sparseBinding                             = ( bool ) coreFeatures.sparseBinding,
		.sparseResidencyBuffer                     = ( bool ) coreFeatures.sparseResidencyBuffer,
		.sparseResidencyImage2D                    = ( bool ) coreFeatures.sparseResidencyImage2D,
		.sparseResidencyImage3D                    = ( bool ) coreFeatures.sparseResidencyImage3D,
		.sparseResidency2Samples                   = ( bool ) coreFeatures.sparseResidency2Samples,
		.sparseResidency4Samples                   = ( bool ) coreFeatures.sparseResidency4Samples,
		.sparseResidency8Samples                   = ( bool ) coreFeatures.sparseResidency8Samples,
		.sparseResidency16Samples                  = ( bool ) coreFeatures.sparseResidency16Samples,
		.sparseResidencyAliased                    = ( bool ) coreFeatures.sparseResidencyAliased,

		.residencyStandard2DBlockShape             = ( bool ) sparseProperties.residencyStandard2DBlockShape,
		.residencyStandard2DMultisampleBlockShape  = ( bool ) sparseProperties.residencyStandard2DMultisampleBlockShape,
		.residencyStandard3DBlockShape             = ( bool ) sparseProperties.residencyStandard3DBlockShape,
		.residencyAlignedMipSize                   = ( bool ) sparseProperties.residencyAlignedMipSize,
		.residencyNonResidentStrict                = ( bool ) sparseProperties.residencyNonResidentStrict,

		.textureCompressionETC2                    = ( bool ) coreFeatures.textureCompressionETC2,
		.textureCompressionASTC_LDR                = ( bool ) coreFeatures.textureCompressionASTC_LDR,
		.textureCompressionBC                      = ( bool ) coreFeatures.textureCompressionBC,

		.bufferDeviceAddress                       = ( bool ) features12.bufferDeviceAddress,
		.bufferDeviceAddressCaptureReplay          = ( bool ) features12.bufferDeviceAddressCaptureReplay,

		.scalarLayout                              = ( bool ) features12.scalarBlockLayout,

		.descriptorIndexing                        = ( bool ) features12.descriptorIndexing,

		.descriptorBindingUpdateUnusedWhilePending = ( bool ) features12.descriptorBindingUpdateUnusedWhilePending,
		.descriptorBindingPartiallyBound           = ( bool ) features12.descriptorBindingPartiallyBound,
		.descriptorBindingVariableDescriptorCount  = ( bool ) features12.descriptorBindingVariableDescriptorCount,

		.runtimeDescriptorArray                    = ( bool ) features12.runtimeDescriptorArray,

		.samplerFilterMinmax                       = ( bool ) features12.samplerFilterMinmax,

		.shaderSampledImageArrayNonUniformIndexing = ( bool ) features12.shaderSampledImageArrayNonUniformIndexing,
		.shaderStorageImageArrayNonUniformIndexing = ( bool ) features12.shaderStorageImageArrayNonUniformIndexing,

		.dynamicRendering                          = ( bool ) features13.dynamicRendering,
		.synchronization2                          = ( bool ) features13.synchronization2,
		
		.maintenance4                              = ( bool ) features13.maintenance4,

		.textureCompressionASTC_HDR                = ( bool ) features13.textureCompressionASTC_HDR,

		.indirectMemoryCopy                        = ( bool ) featuresIndirectMemoryCopy.indirectMemoryCopy,
		.indirectMemoryToImageCopy                 = ( bool ) featuresIndirectMemoryCopy.indirectMemoryToImageCopy,
		.graphicsPipelineLibrary                   = ( bool ) featuresPipelinelibrary.graphicsPipelineLibrary,
	};

	memcpy( cfg.driverUUID, properties11.driverUUID, VK_UUID_SIZE );

	Q_strncpyz( cfg.driverName, properties12.driverName, VK_MAX_DRIVER_NAME_SIZE );
	Q_strncpyz( cfg.driverInfo, properties12.driverInfo, VK_MAX_DRIVER_INFO_SIZE );

	Q_strncpyz( cfg.deviceName, coreProperties.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE );

	memcpy( cfg.pipelineCacheUUID, coreProperties.pipelineCacheUUID, VK_UUID_SIZE );

	switch ( coreProperties.deviceType ) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			cfg.deviceType = EngineConfig::DISCRETE;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			cfg.deviceType = EngineConfig::INTEGRATED;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			cfg.deviceType = EngineConfig::VIRTUAL;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		default:
			cfg.deviceType = EngineConfig::CPU;
			break;
	}

	return cfg;
}
