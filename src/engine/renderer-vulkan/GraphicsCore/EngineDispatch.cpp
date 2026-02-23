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
// EngineDispatch.cpp

#include <unordered_map>

#include "../Memory/DynamicArray.h"

#include "../Thread/TaskList.h"

#include "Decls.h"

#include "Memory/DescriptorSet.h"
#include "ExecutionGraph/ExecutionGraph.h"
#include "ResourceSystem.h"
#include "Queue.h"

#include "Image.h"

#include "GraphicsCoreStore.h"

#include "../GraphicsShared/MsgStreamAPI.h"

#include "EngineDispatch.h"
#include "../Shared/Timer.h"

static std::unordered_map<std::string, Image> images;

struct Msg {
	uint32* memory;
	uint32  offset;

	uint32  Read() {
		uint32 out = memory[offset];
		offset++;

		return out;
	}

	float   ReadFloat() {
		float out  = *( float* ) &memory[offset];
		offset++;

		return out;
	}

	bool    ReadBool() {
		bool out   = *( bool* ) &memory[offset];
		offset++;

		return out;
	}
};

void MsgStream() {
	Msg msg { resourceSystem.engineToCoreBuffer.memory };

	uint32 msgCount = msg.Read();

	struct ImageCfg {
		uint  id;
		uint  format;
		float relativeSize;
		uint  width;
		uint  height;
		uint  depth;
		bool  useMips;
		bool  cube;
	};

	for ( uint32 i = 0; i < msgCount; i++ ) {
		switch ( msg.Read() ) {
			case CORE_ALLOC_IMAGE:
				ImageCfg cfg {
					.id           = msg.Read(),
					.format       = msg.Read(),
					.relativeSize = msg.ReadFloat(),
					.width        = msg.Read(),
					.height       = msg.Read(),
					.depth        = msg.Read(),
					.useMips      = msg.ReadBool(),
					.cube         = msg.ReadBool()
				};

				Image image;

				image.Init( ( Format ) cfg.format, { cfg.width, cfg.height, cfg.depth }, cfg.useMips, cfg.cube );

				images[Str::Format( "~engineImage_%u", cfg.id )] = image;

				if ( cfg.format == RGBA8S ) {
					UpdateDescriptor( cfg.id, image, RGBA8 );
				} else {
					UpdateDescriptor( cfg.id, image );
				}
		}
	}
}

void EngineDispatch() {
	static ExecutionGraph graphicsEG;

	std::string testSrc =
		"external\n"
		// "buffer testBuffer 3 65536 0\n"
		"push { coreToEngine engineToCore }\n"
		"MsgStream msg1 1 {}\n"
		"present\n";

	Timer t;
	DynamicArray<ExecutionGraphNode> nodes = ParseExecutionGraph( testSrc );
	Log::Warn( "parse: %s", t.FormatTime() );
	t.Clear();
	t.Start();
	graphicsEG.Build( GRAPHICS, 0, nodes );
	Log::Warn( "build: %s", t.FormatTime() );
	t.Clear();
	t.Start();
	uint64 i = graphicsEG.Exec();
	Log::Warn( "exec: %s", t.FormatTime() );

	graphicsQueue.executionPhase.Wait( i );

	MsgStream();

	Task t2 { &EngineDispatch };
	taskList.AddTask( t2.Delay( 10000_us ) );
}
