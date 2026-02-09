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
// TaskList.cpp

#include "../Math/Bit.h"
#include "../Sys/CPUInfo.h"
#include "../MiscCVarStore.h"

#include "ThreadMemory.h"
#include "GlobalMemory.h"
#include "ThreadUplink.h"
#include "EventQueue.h"

#include "TaskList.h"

TaskList taskList;

static void SyncThreadCount() {
	TLM.currentMaxThreads = taskList.currentMaxThreads.load( std::memory_order_relaxed );
}

TaskList::TaskList() {
}

TaskList::~TaskList() {
}

void TaskList::AdjustThreadCount( uint32 newMaxThreads ) {
	if ( newMaxThreads > MAX_THREADS ) {
		Log::WarnTag( "Maximum thread count exceeded: %u > %u, setting to %u",
			newMaxThreads, MAX_THREADS, MAX_THREADS );
		newMaxThreads = MAX_THREADS;
	}

	if ( newMaxThreads == 0 ) {
		Log::WarnTag( "Thread count can't be 0" );
		return;
	}

	while ( !threadCountLock.LockWrite() );

	// This must always be set before using ThreadMask*() on main thread
	TLM.currentMaxThreads = newMaxThreads;

	const uint32 currentThreads = currentMaxThreads.load( std::memory_order_relaxed );
	if ( newMaxThreads > currentThreads ) {
		currentMaxThreads.store( newMaxThreads, std::memory_order_relaxed );

		taskList.AddTask( Task { &SyncThreadCount }.ThreadMaskAll() );

		for ( uint32 i = currentThreads; i < newMaxThreads; i++ ) {
			threads[i].Start( i );
		}
	} else if ( newMaxThreads < currentThreads ) {
		currentMaxThreads.store( newMaxThreads, std::memory_order_relaxed );

		taskList.AddTask( Task { &SyncThreadCount }.ThreadMaskAll() );

		for ( uint32 i = newMaxThreads; i < currentThreads; i++ ) {
			threads[i].exiting = true;
		}
	}

	if ( !TLM.main ) {
		threadUplink.AddCommand( ThreadUplink::CMD_SYNC_THREAD_COUNT );
	}

	Log::NoticeTag( "Changed thread count to %u", TLM.currentMaxThreads );

	threadCountLock.UnlockWrite();
}

void TaskList::Init() {
	tasks.Alloc( MAX_TASKS );
	tasksData.Alloc( MAX_TASK_DATA );

	int threads = r_vkThreadCount.Get();
	threads = threads ? threads : CPU_CORES;
	AdjustThreadCount( threads );
}

void TaskList::Shutdown() {
	if ( exiting.load( std::memory_order_relaxed ) ) {
		Log::WarnTag( "Shutdown() has already been called!" );
		return;
	}

	executingThreads.fetch_sub( 1, std::memory_order_relaxed );
	exiting.store( true, std::memory_order_relaxed );
}

void TaskList::FinishShutdown() {
	if ( !TLM.main ) {
		Log::WarnTag( "FinishShutdown() can only be called from the main thread!" );
		return;
	}

	for ( Thread* thread = threads; thread < threads + currentMaxThreads.load( std::memory_order_relaxed ); thread++ ) {
		thread->Exit();
	}

	Log::NoticeTag( "\nmain: add: %s, addQueueWait: %s, sync: %s, unknownTaskCount: %u",
		TLM.addTimer.FormatTime( Timer::ms ), TLM.addQueueWaitTimer.FormatTime( Timer::ms ),
		TLM.syncTimer.FormatTime( Timer::ms ),
		TLM.unknownTaskCount );

	std::string debugOut;
	debugOut.reserve( 3 * currentMaxThreads.load( std::memory_order_relaxed ) );
	for ( uint32 i = 0; i < currentMaxThreads.load( std::memory_order_relaxed ); i++ ) {
		debugOut += Str::Format( "%u ", TLM.idleThreads[i] );
	}
	
	Log::NoticeTag( debugOut );
}

bool TaskList::AddedToTaskList( const uint16 id ) {
	return BitSet( id, TASK_SHIFT_ADDED );
}

bool TaskList::AddedToTaskMemory( const uint16 bufferID ) {
	return bufferID != Task::UNALLOCATED;
}

bool TaskList::HasUntrackedDeps( const uint16 id ) {
	return BitSet( id, TASK_SHIFT_HAS_UNTRACKED_DEPS );
}

bool TaskList::IsTrackedDependency( const uint16 id ) {
	return BitSet( id, TASK_SHIFT_TRACKED_DEPENDENCY );
}

bool TaskList::IsUpdatedDependency( const uint16 id ) {
	return BitSet( id, TASK_SHIFT_UPDATED_DEPENDENCY );
}

byte* TaskList::AllocTaskData( const uint16 dataSize ) {
	return tasksData.GetNextElementMemory( dataSize );
}

void TaskList::FinishTask( Task* task ) {
	if ( task->dataSize ) {
		tasksData.UpdateCurrentElement( ( byte* ) task->data - tasksData.memory );
	}
}

void TaskList::FinishDependency( const uint16 bufferID ) {
	Task& task = tasks[bufferID];

	const uint32 counter = task.dependencyCounter.fetch_sub( 1, std::memory_order_relaxed ) - 1;

	if ( !counter ) {
		TLM.addTimer.Start();

		if ( TimeNs() < task.time ) {
			eventQueue.AddTask( task );
		} else {
			AddToThreadQueue( task );
			taskWithDependenciesCount.fetch_sub( 1, std::memory_order_relaxed );
		}

		TLM.addTimer.Stop();
	}
}

template<IsTask T>
void TaskList::ResolveDependencies( Task& task, TaskInitList<T>& dependencies ) {
	for ( const T* dep = dependencies.start; dep < dependencies.end; dep++ ) {
		if ( !AddedToTaskMemory( ( *dep )->bufferID ) ) {
			Sys::Drop( "Tried to add task with an unallocated dependency" );
		}

		if ( IsTrackedDependency( ( *dep )->id ) ) {
			continue;
		}

		Task& dependency  = tasks[( *dep )->bufferID];

		// The dependency has already been executed, but the ringbuffer wrapped around
		if ( dependency.gen > ( *dep )->gen ) {
			continue;
		}

		const bool locked = dependency.forwardTaskLock.Lock();

		// Already finished execution
		if ( !locked ) {
			continue;
		}

		uint32 id                   = dependency.forwardTaskCounter.fetch_add( 1, std::memory_order_relaxed );

		ASSERT_LE( id, Task::MAX_FORWARD_TASKS );

		dependency.forwardTasks[id] = task.bufferID;

		task.dependencyCounter.fetch_add( 1, std::memory_order_relaxed );

		dependency.forwardTaskLock.Unlock();
	}
}

void ThreadQueue::AddTask( const uint32 threadID, const uint16 bufferID ) {
	TLM.addQueueWaitTimer.Start();

	if ( threadID == TLM.id && !TLM.main ) {
		TLM.AddTask( &taskList.tasks[bufferID] );
	} else {
		uint64 id = pointer.fetch_add( 1, std::memory_order_relaxed );
		id       %= MAX_TASKS;
		while ( tasks[id] != TASK_NONE );

		tasks[id] = bufferID;
	}

	TLM.addQueueWaitTimer.Stop();
}

void TaskList::AddToThreadQueueExt( Task& task ) {
	while ( !SM.taskTimesLock.Lock() );
	TaskTime taskTime;

	if ( SM.taskTimes.contains( task.Execute ) ) {
		GlobalTaskTime& SMTaskTime = SM.taskTimes[task.Execute];
		taskTime.count             = SMTaskTime.count.load( std::memory_order_relaxed );
		taskTime.time              = SMTaskTime.time.load( std::memory_order_relaxed );
	} else {
		TLM.unknownTaskCount++;
	}

	SM.taskTimesLock.Unlock();

	if ( task.threadMask ) {
		uint32 threadMask = task.threadMask;

		while ( threadMask ) {
			const uint32 threadID = FindLSB( threadMask );
			threadQueues[threadID].AddTask( threadID, task.bufferID );

			UnSetBit( &threadMask, threadID );
		}

		return;
	}

	const uint32 projectedTime = taskTime.time / std::max( taskTime.count, 1ull ) / 1000;

	if ( projectedTime < TLM.addToQueueTimer.Time() / TLM.addToQueueCount && !TLM.main ) {
		threadQueues[TLM.id].AddTask( TLM.id, task.bufferID );
		return;
	}

	uint32 node = currentThreadExecutionNode.load( std::memory_order_relaxed );
	if ( node != UINT32_MAX ) {
		uint32 expected = node;
		currentThreadExecutionNode.compare_exchange_strong( expected, UINT32_MAX, std::memory_order_relaxed );

		TLM.idleThreads[node]++;
		threadQueues[node].AddTask( node, task.bufferID );
		return;
	}

	/* Nodes correspond to TLM.currentMaxThreads active ThreadQueues
	Whenever a task is added it either goes to the currentThreadExecutionNode or ~thread that is most free of work
	currentThreadExecutionNode contains either an idle thread that has most recently finished execution,
	unless no tasks have been executed yet,
	or a task was already added to the latest idle thread
	threadExecutionNodes is kept sorted in a non-decreasing order when tasks are added */
	for ( node = 0; node < TLM.currentMaxThreads; node++ ) {
		uint32 baseThreadTime = threadExecutionNodes[node].fetch_add( projectedTime, std::memory_order_relaxed );
		uint32 nextNodeTime   = node == TLM.currentMaxThreads - 1 ?
		                                UINT32_MAX
		                              : threadExecutionNodes[node + 1].load( std::memory_order_relaxed );

		if ( node == TLM.currentMaxThreads - 1
			|| baseThreadTime + projectedTime <= nextNodeTime ) {
			threadQueues[node].AddTask( node, task.bufferID );
			return;
		}

		// We overflowed the current node, so move to the next one
		if ( baseThreadTime + projectedTime > nextNodeTime ) {
			threadExecutionNodes[node].fetch_sub( projectedTime, std::memory_order_relaxed );
			continue;
		}

		/* Current node is overflowed but we don't know if we overflowed it or if another thread did
		Another thread is guaranteed to have overflowed it so we wait until it moves to the next node*/
		do {
			baseThreadTime = threadExecutionNodes[node    ].load( std::memory_order_relaxed );
			nextNodeTime   = threadExecutionNodes[node + 1].load( std::memory_order_relaxed );
		} while ( baseThreadTime - projectedTime > nextNodeTime );

		if ( baseThreadTime <= nextNodeTime ) {
			threadQueues[node].AddTask( node, task.bufferID );
			return;
		}

		// We still overflowed
		threadExecutionNodes[node].fetch_sub( projectedTime, std::memory_order_relaxed );
	}
}

void TaskList::AddToThreadQueue( Task& task ) {
	TLM.addToQueueTimer.Start();

	AddToThreadQueueExt( task );
	TLM.addToQueueCount++;

	TLM.addToQueueTimer.Stop();
}

Task* TaskList::GetTaskMemory( Task& task ) {
	if ( AddedToTaskMemory( task.bufferID ) ) {
		return &tasks[task.bufferID];
	}

	Task* taskMemory     = tasks.GetNextElementMemory();
	taskMemory->data     = task.data;
	taskMemory->dataSize = task.dataSize;

	taskMemory->active   = true;
	task.active          = true;
	task.gen             = taskMemory->gen + 1;

	task.bufferID        = taskMemory - tasks.memory;

	*taskMemory          = task;

	return taskMemory;
}

template<IsTask T>
void TaskList::AddTask( Task& task, TaskInitList<T>&& dependencies ) {
	if ( exiting.load( std::memory_order_relaxed ) && !task.shutdownTask ) {
		return;
	}

	TLM.addTimer.Start();

	Task* taskMemory = GetTaskMemory( task );

	SetBit( &task.id, TASK_SHIFT_ADDED );

	taskMemory->id   = task.id;

	if ( HasUntrackedDeps( task.id ) ) {
		ResolveDependencies( *taskMemory, dependencies );

		const uint32 counter = taskMemory->dependencyCounter.fetch_sub( 1, std::memory_order_relaxed ) - 1;

		if ( !counter ) {
			AddToThreadQueue( *taskMemory );
		} else {
			taskWithDependenciesCount.fetch_add( 1, std::memory_order_relaxed );
		}
	} else if ( dependencies.start == dependencies.end ) {
		AddToThreadQueue( *taskMemory );
	}

	taskCount.fetch_add( 1, std::memory_order_relaxed );

	TLM.addTimer.Stop();
}

template<IsTask T>
void TaskList::MarkDependencies( Task& task, TaskInitList<T>&& dependencies ) {
	Task* mainTask           = GetTaskMemory( task );
	uint32 dependencyCounter = 0;

	for ( const T* dep = dependencies.start; dep < dependencies.end; dep++ ) {
		if ( !AddedToTaskList( ( *dep )->id ) ) {
			Task* taskMemory = GetTaskMemory( ( *dep ).GetTask() );

			taskMemory->forwardTasks[taskMemory->forwardTaskCounterFast] = mainTask->bufferID;
			taskMemory->forwardTaskCounterFast++;

			SetBit( &( *dep )->id, TASK_SHIFT_TRACKED_DEPENDENCY );

			dependencyCounter++;
		}
	}

	if ( dependencyCounter != ( dependencies.end - dependencies.start ) ) {
		SetBit( &task.id, TASK_SHIFT_HAS_UNTRACKED_DEPS );

		mainTask->dependencyCounter += dependencyCounter;
	} else {
		mainTask->dependencyCounter += dependencyCounter - 1;
	}
}

template<IsTask T>
void TaskList::UnMarkDependencies( TaskInitList<T>&& dependencies ) {
	for ( const T* dep = dependencies.start; dep < dependencies.end; dep++ ) {
		UnSetBit( &( *dep )->id, TASK_SHIFT_TRACKED_DEPENDENCY );
		UnSetBit( &( *dep )->id, TASK_SHIFT_UPDATED_DEPENDENCY );
	}
}

void TaskList::AddTask( Task& task, std::initializer_list<TaskProxy> dependencies ) {
	MarkDependencies( task, TaskInitList { dependencies.begin(), dependencies.end() } );

	uint64 time = TimeNs();
	if ( time < task.time && time - task.time > eventQueue.minGranularity ) {
		eventQueue.AddTask( task );
	} else {
		AddTask( task, TaskInitList { dependencies.begin(), dependencies.end() } );
	}

	UnMarkDependencies( TaskInitList{ dependencies.begin(), dependencies.end() } );
}

void TaskList::AddTasksExt( std::initializer_list<TaskInit> dependencies ) {
	// TODO: Currently this is an O( 4 * n ) loop. The tasks form a DAG, which we can instead flatten in O( n ), then loop in O( n )
	for ( const TaskInit& taskInit : dependencies ) {
		MarkDependencies( taskInit.begin()[0].task, TaskInitList { &taskInit.begin()[1], taskInit.end() } );
	}

	/* Tracked dependencies are those that we allocated in the AtomicRingBuffer during this function call.
	This allows us to skip a bunch of atomics, but we have to update the forwardTaskCounters
	*before* we add any of the tasks to the task ring.
	Otherwise we could end up updating it after other threads have already finished all of the dependencies.
	Flattening the DAG would get rid of the need to do this because we'd just add tasks starting from the end of the graph */
	for ( const TaskInit& taskInit : dependencies ) {
		for ( const TaskProxy* task = &taskInit.begin()[1]; task < taskInit.end(); task++ ) {
			if ( IsTrackedDependency( task->task.id ) && !IsUpdatedDependency( task->task.id ) ) {
				Task* taskMemory               = GetTaskMemory( task->task );
				taskMemory->forwardTaskCounter = taskMemory->forwardTaskCounterFast;

				SetBit( &task->task.id, TASK_SHIFT_UPDATED_DEPENDENCY );
			}
		}
	}

	for ( const TaskInit& taskInit : dependencies ) {
		for ( const TaskProxy* task = &taskInit.begin()[1]; task < taskInit.end(); task++ ) {
			if ( !AddedToTaskList( task->task.id ) ) {
				AddTask( task->task );
			}
		}

		AddTask( taskInit.begin()[0].task, TaskInitList{ &taskInit.begin()[1], taskInit.end() } );
	}

	for ( const TaskInit& taskInit : dependencies ) {
		UnMarkDependencies( TaskInitList{ &taskInit.begin()[1], taskInit.end() } );
	}
}

Task* TaskList::FetchTask( Thread* thread, const bool longestTask ) {
	Log::DebugTag( "Thread %u fetching", thread->id );

	Q_UNUSED( longestTask );

	ThreadQueue& threadQueue   = threadQueues[TLM.id];
	uint8  current             = threadQueue.current;
	uint16 id                  = threadQueue.tasks[current];
	if ( id == ThreadQueue::TASK_NONE ) {
		currentThreadExecutionNode.store( TLM.id, std::memory_order_relaxed );
		return nullptr;
	}

	threadQueue.tasks[current] = ThreadQueue::TASK_NONE;
	threadQueue.current        = ( current + 1 ) % ThreadQueue::MAX_TASKS;

	executingThreads.fetch_add( 1, std::memory_order_relaxed );
	taskCount.fetch_sub( 1, std::memory_order_relaxed );

	return &tasks[id];
}

bool TaskList::ThreadFinished( const bool hadTask ) {
	const uint32 threadCount = executingThreads.fetch_sub( hadTask, std::memory_order_relaxed ) - hadTask;
	const bool   exit        = exiting.load( std::memory_order_relaxed );

	if ( exit ) {
		TLM.exitTimer.Start();
	}

	if ( exit && !threadCount ) {
		if ( !taskCount.load( std::memory_order_acquire ) ) {
			exitFence.Signal();

			return true;
		}
	}

	return false;
}