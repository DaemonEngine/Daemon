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

#include <initializer_list>

#include "Sys/Type.h"
#include "Int.h"
#include "Bit.h"
#include "BaseDecls.h"
#include "IteratorSeq.h"
#include "Memory.h"

using DestructorFunction = void( * )( const void* );

template<typename T>
constexpr DestructorFunction GetDestructor() {
	if constexpr ( triviallyDestructible<T> ) {
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

using TaskFunction  = void( * )( void* );
using TaskFunction2 = void( * )( void*, void* );
using TaskFunction3 = void( * )( void*, void*, void* );
using TaskFunction4 = void( * )( void*, void*, void*, void* );

#define argsMsg "Tasks must have the same amount of args as the underlying function;" \
	" each function arg must be a pointer to the corresponding task arg type"

#define argSizeMsg  "Task arg 1 size must not exceed Task::maxArgSize"
#define arg2SizeMsg "Task arg 2 size must not exceed Task::maxArgSize"
#define arg3SizeMsg "Task arg 3 size must not exceed Task::maxArgSize"
#define arg4SizeMsg "Task arg 4 size must not exceed Task::maxArgSize"

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

struct TaskProxy;

struct Task {
	friend struct TaskProxy;
	friend class  TaskList;
	friend struct EventQueue;
	friend class  Thread;

	using TaskFunction = void( * )( void* );
	
	static constexpr uint32 maxArgCount      = 4;
	static constexpr uint32 maxArgSize       = 3072;
	static constexpr uint32 maxTotalArgSize  = maxArgCount * maxArgSize;

	      Task() {};
		  Task( const Task& other ) = delete;
		  Task( const TaskProxy& other );
	      Task( Task&& other );
	      ~Task();

	void  operator=( Task&& other );

	bool  operator==( const Task& other );
	bool  operator!=( const Task& other );

	Task& Delay( const uint64 delay );

	Task& ThreadMask( const uint64 newThreadMask );
	Task& ThreadMaskAll();
	Task& ThreadMaskAllOthers();
	Task& ThreadMaskCurrent();

	void  Wait();

	private:
	struct ArgOffsets {
		byte*  memory;
		uint64 dataOffsets;
	};

	public:
	template<typename FuncType>
	Task( FuncType func ) {
		InitMemory( nullptr, nullptr, ( TaskFunction ) func );
	}

	template<typename FuncType, typename DataType>
	Task( FuncType func, const DataType& newData ) {

		static_assert( isSame<FuncType, void( * )( DataType* )>, argsMsg );
		static_assert( sizeof( DataType ) <= maxArgSize, argSizeMsg );

		Arg args[] {
			Arg {
				0,
				HasDestructor<DataType>(),
				sizeof( DataType ),
				alignof( DataType )
			}
		};

		CopyArgs( InitMemory( args, args + 1, ( TaskFunction ) func ), newData );
	}

	template<typename FuncType, typename DataType, typename DataType2>
	Task( FuncType func, const DataType& newData, const DataType2& newData2 ) {

		static_assert( isSame<FuncType, void( * )( DataType*, DataType2* )>, argsMsg );
		static_assert( sizeof( DataType )  <= maxArgSize, argSizeMsg  );
		static_assert( sizeof( DataType2 ) <= maxArgSize, arg2SizeMsg );

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

		CopyArgs( InitMemory( args, args + 2, ( TaskFunction ) func ), newData, newData2 );
	}

	template<typename FuncType, typename DataType, typename DataType2, typename DataType3>
	Task( FuncType func, const DataType& newData, const DataType2& newData2, const DataType3& newData3 ) {

		static_assert( isSame<FuncType, void( * )( DataType*, DataType2*, DataType3* )>, argsMsg );
		static_assert( sizeof( DataType )  <= maxArgSize, argSizeMsg  );
		static_assert( sizeof( DataType2 ) <= maxArgSize, arg2SizeMsg );
		static_assert( sizeof( DataType3 ) <= maxArgSize, arg3SizeMsg );

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

		CopyArgs( InitMemory( args, args + 3, ( TaskFunction ) func ), newData, newData2, newData3 );
	}

	template<typename FuncType, typename DataType, typename DataType2, typename DataType3, typename DataType4>
	Task( FuncType func, const DataType& newData, const DataType2& newData2, const DataType3& newData3, const DataType4& newData4 ) {

		static_assert( isSame<FuncType, void( * )( DataType*, DataType2*, DataType3*, DataType4* )>, argsMsg );
		static_assert( sizeof( DataType )  <= maxArgSize, argSizeMsg  );
		static_assert( sizeof( DataType2 ) <= maxArgSize, arg2SizeMsg );
		static_assert( sizeof( DataType3 ) <= maxArgSize, arg3SizeMsg );
		static_assert( sizeof( DataType4 ) <= maxArgSize, arg4SizeMsg );

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

		CopyArgs( InitMemory( args, args + 4, ( TaskFunction ) func ), newData, newData2, newData3, newData4 );
	}

	private:
	static constexpr uint32 offsetAdded         = 0;
	static constexpr uint32 offsetAllocated     = 1;
	static constexpr uint32 offsetProcessed     = 2;
	static constexpr uint32 offsetProcessedDeps = 3;

	uint16 bufferID = 0;
	uint16 flags    = 0;

	TaskEnv&   GetEnv() const;

	ArgOffsets InitMemory( Arg* start, Arg* end, TaskFunction execute );

	template<class ... T>
	void      CopyArgs( ArgOffsets&& argOffsets, const T& ... args ) {
		( [&] {
			DestructorFunction destructor = GetDestructor<T>();

			if ( destructor ) {
				CopyAligned( argOffsets.memory, destructor );
			}
		  }(), ...
		);

		uint32 i = 0;

		( [&] {
			uint16 offset = GetBits( argOffsets.dataOffsets, i * 16, 16 );

			CopyAligned( argOffsets.memory + offset, args );

			i++;
		  }(), ...
		);
	}
};

struct TaskProxy {
	Task* task;

	         TaskProxy( Task& newTask );
			 TaskProxy( Task&& newTask );
	TaskEnv& GetEnv()     const;
};

struct TaskInitList {
	const TaskProxy* taskStart;
	const TaskProxy* taskEnd;

	TaskInitList();
	TaskInitList( const TaskProxy* newStart, const TaskProxy* newEnd );
	TaskInitList( std::initializer_list<TaskProxy> list );

	constexpr IteratorSeq<const TaskProxy> begin() const {
		return IteratorSeq<const TaskProxy> { taskStart };
	}

	constexpr IteratorSeq<const TaskProxy> end()   const {
		return IteratorSeq<const TaskProxy> { taskEnd };
	}
};

void AddTasksExt( std::initializer_list<TaskInitList> dependencies );

#define AddTasks( ... ) AddTasksExt( { __VA_ARGS__ } )

#endif // TASK_H