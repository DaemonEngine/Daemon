/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2024 Daemon Developers
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

// DetectGLVendors.h

#ifndef DETECT_OPENGL_VENDORS_H
#define DETECT_OPENGL_VENDORS_H

#include "common/Common.h"
#include "common/Assert.h"
#include "qcommon/q_shared.h"

enum class glHardwareVendor_t
{
	UNKNOWN,
	// Assumed to be slow (running on CPU).
	SOFTWARE,
	/* Assumed to be fast (running on GPU through a translation, like an OpenGL over Vulkan
	driver, or an OpenGL virtualization driver sending commands to an hypervisor. */
	TRANSLATION,
	// Assumed to be fast (running on GPU directly).
	APPLE,
	ARM,
	ATI,
	BROADCOM,
	INTEL,
	NVIDIA,
	MTHREADS,
	NUM_HARDWARE_VENDORS,
};

enum class glDriverVendor_t
{
	UNKNOWN,
	APPLE,
	ATI,
	INTEL,
	MESA,
	NVIDIA,
	MTHREADS,
	NUM_DRIVER_VENDORS,
};

std::string GetGLHardwareVendorName( glHardwareVendor_t hardwareVendor );

std::string GetGLDriverVendorName( glDriverVendor_t driverVendor );

void DetectGLVendors(
	const std::string& vendorString,
	const std::string& versionString,
	const std::string& rendererString,
	glHardwareVendor_t& hardwareVendor,
	glDriverVendor_t& driverVendor );

#endif // DETECT_OPENGL_VENDORS_H
