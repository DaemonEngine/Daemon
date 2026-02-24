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

#include "../../GraphicsShared/MsgStreamAPI.h"

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

Buffer BuildBufferNode( BufferNode* node ) {
	return Buffer();
}

DynamicArray<Buffer> buffers;

static void ExecPushConstNode( PushConstNode* node, VkCommandBuffer cmd, VkPipelineLayout pipelineLayout ) {
	uint64 data[8];

	BitStream specialIDsStream { node->data.specialIDs };
	BitStream nodeDataStream   { node->data.data };
	BitStream dataStream       { data };

	while ( uint8 specialID = specialIDsStream.Read8( 4 ) ) {
		switch ( specialID ) {
			case PUSH_UINT64:
				dataStream.Write( nodeDataStream.Read64( 64 ), 64 );

				break;

			case PUSH_NOP:
			default:
				break;
		}
	}

	vkCmdPushConstants( cmd, pipelineLayout, VK_SHADER_STAGE_ALL, node->offset, node->data.size, data );
}

uint32 ExecExternalNode( ExternalNode* node, VkSemaphore acquireSemaphore ) {
	if ( !node->acquireSwapChain ) {
		return 0;
	}

	return mainSwapChain.AcquireNextImage( UINT64_MAX, nullptr, acquireSemaphore );
}

void ExecPresentNode( Queue& queue, PresentNode* node, VkSemaphore presentSemaphore, uint32 image ) {
	if ( !node->active ) {
		return;
	}

	VkPresentInfoKHR presentInfo {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores    = &presentSemaphore,

		.swapchainCount     = 1,
		.pSwapchains        = &mainSwapChain.swapChain,
		.pImageIndices      = &image,
		.pResults           = nullptr
	};

	vkQueuePresentKHR( queue.queue, &presentInfo );
}

void ExecutionGraph::BuildFromSrc( const QueueType newType, std::string& newSrc ) {
	type = newType;

	if ( newSrc == src ) {
		return;
	}

	src = newSrc;

	DynamicArray<ExecutionGraphNode> nodes = ParseExecutionGraph( src );

	Build( type, 0, nodes );
}

void ExecutionGraph::Build( const QueueType newType, const uint64 newGenID, DynamicArray<ExecutionGraphNode>& nodes ) {
	if ( !buffers.size ) {
		buffers.Resize( 32 );
		buffers.Zero();
	}

	processedNodes.Resize( nodes.size );
	processedNodes.Zero();

	type = newType;

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

	for ( ExecutionGraphNode& node : nodes ) {
		switch ( node.type ) {
			case NODE_EXECUTION:
			{
				ExecutionNode& executionNode = ( ExecutionNode& ) node;
				BuildExecutionNode( executionNode.computeID, &pipeline, &pipelineLayout );

				uint32 nodeDeps      = executionNode.nodeDependencies;
				uint32 nodeDepsTypes = executionNode.nodeDependencyTypes;

				VkPipelineStageFlags2 srcStage  = 0;
				VkAccessFlags2        srcAccess = 0;

				while ( nodeDeps ) {
					uint32 dep     = FindLSB( nodeDeps );

					switch ( nodes[dep].type ) {
						case NODE_EXECUTION:
							srcStage  |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
							srcAccess |= VK_ACCESS_2_SHADER_WRITE_BIT;
							break;
						case NODE_GRAPHICS:
							srcStage  |= BitSet( nodeDepsTypes, dep )
							             ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
							             : VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
							srcAccess |= BitSet( nodeDepsTypes, dep )
							             ? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
							             : VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
							break;
					}

					UnSetBit( &nodeDeps, dep );
				}

				if ( srcStage || srcAccess ) {
					VkMemoryBarrier2 memoryBarrierInfo {
						.srcStageMask  = srcStage,
						.srcAccessMask = srcAccess,
						.dstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT
					};

					VkDependencyInfo dependencyInfo {
						.memoryBarrierCount = 1,
						.pMemoryBarriers    = &memoryBarrierInfo
					};

					vkCmdPipelineBarrier2( cmd, &dependencyInfo );
				}

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

				uint32 nodeDeps      = graphicsNode.nodeDependencies;
				// uint32 nodeDepsTypes = graphicsNode.nodeDependencyTypes;

				VkPipelineStageFlags2 srcStage;
				VkAccessFlags2        srcAccess;
				VkPipelineStageFlags2 dstStage;
				VkAccessFlags2        dstAccess;

				while ( nodeDeps ) {
					uint32 dep     = FindLSB( nodeDeps );

					switch ( nodes[dep].type ) {
						case NODE_EXECUTION:
							srcStage  |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
							srcAccess |= VK_ACCESS_2_SHADER_WRITE_BIT;
							dstStage  |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT
							          |  VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
							dstAccess |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT | VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
							break;
					}

					UnSetBit( &nodeDeps, dep );
				}

				if ( srcStage || srcAccess ) {
					VkMemoryBarrier2 memoryBarrierInfo {
						.srcStageMask  = srcStage,
						.srcAccessMask = srcAccess,
						.dstStageMask  = dstStage,
						.dstAccessMask = dstAccess
					};

					VkDependencyInfo dependencyInfo {
						.memoryBarrierCount = 1,
						.pMemoryBarriers    = &memoryBarrierInfo
					};

					vkCmdPipelineBarrier2( cmd, &dependencyInfo );
				}

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

				if ( id < buffers.size ) {
					if ( buffers[id].buffer ) {
						break;
					}
				} else {
					buffers.Resize( buffers.size + 1 );
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

			case NODE_EXTERNAL:
			{
				acquireNode = *( ExternalNode* ) &node;

				if ( !acquireSemaphore ) {
					VkSemaphoreTypeCreateInfo semaphoreTypeInfo {
						.semaphoreType = VK_SEMAPHORE_TYPE_BINARY
					};

					VkSemaphoreCreateInfo     semaphoreInfo {
						.pNext = &semaphoreTypeInfo
					};

					vkCreateSemaphore( device, &semaphoreInfo, nullptr, &acquireSemaphore );
				}

				break;
			}

			case NODE_PRESENT:
			{
				presentNode = *( PresentNode* ) &node;

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

uint64 ExecutionGraph::Exec() {
	const uint64 cmd = cmdID.load( std::memory_order_relaxed );

	VkCommandBufferSubmitInfo cmdInfo {
		.commandBuffer = cmdBuffers[GetBits( cmd, cmdBits, cmdPoolBits )][GetBits( cmd, 0, cmdBits )]
	};

	VkSubmitInfo2 submitInfo {
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos    = &cmdInfo
	};

	uint32 image = ExecExternalNode( &acquireNode, acquireSemaphore );

	Queue& queue = GetQueueByType( type );

	uint64 out;

	if ( presentNode.active ) {
		out = queue.SubmitForPresent( cmdInfo.commandBuffer, mainSwapChain.presentSemaphores[image] );
		ExecPresentNode( queue, &presentNode, mainSwapChain.presentSemaphores[image], image );
	} else {
		out = queue.Submit( cmdInfo.commandBuffer );
	}

	return out;
}

void ResetCmdBuffer( const uint32 bufID ) {
	VkCommandBuffer cmd = cmdBuffers[TLM.id][bufID];

	vkResetCommandBuffer( cmd, 0 );

	cmdBufferStates[TLM.id].value.fetch_sub( SetBit( 0u, bufID ), std::memory_order_relaxed );
}

void ParseNodeDeps( const char** text, std::unordered_map<std::string, uint32>& nodes, uint32* nodeDeps, uint32* nodeDepsTypes ) {
	const char* token = COM_ParseExt2( text, false );

	if ( *token != '{' ) {
		return;
	}

	*nodeDeps        = 0;
	*nodeDepsTypes   = 0;
	bool   colourDep = false;
	while( true ) {
		token = COM_ParseExt2( text, false );

		if ( *token == '}' ) {
			break;
		}

		if ( !Q_stricmp( token, "COLOR" ) ) {
			colourDep = true;
			break;
		}

		uint32 dep = nodes[token];

		SetBit( nodeDeps, dep );

		if ( colourDep ) {
			SetBit( nodeDepsTypes, dep );
		}
	}
}

PushConstNode ParsePushConst( const char** text, std::unordered_map<std::string, uint32>& nodes ) {
	const char* token = COM_ParseExt2( text, false );

	if ( *token != '{' ) {
		return {};
	}

	PushConstNode out {};

	BitStream specialIDsStream { out.data.specialIDs };
	BitStream dataStream       { out.data.data };

	while( true ) {
		token = COM_ParseExt2( text, false );

		if ( *token == '}' ) {
			break;
		}

		if ( !Q_stricmp( token, "coreToEngine" ) ) {
			specialIDsStream.Write( PUSH_UINT64, 4 );
			dataStream.Write( resourceSystem.coreToEngineBuffer.engineMemory, 64 );

			out.data.size += 8;
			continue;
		}

		if ( !Q_stricmp( token, "engineToCore" ) ) {
			specialIDsStream.Write( PUSH_UINT64, 4 );
			dataStream.Write( resourceSystem.engineToCoreBuffer.engineMemory, 64 );

			out.data.size += 8;
			continue;
		}

		dataStream.Write( nodes[token], 8 );

		out.data.size += 8;
	}

	return out;
}

DynamicArray<ExecutionGraphNode> ParseExecutionGraph( std::string& src ) {
	const char*  start = src.c_str();
	const char** text  = &start;

	std::unordered_map<std::string, uint32> nodesToSPIRV;
	std::unordered_map<std::string, uint32> nodesToBuffer;

	DynamicArray<ExecutionGraphNode> out;

	uint8 id = 0;

	while ( true ) {
		const char* token = COM_ParseExt2( text, true );
		if ( !token || *token == '\0' ) {
			break;
		}

		if ( SPIRVMap.find( token ) != SPIRVMap.end() ) {
			const uint32 SPIRVID     = SPIRVMap.at( token );

			const SPIRVModule& spirv = SPIRVBin[SPIRVID];

			token                    = COM_ParseExt2( text, true );
			nodesToSPIRV[token]      = id;

			uint32 nodeDeps;
			uint32 nodeDepsTypes;

			switch ( spirv.type ) {
				case SPIRV_COMPUTE:
					token = COM_ParseExt2( text, false );

					int workgroupCount;
					Q_strtoi( token, &workgroupCount );

					ParseNodeDeps( text, nodesToSPIRV, &nodeDeps, &nodeDepsTypes );

					{
						ExecutionNode node {
							.id                  = id,
							.computeID           = ( uint16 ) SPIRVID,
							.workgroupCount      = ( uint32 ) workgroupCount,
							.nodeDependencies    = nodeDeps,
							.nodeDependencyTypes = nodeDepsTypes
						};

						out.Push( *( ExecutionGraphNode* ) &node );
					}

					break;
				case SPIRV_VERTEX:
				case SPIRV_FRAGMENT:
					uint32 vertex;
					uint32 fragment;

					if ( spirv.type == SPIRV_VERTEX ) {
						vertex   = SPIRVID;
						token    = COM_ParseExt2( text, false );
						fragment = SPIRVMap.at( token );
					} else {
						fragment = SPIRVID;
						token    = COM_ParseExt2( text, false );
						vertex   = SPIRVMap.at( token );
					}

					ParseNodeDeps( text, nodesToSPIRV, &nodeDeps, &nodeDepsTypes );

					{
						GraphicsNode node {
							.id                  = id,
							.vertexID            = ( uint16 ) vertex,
							.fragmentID          = ( uint16 ) fragment,
							.nodeDependencies    = nodeDeps,
							.nodeDependencyTypes = nodeDepsTypes
						};

						out.Push( *( ExecutionGraphNode* ) &node );
					}

					break;
			}

			id++;

			continue;
		}

		if ( !Q_stricmp( token, "bind" ) ) {
			int indirect;
			int count;
			int index;
			int vertex;

			token = COM_ParseExt2( text, false );
			Q_strtoi( token, &indirect );

			token = COM_ParseExt2( text, false );
			Q_strtoi( token, &count );

			token = COM_ParseExt2( text, false );
			Q_strtoi( token, &index );

			token = COM_ParseExt2( text, false );
			Q_strtoi( token, &vertex );

			BufferBindNode node {
				.id             = ( uint32 ) id,
				.indirectBuffer = ( uint32 ) indirect,
				.countBuffer    = ( uint32 ) count,
				.indexBuffer    = ( uint32 ) index,
				.vertexBuffer   = ( uint32 ) vertex
			};

			out.Push( *( ExecutionGraphNode* ) &node );

			id++;

			continue;
		}

		if ( !Q_stricmp( token, "push" ) ) {
			PushConstNode node = ParsePushConst( text, nodesToBuffer );
			out.Push( *( ExecutionGraphNode* ) &node );

			id++;

			continue;
		}

		if ( !Q_stricmp( token, "buffer" ) ) {
			token = COM_ParseExt2( text, false );

			std::string name = token;

			int bufferID;
			int size;
			int usage;

			token = COM_ParseExt2( text, false );
			Q_strtoi( token, &bufferID );

			token = COM_ParseExt2( text, false );
			Q_strtoi( token, &size );

			token = COM_ParseExt2( text, false );
			Q_strtoi( token, &usage );

			nodesToBuffer[name] = bufferID;

			BufferNode node {
				.id       = id,
				.bufferID = ( uint16 ) bufferID,
				.size     = ( uint64 ) size,
				.usage    = ( uint32 ) usage
			};

			out.Push( *( ExecutionGraphNode* ) &node );

			id++;

			continue;
		}

		if ( !Q_stricmp( token, "image" ) ) {
			// out.Push( *( ExecutionGraphNode* ) &ParsePushConst( text, nodesToBuffer ) );

			id++;

			continue;
		}

		if ( !Q_stricmp( token, "external" ) ) {
			ExternalNode node {
				.id               = id,
				.acquireSwapChain = true
			};

			out.Push( *( ExecutionGraphNode* ) &node );

			id++;

			continue;
		}

		if ( !Q_stricmp( token, "present" ) ) {
			PresentNode node {
				.id     = id,
				.active = true
			};

			out.Push( *( ExecutionGraphNode* ) &node );

			id++;

			continue;
		}
	}

	return out;
}