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

#include "TaskList.h"

#include "../Math/Bit.h"
#include "../Sys/CPUInfo.h"
#include "../MiscCVarStore.h"

#include "ThreadMemory.h"

TaskList taskList;

TaskList::TaskList() {
}

TaskList::~TaskList() {
}

void TaskList::AdjustThreadCount( uint32_t newMaxThreads ) {
	if ( newMaxThreads > MAX_THREADS ) {
		Log::WarnTag( "Maximum thread count exceeded: %u > %u, setting to %u",
			newMaxThreads, MAX_THREADS, MAX_THREADS );
		newMaxThreads = MAX_THREADS;
	}

	if ( newMaxThreads == 0 ) {
		Log::WarnTag( "Thread count can't be 0" );
		return;
	}

	if ( newMaxThreads > currentMaxThreads ) {
		for ( uint32_t i = currentMaxThreads; i < newMaxThreads; i++ ) {
			threads[i].Start( i );
		}

		currentMaxThreads = newMaxThreads;
	} else if ( newMaxThreads < currentMaxThreads ) {
		for ( uint32_t i = newMaxThreads; i < currentMaxThreads; i++ ) {
			threads[i].exiting = true;
		}

		currentMaxThreads = newMaxThreads;
	}
}

void TaskList::Init() {
	tasks.Alloc( 2048 );

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

	for ( Thread* thread = threads; thread < threads + currentMaxThreads; thread++ ) {
		thread->Exit();
	}
}

uint8_t TaskRing::LockQueueForTask( Task* task ) {
	Q_UNUSED( task );

	uint64_t expected = queueLocks.load( std::memory_order_acquire );
	uint64_t desired;
	uint8_t queue;
	do {
		// TODO: Use projected task time or a uniform distribution to select a queue
		queue = rand() % 63;
		desired = SetBit( expected, queue );
	} while ( !queueLocks.compare_exchange_weak( expected, desired, std::memory_order_relaxed ) );

	return queue;
}

void TaskRing::LockQueue( const uint8_t queue ) {
	uint64_t expected = queueLocks.load( std::memory_order_acquire );
	uint64_t desired;
	do {
		desired = SetBit( expected, queue );
	} while ( !queueLocks.compare_exchange_weak( expected, desired, std::memory_order_relaxed ) );
}

void TaskRing::UnlockQueue( const uint8_t queue ) {
	queueLocks -= SetBit( 0ull, queue );
}

void TaskRing::RemoveTask( const uint8_t queue, const uint8_t taskID, const bool queueLocked ) {
	if ( !queueLocked ) {
		LockQueue( queue );
	}

	UnSetBit( &queues[queue].availableTasks, taskID );
	queues[queue].tasks[taskID] = 0;

	taskCount.fetch_sub( 1, std::memory_order_relaxed );

	if ( !queueLocked ) {
		UnlockQueue( queue );
	}
}

bool TaskList::AddedToTaskRing( const uint16_t id ) {
	return BitSet( id, TASK_SHIFT_ALLOCATED );
}

bool TaskList::AddedToTaskMemory( const uint16_t id ) {
	return id != Task::UNALLOCATED;
}

bool TaskList::HasUntrackedDeps( const uint16_t id ) {
	return BitSet( id, TASK_SHIFT_HAS_UNTRACKED_DEPS );
}

bool TaskList::IsTrackedDependency( const uint16_t id ) {
	return BitSet( id, TASK_SHIFT_TRACKED_DEPENDENCY );
}

bool TaskList::IsUpdatedDependency( const uint16_t id ) {
	return BitSet( id, TASK_SHIFT_UPDATED_DEPENDENCY );
}

uint8_t TaskList::IDToTaskQueue( const uint16_t id ) {
	return ( id >> TASK_SHIFT_QUEUE ) & TASK_QUEUE_MASK;
}

uint16_t TaskList::IDToTaskID( const uint16_t id ) {
	return ( id >> TASK_SHIFT_ID ) & TASK_ID_MASK;
}

/* If unlockQueueAfterAdd is true, the queue this task was added to will bbe locked automatically
before this function returns
Otherwise you must use taskRing.UnlockQueue( IDToTaskQueue( task.id ) ) to unlock the queue after using this function!
This is required to avoid race conditions because some of the code calling AddToTaskRing() further modifies the task */
void TaskRing::AddToTaskRing( Task& task, const bool unlockQueueAfterAdd ) {
	uint8_t queue;
	while ( true ) {
		queue = LockQueueForTask( &task );
		
		if ( queues[queue].availableTasks != UINT64_MAX ) {
			break;
		}

		UnlockQueue( queue );

		std::this_thread::yield();
	}

	uint32_t taskSlot = FindLZeroBit( queues[queue].availableTasks );

	SetBit( &queues[queue].availableTasks, taskSlot );
	queues[queue].tasks[taskSlot] = task.bufferID;

	if ( unlockQueueAfterAdd ) {
		UnlockQueue( queue );
	}

	taskCount.fetch_add( 1, std::memory_order_relaxed );
}

void TaskList::MoveToTaskRing( TaskRing& taskRing, Task& task ) {
	TLM.addTimer.Start();

	taskRing.AddToTaskRing( task );

	TLM.addTimer.Stop();
}

void TaskList::FinishDependency( const uint16_t bufferID ) {
	Task& task = tasks[bufferID];

	const uint32_t counter = task.dependencyCounter.fetch_sub( 1, std::memory_order_relaxed ) - 1;

	if ( !counter ) {
		MoveToTaskRing( mainTaskRing, task );
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

		Task& dependency = tasks[( *dep )->bufferID];

		const bool locked = dependency.forwardTaskLock.Lock();

		if ( !locked ) {
			continue;
		}

		uint32_t id = dependency.forwardTaskCounter.fetch_add( 1, std::memory_order_relaxed );

		ASSERT_LE( id, Task::MAX_FORWARD_TASKS );

		dependency.forwardTasks[id] = task.bufferID;

		task.dependencyCounter.fetch_add( 1, std::memory_order_relaxed );

		dependency.forwardTaskLock.Unlock();
	}
}

Task* TaskList::GetTaskMemory( Task& task ) {
	if ( AddedToTaskMemory( task.bufferID ) ) {
		return &tasks[task.bufferID];
	}

	Task* taskMemory = tasks.GetNextElementMemory();

	taskMemory->active = true;
	task.active = true;

	task.bufferID = taskMemory - tasks.memory;

	*taskMemory = task;

	return taskMemory;
}

template<IsTask T>
void TaskList::AddTask( Task& task, TaskInitList<T>&& dependencies ) {
	if ( exiting.load( std::memory_order_relaxed ) && !task.shutdownTask ) {
		return;
	}

	TLM.addTimer.Start();

	Task* taskMemory = GetTaskMemory( task );

	SetBit( &task.id, TASK_SHIFT_ALLOCATED );

	taskMemory->id = task.id;

	if ( HasUntrackedDeps( task.id ) ) {
		ResolveDependencies( *taskMemory, dependencies );

		const uint32_t counter = taskMemory->dependencyCounter.fetch_sub( 1, std::memory_order_relaxed ) - 1;

		if ( !counter ) {
			mainTaskRing.AddToTaskRing( *taskMemory );
		}
	} else if ( !( dependencies.end - dependencies.start ) ) {
		mainTaskRing.AddToTaskRing( *taskMemory );
	}

	TLM.addTimer.Stop();
}

template<IsTask T>
void TaskList::MarkDependencies( Task& task, TaskInitList<T>&& dependencies ) {
	Task* mainTask = GetTaskMemory( task );
	uint32_t dependencyCounter = 0;

	for ( const T* dep = dependencies.start; dep < dependencies.end; dep++ ) {
		if ( !AddedToTaskRing( ( *dep )->id ) ) {
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

	AddTask( task, TaskInitList { dependencies.begin(), dependencies.end() } );

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
				Task* taskMemory = GetTaskMemory( task->task );
				taskMemory->forwardTaskCounter = taskMemory->forwardTaskCounterFast;

				SetBit( &task->task.id, TASK_SHIFT_UPDATED_DEPENDENCY );
			}
		}
	}

	for ( const TaskInit& taskInit : dependencies ) {
		for ( const TaskProxy* task = &taskInit.begin()[1]; task < taskInit.end(); task++ ) {
			if ( !AddedToTaskRing( task->task.id ) ) {
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

	uint8_t queue;
	uint32_t task;

	uint64_t mask = 0;

	TLM.fetchOuterTimer.Start();

	if ( !mainTaskRing.taskCount.load( std::memory_order_relaxed ) ) {
		TLM.fetchOuterTimer.Stop();
		return nullptr;
	}

	while ( true ) {
		uint64_t expected = mainTaskRing.queueLocks.load();
		uint64_t desired;

		TLM.fetchQueueLockTimer.Start();
		while ( true ) {
			if ( expected == UINT64_MAX ) {
				std::this_thread::yield();
				expected = mainTaskRing.queueLocks.load();
				continue;
			}

			uint64_t queueLocks = expected | mask;
			queue = longestTask ? FindMZeroBit( queueLocks ) : FindLZeroBit( queueLocks );

			if ( queue == 64 ) {

				TLM.fetchQueueLockTimer.Stop();
				TLM.fetchOuterTimer.Stop();
				return nullptr;
			}

			desired = SetBit( expected, queue );

			if ( !mainTaskRing.queueLocks.compare_exchange_strong( expected, desired, std::memory_order_relaxed ) ) {
				continue;
			}

			break;
		}

		TLM.fetchQueueLockTimer.Stop();

		task = FindLSB( mainTaskRing.queues[queue].availableTasks );

		if ( task == 64 ) {
			mainTaskRing.UnlockQueue( queue );
			SetBit( &mask, queue );
			continue;
		}

		break;
	}

	UnSetBit( &mainTaskRing.queues[queue].availableTasks, task );

	uint16_t globalTaskID = mainTaskRing.queues[queue].tasks[task];

	mainTaskRing.queues[queue].tasks[task] = 0;

	mainTaskRing.taskCount.fetch_sub( 1, std::memory_order_relaxed );
	executingThreads.fetch_add( 1, std::memory_order_relaxed );

	mainTaskRing.UnlockQueue( queue );

	TLM.fetchOuterTimer.Stop();

	return tasks.memory + globalTaskID;
}

bool TaskList::ThreadFinished( const bool hadTask ) {
	const uint32_t threadCount = executingThreads.fetch_sub( hadTask, std::memory_order_relaxed ) - hadTask;

	if ( exiting.load( std::memory_order_relaxed ) && !threadCount ) {
		if ( !mainTaskRing.taskCount.load( std::memory_order_acquire ) ) {
			exitFence.Signal();

			return true;
		}
	}

	return false;
}