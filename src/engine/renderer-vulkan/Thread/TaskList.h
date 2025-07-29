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
// TaskList.h

#ifndef TASKLIST_H
#define TASKLIST_H

#include <stdint.h>
#include <atomic>
#include <mutex>

#include "Task.h"
#include "Thread.h"

#include "../Memory/RingBuffer.h"

struct TaskQueue {
	uint64_t availableTasks = 0;
	uint16_t tasks[64];
};

struct TaskRing {
	std::atomic<uint64_t> queueLocks = 0;
	uint64_t queuesWithTasks = 0;
	TaskQueue queues[64];
};

class TaskList :
	public Tag {
	public:
	friend class Thread;

	static const uint32_t MAX_THREADS = 256;

	TaskList();
	~TaskList();

	uint8_t LockQueue( Task* task );
	void UnlockQueue( const uint8_t queue );
	void AddTask( Task task );
	Task* FetchTask( Thread* thread, const bool longestTask );

	void AdjustThreadCount( const uint32_t newMaxThreads );

	void Init();
	void Shutdown();
	void FinishShutdown();

	private:
	AtomicRingBuffer<Task> tasks { "GlobalTaskMemory" };
	TaskRing taskRing;

	uint32_t currentMaxThreads = 0;
	Thread threads[MAX_THREADS];

	std::atomic_bool exiting = false;
};

extern TaskList taskList;

#endif // TASKLIST_H