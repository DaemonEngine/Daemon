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

#include "Timer.h"

#ifdef _MSC_VER
	#include <windows.h>

	#include "../Sys/CPUInfo.h"

	uint64 TimeNs() {
		LARGE_INTEGER time;
		QueryPerformanceCounter( &time );

		return time.QuadPart * CLOCK_PRECISION;
	}
#else
	#include <time.h>

	uint64 TimeNs() {
		timespec ts;
		clock_gettime( CLOCK_MONOTONIC_RAW, &ts );

		return ts.tv_nsec;
	}
#endif

uint64 NsToMs( const uint64 time ) {
	return time / 1000;
}

uint64 NsToUs( const uint64 time ) {
	return time / 1000000;
}

uint64 NsToS( const uint64 time ) {
	return time / 1000000000;
}

uint64 NsToM( const uint64 time ) {
	return time / ( 1000000000ull * 60 );
}

uint64 NsToH( const uint64 time ) {
	return time / ( 1000000000ull * 60 * 60 );
}

uint64 operator""_ns( const uint64Ext time ) {
	return time;
}

uint64 operator""_us( const uint64Ext time ) {
	return time * 1000;
}

uint64 operator""_ms( const uint64Ext time ) {
	return time * 1000000;
}

uint64 operator""_s(  const uint64Ext time ) {
	return time * 1000000000;
}

uint64 operator""_m(  const uint64Ext time ) {
	return time * 60 * 1000000000;
}

uint64 operator""_h(  const uint64Ext time ) {
	return time * 60 * 60 * 1000000000;
}

Timer::Timer( uint64* newTimeVar ) :
	timeVar( newTimeVar ) {
	Start();
}

// If newTimeVar is specified, it will be set to the Timer's runTime when the destructor is called
Timer::Timer( const bool start, uint64* newTimeVar ) :
	timeVar( newTimeVar ) {
	if ( start ) {
		Start();
	}
}

Timer::~Timer() {
	if ( timeVar ) {
		*timeVar += Time();
	}
}

std::string FormatTime( uint64 time, const TimeUnit maxTimeUnit ) {
	const char* suf[] = { "ns", "us", "ms", "s" };

	int s = 0;
	// Formats as xxxx[suffix] as it's more informative than xxx[suffix]
	for ( s = 0; s < maxTimeUnit && time >= 10000; s++ ) {
		time = time / 1000;
	}

	return Str::Format( "%u%s", time, suf[s] );
}

std::string Timer::FormatTime( const TimeUnit maxTimeUnit ) {
	return ::FormatTime( Time(), maxTimeUnit );
}

uint64 Timer::Time() const {
	if ( running ) {
		return TimeNs() - time + runTime;
	}

	return runTime;
}

void Timer::Start() {
	if ( running ) {
		return;
	}

	time    = TimeNs();
	running = true;
}

void Timer::Stop() {
	if ( !running ) {
		return;
	}

	runTime += TimeNs() - time;
	running  = false;
}

void Timer::Clear() {
	runTime = 0;
	running = false;
}

uint64 Timer::Restart() {
	Stop();
	const uint64 diff = runTime;
	Clear();
	Start();

	return diff;
}

GlobalTimer::GlobalTimer( uint64* newTimeVar ) :
	Timer( false, newTimeVar ) {
}