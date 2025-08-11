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
// SyncPoint.h

#ifndef SYNC_POINT_H
#define SYNC_POINT_H

#include <atomic>

#include "../Math/NumberTypes.h"

#include "Fence.h"

struct SyncPoint {
	using SyncFunction = void( * )( void* );

	SyncFunction Execute;
	void* data;

	FenceMain fence;
	Fence done;
	std::atomic<bool> active = false;

	void Access();

	template<typename FunctionType, typename DataType>
	void Sync( FunctionType func, DataType newData, const uint64 syncCount ) {
		if ( active.load( std::memory_order_relaxed ) ) {
			// TODO: error
		}

		Execute = ( SyncFunction ) func;
		data = ( void* ) newData;
		fence.target = fence.value.load( std::memory_order_relaxed ) + syncCount;

		active.store( true, std::memory_order_release );
	}

	template<typename FunctionType, typename DataType>
	void Sync( FunctionType func, DataType newData, const uint64 syncCount, Fence& newDone ) {
		if ( active.load( std::memory_order_relaxed ) ) {
			// TODO: error
		}

		Execute = ( SyncFunction ) func;
		data = ( void* ) newData;
		fence.target = fence.value.load( std::memory_order_relaxed ) + syncCount;
		done = newDone;

		active.store( true, std::memory_order_release );
	}
};

#endif // SYNC_POINT_H