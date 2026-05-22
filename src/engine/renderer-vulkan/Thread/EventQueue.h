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

#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "../Memory/Array.h"

#include "../Sync/AccessLock.h"

#include "Task.h"

struct EventRing {
	static constexpr uint32 sectors    = 8;
	static constexpr uint32 sectorMask = 7;
	static constexpr uint32 sectorSize = 64;

	enum EventResult {
		EVENT_SUCCESS,
		EVENT_FAIL,
		EVENT_EXPIRED
	};

	AccessLock lock;

	uint64     currentTime;
	uint64     granularity;

	uint32     currentSector;

	Task       events[sectors][sectorSize] {};
	uint64     allocatedEvents[sectors]    {};

	EventResult AddTask( Task& task );
	void        Rotate();
};

struct EventQueue {
	Array<EventRing, 10> eventRings {
		EventRing { .granularity = 1_us   },
		EventRing { .granularity = 8_us   },
		EventRing { .granularity = 64_us  },
		EventRing { .granularity = 512_us },
		EventRing { .granularity = 1_ms   },
		EventRing { .granularity = 4_ms   },
		EventRing { .granularity = 32_ms  },
		EventRing { .granularity = 256_ms },
		EventRing { .granularity = 2_s    },
		EventRing { .granularity = 1_m    }
	};

	const uint64      minGranularity = eventRings[0].granularity;

	std::atomic<bool> exiting        = false;

	void AddTask( Task& task );
	void Rotate();

	void Shutdown();
};

extern EventQueue eventQueue;

#endif // EVENT_QUEUE_H