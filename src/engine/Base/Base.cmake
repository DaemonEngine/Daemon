set( engineBase ${ENGINE_DIR}/Base )

set( mathList
    ${engineBase}/Math/Bit.h
)

set( memoryList
    ${engineBase}/Memory/Allocator.cpp
    ${engineBase}/Memory/Allocator.h
    ${engineBase}/Memory/Array.h
    ${engineBase}/Memory/BitStream.cpp
    ${engineBase}/Memory/BitStream.h
    ${engineBase}/Memory/DynamicArray.h
    ${engineBase}/Memory/IteratorSeq.h
    ${engineBase}/Memory/Memory.cpp
    ${engineBase}/Memory/Memory.h
    ${engineBase}/Memory/MemoryChunk.h
    ${engineBase}/Memory/MemoryChunkSystem.cpp
    ${engineBase}/Memory/MemoryChunkSystem.h
    ${engineBase}/Memory/RingBuffer.h
    ${engineBase}/Memory/StackAllocator.cpp
    ${engineBase}/Memory/StackAllocator.h
    ${engineBase}/Memory/SysAllocator.cpp
    ${engineBase}/Memory/SysAllocator.h
)

set( srcDebugList
    ${engineBase}/SrcDebug/StackTrace.cpp
    ${engineBase}/SrcDebug/StackTrace.h
    ${engineBase}/SrcDebug/Tag.cpp
    ${engineBase}/SrcDebug/Tag.h
)

set( syncList
    ${engineBase}/Sync/AccessLock.cpp
    ${engineBase}/Sync/AccessLock.h
    ${engineBase}/Sync/AlignedAtomic.h
    ${engineBase}/Sync/Barrier.cpp
    ${engineBase}/Sync/Barrier.h
    ${engineBase}/Sync/Fence.cpp
    ${engineBase}/Sync/Fence.h
    ${engineBase}/Sync/SyncPoint.cpp
    ${engineBase}/Sync/SyncPoint.h
)

set( sysList
    ${engineBase}/Sys/CPUInfo.cpp
    ${engineBase}/Sys/CPUInfo.h
    ${engineBase}/Sys/MemoryInfo.cpp
    ${engineBase}/Sys/MemoryInfo.h
    ${engineBase}/Sys/OSLoad.cpp
    ${engineBase}/Sys/OSLoad.h
    ${engineBase}/Sys/OSThread.cpp
    ${engineBase}/Sys/OSThread.h
    ${engineBase}/Sys/PDH.cpp
    ${engineBase}/Sys/PDH.h
    ${engineBase}/Sys/Windows.h
)

set( threadList
    ${engineBase}/Thread/EventQueue.cpp
    ${engineBase}/Thread/EventQueue.h
    ${engineBase}/Thread/GlobalMemory.cpp
    ${engineBase}/Thread/GlobalMemory.h
    ${engineBase}/Thread/Task.cpp
    ${engineBase}/Thread/Task.h
    ${engineBase}/Thread/TaskData.h
    ${engineBase}/Thread/TaskList.cpp
    ${engineBase}/Thread/TaskList.h
    ${engineBase}/Thread/Thread.cpp
    ${engineBase}/Thread/Thread.h
    ${engineBase}/Thread/ThreadCommon.h
    ${engineBase}/Thread/ThreadMemory.cpp
    ${engineBase}/Thread/ThreadMemory.h
    ${engineBase}/Thread/ThreadUplink.cpp
    ${engineBase}/Thread/ThreadUplink.h
    ${engineBase}/Thread/TLMAllocator.cpp
    ${engineBase}/Thread/TLMAllocator.h
)

set( engineBaseList
    ${mathList}
    ${memoryList}
    ${srcDebugList}
    ${syncList}
    ${sysList}
    ${threadList}
    ${engineBase}/BaseCVars.cpp
    ${engineBase}/BaseCVars.h
    ${engineBase}/BaseDecls.h
    ${engineBase}/Error.cpp
    ${engineBase}/Error.h
    ${engineBase}/Int.h
    ${engineBase}/Parser.cpp
    ${engineBase}/Parser.h
    ${engineBase}/Timer.cpp
    ${engineBase}/Timer.h
    ${engineBase}/Version.cpp
    ${engineBase}/Version.h
)

set( engineBaseIncludeList
    ${engineBase}
    ${engineBase}/Math
    ${engineBase}/Memory
    ${engineBase}/Sync
)