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

#ifndef TASK_H
#define TASK_H

#include <atomic>
#include <type_traits>

#include "Sys/MemoryInfo.h"
#include "Int.h"
#include "Bit.h"
#include "Memory.h"
#include "AccessLock.h"
#include "Fence.h"
#include "Timer.h"

#include "TaskData.h"

/* template<typename T>
struct IsPointer_ {
	static constexpr bool out = false;
};

template<typename T>
struct IsPointer_<T*> {
	static constexpr bool out = true;
};

template<typename T>
constexpr bool IsPointer = IsPointer_<T>::out; */

using DestructorFunction = void( * )( const void* );

template<typename T>
constexpr DestructorFunction GetDestructor() {
	if constexpr ( std::is_trivially_destructible_v<T> ) {
		return nullptr;
	}

	return ( DestructorFunction )[]( const void* memory ) {
		static_cast<const T*>( memory )->~T();
	};
}

template<typename T>
bool HasDestructor() {
	return GetDestructor<T>() != nullptr;
}

struct Arg {
	uint8  id;
	bool   hasDestructor;
	uint16 size;
	uint16 alignment;

	Arg() :
		id( 0 ),
		hasDestructor( false ),
		size( 0 ),
		alignment( 0 ) {
	}

	Arg( const Arg& other ) :
		id( other.id ),
		hasDestructor( other.hasDestructor ),
		size( other.size ),
		alignment( other.alignment ) {
	}

	Arg( const uint16 newID, const bool newHasDestructor, const uint16 newSize, const uint16 newAlignment ) :
		id( newID ),
		hasDestructor( newHasDestructor ),
		size( newSize ),
		alignment( newAlignment ) {
	}
};

using TaskFunction  = void( * )( void* );
using TaskFunction2 = void( * )( void*, void* );
using TaskFunction3 = void( * )( void*, void*, void* );
using TaskFunction4 = void( * )( void*, void*, void*, void* );
using TaskFunction5 = void( * )( void*, void*, void*, void*, void* );
using TaskFunction6 = void( * )( void*, void*, void*, void*, void*, void* );
using TaskFunction7 = void( * )( void*, void*, void*, void*, void*, void*, void* );
using TaskFunction8 = void( * )( void*, void*, void*, void*, void*, void*, void*, void* );

#define argsMsg "Tasks must have the same amount of args as the underlying function;" \
	" each function arg must be a pointer to the corresponding task arg type"

#define argSizeMsg "Task arg size must not exceed Task::maxArgSize"
//char[] = "ALongTaskNameToBeSure?";

struct Task {
	static constexpr uint32 maxArgCount      = 8;
	static constexpr uint32 maxArgSize       = 3072;
	static constexpr uint32 maxTotalArgSize  = maxArgCount * maxArgSize;

	TaskFunction       Execute;

	uint16             dataOffsets[8]           { 0 };
	// 40 bits for task data so it supports up to ~207 days with 1024 tasks with 64 byte args per frame on average @ 60 FPS
	uint32             dataOffset             = 0;
	uint8              dataOffset2            = 0;

	uint8              flags                  = 0;

	static constexpr uint16 UNALLOCATED       = UINT16_MAX;
	uint16             bufferID               = UNALLOCATED; // Task RingBuffer id
	uint32             gen                    = 0;

	uint32             argsMap                = 0; // Bits 0-7: destructor map, 8-31: arg id -> dataOffsets remap

	uint64             time                   = 0;
	uint64             threadMask             = 0;

	uint8              id                     = 0; // Task memory/dependency tracking in TaskList
	std::atomic<uint8> dependencyCounter      = 1; // Starts at 1 so it wouldn't start executing before being resolved in AddTask[s]
	uint8              forwardTaskCounter     = 0;

	FenceBool          complete;
	AccessLock         threadCount            = {};

	uint16             srcLine                = 0;

	static constexpr uint32 maxForwardTasks   = 8;
	uint16 forwardTasks[maxForwardTasks]        { 0 };

	static constexpr uint32 maxSrcSize  = 24;
	static constexpr uint32 maxNameSize = 24;

	char               src[maxSrcSize];
	char               name[maxNameSize];

	                Task();
	                Task( const Task& other );

	const     Task& operator*();
	const     Task* operator->() const;
	constexpr Task& GetTask();

	void            operator=( const Task& other );

	template<typename FuncType>
	Task( FuncType func ) :
		Execute( ( TaskFunction ) func ) {
		SetValid( true );
	}

	template<typename FuncType, typename DataType>
	Task( FuncType func, const DataType& newData ) :
		Execute( ( TaskFunction ) func ) {

		static_assert( std::is_same_v<FuncType, void( * )( DataType* )>, argsMsg );
		static_assert( sizeof( DataType ) <= maxArgSize, argSizeMsg );

		Arg args[] {
			Arg {
				0,
				HasDestructor<DataType>(),
				sizeof( DataType ),
				alignof( DataType )
			}
		};

		CopyArgs( InitMemory( args, args + 1 ), newData );
	}

	template<typename FuncType, typename DataType, typename DataType2>
	Task( FuncType func, const DataType& newData, const DataType2& newData2 ) :
		Execute( ( TaskFunction ) func ) {

		static_assert( std::is_same_v<FuncType, void( * )( DataType*, DataType2* )>, argsMsg );
		static_assert( sizeof( DataType )  <= maxArgSize, argSizeMsg );
		static_assert( sizeof( DataType2 ) <= maxArgSize, argSizeMsg );

		Arg args[] {
			Arg {
				0,
				HasDestructor<DataType>(),
				sizeof( DataType ),
				alignof( DataType )
			},
			Arg {
				1,
				HasDestructor<DataType2>(),
				sizeof( DataType2 ),
				alignof( DataType2 )
			}
		};

		CopyArgs( InitMemory( args, args + 2 ), newData, newData2 );
	}

	template<typename FuncType, typename DataType, typename DataType2, typename DataType3>
	Task( FuncType func, const DataType& newData, const DataType2& newData2, const DataType3& newData3 ) :
		Execute( ( TaskFunction ) func ) {

		static_assert( std::is_same_v<FuncType, void( * )( DataType*, DataType2*, DataType3* )>, argsMsg );
		static_assert( sizeof( DataType )  <= maxArgSize, argSizeMsg );
		static_assert( sizeof( DataType2 ) <= maxArgSize, argSizeMsg );
		static_assert( sizeof( DataType3 ) <= maxArgSize, argSizeMsg );

		Arg args[] {
			Arg {
				0,
				HasDestructor<DataType>(),
				sizeof( DataType ),
				alignof( DataType )
			},
			Arg {
				1,
				HasDestructor<DataType2>(),
				sizeof( DataType2 ),
				alignof( DataType2 )
			},
			Arg {
				2,
				HasDestructor<DataType3>(),
				sizeof( DataType3 ),
				alignof( DataType3 )
			}
		};

		CopyArgs( InitMemory( args, args + 3 ), newData, newData2, newData3 );
	}

	template<typename FuncType, typename DataType, typename DataType2, typename DataType3, typename DataType4>
	Task( FuncType func, const DataType& newData, const DataType2& newData2, const DataType3& newData3, const DataType4& newData4 ) :
		Execute( ( TaskFunction ) func ) {

		static_assert( std::is_same_v<FuncType, void( * )( DataType*, DataType2*, DataType3*, DataType4* )>, argsMsg );
		static_assert( sizeof( DataType )  <= maxArgSize, argSizeMsg );
		static_assert( sizeof( DataType2 ) <= maxArgSize, argSizeMsg );
		static_assert( sizeof( DataType3 ) <= maxArgSize, argSizeMsg );
		static_assert( sizeof( DataType4 ) <= maxArgSize, argSizeMsg );

		Arg args[] {
			Arg {
				0,
				HasDestructor<DataType>(),
				sizeof( DataType ),
				alignof( DataType )
			},
			Arg {
				1,
				HasDestructor<DataType2>(),
				sizeof( DataType2 ),
				alignof( DataType2 )
			},
			Arg {
				2,
				HasDestructor<DataType3>(),
				sizeof( DataType3 ),
				alignof( DataType3 )
			},
			Arg {
				3,
				HasDestructor<DataType4>(),
				sizeof( DataType4 ),
				alignof( DataType4 )
			}
		};

		CopyArgs( InitMemory( args, args + 4 ), newData, newData2, newData3, newData4 );
	}

	Task&  Delay( const uint64 delay );

	Task&  ThreadMask( const uint64 newThreadMask );
	Task&  ThreadMaskAll();
	Task&  ThreadMaskAllOthers();
	Task&  ThreadMaskCurrent();

	void   Wait();

	void   ExecuteDestructors();

	bool   IsValid();
	bool   IsShutdownTask();
	bool   IsActive();
	uint8  GetArgCount();

	uint64 GetDataOffset();

	void   SetActive( const bool active );

	byte*  GetArgMemory( const uint32 arg );

	private:
	static constexpr uint32 validOffset     = 0;
	static constexpr uint32 activeOffset    = 1;
	static constexpr uint32 shutdownOffset  = 2;
	static constexpr uint32 argCountOffset  = 3;

	static constexpr uint32 argMapArgOffset = 8;
	static constexpr uint32 argMapMask      = 255;
	static constexpr uint32 argMapArgSize   = 3;

	uint32 SetArgsMap( Arg* start, Arg* end );
	uint32 RemapArg( const uint32 arg );

	byte*  InitMemory( Arg* start, Arg* end );

	void   SetValid( const bool valid );

	template<class ... T>
	void   CopyArgs( byte* memory, const T& ... args ) {
		( [&] {
			DestructorFunction destructor = GetDestructor<T>();

			if ( destructor ) {
				memory = CopyAligned( memory, destructor );
			}
		  }(), ...
		);

		uint32 i = 0;

		( [&] {
			CopyAligned( memory + dataOffsets[RemapArg( i )], args );

			dataOffsets[RemapArg( i )] += CountBits( argsMap & argMapMask ) * sizeof( DestructorFunction );
			i++;
		  }(), ...
		);
	}
};

struct TaskProxy {
	Task& task;

	                TaskProxy( Task& newTask );
	Task*           operator->() const;

	constexpr Task& GetTask() const {
		return task;
	};
};

#endif // TASK_H