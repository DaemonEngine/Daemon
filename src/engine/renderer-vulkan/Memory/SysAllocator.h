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
// SysAllocator.h

#ifndef SYS_ALLOCATOR_H
#define SYS_ALLOCATOR_H

#include <atomic>

#include "../Math/NumberTypes.h"

#include "Allocator.h"

class SysAllocator : public Allocator {
	public:
	SysAllocator() = default;
	~SysAllocator() = default;

	void Init();

	byte* Alloc( const uint64 size, const uint64 alignment ) override;
	void Free( byte* memory ) override;

	private:
	struct AllocationRecord {
		uint64 alignment;

		byte* memory;

		uint32 pageCount;
		uint8 id;

		char source[107];
	};

	static constexpr uint32 MAX_THREAD_ALLOCATIONS = 256;
	AllocationRecord allocations[MAX_THREAD_ALLOCATIONS];

	static constexpr uint32 MAX_THREAD_ALLOCATION_SYNC_VARS = MAX_THREAD_ALLOCATIONS / 64;
	static_assert( MAX_THREAD_ALLOCATION_SYNC_VARS * 64 == MAX_THREAD_ALLOCATIONS,
		"MAX_THREAD_ALLOCATIONS must be a multiple of 64" );
	std::atomic<uint64> availableAllocations[MAX_THREAD_ALLOCATION_SYNC_VARS];

	std::atomic<uint32> currentAllocations;
	std::atomic<uint32> currentAllocatedSize;
	std::atomic<uint32> currentAllocatedPages;

	#ifdef _MSC_VER
		uint32 allocationFlags;
		uint32 allocationProtection;
	#else
		int    allocationFlags;
		int    allocationProtection;
	#endif

	uint64 pageSize;
};

extern SysAllocator sysAllocator;

#endif // SYS_ALLOCATOR_H