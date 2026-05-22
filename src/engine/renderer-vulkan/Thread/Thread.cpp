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

#include <unordered_map>

#include "ThreadMemory.h"
#include "GlobalMemory.h"
#include "TaskList.h"
#include "EventQueue.h"

#include "Thread.h"

Thread::Thread() {
}

Thread::~Thread() {
}

void Thread::Start( const uint32 newID ) {
	id         = newID;

	runTime    = 0;
	baseThread = std::thread( &Thread::Run, this );

	Log::DebugTag( "id: %u", id );
}

void Thread::Run() {
	total.Clear();
	idle.Clear();
	executing.Clear();

	TLM.id = id;

	osThread.Init();
	osThread.SetAffinity( id );
	maxCoreFrequencyScale = osThread.GetMaxFrequencyScale();

	total.Start();

	bool fetched = false;

	while ( !exiting ) {
		if ( !running ) {
			std::this_thread::yield();
			continue;
		}

		ASSERT_EQ( task, nullptr );

		eventQueue.Rotate();

		task = TLM.FetchTask();

		Timer fetching;
		if ( !task ) {
			task = taskList.FetchTask();
			fetching.Stop();
		}

		if ( task ) {
			fetchTask += fetching.Time();
			fetched    = true;
			taskFetchActual++;
		} else {
			fetchIdle += fetching.Time();
			taskFetchNone++;
		}

		if ( fetched && TLM.initialised ) {
			actual.Start();
			fetchIdleTimer.Start();
		}

		if ( !task ) {
			idle.Start();
			std::this_thread::yield();
			idle.Stop();

			exiting = taskList.ThreadFinished( false );
			continue;
		}

		fetchIdleTimer.Stop();

		taskList.TaskStarted();

		Timer t;
		executing.Start();
		task->Execute( task->data );
		executing.Stop();

		dependencyTimer.Start();

		if ( !task->threadMask || !UnSetBit( task->threadMask, TLM.id )
			|| task->threadCount.fetch_sub( 1, std::memory_order_relaxed ) == 1 ) {
			task->complete.Signal();
			task->active = false;
			taskList.FinishTask( task );
			task->forwardTaskLock.LockWrite();

			const uint32 forwardTasks = task->forwardTaskCounter.load( std::memory_order_relaxed );

			for ( uint32 i = 0; i < forwardTasks; i++ ) {
				taskList.FinishDependency( task->forwardTasks[i] );
			}
		}

		dependencyTimer.Stop();

		const uint64 taskExecTime = t.Time() * maxCoreFrequencyScale;

		TaskTime& taskTime = TLM.taskTimes[task->Execute];
		taskTime.count++;
		taskTime.time += taskExecTime;

		if ( !taskTime.syncedWithSM ) {
			while( !SM.taskTimesLock.LockWrite() );

			GlobalTaskTime& SMTaskTime = SM.taskTimes[task->Execute];
			SMTaskTime.count           = 1;
			SMTaskTime.time            = taskExecTime;
			taskTime.syncedWithSM      = true;

			SM.taskTimesLock.UnlockWrite();
		} else {
			while( !SM.taskTimesLock.Lock() );

			GlobalTaskTime& SMTaskTime = SM.taskTimes[task->Execute];
			SMTaskTime.count.fetch_add( 1, std::memory_order_relaxed );
			SMTaskTime.time.fetch_add( taskExecTime, std::memory_order_relaxed );

			SM.taskTimesLock.Unlock();
		}

		task = nullptr;

		TLM.FreeAllChunks();

		exiting = taskList.ThreadFinished( true );
	}

	fetchQueueLock = TLM.fetchQueueLockTimer.Time();
	fetchOuter     = TLM.fetchOuterTimer.Time();

	addQueueWait   = TLM.addQueueWaitTimer.Time();

	taskAdd        = TLM.addTimer.Time();
	taskSync       = TLM.syncTimer.Time();

	taskTimes      = TLM.taskTimes;

	exitTime       = TLM.exitTimer.Time();

	actual.Stop();

	total.Stop();
}

void Thread::Exit() {
	exiting = true;

	baseThread.join();

	Log::NoticeTag( "\nid: %u", id );

	Log::NoticeTag( "id: %u: total: %s, actual: %s, exit: %s,"
		" fetching (task/idle): %s/%s, executing: %s, dependency: %s, idle: %s",
		id, total.FormatTime( ms ), actual.FormatTime( ms ), FormatTime( exitTime, ms ),
		FormatTime( fetchTask, ms ), FormatTime( fetchIdle, ms ),
		FormatTime( executing.Time(), ms ), dependencyTimer.FormatTime( ms ),
		idle.FormatTime( ms ) );

	Log::NoticeTag( "id: %u, fetchIdleTimer: %s, taskFetch (none/actual): %u/%u",
		id, fetchIdleTimer.FormatTime( ms ),
		taskFetchNone, taskFetchActual );

	Log::NoticeTag( "id: %u: fetch: queueLock: %s, outer: %s, add: %s, addQueueWait: %s, sync: %s", id,
		FormatTime( fetchQueueLock, ms ), FormatTime( fetchOuter, ms ),
		FormatTime( addQueueWait, ms ),
		FormatTime( taskAdd, ms ), FormatTime( taskSync, ms ) );

	for ( const std::pair<Task::TaskFunction, TaskTime>& taskTime : taskTimes ) {
		Log::NoticeTag( "task: avg: %s, count: %u, time: %u",
			FormatTime( taskTime.second.time / maxCoreFrequencyScale / std::max( 1ull, taskTime.second.count ), us ),
			taskTime.second.count, FormatTime( taskTime.second.time / maxCoreFrequencyScale, us ) );
	}
}