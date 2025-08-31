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

#include <atomic>
#include <mutex>

#include "../Math/NumberTypes.h"

#include "Thread.h"

#include "../Memory/RingBuffer.h"

#include "Task.h"

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

struct ThreadQueue {
	std::atomic<uint64> pointer = 0;
	uint8 current = 0;

	static constexpr uint16 TASK_NONE = UINT16_MAX;
	static constexpr uint32 MAX_TASKS = 59;

	uint16 tasks[MAX_TASKS] { TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE,
		TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE,
		TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE,
		TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE,
		TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE,
		TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE,
		TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE,
		TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE, TASK_NONE };

	void AddTask( const uint16 bufferID );
};

class TaskList :
	public Tag {
	public:
	friend class Thread;

	static constexpr uint32 MAX_THREADS = 256;

	static constexpr uint32 MAX_TASKS = 2048;
	static constexpr uint32 MAX_DATA_PER_TASK = 128;
	static constexpr uint32 MAX_TASK_DATA = MAX_TASKS * MAX_DATA_PER_TASK;

	static constexpr uint16 TASK_SHIFT_ADDED = 0;
	static constexpr uint16 TASK_SHIFT_HAS_UNTRACKED_DEPS = 1;
	static constexpr uint16 TASK_SHIFT_TRACKED_DEPENDENCY = 2;
	static constexpr uint16 TASK_SHIFT_UPDATED_DEPENDENCY = 3;

	AccessLock threadCountLock;
	std::atomic<uint32> currentMaxThreads = 0;

	FenceMain exitFence;

	TaskList();
	~TaskList();

	void Init();
	void Shutdown();
	void FinishShutdown();

	bool AddedToTaskList( const uint16 id );
	bool AddedToTaskMemory( const uint16 bufferID );
	bool HasUntrackedDeps( const uint16 id );
	bool IsTrackedDependency( const uint16 id );
	bool IsUpdatedDependency( const uint16 id );

	byte* AllocTaskData( const uint16 dataSize );

	void AddTask( Task& task, std::initializer_list<TaskProxy> dependencies = {} );
	void AddTasksExt( std::initializer_list<TaskInit> dependencies );
	Task* FetchTask( Thread* thread, const bool longestTask );

	bool ThreadFinished( const bool hadTask );

	void FinishTask( Task* task );
	void FinishDependency( const uint16 bufferID );

	void AdjustThreadCount( const uint32 newMaxThreads );

	private:
	struct ThreadExecutionNode {
		uint8 nextThreadExecutionNode;
	};

	AtomicRingBuffer<Task> tasks { "GlobalTaskMemory" };
	AtomicRingBuffer<byte, true> tasksData { "GlobalTaskDataMemory" };

	Thread threads[MAX_THREADS];

	std::atomic<uint32> threadExecutionNodes[MAX_THREADS];
	ThreadQueue threadQueues[MAX_THREADS];
	std::atomic<uint32> currentThreadExecutionNode = UINT32_MAX;
	std::atomic<uint32> taskCount;
	std::atomic<uint32> taskWithDependenciesCount;

	ALIGN_CACHE std::atomic<uint32> executingThreads = 1;
	ALIGN_CACHE std::atomic<bool> exiting = false;

	void AddToThreadQueue( Task& task );

	Task* GetTaskMemory( Task& task );

	template<IsTask T>
	void ResolveDependencies( Task& task, TaskInitList<T>& dependencies );

	template<IsTask T>
	void AddTask( Task& task, TaskInitList<T>&& dependencies );

	template<IsTask T>
	void MarkDependencies( Task& task, TaskInitList<T>&& dependencies );

	template<IsTask T>
	void UnMarkDependencies( TaskInitList<T>&& dependencies );
};

extern TaskList taskList;

#endif // TASKLIST_H