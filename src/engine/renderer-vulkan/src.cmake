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
    ${ENGINE_DIR}/renderer-vulkan/Memory/IteratorSeq.h
)

set( sysList
    ${ENGINE_DIR}/renderer-vulkan/Sys/CPUInfo.cpp
    ${ENGINE_DIR}/renderer-vulkan/Sys/CPUInfo.h
    ${ENGINE_DIR}/renderer-vulkan/Sys/MemoryInfo.cpp
    ${ENGINE_DIR}/renderer-vulkan/Sys/MemoryInfo.h
)

set( RENDERERLIST
    ${mathList}
    ${memoryList}
    ${sysList}
    ${ENGINE_DIR}/renderer-vulkan/DispatchRawData.cpp
    ${ENGINE_DIR}/renderer-vulkan/DispatchRawData.h
    ${ENGINE_DIR}/renderer-vulkan/Error.cpp
    ${ENGINE_DIR}/renderer-vulkan/Error.h
    ${ENGINE_DIR}/renderer-vulkan/Shared/Timer.cpp
    ${ENGINE_DIR}/renderer-vulkan/Shared/Timer.h
)