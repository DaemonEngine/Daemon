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
// Fence.cpp

#include "Fence.h"

void FenceMain::Signal() {
	const uint64 current = value.fetch_add( 1, std::memory_order_relaxed ) + 1;

	if ( current >= target ) {
		done.store( true, std::memory_order_relaxed );
		done.notify_all();
	}
}

void FenceMain::Wait( const std::memory_order order ) {
	done.wait( false, order );
}

Fence FenceMain::Target( const uint64 target ) {
	Fence fence { *this };
	fence.target = target;
	return fence;
}

void FenceMain::operator=( const FenceMain& other ) {
	value  = other.value.load( std::memory_order_relaxed );
	done   = other.done.load( std::memory_order_relaxed );
	target = other.target;
}

Fence::Fence() :
	value( nullptr ),
	done( nullptr ) {
}

Fence::Fence( const Fence& other ) :
	value( other.value ),
	done( other.done ),
	target( other.target ) {
}

Fence::Fence( Fence&& other ) :
	value( other.value ),
	done( other.done ),
	target( other.target ) {
}

Fence::Fence( FenceMain& other ) :
	value( &other.value ),
	done( &other.done ),
	target( other.target ) {
}

void Fence::operator=( const Fence& other ) {
	value = other.value;
	done = other.done;
	target = other.target;
}

void Fence::Signal() {
	if ( !value ) {
		return;
	}

	const uint64 current = value->fetch_add( 1, std::memory_order_relaxed ) + 1;

	if ( current >= target ) {
		done->store( true, std::memory_order_relaxed );
		done->notify_all();
	}
}

void Fence::Wait( const std::memory_order order ) {
	if ( !value ) {
		return;
	}

	done->wait( false, order );
}
