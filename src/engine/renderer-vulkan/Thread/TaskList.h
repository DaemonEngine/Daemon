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
	ALIGN_CACHE std::atomic<uint32_t> taskCount = 0;
	TaskQueue queues[64];

	const uint32_t id;

	TaskRing( const uint32_t newID ) :
		id( newID ) {
	}

	uint8_t LockQueueForTask( Task* task );

	void LockQueue( const uint8_t queue );
	void UnlockQueue( const uint8_t queue );

	void RemoveTask( const uint8_t queue, const uint8_t taskID, const bool queueLocked = false );

	void AddToTaskRing( Task& task, const bool unlockQueueAfterAdd = true );
};

using TaskInit = std::initializer_list<TaskProxy>;
#define AddTasks( ... ) AddTasksExt( { __VA_ARGS__ } )

template<typename T>
concept IsTask = requires ( T value ) {
	{ std::is_convertible<T, Task>::value || std::is_convertible<T, TaskProxy>::value };
};

// Use this for task dependencies because it allows natvis visualisation
template<IsTask T>
struct TaskInitList {
	const T* start;
	const T* end;

	TaskInitList( const T* newStart, const T* newEnd ) :
		start( newStart ),
		end( newEnd ) {
	}
};

class TaskList :
	public Tag {
	public:
	friend class Thread;

	static constexpr uint32_t MAX_THREADS = 256;

	enum TaskRingID {
		MAIN = 0,
		FORWARD = 1
	};

	static constexpr uint16_t TASK_RING_MASK = 1;
	static constexpr uint16_t TASK_QUEUE_MASK = 63;
	static constexpr uint16_t TASK_ID_MASK = 63;

	static constexpr uint16_t TASK_SHIFT_QUEUE = 0;
	static constexpr uint16_t TASK_SHIFT_ID = 6;
	static constexpr uint16_t TASK_SHIFT_ALLOCATED = 12;
	static constexpr uint16_t TASK_SHIFT_HAS_UNTRACKED_DEPS = 13;
	static constexpr uint16_t TASK_SHIFT_TRACKED_DEPENDENCY = 14;
	static constexpr uint16_t TASK_SHIFT_UPDATED_DEPENDENCY = 15;

	FenceMain exitFence;

	TaskList();
	~TaskList();

	void Init();
	void Shutdown();
	void FinishShutdown();

	bool AddedToTaskRing( const uint16_t id );
	bool AddedToTaskMemory( const uint16_t id );
	bool HasUntrackedDeps( const uint16_t id );
	bool IsTrackedDependency( const uint16_t id );
	bool IsUpdatedDependency( const uint16_t id );

	uint8_t IDToTaskQueue( const uint16_t id );
	uint16_t IDToTaskID( const uint16_t id );

	void AddTask( Task& task, std::initializer_list<TaskProxy> dependencies = {} );
	void AddTasksExt( std::initializer_list<TaskInit> dependencies );
	Task* FetchTask( Thread* thread, const bool longestTask );

	bool ThreadFinished( const bool hadTask );

	void FinishDependency( const uint16_t bufferID );

	void AdjustThreadCount( const uint32_t newMaxThreads );

	private:
	AtomicRingBuffer<Task> tasks { "GlobalTaskMemory" };
	TaskRing mainTaskRing{ MAIN };

	uint32_t currentMaxThreads = 0;
	Thread threads[MAX_THREADS];

	ALIGN_CACHE std::atomic<uint32_t> executingThreads = 1;
	ALIGN_CACHE std::atomic<bool> exiting = false;

	Task* GetTaskMemory( Task& task );

	template<IsTask T>
	void ResolveDependencies( Task& task, TaskInitList<T>& dependencies );

	template<IsTask T>
	void AddTask( Task& task, TaskInitList<T>&& dependencies );

	template<IsTask T>
	void MarkDependencies( Task& task, TaskInitList<T>&& dependencies );

	template<IsTask T>
	void UnMarkDependencies( TaskInitList<T>&& dependencies );

	void MoveToTaskRing( TaskRing& taskRing, Task& task );
};

extern TaskList taskList;

#endif // TASKLIST_H