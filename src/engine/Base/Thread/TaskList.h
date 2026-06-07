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

#ifndef TASKLIST_H
#define TASKLIST_H

#include "Int.h"
#include "RingBuffer.h"
#include "SysAllocator.h"

#include "Thread.h"
#include "ThreadCommon.h"

#include "Task.h"

using TaskInit = std::initializer_list<TaskProxy>;
#define AddTasks( ... ) AddTasksExt( { __VA_ARGS__ } )

// Use this for task dependencies because it allows natvis visualisation
struct TaskInitList {
	const TaskProxy* start;
	const TaskProxy* end;

	TaskInitList() :
		start( nullptr ),
		end( nullptr ) {
	}

	TaskInitList( const TaskProxy* newStart, const TaskProxy* newEnd ) :
		start( newStart ),
		end( newEnd ) {
	}

	TaskInitList( std::initializer_list<TaskProxy> list ) :
		start( list.begin() ),
		end( list.end() ) {
	}
};

struct ThreadQueue {
	std::atomic<uint64> pointer = 0;
	uint8               current = 0;

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

	void AddTask( const uint32 threadID, const uint16 bufferID );
};

class TaskList :
	public Tag {
	public:
	friend class  Thread;
	friend struct ThreadQueue;

	static constexpr uint32 MAX_TASKS                     = 2048;
	static constexpr uint32 MAX_DATA_PER_TASK             = 128;
	static constexpr uint32 MAX_TASK_DATA                 = MAX_TASKS * MAX_DATA_PER_TASK;

	static constexpr uint16 TASK_SHIFT_ADDED              = 0;
	static constexpr uint16 TASK_SHIFT_HAS_UNTRACKED_DEPS = 1;
	static constexpr uint16 TASK_SHIFT_TRACKED_DEPENDENCY = 2;
	static constexpr uint16 TASK_SHIFT_UPDATED_DEPENDENCY = 3;
	static constexpr uint16 TASK_SHIFT_FORWARD_COUNTER    = 4;

	std::atomic<uint32> currentMaxThreads = 0;
	FenceMain           exitFence;

	TaskList();
	~TaskList();

	void  Init();
	void  Shutdown();
	void  FinishShutdown();

	byte* AllocTaskData( const uint16 dataSize, uint64* offset );
	byte* GetTaskData( const uint64 offset );

	void  AddTask( Task& task, std::initializer_list<TaskProxy> dependencies = {} );
	void  AddTasksExt( std::initializer_list<TaskInitList> dependencies );
	Task* FetchTask();

	void  TaskWait( Task& task );

	void  TasksCleared( const uint32 count );
	void  TaskStarted();
	bool  ThreadFinished( const bool hadTask );

	void  FinishTask( Task* task );
	void  FinishDependency( const uint16 bufferID );

	void  AdjustThreadCount( const uint32 newMaxThreads );

	private:
	struct ThreadExecutionNode {
		uint8 nextThreadExecutionNode;
	};

	AccessLock                   threadCountLock;

	std::atomic<uint64>          threadExecutionNodes[MAX_THREADS];

	AtomicRingBuffer<Task>       tasks     { "GlobalTaskMemory",     &sysAllocator };
	AtomicRingBuffer<byte, true> tasksData { "GlobalTaskDataMemory", &sysAllocator };

	Thread                       threads[MAX_THREADS];

	ThreadQueue                  threadQueues[MAX_THREADS];
	std::atomic<uint32>          taskCount;
	std::atomic<uint32>          taskWithDependenciesCount;

	std::atomic<uint32>          executingThreads = 1;
	std::atomic<bool>            exiting          = false;

	bool  AddedToTaskList( const uint8 id );
	bool  AddedToTaskMemory( const uint16 bufferID );
	bool  HasUntrackedDeps( const uint8 id );
	bool  IsTrackedDependency( const uint8 id );
	bool  IsUpdatedDependency( const uint8 id );
	uint8 GetForwardCounterFast( const uint8 id );
	void  IncrementForwardCounterFast( uint8* id );

	void  AddToThreadQueueExt( Task& task );
	void  AddToThreadQueue( Task& task );

	Task* GetTaskMemory( Task& task );

	void  ResolveDependencies( Task& task, TaskInitList& dependencies );

	void  AddTaskExt( Task& task, TaskInitList&& dependencies = {} );

	void  MarkDependencies( Task& task, TaskInitList&& dependencies );
	void  UnMarkDependencies( TaskInitList&& dependencies );
};

extern TaskList taskList;

#endif // TASKLIST_H