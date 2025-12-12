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
    ${ENGINE_DIR}/renderer-vulkan/Thread/GlobalMemory.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/GlobalMemory.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/SyncTask.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/SyncTask.h
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
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadCommand.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadCommand.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadCommon.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadMemory.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadMemory.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadUplink.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadUplink.h
)

set( utilsList
    ${ENGINE_DIR}/renderer-vulkan/SrcDebug/LogExtend.h
    ${ENGINE_DIR}/renderer-vulkan/SrcDebug/Tag.cpp
    ${ENGINE_DIR}/renderer-vulkan/SrcDebug/Tag.h
)

# Graphics Core
set( graphicsCoreMemory
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Memory/CoreThreadMemory.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Memory/CoreThreadMemory.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Memory/DescriptorSet.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Memory/DescriptorSet.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Memory/EngineAllocator.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Memory/EngineAllocator.h
)

set( graphicsCoreList
    ${graphicsCoreMemory}
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/ExecutionGraph/ExecutionGraph.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/ExecutionGraph/ExecutionGraph.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Decls.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/CapabilityPack.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/CapabilityPack.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/EngineConfig.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/EngineConfig.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/GraphicsCoreCVars.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/GraphicsCoreCVars.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/GraphicsCoreStore.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/GraphicsCoreStore.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/GraphicsResource.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/GraphicsResource.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Init.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Init.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Instance.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Instance.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/PhysicalDevice.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/PhysicalDevice.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Queue.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Queue.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/QueuesConfig.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/QueuesConfig.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/ResultCheck.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/ResultCheck.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/SwapChain.cpp
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/SwapChain.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsCore/Vulkan.h
)

# Graphics Engine
set( graphicsEngineListH
    Common.glsl
)

set( graphicsEngineList
    MsgStream.glsl
    TestV.glsl
    TestF.glsl
)

set( graphicsEngineIDEList ${graphicsEngineList} )
list( APPEND graphicsEngineIDEList ${graphicsEngineListH} )
list( TRANSFORM graphicsEngineIDEList PREPEND ${ENGINE_DIR}/renderer-vulkan/GraphicsEngine/ )

# Graphics Shared
set( graphicsSharedList
    ${ENGINE_DIR}/renderer-vulkan/GraphicsShared/Bindings.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsShared/MemoryPool.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsShared/MsgStreamAPI.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsShared/NumberTypes.h
    ${ENGINE_DIR}/renderer-vulkan/GraphicsShared/PushLayout.h
)

# Vulkan Loader
set( vulkanLoaderList
    ${ENGINE_DIR}/renderer-vulkan/VulkanLoader/Vulkan.cpp
    ${ENGINE_DIR}/renderer-vulkan/VulkanLoader/VulkanLoadFunctions.cpp
    ${ENGINE_DIR}/renderer-vulkan/VulkanLoader/VulkanLoadFunctions.h
    ${ENGINE_DIR}/renderer-vulkan/VulkanLoader/Vulkan.h
)

set( graphicsList
    ${graphicsCoreList}
    ${graphicsEngineIDEList}
    ${graphicsSharedList}
    ${vulkanLoaderList}
)

set( RENDERERLIST
    ${mathList}
    ${memoryList}
    ${syncList}
    ${sysList}
    ${threadList}
    ${utilsList}
    ${graphicsList}
    ${ENGINE_DIR}/renderer-vulkan/Surface/Surface.cpp
    ${ENGINE_DIR}/renderer-vulkan/Surface/Surface.h
    ${ENGINE_DIR}/renderer-vulkan/Error.cpp
    ${ENGINE_DIR}/renderer-vulkan/Error.h
    ${ENGINE_DIR}/renderer-vulkan/Init.cpp
    ${ENGINE_DIR}/renderer-vulkan/Init.h
    ${ENGINE_DIR}/renderer-vulkan/MiscCVarStore.cpp
    ${ENGINE_DIR}/renderer-vulkan/MiscCVarStore.h
    ${ENGINE_DIR}/renderer-vulkan/RefAPI.cpp
    ${ENGINE_DIR}/renderer-vulkan/Version.cpp
    ${ENGINE_DIR}/renderer-vulkan/Version.h
    ${ENGINE_DIR}/renderer-vulkan/Shared/Timer.cpp
    ${ENGINE_DIR}/renderer-vulkan/Shared/Timer.h
)