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

#include "EventQueue.h"
#include "GlobalMemory.h"
#include "TaskData.h"
#include "ThreadMemory.h"
#include "ThreadUplink.h"

#include "TaskList.h"

TaskList taskList;

static void SyncThreadCount() {
	TLM.currentMaxThreads = taskList.currentMaxThreads.load( std::memory_order_relaxed );
}

TaskList::TaskList() {
}

TaskList::~TaskList() {
}

void TaskList::AdjustThreadCount( uint32 newMaxThreads ) {
	if ( newMaxThreads > MAX_THREADS ) {
		Log::WarnTag( "Maximum thread count exceeded: %u > %u, setting to %u",
			newMaxThreads, MAX_THREADS, MAX_THREADS );
		newMaxThreads = MAX_THREADS;
	}

	if ( newMaxThreads == 0 ) {
		Log::WarnTag( "Thread count can't be 0" );
		return;
	}

	while ( !threadCountLock.LockWrite() );

	// This must always be set before using ThreadMask*() on main thread
	TLM.currentMaxThreads       = newMaxThreads;

	const uint32 currentThreads = currentMaxThreads.load( std::memory_order_relaxed );
	if ( newMaxThreads > currentThreads ) {
		currentMaxThreads.store( newMaxThreads, std::memory_order_relaxed );

		taskList.AddTask( Task { &SyncThreadCount }.ThreadMaskAll() );

		for ( uint32 i = currentThreads; i < newMaxThreads; i++ ) {
			threads[i].Start( i );
		}
	} else if ( newMaxThreads < currentThreads ) {
		currentMaxThreads.store( newMaxThreads, std::memory_order_relaxed );

		taskList.AddTask( Task { &SyncThreadCount }.ThreadMaskAll() );

		for ( uint32 i = newMaxThreads; i < currentThreads; i++ ) {
			threads[i].exiting = true;
		}
	}

	if ( !TLM.main ) {
		taskList.AddTask( Task { &SyncThreadCount }.ThreadMaskAllOthers() );
		SyncThreadCount();
	}

	Log::NoticeTag( "Changed thread count to %u", TLM.currentMaxThreads );

	threadCountLock.UnlockWrite();
}

void TaskList::Init() {
	tasks.Alloc( MAX_TASKS );
	tasksData.Alloc( MAX_TASK_DATA );

	int threads = e_threadCount.Get();
	threads     = threads ? threads : CPU_CORES;
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

	for ( Thread* thread = threads; thread < threads + currentMaxThreads.load( std::memory_order_relaxed ); thread++ ) {
		thread->Exit();
	}

	Log::NoticeTag( "\nmain: add: %s, addQueueWait: %s, sync: %s, unknownTaskCount: %u",
		TLM.addTimer.FormatTime( ms ), TLM.addQueueWaitTimer.FormatTime( ms ),
		TLM.syncTimer.FormatTime( ms ),
		TLM.unknownTaskCount );

	std::string debugOut;
	debugOut.reserve( 3 * currentMaxThreads.load( std::memory_order_relaxed ) );
	for ( uint32 i = 0; i < currentMaxThreads.load( std::memory_order_relaxed ); i++ ) {
		debugOut += Str::Format( "%u ", TLM.idleThreads[i] );
	}
	
	Log::NoticeTag( debugOut );
}

bool  TaskList::AddedToTaskList( const uint8 id ) {
	return BitSet( id, TASK_SHIFT_ADDED );
}

bool  TaskList::AddedToTaskMemory( const uint16 bufferID ) {
	return bufferID != Task::UNALLOCATED;
}

bool  TaskList::HasUntrackedDeps( const uint8 id ) {
	return BitSet( id, TASK_SHIFT_HAS_UNTRACKED_DEPS );
}

bool  TaskList::IsTrackedDependency( const uint8 id ) {
	return BitSet( id, TASK_SHIFT_TRACKED_DEPENDENCY );
}

bool  TaskList::IsUpdatedDependency( const uint8 id ) {
	return BitSet( id, TASK_SHIFT_UPDATED_DEPENDENCY );
}

byte* TaskList::AllocTaskData( const uint16 dataSize, uint64* offset ) {
	*offset   = tasksData.GetNextElement( PAD( dataSize, CACHE_LINE_SIZE ) );
	byte* out = tasksData.memory + ( *offset & tasksData.mask );
	*offset >>= cacheLineBits;

	return out;
}

byte* TaskList::GetTaskData( const uint64 offset ) {
	return tasksData.memory + ( ( offset << cacheLineBits ) & tasksData.mask );
}

void TaskList::FinishTask( Task* task ) {
	task->complete.Signal();
	task->ExecuteDestructors();

	if ( task->GetArgCount() ) {
		tasksData.UpdateCurrentElement( task->GetDataOffset() );
	}

	for ( uint8 i = 0; i < task->forwardTaskCounter; i++ ) {
		taskList.FinishDependency( task->forwardTasks[i] );
	}

	task->SetActive( false );
}

void TaskList::FinishDependency( const uint16 bufferID ) {
	Task&        task    = tasks[bufferID];

	const uint32 counter = task.dependencyCounter.fetch_sub( 1, std::memory_order_relaxed ) - 1;

	if ( !counter ) {
		TLM.addTimer.Start();

		if ( TimeNs() < task.time ) {
			eventQueue.AddTask( task );
		} else {
			AddToThreadQueue( task );
			taskWithDependenciesCount.fetch_sub( 1, std::memory_order_relaxed );
		}

		TLM.addTimer.Stop();
	}
}

void TaskList::ResolveDependencies( Task& task, TaskInitList& dependencies ) {
	for ( const TaskProxy* dep = dependencies.start; dep < dependencies.end; dep++ ) {
		if ( !AddedToTaskMemory( dep->task.bufferID ) ) {
			Sys::Drop( "Tried to add task with an unallocated dependency" );
		}

		if ( IsTrackedDependency( dep->task.id ) ) {
			continue;
		}

		Task& dependency  = tasks[dep->task.bufferID];

		// The dependency has already been executed, but the ringbuffer wrapped around
		if ( dependency.gen > dep->task.gen ) {
			continue;
		}

		if ( !dependency.threadCount.Lock() ) {
			continue;
		}

		uint32 id = dependency.forwardTaskCounter;

		ASSERT_LE( id, Task::maxForwardTasks );

		dependency.forwardTasks[id] = task.bufferID;
		dependency.forwardTaskCounter++;

		task.dependencyCounter.fetch_add( 1, std::memory_order_relaxed );

		if ( dependency.threadCount.Unlock() ) {
			taskList.FinishTask( &dependency );
		}
	}
}

void ThreadQueue::AddTask( const uint32 threadID, const uint16 bufferID ) {
	TLM.addQueueWaitTimer.Start();

	if ( threadID == TLM.id && !TLM.main ) {
		TLM.AddTask( &taskList.tasks[bufferID] );
	} else {
		uint64 id = pointer.fetch_add( 1, std::memory_order_relaxed );
		id       %= MAX_TASKS;
		while ( tasks[id] != TASK_NONE );

		tasks[id] = bufferID;
	}

	TLM.addQueueWaitTimer.Stop();
}

void TaskList::AddToThreadQueueExt( Task& task ) {
	while ( !SM.taskTimesLock.Lock() );

	TaskTime taskTime;

	if ( SM.taskTimes.contains( task.Execute ) ) {
		GlobalTaskTime& SMTaskTime = SM.taskTimes[task.Execute];
		taskTime.count             = SMTaskTime.count.load( std::memory_order_relaxed );
		taskTime.time              = SMTaskTime.time.load( std::memory_order_relaxed );
	} else {
		TLM.unknownTaskCount++;
	}

	SM.taskTimesLock.Unlock();

	if ( task.threadMask ) {
		uint32 threadMask = task.threadMask;

		taskCount.fetch_add( CountBits( threadMask ), std::memory_order_relaxed );

		while ( threadMask ) {
			const uint32 threadID = FindLSB( threadMask );
			threadQueues[threadID].AddTask( threadID, task.bufferID );

			UnSetBit( &threadMask, threadID );
		}

		return;
	}

	taskCount.fetch_add( 1, std::memory_order_relaxed );

	const uint64 projectedTime = taskTime.time / std::max( taskTime.count, 1ull );

	if ( projectedTime < TLM.addToQueueTimer.Time() / TLM.addToQueueCount && !TLM.main ) {
		threadQueues[TLM.id].AddTask( TLM.id, task.bufferID );
		return;
	}

	/* Nodes correspond to TLM.currentMaxThreads active ThreadQueues
	Whenever a task is added it either goes to the currentThreadExecutionNode or ~thread that is most free of work
	currentThreadExecutionNode contains either an idle thread that has most recently finished execution,
	unless no tasks have been executed yet,
	or a task was already added to the latest idle thread
	threadExecutionNodes is kept sorted in a non-decreasing order when tasks are added */
	for ( uint8 node = 0; node < TLM.currentMaxThreads; node++ ) {
		const uint64 scaledProjectedTime = projectedTime / threads[node].maxCoreFrequencyScale;

		uint64       baseThreadTime      = threadExecutionNodes[node].fetch_add( scaledProjectedTime, std::memory_order_relaxed );
		uint64       nextNodeTime        = node == TLM.currentMaxThreads - 1 ?
		                                           UINT64_MAX
		                                         : threadExecutionNodes[node + 1].load( std::memory_order_relaxed );

		if ( node == TLM.currentMaxThreads - 1
			|| baseThreadTime + scaledProjectedTime <= nextNodeTime ) {
			threadQueues[node].AddTask( node, task.bufferID );
			return;
		}

		// We overflowed the current node, so move to the next one
		if ( baseThreadTime + scaledProjectedTime > nextNodeTime ) {
			threadExecutionNodes[node].fetch_sub( scaledProjectedTime, std::memory_order_relaxed );
			continue;
		}

		/* Current node is overflowed but we don't know if we overflowed it or if another thread did
		Another thread is guaranteed to have overflowed it so we wait until it moves to the next node*/
		do {
			baseThreadTime = threadExecutionNodes[node    ].load( std::memory_order_relaxed );
			nextNodeTime   = threadExecutionNodes[node + 1].load( std::memory_order_relaxed );
		} while ( baseThreadTime - scaledProjectedTime > nextNodeTime );

		if ( baseThreadTime <= nextNodeTime ) {
			threadQueues[node].AddTask( node, task.bufferID );
			return;
		}

		// We still overflowed
		threadExecutionNodes[node].fetch_sub( scaledProjectedTime, std::memory_order_relaxed );
	}
}

void TaskList::AddToThreadQueue( Task& task ) {
	TLM.addToQueueTimer.Start();

	AddToThreadQueueExt( task );
	TLM.addToQueueCount++;

	TLM.addToQueueTimer.Stop();
}

Task* TaskList::GetTaskMemory( Task& task ) {
	if ( AddedToTaskMemory( task.bufferID ) ) {
		return &tasks[task.bufferID];
	}

	Task* taskMemory     = tasks.GetNextElementMemory();

	task.SetActive( true );
	task.gen             = taskMemory->gen + 1;
	task.bufferID        = taskMemory - tasks.memory;

	*taskMemory          = task;

	return taskMemory;
}

void TaskList::AddTaskExt( Task& task, TaskInitList&& dependencies ) {
	if ( exiting.load( std::memory_order_relaxed ) && !task.IsShutdownTask() ) {
		return;
	}

	if ( !task.IsValid() ) {
		return;
	}

	if ( AddedToTaskList( task.id ) ) {
		return;
	}

	TLM.addTimer.Start();

	Task* taskMemory = GetTaskMemory( task );
	taskMemory->id   = task.id;
	taskMemory->threadCount.value.store( taskMemory->threadMask ? CountBits( taskMemory->threadMask ) : 1, std::memory_order_relaxed );

	if ( HasUntrackedDeps( task.id ) ) {
		ResolveDependencies( *taskMemory, dependencies );

		const uint32 counter = taskMemory->dependencyCounter.fetch_sub( 1, std::memory_order_relaxed ) - 1;

		if ( !counter ) {
			AddToThreadQueue( *taskMemory );
			SetBit( &task.id, TASK_SHIFT_ADDED );
		} else {
			taskWithDependenciesCount.fetch_add( 1, std::memory_order_relaxed );
		}
	} else if ( !taskMemory->dependencyCounter ) {
		AddToThreadQueue( *taskMemory );
		SetBit( &task.id, TASK_SHIFT_ADDED );
	}

	TLM.addTimer.Stop();
}

void TaskList::MarkDependencies( Task& task, TaskInitList&& dependencies ) {
	Task* mainTask          = GetTaskMemory( task );
	uint8 dependencyCounter = 0;

	if ( IsUpdatedDependency( mainTask->id ) ) {
		return;
	}

	if ( !dependencies.start ) {
		mainTask->dependencyCounter.store( 0, std::memory_order_relaxed );
		return;
	}

	for ( const TaskProxy* dep = dependencies.start; dep < dependencies.end; dep++ ) {
		if ( AddedToTaskList( dep->task.id ) ) {
			continue;
		}

		Task* taskMemory = GetTaskMemory( ( *dep ).GetTask() );

		taskMemory->forwardTasks[taskMemory->forwardTaskCounter] = mainTask->bufferID;
		taskMemory->forwardTaskCounter++;

		SetBit( &dep->task.id, TASK_SHIFT_TRACKED_DEPENDENCY );

		dependencyCounter++;
	}

	if ( dependencyCounter != ( dependencies.end - dependencies.start ) ) {
		SetBit( &task.id, TASK_SHIFT_HAS_UNTRACKED_DEPS );

		mainTask->dependencyCounter.fetch_add( dependencyCounter,     std::memory_order_relaxed );
	} else {
		mainTask->dependencyCounter.fetch_add( dependencyCounter - 1, std::memory_order_relaxed );
	}

	SetBit( &task.id, TASK_SHIFT_UPDATED_DEPENDENCY );
}

void TaskList::UnMarkDependencies( TaskInitList&& dependencies ) {
	for ( const TaskProxy* dep = dependencies.start; dep < dependencies.end; dep++ ) {
		UnSetBit( &( *dep )->id, TASK_SHIFT_TRACKED_DEPENDENCY );
		UnSetBit( &( *dep )->id, TASK_SHIFT_UPDATED_DEPENDENCY );
	}
}

void TaskList::AddTask( Task& task, std::initializer_list<TaskProxy> dependencies ) {
	MarkDependencies( task, TaskInitList { dependencies.begin(), dependencies.end() } );

	uint64 time = TimeNs();
	if ( time < task.time && task.time - time > eventQueue.minGranularity ) {
		eventQueue.AddTask( task );
	} else {
		AddTaskExt( task, TaskInitList { dependencies.begin(), dependencies.end() } );
	}

	for ( const TaskProxy& dep : dependencies ) {
		AddTaskExt( dep.task );
	}

	UnMarkDependencies( TaskInitList { dependencies.begin(), dependencies.end() } );
}

void TaskList::AddTasksExt( std::initializer_list<TaskInitList> dependencies ) {
	// TODO: Currently this is an O( 3 * n ) loop. The tasks form a DAG, which we can instead flatten in O( n ), then loop in O( n )
	for ( const TaskInitList& taskInit : dependencies ) {
		MarkDependencies( taskInit.start->task, { taskInit.start + 1, taskInit.end } );
	}

	for ( const TaskInitList& taskInit : dependencies ) {
		for ( const TaskProxy* task = taskInit.start + 1; task < taskInit.end; task++ ) {
			if ( !IsUpdatedDependency( task->task.id ) && !AddedToTaskList( task->task.id ) ) {
				GetTaskMemory( task->task )->dependencyCounter.store( 0, std::memory_order_relaxed );
			}

			uint64 time = TimeNs();
			if ( time < task->task.time && task->task.time - time > eventQueue.minGranularity ) {
				eventQueue.AddTask( task->task );
			} else {
				AddTaskExt( task->task );
			}
		}

		AddTaskExt( taskInit.start->task, { taskInit.start + 1, taskInit.end } );
	}

	for ( const TaskInitList& taskInit : dependencies ) {
		UnMarkDependencies( { taskInit.start + 1, taskInit.end } );
	}
}

Task* TaskList::FetchTask() {
	ThreadQueue& threadQueue = threadQueues[TLM.id];
	uint8        current     = threadQueue.current;
	uint16       id          = threadQueue.tasks[current];

	if ( id == ThreadQueue::TASK_NONE ) {
		return nullptr;
	}

	threadQueue.tasks[current] = ThreadQueue::TASK_NONE;
	threadQueue.current        = ( current + 1 ) % ThreadQueue::MAX_TASKS;

	return &tasks[id];
}

void TaskList::TaskWait( Task& task ) {
	if ( !AddedToTaskMemory( task.bufferID ) ) {
		Log::WarnTag( "Tried to wait for a non-added task" );

		return;
	}

	Task* taskMemory = GetTaskMemory( task );

	if ( taskMemory->gen > task.gen ) {
		return;
	}

	taskMemory->complete.Wait();
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
	const bool   exit        = exiting.load( std::memory_order_relaxed );

	if ( exit ) {
		TLM.exitTimer.Start();
		eventQueue.Shutdown();
	}

	if ( exit && !threadCount ) {
		if ( !taskCount.load( std::memory_order_acquire ) ) {
			exitFence.Signal();

			return true;
		}
	}

	return false;
}

byte* AllocTaskData( const uint16 dataSize, uint64* offset ) {
	return taskList.AllocTaskData( dataSize, offset );
}