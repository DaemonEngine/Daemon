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

TaskList taskList;

TaskList::TaskList() {
	tasks.Alloc( 2048 );
	AdjustThreadCount( std::thread::hardware_concurrency() );
}

TaskList::~TaskList() {
	Shutdown();
}

void TaskList::AdjustThreadCount( uint32_t newMaxThreads ) {
	if ( newMaxThreads >= MAX_THREADS ) {
		newMaxThreads = MAX_THREADS - 1;
	}

	if ( newMaxThreads > currentMaxThreads ) {
		for ( uint32_t i = currentMaxThreads; i < newMaxThreads; i++ ) {
			threads[i].Start( i );
		}

		currentMaxThreads = newMaxThreads;
	}
}

void TaskList::Shutdown() {
	exiting = true;
}

void TaskList::FinishShutdown() {
	for ( Thread* thread = threads; thread < threads + currentMaxThreads; thread++ ) {
		thread->Exit();
	}
}

uint8_t TaskList::LockQueue( Task* task ) {
	Q_UNUSED( task );

	uint64_t expected = taskRing.queueLocks.load();
	uint64_t desired;
	uint8_t queue;
	do {
		queue = rand() % 63;
		desired = SetBit( expected, queue );
	} while ( !taskRing.queueLocks.compare_exchange_weak( expected, desired, std::memory_order_relaxed ) );

	return queue;
}

void TaskList::UnlockQueue( const uint8_t queue ) {
	uint64_t expected = taskRing.queueLocks.load();
	uint64_t desired;
	do {
		desired = UnSetBit( expected, queue );
	} while ( !taskRing.queueLocks.compare_exchange_weak( expected, desired, std::memory_order_relaxed ) );
}

void TaskList::AddTask( Task task ) {
	if ( exiting && !task.shutdownTask ) {
		return;
	}

	Task* taskMemory = tasks.GetNextElementMemory();
	task.active = true;
	*taskMemory = task;

	
	uint8_t queue;
	while( true ) {
		queue = LockQueue( &task );
		if ( taskRing.queues[queue].availableTasks == UINT64_MAX ) {
			UnlockQueue( queue );
		} else {
			break;
		}

		std::this_thread::yield();
	}

	SetBit( &taskRing.queuesWithTasks, queue );

	uint32_t taskSlot = FindZeroBitFast( taskRing.queues[queue].availableTasks );

	SetBit( &taskRing.queues[queue].availableTasks, taskSlot );
	taskRing.queues[queue].tasks[taskSlot] = taskMemory - tasks.memory;

	UnlockQueue( queue );
}

Task* TaskList::FetchTask( Thread* thread ) {
	printf( "thread %u fetching\n", thread->id );

	thread->exiting = exiting;

	uint64_t expected = taskRing.queueLocks.load();
	uint64_t desired;
	uint8_t queue;
	do {
		uint64_t q = taskRing.queuesWithTasks;
		while ( true ) {
			if ( !q ) {
				return nullptr;
			}

			queue = CountLeadingZeroes( q );
			UnSetBit( &q, queue );

			if ( !BitSet( expected, queue ) ) {
				break;
			}
		}
		desired = SetBit( expected, queue );
	} while ( !taskRing.queueLocks.compare_exchange_weak( expected, desired, std::memory_order_relaxed ) );

	uint64_t task = FindLSB( taskRing.queues[queue].availableTasks );
	UnSetBit( &taskRing.queues[queue].availableTasks, task );

	if ( !taskRing.queues[queue].availableTasks ) {
		UnSetBit( &taskRing.queuesWithTasks, queue );
	}

	UnlockQueue( queue );

	return tasks.memory + taskRing.queues[queue].tasks[task];
}

/* TaskList::TaskList() {
	AdjustThreadCount( 4 );
}

TaskList::~TaskList() {
	Shutdown();
}

int TaskList::AddTask( Task task ) {
	if ( exiting && !task.shutdownTask ) {
		return -1;
	}

	int dependency = task.dependency;

	uint32_t phaseID;
	if ( dependency == -1 ) {
		phaseID = phase;
	} else if ( task.shutdownTask ) {
		phaseID = ( phase + 1 ) & ( MAX_PHASES - 1 );
	} else {
		uint32_t unused;
		UnpackTaskID( dependency, &phaseID, &unused );
		
		if ( phaseID == MAX_PHASES - 1 ) {
			phaseID = 0;
		}
	}

	uint32_t taskID = currentTask[phaseID]++;

	if ( taskID >= TaskListShared::MAX_PHASE_TASKS ) {
		return -1;
	}

	tasks[phaseID][taskID] = task;
	task.phase = phaseID;

	uint32_t id = PackTaskID( phaseID, taskID );

	return id;
}

Task* TaskList::FetchTask( Thread* thread ) {
	tasksLock.lock();

	uint32_t totalTasks = currentTask[phase];

	if ( !totalTasks ) {
		phase = ( phase + 1 ) & ( MAX_PHASES - 1 );
		totalTasks = currentTask[phase];
	}

	Task* task = nullptr;
	if ( totalTasks ) {
		const uint32_t taskID = currentTask[phase]--;
		task = &tasks[phase][taskID];
	}

	if ( !totalTasks ) {
		thread->Exit();
	}

	tasksLock.unlock();

	return task;
}

void TaskList::AdjustThreadCount( uint32_t newMaxThreads ) {
	if ( newMaxThreads >= MAX_THREADS ) {
		newMaxThreads = MAX_THREADS - 1;
	}

	if ( newMaxThreads > currentMaxThreads ) {
		for ( uint32_t i = currentMaxThreads; i < newMaxThreads; i++ ) {
			threads[i].Start( i );
		}

		currentMaxThreads = newMaxThreads;
	}
}

void TaskList::Shutdown() {
	exiting = true;
}

uint32_t TaskList::PackTaskID( const uint32_t phase, const uint32_t task ) {
	return ( phase << ID_PHASE_SHIFT ) | task;
}

void TaskList::UnpackTaskID( const uint32_t id, uint32_t* phase, uint32_t* task ) {
	*phase = ( id >> ID_PHASE_SHIFT ) & ID_PHASE_MASK;
	*task = id & ID_TASK_MASK;
} */
