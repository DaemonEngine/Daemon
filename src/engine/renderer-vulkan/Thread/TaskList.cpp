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
	}
}

void TaskList::Init() {
	tasks.Alloc( 2048 );

	int threads = r_vkThreadCount.Get();
	threads = threads ? threads : CPU_CORES;
	AdjustThreadCount( threads );
}

void TaskList::Shutdown() {
	exiting = true;
}

void TaskList::FinishShutdown() {
	for ( Thread* thread = threads; thread < threads + currentMaxThreads; thread++ ) {
		thread->Exit();
	}
}

uint8_t TaskRing::LockQueueForTask( Task* task ) {
	Q_UNUSED( task );

	uint64_t expected = queueLocks.load();
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
	uint64_t expected = queueLocks.load();
	uint64_t desired;
	do {
		desired = SetBit( expected, queue );
	} while ( !queueLocks.compare_exchange_weak( expected, desired, std::memory_order_relaxed ) );
}

void TaskRing::UnlockQueue( const uint8_t queue ) {
	queueLocks -= 1ull << queue;
}

void TaskRing::RemoveTask( const uint8_t queue, const uint8_t id ) {
	LockQueue( queue );

	UnSetBit( &queues[queue].availableTasks, id );
	queues[queue].tasks[id] = 0;

	UnlockQueue( queue );
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

uint16_t TaskList::AddToTaskRing( TaskRing& taskRing, Task& task ) {
	uint8_t queue;
	while ( true ) {
		queue = taskRing.LockQueueForTask( &task );
		if ( taskRing.queues[queue].availableTasks == UINT64_MAX ) {
			taskRing.UnlockQueue( queue );
		} else {
			break;
		}

		std::this_thread::yield();
	}

	uint32_t taskSlot = FindLZeroBit( taskRing.queues[queue].availableTasks );

	SetBit( &taskRing.queues[queue].availableTasks, taskSlot );
	taskRing.queues[queue].tasks[taskSlot] = task.bufferID;

	// taskRing.UnlockQueue( queue );

	return taskRing.id | ( queue << TASK_SHIFT_QUEUE ) | ( taskSlot << TASK_SHIFT_ID ) | ( 1 << TASK_SHIFT_ALLOCATED );
}

void TaskList::MoveToTaskRing( TaskRing& taskRing, Task& task ) {
	IDToTaskRing( task.id ).RemoveTask( IDToTaskQueue( task.id ), IDToTaskID( task.id ) );

	AddToTaskRing( taskRing, task );

	taskRing.UnlockQueue( IDToTaskQueue( task.id ) );
}

void TaskList::FinishDependency( const uint16_t bufferID ) {
	Task& task = tasks.memory[bufferID];

	const uint32_t counter = task.dependencyCounter--;

	if ( !counter ) {
		MoveToTaskRing( mainTaskRing, task );
	}
}

bool TaskList::ResolveDependencies( Task& task, const TaskProxy* start, const TaskProxy* end ) {
	uint32_t counter = 0;
	for ( const TaskProxy* dep = start; dep < end; dep++ ) {
		if ( !BitSet( dep->task.id, TASK_SHIFT_ALLOCATED ) ) {
			Sys::Drop( "Tried to add task with an unallocated dependency" );
		}

		Task& dependency = tasks[dep->task.bufferID];

		const bool locked = dependency.forwardTaskLock.Lock();

		if ( !locked ) {
			continue;
		}

		uint32_t id = dependency.forwardTaskCounter.fetch_add( 1 );

		ASSERT_LE( id, Task::MAX_FORWARD_TASKS );

		dependency.forwardTasks[id] = task.bufferID;

		dependency.forwardTaskLock.Unlock();

		counter++;
	}

	task.dependencyCounter += counter;

	return counter;
}

bool TaskList::ResolveDependencies( Task& task, std::initializer_list<Task>& dependencies ) {
	uint32_t counter = 0;
	for ( const Task& dep : dependencies ) {
		if ( !BitSet( dep.id, TASK_SHIFT_ALLOCATED ) ) {
			Sys::Drop( "Tried to add task with an unallocated dependency" );
		}

		Task& dependency = tasks[dep.bufferID];

		const bool locked = dependency.forwardTaskLock.Lock();

		if ( !locked ) {
			continue;
		}

		uint32_t id = dependency.forwardTaskCounter.fetch_add( 1 );

		ASSERT_LE( id, Task::MAX_FORWARD_TASKS );

		dependency.forwardTasks[id] = task.bufferID;

		dependency.forwardTaskLock.Unlock();

		counter++;
	}

	task.dependencyCounter += counter;

	return counter;
}

void TaskList::AddTask( Task& task, const TaskProxy* start, const TaskProxy* end ) {
	if ( exiting && !task.shutdownTask ) {
		return;
	}

	Task* taskMemory = tasks.GetNextElementMemory();
	taskMemory->active = true;
	task.active = true;

	task.bufferID = taskMemory - tasks.memory;

	TaskRing* taskRing;
	if ( ResolveDependencies( task, start, end ) ) {
		task.id = AddToTaskRing( forwardTaskRing, task );
		taskRing = &forwardTaskRing;
	} else {
		task.id = AddToTaskRing( mainTaskRing, task );
		taskRing = &mainTaskRing;
	}

	SetBit( &task.id, TASK_SHIFT_ALLOCATED );

	task.dependencyCounter--;

	*taskMemory = task;

	taskRing->UnlockQueue( IDToTaskQueue( task.id ) );
}

void TaskList::AddTask( Task& task, std::initializer_list<Task> dependencies ) {
	if ( exiting && !task.shutdownTask ) {
		return;
	}

	Task* taskMemory = tasks.GetNextElementMemory();
	taskMemory->active = true;
	task.active = true;

	task.bufferID = taskMemory - tasks.memory;

	TaskRing* taskRing;
	if ( ResolveDependencies( task, dependencies ) ) {
		task.id = AddToTaskRing( forwardTaskRing, task );
		taskRing = &forwardTaskRing;
	} else {
		task.id = AddToTaskRing( mainTaskRing, task );
		taskRing = &mainTaskRing;
	}

	SetBit( &task.id, TASK_SHIFT_ALLOCATED );

	task.dependencyCounter--;

	*taskMemory = task;

	taskRing->UnlockQueue( IDToTaskQueue( task.id ) );
}

void TaskList::AddTasksExt( std::initializer_list<TaskInit> dependencies ) {
	for ( const TaskInit& taskInit : dependencies ) {
		for ( const TaskProxy* task = &taskInit.begin()[1]; task < taskInit.end(); task++ ) {
			if ( !AddedToTaskRing( task->task.id ) ) {
				AddTask( task->task );
			}
		}
		AddTask( taskInit.begin()[0].task, &taskInit.begin()[1], taskInit.end() );
	}
}

Task* TaskList::FetchTask( Thread* thread, const bool longestTask ) {
	Log::DebugTag( "Thread %u fetching", thread->id );

	thread->exiting = exiting;

	uint8_t queue;
	uint32_t task;

	uint64_t mask = 0;
	while ( true ) {

		uint64_t expected = mainTaskRing.queueLocks.load();
		uint64_t desired;

		while ( true ) {
			if ( expected == UINT64_MAX ) {
				std::this_thread::yield();
				expected = mainTaskRing.queueLocks.load();
				continue;
			}

			uint64_t queueLocks = expected | mask;
			queue = longestTask ? FindMZeroBit( queueLocks ) : FindLZeroBit( queueLocks );

			if ( queue == 64 ) {
				return nullptr;
			}

			desired = SetBit( expected, queue );

			if ( !mainTaskRing.queueLocks.compare_exchange_strong( expected, desired ) ) {
				continue;
			}

			break;
		}

		task = FindLSB( mainTaskRing.queues[queue].availableTasks );

		if ( task == 64 ) {
			mainTaskRing.queueLocks -= 1ull << queue;
			SetBit( &mask, queue );
			continue;
		}

		break;
	}

	UnSetBit( &mainTaskRing.queues[queue].availableTasks, task );

	uint16_t globalTaskID = mainTaskRing.queues[queue].tasks[task];

	mainTaskRing.queues[queue].tasks[task] = 0;

	mainTaskRing.UnlockQueue( queue );

	return tasks.memory + globalTaskID;
}
