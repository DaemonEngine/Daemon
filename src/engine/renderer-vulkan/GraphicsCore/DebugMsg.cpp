/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2026 Daemon Developers
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
// DebugMsg.cpp

#include "../Math/Bit.h"

#include "../Memory/DynamicArray.h"

#include "Vulkan.h"

#include "GraphicsCoreCVars.h"

#include "GraphicsCoreStore.h"

#include "FeaturesConfig.h"

#include "Instance.h"

#include "DebugMsg.h"

static VkDebugUtilsMessengerEXT debugUtilsMessenger;

const char* ObjectTypeToString( const VkObjectType objectType ) {
	switch ( objectType ) {
        case VK_OBJECT_TYPE_UNKNOWN:
            return "Unknown";
        case VK_OBJECT_TYPE_INSTANCE:
            return "Instance";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
            return "Physical Device";
        case VK_OBJECT_TYPE_DEVICE:
            return "Device";
        case VK_OBJECT_TYPE_QUEUE:
            return "Queue";
        case VK_OBJECT_TYPE_SEMAPHORE:
            return "Semaphore";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:
            return "Command Buffer";
        case VK_OBJECT_TYPE_FENCE:
            return "Fence";
        case VK_OBJECT_TYPE_DEVICE_MEMORY:
            return "Device Memory";
        case VK_OBJECT_TYPE_BUFFER:
            return "Buffer";
        case VK_OBJECT_TYPE_IMAGE:
            return "Image";
        case VK_OBJECT_TYPE_EVENT:
            return "Event";
        case VK_OBJECT_TYPE_QUERY_POOL:
            return "Query Pool";
        case VK_OBJECT_TYPE_IMAGE_VIEW:
            return "Image View";
        case VK_OBJECT_TYPE_PIPELINE_CACHE:
            return "Pipeline Cache";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
            return "Pipeline Layout";
        case VK_OBJECT_TYPE_PIPELINE:
            return "Pipeline";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
            return "DescriptorSet Layout";
        case VK_OBJECT_TYPE_SAMPLER:
            return "Sampler";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
            return "Descriptor Pool";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
            return "DescriptorSet";
        case VK_OBJECT_TYPE_COMMAND_POOL:
            return "Command Pool";
        case VK_OBJECT_TYPE_SURFACE_KHR:
            return "Surface";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
            return "SwapChain";
        case VK_OBJECT_TYPE_DISPLAY_KHR:
            return "Display";
        case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
            return "Display Mode";
        case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
            return "Debug Report Callback";
        case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
            return "DebugUtilsMessenger";
        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:
            return "Acceleration Structure";
        case VK_OBJECT_TYPE_MICROMAP_EXT:
            return "Micromap";
        case VK_OBJECT_TYPE_PIPELINE_BINARY_KHR:
            return "Pipeline Binary";
        case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_EXT:
            return "DGC Layout";
        case VK_OBJECT_TYPE_INDIRECT_EXECUTION_SET_EXT:
            return "DGC ExecutionSet";
        default:
            return "Unsupported Type";
    }
}

static VkBool32 DebugUtilsMsg( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                               const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /* pUserData */ ) {
	uint32 severityMask = r_vkDebugMsgSeverity.Get();
	uint32 typeMask     = r_vkDebugMsgType.Get();

	uint32 msgSeverity = ( uint32 ) messageSeverity;

	uint32 severity = 
		  ( ( bool ) ( msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ) ) << ( DEBUG_MSG_VERBOSE - 1 )
		| ( ( bool ) ( msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )    ) << ( DEBUG_MSG_INFO    - 1 )
		| ( ( bool ) ( msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) ) << ( DEBUG_MSG_WARNING - 1 )
		| ( ( bool ) ( msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )   ) << ( DEBUG_MSG_ERROR   - 1 );

	if ( !( severity & severityMask ) || !( messageTypes & typeMask ) ) {
		return false;
	}

	const char* debugMsgColours[] {
		"^9", // DEBUG_MSG_VERBOSE
		"^5", // DEBUG_MSG_INFO
		"^3", // DEBUG_MSG_WARNING
		"^1"  // DEBUG_MSG_ERROR
	};

	const char* msgColour = debugMsgColours[severity];

	std::string msg;

	if ( pCallbackData->objectCount ) {
		const char* name = pCallbackData->pObjects[0].pObjectName ? pCallbackData->pObjects[0].pObjectName : "";

		msg = Str::Format( "[%s%s%s] %s", ObjectTypeToString( pCallbackData->pObjects[0].objectType ), name, *name ? " " : "", msg );
	}

	if ( messageTypes & DEBUG_MSG_VALIDATION ) {
		msg = Str::Format( "[VUID: %s]: %s", pCallbackData->pMessageIdName, pCallbackData->pMessage );
	} else {
		msg = Str::Format( "[%s]: %s", pCallbackData->pMessageIdName, pCallbackData->pMessage );
	}

	for ( const VkDebugUtilsObjectNameInfoEXT* obj = pCallbackData->pObjects + 1; obj < pCallbackData->pObjects + pCallbackData->objectCount; obj++ ) {
		const char* name = obj->pObjectName ? obj->pObjectName : "";

		msg = Str::Format( "%s {%s %s}", msg, ObjectTypeToString( obj->objectType ), name );
	}

	for ( const VkDebugUtilsLabelEXT* label = pCallbackData->pQueueLabels; label < pCallbackData->pQueueLabels + pCallbackData->queueLabelCount; label++ ) {
		const char* name = label->pLabelName ? label->pLabelName : "";

		msg = Str::Format( "%s {queue %s}", msg, name );
	}

	for ( const VkDebugUtilsLabelEXT* label = pCallbackData->pCmdBufLabels; label < pCallbackData->pCmdBufLabels + pCallbackData->cmdBufLabelCount; label++ ) {
		const char* name = label->pLabelName ? label->pLabelName : "";

		msg = Str::Format( "%s {cmd %s}", msg, name );
	}

	if ( severity >= DEBUG_MSG_WARNING ) {
		Log::Warn(   "%s%s", msgColour, msg );
	} else {
		Log::Notice( "%s%s", msgColour, msg );
	}

	return false;
}

void LogDeviceFaultInfo() {
	if ( !featuresConfig.deviceFault ) {
		Log::Warn( "Unable to generate device fault info: deviceFault not supported" );
		return;
	}

	VkDeviceFaultCountsEXT deviceFaultCounts {};

	vkGetDeviceFaultInfoEXT( device, &deviceFaultCounts, nullptr );

	DynamicArray<VkDeviceFaultAddressInfoEXT> addressInfos;
	addressInfos.Resize( deviceFaultCounts.addressInfoCount );

	DynamicArray<VkDeviceFaultVendorInfoEXT>  vendorInfos;
	vendorInfos.Resize( deviceFaultCounts.vendorInfoCount );

	VkDeviceFaultInfoEXT deviceFaultInfo {
		.pAddressInfos = addressInfos.memory,
		.pVendorInfos  = vendorInfos.memory
	};

	vkGetDeviceFaultInfoEXT( device, &deviceFaultCounts, &deviceFaultInfo );

	Log::Warn( "^1Device fault: %s", deviceFaultInfo.description );

	for ( const VkDeviceFaultAddressInfoEXT& addressInfo : addressInfos ) {
		const uint64      addressLower  = ( addressInfo.reportedAddress & ~( addressInfo.addressPrecision - 1 ) );
		const uint64      addressUpper  = ( addressInfo.reportedAddress |  ( addressInfo.addressPrecision - 1 ) );

		const std::string addressString = addressLower == addressUpper
		                                ? Str::Format( "%u",   addressLower )
		                                : Str::Format( "%u-%u", addressLower, addressUpper );

		switch ( addressInfo.addressType ) {
			case VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT:
				Log::Warn( "^1Unknown fault type" );
				continue;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT:
				Log::Warn( "^1[Invalid Read]: %s",                addressString );
				break;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT:
				Log::Warn( "^1[Invalid Write]: %s",               addressString );
				break;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT:
				Log::Warn( "^1[Invalid Execution]: %s",           addressString );
				break;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT:
				Log::Warn( "^1[Unknown Instruction Pointer]: %s", addressString );
				break;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT:
				Log::Warn( "^1[Invalid Instruction Pointer]: %s", addressString );
				break;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT:
				Log::Warn( "^1[Related Instruction Pointer]: %s", addressString );
				break;
		}
	}

	for ( const VkDeviceFaultVendorInfoEXT& vendorInfo : vendorInfos ) {
		Log::Warn( "^1[%u:%u]: %s", vendorInfo.vendorFaultCode, vendorInfo.vendorFaultCode, vendorInfo.description );
	}
}

void LogDeviceFaultInfo() {
	if ( !featuresConfig.deviceFault ) {
		Log::Warn( "Unable to generate device fault info: deviceFault not supported" );
		return;
	}

	VkDeviceFaultCountsEXT deviceFaultCounts {};

	vkGetDeviceFaultInfoEXT( device, &deviceFaultCounts, nullptr );

	DynamicArray<VkDeviceFaultAddressInfoEXT> addressInfos;
	addressInfos.Resize( deviceFaultCounts.addressInfoCount );

	DynamicArray<VkDeviceFaultVendorInfoEXT>  vendorInfos;
	vendorInfos.Resize( deviceFaultCounts.vendorInfoCount );

	VkDeviceFaultInfoEXT deviceFaultInfo {
		.pAddressInfos = addressInfos.memory,
		.pVendorInfos  = vendorInfos.memory
	};

	vkGetDeviceFaultInfoEXT( device, &deviceFaultCounts, &deviceFaultInfo );

	Log::Warn( "^1Device fault: %s", deviceFaultInfo.description );

	for ( const VkDeviceFaultAddressInfoEXT& addressInfo : addressInfos ) {
		const uint64      addressLower  = ( addressInfo.reportedAddress & ~( addressInfo.addressPrecision - 1 ) );
		const uint64      addressUpper  = ( addressInfo.reportedAddress |  ( addressInfo.addressPrecision - 1 ) );

		const std::string addressString = addressLower == addressUpper
		                                ? Str::Format( "%u",   addressLower )
		                                : Str::Format( "%u-%u", addressLower, addressUpper );

		switch ( addressInfo.addressType ) {
			case VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT:
				Log::Warn( "^1Unknown fault type" );
				continue;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT:
				Log::Warn( "^1[Invalid Read]: %s",                addressString );
				break;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT:
				Log::Warn( "^1[Invalid Write]: %s",               addressString );
				break;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT:
				Log::Warn( "^1[Invalid Execution]: %s",           addressString );
				break;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT:
				Log::Warn( "^1[Unknown Instruction Pointer]: %s", addressString );
				break;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT:
				Log::Warn( "^1[Invalid Instruction Pointer]: %s", addressString );
				break;
			case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT:
				Log::Warn( "^1[Related Instruction Pointer]: %s", addressString );
				break;
		}
	}

	for ( const VkDeviceFaultVendorInfoEXT& vendorInfo : vendorInfos ) {
		Log::Warn( "^1[%u:%u]: %s", vendorInfo.vendorFaultCode, vendorInfo.vendorFaultCode, vendorInfo.description );
	}
}

void InitDebugMsg() {
	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerInfo {
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
		                 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &DebugUtilsMsg
	};

	vkCreateDebugUtilsMessengerEXT( instance.instance, &debugUtilsMessengerInfo, nullptr, &debugUtilsMessenger );
}