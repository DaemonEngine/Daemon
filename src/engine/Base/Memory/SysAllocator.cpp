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
	#include <memoryapi.h>

	#pragma comment( lib, "mincore" )
#else
	#include <sys/mman.h>
	#include <linux/mman.h>
#endif

#include <thread>

#include "SrcDebug/Tag.h"
#include "Sys/MemoryInfo.h"
#include "Bit.h"
#include "BaseCVars.h"

#include "SysAllocator.h"

SysAllocator sysAllocator;

void SysAllocator::Init() {
	#ifdef _MSC_VER
		allocationFlags = MEM_RESERVE | MEM_COMMIT;
		switch ( r_vkMemoryPageSize.Get() ) {
			case PageSize::SIZE_64:
				allocationFlags |= MEM_64K_PAGES;
				pageSize         = memoryInfo.PAGE_SIZE_64;
				break;
			case PageSize::SIZE_LARGE:
				if ( memoryInfo.PAGE_SIZE_LARGE ) {
					allocationFlags |= MEM_LARGE_PAGES;
					pageSize         = memoryInfo.PAGE_SIZE_LARGE;
				} else {
					allocationFlags |= MEM_64K_PAGES;
					pageSize         = memoryInfo.PAGE_SIZE_64;
				}
				break;
			case PageSize::SIZE_DEFAULT:
			default:
				pageSize = memoryInfo.PAGE_SIZE_DEFAULT;
				break;
		}

		allocationProtection = PAGE_READWRITE;
	#else
		allocationFlags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE;
		switch ( r_vkMemoryPageSize.Get() ) {
			case PageSize::SIZE_64:
				Log::Notice( "64kb pages unsupported on Linux" );
				break;
			case PageSize::SIZE_LARGE:
				if ( memoryInfo.PAGE_SIZE_LARGE ) {
					allocationFlags |= MAP_HUGETLB | MAP_HUGE_2MB;
					pageSize         = memoryInfo.PAGE_SIZE_LARGE;
				}
				break;
			case PageSize::SIZE_DEFAULT:
			default:
				pageSize = memoryInfo.PAGE_SIZE_DEFAULT;
				break;
		}

		allocationProtection = PROT_WRITE | PROT_READ | PROT_EXEC;
	#endif
}

MALLOC_LIKE byte* SysAllocator::Alloc( const uint64 size, const uint64 alignment ) {
	bool   allocated      = false;
	uint32 allocationSync = 0;
	uint32 sync;

	while ( !allocated ) {
		uint64 expectedSync;
		uint64 desiredSync;

		allocated = true;

		do {
			expectedSync = availableAllocations[allocationSync].load( std::memory_order_relaxed );
			sync         = FindLZeroBit( expectedSync );

			if ( sync == 64 ) {
				allocated = false;
				break;
			}

			desiredSync = SetBit( expectedSync, sync );
		} while (
			!availableAllocations[allocationSync].compare_exchange_strong( expectedSync, desiredSync, std::memory_order_relaxed )
		);

		if ( allocated ) {
			break;
		}

		allocationSync = ( allocationSync + 1 ) % MAX_THREAD_ALLOCATION_SYNC_VARS;

		if ( allocationSync == 0 ) {
			std::this_thread::yield();
		}
	}

	const uint32 allocationID   = allocationSync * 64 + sync;
	const uint64 allocationSize = PAD( size, pageSize ) + 2 * pageSize;
	const uint32 pageCount      = allocationSize / pageSize;

	AllocationRecord& alloc = allocations[allocationID];

	alloc.pageCount = pageCount;
	alloc.alignment = alignment;
	alloc.id        = allocationID;

	#ifdef _MSC_VER
		alloc.memory = ( byte* ) VirtualAlloc2( nullptr, nullptr, allocationSize, allocationFlags, PAGE_NOACCESS, nullptr, 0 );
		
		unsigned long unused;
		VirtualProtect( alloc.memory,                                ( pageCount - 1 ) * pageSize, allocationProtection,       &unused );
		VirtualProtect( alloc.memory + ( pageCount - 1 ) * pageSize, pageSize,                     PAGE_READONLY | PAGE_GUARD, &unused );
	#else
		alloc.memory = ( byte* ) mmap( nullptr, allocationSize, allocationProtection, allocationFlags, -1, 0 );

		mprotect( alloc.memory,                                ( pageCount - 1 ) * pageSize, allocationProtection );
		mprotect( alloc.memory + ( pageCount - 1 ) * pageSize, pageSize,                     PROT_NONE );
	#endif

	memcpy( alloc.memory, &alloc, sizeof( AllocationRecord ) );

	#ifdef _MSC_VER
		VirtualProtect( alloc.memory, pageSize, PAGE_READONLY | PAGE_GUARD, &unused );
	#else
		mprotect( alloc.memory, pageSize, PROT_NONE );
	#endif

	currentAllocations.fetch_add(    1,              std::memory_order_relaxed );
	currentAllocatedSize.fetch_add(  allocationSize, std::memory_order_relaxed );
	currentAllocatedPages.fetch_add( pageCount,      std::memory_order_relaxed );

	return alloc.memory + pageSize;
}

void SysAllocator::Free( byte* memory ) {
	if ( !memory ) {
		Log::WarnTag( "memory is nullptr" );
		return;
	}

	#ifdef _MSC_VER
		unsigned long unused;
		VirtualProtect( memory - pageSize, pageSize, PAGE_READONLY, &unused );

		AllocationRecord& alloc = allocations[( ( AllocationRecord* ) ( memory - pageSize ) )->id];

		VirtualFree( alloc.memory, 0, MEM_RELEASE );
	#else
		AllocationRecord& alloc = allocations[( ( AllocationRecord* ) ( memory - pageSize ) )->id];
		munmap( alloc.memory, alloc.pageCount * pageSize );
	#endif

	availableAllocations[alloc.id / 64].fetch_sub( 1ull << ( alloc.id & 63 ), std::memory_order_relaxed );

	currentAllocations.fetch_sub(    1,                          std::memory_order_relaxed );
	currentAllocatedSize.fetch_sub(  alloc.pageCount * pageSize, std::memory_order_relaxed );
	currentAllocatedPages.fetch_sub( alloc.pageCount,            std::memory_order_relaxed );
}