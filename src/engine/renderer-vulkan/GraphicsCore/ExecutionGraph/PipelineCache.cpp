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
// PipelineCache.cpp

#include "../../Sys/MemoryInfo.h"

#include "../../Memory/Array.h"

#include "../ResultCheck.h"

#include "../GraphicsCoreStore.h"

#include "../../GraphicsShared/SPIRVIDs.h"
#include "SPIRV.h"
#include "../../GraphicsShared/PushLayout.h"

#include "PipelineCache.h"

enum PipelineLayoutState {
	NONE,
	INITIALISING,
	INITIALISED
};

static ALIGN_CACHE std::atomic<uint32> pipelineLayoutsState[pushConstSizesCount] {};
static VkPipelineLayout                pipelineLayouts[pushConstSizesCount] {};

VkPipelineLayout GetPipelineLayout( uint32 pushConstSizeID ) {
	std::atomic<uint32>& layoutState  = pipelineLayoutsState[pushConstSizeID];
	uint32 layoutStateID              = layoutState.load( std::memory_order_relaxed );
	VkPipelineLayout& pipelineLayout  = pipelineLayouts[pushConstSizeID];

	if ( layoutStateID == INITIALISED ) {
		return pipelineLayout;
	}

	if ( layoutStateID == INITIALISING ) {
		layoutState.wait( INITIALISING, std::memory_order_relaxed );
		return pipelineLayout;
	}

	uint32 expected = NONE;
	if ( !layoutState.compare_exchange_strong( expected, INITIALISING, std::memory_order_relaxed ) ) {
		layoutState.wait( INITIALISING, std::memory_order_relaxed );
		return pipelineLayout;
	}

	VkPushConstantRange pushConst {
		.stageFlags = VK_SHADER_STAGE_ALL,
		.offset     = 0,
		.size       = pushConstSizes[pushConstSizeID]
	};

	VkPipelineLayoutCreateInfo info {
		.setLayoutCount         = 1,
		.pSetLayouts            = &descriptorSetLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges    = &pushConst
	};

	ResultCheckRet( vkCreatePipelineLayout( device, &info, nullptr, &pipelineLayout ) );

	layoutState.store( INITIALISED, std::memory_order_relaxed );
	layoutState.notify_all();

	return pipelineLayout;
}

bool BuildExecutionNode( const uint32 SPIRVID, VkPipeline* pipeline, VkPipelineLayout* pipelineLayout ) {
	VkPipelineRobustnessCreateInfo pipelineRobustnessInfo {
		.storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED,
		.uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED,
		.images         = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DISABLED
	};

	const SPIRVModule& SPIRV = SPIRVBin[SPIRVID];

	VkShaderModuleCreateInfo spirvInfo {
		.pNext    = &pipelineRobustnessInfo,
		.codeSize = SPIRV.size,
		.pCode    = SPIRV.code
	};

	VkPipelineShaderStageCreateInfo pipelineStageInfo {
		.pNext  = &spirvInfo,
		.stage  = VK_SHADER_STAGE_COMPUTE_BIT,
		.pName  = "main"
	};

	*pipelineLayout = GetPipelineLayout( SPIRV.pushConstSize );

	VkComputePipelineCreateInfo computeInfo {
		.stage  = pipelineStageInfo,
		.layout = *pipelineLayout
	};

	ResultCheckRet( vkCreateComputePipelines( device, nullptr, 1, &computeInfo, nullptr, pipeline ) );

	return true;
}

bool BuildGraphicsNode( const uint32 SPIRVIDVertex, const uint32 SPIRVIDFragment,
                        VkPipeline* pipeline, VkPipelineLayout* pipelineLayout ) {
	VkPipelineRobustnessCreateInfo pipelineRobustnessInfo {
		.storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED,
		.uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED,
		.images         = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DISABLED
	};

	const SPIRVModule& SPIRVVertex   = SPIRVBin[SPIRVIDVertex];
	const SPIRVModule& SPIRVFragment = SPIRVBin[SPIRVIDFragment];

	VkShaderModuleCreateInfo spirvVertexInfo {
		.pNext    = &pipelineRobustnessInfo,
		.codeSize = SPIRVVertex.size,
		.pCode    = SPIRVVertex.code
	};

	VkShaderModuleCreateInfo spirvFragmentInfo {
		.pNext    = &pipelineRobustnessInfo,
		.codeSize = SPIRVFragment.size,
		.pCode    = SPIRVFragment.code
	};

	VkPipelineShaderStageCreateInfo pipelineStagesInfo[] {
		{
			.pNext  = &spirvVertexInfo,
			.stage  = VK_SHADER_STAGE_VERTEX_BIT,
			.pName  = "main"
		},
		{
			.pNext  = &spirvFragmentInfo,
			.stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pName  = "main"
		}
	};

	VkPipelineVertexInputStateCreateInfo   vertexInputInfo {};
	VkPipelineTessellationStateCreateInfo  tesselationInfo {};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {
		// .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		.primitiveRestartEnable = true
	};

	VkPipelineRasterizationStateCreateInfo rasterisationInfo {
		.cullMode  = VK_CULL_MODE_BACK_BIT, // VK_CULL_MODE_NONE for transparent materials
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.lineWidth = 1.0f
	};

	VkPipelineSampleLocationsStateCreateInfoEXT sampleLocationsInfo {};

	VkPipelineMultisampleStateCreateInfo   MSAAInfo {
		// .pNext = &sampleLocationsInfo,
		.rasterizationSamples = ( VkSampleCountFlagBits ) 1
	};

	VkPipelineDepthStencilStateCreateInfo  depthStencilInfo {
		.depthTestEnable  = true,
		.depthWriteEnable = true,
		.depthCompareOp   = VK_COMPARE_OP_GREATER_OR_EQUAL
	};

	VkPipelineColorBlendAttachmentState    colorBlendAttachmentInfo {

	};

	VkPipelineColorBlendStateCreateInfo    colorBlendInfo {
		.logicOpEnable   = false,
		.attachmentCount = 1,
		.pAttachments    = &colorBlendAttachmentInfo
	};

	Array dynamicStates {
		VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY, VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
		VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,
		// VK_DYNAMIC_STATE_POLYGON_MODE_EXT
	};

	VkPipelineDynamicStateCreateInfo dynamicStatesInfo {
		.dynamicStateCount = ( uint32 ) dynamicStates.size,
		.pDynamicStates    = dynamicStates.memory
	};

	*pipelineLayout = GetPipelineLayout( std::max( SPIRVVertex.pushConstSize, SPIRVFragment.pushConstSize ) );

	VkGraphicsPipelineCreateInfo graphicsInfo {
		.stageCount          = 2,
		.pStages             = pipelineStagesInfo,
		.pVertexInputState   = &vertexInputInfo,
		.pInputAssemblyState = &inputAssemblyInfo,
		.pTessellationState  = &tesselationInfo,
		.pRasterizationState = &rasterisationInfo,
		.pMultisampleState   = &MSAAInfo,
		.pDepthStencilState  = &depthStencilInfo,
		.pColorBlendState    = &colorBlendInfo,
		.pDynamicState       = &dynamicStatesInfo,
		.layout              = *pipelineLayout
	};

	ResultCheckRet( vkCreateGraphicsPipelines( device, nullptr, 1, &graphicsInfo, nullptr, pipeline ) );

	return true;
}