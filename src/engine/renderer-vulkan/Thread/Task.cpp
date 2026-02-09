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
// Task.cpp

#include "../Math/Bit.h"

#include "ThreadMemory.h"

#include "Task.h"

Task::Task() :
	id( SetBit( 0u, 15 ) ) {
}

Task::Task( const Task& other ) {
	*this = other;
}

Task& Task::Delay( const uint64 delay ) {
	time = TimeNs() + delay;

	return *this;
}

Task& Task::ThreadMask( const uint64 newThreadMask ) {
	threadMask = newThreadMask;
	threadCount.store( CountBits( threadMask ), std::memory_order_relaxed );

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

void Task::operator=( const Task& other ) {
	Execute            = other.Execute;
	data               = other.data;

	complete           = other.complete;

	active             = other.active;
	shutdownTask       = other.shutdownTask;

	eventMask          = other.eventMask;

	gen                = other.gen;
	time               = other.time;

	dependencyCounter  = other.dependencyCounter.load( std::memory_order_relaxed );
	forwardTaskCounter = other.forwardTaskCounter.load( std::memory_order_relaxed );

	id                 = other.id;

	bufferID           = other.bufferID;
	threadMask         = other.threadMask;
	threadCount        = other.threadCount.load( std::memory_order_relaxed );

	forwardTaskLock    = other.forwardTaskLock;

	memcpy( forwardTasks, other.forwardTasks, MAX_FORWARD_TASKS * sizeof( uint16 ) );

	dataSize           = other.dataSize;
}
