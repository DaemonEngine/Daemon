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

#include "Thread/ThreadMemory.h"
#include "Bit.h"

#include "AccessLock.h"

bool AccessLock::Lock() {
	uint8 expected = value.load( std::memory_order_relaxed );
	uint8 desired;

	do {
		if( !expected ) {
			return false;
		}

		desired = expected + 1;
	} while( !value.compare_exchange_strong( expected, desired, std::memory_order_acquire ) );

	return true;
}

bool AccessLock::Unlock() {
	const bool noRefs = value.fetch_sub( 1, std::memory_order_release ) - 1;
	return !noRefs;
}

bool AccessLock::LockWrite() {
	const uint8 current = value.fetch_sub( 1, std::memory_order_relaxed );

	if ( current != 1 ) {
		UnlockWrite();

		return false;
	}

	return true;
}

void AccessLock::UnlockWrite() {
	value.fetch_add( 1, std::memory_order_release );
}

void AccessLock::operator=( const AccessLock& other ) {
	value = other.value.load( std::memory_order_relaxed );
}

bool AccessLockRecursive::Lock() {
	return lock.Lock();
}

bool AccessLockRecursive::LockWrite() {
	if ( BitSet( acquired, TLM.id ) ) {
		callDepth++;

		return true;
	}

	bool success = lock.LockWrite();

	if ( success ) {
		SetBit( &acquired, TLM.id );
		callDepth++;
	}

	return success;
}

bool AccessLockRecursive::Unlock() {
	return lock.Unlock();
}

void AccessLockRecursive::UnlockWrite() {
	callDepth--;

	if ( !callDepth ) {
		UnSetBit( &acquired, TLM.id );

		lock.UnlockWrite();
	}
}