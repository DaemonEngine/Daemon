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

#include <algorithm>

#include "Int.h"

#include "TaskList.h"
#include "ThreadMemory.h"

#include "Task.h"

Task::Task() :
	id( SetBit( 0u, 15 ) ) {
	SetValid( true );
}

Task::Task( const Task& other ) {
	*this = other;
}

const Task& Task::operator*() {
	return *this;
}

const Task* Task::operator->() const {
	return this;
}

constexpr Task& Task::GetTask() {
	return *this;
}

Task& Task::Delay( const uint64 delay ) {
	if ( IsShutdownTask() ) {
		Log::Warn( "Shutdown tasks may not be delayed! (task: %s, delay: %u)", Execute, delay );
	} else {
		time = TimeNs() + delay;
	}

	return *this;
}

Task& Task::ThreadMask( const uint64 newThreadMask ) {
	threadMask = newThreadMask;

	if ( !threadMask ) {
		SetValid( false );
	}

	return *this;
}

Task& Task::ThreadMaskAll() {
	return ThreadMask( BitMask64( 0, TLM.currentMaxThreads ) );
}

Task& Task::ThreadMaskAllOthers() {
	return ThreadMask( UnSetBit( BitMask64( 0, TLM.currentMaxThreads ), TLM.id ) );
}

Task& Task::ThreadMaskCurrent() {
	return ThreadMask( SetBit( 0ull, TLM.id ) );
}

void Task::Wait() {
	if ( IsValid() ) {
		taskList.TaskWait( *this );
	}
}

void Task::ExecuteDestructors() {
	uint8  destructors = argsMap & 255;
	uint32 offset      = 0;

	while ( destructors ) {
		DestructorFunction destructor = *( DestructorFunction* ) ( taskList.GetTaskData( GetDataOffset() ) + offset );

		uint32             arg        = FindLSB( destructors );
		destructor( GetArgMemory( arg ) );

		offset                       += sizeof( DestructorFunction );
		UnSetBit( &destructors, arg );
	}
}

bool Task::IsValid() {
	return BitSet(  flags,  validOffset );
}

bool Task::IsActive() {
	return BitSet(  flags,  activeOffset );
}

bool Task::IsShutdownTask() {
	return BitSet(  flags,  shutdownOffset );
}

uint8 Task::GetArgCount() {
	return GetBits( flags, argCountOffset, 3 );
}

void Task::SetValid(  const bool valid ) {
	valid  ? SetBit( &flags, validOffset )  : UnSetBit( &flags, validOffset );
}

void Task::SetActive( const bool active ) {
	active ? SetBit( &flags, activeOffset ) : UnSetBit( &flags, activeOffset );
}

uint32 Task::RemapArg( const uint32 arg ) {
	return GetBits( argsMap, arg * argMapArgSize + argMapArgOffset, argMapArgSize );
}

uint64 Task::GetDataOffset() {
	return SetBits( ( uint64 ) dataOffset, ( uint64 ) dataOffset2, 32, 8 );
}

uint32 Task::SetArgsMap( Arg* start, Arg* end ) {
	uint32 size = 0;

	for ( Arg* arg = start; arg < end; arg++ ) {
		if ( arg > start ) {
			dataOffsets[arg - start] = size;
		}

		size = PAD( size, arg->size > 8 ? 8 : arg->size ) + arg->size;
		SetBits( &argsMap, arg - start, arg->id * argMapArgSize + argMapArgOffset, argMapArgSize );

		if ( arg->hasDestructor ) {
			SetBit( &argsMap, arg->id );
		};
	}

	SetBits( &flags, end - start, argCountOffset, 3 );

	return CountBits( argsMap & argMapMask ) * sizeof( DestructorFunction ) + PAD( size, 8 );
}

byte* Task::InitMemory( Arg* start, Arg* end ) {
	SetValid( true );

	std::sort( start, end,
		[]( const Arg& lhs, const Arg& rhs ) {
			return lhs.size > rhs.size;
		}
	);

	uint32 dataSize = SetArgsMap( start, end );

	uint64 offset;
	byte* data  = AllocTaskData( dataSize, &offset );

	dataOffset  = GetBits( offset, 0, 32 );
	dataOffset2 = GetBits( offset, 32, 8 );

	return data;
}

byte* Task::GetArgMemory( const uint32 arg ) {
	return taskList.GetTaskData( GetDataOffset() ) + dataOffsets[RemapArg( arg )];
}

void Task::operator=( const Task& other ) {
	Execute            = other.Execute;
	complete           = other.complete;

	dataOffset         = other.dataOffset;
	dataOffset2        = other.dataOffset2;

	flags              = other.flags;
	id                 = other.id;

	bufferID           = other.bufferID;

	gen                = other.gen;

	time               = other.time;
	threadMask         = other.threadMask;

	dependencyCounter.store( other.dependencyCounter.load( std::memory_order_relaxed ), std::memory_order_relaxed );
	forwardTaskCounter = other.forwardTaskCounter;
	threadCount        = other.threadCount;

	memcpy( dataOffsets,  other.dataOffsets,  maxArgCount     * sizeof( uint16 ) );
	memcpy( forwardTasks, other.forwardTasks, maxForwardTasks * sizeof( uint16 ) );

	argsMap            = other.argsMap;
}

TaskProxy::TaskProxy( Task& newTask ) :
	task( newTask ) {
}

Task* TaskProxy::operator->() const {
	return &task;
}