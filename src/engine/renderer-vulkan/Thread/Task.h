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
// Task.h

#ifndef TASK_H
#define TASK_H

#include <atomic>

#include "../Math/NumberTypes.h"

#include "../Sys/MemoryInfo.h"
#include "../Sync/Fence.h"
#include "../Sync/AccessLock.h"

template<typename T>
struct IsPointer_ {
	static constexpr bool out = false;
};

template<typename T>
struct IsPointer_<T*> {
	static constexpr bool out = true;
};

template<typename T>
constexpr bool IsPointer = IsPointer_<T>::out;

template<typename DataType>
consteval uint16 DataSize( DataType data ) {
	if constexpr ( IsPointer<DataType> ) {
		return sizeof( *data );
	}

	return sizeof( data );
}

struct Task {
	using TaskFunction = void( * )( void* );

	TaskFunction Execute;
	void* data;

	Fence complete;

	bool active = false;
	bool shutdownTask = false;

	static constexpr uint32 MAX_FORWARD_TASKS = 18;
	uint16 forwardTasks[MAX_FORWARD_TASKS] { 0 };

	ALIGN_CACHE std::atomic<uint32> dependencyCounter = 1;
	std::atomic<uint32> forwardTaskCounter = 0;
	uint32 forwardTaskCounterFast = 0;

	uint16 id = 0; // LSB->MSB: 6 bits - queue, 6 bits - queue slot, 4 bits - task memory/dependency tracking in TaskList

	static constexpr uint32 UNALLOCATED = UINT16_MAX;
	uint16 bufferID = UNALLOCATED; // Task RingBuffer id
	AccessLock forwardTaskLock;

	uint16 dataSize = 0;

	// bool useTaskFence = false;
	// Fence taskFence;

	Task();

	// We have to use templates here because clang fails to cast function pointers to void*
	template<typename FuncType>
	Task( FuncType func ) :
		Execute( ( TaskFunction ) func ) {
	}

	template<typename FuncType>
	Task( FuncType func, FenceMain& fence ) :
		Execute( ( TaskFunction ) func ),
		complete( fence ) {
	}

	template<typename FuncType, typename DataType>
	Task( FuncType func, DataType newData ) :
		Execute( ( TaskFunction ) func ) {

		if constexpr ( IsPointer<DataType> ) {
			data = ( void* ) newData;
			dataSize = sizeof( void* );
		} else {
			data = ( void* ) &newData;
			dataSize = sizeof( newData );
		}
	}

	template<typename FuncType, typename DataType>
	Task( FuncType func, DataType newData, FenceMain& fence ) :
		Execute( ( TaskFunction ) func ),
		complete( fence ) {

		if constexpr ( IsPointer<DataType> ) {
			data = ( void* ) newData;
			dataSize = sizeof( void* );
		} else {
			data = ( void* ) &newData;
			dataSize = sizeof( newData );
		}
	}

	void operator=( const Task& other );

	Task( const Task& other );

	const Task& operator*() {
		return *this;
	}

	const Task* operator->() const {
		return this;
	}

	constexpr Task& GetTask() {
		return *this;
	}
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