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
// ExecutionGraph.cpp

#include <atomic>

#include "../../Math/Bit.h"

#include "../../Memory/Array.h"
#include "../../Memory/DynamicArray.h"

#include "../../Thread/ThreadMemory.h"

#include "../../Sys/MemoryInfo.h"

#include "../Vulkan.h"

#include "../GraphicsCoreStore.h"
#include "../Queue.h"
#include "../ResultCheck.h"

#include "../../GraphicsShared/SPIRVIDs.h"
#include "SPIRV.h"
#include "../../GraphicsShared/PushLayout.h"

#include "../Memory/CoreThreadMemory.h"
#include "../Memory/EngineAllocator.h"
#include "../ResourceSystem.h"

#include "../SwapChain.h"

#include "ExecutionGraph.h"

enum PipelineLayoutState {
	NONE,
	INITIALISING,
	INITIALISED
};

static ALIGN_CACHE std::atomic<uint32> pipelineLayoutsState[pushConstSizesCount] {};
static VkPipelineLayout                pipelineLayouts[pushConstSizesCount] {};

VkPipelineLayout GetPipelineLayout( const VkShaderStageFlags stages, uint32 pushConstSizeID ) {
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
		.stageFlags = stages,
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
	const SPIRVModule& SPIRV = SPIRVBin[SPIRVID];

	VkShaderModuleCreateInfo spirvInfo {
		.codeSize = SPIRV.size,
		.pCode    = SPIRV.code
	};

	VkShaderModule module;
	ResultCheckRet( vkCreateShaderModule( device, &spirvInfo, nullptr, &module ) );

	/* VkPipelineRobustnessCreateInfo pipelineRobustness {
		.storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED,
		.uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED,
		.images         = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DISABLED
	}; */

	VkPipelineRobustnessCreateInfo pipelineRobustness {
		.storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT,
		.uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT,
		.images         = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DEVICE_DEFAULT
	};

	VkPipelineShaderStageCreateInfo pipelineStageInfo {
		.pNext  = &pipelineRobustness,
		.stage  = SPIRV.stage,
		.module = module,
		.pName  = "main"
	};

	*pipelineLayout = GetPipelineLayout( VK_SHADER_STAGE_COMPUTE_BIT, SPIRV.pushConstSize );

	VkComputePipelineCreateInfo computeInfo {
		.stage  = pipelineStageInfo,
		.layout = *pipelineLayout
	};

	ResultCheckRet( vkCreateComputePipelines( device, nullptr, 1, &computeInfo, nullptr, pipeline ) );

	return true;
}

bool BuildGraphicsNode( const uint32 SPIRVIDVertex, const uint32 SPIRVIDFragment,
	VkPipeline* pipeline, VkPipelineLayout* pipelineLayout ) {
	const SPIRVModule& SPIRVVertex   = SPIRVBin[SPIRVIDVertex];
	const SPIRVModule& SPIRVFragment = SPIRVBin[SPIRVIDFragment];

	VkShaderModuleCreateInfo spirvInfoVertex {
		.codeSize = SPIRVVertex.size,
		.pCode    = SPIRVVertex.code
	};

	VkShaderModule moduleVertex;
	ResultCheckRet( vkCreateShaderModule( device, &spirvInfoVertex, nullptr, &moduleVertex ) );

	VkShaderModuleCreateInfo spirvInfoFragment {
		.codeSize = SPIRVFragment.size,
		.pCode    = SPIRVFragment.code
	};

	VkShaderModule moduleFragment;
	ResultCheckRet( vkCreateShaderModule( device, &spirvInfoFragment, nullptr, &moduleFragment ) );

	/* VkPipelineRobustnessCreateInfo pipelineRobustness {
		.storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED,
		.uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED,
		.images         = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DISABLED
	}; */

	VkPipelineRobustnessCreateInfo pipelineRobustness {
		.storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT,
		.uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT,
		.images         = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DEVICE_DEFAULT
	};

	VkPipelineShaderStageCreateInfo pipelineStagesInfo[] {
		{
			.pNext  = &pipelineRobustness,
			.stage  = SPIRVVertex.stage,
			.module = moduleVertex,
			.pName  = "main"
		},
		{
			.pNext  = &pipelineRobustness,
			.stage  = SPIRVFragment.stage,
			.module = moduleFragment,
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

	*pipelineLayout = GetPipelineLayout( VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		std::max( SPIRVVertex.pushConstSize, SPIRVFragment.pushConstSize ) );

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

Buffer BuildBufferNode( BufferNode* node ) {
	return Buffer();
}

Buffer extraBuffers[2];
DynamicArray<Buffer> buffers;

static void ExecPushConstNode( PushConstNode* node, VkCommandBuffer cmd, VkPipelineLayout pipelineLayout ) {
	uint64 data[8];

	BitStream specialIDsStream { node->data.specialIDs };
	BitStream nodeDataStream   { node->data.data };
	BitStream dataStream       { data };

	while ( uint8 specialID = specialIDsStream.Read8( 4 ) ) {
		switch ( specialID ) {
			case PUSH_BUFFER_ADDRESS:
				dataStream.Write( buffers[nodeDataStream.Read8( 8 )].engineMemory,      64 );

				break;

			case PUSH_BUFFER_EXTRA_ADDRESS:
				dataStream.Write( extraBuffers[nodeDataStream.Read8( 8 )].engineMemory, 64 );

				break;

			case PUSH_NOP:
			default:
				break;
		}
	}

	vkCmdPushConstants( cmd, pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
		node->offset, node->data.size, data );
}

void ExecutionGraph::Build( const uint64 newGenID, DynamicArray<ExecutionNode>& nodes ) {
	if ( !buffers.size ) {
		buffers.Resize( 32 );
		buffers.Zero();
	}

	processedNodes.Resize( nodes.size );
	processedNodes.Zero();

	uint64 state        = cmdBufferStates[TLM.id].value.load( std::memory_order_relaxed );
	uint32 bufID        = FindZeroBitFast( state );

	if ( !BitSet( cmdBufferAllocState, bufID ) ) {
		VkCommandBufferAllocateInfo cmdInfo {
			.commandPool        = GMEM.graphicsCmdPool,
			.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		vkAllocateCommandBuffers( device, &cmdInfo, &cmdBuffers[TLM.id][bufID] );

		UnSetBit( &cmdBufferAllocState, bufID );
	}

	VkCommandBuffer cmd = cmdBuffers[TLM.id][bufID];

	cmdBufferStates[TLM.id].value.fetch_add( SetBit( 0u, bufID ), std::memory_order_relaxed );

	VkCommandBufferBeginInfo cmdBegin {
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
	};

	vkBeginCommandBuffer( cmd, &cmdBegin );

	VkPipelineLayout pipelineLayout;
	VkPipeline       pipeline;

	VkBuffer         indirectBuffer;
	VkBuffer         countBuffer;

	PushConstNode* pushNode = nullptr;

	for ( ExecutionNode& node : nodes ) {
		switch ( node.type ) {
			case NODE_EXECUTION:
			{
				ExecutionNode& executionNode = node;
				BuildExecutionNode( executionNode.computeID, &pipeline, &pipelineLayout );

				uint64 nodeDeps = executionNode.nodeDependencies;
				uint32 readDeps = executionNode.readResources;

				DynamicArray<VkBufferMemoryBarrier2> bufferBarriers;

				uint32 i = 0;
				while ( nodeDeps ) {
					uint32 dep     = FindLSB( nodeDeps );
					uint32 depType = BitSet( nodeDeps, dep + 1 );

					uint32 depMask = readDeps & nodes[dep].writeResources;
					i             += CountBits( depMask );
				}

				bufferBarriers.Resize( i );

				i = 0;

				while ( nodeDeps ) {
					uint32 dep     = FindLSB( nodeDeps );
					uint32 depType = BitSet( nodeDeps, dep + 1 );

					uint32 depMask = readDeps & nodes[dep].writeResources;

					const bool executionDep = nodes[dep].type == NODE_EXECUTION;
					const bool fragmentDep  = executionDep && depType == RESOURCE_FRAGMENT_WRITE;

					while ( depMask ) {
						bufferBarriers[i]  = VkBufferMemoryBarrier2 {
							.srcStageMask  = executionDep
								? VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
								: VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
							.srcAccessMask = fragmentDep
								? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
								: VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
							.dstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
							.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
							.buffer        = buffers[FindLSB( depMask )].buffer,
							.offset        = 0,
							.size          = VK_WHOLE_SIZE
						};

						UnSetBit( &depMask, FindLSB( depMask ) );

						i++;
					}
				}

				VkDependencyInfo dependencyInfo {
					.bufferMemoryBarrierCount = ( uint32 ) bufferBarriers.size,
					.pBufferMemoryBarriers    = bufferBarriers.memory
				};

				vkCmdPipelineBarrier2( cmd, &dependencyInfo );

				vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline );
				vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr );

				if ( pushNode ) {
					ExecPushConstNode( pushNode, cmd, pipelineLayout );
				}

				vkCmdDispatch( cmd, executionNode.workgroupCount, 1, 1 );

				break;
			}

			case NODE_GRAPHICS:
			{
				GraphicsNode& graphicsNode = *( GraphicsNode* ) &node;
				BuildGraphicsNode( graphicsNode.vertexID, graphicsNode.fragmentID, &pipeline, &pipelineLayout );

				uint64 nodeDeps = graphicsNode.nodeDependencies;
				uint32 readDeps = graphicsNode.readResources;

				DynamicArray<VkBufferMemoryBarrier2> bufferBarriers;

				uint32 i = 0;
				while ( nodeDeps ) {
					uint32 dep     = FindLSB( nodeDeps );
					uint32 depType = BitSet( nodeDeps, dep + 1 );

					uint32 depMask = readDeps & nodes[dep].writeResources;
					i             += CountBits( depMask );
				}

				bufferBarriers.Resize( i );

				i = 0;

				while ( nodeDeps ) {
					uint32 dep     = FindLSB( nodeDeps );
					uint32 depType = BitSet( nodeDeps, dep + 1 );

					uint32 depMask = readDeps & nodes[dep].writeResources;

					const bool executionDep = nodes[dep].type == NODE_EXECUTION;
					const bool fragmentDep  = executionDep && depType == RESOURCE_FRAGMENT_READ;

					while ( depMask ) {
						Buffer& buffer = buffers[FindLSB( depMask )];

						VkPipelineStageFlags2 dstStage = fragmentDep ? VK_ACCESS_2_SHADER_STORAGE_READ_BIT : 0;

						dstStage |= ( buffer.usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT ) ? VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT : 0;
						dstStage |= ( buffer.usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT )    ? VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT   : 0;

						VkAccessFlags2 dstAccess = fragmentDep ? VK_ACCESS_2_SHADER_STORAGE_READ_BIT : 0;

						dstStage |= ( buffer.usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT ) ? VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT : 0;
						dstStage |= ( buffer.usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT )    ? VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT   : 0;
						dstStage |= ( buffer.usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT )
							? VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT : VK_ACCESS_2_SHADER_STORAGE_READ_BIT;

						bufferBarriers[i]  = VkBufferMemoryBarrier2 {
							.srcStageMask  = executionDep
								? VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
								: VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
							.srcAccessMask = fragmentDep
								? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
								: VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
							.dstStageMask  = dstStage,
							.dstAccessMask = dstAccess,
							.buffer        = buffer.buffer,
							.offset        = 0,
							.size          = VK_WHOLE_SIZE
						};

						UnSetBit( &depMask, FindLSB( depMask ) );

						i++;
					}
				}

				VkDependencyInfo dependencyInfo {
					.bufferMemoryBarrierCount = ( uint32 ) bufferBarriers.size,
					.pBufferMemoryBarriers    = bufferBarriers.memory
				};

				vkCmdPipelineBarrier2( cmd, &dependencyInfo );

				vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
				vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr );

				if ( pushNode ) {
					ExecPushConstNode( pushNode, cmd, pipelineLayout );
				}

				vkCmdDrawIndexedIndirectCount( cmd, indirectBuffer, 0, countBuffer, 0, 1, 0 );

				break;
			}

			case NODE_BUFFER:
			{
				BufferNode& bufferNode = *( BufferNode* ) &node;

				uint32 id = node.id;

				if ( id < buffers.elements ) {
					if ( buffers[id].buffer ) {
						break;
					}
				} else {
					buffers.Resize( buffers.elements + 1 );
				}

				// MemoryRequirements reqs = GetBufferRequirements( bufferNode.usage, bufferNode.size );

				buffers[id] = resourceSystem.AllocBuffer( bufferNode.size, ( Buffer::Usage ) bufferNode.usage );

				break;
			}

			case NODE_PUSH:
			{
				pushNode = ( PushConstNode* ) &node;

				break;
			}

			case NODE_BIND:
			{
				BufferBindNode& bindNode = *( BufferBindNode* ) &node;

				vkCmdBindIndexBuffer( cmd, buffers[bindNode.indexBuffer].buffer, 0, VK_INDEX_TYPE_UINT32 );

				indirectBuffer = buffers[bindNode.indirectBuffer].buffer;
				countBuffer    = buffers[bindNode.countBuffer].buffer;

				break;
			}
		}
	}

	vkEndCommandBuffer( cmd );

	uint64 combinedGenID = SetBits( ( uint64 ) ( TLM.id << cmdBits ) | bufID, newGenID, cmdPoolBits + cmdBits, genIDBits );
	uint64 expected      = cmdID.load( std::memory_order_relaxed );

	do {
		if ( combinedGenID < expected ) {
			vkResetCommandBuffer( cmd, 0 );
			break;
		}
	} while ( !cmdID.compare_exchange_strong( expected, combinedGenID ) );
}

void ExecutionGraph::Exec() {
	const uint64 cmd = cmdID.load( std::memory_order_relaxed );

	VkCommandBufferSubmitInfo cmdInfo {
		.commandBuffer = cmdBuffers[GetBits( cmd, cmdBits, cmdPoolBits )][GetBits( cmd, 0, cmdBits )]
	};

	VkSubmitInfo2 submitInfo {
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos    = &cmdInfo
	};

	vkQueueSubmit2( graphicsQueue.queue, 1, &submitInfo, nullptr );
}

void ResetCmdBuffer( const uint32 bufID ) {
	VkCommandBuffer cmd = cmdBuffers[TLM.id][bufID];

	vkResetCommandBuffer( cmd, 0 );

	cmdBufferStates[TLM.id].value.fetch_sub( SetBit( 0u, bufID ), std::memory_order_relaxed );
}

void TestCmd() {
	extraBuffers[0] = engineAllocator.AllocDedicatedBuffer( MemoryHeap::CORE_TO_ENGINE, 65536 );
	memset( extraBuffers[0].memory, 0, 65536 );

	for ( int i = 0; i < 64; i++ ) {
		extraBuffers[0].memory[i] = i % 3;
	}

	extraBuffers[1] = engineAllocator.AllocDedicatedBuffer( MemoryHeap::ENGINE_TO_CORE, 65536 );
	memset( extraBuffers[1].memory, 0, 65536 );

	BufferNode testBuffer {
		.id = 2,
		.size = 65536
	};

	PushConstNode testPush {
		.id = 1,
		.offset = 0,
		.data = {
			{ { PUSH_BUFFER_EXTRA_ADDRESS, 0 }, { PUSH_BUFFER_EXTRA_ADDRESS, 1 } },
			// { 0, 1, 2, 3, 4, 5, 6 , 7, 8, 9, 10 }
		}
	};

	ExecutionNode testExec {
		.id = 2,
		.computeID = MsgStream,
		.workgroupCount = 1
	};

	DynamicArray<ExecutionNode> nodes { *( ExecutionNode* ) &testBuffer, *( ExecutionNode* ) &testPush, testExec };

	InitCmdPools();

	ExecutionGraph testEG;
	testEG.Build( 0, nodes );

	VkSemaphoreTypeCreateInfo sInfo {
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = 0
	};

	VkSemaphoreCreateInfo sInfo2 {
		.pNext = &sInfo
	};

	VkSemaphore semaphore;
	vkCreateSemaphore( device, &sInfo2, nullptr, &semaphore );

	DynamicArray<VkImage> images;
	images.Resize( mainSwapChain.imageCount );

	uint32 imageCount = mainSwapChain.imageCount;
	vkGetSwapchainImagesKHR( device, mainSwapChain.swapChain, &imageCount, images.memory );

	uint64 waitValue = 1;
	Timer t;
	for ( int j = 0; j < 100; j++ ) {
		VkSemaphoreWaitInfo waitInfo {
			.semaphoreCount = 1,
			.pSemaphores = &semaphore,
			.pValues = &waitValue
		};

		VkTimelineSemaphoreSubmitInfo qsInfo {
			.signalSemaphoreValueCount = 1,
			.pSignalSemaphoreValues = &waitValue
		};

		/* VkSubmitInfo qInfo {
			.pNext = &qsInfo,
			.commandBufferCount = 1,
			.pCommandBuffers = &cmd,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &semaphore
		};

		vkQueueSubmit( graphicsQueue.queues[0], 1, &qInfo, nullptr );

		vkWaitSemaphores( device, &waitInfo, UINT64_MAX ); */

		testEG.Exec();

		/* std::string a;
		for ( int i = 0; i < 64; i++ ) {
			a += Str::Format( "%u ", extraBuffers[1].memory[i] );
		}

		Log::Notice( a ); */

		/* uint32 index;
		while ( true ) {
			VkResult res = vkAcquireNextImageKHR( device, mainSwapChain.swapChain, UINT64_MAX, nullptr, nullptr, &index );
			if ( res == VK_SUCCESS ) {
				break;
			}
		}

		VkPresentInfoKHR presentInfo {};
		presentInfo.waitSemaphoreCount = 0;
		presentInfo.pWaitSemaphores = &semaphore;

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &mainSwapChain.swapChain;
		presentInfo.pImageIndices = &index;
		presentInfo.pResults = nullptr;

		vkQueuePresentKHR( graphicsQueue.queues[0], &presentInfo );

		vkDeviceWaitIdle( device ); */

		waitValue++;
	}
	vkDeviceWaitIdle( device );
	Log::Notice( t.FormatTime() );
}