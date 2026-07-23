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
#include "RingBufferArray.h"
#include "SysAllocator.h"

#include "TaskEnv.h"
#include "TaskID.h"
#include "Thread.h"
#include "ThreadCommon.h"

#include "Task.h"

// Use this for task dependencies because it allows natvis visualisation

struct ThreadQueue {
	std::atomic<uint64> pointer = 0;
	uint8               current = 0;

	static constexpr uint32 maxTasks  = 59;

	TaskID tasks[maxTasks] {};

	void AddTask( const uint32 threadID, const TaskID& task );
};

struct AtomicThreadRunTime;

struct ThreadRunTime {
	uint64 time[MAX_THREADS] {};

	              ThreadRunTime() = default;
	              ThreadRunTime( const AtomicThreadRunTime& other );

	void          operator=( const AtomicThreadRunTime& other );
	ThreadRunTime operator-( const ThreadRunTime& other );
};

struct AtomicThreadRunTime {
	std::atomic<uint64> time[MAX_THREADS] {};

	void operator=( const ThreadRunTime& other );
	void operator+=( const ThreadRunTime& other );
};

class TaskList :
	public Tag {
	public:
	friend class  Thread;
	friend struct ThreadQueue;

	FenceMain exitFence;

	         TaskList();
	         ~TaskList();

	void     Init();
	void     Shutdown();
	void     FinishShutdown();

	byte*    AllocTaskData( const uint16 dataSize, uint64* offset );
	byte*    GetTaskData( const uint64 offset );

	void     AddTasksExt( std::initializer_list<TaskInitList> dependencies );
	TaskEnv* FetchTask();

	TaskEnv* InitTaskEnv( Task* task );

	void     TaskWait( const Task& task );

	void     TasksCleared( const uint32 count );
	void     TaskStarted();
	bool     ThreadFinished( const bool hadTask );

	void     UpdateThreadRunTime( const uint64 time );
	void     FinishTask( TaskEnv* task );

	void     SetActiveThreads( const uint64 threadMask );

	TaskEnv& BufferIDToTask( const uint16 bufferID );

	private:
	static constexpr uint32 maxThreadTasks                = 512;
	static constexpr uint32 dataPerTask                   = 128;
	static constexpr uint32 maxThreadTaskData             = maxThreadTasks * dataPerTask;

	uint64                            coreMask;

	AccessLock                        threadCountLock;

	static constexpr uint32           taskIDThreadOffset = 9;
	static constexpr uint32           taskIDThreadBits   = 7;

	AtomicRingBufferArray<TaskEnv>    tasks     { "GlobalTaskMemory",     &sysAllocator };
	AtomicRingBufferArray<byte, true> tasksData { "GlobalTaskDataMemory", &sysAllocator };

	AtomicThreadRunTime               threadRunTime;

	Thread                            threads[MAX_THREADS];

	FenceMain                         threadInitCount {};

	ThreadQueue                       threadQueues[MAX_THREADS];
	std::atomic<uint32>               taskCount;
	std::atomic<uint32>               taskWithDependenciesCount;

	std::atomic<uint32>               executingThreads = 1;

	bool     AddedToTaskList( const Task& task );
	bool     IsUpdatedDependency( const Task& task );

	void     AddToThreadQueue( const Task& task, ThreadRunTime* runTime );

	void     AddTaskExt( Task& task, ThreadRunTime* runTime );

	void     MarkDependencies( const Task& task, const TaskInitList& dependencies );
	void     UnMarkDependencies( const TaskInitList& dependencies );

	void     ThreadInitialised();
};

extern TaskList taskList;

#endif // TASKLIST_H