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
// Timer.cpp

#include <chrono>

#include "common/Common.h"

#include "Timer.h"

uint64 operator""_ns( uint64 time ) {
	return time;
}

uint64 operator""_us( uint64 time ) {
	return time * 1000;
}

uint64 operator""_ms( uint64 time ) {
	return time * 1000000;
}

uint64 operator""_s( uint64 time ) {
	return time * 1000000000;
}

uint64 operator""_m( uint64 time ) {
	return time * 60 * 1000000000;
}

uint64 operator""_h( uint64 time ) {
	return time * 60 * 60 * 1000000000;
}

uint64 Time() {
	return std::chrono::duration_cast< std::chrono::nanoseconds >( Sys::SteadyClock::now().time_since_epoch() ).count();
}

Timer::Timer( uint64* newTimeVar ) :
	timeVar( newTimeVar ) {
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
		*timeVar = Time();
	}
}

std::string Timer::FormatTime( uint64 time, const TimeUnit maxTimeUnit ) {
	const char* suf[] = { "ns", "us", "ms", "s" };

	int s = 0;
	// Formats as xxxx[suffix] as it's more informative than xxx[suffix]
	for ( s = 0; s < maxTimeUnit && time >= 10000; s++ ) {
		time = time / 1000;
	}

	return Str::Format( "%u%s", time, suf[s] );
}

std::string Timer::FormatTime( const TimeUnit maxTimeUnit ) {
	return FormatTime( Time(), maxTimeUnit );
}

uint64 Timer::Time() const {
	if ( running ) {
		return Time() - time + runTime;
	}

	return runTime;
}

void Timer::Start() {
	if ( running ) {
		return;
	}

	time = Time();
	running = true;
}

void Timer::Stop() {
	if ( !running ) {
		return;
	}

	runTime += Time() - time;
	running = false;
}

void Timer::Clear() {
	runTime = 0;
	running = false;
}

uint64 Timer::Restart() {
	Stop();
	const uint64 diff = runTime;
	Start();

	return diff;
}

GlobalTimer::GlobalTimer( uint64* newTimeVar ) :
	Timer( false, newTimeVar ) {
}
