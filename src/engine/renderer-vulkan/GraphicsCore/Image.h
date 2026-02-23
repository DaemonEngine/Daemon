/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2026 Daemon Developers
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
// Image.h

#ifndef IMAGE_H
#define IMAGE_H

#include "Vulkan.h"

#include "Decls.h"

namespace ImageUsage {
	enum ImageUsage {
		ATTACHMENT      = 1,
		STORAGE         = 2,
		RESOURCE        = 4,
		COMPRESSED_VIEW = 8
	};
}

enum Format          : uint32;
enum SwapChainFormat : uint32;

struct Image {
	VkImage         image;
	VkImageViewType type;
	Format          format;
	uint32          mipLevels;
	bool            cube;
	bool            storage;
	bool            depth;
	bool            stencil;
	bool            external;

	void Init( const Format newFormat, const VkExtent3D imageSize, const bool useMipLevels, const bool newCube = false,
	           const ImageUsage::ImageUsage usage = ImageUsage::RESOURCE,
	           const bool shared = false );

	void Init( VkImage newImage, const SwapChainFormat newFormat );

	VkImageView GenView();
};

struct FormatConfig {
	VkExtent3D maxSize;
	uint32     maxLayers;
	uint32     maxSamples;

	bool       hostCopyOptimal;
	bool       hostIdenticalLayout;

	bool       indirectCopy;

	bool       sampled;
	bool       minMaxSampler;

	bool       atomicStorage;
	bool       storage;

	bool       colourAttachment;
	bool       colourAttachmentBlend;

	bool       depthAttachment;

	bool       supported;
};

enum Format : uint32 {
	ABGR_2_10,

	RGBA8,
	RGBA8I,
	RGBA16,
	RGBA16I,
	RGBA16F,
	R32F,
	RGBA32I,
	RGBA32F,

	RGBA8S,

	FORMAT_COLOUR = RGBA8S,

	D16,
	D32F,

	FORMAT_DEPTH = D32F,

	D24S8,
	D32S8,

	FORMAT_DEPTH_STENCIL = D32S8,

	BC1,
	BC1S,
	BC1A,
	BC1AS,

	BC2,
	BC2S,

	BC3,
	BC3S,

	BC4,

	BC5,

	BC6F,

	BC7,
	BC7S,

	FORMAT_COUNT
};

constexpr VkFormat formats[FORMAT_COUNT] {
	VK_FORMAT_A2B10G10R10_UNORM_PACK32, // AGBR_2_10

	VK_FORMAT_R8G8B8A8_UNORM,           // RGBA8
	VK_FORMAT_R8G8B8A8_UINT,            // RGBA8I
	VK_FORMAT_R16G16B16A16_UNORM,       // RGBA16
	VK_FORMAT_R16G16B16A16_UINT,        // RGBA16I
	VK_FORMAT_R16G16B16A16_SFLOAT,      // RGBA16F
	VK_FORMAT_R32_SFLOAT,               // R32F
	VK_FORMAT_R32G32B32A32_UINT,        // RGBA32I
	VK_FORMAT_R32G32B32A32_SFLOAT,      // RGBA32F

	VK_FORMAT_R8G8B8A8_SRGB,            // RGBA8S

	VK_FORMAT_D16_UNORM,                // D16
	VK_FORMAT_D32_SFLOAT,               // D32F
	VK_FORMAT_D24_UNORM_S8_UINT,        // D24S8
	VK_FORMAT_D32_SFLOAT_S8_UINT,       // D32S8

	VK_FORMAT_BC1_RGB_UNORM_BLOCK,      // BC1
	VK_FORMAT_BC1_RGB_SRGB_BLOCK,       // BC1S
	VK_FORMAT_BC1_RGBA_UNORM_BLOCK,     // BC1A
	VK_FORMAT_BC1_RGBA_SRGB_BLOCK,      // BC1AS

	VK_FORMAT_BC2_UNORM_BLOCK,          // BC2
	VK_FORMAT_BC2_SRGB_BLOCK,           // BC2S

	VK_FORMAT_BC3_UNORM_BLOCK,          // BC3
	VK_FORMAT_BC3_SRGB_BLOCK,           // BC3S

	VK_FORMAT_BC4_UNORM_BLOCK,          // BC4

	VK_FORMAT_BC5_UNORM_BLOCK,          // BC5

	VK_FORMAT_BC6H_UFLOAT_BLOCK,        // BC6F

	VK_FORMAT_BC7_UNORM_BLOCK,          // BC7
	VK_FORMAT_BC7_SRGB_BLOCK            // BC7S
};

enum SwapChainFormat : uint32 {
	S_RGBA8,
	S_RGBA8S,

	S_ABGR_2_10,

	SWAPCHAIN_FORMAT_COUNT
};

constexpr VkFormat swapchainFormats[SWAPCHAIN_FORMAT_COUNT] {
	VK_FORMAT_R8G8B8A8_UNORM,          // S_RGBA8
	VK_FORMAT_R8G8B8A8_SRGB,           // S_RGBA8S

	VK_FORMAT_A2B10G10R10_UNORM_PACK32 // S_AGBR_2_10
};

extern FormatConfig formatConfigs[FORMAT_COUNT];
extern FormatConfig swapchainFormatConfigs[SWAPCHAIN_FORMAT_COUNT];

void InitFormatConfigs();

#endif // IMAGE_H