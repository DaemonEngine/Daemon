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

#include "../Sys/MemoryInfo.h"
#include "../Sync/Fence.h"
#include "../Sync/AccessLock.h"

struct Task {
	using TaskFunction = void( * )( void* );

	TaskFunction Execute;
	void* data;

	Fence complete;

	bool active = false;
	bool shutdownTask = false;

	static constexpr uint32_t MAX_FORWARD_TASKS = 18;
	uint16_t forwardTasks[MAX_FORWARD_TASKS] { 0 };

	ALIGN_CACHE std::atomic<uint32_t> dependencyCounter = 1;
	std::atomic<uint32_t> forwardTaskCounter = 0;

	uint16_t id = 0; // LSB->MSB: 2 bits - taskRing, 6 bits - queue, 6 bits - queue slot, 1 bit - added to taskList
	uint16_t bufferID; // Task RingBuffer id
	AccessLock forwardTaskLock;

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

	template<typename FuncType>
	Task( FuncType func, void* newData ) :
		Execute( ( TaskFunction ) func ),
		data( newData ) {
	}

	template<typename FuncType>
	Task( FuncType func, void* newData, FenceMain& fence ) :
		Execute( ( TaskFunction ) func ),
		data( newData ),
		complete( fence ) {
	}

	void operator=( const Task& other );

	Task( const Task& other );

	const Task& operator*() {
		return *this;
	}

	const Task* operator->() const {
		return this;
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
};

#endif // TASK_H