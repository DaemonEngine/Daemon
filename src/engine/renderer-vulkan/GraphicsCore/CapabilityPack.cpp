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
// CapabilityPack.cpp

#include <unordered_set>

#include "../Memory/DynamicArray.h"

#include "FeaturesConfig.h"
#include "FeaturesConfigMap.h"

#include "CapabilityPack.h"

void SetConfigFeatures( const IteratorSeq<const char* const> featuresStart, const IteratorSeq<const char* const> featuresEnd, const bool optional,
	const FeaturesConfig& cfg, FeaturesConfig* cfgOut, std::unordered_set<const char*>& extensions ) {
	for ( IteratorSeq<const char* const> feature = featuresStart; feature < featuresEnd; feature++ ) {
		const FeatureData& featureData = featuresConfigMap[*feature];

		bool* cfgFeature    = ( bool* ) ( ( ( uint8* ) &cfg )    + featureData.offset );
		bool* cfgOutFeature = ( bool* ) ( ( ( uint8* )  cfgOut ) + featureData.offset );

		if ( optional && !*cfgFeature ) {
			continue;
		}

		*cfgOutFeature = true;

		if ( featureData.version > Version { 1, 4, 0 } && !extensions.contains( featureData.extension.c_str() ) ) {
			extensions.insert( featureData.extension.c_str() );
		}
	}
}

constexpr bool EngineConfigSupportedMinimal( const EngineConfig& config ) {
	const bool ret =
		config.subgroupQuadOpsAllStages && config.robustBufferAccessDynamic
		&& config.quadDivergentImplicitLod
		&& config.textureCompressionBC
		&& config.descriptorIndexing
		&& config.descriptorBindingUpdateUnusedWhilePending
		&& config.descriptorBindingPartiallyBound
		&& config.descriptorBindingVariableDescriptorCount
		&& config.runtimeDescriptorArray
		&& config.shaderSampledImageArrayNonUniformIndexing
		&& config.shaderStorageImageArrayNonUniformIndexing
		&& config.dynamicRendering
		&& config.synchronization2
		&& config.bufferDeviceAddress;

	return ret;
};

CapabilityPackType::Type GetHighestSuppportedCapabilityPack( const EngineConfig& config ) {
	if ( EngineConfigSupportedMinimal( config ) ) {
		return CapabilityPackType::MINIMAL;
	}

	return CapabilityPackType::NONE;
}

void AddRequiredExtensions( const IteratorSeq<const char* const> extensionsStart, const IteratorSeq<const char* const> extensionsEnd,
	std::unordered_set<const char*>& extensions ) {
	for ( IteratorSeq<const char* const> extension = extensionsStart; extension < extensionsEnd; extension++ ) {
		if ( !extensions.contains( *extension ) ) {
			extensions.insert( *extension );
		}
	}
}

DynamicArray<const char*> GetCapabilityPackFeatures( const CapabilityPackType::Type type, const FeaturesConfig& cfg, FeaturesConfig* cfgOut ) {
	std::unordered_set<const char*> extensionsSet;

	memset( cfgOut, 0, sizeof( FeaturesConfig ) );

	switch ( type ) {
		case CapabilityPackType::MINIMAL:
			SetConfigFeatures(      featuresMinimal.begin(),      featuresMinimal.end(), false, cfg, cfgOut, extensionsSet );
			AddRequiredExtensions( extensionsMinimal.begin(), extensionsMinimal.end(), extensionsSet );
		case CapabilityPackType::RECOMMENDED:
			SetConfigFeatures(  featuresRecommended.begin(),  featuresRecommended.end(), false, cfg, cfgOut, extensionsSet );
			AddRequiredExtensions( extensionsMinimal.begin(), extensionsMinimal.end(), extensionsSet );
		case CapabilityPackType::EXPERIMENTAL:
			SetConfigFeatures( featuresExperimental.begin(), featuresExperimental.end(), false, cfg, cfgOut, extensionsSet );
			AddRequiredExtensions( extensionsMinimal.begin(), extensionsMinimal.end(), extensionsSet );
			break;
		case CapabilityPackType::NONE:
		default:
			break;
	}

	SetConfigFeatures( featuresOptional.begin(), featuresOptional.end(), true, cfg, cfgOut, extensionsSet );

	DynamicArray<const char*> extensions;
	extensions.Resize( extensionsSet.size() );
	extensions.Init();

	for ( const char* extension : extensionsSet ) {
		extensions.Push( extension );
	}

	return extensions;
}