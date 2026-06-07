set( rendererVulkan ${ENGINE_DIR}/renderer-vulkan )

# Graphics Core
set( graphicsCore ${rendererVulkan}/GraphicsCore )

set( graphicsCoreMemory
    ${graphicsCore}/Memory/CoreThreadMemory.cpp
    ${graphicsCore}/Memory/CoreThreadMemory.h
    ${graphicsCore}/Memory/DescriptorSet.cpp
    ${graphicsCore}/Memory/DescriptorSet.h
    ${graphicsCore}/Memory/EngineAllocator.cpp
    ${graphicsCore}/Memory/EngineAllocator.h
)

set( graphicsCoreList
    ${graphicsCoreMemory}
    ${graphicsCore}/ExecutionGraph/ExecCmd.cpp
    ${graphicsCore}/ExecutionGraph/ExecCmd.h
    ${graphicsCore}/ExecutionGraph/ExecutionGraph.cpp
    ${graphicsCore}/ExecutionGraph/ExecutionGraph.h
    ${graphicsCore}/ExecutionGraph/PipelineCache.cpp
    ${graphicsCore}/ExecutionGraph/PipelineCache.h
    ${graphicsCore}/Decls.h
    ${graphicsCore}/CapabilityPack.cpp
    ${graphicsCore}/CapabilityPack.h
    ${graphicsCore}/DebugMsg.cpp
    ${graphicsCore}/DebugMsg.h
    ${graphicsCore}/EngineConfig.cpp
    ${graphicsCore}/EngineConfig.h
    ${graphicsCore}/EngineDispatch.cpp
    ${graphicsCore}/EngineDispatch.h
    ${graphicsCore}/FeaturesConfig.cpp
    ${graphicsCore}/FeaturesConfig.h
    ${graphicsCore}/FeaturesConfigMap.cpp
    ${graphicsCore}/FeaturesConfigMap.h
    ${graphicsCore}/GraphicsCoreCVars.cpp
    ${graphicsCore}/GraphicsCoreCVars.h
    ${graphicsCore}/GraphicsCoreStore.cpp
    ${graphicsCore}/GraphicsCoreStore.h
    ${graphicsCore}/GraphicsResource.cpp
    ${graphicsCore}/GraphicsResource.h
    ${graphicsCore}/Image.cpp
    ${graphicsCore}/Image.h
    ${graphicsCore}/Init.cpp
    ${graphicsCore}/Init.h
    ${graphicsCore}/Instance.cpp
    ${graphicsCore}/Instance.h
    ${graphicsCore}/PhysicalDevice.cpp
    ${graphicsCore}/PhysicalDevice.h
    ${graphicsCore}/Queue.cpp
    ${graphicsCore}/Queue.h
    ${graphicsCore}/ResourceSystem.cpp
    ${graphicsCore}/ResourceSystem.h
    ${graphicsCore}/ResultCheck.cpp
    ${graphicsCore}/ResultCheck.h
    ${graphicsCore}/Semaphore.cpp
    ${graphicsCore}/Semaphore.h
    ${graphicsCore}/SwapChain.cpp
    ${graphicsCore}/SwapChain.h
    ${graphicsCore}/Vulkan.h
)

# Graphics Engine
set( graphicsEngineListH
    Common.glsl
    Buffers.glsl
    Entity.glsl
    Light.glsl
    Images.glsl
    Resources.glsl
)

set( graphicsEngineList
    MsgStream.glsl
    SparseCopy.glsl
    Tonemap.glsl
    TestV.glsl
    TestF.glsl
)

set( graphicsEngineIDEList ${graphicsEngineList} )
list( APPEND graphicsEngineIDEList ${graphicsEngineListH} )
list( TRANSFORM graphicsEngineIDEList PREPEND ${rendererVulkan}/GraphicsEngine/ )

# Graphics Shared
set( graphicsShared ${rendererVulkan}/GraphicsShared )

set( graphicsSharedList
    ${graphicsShared}/Bindings.h
    ${graphicsShared}/CoreData.h
    ${graphicsShared}/Entity.h
    ${graphicsShared}/Light.h
    ${graphicsShared}/MemoryPool.h
    ${graphicsShared}/MsgStreamAPI.h
    ${graphicsShared}/Int.h
    ${graphicsShared}/PushLayout.h
    ${graphicsShared}/SharedResources.h
)

# Vulkan Loader
set( vulkanLoaderList
    ${rendererVulkan}/VulkanLoader/Vulkan.cpp
    ${rendererVulkan}/VulkanLoader/VulkanLoadFunctions.cpp
    ${rendererVulkan}/VulkanLoader/VulkanLoadFunctions.h
    ${rendererVulkan}/VulkanLoader/Vulkan.h
)

set( RENDERERLIST
    ${graphicsCoreList}
    ${graphicsEngineIDEList}
    ${graphicsSharedList}
    ${vulkanLoaderList}
    ${rendererVulkan}/Surface/Surface.cpp
    ${rendererVulkan}/Surface/Surface.h
    ${rendererVulkan}/DispatchRawData.cpp
    ${rendererVulkan}/DispatchRawData.h
    ${rendererVulkan}/Init.cpp
    ${rendererVulkan}/Init.h
    ${rendererVulkan}/RefAPI.cpp
)