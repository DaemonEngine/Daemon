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

#include "Thread/TaskEnv.h"
#include "Thread/TaskList.h"

#include "Task.h"

Task::Task( const TaskProxy& other ) {
	bufferID = other.task->bufferID;
}

Task::Task( Task&& other ) {
	*this = std::move( other );
}

Task::~Task() {
	if ( BitSet( flags, offsetProcessed ) && !flags ) {
		Log::Warn( "Task %s was not added", GetEnv().Execute );
	}
}

void Task::operator=( Task&& other ) {
	bufferID = other.bufferID;
	flags    = other.flags;
}

bool Task::operator==( const Task& other ) {
	return bufferID == other.bufferID;
}

bool Task::operator!=( const Task& other ) {
	return !( *this == other );
}

Task& Task::Delay( const uint64 delay ) {
	GetEnv().Delay( delay );

	return *this;
}

Task& Task::ThreadMask( const uint64 newThreadMask ) {
	GetEnv().ThreadMask( newThreadMask );

	return *this;
}

Task& Task::ThreadMaskAll() {
	GetEnv().ThreadMaskAll();

	return *this;
}

Task& Task::ThreadMaskAllOthers() {
	GetEnv().ThreadMaskAllOthers();

	return *this;
}

Task& Task::ThreadMaskCurrent() {
	GetEnv().ThreadMaskCurrent();

	return *this;
}

void Task::Wait() {
	taskList.TaskWait( *this );
}

TaskEnv& Task::GetEnv() const {
	return taskList.BufferIDToTask( bufferID );
}

Task::ArgOffsets Task::InitMemory( Arg* start, Arg* end, TaskFunction execute ) {
	uint64   dataOffsets;
	TaskEnv* env    = taskList.InitTaskEnv( this );
	byte*    memory = env->InitMemory( start, end, execute, &dataOffsets );

	SetBit( &flags, offsetAllocated );

	return { memory, dataOffsets };
}

TaskProxy::TaskProxy( Task& newTask ) :
	task( &newTask ) {
}

TaskProxy::TaskProxy( Task&& newTask ) :
	task( &newTask ) {
}

TaskEnv& TaskProxy::GetEnv()  const {
	return task->GetEnv();
}

TaskInitList::TaskInitList() :
	taskStart( nullptr ),
	taskEnd( nullptr ) {
}

TaskInitList::TaskInitList( const TaskProxy* newStart, const TaskProxy* newEnd ) :
	taskStart( newStart ),
	taskEnd( newEnd ) {
}

TaskInitList::TaskInitList( std::initializer_list<TaskProxy> list ) :
	taskStart( list.begin() ),
	taskEnd( list.end() ) {
}

void AddTasksExt( std::initializer_list<TaskInitList> dependencies ) {
	taskList.AddTasksExt( dependencies );
}