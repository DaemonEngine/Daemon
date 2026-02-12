/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2026 Daemon Developers
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
// EventQueue.cpp

#include "../Math/Bit.h"

#include "TaskList.h"

#include "EventQueue.h"

EventRing::EventResult EventRing::AddTask( Task& task, const uint32 ringID ) {
	uint64 targetTime = task.time;
	uint32 sector     = ( targetTime - currentTime ) / granularity;

	// We took too long to add this to the EventQueue, and the task needs to be executed already
	if ( sector >= sectors ) {
		return EVENT_EXPIRED;
	}

	sector            = ( ( sector > 0 ? sector - 1 : sector ) + currentSector ) & sectorMask;

	uint32 eventID    = FindLZeroBit( allocatedEvents[sector] );

	if ( eventID == 64 ) {
		return EVENT_FAIL;
	}

	SetBit( &task.eventMask, ringID );

	SetBit( &allocatedEvents[sector], eventID );
	events[sector][eventID] = task;

	return EVENT_SUCCESS;
}

void EventRing::Rotate() {
	uint32 count = 0;

	while ( ( count + 1 ) * granularity + currentTime <= TimeNs() && count < sectors ) {
		uint64* sector = &allocatedEvents[( currentSector + count ) & sectorMask];

		for ( uint32 eventID = FindLSB( *sector ); eventID != 64; eventID = FindLSB( *sector ) ) {
			taskList.AddTask( events[( currentSector + count ) & sectorMask][eventID] );
			UnSetBit( sector, eventID );
		}

		count++;
	}

	if ( count ) {
		currentTime   = TimeNs();
		currentSector = ( currentSector + count ) & sectorMask;
	}
}

void EventQueue::AddTask( Task& task ) {
	const uint64 time          = TimeNs();
	const uint64 delay         = time <= task.time ? task.time - time : 0;

	EventRing::EventResult res = EventRing::EVENT_FAIL;

	if ( delay ) {
		for ( EventRing& eventRing : eventRings ) {
			if ( !BitSet( task.eventMask, &eventRing - eventRings.memory )
				&& delay <= eventRing.granularity * EventRing::sectors ) {
				while ( !eventRing.lock.LockWrite() );

				res    = eventRing.AddTask( task, &eventRing - eventRings.memory );

				eventRing.lock.UnlockWrite();

				break;
			}
		}
	} else {
		taskList.AddTask( task, {} );

		return;
	}

	switch ( res ) {
		case EventRing::EVENT_SUCCESS:
			return;
		case EventRing::EVENT_FAIL:
			task.time = 0;
			taskList.AddTask( task, {} );

			return;
		case EventRing::EVENT_EXPIRED:
			taskList.AddTask( task, {} );

			return;
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
		if ( eventRing.lock.LockWrite() ) {
			for ( uint64 sector : eventRing.allocatedEvents ) {
				count += CountBits( sector );
			}

			memset( eventRing.allocatedEvents, 0, EventRing::sectors * sizeof( uint64 ) );

			eventRing.lock.UnlockWrite();
		}
	}

	exiting.store( true, std::memory_order_relaxed );
}

EventQueue eventQueue;