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

#include <thread>
#include <iostream>
#include <unordered_map>

#include "Thread.h"
#include "ThreadMemory.h"
#include "TaskList.h"

Thread::Thread() {
}

Thread::~Thread() {
}

void Thread::Start( const uint32_t newID ) {
	id = newID;
	TLM.id = id;

	runTime = 0;
	osThread = std::thread( &Thread::Run, this );

	Log::DebugTag( "id: %u", id );
}

void Thread::Run() {
	Timer runTimeTimer;
	static const uint64_t RUNTIME_GATHER_PERIOD = 1000000000;

	total.Clear();
	fetching.Clear();
	idle.Clear();
	execing.Clear();

	TLM.Init();

	total.Start();

	struct TaskTime {
		uint64_t count = 0;
		uint64_t time = 0;
	};

	std::unordered_map<Task::TaskFunction, TaskTime> taskTimes;

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

		fetching.Start();
		task = taskList.FetchTask( this, true );
		fetching.Stop();

		if ( !task ) {
			idle.Start();
			std::this_thread::yield();
			idle.Stop();
			Log::DebugTag( "id: %u, yielding", id );

			exiting = taskList.ThreadFinished( false );
			continue;
		}

		Log::DebugTag( "id: %u, execing", id );

		Timer t;
		execing.Start();
		task->Execute( task->data );
		task->active = false;
		task->complete.Signal();

		task->forwardTaskLock.Finish();
		const uint32_t forwardTasks = task->forwardTaskCounter.load( std::memory_order_relaxed );
		for ( uint32_t i = 0; i < forwardTasks; i++ ) {
			taskList.FinishDependency( task->forwardTasks[i] );
		}

		execing.Stop();
		t.Stop();

		taskTimes[task->Execute].count++;
		taskTimes[task->Execute].time += t.Time();

		task = nullptr;

		TLM.FreeAllChunks();

		exiting = taskList.ThreadFinished( true );
	}

	total.Stop();
}

void Thread::Exit() {
	running = false;
	exiting = true;

	osThread.join();

	Log::NoticeTag( "id: %u", id );

	Log::NoticeTag( "id: %u: total: %s, fetching: %s, execing: %s, idle: %s\n", id, Timer::FormatTime( total.Time() ),
		Timer::FormatTime( fetching.Time() ), Timer::FormatTime( execing.Time() ),
		Timer::FormatTime( idle.Time() ) );
}
