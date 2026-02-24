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
// ExecutionGraph.h

#ifndef EXECUTION_GRAPH_H
#define EXECUTION_GRAPH_H

#include <atomic>
#include <initializer_list>

#include "../../Memory/BitStream.h"

#include "../../Sync/AccessLock.h"

#include "../Decls.h"

#include "../../GraphicsShared/PushLayout.h"

/* enum NodeType : uint16 {
	NODE_EXECUTION = 1 << 12,
	NODE_GRAPHICS  = 2 << 12,
	NODE_BIND      = 3 << 12,
	NODE_PUSH      = 4 << 12,
	NODE_BUFFER    = 5 << 12,
	NODE_ID_MASK   = NODE_EXECUTION | NODE_GRAPHICS | NODE_BIND | NODE_PUSH | NODE_BUFFER
}; */

enum NodeType : uint8 {
	NODE_EXECUTION = 0,
	NODE_GRAPHICS  = 1,
	NODE_BIND      = 2,
	NODE_PUSH      = 3,
	NODE_BUFFER    = 4,
	NODE_IMAGE     = 5,
	NODE_EXTERNAL  = 6,
	NODE_PRESENT   = 7
};

enum ResourceType : uint32 {
	RESOURCE_VERTEX_READ,
	RESOURCE_FRAGMENT_READ,

	RESOURCE_COMPUTE_WRITE  = RESOURCE_VERTEX_READ,

	RESOURCE_VERTEX_WRITE   = RESOURCE_VERTEX_READ,
	RESOURCE_FRAGMENT_WRITE = RESOURCE_FRAGMENT_READ
};

struct ExecutionGraphNode {
	uint8  type;
	uint8  id;
	uint8  data[23];
};

struct ExecutionNode {
	uint8  type = NODE_EXECUTION;
	uint8  id;
	uint16 computeID;
	uint32 workgroupCount;
	uint32 nodeDependencies;
	uint32 nodeDependencyTypes;
	uint64 graphicsSettings;
};

struct BufferBindNode {
	uint8  type = NODE_BIND;
	uint8  id;
	uint32 indirectBuffer;
	uint32 countBuffer;
	uint32 indexBuffer;
	uint32 vertexBuffer;
	uint32 padding;
};

struct GraphicsNode {
	uint8  type = NODE_GRAPHICS;
	uint8  id;
	uint16 vertexID;
	uint16 fragmentID;
	uint16 pipelineStates;
	uint32 nodeDependencies;
	uint32 nodeDependencyTypes;
	uint64 graphicsSettings;
};

enum PushConstNodeIDs : uint32 {
	PUSH_NOP,
	PUSH_UINT64
};

struct PushConstNodeInit {
	uint32 type;
	uint8  data;
	uint32 size;

	PushConstNodeInit( const uint32 newType, const uint8 newData ) :
		type( newType ),
		data( newData ) {
		switch ( type ) {
			case PUSH_NOP:
				size = 0;
				break;

			case PUSH_UINT64:
				size = 8;
				break;

			default:
				size = 0;
				break;
		}
	}
};

struct PushConstNode {
	uint8  type = NODE_PUSH;
	uint8  id;
	uint8  offset;

	struct Data {
		uint8 size          = 0;
		uint8 specialIDs[4] = {};
		uint8 data[16]      = {};

		Data() {
		}

		Data( std::initializer_list<PushConstNodeInit> specialNodes ) {
			BitStream specialIDsStream { specialIDs };
			BitStream dataStream       { data };

			for ( const PushConstNodeInit& node : specialNodes ) {
				specialIDsStream.Write( node.type, 4 );
				dataStream.Write( node.data, 8 );

				size += node.size;
			}
		}

		Data( std::initializer_list<PushConstNodeInit> specialNodes, std::initializer_list<uint8> extraData ) {
			BitStream specialIDsStream { specialIDs };
			BitStream dataStream       { data };

			for ( const PushConstNodeInit& node : specialNodes ) {
				specialIDsStream.Write( node.type, 4 );
				dataStream.Write( node.data, 8 );

				size += node.size;
			}

			for ( const uint64 extra : extraData ) {
				dataStream.Write( extra, 64 );

				size += 8;
			}
		}
	} data;
};

enum BufferSrc : uint32 {
	BUFFER_EXECUTION_GRAPH,
	BUFFER_EXTRA
};

struct BufferNode {
	uint8     type = NODE_BUFFER;
	uint8     id;
	uint16    bufferID;
	uint64    size;
	uint32    usage;
};

struct ImageNode {
	uint8     type = NODE_IMAGE;
	uint8     id;
	uint16    imageID;
	uint32    TBD[5];
};

struct ExternalNode {
	uint8     type = NODE_EXTERNAL;
	uint8     id;
	bool      acquireSwapChain;
	uint8     padding[21];
};

struct PresentNode {
	uint8     type = NODE_PRESENT;
	uint8     id;
	bool      active;
	uint8     padding[21];
};

class ExecutionGraph {
	public:
	void   BuildFromSrc( const QueueType newType, std::string& newSrc );
	void   Build( const QueueType newType, const uint64 newGenID, DynamicArray<ExecutionGraphNode>& nodes );
	uint64 Exec();

	private:
	static constexpr uint32 cmdBits     = 6;
	static constexpr uint32 cmdPoolBits = 8;
	static constexpr uint32 genIDBits   = 50;

	std::string                      src;

	QueueType                        type;

	ExternalNode                     acquireNode {};
	VkSemaphore                      acquireSemaphore = nullptr;
	PresentNode                      presentNode { .active = false };

	DynamicArray<ExecutionGraphNode> processedNodes {};

	std::atomic<uint64>              cmdID = 0;
};

bool BuildExecutionNode( const uint32 SPIRVID, VkPipeline* pipeline, VkPipelineLayout* pipelineLayout );

DynamicArray<ExecutionGraphNode> ParseExecutionGraph( std::string& src );

void TestCmd();

#endif // EXECUTION_GRAPH_H