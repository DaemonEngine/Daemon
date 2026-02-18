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
// CapabilityPack.h

#ifndef CAPABILITY_PACK_H
#define CAPABILITY_PACK_H

#include "engine/qcommon/q_shared.h"

#include "../Math/NumberTypes.h"
#include "../Version.h"
#include "../Memory/Array.h"
#include "../Memory/DynamicArray.h"

#include "Vulkan.h"

#include "Decls.h"

#include "EngineConfig.h"

template<const uint32 instanceExtensionCount, const uint32 extensionCount, const uint32 featureCount>
struct CapabilityPack {
	const Version                                    minVersion;

	const Array<const char*, instanceExtensionCount> requiredInstanceExtensions;
	const Array<const char*, extensionCount>         requiredExtensions;
	const Array<const char*, featureCount>           requiredFeatures;
};

namespace CapabilityPackType {
	enum Type {
		NONE,
		MINIMAL,
		RECOMMENDED,
		EXPERIMENTAL
	};

	constexpr const char* typeStrings[] = {
		"NONE",
		"MINIMAL",
		"RECOMMENDED",
		"EXPERIMENTAL"
	};
}

constexpr Array instanceExtensions {
	// "VK_LAYER_KHRONOS_validation",
	"VK_KHR_get_physical_device_properties2",
	"VK_KHR_get_surface_capabilities2",
	"VK_EXT_surface_maintenance1"
};

constexpr Array extensionsMinimal {
	"VK_KHR_swapchain",
	#ifdef _MSC_VER
		// "VK_EXT_full_screen_exclusive"
	#endif
};

constexpr Array featuresMinimal {
	"drawIndirectFirstInstance",
	"dualSrcBlend",
	"fragmentStoresAndAtomics",
	"fullDrawIndexUint32",
	"imageCubeArray",
	"independentBlend",
	"multiDrawIndirect",
	"multiViewport",
	"samplerAnisotropy",
	"shaderClipDistance",
	"shaderCullDistance",
	"shaderImageGatherExtended",
	"shaderInt16",
	"shaderInt64",
	"shaderSampledImageArrayDynamicIndexing",
	"shaderStorageImageArrayDynamicIndexing",
	"shaderStorageImageExtendedFormats",
	"shaderStorageImageWriteWithoutFormat",
	"textureCompressionBC",
	"vertexPipelineStoresAndAtomics",
	"wideLines",
	"storageBuffer16BitAccess",
	"variablePointersStorageBuffer",
	"variablePointers",
	"shaderDrawParameters",
	"bufferDeviceAddress",
	"bufferDeviceAddressCaptureReplay",
	"descriptorBindingPartiallyBound",
	"descriptorBindingSampledImageUpdateAfterBind",
	"descriptorBindingStorageImageUpdateAfterBind",
	"descriptorBindingUpdateUnusedWhilePending",
	"descriptorIndexing",
	"drawIndirectCount",
	"runtimeDescriptorArray",
	"samplerFilterMinmax",
	"samplerMirrorClampToEdge",
	"scalarBlockLayout",
	"shaderBufferInt64Atomics",
	"shaderFloat16",
	"shaderInt8",
	"shaderOutputLayer",
	"shaderOutputViewportIndex",
	"shaderSampledImageArrayNonUniformIndexing",
	"shaderStorageImageArrayNonUniformIndexing",
	"shaderSubgroupExtendedTypes",
	"storageBuffer8BitAccess",
	"subgroupBroadcastDynamicId",
	"timelineSemaphore",
	"uniformBufferStandardLayout",
	"vulkanMemoryModel",
	"vulkanMemoryModelAvailabilityVisibilityChains",
	"computeFullSubgroups",
	"dynamicRendering",
	"maintenance4",
	"pipelineCreationCacheControl",
	"shaderDemoteToHelperInvocation",
	"shaderIntegerDotProduct",
	"shaderTerminateInvocation",
	"shaderZeroInitializeWorkgroupMemory",
	"subgroupSizeControl",
	"synchronization2",
	"maintenance5",
	"maintenance6",
	"formatA4R4G4B4",
	"extendedDynamicState",
	"minLod",
	"indexTypeUint8",
	"shaderBufferFloat32Atomics",
	"shaderImageFloat32Atomics",
	"shaderBufferInt64Atomics",
	"shaderSubgroupClock",
	"shaderExpectAssume",
	"shaderFloatControls2",
	"shaderMaximalReconvergence",
	"shaderQuadControl",
	"shaderSubgroupRotate",
	"shaderSubgroupUniformControlFlow",
	"workgroupMemoryExplicitLayout",
	"workgroupMemoryExplicitLayout16BitAccess",
	"workgroupMemoryExplicitLayoutScalarBlockLayout"
};

constexpr Array featuresRecommended {
	"fullDrawIndexUint32"
};

constexpr Array featuresExperimental {
	"fullDrawIndexUint32"
};

constexpr Array featuresOptional {
	"hostImageCopy",
	"swapchainMaintenance1",
	"descriptorHeap",
	"zeroInitializeDeviceMemory"
};

constexpr bool            EngineConfigSupportedMinimal( const EngineConfig& config );

CapabilityPackType::Type  GetHighestSuppportedCapabilityPack( const EngineConfig& config );

DynamicArray<const char*> GetCapabilityPackFeatures( const CapabilityPackType::Type type, const FeaturesConfig& cfg, FeaturesConfig* cfgOut );

constexpr CapabilityPack<instanceExtensions.Size(), extensionsMinimal.Size(), featuresMinimal.Size()> capabilityPackMinimal {
	.minVersion                   { 1, 4, 0 },
	.requiredInstanceExtensions = instanceExtensions,
	.requiredExtensions         = extensionsMinimal,
	.requiredFeatures           = featuresMinimal
};

#endif // CAPABILITY_PACK_H