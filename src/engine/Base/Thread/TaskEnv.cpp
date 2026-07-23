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

#include "TaskEnv.h"

TaskEnv::TaskEnv() {
}

TaskEnv::TaskEnv( const TaskEnv& other ) {
	*this = other;
}

TaskEnv& TaskEnv::Delay( const uint64 delay ) {
	if ( IsShutdownTask() ) {
		Log::Warn( "Shutdown tasks may not be delayed! (task: %s, delay: %u)", Execute, delay );
	} else {
		time = TimeNs() + delay;
	}

	return *this;
}

TaskEnv& TaskEnv::ThreadMask( const uint64 newThreadMask ) {
	threadMask = newThreadMask;

	return *this;
}

TaskEnv& TaskEnv::ThreadMaskAll() {
	return ThreadMask( BitMask64( 0, TLM.currentMaxThreads ) );
}

TaskEnv& TaskEnv::ThreadMaskAllOthers() {
	return TLM.main ? ThreadMaskAll() : ThreadMask( UnSetBit( BitMask64( 0, TLM.currentMaxThreads ), TLM.id ) );
}

TaskEnv& TaskEnv::ThreadMaskCurrent() {
	return ThreadMask( SetBit( 0ull, TLM.id ) );
}

void TaskEnv::ExecuteDestructors() {
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

bool TaskEnv::IsActive() {
	return BitSet(  flags,  activeOffset );
}

bool TaskEnv::IsShutdownTask() {
	return BitSet(  flags,  shutdownOffset );
}

uint8 TaskEnv::GetArgCount() {
	return GetBits( flags, argCountOffset, 3 );
}

void TaskEnv::SetActive( const bool active ) {
	active ? SetBit( &flags, activeOffset ) : UnSetBit( &flags, activeOffset );
}

uint32 TaskEnv::RemapArg( const uint32 arg ) {
	return GetBits( argsMap, arg * argMapArgSize + argMapArgOffset, argMapArgSize );
}

uint64 TaskEnv::GetDataOffset() {
	return SetBits( ( uint64 ) dataOffset, ( uint64 ) dataOffset2, 32, 8 );
}

uint32 TaskEnv::SetArgsMap( Arg* start, Arg* end ) {
	uint32 size = 0;

	for ( Arg* arg = start; arg < end; arg++ ) {
		const uint32 argOffset = size;

		size = PAD( size, arg->alignment ) + arg->size;
		SetBits( &argsMap, arg - start, arg->id * argMapArgSize + argMapArgOffset, argMapArgSize );

		if ( arg > start ) {
			dataOffsets[RemapArg( arg - start )] = argOffset;
		}

		if ( arg->hasDestructor ) {
			SetBit( &argsMap, arg->id );
		};
	}

	for ( Arg* arg = start; arg < end; arg++ ) {
		dataOffsets[RemapArg( arg - start )] += CountBits( argsMap & argMapMask ) * sizeof( DestructorFunction );
	}

	SetBits( &flags, end - start, argCountOffset, 3 );

	return CountBits( argsMap & argMapMask ) * sizeof( DestructorFunction ) + PAD( size, 8 );
}

byte* TaskEnv::InitMemory( Arg* start, Arg* end, TaskFunction execute, uint64* dataOffsetsOut ) {
	Execute = execute;

	std::sort( start, end,
		[]( const Arg& lhs, const Arg& rhs ) {
			return lhs.alignment > rhs.alignment;
		}
	);

	uint32 dataSize = SetArgsMap( start, end );

	uint64 offset;
	byte*  data     = taskList.AllocTaskData( dataSize, &offset );

	dataOffset      = GetBits( offset, 0, 32 );
	dataOffset2     = GetBits( offset, 32, 8 );

	memcpy( dataOffsetsOut, dataOffsets, sizeof( uint64 ) );

	return data;
}

byte* TaskEnv::GetArgMemory( const uint32 arg ) {
	return taskList.GetTaskData( GetDataOffset() ) + dataOffsets[arg];
}

void TaskEnv::operator=( const TaskEnv& other ) {
	Execute            = other.Execute;
	complete           = other.complete;

	dataOffset         = other.dataOffset;
	dataOffset2        = other.dataOffset2;

	flags              = other.flags;

	bufferID           = other.bufferID;

	gen                = other.gen;

	time               = other.time;
	threadMask         = other.threadMask;

	dependencyCounter.store( other.dependencyCounter.load( std::memory_order_relaxed ), std::memory_order_relaxed );
	forwardTaskCounter = other.forwardTaskCounter;
	threadCount        = other.threadCount;

	memcpy( dataOffsets,  other.dataOffsets,  Task::maxArgCount * sizeof( uint16 ) );
	memcpy( forwardTasks, other.forwardTasks, maxForwardTasks   * sizeof( uint16 ) );

	argsMap            = other.argsMap;
}