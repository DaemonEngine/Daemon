/*
===========================================================================
Copyright (C) 2011 Matthias Bentrup <matthias.bentrup@googlemail.com>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "tr_local.h"
#include <webp/decode.h>

namespace {
bool LoadInMemoryWEBP( const char *path, const uint8_t* webpData, size_t webpSize, byte **pic, int *width,
	int *height ) {
	// Validate data and query image size.
	if ( !WebPGetInfo( webpData, webpSize, width, height ) )
	{
		Log::Warn( "WebP image '%s' has an invalid format", path );
		return false;
	}

	const size_t stride{ *width * sizeof( u8vec4_t ) };
	const size_t size{ *height * stride };
	auto *out = (byte*)Z_Malloc( size );

	// Decode into RGBA.
	if ( !WebPDecodeRGBAInto( webpData, webpSize, out, size, stride ) )
	{
		Log::Warn( "WebP image '%s' has bad header or data", path );
		return false;
	}

	*pic = out;
	return true;
}
}

/*
=========================================================

LoadWEBP

=========================================================
*/
void LoadWEBP( const char *path, byte **pic, int *width, int *height, int *, int *, int *, byte )
{
	*pic = nullptr;
	
	std::error_code err;
	std::string webpData = FS::PakPath::ReadFile( path, err );

	if ( err ) {
		return;
	}

	if ( !LoadInMemoryWEBP( path, reinterpret_cast<const uint8_t*>(webpData.data()), webpData.size(), pic, width, height ) ) {
		Z_Free( *pic );
		*pic = nullptr; // This signals failure.
	}
}
