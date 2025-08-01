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

static uint64_t TimeNanoseconds() {
	return std::chrono::duration_cast< std::chrono::nanoseconds >( Sys::SteadyClock::now().time_since_epoch() ).count();
}

Timer::Timer( uint64_t* newTimeVar ) :
	timeVar( newTimeVar ) {
}

// If newTimeVar is specified, it will be set to the Timer's runTime when the destructor is called
Timer::Timer( const bool start, uint64_t* newTimeVar ) :
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

std::string Timer::FormatTime( uint64_t time ) {
	const char* suf[] = { "ns", "us", "ms", "s" };

	int s = 0;
	// Formats as xxxx[suffix] as it's more informative than xxx[suffix]
	for ( s = 0; s < 4 && time >= 10000; s++ ) {
		time = time / 1000;
	}

	return Str::Format( "%u%s", time, suf[s] );
}

uint64_t Timer::Time() const {
	if ( running ) {
		return TimeNanoseconds() - time + runTime;
	}

	return runTime;
}

void Timer::Start() {
	if ( running ) {
		return;
	}

	time = TimeNanoseconds();
	running = true;
}

void Timer::Stop() {
	if ( !running ) {
		return;
	}

	runTime += TimeNanoseconds() - time;
	running = false;
}

void Timer::Clear() {
	runTime = 0;
	running = false;
}

uint64_t Timer::Restart() {
	Stop();

	const uint64_t diff = runTime;
	runTime = 0;
	running = true;

	return diff;
}

GlobalTimer::GlobalTimer( uint64_t* newTimeVar ) :
	Timer( false, newTimeVar ) {
}
