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
	#include <intrin.h>

	#include "Windows.h"

	#include <powerbase.h>
	#include <Pdh.h>

	#include "PDH.h"
#elif __linux__
	#include <error.h>
	#include <sched.h>
	#include <string.h>
	#include <unistd.h>
#elifndef __APPLE__
	#include <pthread.h>
#endif

#include "Thread/Core.h"
#include "Thread/ThreadCommon.h"
#include "SysAllocator.h"
#include "Bit.h"
#include "DynamicArray.h"
#include "Timer.h"
#include "Parser.h"

#include "CPUInfo.h"

#include "OSThread.h"

#define OSErrRetFloat( value )\
	if ( value ) {\
		Log::Warn( "OS error: %u", value );\
		return 1.0f;\
	}

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

		bool ret = SetThreadGroupAffinity( id, &cpu, nullptr );

		if ( !ret ) {
			Log::Warn( "SetThreadGroupAffinity error: %i", GetLastError() );
			baseMaxFrequency = 1.0;

			return;
		}
	#elif __linux__
		cpu_set_t cpu;

		CPU_ZERO( &cpu );
		CPU_SET( core, &cpu );

		int ret = sched_setaffinity( id, sizeof( cpu_set_t ), &cpu );

		if ( ret == -1 ) {
			Log::Warn( "sched_setaffinity error: %s", strerror( errno ) );
		}
	#elifndef __APPLE__
        pthread_t thread = pthread_self();

		cpu_set_t cpu;

		CPU_ZERO( &cpu );
		CPU_SET( core, &cpu );

		int ret = pthread_setaffinity_np( thread, sizeof( cpu ), &cpu );

		if ( ret != 0 ) {
			Log::Warn( "pthread_setaffinity_np error: %s", strerror( errno ) );
		}
	#endif
}

static uint64 GetMinExecutionTime( const uint64 testCount ) {
	uint64       out = UINT64_MAX;

	volatile int tmp = 0;

	for ( uint32 i = 0; i < testCount; i++ ) {
		Timer t;

		__m128 x = _mm_setzero_ps();

		for ( uint32 j = 0; j < 100000; j++ ) {
			x = _mm_add_ps( x, _mm_set1_ps( 1.0f ) );
		}

		tmp += _mm_cvt_ss2si( x );

		out  = t.Time() < out ? t.Time() : out;
	}

	return out;
}

void OSThread::SetBaseMaxFrequency() {
	#ifdef _MSC_VER
		// PROCESSOR_POWER_INFORMATION, which is missing from windows headers
		struct CPUPowerInfo {
			ULONG Number;
			ULONG MaxMhz;
			ULONG CurrentMhz;
			ULONG MhzLimit;
			ULONG MaxIdleState;
			ULONG CurrentIdleState;
		};

		// Set at boot for each core
		CPUPowerInfo cpuInfo[MAX_THREADS];

		NTSTATUS ret2 = CallNtPowerInformation( ProcessorInformation, nullptr, 0, cpuInfo, CPU_CORES * sizeof( CPUPowerInfo ) );

		if ( ret2 ) {
			Log::WarnTagT( "CallNtPowerInformation error: %u", ret2 );
			baseMaxFrequency = 4600;
		} else {
			baseMaxFrequency = cpuInfo[core].MaxMhz;
		}
	#endif
}

// Scale to ~4.6 GHz
double OSThread::GetMaxFrequencyScale() {
	#ifdef _MSC_VER
		if ( !pdhAvailable ) {
			return GetMinExecutionTime( 1000 ) / 38500.0;
		}

		PDH_HQUERY   query;
		PDH_HCOUNTER counter;

		OSErrRetFloat( PdhOpenQueryAf( nullptr, 0, &query ) );
		OSErrRetFloat( PdhAddCounterAf( query,
			Str::Format( "\\Processor Information(%u,%u)\\%s Processor Performance", core >> 6, core & 63, "%" ).c_str(),
			0, &counter ) );
		OSErrRetFloat( PdhCollectQueryDataf( query ) );
		
		GetMinExecutionTime( 1000 );

		OSErrRetFloat( PdhCollectQueryDataf( query ) );

		PDH_FMT_COUNTERVALUE scale;
		OSErrRetFloat( PdhGetFormattedCounterValuef( counter, PDH_FMT_DOUBLE, nullptr, &scale ) );

		OSErrRetFloat( PdhCloseQueryf( query ) );

		return 460000 / ( baseMaxFrequency * scale.doubleValue );
	#elifndef __APPLE__
		return GetMinExecutionTime( 1000 ) / 38500.0;
	#else // Can't set thread affinity on apple, so no point in scaling by max frequency
		return 1.0;
	#endif
}

struct File {
	FILE*    file;
	uint32_t size;

	File( const std::string& path, const char* mode ) {
		file = fopen( path.c_str(), mode );

		if ( !file ) {
			printf( "Failed to open file: %s, mode: %s\n", path.c_str(), mode );
			printf( strerror( errno ) );
			exit( 1 );
		}

		fseek( file, 0, SEEK_END );
		size = ftell( file );

		fseek( file, 0, 0 );
	}

	File( const std::string& path, const std::string& path2, const char* mode ) {
		file = fopen( path.c_str(), mode );

		if ( !file ) {
			file = fopen( path2.c_str(), mode );

			if( !file ) {
				Log::Warn( "Failed to open file: %s, mode: %s\n", path.c_str(), mode );
				Log::Warn( strerror( errno ) );
			}
		}

		fseek( file, 0, SEEK_END );
		size = ftell( file );

		fseek( file, 0, 0 );
	}

	~File() {
		fclose( file );
	}

	std::string ReadAll() {
		std::string data;
		data.resize( size );
		size = fread( data.data(), sizeof( char ), size, file );

		data.resize( size );
		data.shrink_to_fit();

		return data;
	}
};

Core OSThread::GetCoreInfo() {
	#ifdef _MSC_VER
		DWORD size = 0;

		GetLogicalProcessorInformationEx( RelationProcessorCore, nullptr, &size );
		SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buf = ( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* ) sysAllocator.Alloc( size, 64 );
		bool                                     ret = GetLogicalProcessorInformationEx( RelationProcessorCore, buf, &size );

		if ( !ret ) {
			Log::Warn( "GetLogicalProcessorInformationEx error: %i", GetLastError() );

			return {
				.type              = CORE_UNKNOWN,
				.smt               = false,
				.maxFrequencyScale = GetMaxFrequencyScale()
			};
		}

		SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* coreInfo = ( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* ) ( ( ( byte* ) buf ) + core * buf->Size );

		return {
			.type              = coreInfo->Processor.EfficiencyClass ? CORE_PERFORMANCE : CORE_EFFICIENCY,
			.smt               = coreInfo->Processor.Flags == LTP_PC_SMT,
			.maxFrequencyScale = GetMaxFrequencyScale()
		};
	#elifdef __linux__
		File        cpuInfoFile { "/proc/cpuinfo", "r" };
		std::string cpuInfo   = cpuInfoFile.ReadAll();

		StringView  v { cpuInfo.c_str(), cpuInfo.size() };

		int         physicalCores = 0;
		int         logicalCores  = 0;
	
		do {
			StringView o = Parse( v );

			if ( o == "cpu" ) {
				if ( Parse( v ) == "cores" ) {
					Parse( v ); // :
					Str::ParseInt( physicalCores, std::string( o.memory, o.size ) );
				}
			} else if ( o == "siblings" ) {
				Parse( v ); // :
				Str::ParseInt( logicalCores, std::string( o.memory, o.size ) );
			}
		} while ( v.size && !( physicalCores && logicalCores ) );

		return {
			.type              = CORE_UNKNOWN,
			.smt               = physicalCores == logicalCores,
			.maxFrequencyScale = GetMaxFrequencyScale()
		};
	#else
		return {
			.type              = CORE_PERFORMANCE,
			.smt               = false,
			.maxFrequencyScale = GetMaxFrequencyScale()
		};
	#endif
}

std::string GetCPUModel() {
	#ifdef _MSC_VER
		int         info[4] {};
		std::string model;

		static constexpr uint32 infoSize = 4 * sizeof( int );

		model.resize( 65 );

		__cpuid( info, 0x80000002 );
		memcpy( model.data(),                info, infoSize );

		__cpuid( info, 0x80000003 );
		memcpy( model.data() +     infoSize, info, infoSize );

		__cpuid( info, 0x80000004 );
		memcpy( model.data() + 2 * infoSize, info, infoSize );

		model.resize( strlen( model.c_str() ) ); // Avoid the ugly NUL characters in logger

		return model;
	#elifdef __linux__
		File        cpuInfoFile { "/proc/cpuinfo", "r" };
		std::string cpuInfo   = cpuInfoFile.ReadAll();

		StringView  v { cpuInfo.c_str(), cpuInfo.size() };
	
		do {
			StringView o = Parse( v );

			if ( o == "model" ) {
				if ( Parse( v ) == "name" ) {
					Parse( v ); // :
					return Parse( v );
				}
			}
		} while ( v.size );

		return "Unknown";
	#else
		return "Unknown";
	#endif
}