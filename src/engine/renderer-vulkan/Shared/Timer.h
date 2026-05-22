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

#ifndef TIMER_H
#define TIMER_H

#include <string>

#include "../Math/NumberTypes.h"

uint64 NsToMs( const uint64 time );
uint64 NsToUs( const uint64 time );
uint64 NsToS( const uint64 time );
uint64 NsToM( const uint64 time );
uint64 NsToH( const uint64 time );

#ifdef _MSC_VER
	using uint64Ext = uint64;
#else
	using uint64Ext = unsigned long long; // clang/gcc bullshit - can't use fixed-width types in literal operators
#endif

uint64 operator ""_ns( const uint64Ext time );
uint64 operator ""_us( const uint64Ext time );
uint64 operator ""_ms( const uint64Ext time );
uint64 operator ""_s(  const uint64Ext time );
uint64 operator ""_m(  const uint64Ext time );
uint64 operator ""_h(  const uint64Ext time );

enum TimeUnit {
	ns,
	us,
	ms,
	s
};

uint64      TimeNs();
std::string FormatTime( uint64 time, const TimeUnit maxTimeUnit = s );

class Timer {
	public:
	Timer( uint64* newTimeVar );
	Timer( const bool start = true, uint64* newTimeVar = nullptr );
	~Timer();

	std::string FormatTime( const TimeUnit maxTimeUnit = s );

	uint64      Time() const;
	void        Start();
	void        Stop();
	void        Clear();
	uint64      Restart();

	private:
	uint64* timeVar;

	bool    running = false;
	uint64  time;
	uint64  runTime = 0;
};

class GlobalTimer :
	public Timer {
	public:

	GlobalTimer( uint64* newTimeVar = nullptr );
};

#endif // TIMER_H