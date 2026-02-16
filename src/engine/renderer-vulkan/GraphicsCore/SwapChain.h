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
// SwapChain.h

#ifndef SWAP_CHAIN_H
#define SWAP_CHAIN_H

#include "Decls.h"

#include "../Math/NumberTypes.h"

#include "../Memory/DynamicArray.h"

#include "Image.h"

#include "GraphicsResource.h"

namespace PresentMode {
	enum PresentMode {
		IMMEDIATE,             // VK_PRESENT_MODE_IMMEDIATE_KHR
		SCANOUT_ONE,           // VK_PRESENT_MODE_MAILBOX_KHR
		SCANOUT_FIRST,         // VK_PRESENT_MODE_FIFO_KHR
		SCANOUT_FIRST_RELAXED, // VK_PRESENT_MODE_FIFO_RELAXED_KHR
		SCANOUT_LATEST         // VK_PRESENT_MODE_FIFO_LATEST_READY_KHR
	};
}

struct SwapChain : public GraphicsResource {
	VkSurfaceKHR   surface;
	VkSwapchainKHR swapChain;

	uint32         minImages;
	uint32         maxImages;
	uint32         imageCount;

	DynamicArray<Image> images;

	void Init( const VkInstance instance );
	void Free() override;

	uint32 AcquireNextImage( const uint64 timeout, VkFence fence, VkSemaphore semaphore );
};

#endif // SWAP_CHAIN_H