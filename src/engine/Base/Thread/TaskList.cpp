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

#include "Sys/CPUInfo.h"
#include "Bit.h"
#include "BaseCVars.h"
#include "Task.h"
#include "Timer.h"

#include "EventQueue.h"
#include "GlobalMemory.h"
#include "TaskEnv.h"
#include "ThreadMemory.h"
#include "ThreadUplink.h"

#include "TaskList.h"

TaskList taskList;

static void SyncActiveThreads( uint64* activeThreadMask ) {
	TLM.activeThreadMask  = *activeThreadMask;
	TLM.currentMaxThreads = CountBits( *activeThreadMask );
}

TaskList::TaskList() {
}

TaskList::~TaskList() {
}

void TaskList::SetActiveThreads( uint64 threadMask ) {
	threadMask &= BitMask64( 0, coreCount );

	if ( !threadMask ) {
		Log::WarnTag( "Thread count can't be 0" );

		return;
	}

	while ( !threadCountLock.LockWrite() );

	// This must always be set before using ThreadMask*() on the main thread
	SyncActiveThreads( &threadMask );

	Task t { &SyncActiveThreads, threadMask };
	AddTasks( { t.ThreadMaskAll() } );
	t.Wait();

	Log::NoticeTag( "Changed thread count to %u", TLM.currentMaxThreads );

	threadCountLock.UnlockWrite();
}

void TaskList::ThreadInitialised() {
	threadInitCount.Signal();
}

void TaskList::Init() {
	tasks.Alloc( maxThreadTasks );
	tasksData.Alloc( maxThreadTaskData );

	CPU_CORES = CPU_CORES > 64 ? 64 : CPU_CORES;
	coreCount = CPU_CORES - 1;
	coreMask  = BitMask64( 0, coreCount );

	for ( uint32 i = 0; i < CPU_CORES; i++ ) {
		threads[i].Start( i );
	}

	threadInitCount.target = CPU_CORES;
	threadInitCount.Wait();

	SetActiveThreads( coreMask );
}

static void ThreadShutdown() {
	TLM.shutdown = true;
}

void TaskList::Shutdown() {
	if ( TLM.shutdown ) {
		Log::WarnTag( "Shutdown() has already been called!" );

		return;
	}

	AddTasks( { Task { &ThreadShutdown }.ThreadMaskAllOthers() } );
	ThreadShutdown();

	executingThreads.fetch_sub( 1, std::memory_order_relaxed );
}

void TaskList::FinishShutdown() {
	if ( !TLM.main ) {
		Log::WarnTag( "FinishShutdown() can only be called from the main thread!" );

		return;
	}

	taskList.exitFence.Wait();

	for ( Thread* thread = threads; thread < threads + CPU_CORES; thread++ ) {
		thread->Exit();
	}

	Log::NoticeTag( "\nmain: add: %s, addQueueWait: %s, sync: %s, unknownTaskCount: %u",
		TLM.addTimer.FormatTime( ms ), TLM.addQueueWaitTimer.FormatTime( ms ),
		TLM.syncTimer.FormatTime( ms ),
		TLM.unknownTaskCount );
}

bool  TaskList::AddedToTaskList( const Task& task ) {
	return BitSet( task.flags, Task::offsetAdded );
}

bool  TaskList::IsUpdatedDependency( const Task& task ) {
	return BitSet( task.flags, Task::offsetProcessedDeps );
}

TaskEnv& TaskList::BufferIDToTask( const uint16 bufferID ) {
	return tasks[GetBits( bufferID, taskIDThreadOffset, taskIDThreadBits ), GetBits( bufferID, 0, taskIDThreadOffset )];
}

byte* TaskList::AllocTaskData( const uint16 dataSize, uint64* offset ) {
	*offset   = tasksData.GetNextElement( TLM.id, PAD( dataSize, CACHE_LINE_SIZE ) );
	byte* out = tasksData.memory + ( *offset & tasksData.mask );
	*offset >>= cacheLineBits;

	return out;
}

byte* TaskList::GetTaskData( const uint64 offset ) {
	return tasksData.memory + ( ( offset << cacheLineBits ) & tasksData.mask );
}

void TaskList::UpdateThreadRunTime( const uint64 time ) {
	threadRunTime.time[TLM.id].fetch_sub( time, std::memory_order_relaxed );
}

void TaskList::FinishTask( TaskEnv* task ) {
	task->complete.Signal();
	task->ExecuteDestructors();

	if ( task->GetArgCount() ) {
		tasksData.UpdateCurrentElement( GetBits( task->bufferID, taskIDThreadOffset, taskIDThreadBits ), task->GetDataOffset() * CACHE_LINE_SIZE );
	}

	ThreadRunTime runTime = threadRunTime;

	for ( uint8 i = 0; i < task->forwardTaskCounter; i++ ) {
		TaskEnv&     forwardTask = BufferIDToTask( task->forwardTasks[i] );

		const uint32 counter     = forwardTask.dependencyCounter.fetch_sub( 1, std::memory_order_relaxed ) - 1;

		if ( counter ) {
			continue;
		}

		TLM.addTimer.Start();

		Task tmp;
		tmp.bufferID = forwardTask.bufferID;
		SetBit( &tmp.flags, Task::offsetAllocated );
		SetBit( &tmp.flags, Task::offsetProcessed );

		AddTaskExt( tmp, &runTime );
		taskWithDependenciesCount.fetch_sub( 1, std::memory_order_relaxed );

		TLM.addTimer.Stop();
	}

	threadRunTime += runTime;

	task->SetActive( false );
}

void ThreadQueue::AddTask( const uint32 threadID, const TaskID& task ) {
	TLM.addQueueWaitTimer.Start();

	if ( threadID == TLM.id && !TLM.main ) {
		TLM.AddTask( task );
	} else {
		uint64 id = pointer.fetch_add( 1, std::memory_order_relaxed );
		id       %= maxTasks;
		while ( tasks[id].bufferID != TaskID::idNone );

		tasks[id] = task;
	}

	TLM.addQueueWaitTimer.Stop();
}

void TaskList::AddToThreadQueue( const Task& task, ThreadRunTime* runTime ) {
	TLM.addToQueueTimer.Start();

	TaskEnv& env = task.GetEnv();

	while ( !SM.taskTimesLock.Lock() );

	TaskTime taskTime;

	if ( SM.taskTimes.contains( env.Execute ) ) {
		GlobalTaskTime& SMTaskTime = SM.taskTimes[env.Execute];
		taskTime.count             = SMTaskTime.count.load( std::memory_order_relaxed );
		taskTime.time              = SMTaskTime.time.load( std::memory_order_relaxed );
	} else {
		TLM.unknownTaskCount++;
	}

	SM.taskTimesLock.Unlock();

	const uint64 projectedTime = taskTime.time / std::max( taskTime.count, 1ull ) + 50_us;
	env.time                   = projectedTime;

	if ( env.threadMask ) {
		uint32 threadMask = env.threadMask;

		taskCount.fetch_add( CountBits( threadMask ), std::memory_order_relaxed );

		while ( threadMask ) {
			const uint32 threadID    = FindLSB( threadMask );
			threadQueues[threadID].AddTask( threadID, { task.bufferID } );

			runTime->time[threadID] += projectedTime * threads[threadID].maxCoreFrequencyScale;

			UnSetBit( &threadMask, threadID );
		}

		return;
	}

	taskCount.fetch_add( 1, std::memory_order_relaxed );

	if ( projectedTime < TLM.addToQueueTimer.Time() / TLM.addToQueueCount && !TLM.main ) {
		threadQueues[TLM.id].AddTask( TLM.id, { task.bufferID } );
		runTime->time[TLM.id] += projectedTime * threads[TLM.id].maxCoreFrequencyScale;

		return;
	}

	uint64 minTime = runTime->time[0] + projectedTime * threads[0].maxCoreFrequencyScale;
	uint8  minID   = 0;

	for ( uint8 i = 1; i < coreCount; i++ ) {
		const uint64 scaledProjectedTime = projectedTime * threads[i].maxCoreFrequencyScale;

		if ( runTime->time[i] + scaledProjectedTime < minTime ) {
			minID   = i;
			minTime = runTime->time[i] + scaledProjectedTime;
		}
	}

	runTime->time[minID] += projectedTime * threads[minID].maxCoreFrequencyScale;

	threadQueues[minID].AddTask( minID, { task.bufferID } );

	TLM.addToQueueCount++;

	TLM.addToQueueTimer.Stop();
}

TaskEnv* TaskList::InitTaskEnv( Task* task ) {
	TaskEnv* env   = tasks.GetNextElementMemory( TLM.id );
	*env           = {};

	env->gen++;
	env->bufferID  = SetBits( env - ( tasks.memory + TLM.id * tasks.size ), TLM.id, taskIDThreadOffset, taskIDThreadBits );

	task->bufferID = env->bufferID;
	SetBit( &task->flags, Task::offsetAllocated );

	return env;
}

ThreadRunTime::ThreadRunTime( const AtomicThreadRunTime& other ) {
	*this = other;
}

void ThreadRunTime::operator=( const AtomicThreadRunTime& other ) {
	uint64 minTime = UINT64_MAX;

	for ( uint32 i = 0; i < coreCount; i++ ) {
		time[i] = other.time[i].load( std::memory_order_relaxed );

		minTime = time[i] < minTime ? time[i] : minTime;
	}

	for ( uint8 i = 0; i < coreCount; i++ ) {
		time[i] -= minTime;
	}
}

ThreadRunTime ThreadRunTime::operator-( const ThreadRunTime& other ) {
	ThreadRunTime out;

	for ( uint8 i = 0; i < coreCount; i++ ) {
		out.time[i] = time[i] - other.time[i];
	}

	return out;
}

void AtomicThreadRunTime::operator=( const ThreadRunTime& other ) {
	for ( uint32 i = 0; i < coreCount; i++ ) {
		time[i].store( other.time[i], std::memory_order_relaxed );
	}
}

void AtomicThreadRunTime::operator+=( const ThreadRunTime& other ) {
	for ( uint32 i = 0; i < coreCount; i++ ) {
		time[i].fetch_add( other.time[i], std::memory_order_relaxed );
	}
}

void TaskList::AddTaskExt( Task& task, ThreadRunTime* runTime ) {
	TaskEnv& env = task.GetEnv();

	SetBit( &task.flags, Task::offsetProcessed );

	if ( TLM.shutdown && !env.IsShutdownTask() ) {
		return;
	}

	if ( env.threadMask && !( env.threadMask & TLM.activeThreadMask ) ) {
		return;
	}

	if ( AddedToTaskList( task ) ) {
		return;
	}

	TLM.addTimer.Start();

	env.threadCount.value.store( env.threadMask ? CountBits( env.threadMask ) : 1, std::memory_order_relaxed );

	uint64 time = TimeNs();
	if ( time < env.time && env.time - time > eventQueue.minGranularity ) {
		if ( eventQueue.AddTask( std::move( task ) ) ) {
			return;
		}
	}

	if ( !env.dependencyCounter ) {
		AddToThreadQueue( task, runTime );
		SetBit( &task.flags, Task::offsetAdded );
	}

	TLM.addTimer.Stop();
}

void TaskList::MarkDependencies( const Task& task, const TaskInitList& dependencies ) {
	uint8 dependencyCounter = 0;

	if ( IsUpdatedDependency( task ) ) {
		return;
	}

	TaskEnv& mainEnv = task.GetEnv();

	if ( !dependencies.taskStart || dependencies.taskStart == dependencies.taskEnd ) {
		mainEnv.dependencyCounter.store( 0, std::memory_order_relaxed );
		return;
	}

	for ( const TaskProxy& dep : dependencies ) {
		TaskEnv& env = dep.GetEnv();

		if ( AddedToTaskList( *dep.task ) ) {
			if ( !env.threadCount.Lock() ) {
				continue;
			}

			uint32 id = env.forwardTaskCounter;

			ASSERT_LE( id, TaskEnv::maxForwardTasks );

			env.forwardTasks[id] = task.bufferID;
			env.forwardTaskCounter++;

			mainEnv.dependencyCounter.fetch_add( 1, std::memory_order_relaxed );

			if ( env.threadCount.Unlock() ) {
				FinishTask( &env );
			}

			continue;
		}

		env.forwardTasks[env.forwardTaskCounter] = task.bufferID;
		env.forwardTaskCounter++;

		dependencyCounter++;
	}

	mainEnv.dependencyCounter.fetch_add( dependencyCounter - 1, std::memory_order_relaxed );

	SetBit( &mainEnv.flags, Task::offsetProcessedDeps );
}

void TaskList::UnMarkDependencies( const TaskInitList& dependencies ) {
	for ( const TaskProxy& dep : dependencies ) {
		UnSetBit( &dep.task->flags, Task::offsetProcessedDeps );
	}
}

void TaskList::AddTasksExt( std::initializer_list<TaskInitList> dependencies ) {
	// TODO: Currently this is an O( 3 * n ) loop. The tasks form a DAG, which we can instead flatten in O( n ), then loop in O( n )
	for ( const TaskInitList& taskInit : dependencies ) {
		MarkDependencies( *taskInit.taskStart->task, { taskInit.taskStart + 1, taskInit.taskEnd } );
	}
	
	ThreadRunTime runTime     = threadRunTime;
	ThreadRunTime baseRunTime = runTime;

	for ( const TaskInitList& taskInit : dependencies ) {
		for ( const TaskProxy* task = taskInit.taskStart + 1; task < taskInit.taskEnd; task++ ) {
			if ( !AddedToTaskList( *task->task ) ) {
				task->GetEnv().dependencyCounter.store( 0, std::memory_order_relaxed );
			}

			AddTaskExt( *task->task, &runTime );
		}

		AddTaskExt( *taskInit.taskStart->task, &runTime );
	}

	threadRunTime += runTime - baseRunTime;

	for ( const TaskInitList& taskInit : dependencies ) {
		UnMarkDependencies( { taskInit.taskStart + 1, taskInit.taskEnd } );
	}
}

TaskEnv* TaskList::FetchTask() {
	ThreadQueue& threadQueue = threadQueues[TLM.id];
	uint8        current     = threadQueue.current;
	TaskID       task        = threadQueue.tasks[current];

	if ( task.bufferID == TaskID::idNone ) {
		return nullptr;
	}

	threadQueue.tasks[current] = {};
	threadQueue.current        = ( current + 1 ) % ThreadQueue::maxTasks;

	return &task.GetEnv();
}

void TaskList::TaskWait( const Task& task ) {
	if ( !AddedToTaskList( task ) ) {
		Log::WarnTag( "Tried to wait for a non-added task" );

		return;
	}

	task.GetEnv().complete.Wait();
}

void TaskList::TasksCleared( const uint32 count ) {
	taskCount.fetch_sub( count, std::memory_order_relaxed );
}

void TaskList::TaskStarted() {
	executingThreads.fetch_add( 1, std::memory_order_relaxed );

	taskCount.fetch_sub( 1, std::memory_order_relaxed );
}

bool TaskList::ThreadFinished( const bool hadTask ) {
	const uint32 threadCount = executingThreads.fetch_sub( hadTask, std::memory_order_relaxed ) - hadTask;

	if ( !TLM.shutdown ) {
		return false;
	}

	TLM.exitTimer.Start();
	eventQueue.Shutdown();

	if ( !threadCount ) {
		if ( !taskCount.load( std::memory_order_acquire ) ) {
			exitFence.Signal();

			return true;
		}
	}

	return false;
}