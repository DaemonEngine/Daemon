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
// Thread.cpp

#include <iostream>
#include <unordered_map>

#include "ThreadMemory.h"
#include "GlobalMemory.h"
#include "TaskList.h"

#include "Thread.h"

Thread::Thread() {
}

Thread::~Thread() {
}

void Thread::Start( const uint32 newID ) {
	id = newID;

	runTime = 0;
	osThread = std::thread( &Thread::Run, this );

	Log::DebugTag( "id: %u", id );
}

void Thread::Run() {
	Timer runTimeTimer;
	static const uint64_t RUNTIME_GATHER_PERIOD = 1000000000;

	total.Clear();
	idle.Clear();
	executing.Clear();

	TLM.Init();
	TLM.id = id;

	total.Start();

	bool fetched = false;

	while ( !exiting ) {
		if ( !running ) {
			std::this_thread::yield();
			continue;
		}

		if ( runTimeTimer.Time() >= RUNTIME_GATHER_PERIOD ) {
			runTimeTimer.Restart();
		}

		runTime = runTimeTimer.Time();

		ASSERT_EQ( task, nullptr );

		Timer fetching;
		task = taskList.FetchTask( this, true );
		fetching.Stop();

		if ( task ) {
			fetchTask += fetching.Time();
			fetched = true;
			taskFetchActual++;
		} else {
			fetchIdle += fetching.Time();
			taskFetchNone++;
		}

		if ( fetched ) {
			actual.Start();
			fetchIdleTimer.Start();
		}

		if ( !task ) {
			idle.Start();
			std::this_thread::yield();
			idle.Stop();
			Log::DebugTag( "id: %u, yielding", id );

			exiting = taskList.ThreadFinished( false );
			continue;
		}

		fetchIdleTimer.Stop();

		Log::DebugTag( "id: %u, executing", id );

		Timer t;
		executing.Start();
		task->Execute( task->data );
		task->active = false;
		task->complete.Signal();
		taskList.FinishTask( task );
		executing.Stop();

		dependencyTimer.Start();
		task->forwardTaskLock.Finish();
		const uint32 forwardTasks = task->forwardTaskCounter.load( std::memory_order_relaxed );
		for ( uint32 i = 0; i < forwardTasks; i++ ) {
			taskList.FinishDependency( task->forwardTasks[i] );
		}
		dependencyTimer.Stop();

		t.Stop();

		TaskTime& taskTime = TLM.taskTimes[task->Execute];
		taskTime.count++;
		taskTime.time += t.Time();

		if ( !taskTime.syncedWithSM ) {
			SM.taskTimesLock.Finish();

			GlobalTaskTime& SMTaskTime = SM.taskTimes[task->Execute];
			SMTaskTime.count = taskTime.count;
			SMTaskTime.time = taskTime.time;
			taskTime.syncedWithSM = true;

			SM.taskTimesLock.Reset();
		} else {
			while( !SM.taskTimesLock.Lock() );

			GlobalTaskTime& SMTaskTime = SM.taskTimes[task->Execute];
			SMTaskTime.count.fetch_add( taskTime.count, std::memory_order_relaxed );
			SMTaskTime.time.fetch_add( taskTime.time, std::memory_order_relaxed );

			SM.taskTimesLock.Unlock();
		}

		task = nullptr;

		TLM.FreeAllChunks();

		exiting = taskList.ThreadFinished( true );
	}

	fetchQueueLock = TLM.fetchQueueLockTimer.Time();
	fetchOuter = TLM.fetchOuterTimer.Time();

	addQueueWait = TLM.addQueueWaitTimer.Time();

	taskAdd = TLM.addTimer.Time();
	taskSync = TLM.syncTimer.Time();

	taskTimes = TLM.taskTimes;

	exitTime = TLM.exitTimer.Time();

	actual.Stop();

	total.Stop();
}

void Thread::Exit() {
	exiting = true;

	osThread.join();

	Log::NoticeTag( "\nid: %u", id );

	Log::NoticeTag( "id: %u: total: %s, actual: %s, exit: %s, fetching (task/idle): %s/%s, executing: %s, dependency: %s, idle: %s",
		id, total.FormatTime( Timer::ms ), actual.FormatTime( Timer::ms ), Timer::FormatTime( exitTime, Timer::ms ),
		Timer::FormatTime( fetchTask, Timer::ms ), Timer::FormatTime( fetchIdle, Timer::ms ),
		executing.FormatTime( Timer::ms ), dependencyTimer.FormatTime( Timer::ms ),
		idle.FormatTime( Timer::ms ) );

	Log::NoticeTag( "id: %u, fetchIdleTimer: %s, taskFetch (none/actual): %u/%u",
		id, fetchIdleTimer.FormatTime( Timer::ms ),
		taskFetchNone, taskFetchActual );

	Log::NoticeTag( "id: %u: fetch: queueLock: %s, outer: %s, add: %s, addQueueWait: %s, sync: %s", id,
		Timer::FormatTime( fetchQueueLock, Timer::ms ), Timer::FormatTime( fetchOuter, Timer::ms ),
		Timer::FormatTime( addQueueWait, Timer::ms ),
		Timer::FormatTime( taskAdd, Timer::ms ), Timer::FormatTime( taskSync, Timer::ms ) );

	for ( const std::pair<Task::TaskFunction, TaskTime>& taskTime : taskTimes ) {
		Log::NoticeTag( "task: avg: %s, count: %u, time: %u",
			Timer::FormatTime( taskTime.second.time / std::max( 1ull, taskTime.second.count ), Timer::us ),
			taskTime.second.count, Timer::FormatTime( taskTime.second.time, Timer::us ) );
	}
}
