/*
=============================================================================
Daemon-Vulkan BSD Source Code
Copyright (c) 2025-2026 Reaper
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the Reaper nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL REAPER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=============================================================================
*/

#ifndef TASK_ENV_H
#define TASK_ENV_H

#include <atomic>

#include "Int.h"
#include "Bit.h"
#include "Memory.h"
#include "AccessLock.h"
#include "Fence.h"

/* template<typename T>
struct IsPointer_ {
	static constexpr bool out = false;
};

template<typename T>
struct IsPointer_<T*> {
	static constexpr bool out = true;
};

template<typename T>
constexpr bool IsPointer = IsPointer_<T>::out; */

struct Arg;
using TaskFunction = void( * )( void* );

struct TaskEnv {
	TaskFunction       Execute;

	uint16             dataOffsets[4]           { 0 };
	uint64             pad                    = 0;
	// 40 bits for task data so it supports up to ~207 days with 1024 tasks with 64 byte args per frame on average @ 60 FPS
	uint32             dataOffset             = 0;
	uint8              dataOffset2            = 0;

	uint8              flags                  = 0;

	uint16             bufferID               = 0; // Task RingBuffer id
	uint32             gen                    = 0;

	uint32             argsMap                = 0; // Bits 0-7: destructor map, 8-31: arg id -> dataOffsets remap

	uint64             time                   = 0;
	uint64             threadMask             = 0;

	uint8              pad2                   = 0;
	std::atomic<uint8> dependencyCounter      = 1; // Starts at 1 so it wouldn't start executing before being resolved in AddTask[s]
	uint8              forwardTaskCounter     = 0;

	FenceBool          complete;
	AccessLock         threadCount            = {};

	uint16             srcLine                = 0;

	static constexpr uint32 maxForwardTasks   = 8;
	uint16 forwardTasks[maxForwardTasks]        { 0 };

	static constexpr uint32 maxSrcSize  = 24;
	static constexpr uint32 maxNameSize = 24;

	char               src[maxSrcSize];
	char               name[maxNameSize];

	                   TaskEnv();
	                   TaskEnv( const TaskEnv& other );

	void               operator=( const TaskEnv& other );

	TaskEnv&           Delay( const uint64 delay );

	TaskEnv&           ThreadMask( const uint64 newThreadMask );
	TaskEnv&           ThreadMaskAll();
	TaskEnv&           ThreadMaskAllOthers();
	TaskEnv&           ThreadMaskCurrent();

	void               Wait();

	void               ExecuteDestructors();

	bool               IsShutdownTask();
	bool               IsActive();
	uint8              GetArgCount();

	uint64             GetDataOffset();

	void               SetActive( const bool active );

	byte*              GetArgMemory( const uint32 arg );

	byte*              InitMemory( Arg* start, Arg* end, TaskFunction execute, uint64* dataOffsetsOut );

	private:
	static constexpr uint32 activeOffset    = 0;
	static constexpr uint32 shutdownOffset  = 1;
	static constexpr uint32 argCountOffset  = 2;

	static constexpr uint32 argMapArgOffset = 8;
	static constexpr uint32 argMapMask      = 255;
	static constexpr uint32 argMapArgSize   = 3;

	uint32             SetArgsMap( Arg* start, Arg* end );
	uint32             RemapArg( const uint32 arg );

	void               SetValid( const bool valid );
};

#endif // TASK_ENV_H