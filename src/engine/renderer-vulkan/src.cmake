set( mathList
    ${ENGINE_DIR}/renderer-vulkan/Math/Bit.h
    ${ENGINE_DIR}/renderer-vulkan/Math/NumberTypes.h
)

set( memoryList
    ${ENGINE_DIR}/renderer-vulkan/Memory/Allocator.cpp
    ${ENGINE_DIR}/renderer-vulkan/Memory/Allocator.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/Array.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/BitStream.cpp
    ${ENGINE_DIR}/renderer-vulkan/Memory/BitStream.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/DynamicArray.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/Memory.cpp
    ${ENGINE_DIR}/renderer-vulkan/Memory/Memory.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/MemoryChunk.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/MemoryChunkSystem.cpp
    ${ENGINE_DIR}/renderer-vulkan/Memory/MemoryChunkSystem.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/IteratorSeq.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/StackAllocator.cpp
    ${ENGINE_DIR}/renderer-vulkan/Memory/StackAllocator.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/SysAllocator.cpp
    ${ENGINE_DIR}/renderer-vulkan/Memory/SysAllocator.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/RingBuffer.h
)

set( syncList
    ${ENGINE_DIR}/renderer-vulkan/Sync/AccessLock.cpp
    ${ENGINE_DIR}/renderer-vulkan/Sync/AccessLock.h
    ${ENGINE_DIR}/renderer-vulkan/Sync/AlignedAtomic.h
    ${ENGINE_DIR}/renderer-vulkan/Sync/Barrier.cpp
    ${ENGINE_DIR}/renderer-vulkan/Sync/Barrier.h
    ${ENGINE_DIR}/renderer-vulkan/Sync/Fence.cpp
    ${ENGINE_DIR}/renderer-vulkan/Sync/Fence.h
    ${ENGINE_DIR}/renderer-vulkan/Sync/SyncPoint.cpp
    ${ENGINE_DIR}/renderer-vulkan/Sync/SyncPoint.h
)

set( sysList
    ${ENGINE_DIR}/renderer-vulkan/Sys/CPUInfo.cpp
    ${ENGINE_DIR}/renderer-vulkan/Sys/CPUInfo.h
    ${ENGINE_DIR}/renderer-vulkan/Sys/MemoryInfo.cpp
    ${ENGINE_DIR}/renderer-vulkan/Sys/MemoryInfo.h
)

set( threadList
    ${ENGINE_DIR}/renderer-vulkan/Thread/EventQueue.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/EventQueue.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/GlobalMemory.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/GlobalMemory.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/Task.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/Task.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/TaskData.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/TaskData.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/TaskList.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/TaskList.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/TLMAllocator.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/TLMAllocator.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/Thread.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/Thread.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadCommon.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadMemory.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadMemory.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadUplink.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadUplink.h
)

set( utilsList
    ${ENGINE_DIR}/renderer-vulkan/SrcDebug/Tag.cpp
    ${ENGINE_DIR}/renderer-vulkan/SrcDebug/Tag.h
)

set( RENDERERLIST
    ${mathList}
    ${memoryList}
    ${syncList}
    ${sysList}
    ${threadList}
    ${utilsList}
    ${ENGINE_DIR}/renderer-vulkan/DispatchRawData.cpp
    ${ENGINE_DIR}/renderer-vulkan/DispatchRawData.h
    ${ENGINE_DIR}/renderer-vulkan/Error.cpp
    ${ENGINE_DIR}/renderer-vulkan/Error.h
    ${ENGINE_DIR}/renderer-vulkan/MiscCVarStore.cpp
    ${ENGINE_DIR}/renderer-vulkan/MiscCVarStore.h
    ${ENGINE_DIR}/renderer-vulkan/Version.cpp
    ${ENGINE_DIR}/renderer-vulkan/Version.h
    ${ENGINE_DIR}/renderer-vulkan/Shared/Timer.cpp
    ${ENGINE_DIR}/renderer-vulkan/Shared/Timer.h
)