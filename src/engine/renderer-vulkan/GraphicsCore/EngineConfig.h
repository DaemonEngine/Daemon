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

#ifndef ENGINE_CONFIG_H
#define ENGINE_CONFIG_H

#include "Vulkan.h"

#include "../Math/NumberTypes.h"

struct EngineConfig {
	enum Type {
		DISCRETE,
		INTEGRATED,
		VIRTUAL,
		CPU
	};

	int    capabilityPack;

	uint8  driverUUID[VK_UUID_SIZE];

	VkDriverId driverID;
	char   driverName[VK_MAX_DRIVER_NAME_SIZE];
	char   driverInfo[VK_MAX_DRIVER_INFO_SIZE];
	VkConformanceVersion conformanceVersion;

	uint32 driverVersion;
	uint32 vendorID;
	uint32 deviceID;
	Type   deviceType;
	char   deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
	uint8  pipelineCacheUUID[VK_UUID_SIZE];

	uint64 maxAllocationSize;
	uint64 maxBufferSize;

	uint32 subgroupSize;

	uint32 subgroupSupportedStages;
	uint32 subgroupSupportedOps;
	bool   subgroupQuadOpsAllStages;

	uint32 minSubgroupSize;
	uint32 maxSubgroupSize;
	uint32 maxComputeWorkgroupSubgroups;
	uint32 requiredSubgroupSizeStages;

	uint32 maxSetDescriptors;
	uint32 maxTotalDynamicDescriptors;
	bool   robustBufferAccessDynamic;

	bool   quadDivergentImplicitLod;

	bool   filterMinmaxSingleComponentFormats;
	bool   filterMinmaxImageComponentMapping;

	uint32 maxSamplers;
	uint32 maxImages;
	uint32 maxStorageImages;

	uint32 maxImageSize2D;
	uint32 maxImageSize3D;
	uint32 maxImageSizeCube;
	uint32 maxImageArrayLayers;

	uint32 maxPushConstSize;
	uint32 maxAllocations;
	uint64 bufferImageGranularity;
	uint64 sparseAddressSpaceSize;

	uint32 maxComputeSharedMemSize;
	uint32 maxComputeWorkGroupCount[3];
	uint32 maxComputeWorkGroupInvocations;
	uint32 maxComputeWorkGroupSize[3];

	uint32 subPixelPrecisionBits;
	uint32 subTexelPrecisionBits;
	uint32 mipmapPrecisionBits;

	uint64 minMemoryMapAlignment;

	uint32 discreteQueuePriorities;

	uint64 optimalBufferCopyOffsetAlignment;
	uint64 optimalBufferCopyRowPitchAlignment;
	uint64 coherentAccessAlignment;

	bool   shaderResourceResidency;
	bool   shaderResourceMinLod;

	bool   depthBounds;

	bool   sparseBinding;
	bool   sparseResidencyBuffer;
	bool   sparseResidencyImage2D;
	bool   sparseResidencyImage3D;
	bool   sparseResidency2Samples;
	bool   sparseResidency4Samples;
	bool   sparseResidency8Samples;
	bool   sparseResidency16Samples;
	bool   sparseResidencyAliased;

	bool   residencyStandard2DBlockShape;
	bool   residencyStandard2DMultisampleBlockShape;
	bool   residencyStandard3DBlockShape;
	bool   residencyAlignedMipSize;
	bool   residencyNonResidentStrict;

	bool   textureCompressionETC2;
	bool   textureCompressionASTC_LDR;
	bool   textureCompressionBC;

	bool   bufferDeviceAddress;
	bool   bufferDeviceAddressCaptureReplay;

	bool   scalarLayout;

	bool   descriptorIndexing;

	bool   descriptorBindingUpdateUnusedWhilePending;
	bool   descriptorBindingPartiallyBound;
	bool   descriptorBindingVariableDescriptorCount;

	bool   runtimeDescriptorArray;

	bool   samplerFilterMinmax;

	bool   shaderSampledImageArrayNonUniformIndexing;
	bool   shaderStorageImageArrayNonUniformIndexing;

	bool   dynamicRendering;
	bool   synchronization2;

	bool   maintenance4;

	bool   textureCompressionASTC_HDR;

	bool   indirectMemoryCopy;
	bool   indirectMemoryToImageCopy;
	bool   graphicsPipelineLibrary;
};

EngineConfig GetEngineConfigForDevice( const VkPhysicalDevice& device );

#endif // ENGINE_CONFIG_H