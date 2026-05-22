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

#ifndef TASK_H
#define TASK_H

#include <atomic>

#include "../Math/NumberTypes.h"
#include "../Math/Bit.h"

#include "../Sys/MemoryInfo.h"
#include "../Sync/Fence.h"
#include "../Sync/AccessLock.h"

#include "../Shared/Timer.h"

#include "TaskData.h"

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

struct Task {
	using TaskFunction = void( * )( void* );

	TaskFunction       Execute;
	void*              data;

	FenceMain           complete;

	uint8               flags                 = 0;
	bool                active                = false;

	static constexpr uint32 UNALLOCATED       = UINT16_MAX;
	uint16             bufferID               = UNALLOCATED; // Task RingBuffer id

	uint32             eventMask              = 0;
	uint32             gen                    = 0;

	std::atomic<uint8> dependencyCounter      = 1;
	std::atomic<uint8> forwardTaskCounter     = 0;
	std::atomic<uint8> threadCount            = 0;
	uint8              id                     = 0; // 4 bits - task memory/dependency tracking in TaskList

	AccessLock         forwardTaskLock;
	uint32             forwardTaskCounterFast = 0;

	uint64             time                   = 0;
	uint64             threadMask             = 0;

	uint16             dataSize               = 0;

	static constexpr uint32 MAX_FORWARD_TASKS = 14;
	uint16 forwardTasks[MAX_FORWARD_TASKS]      { 0 };

	byte reserved[18];

	Task();

	// We have to use templates here because clang fails to cast function pointers to void*
	template<typename FuncType>
	Task( FuncType func ) :
		Execute( ( TaskFunction ) func ) {
		SetValid( true );
	}

	template<typename FuncType, typename DataType>
	Task( FuncType func, const DataType& newData ) :
		Execute( ( TaskFunction ) func ) {

		SetValid( true );

		dataSize                = sizeof( newData );
		data                    = AllocTaskData( dataSize );
		*( ( DataType* ) data ) = newData;
	}

	void operator=( const Task& other );

	Task( const Task& other );

	Task& Delay( const uint64 delay );

	Task& ThreadMask( const uint64 newThreadMask );
	Task& ThreadMaskAll();
	Task& ThreadMaskAllOthers();
	Task& ThreadMaskCurrent();

	void  Wait();

	bool  IsValid();
	bool  IsShutdownTask();

	const Task& operator*() {
		return *this;
	}

	const Task* operator->() const {
		return this;
	}

	constexpr Task& GetTask() {
		return *this;
	}

	private:
	static constexpr uint32 validOffset    = 0;
	static constexpr uint32 shutdownOffset = 1;

	void  SetValid( const bool valid );
};

struct TaskProxy {
	Task& task;

	TaskProxy( Task& newTask ) :
		task( newTask ) {
	}

	Task* operator->() const {
		return &task;
	}

	constexpr Task& GetTask() const {
		return task;
	}
};

#endif // TASK_H