set(MATHLIST
    ${ENGINE_DIR}/renderer-vulkan/Math/Bit.h
)

set(MEMORYLIST
    ${ENGINE_DIR}/renderer-vulkan/Memory/Array.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/DynamicArray.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/Memory.cpp
    ${ENGINE_DIR}/renderer-vulkan/Memory/Memory.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/MemoryChunk.cpp
    ${ENGINE_DIR}/renderer-vulkan/Memory/MemoryChunk.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/IteratorSeq.h
    ${ENGINE_DIR}/renderer-vulkan/Memory/RingBuffer.cpp
    ${ENGINE_DIR}/renderer-vulkan/Memory/RingBuffer.h
)

set(TASKLIST
    ${ENGINE_DIR}/renderer-vulkan/Thread/Task.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/TaskList.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/TaskList.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/Thread.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/Thread.h
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadMemory.cpp
    ${ENGINE_DIR}/renderer-vulkan/Thread/ThreadMemory.h
)

set(UTILSLIST
    ${ENGINE_DIR}/renderer-vulkan/SrcDebug/LogExtend.h
    ${ENGINE_DIR}/renderer-vulkan/SrcDebug/Tag.h
)

set(RENDERERLIST
    ${MATHLIST}
    ${MEMORYLIST}
    ${TASKLIST}
    ${UTILSLIST}
    ${ENGINE_DIR}/renderer-vulkan/MiscCVarStore.cpp
    ${ENGINE_DIR}/renderer-vulkan/RefAPI.cpp
    ${ENGINE_DIR}/renderer-vulkan/RefAPI.h
    ${ENGINE_DIR}/renderer-vulkan/Shared/Timer.cpp
    ${ENGINE_DIR}/renderer-vulkan/Shared/Timer.h
)
