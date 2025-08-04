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
	Shutdown();
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
	executingThreads.fetch_sub( 1, std::memory_order_relaxed );
	exiting.store( true, std::memory_order_relaxed );
}

void TaskList::FinishShutdown() {
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

TaskRing& TaskList::IDToTaskRing( const uint16_t id ) {
	return TaskRingIDToTaskRing( ( TaskRingID ) ( id & TASK_RING_MASK ) );
}

uint8_t TaskList::IDToTaskQueue( const uint16_t id ) {
	return ( id >> TASK_SHIFT_QUEUE ) & TASK_QUEUE_MASK;
}

uint16_t TaskList::IDToTaskID( const uint16_t id ) {
	return ( id >> TASK_SHIFT_ID ) & TASK_ID_MASK;
}

constexpr TaskRing& TaskList::TaskRingIDToTaskRing( const TaskRingID taskRingID ) {
	switch ( taskRingID ) {
		case MAIN:
			return mainTaskRing;
		case FORWARD:
			return forwardTaskRing;
		default:
			ASSERT_UNREACHABLE();
	}
}

/* If unlockQueueAfterAdd is true, the queue this task was added to will bbe locked automatically
before this function returns
Otherwise you must use taskRing.UnlockQueue( IDToTaskQueue( task.id ) ) to unlock the queue after using this function!
This is required to avoid race conditions because some of the code calling AddToTaskRing() further modifies the task */
uint16_t TaskRing::AddToTaskRing( Task& task, const bool unlockQueueAfterAdd ) {
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

	return id | ( queue << TaskList::TASK_SHIFT_QUEUE )
	          | ( taskSlot << TaskList::TASK_SHIFT_ID )
	          | ( 1 << TaskList::TASK_SHIFT_ALLOCATED );
}

void TaskList::MoveToTaskRing( TaskRing& taskRing, Task& task ) {
	TLM.addTimer.Start();

	IDToTaskRing( task.id ).RemoveTask( IDToTaskQueue( task.id ), IDToTaskID( task.id ) );
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
bool TaskList::ResolveDependencies( Task& task, TaskInitList<T>& dependencies ) {
	uint32_t counter = 0;
	for ( const T* dep = dependencies.start; dep < dependencies.end; dep++ ) {
		if ( !BitSet( ( *dep )->id, TASK_SHIFT_ALLOCATED ) ) {
			Sys::Drop( "Tried to add task with an unallocated dependency" );
		}

		Task& dependency = tasks[( *dep )->bufferID];

		const bool locked = dependency.forwardTaskLock.Lock();

		if ( !locked ) {
			continue;
		}

		uint32_t id = dependency.forwardTaskCounter.fetch_add( 1, std::memory_order_relaxed );

		ASSERT_LE( id, Task::MAX_FORWARD_TASKS );

		dependency.forwardTasks[id] = task.bufferID;

		dependency.forwardTaskLock.Unlock();

		counter++;
	}

	task.dependencyCounter.fetch_add( counter, std::memory_order_relaxed );

	return counter;
}

template<IsTask T>
void TaskList::AddTask( Task& task, TaskInitList<T>&& dependencies ) {
	if ( exiting.load( std::memory_order_relaxed ) && !task.shutdownTask ) {
		return;
	}

	TLM.addTimer.Start();

	Task* taskMemory = tasks.GetNextElementMemory();
	taskMemory->active = true;
	task.active = true;

	task.bufferID = taskMemory - tasks.memory;

	*taskMemory = task;

	TaskRing* taskRing;
	if ( ResolveDependencies( *taskMemory, dependencies ) ) {
		task.id = forwardTaskRing.AddToTaskRing( task, false );
		taskRing = &forwardTaskRing;
	} else {
		task.id = mainTaskRing.AddToTaskRing( task, false );
		taskRing = &mainTaskRing;
	}

	SetBit( &task.id, TASK_SHIFT_ALLOCATED );

	taskMemory->id = task.id;

	const uint32_t counter = taskMemory->dependencyCounter.fetch_sub( 1, std::memory_order_relaxed ) - 1;

	if ( !counter && taskRing == &forwardTaskRing ) {
		forwardTaskRing.RemoveTask( IDToTaskQueue( task.id ), IDToTaskID( task.id ), true );
		taskRing->AddToTaskRing( task );
	}

	taskRing->UnlockQueue( IDToTaskQueue( task.id ) );

	TLM.addTimer.Stop();	
}

void TaskList::AddTask( Task& task, std::initializer_list<Task> dependencies ) {
	AddTask( task, TaskInitList{ dependencies.begin(), dependencies.end() } );
}

void TaskList::AddTasksExt( std::initializer_list<TaskInit> dependencies ) {
	for ( const TaskInit& taskInit : dependencies ) {
		for ( const TaskProxy* task = &taskInit.begin()[1]; task < taskInit.end(); task++ ) {
			if ( !AddedToTaskRing( task->task.id ) ) {
				AddTask( task->task );
			}
		}
		AddTask( taskInit.begin()[0].task, TaskInitList{ &taskInit.begin()[1], taskInit.end() } );
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

	if ( !threadCount ) {
		if ( !mainTaskRing.taskCount.load( std::memory_order_acquire ) ) {
			exitFence.Signal();

			return true;
		}
	}

	return false;
}