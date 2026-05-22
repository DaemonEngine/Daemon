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

#ifdef _MSC_VER
	#include <windows.h>
	#include <powerbase.h>
#elif __linux__
	#include <error.h>
	#include <sched.h>
	#include <string.h>
	#include <unistd.h>
#endif

#include "common/Common.h"

#include "../Math/Bit.h"
#include "../Shared/Timer.h"
#include "CPUInfo.h"
#include "../Thread/ThreadCommon.h"

#include "../Memory/DynamicArray.h"

#include "OSThread.h"

void OSThread::Init() {
	#ifdef _MSC_VER
		id = GetCurrentThread();
	#elif __linux__
		id = gettid();
	#else
		id = 0;
	#endif
}

void OSThread::SetAffinity( const uint32 newCore ) {
	core = newCore;

	#ifdef _MSC_VER
		GROUP_AFFINITY cpu {
			.Mask  = SetBit( 0ull, core & 63 ),
			.Group = ( uint16 ) ( core >> 6 )
		};

		uint32 ret = SetThreadGroupAffinity( id, &cpu, nullptr );

		if ( !ret ) {
			Log::Warn( "SetThreadGroupAffinity error: %i", GetLastError() );
		}
	#elif __linux__
		cpu_set_t cpu;

		CPU_ZERO( &cpu );
		CPU_SET( core, &cpu );

		int ret = sched_setaffinity( id, sizeof( cpu_set_t ), &cpu );

		if ( ret == -1 ) {
			Log::Warn( "sched_setaffinity error: %s", strerror( errno ) );
		}
	#endif
}
