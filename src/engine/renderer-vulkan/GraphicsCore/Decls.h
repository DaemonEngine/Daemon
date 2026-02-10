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
// Decls.h

#ifndef GRAPHICS_CORE_DECLS_H
#define GRAPHICS_CORE_DECLS_H

#include "../Math/NumberTypes.h"

#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;

class  Instance;
class  Surface;
struct SwapChain;

struct EngineConfig;
struct QueuesConfig;
struct FeaturesConfig;

enum   QueueType : uint32;

struct VkAllocationCallbacks;

struct Semaphore;

struct VkPhysicalDeviceProperties2;
struct VkPhysicalDeviceFeatures2;
struct VkDeviceCreateInfo;

VK_DEFINE_HANDLE( VkInstance );
VK_DEFINE_HANDLE( VkPhysicalDevice );
VK_DEFINE_HANDLE( VkDevice );
VK_DEFINE_HANDLE( VkQueue );

VK_DEFINE_HANDLE( VkCommandBuffer );

VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkSurfaceKHR )
VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkSwapchainKHR )

VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkDescriptorSetLayout )
VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkDescriptorSet )

VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkBuffer )
VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkImage )

VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkPipelineLayout )
VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkPipeline )
VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkCommandPool )

VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkFence )
VK_DEFINE_NON_DISPATCHABLE_HANDLE( VkSemaphore )

using VkPipelineStageFlags2     = uint64;
using VkCommandBufferUsageFlags = uint32;

struct VkSemaphoreSubmitInfo;

struct GraphicsQueueRingBuffer;

extern GraphicsQueueRingBuffer graphicsQueue;
extern GraphicsQueueRingBuffer computeQueue;
extern GraphicsQueueRingBuffer transferQueue;
extern GraphicsQueueRingBuffer sparseQueue;

class EngineAllocator;

#endif // GRAPHICS_CORE_DECLS_H