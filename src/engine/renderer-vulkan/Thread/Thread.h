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
// Thread.h

#ifndef THREAD_H
#define THREAD_H

#include <thread>

#include "../Math/NumberTypes.h"

#include "Task.h"

#include "../Shared/Timer.h"
#include "../SrcDebug/Tag.h"

struct TaskTime;

class Thread :
	public Tag {
	public:
	Thread();
	~Thread();

	void Start( const uint32 newID );
	void Run();
	void Exit();

	private:
	friend class TaskList;

	std::thread osThread;

	uint32      id;
	uint64      runTime;

	Task*       task;

	bool        running = true;
	bool        exiting = false;

	GlobalTimer total;
	GlobalTimer actual;
	GlobalTimer fetchIdleTimer;
	uint64      fetchTask = 0;
	uint64      fetchIdle = 0;
	GlobalTimer idle;
	GlobalTimer executing;
	GlobalTimer dependencyTimer;

	uint64      fetchQueueLock;
	uint64      fetchOuter;

	uint64      addQueueWait;

	uint64      taskAdd;
	uint64      taskSync;

	uint64      taskFetchNone = 0;
	uint64      taskFetchActual = 0;

	uint64      exitTime;

	std::unordered_map<Task::TaskFunction, TaskTime> taskTimes;
};

#endif // THREAD_H