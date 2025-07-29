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

uint8_t TaskList::LockQueue( Task* task ) {
	Q_UNUSED( task );

	uint64_t expected = taskRing.queueLocks.load();
	uint64_t desired;
	uint8_t queue;
	do {
		// TODO: Use projected task time or a uniform distribution to select a queue
		queue = rand() % 63;
		desired = SetBit( expected, queue );
	} while ( !taskRing.queueLocks.compare_exchange_weak( expected, desired, std::memory_order_relaxed ) );

	return queue;
}

void TaskList::UnlockQueue( const uint8_t queue ) {
	taskRing.queueLocks -= 1ull << queue;
}

void TaskList::AddTask( Task task ) {
	if ( exiting && !task.shutdownTask ) {
		return;
	}

	Task* taskMemory = tasks.GetNextElementMemory();
	task.active = true;
	*taskMemory = task;


	uint8_t queue;
	while ( true ) {
		queue = LockQueue( &task );
		if ( taskRing.queues[queue].availableTasks == UINT64_MAX ) {
			UnlockQueue( queue );
		} else {
			break;
		}

		std::this_thread::yield();
	}

	uint32_t taskSlot = FindLZeroBit( taskRing.queues[queue].availableTasks );

	SetBit( &taskRing.queues[queue].availableTasks, taskSlot );
	taskRing.queues[queue].tasks[taskSlot] = taskMemory - tasks.memory;

	UnlockQueue( queue );
}

Task* TaskList::FetchTask( Thread* thread, const bool longestTask ) {
	Log::DebugTag( "Thread %u fetching", thread->id );

	thread->exiting = exiting;

	uint8_t queue;
	uint32_t task;

	uint64_t mask = 0;
	while ( true ) {

		uint64_t expected = taskRing.queueLocks.load();
		uint64_t desired;

		while ( true ) {
			if ( expected == UINT64_MAX ) {
				std::this_thread::yield();
				expected = taskRing.queueLocks.load();
				continue;
			}

			uint64_t queueLocks = expected | mask;
			queue = longestTask ? FindMZeroBit( queueLocks ) : FindLZeroBit( queueLocks );

			if ( queue == 64 ) {
				return nullptr;
			}

			desired = SetBit( expected, queue );

			if ( !taskRing.queueLocks.compare_exchange_strong( expected, desired ) ) {
				continue;
			}

			break;
		}

		task = FindLSB( taskRing.queues[queue].availableTasks );

		if ( task == 64 ) {
			taskRing.queueLocks -= 1ull << queue;
			SetBit( &mask, queue );
			continue;
		}

		break;
	}

	UnSetBit( &taskRing.queues[queue].availableTasks, task );

	uint16_t globalTaskID = taskRing.queues[queue].tasks[task];

	taskRing.queues[queue].tasks[task] = 0;

	UnlockQueue( queue );

	return tasks.memory + globalTaskID;
}