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

#include "Int.h"

#include "TaskEnv.h"
#include "TaskList.h"
#include "ThreadMemory.h"

#include "EventQueue.h"

EventRing::EventResult EventRing::AddTask( Task&& task, const uint64 targetTime ) {
	uint32 sector = ( targetTime - currentTime ) / granularity;

	// This shouldn't happen unless the eventScheduler thread is far behind, which causes currentTime to differ too much from TimeNs()
	if ( sector >= sectors ) {
		return EVENT_EXPIRED;
	}

	sector         = ( ( sector > 0 ? sector - 1 : sector ) + currentSector ) & sectorMask;
	uint32 eventID = FindLZeroBit( allocatedEvents[sector] );

	if ( eventID == 64 ) {
		return EVENT_FAIL;
	}

	SetBit( &allocatedEvents[sector], eventID );
	events[sector][eventID] = std::move( task );

	return EVENT_SUCCESS;
}

void EventRing::Rotate() {
	uint32 count = 0;

	while ( ( count + 1 ) * granularity + currentTime <= TimeNs() && count < sectors ) {
		uint64* sector = &allocatedEvents[( currentSector + count ) & sectorMask];

		for ( uint32 eventID = FindLSB( *sector ); eventID != 64; eventID = FindLSB( *sector ) ) {
			AddTasks( { events[( currentSector + count ) & sectorMask][eventID] } );
			UnSetBit( sector, eventID );
		}

		count++;
	}

	if ( count ) {
		currentTime   = TimeNs();
		currentSector = ( currentSector + count ) & sectorMask;
	}
}

bool EventQueue::AddTask( Task&& task ) {
	const uint64           time       = TimeNs();
	const uint64           targetTime = task.GetEnv().time;
	const uint64           delay      = time <= targetTime ? targetTime - time : 0;

	EventRing::EventResult res        = EventRing::EVENT_FAIL;

	if ( !delay ) {
		return false;
	}

	for ( EventRing& eventRing : eventRings ) {
		if ( delay <= eventRing.granularity * EventRing::sectors ) {
			while ( !eventRing.lock.LockWrite() );

			res = eventRing.AddTask( std::move( task ), targetTime );

			eventRing.lock.UnlockWrite();

			if ( res == EventRing::EVENT_SUCCESS ) {
				break;
			}
		}
	}

	switch ( res ) {
		case EventRing::EVENT_SUCCESS:
			return true;
		case EventRing::EVENT_FAIL:
		case EventRing::EVENT_EXPIRED:
			task.GetEnv().time = 0;

			return false;
	}
}

void EventQueue::Rotate() {
	for ( EventRing& eventRing : eventRings ) {
		if ( eventRing.lock.LockWrite() ) {
			eventRing.Rotate();

			eventRing.lock.UnlockWrite();
		}
	}
}

void EventQueue::Shutdown() {
	if ( exiting.load( std::memory_order_relaxed ) ) {
		return;
	}

	uint32 count = 0;

	for ( EventRing& eventRing : eventRings ) {
		while ( !eventRing.lock.LockWrite() );

		for ( uint64 sector : eventRing.allocatedEvents ) {
			count += CountBits( sector );
		}

		memset( eventRing.allocatedEvents, 0, EventRing::sectors * sizeof( uint64 ) );

		eventRing.lock.UnlockWrite();
	}

	exiting.store( true, std::memory_order_relaxed );
}

EventQueue eventQueue;