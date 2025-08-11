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
// AccessLock.cpp

#include "AccessLock.h"

bool AccessLock::Lock() {
	uint32 expected = value.load( std::memory_order_relaxed );
	uint32 desired;
	do {
		if( !expected ) {
			return false;
		}

		desired = expected + 1;
	} while( !value.compare_exchange_strong( expected, desired, std::memory_order_acq_rel ) );

	return true;
}

void AccessLock::Unlock() {
	value.fetch_sub( 1, std::memory_order_release );
}

bool AccessLock::LockWrite() {
	const uint32 current = value.fetch_sub( 1, std::memory_order_release );

	// TODO: error; only 1 writer is allowed
	if ( current == 0 ) {
		UnlockWrite();
		return false;
	}

	while( value.load( std::memory_order_acquire ) );

	return true;
}

void AccessLock::UnlockWrite() {
	value.fetch_add( 1, std::memory_order_release );
}

void AccessLock::operator=( const AccessLock& other ) {
	value = other.value.load( std::memory_order_acquire );
}