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

#include "DetectGLVendors.h"

std::string GetGLHardwareVendorName( glHardwareVendor_t hardwareVendor )
{
	static const std::string hardwareVendorNames[] = {
		"Unknown",
		"Software rasterizer",
		"API Translator",
		"Apple",
		"Arm",
		"AMD/ATI",
		"Broadcom",
		"Intel",
		"Nvidia",
		"Moore Threads",
		"Imagination",
		"OutOfRange",
	};

	int index = Util::ordinal( hardwareVendor );
	static constexpr size_t lastNameIndex = ARRAY_LEN( hardwareVendorNames ) - 1;
	static constexpr int lastEnumIndex = Util::ordinal( glHardwareVendor_t::NUM_HARDWARE_VENDORS );

	static_assert( lastNameIndex == lastEnumIndex, "glHardwareVendor_t and hardwareVendorNames count mismatch" );
	ASSERT_GE( index, 0 );
	ASSERT_LT( index, lastEnumIndex );

	index = ( index < 0 || index > lastEnumIndex ) ? lastNameIndex : index;

	return hardwareVendorNames[ index ];
}

std::string GetGLDriverVendorName( glDriverVendor_t driverVendor )
{
	static const std::string driverVendorNames[] = {
		"Unknown",
		"Apple",
		"AMD/ATI",
		"Intel",
		"Mesa",
		"Nvidia",
		"Moore Threads",
		"Imagination",
		"GL4ES",
		"OutOfRange",
	};

	int index = Util::ordinal( driverVendor );
	static constexpr size_t lastNameIndex = ARRAY_LEN( driverVendorNames ) - 1;
	static constexpr int lastEnumIndex = Util::ordinal( glDriverVendor_t::NUM_DRIVER_VENDORS );

	static_assert( lastNameIndex == lastEnumIndex, "glDriverVendor_t and driverVendorNames count mismatch" );
	ASSERT_GE( index, 0 );
	ASSERT_LT( index, lastEnumIndex );

	index = ( index < 0 || index > lastEnumIndex ) ? lastNameIndex : index;

	return driverVendorNames[ index ];
}

static std::string StripPrefix( const std::string &prefix, const std::string &string )
{
	if ( Str::IsPrefix( prefix, string ) )
	{
		size_t prefixLen = prefix.length();
		size_t stringLen = string.length();
		size_t subLen = stringLen - prefixLen;
		return string.substr( prefixLen, subLen );
	}

	return string;
}

void DetectGLVendors(
	const std::string& vendorString,
	const std::string& versionString,
	const std::string& rendererString,
	glHardwareVendor_t& hardwareVendor,
	glDriverVendor_t& driverVendor )
{
	hardwareVendor = glHardwareVendor_t::UNKNOWN;
	driverVendor = glDriverVendor_t::UNKNOWN;

	// Those vendor strings are assumed to be unambiguous about both driver and hardware vendors.
	static const std::unordered_map<std::string, std::pair<glDriverVendor_t, glHardwareVendor_t>>
		vendorDriverHardware =
	{
		// Mesa AMD/ATI.
		{ "AMD", { glDriverVendor_t::MESA, glHardwareVendor_t::ATI } },
		{ "AMD inc.", { glDriverVendor_t::MESA, glHardwareVendor_t::ATI } },
		{ "X.Org R300 Project", { glDriverVendor_t::MESA, glHardwareVendor_t::ATI } },
		// Proprietary ATI or AMD drivers on all systems like Linux, Windows, and macOS: OGLP, Catalyst…
		{ "ATI Technologies Inc.", { glDriverVendor_t::ATI, glHardwareVendor_t::ATI } },
		// Proprietary Intel driver on macOS.
		{ "Intel Inc.", { glDriverVendor_t::INTEL, glHardwareVendor_t::INTEL } },
		// Mesa Intel.
		{ "Intel Open Source Technology Center", { glDriverVendor_t::MESA, glHardwareVendor_t::INTEL } },
		{ "Tungsten Graphics, Inc", { glDriverVendor_t::MESA, glHardwareVendor_t::INTEL } },
		// Mesa V3D and VC4 (Raspberry Pi).
		{ "Broadcom", { glDriverVendor_t::MESA, glHardwareVendor_t::BROADCOM } },
		// Mesa Panfrost, newer Panfrost uses "Mesa" instead.
		{ "Panfrost", { glDriverVendor_t::MESA, glHardwareVendor_t::ARM } },
		// Mesa Nvidia for supported OpenGL 2+ hardware.
		// Mesa Amber also provides "Nouveau", but this is for unsupported pre-OpenGL 2 Nvidia.
		{ "nouveau", { glDriverVendor_t::MESA, glHardwareVendor_t::NVIDIA } },
		// Proprietary Nvidia drivers on all systems like Linux, Windows, and macOS.
		{ "NVIDIA Corporation", { glDriverVendor_t::NVIDIA, glHardwareVendor_t::NVIDIA } },
		// Moore Threads drivers on Linux and Windows.
		{ "Moore Threads", { glDriverVendor_t::MTHREADS, glHardwareVendor_t::MTHREADS } },
		// Proprietary Imagination driver for PowerVR.
		{ "Imagination Technologies", { glDriverVendor_t::IMAGINATION, glHardwareVendor_t::IMAGINATION } },
	};

	auto it = vendorDriverHardware.find( vendorString );
	if ( it != vendorDriverHardware.end() )
	{
		driverVendor = it->second.first;	
		hardwareVendor = it->second.second;	
		return;
	}

	static const std::string appleVendors[] = {
		"Apple",
		"Apple Inc.",
		"Apple Computer, Inc.",
	};

	for ( auto& s : appleVendors )
	{
		if ( vendorString == s )
		{
			driverVendor = glDriverVendor_t::APPLE;
			break;
		}
	}

	if ( driverVendor == glDriverVendor_t::APPLE )
	{
		if ( rendererString == "Apple Software Renderer" )
		{
			hardwareVendor = glHardwareVendor_t::SOFTWARE;
			return;
		}

		hardwareVendor = glHardwareVendor_t::APPLE;
		return;
	}

	// This vendor string is used by at least two different Intel drivers.
	if ( vendorString == "Intel Corporation" )
	{
		if ( Str::IsPrefix( "SWR ", rendererString ) )
		{
			/* It is part of Mesa Amber, but was merged in Mesa lately after being an internal
			Intel product for years. It doesn't share much things with Mesa, and we better
			not assume it behaves the same as Mesa, it may even behave like Intel.
			We keep driverVendor as glDriverVendor_t::UNKNOWN on purpose. */
			hardwareVendor = glHardwareVendor_t::SOFTWARE;
			return;
		}
		else
		{
			driverVendor = glDriverVendor_t::MESA;
			hardwareVendor = glHardwareVendor_t::INTEL;
			return;
		}
	}

	// This vendor string is used by at least two different Microsoft drivers.
	if ( vendorString == "Microsoft Corporation" )
	{
		/* The GL_VENDOR string can also be "Microsoft Corporation" with GL_RENDERER
		set to "GDI Generic" which is not Mesa and isn't supported (OpenGL 1.1). */
		if ( Str::IsPrefix( "DRD12 ", rendererString ) )
		{
			driverVendor = glDriverVendor_t::MESA;
			// OpenGL over Direct3D12 translation.
			hardwareVendor = glHardwareVendor_t::TRANSLATION;
			return;
		}
	}

	/* Those substrings at the beginning of a renderer string are assumed to be unambiguous about
	both driver and hardware. */
	static const std::pair<std::string, glHardwareVendor_t> rendererMesaStartStringHardware[] = {
		// "Mesa DRI R200" exists for ATI but isn't supported.
		{ "Mesa DRI nv", glHardwareVendor_t::NVIDIA },
		{ "Mesa DRI Intel(R)", glHardwareVendor_t::INTEL },
	};

	for ( auto& p : rendererMesaStartStringHardware )
	{
		if ( Str::IsPrefix( p.first, rendererString ) )
		{
			driverVendor = glDriverVendor_t::MESA;
			hardwareVendor = p.second;
			return;
		}
	}
	
	// Those vendor strings are assumed to be unambiguous about being from Mesa drivers.
	static const std::string mesaVendors[] = {
		// Mesa.
		"Mesa",
		"Mesa Project",
		"Mesa/X.org",
		"X.Org",
		// Zink.
		"Collabora Ltd",
		// virgl.
		"Red Hat",
		// llvmpipe, softpipe, SVGA3D.
		"VMware, Inc.",
	};

	for ( auto& s : mesaVendors )
	{
		if ( vendorString == s )
		{
			driverVendor = glDriverVendor_t::MESA;
			break;
		}
	}

	/* This substring in a version string is assumed to be unambiguous about being from Mesa
	drivers. Mesa GL_VENDOR and GL_RENDERER strings are very fluid, but it is believed that
	GL_VERSION always contains the " Mesa " substring and that substring always means Mesa. */
	if ( driverVendor == glDriverVendor_t::UNKNOWN && versionString.find( " Mesa " ) != std::string::npos )
	{
		driverVendor = glDriverVendor_t::MESA;
	}

	if ( driverVendor == glDriverVendor_t::MESA )
	{
		// This vendor string for a Mesa driver is assumed to be unambiguous about the hardware.
		if ( vendorString == "Intel" )
		{
			hardwareVendor = glHardwareVendor_t::INTEL;
			return;
		}

		/* Those substrings at the beginning of a renderer string are assumed to be unambiguous about
		the hardware or underlying technology. */
		static const std::pair<std::string, glHardwareVendor_t> rendererStartStringHardware[] = {
			{ "Apple ", glHardwareVendor_t::APPLE },
			{ "ATI ", glHardwareVendor_t::ATI },
			{ "AMD ", glHardwareVendor_t::ATI },
			{ "i915 ", glHardwareVendor_t::INTEL },
			{ "NV", glHardwareVendor_t::NVIDIA },
			{ "Mali-", glHardwareVendor_t::ARM },
			// OpenGL over Vulkan translation.
			{ "zink ", glHardwareVendor_t::TRANSLATION },
			// Virtualization.
			{ "virgl", glHardwareVendor_t::TRANSLATION },
		};

		for ( auto& p : rendererStartStringHardware )
		{
			if ( Str::IsPrefix( p.first, rendererString ) )
			{
				hardwareVendor = p.second;
				return;
			}
		}

		/* Those substrings within a renderer string are assumed to be unambiguous about
		the underlying technology. */
		static const std::pair<std::string, glHardwareVendor_t> rendererSubStringHardware[] = {
			{ "Panfrost", glHardwareVendor_t::ARM },
			// Software rendering.
			{ "llvmpipe", glHardwareVendor_t::SOFTWARE },
			{ "softpipe", glHardwareVendor_t::SOFTWARE },
			// Virtualization.
			{ "SVGA3D", glHardwareVendor_t::TRANSLATION },
		};

		for ( auto& p : rendererSubStringHardware )
		{
			if ( rendererString.find( p.first ) != std::string::npos )
			{
				hardwareVendor = p.second;
				return;
			}
		}
	}

	/* As both proprietary Intel driver on Windows and Mesa may report "Intel",
	we rely on the fact Mesa is already detected to know it is the proprietary
	Windows driver if not Mesa. */
	if ( vendorString == "Intel" )
	{
		driverVendor = glDriverVendor_t::INTEL;
		hardwareVendor = glHardwareVendor_t::INTEL;
		return;
	}

	// Newer GL4ES strings disclosing the underlying technology.
	if ( Str::IsPrefix( "GL4ES wrapping ", vendorString ) )
	{
		std::string subVendorString = StripPrefix( "GL4ES wrapping ", vendorString );
		std::string subRendererString = StripPrefix( "GL4ES using ", rendererString );
		DetectGLVendors( subVendorString, versionString, subRendererString, hardwareVendor, driverVendor );
		driverVendor = glDriverVendor_t::GL4ES;
	}

	/* Older GL4ES string not disclosing the underlying technology,
	also had “ptitSeb” as vendorString. */
	if ( rendererString == "GL4ES wrapper" )
	{
		driverVendor = glDriverVendor_t::GL4ES;
		// Older GL4ES doesn't disclose the underlying hardware.
		if ( hardwareVendor == glHardwareVendor_t::UNKNOWN )
		{
			hardwareVendor = glHardwareVendor_t::TRANSLATION;
		}
		return;
	}

	/* GL4ES always use such kind of version string:
	> 2.1 gl4es wrapper 1.1.7
	And this is unlikely to change. */
	if ( versionString.find( "gl4es wrapper" ) != std::string::npos )
	{
		driverVendor = glDriverVendor_t::GL4ES;
		if ( hardwareVendor == glHardwareVendor_t::UNKNOWN )
		{
			hardwareVendor = glHardwareVendor_t::TRANSLATION;
		}
		return;
	}
}
