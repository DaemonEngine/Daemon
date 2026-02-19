/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2009 Peter McNeill <n27@bigpond.net.au>

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
// tr_bsp.c
#include "tr_local.h"
#include "framework/CommandSystem.h"
#include "GeometryCache.h"
#include "GeometryOptimiser.h"
#include "ShadeCommon.h"

/*
========================================================
Loads and prepares a map file for scene rendering.

A single entry point:
RE_LoadWorldMap(const char *name);
========================================================
*/

static world_t    s_worldData;
static byte       *fileBase;

//===============================================================================

static void R_LinearizeLightingColorBytes( byte* bytes )
{
	if ( !tr.worldLinearizeLightMap )
	{
		return;
	}

	convertFromSRGB( bytes );
}

static bool cannotColorShiftLighting()
{
	if ( tr.overbrightBits >= tr.mapOverBrightBits )
	{
		return true;
	}

	if ( tr.worldLinearizeLightMap )
	{
		return true;
	}

	return false;
}

/*
===============
R_ColorShiftLightingBytes
===============
*/
static void R_ColorShiftLightingBytes( byte bytes[ 4 ] )
{
	if ( cannotColorShiftLighting() )
	{
		return;
	}

	int shift = tr.mapOverBrightBits - tr.overbrightBits;

	// shift the data based on overbright range
	int r = bytes[ 0 ] << shift;
	int g = bytes[ 1 ] << shift;
	int b = bytes[ 2 ] << shift;

	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 )
	{
		int max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	bytes[ 0 ] = r;
	bytes[ 1 ] = g;
	bytes[ 2 ] = b;
}

static void R_ColorShiftLightingBytesCompressed( byte bytes[ 8 ] )
{
	if ( cannotColorShiftLighting() )
	{
		return;
	}

	// color shift the endpoint colors in the dxt block
	unsigned short rgb565 = bytes[1] << 8 | bytes[0];
	byte rgba[4];

	rgba[0] = (rgb565 >> 8) & 0xf8;
	rgba[1] = (rgb565 >> 3) & 0xfc;
	rgba[2] = (rgb565 << 3) & 0xf8;
	rgba[3] = 0xff;

	R_ColorShiftLightingBytes( rgba );

	rgb565 = ((rgba[0] >> 3) << 11) |
		((rgba[1] >> 2) << 5) |
		((rgba[2] >> 3) << 0);
	bytes[0] = rgb565 & 0xff;
	bytes[1] = rgb565 >> 8;

	rgb565 = bytes[3] << 8 | bytes[2];
	rgba[0] = (rgb565 >> 8) & 0xf8;
	rgba[1] = (rgb565 >> 3) & 0xfc;
	rgba[2] = (rgb565 << 3) & 0xf8;
	rgba[3] = 0xff;

	R_ColorShiftLightingBytes( rgba );

	rgb565 = ((rgba[0] >> 3) << 11) |
		((rgba[1] >> 2) << 5) |
		((rgba[2] >> 3) << 0);
	bytes[2] = rgb565 & 0xff;
	bytes[3] = rgb565 >> 8;
}

/*
===============
R_ProcessLightmap
===============
*/
void R_ProcessLightmap( byte *bytes, int width, int height, int bits )
{
	if ( cannotColorShiftLighting() )
	{
		return;
	}

	if ( bits & IF_BC1 ) {
		for ( int i = 0; i < ((width + 3) >> 2) * ((height + 3) >> 2); i++ )
		{
			R_ColorShiftLightingBytesCompressed( &bytes[ i * 8 ] );
		}
	} else if( bits & (IF_BC2 | IF_BC3) ) {
		for ( int i = 0; i < ((width + 3) >> 2) * ((height + 3) >> 2); i++ )
		{
			R_ColorShiftLightingBytesCompressed( &bytes[ i * 16 ] );
		}
	} else {
		for ( int i = 0; i < width * height; i++ )
		{
			R_ColorShiftLightingBytes( &bytes[ i * 4 ] );
		}
	}
}

static int LightmapNameCompare( const char *s1, const char *s2 )
{
	int  c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if ( c1 >= 'a' && c1 <= 'z' )
		{
			c1 -= ( 'a' - 'A' );
		}

		if ( c2 >= 'a' && c2 <= 'z' )
		{
			c2 -= ( 'a' - 'A' );
		}

		if ( c1 == '\\' || c1 == ':' )
		{
			c1 = '/';
		}

		if ( c2 == '\\' || c2 == ':' )
		{
			c2 = '/';
		}

		if ( c1 < c2 )
		{
			// strings not equal
			return -1;
		}

		if ( c1 > c2 )
		{
			return 1;
		}
	}
	while ( c1 );

	// strings are equal
	return 0;
}

static bool LightmapNameLess( const std::string& a, const std::string& b)
{
	return LightmapNameCompare( a.c_str(), b.c_str() ) < 0;
}

void LoadRGBEToFloats( const char *name, float **pic, int *width, int *height )
{
	int      i, j;
	const byte *buf_p;
	float    *floatbuf;
	int      w, h, c;
	bool formatFound;

	union
	{
		byte  b[ 4 ];
		float f;
	} sample;

	*pic = nullptr;

	// load the file
	std::error_code err;
	std::string buffer = FS::PakPath::ReadFile( name, err );

	if ( err )
	{
		Sys::Drop( "RGBE image '%s' is not found", name );
	}

	buf_p = reinterpret_cast<const byte*>(buffer.c_str());

	formatFound = false;
	w = h = 0;

	while ( true )
	{
		const char *token = COM_ParseExt2( ( const char ** ) &buf_p, true );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( !Q_stricmp( token, "FORMAT" ) )
		{
			token = COM_ParseExt2( ( const char ** ) &buf_p, false );

			if ( !Q_stricmp( token, "=" ) )
			{
				token = COM_ParseExt2( ( const char ** ) &buf_p, false );

				if ( !Q_stricmp( token, "32" ) )
				{
					token = COM_ParseExt2( ( const char ** ) &buf_p, false );

					if ( !Q_stricmp( token, "-" ) )
					{
						token = COM_ParseExt2( ( const char ** ) &buf_p, false );

						if ( !Q_stricmp( token, "bit_rle_rgbe" ) )
						{
							formatFound = true;
						}
						else
						{
							Log::Warn("RGBE image '%s' has expected 'bit_rle_rgbe' but found '%s' instead",
								name, token );
						}
					}
					else
					{
						Log::Warn("RGBE image '%s' has expected '-' but found '%s' instead", name, token );
					}
				}
				else
				{
					Log::Warn("RGBE image '%s' has expected '32' but found '%s' instead", name, token );
				}
			}
			else
			{
				Log::Warn("RGBE image '%s' has expected '=' but found '%s' instead", name, token );
			}
		}

		if ( !Q_stricmp( token, "-" ) )
		{
			token = COM_ParseExt2( ( const char ** ) &buf_p, false );

			if ( !Q_stricmp( token, "Y" ) )
			{
				token = COM_ParseExt2( ( const char ** ) &buf_p, false );
				w = atoi( token );

				token = COM_ParseExt2( ( const char ** ) &buf_p, false );

				if ( !Q_stricmp( token, "+" ) )
				{
					token = COM_ParseExt2( ( const char ** ) &buf_p, false );

					if ( !Q_stricmp( token, "X" ) )
					{
						token = COM_ParseExt2( ( const char ** ) &buf_p, false );
						h = atoi( token );
						break;
					}
					else
					{
						Log::Warn("RGBE image '%s' has expected 'X' but found '%s' instead", name, token );
					}
				}
				else
				{
					Log::Warn("RGBE image '%s' has expected '+' but found '%s' instead", name, token );
				}
			}
			else
			{
				Log::Warn("RGBE image '%s' has expected 'Y' but found '%s' instead", name, token );
			}
		}
	}

	// go to the first byte
	while ( ( c = *buf_p++ ) != 0 )
	{
		if ( c == '\n' )
		{
			break;
		}
	}

	if ( width )
	{
		*width = w;
	}

	if ( height )
	{
		*height = h;
	}

	if ( !formatFound )
	{
		Sys::Drop( "RGBE image '%s' has no format", name );
	}

	if ( !w || !h )
	{
		Sys::Drop( "RGBE image '%s' has an invalid image size", name );
	}

	*pic = (float*) Z_AllocUninit( w * h * 3 * sizeof( float ) );
	floatbuf = *pic;

	for ( i = 0; i < ( w * h ); i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			sample.b[ 0 ] = *buf_p++;
			sample.b[ 1 ] = *buf_p++;
			sample.b[ 2 ] = *buf_p++;
			sample.b[ 3 ] = *buf_p++;

			*floatbuf++ = sample.f / 255.0f; // FIXME XMap2's output is 255 times too high
		}
	}
}

static void LoadRGBEToBytes( const char *name, byte **ldrImage, int *width, int *height )
{
	int    i, j;
	int    w, h;
	float  *hdrImage;
	float  *floatbuf;
	byte   *pixbuf;
	vec3_t sample;
	float  max;

	w = h = 0;
	LoadRGBEToFloats( name, &hdrImage, &w, &h );

	*width = w;
	*height = h;

	*ldrImage = (byte*) Z_Malloc( w * h * 4 );
	pixbuf = *ldrImage;

	floatbuf = hdrImage;

	for ( i = 0; i < ( w * h ); i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			sample[ j ] = *floatbuf++ * 255.0f;
		}

		// clamp with color normalization
		max = sample[ 0 ];

		if ( sample[ 1 ] > max )
		{
			max = sample[ 1 ];
		}

		if ( sample[ 2 ] > max )
		{
			max = sample[ 2 ];
		}

		if ( max > 255.0f )
		{
			VectorScale( sample, ( 255.0f / max ), sample );
		}

		*pixbuf++ = ( byte ) sample[ 0 ];
		*pixbuf++ = ( byte ) sample[ 1 ];
		*pixbuf++ = ( byte ) sample[ 2 ];
		*pixbuf++ = ( byte ) 255;
	}

	Z_Free( hdrImage );
}

static std::vector<std::string> R_LoadExternalLightmaps( const char *mapName )
{
	const char *const extensions[] {".png", ".tga", ".webp", ".crn", ".jpg", ".jpeg"};
	std::vector<std::string> files[ ARRAY_LEN( extensions ) ];
	for ( const std::string& filename : FS::PakPath::ListFiles( mapName ) ) {
		for ( size_t i = 0; i < ARRAY_LEN( extensions ); i++ )
		{
			if ( Str::IsISuffix( extensions[ i ], filename ) )
			{
				files[ i ].push_back( filename );
			}
		}
	}
	for ( auto& fileList : files )
	{
		if ( !fileList.empty() )
		{
			std::sort( fileList.begin(), fileList.end(), LightmapNameLess );
			return std::move( fileList );
		}
	}
	return {};
}

/*
===============
R_LoadLightmaps
===============
*/
static void R_LoadLightmaps( lump_t *l, const char *bspName )
{
	tr.worldLightMapping = r_precomputedLighting->integer && tr.lightMode == lightMode_t::MAP;

	/* All lightmaps will be loaded if either light mapping
	or deluxe mapping is enabled. */
	if ( !tr.worldLightMapping && !tr.worldDeluxeMapping )
	{
		return;
	}

	int lightMapBits = IF_LIGHTMAP | IF_NOPICMIP;
	int deluxeMapBits = IF_NORMALMAP | IF_NOPICMIP;

	if ( tr.worldLinearizeLightMap )
	{
		lightMapBits |= IF_SRGB;
	}

	int len = l->filelen;
	if ( !len )
	{
		char mapName[ MAX_QPATH ];

		Q_strncpyz( mapName, bspName, sizeof( mapName ) );
		COM_StripExtension3( mapName, mapName, sizeof( mapName ) );

		if ( tr.worldHDR_RGBE )
		{
			// we are about to upload textures
			R_SyncRenderThread();

			// load HDR lightmaps
			int  width, height;
			byte *ldrImage;

			std::vector<std::string> hdrFiles;
			for ( const std::string& filename : FS::PakPath::ListFiles( mapName ) )
			{
				if ( Str::IsISuffix( ".hdr", filename ) )
				{
					hdrFiles.push_back( filename );
				}
			}
			if ( hdrFiles.empty() )
			{
				Log::Warn("no lightmap files found");
				tr.worldLightMapping = false;
				tr.worldDeluxeMapping = false;
				return;
			}
			std::sort( hdrFiles.begin(), hdrFiles.end(), LightmapNameLess );

			for ( const std::string& filename : hdrFiles )
			{
				Log::Debug("...loading external lightmap as RGB8 LDR '%s/%s'", mapName, filename );

				width = height = 0;
				LoadRGBEToBytes( va( "%s/%s", mapName, filename.c_str() ), &ldrImage, &width, &height );

				imageParams_t imageParams = {};
				imageParams.bits = lightMapBits;
				imageParams.filterType = filterType_t::FT_DEFAULT;
				imageParams.wrapType = wrapTypeEnum_t::WT_CLAMP;

				auto image = R_CreateImage( va( "%s/%s", mapName, filename.c_str() ), (const byte **)&ldrImage, width, height, 1, imageParams );

				tr.lightmaps.push_back( image );

				Z_Free( ldrImage );
			}

			if (tr.worldDeluxeMapping) {
				// load deluxemaps
				std::vector<std::string> lightmapFiles = R_LoadExternalLightmaps(mapName);
				if (lightmapFiles.empty()) {
					Log::Warn("no lightmap files found");
					tr.worldLightMapping = false;
					tr.worldDeluxeMapping = false;
					return;
				}

				Log::Debug("...loading %i deluxemaps", lightmapFiles.size());

				for (const std::string& filename : lightmapFiles) {
					Log::Debug("...loading external lightmap '%s/%s'", mapName, filename);

					imageParams_t imageParams = {};
					imageParams.bits = deluxeMapBits;
					imageParams.filterType = filterType_t::FT_DEFAULT;
					imageParams.wrapType = wrapTypeEnum_t::WT_CLAMP;

					auto image = R_FindImageFile(va("%s/%s", mapName, filename.c_str()), imageParams);
					tr.deluxemaps.push_back(image);
				}
			}
		}
		else
		{
			std::vector<std::string> lightmapFiles = R_LoadExternalLightmaps(mapName);
			if (lightmapFiles.empty()) {
				Log::Warn("no lightmap files found");
				tr.worldLightMapping = false;
				tr.worldDeluxeMapping = false;
				return;
			}

			Log::Debug("...loading %i lightmaps", lightmapFiles.size() );

			// we are about to upload textures
			R_SyncRenderThread();

			for (size_t i = 0; i < lightmapFiles.size(); i++) {
				Log::Debug("...loading external lightmap '%s/%s'", mapName, lightmapFiles[i]);

				if (!tr.worldDeluxeMapping || i % 2 == 0) {
					imageParams_t imageParams = {};
					imageParams.bits = lightMapBits;
					imageParams.filterType = filterType_t::FT_LINEAR;
					imageParams.wrapType = wrapTypeEnum_t::WT_CLAMP;

					auto image = R_FindImageFile(va("%s/%s", mapName, lightmapFiles[i].c_str()), imageParams);
					tr.lightmaps.push_back(image);
				}
				else if (tr.worldDeluxeMapping)
				{
					imageParams_t imageParams = {};
					imageParams.bits = deluxeMapBits;
					imageParams.filterType = filterType_t::FT_LINEAR;
					imageParams.wrapType = wrapTypeEnum_t::WT_CLAMP;

					auto image = R_FindImageFile(va("%s/%s", mapName, lightmapFiles[i].c_str()), imageParams);
					tr.deluxemaps.push_back( image );
				}
			}
		}
	} else {
		len = l->filelen;

		if ( !len )
		{
			Log::Warn("no lightmap files found");
			tr.worldLightMapping = false;
			tr.worldDeluxeMapping = false;
			return;
		}

		byte *buf = fileBase + l->fileofs;

		// we are about to upload textures
		R_SyncRenderThread();

		const int internalLightMapSize = 128;

		// create all the lightmaps
		int numLightmaps = len / ( internalLightMapSize * internalLightMapSize * 3 );

		if ( numLightmaps == 1 )
		{
			//FIXME: HACK: maps with only one lightmap turn up fullbright for some reason.
			//this hack avoids that scenario, but isn't the correct solution.
			numLightmaps++;
		}
		else if ( numLightmaps >= MAX_LIGHTMAPS )
		{
			// 20051020 misantropia
			Log::Warn("number of lightmaps > MAX_LIGHTMAPS" );
			numLightmaps = MAX_LIGHTMAPS;
		}

		for ( int i = 0; i < numLightmaps; i++ )
		{
			byte *lightMapBuffer = (byte*) ri.Hunk_AllocateTempMemory( sizeof( byte ) * internalLightMapSize * internalLightMapSize * 4 );

			memset( lightMapBuffer, 128, internalLightMapSize * internalLightMapSize * 4 );

			// expand the 24 bit on-disk to 32 bit
			byte *buf_p = buf + i * internalLightMapSize * internalLightMapSize * 3;

			for ( int y = 0; y < internalLightMapSize; y++ )
			{
				for ( int x = 0; x < internalLightMapSize; x++ )
				{
					int index = x + ( y * internalLightMapSize );
					lightMapBuffer[( index * 4 ) + 0 ] = buf_p[( ( x + ( y * internalLightMapSize ) ) * 3 ) + 0 ];
					lightMapBuffer[( index * 4 ) + 1 ] = buf_p[( ( x + ( y * internalLightMapSize ) ) * 3 ) + 1 ];
					lightMapBuffer[( index * 4 ) + 2 ] = buf_p[( ( x + ( y * internalLightMapSize ) ) * 3 ) + 2 ];
					lightMapBuffer[( index * 4 ) + 3 ] = 255;

					R_ColorShiftLightingBytes( &lightMapBuffer[( index * 4 ) + 0 ] );
				}
			}

			imageParams_t imageParams = {};
			imageParams.bits = lightMapBits;
			imageParams.filterType = filterType_t::FT_DEFAULT;
			imageParams.wrapType = wrapTypeEnum_t::WT_CLAMP;

			image_t *internalLightMap = R_CreateImage( va( "_internalLightMap%d", i ), (const byte **)&lightMapBuffer, internalLightMapSize, internalLightMapSize, 1, imageParams );
			tr.lightmaps.push_back( internalLightMap );

			ri.Hunk_FreeTempMemory( lightMapBuffer );
		}
	}
}

/*
=================
RE_SetWorldVisData

This is called by the clipmodel subsystem so we can share the 1.8 megs of
space in big maps...
=================
*/
void RE_SetWorldVisData( const byte *vis )
{
	tr.externalVisData = vis;
}

/*
=================
R_LoadVisibility
=================
*/
static void R_LoadVisibility( lump_t *l )
{
	int  len, i, j, k;
	byte *buf;

	Log::Debug("...loading visibility" );

	len = ( s_worldData.numClusters + 63 ) & ~63;
	s_worldData.novis = (byte*) ri.Hunk_Alloc( len, ha_pref::h_low );
	memset( s_worldData.novis, 0xff, len );

	len = l->filelen;

	if ( !len )
	{
		return;
	}

	buf = fileBase + l->fileofs;

	s_worldData.numClusters = LittleLong( ( ( int * ) buf ) [ 0 ] );
	s_worldData.clusterBytes = LittleLong( ( ( int * ) buf ) [ 1 ] );

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	if ( tr.externalVisData )
	{
		s_worldData.vis = tr.externalVisData;
	}
	else
	{
		byte *dest;

		dest = (byte*) ri.Hunk_Alloc( len - 8, ha_pref::h_low );
		memcpy( dest, buf + 8, len - 8 );
		s_worldData.vis = dest;
	}

	// initialize visvis := vis
	len = s_worldData.numClusters * s_worldData.clusterBytes;
	s_worldData.visvis = (byte*) ri.Hunk_Alloc( len, ha_pref::h_low );
	memcpy( s_worldData.visvis, s_worldData.vis, len );

	for ( i = 0; i < s_worldData.numClusters; i++ )
	{
		const byte *src;
		const int *src2;
		byte *dest;

		src  = s_worldData.vis + i * s_worldData.clusterBytes;
		dest = s_worldData.visvis + i * s_worldData.clusterBytes;

		// for each byte in the current cluster's vis data
		for ( j = 0; j < s_worldData.clusterBytes; j++ )
		{
			byte bitbyte = src[ j ];

			if ( !bitbyte )
			{
				continue;
			}

			for ( k = 0; k < 8; k++ )
			{
				int index;

				// check if this cluster ( k = ( cluster & 7 ) ) is visible from the current cluster
				if ( ! ( bitbyte & ( 1 << k ) ) )
				{
					continue;
				}

				// retrieve vis data for the cluster
				index = ( ( j << 3 ) | k );
				src2 = ( int * ) ( s_worldData.vis + index * s_worldData.clusterBytes );

				// OR this vis data with the current cluster's
				for (unsigned m = 0; m < ( s_worldData.clusterBytes / sizeof( int ) ); m++ )
				{
					( ( int * ) dest )[ m ] |= src2[ m ];
				}
			}
		}
	}
}

//===============================================================================

// Generate the skybox mesh and add it to world
static void FinishSkybox() {
	// Min and max coordinates of the skybox cube corners
	static const vec3_t min = { -100.0, -100.0, -100.0 };
	static const vec3_t max = { 100.0, 100.0, 100.0 };
	/*
		Skybox is a static mesh with 8 vertices and 12 triangles

			      5------6
		 z	     /|     /|
		 ^ 	    / |    / |
		 |     4------7  |
		 |   y |  1---|--2
		 |  /  | /    | /
		 | /   |/     |/
		 |/    0------3
		 0---------->x  
		   Verts:
		   0: -100 -100 -100
		   1: -100 100 -100
		   2: 100 100 -100
		   3: 100 -100 -100
		   4: -100 -100 100
		   5: -100 100 100
		   6: 100 100 100
		   7: 100 -100 100
		   Surfs:
		   0: 0 2 1 / 0 3 2
		   1: 7 5 6 / 7 4 5
		   2: 0 1 5 / 0 5 4
		   3: 1 6 5 / 1 2 6
		   4: 2 7 6 / 2 3 7
		   5: 3 4 7 / 3 0 4
	*/

	drawSurf_t* skybox;
	skybox = ( drawSurf_t* ) ri.Hunk_Alloc( sizeof( *skybox ), ha_pref::h_low );
	skybox->entity = &tr.worldEntity;
	srfVBOMesh_t* surface;
	surface = ( srfVBOMesh_t* ) ri.Hunk_Alloc( sizeof( *surface ), ha_pref::h_low );
	surface->surfaceType = surfaceType_t::SF_VBO_MESH;
	surface->numVerts = 8;
	surface->numTriangles = 12;
	surface->firstIndex = 0;

	vec3_t verts[ 8 ] {
		{ min[0], min[1], min[2] },
		{ min[0], max[1], min[2] },
		{ max[0], max[1], min[2] },
		{ max[0], min[1], min[2] },
		{ min[0], min[1], max[2] },
		{ min[0], max[1], max[2] },
		{ max[0], max[1], max[2] },
		{ max[0], min[1], max[2] },
	};
	vertexAttributeSpec_t attr[] = {
		{ ATTR_INDEX_POSITION, GL_FLOAT, GL_FLOAT, verts, 3, sizeof( vec3_t ), 0 },
	};
	surface->vbo = R_CreateStaticVBO( "skybox_VBO", std::begin( attr ), std::end( attr ), surface->numVerts );

	glIndex_t indexes[36] = { 0, 2, 1,  0, 3, 2,   // Bottom
							  7, 5, 6,  7, 4, 5,   // Top
						      0, 1, 5,  0, 5, 4,   // Left
							  1, 6, 5,  1, 2, 6,   // Front
							  2, 7, 6,  2, 3, 7,   // Right
							  3, 4, 7,  3, 0, 4 }; // Back

	surface->ibo = R_CreateStaticIBO( "skybox_IBO", indexes, surface->numTriangles * 3 );

	SetupVAOBuffers( surface->vbo, surface->ibo,
		ATTR_POSITION,
		&surface->vbo->VAO );

	skybox->surface = ( surfaceType_t* ) surface;

	tr.skybox = skybox;
}

static shader_t* ShaderForShaderNum( int shaderNum ) {
	shaderNum = LittleLong( shaderNum ) + 0;  // silence the warning

	if ( shaderNum < 0 || shaderNum >= s_worldData.numShaders ) {
		Sys::Drop( "ShaderForShaderNum: bad num %i", shaderNum );
	}

	dshader_t* dsh = &s_worldData.shaders[shaderNum];

	shader_t* shader = R_FindShader( dsh->shader, RSF_3D );

	// If the shader had errors, just use default shader
	if ( shader->defaultShader ) {
		return tr.defaultShader;
	}

	return shader;
}

static void SphereFromBounds( vec3_t mins, vec3_t maxs, vec3_t origin, float* radius ) {
	vec3_t temp;

	VectorAdd( mins, maxs, origin );
	VectorScale( origin, 0.5, origin );
	VectorSubtract( maxs, origin, temp );
	*radius = VectorLength( temp );
}

static std::string VertexToString( srfVert_t* vertex ) {
	return Str::Format( "xyz: %f %f %f normal: %f %f %f uv: %f %f lightmap: %f %f", vertex->xyz[0], vertex->xyz[1], vertex->xyz[2],
		vertex->normal[0], vertex->normal[1], vertex->normal[2], vertex->st[0], vertex->st[1],
		vertex->lightmap[0], vertex->lightmap[1] );
}

void ValidateVertex( srfVert_t* vertex, int vertexID, shader_t* shader ) {
	for ( int i = 0; i < 3; i++ ) {
		/* NetRadiant only allows vertices in range [-65535, 65536],
		but some vertices still end up outside of that range, e.g: station15 sun */
		if ( !Math::IsFinite( vertex->xyz[i] ) || vertex->xyz[i] < -1e9 || vertex->xyz[i] > 1e9 ) {
			Log::Warn( "BSP: Bad position in vertex %i: %s, %s; setting to 0.0f", vertexID,
				VertexToString( vertex ), shader->name);
			vertex->xyz[i] = 0.0f;
		}

		if ( !Math::IsFinite( vertex->normal[i] ) ) {
			Log::Warn( "BSP: Bad vertex normal %i: %s, %s; setting to 0.0f", vertexID,
				VertexToString( vertex ), shader->name );
			vertex->normal[i] = 0.0f;

			VectorNormalize( vertex->normal );
		}
	}

	for ( int i = 0; i < 2; i++ ) {
		if ( !Math::IsFinite( vertex->st[i] ) ) {
			Log::Warn( "BSP: Bad uv in vertex %i: %s, %s; setting to 0.0f", vertexID,
				VertexToString( vertex ), shader->name );
			vertex->st[i] = 0.0f;
		}

		// q3map2 sometimes produces garbage lightmap uv's on patch meshes and on surfaces that don't seem to use lightmaps
		if ( !Math::IsFinite( vertex->lightmap[i] ) || vertex->lightmap[i] < -1.0f || vertex->lightmap[i] > 1.0f ) {
			/* Bad lightmap uv's on surfaces that don't use lightmapping seems to be very common in q3map2,
			so don't spam the log with them. We still need to fix such values though */
			for ( const shaderStage_t* pStage = shader->stages; pStage < shader->lastStage; pStage++ ) {
				if ( pStage->colorRenderer == &Render_lightMapping && !hasExplicitelyDisabledLightMap( shader ) ) {
					Log::Warn( "BSP: Bad lightmap in vertex %i: %s, %s; setting to 0.0f", vertexID,
						VertexToString( vertex ), shader->name );
					break;
				}
			}

			vertex->lightmap[i] = 0.0f;
		}
	}
}

static void ParseTriangleSurface( dsurface_t* ds, drawVert_t* verts, bspSurface_t* surf, int* indexes ) {
	int realLightmapNum = LittleLong( ds->lightmapNum );

	if ( tr.worldLightMapping || tr.worldDeluxeMapping ) {
		surf->lightmapNum = realLightmapNum;
	} else {
		surf->lightmapNum = -1;
	}

	if ( tr.worldDeluxeMapping && surf->lightmapNum >= 2 ) {
		surf->lightmapNum /= 2;
	}

	surf->shader = ShaderForShaderNum( ds->shaderNum );

	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	// We may have a nodraw surface, because they might still need to be around for movement clipping
	if ( s_worldData.shaders[LittleLong( ds->shaderNum )].surfaceFlags & SURF_NODRAW ) {
		static surfaceType_t skipData = surfaceType_t::SF_SKIP;
		surf->data = &skipData;
		return;
	}

	srfGeneric_t* cv = ( srfGeneric_t* ) ri.Hunk_Alloc( sizeof( *cv ), ha_pref::h_low );
	cv->surfaceType = surfaceType_t::SF_BAD; // Will be set later by ParseFace() or ParseTriSurf()

	cv->numTriangles = LittleLong( ds->numIndexes ) / 3;
	cv->triangles = ( srfTriangle_t* ) ri.Hunk_Alloc( cv->numTriangles * sizeof( cv->triangles[ 0 ] ), ha_pref::h_low );

	cv->numVerts = LittleLong( ds->numVerts );
	cv->verts = ( srfVert_t* ) ri.Hunk_Alloc( cv->numVerts * sizeof( cv->verts[ 0 ] ), ha_pref::h_low );

	surf->data = ( surfaceType_t* ) cv;

	// Copy vertexes
	ClearBounds( cv->bounds[ 0 ], cv->bounds[ 1 ] );
	verts += LittleLong( ds->firstVert );

	struct vertexComponent_t {
		vec2_t stBounds[ 2 ];
		int minVertex;
	};
	vertexComponent_t* components = ( vertexComponent_t* ) ri.Hunk_AllocateTempMemory( cv->numVerts * sizeof( vertexComponent_t ) );

	for ( int i = 0; i < cv->numVerts; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			cv->verts[ i ].xyz[ j ] = LittleFloat( verts[ i ].xyz[ j ] );
			cv->verts[ i ].normal[ j ] = LittleFloat( verts[ i ].normal[ j ] );
		}

		components[ i ].minVertex = i;

		for ( int j = 0; j < 2; j++ ) {
			cv->verts[ i ].st[ j ] = LittleFloat( verts[ i ].st[ j ] );
			cv->verts[ i ].lightmap[ j ] = LittleFloat( verts[ i ].lightmap[ j ] );
		}

		cv->verts[ i ].lightmap[ 0 ] = LittleFloat( verts[ i ].lightmap[ 0 ] );
		cv->verts[ i ].lightmap[ 1 ] = LittleFloat( verts[ i ].lightmap[ 1 ] );

		ValidateVertex( &cv->verts[i], ds->firstVert + i, surf->shader );

		AddPointToBounds( cv->verts[ i ].xyz, cv->bounds[ 0 ], cv->bounds[ 1 ] );

		for( int j = 0; j < 2; j++ ) {
			components[ i ].stBounds[ 0 ][ j ] = cv->verts[ i ].st[ j ];
			components[ i ].stBounds[ 1 ][ j ] = cv->verts[ i ].st[ j ];
		}

		cv->verts[ i ].lightColor = Color::Adapt( verts[ i ].color );

		R_LinearizeLightingColorBytes( cv->verts[ i ].lightColor.ToArray() );

		R_ColorShiftLightingBytes( cv->verts[ i ].lightColor.ToArray() );
	}

	// Copy triangles
	indexes += LittleLong( ds->firstIndex );

	for ( int i = 0; i < cv->numTriangles; i++ ) {
		srfTriangle_t* tri = &cv->triangles[ i ];
		for ( int j = 0; j < 3; j++ ) {
			tri->indexes[ j ] = LittleLong( indexes[ i * 3 + j ] );

			if ( tri->indexes[j] < 0 || tri->indexes[j] >= cv->numVerts ) {
				Sys::Drop( "Index out of range in surface: %i not in [0, %i] (index in BSP: %i, surface: indexes: %i-%i, shader: %s)",
					tri->indexes[j], cv->numVerts, i * 3 + j, ds->firstIndex, ds->firstIndex + ds->numIndexes, surf->shader->name );
			}
		}
	}

	// Compute strongly connected components and TC bounds per component
	bool updated;
	do {
		updated = false;

		for ( int i = 0; i < cv->numTriangles; i++ ) {
			srfTriangle_t* tri = &cv->triangles[ i ];
			int minVertex = std::min( std::min( components[ tri->indexes[ 0 ] ].minVertex,
				components[ tri->indexes[ 1 ] ].minVertex ),
				components[ tri->indexes[ 2 ] ].minVertex );
			for ( int j = 0; j < 3; j++ ) {
				int vertex = tri->indexes[ j ];
				if ( components[ vertex ].minVertex != minVertex ) {
					updated = true;
					components[ vertex ].minVertex = minVertex;
					components[ minVertex ].stBounds[ 0 ][ 0 ] = std::min( components[ minVertex ].stBounds[ 0 ][ 0 ],
						components[ vertex ].stBounds[ 0 ][ 0 ] );
					components[ minVertex ].stBounds[ 0 ][ 1 ] = std::min( components[ minVertex ].stBounds[ 0 ][ 1 ],
						components[ vertex ].stBounds[ 0 ][ 1 ] );
					components[ minVertex ].stBounds[ 1 ][ 0 ] = std::max( components[ minVertex ].stBounds[ 1 ][ 0 ],
						components[ vertex ].stBounds[ 1 ][ 0 ] );
					components[ minVertex ].stBounds[ 1 ][ 1 ] = std::max( components[ minVertex ].stBounds[ 1 ][ 1 ],
						components[ vertex ].stBounds[ 1 ][ 1 ] );
				}
			}
		}
	} while ( updated );

	// Center texture coords
	for ( int i = 0; i < cv->numVerts; i++ ) {
		if ( components[ i ].minVertex == i ) {
			for ( int j = 0; j < 2; j++ ) {
				/* Some words about the “minus 0.5” trick:
				 *
				 * The vertexpack engine tries to optimize texture coords, because fp16 numbers have 1/2048 resolution only between -1.0 and 1.0,
				 * and so we could have several texel rounding errors if the coords are too large. The optimization finds connected surfaces and
				 * then shifts the texture coords so that their average value is near 0. If the range is 0-1, there are two equally good solutions,
				 * either keep the range 0-1 or shift to -1-0, and in this case tiny rounding errors decide which one is used.
				 * For non-animated textures this is no issue, but the rotating textures always rotate around 0.5/0.5, and if the texture coords
				 * are shifted, it shifts the rotation center too. The easy fix is to move the average as close as possible to 0.5 instead of 0.0,
				 * then all standard 0-1 textures will not be shifted.
				 * -- @gimhael https://github.com/DaemonEngine/Daemon/issues/35#issuecomment-507406783
				 *
				 * Instead of round(x - 0.5) we can do floor(x). (using std::floorf)
				 * Surprisingly to me, rintf is apparently a builtin in gcc (so is floorf, but not roundf).
				 * And rounding x - 0.5 actually generates fewer instructions. So it looks like the original version is pretty great.
				 * -- @slipher https://github.com/DaemonEngine/Daemon/pull/208#discussion_r299864660
				 */
				components[ i ].stBounds[ 0 ][ j ] =
					rintf( 0.5f * ( components[ i ].stBounds[ 1 ][ j ] + components[ i ].stBounds[ 0 ][ j ] ) - 0.5f );
			}
		}

		for ( int j = 0; j < 2; j++ ) {
			cv->verts[ i ].st[ j ] -= components[ components[ i ].minVertex ].stBounds[ 0 ][ j ];
		}
	}

	ri.Hunk_FreeTempMemory( components );

	surf->data = ( surfaceType_t* ) cv;

	for ( int i = 0; i < cv->numTriangles; i++ ) {
		srfTriangle_t* tri = &cv->triangles[ i ];
		srfVert_t* dv0 = &cv->verts[ tri->indexes[ 0 ] ];
		srfVert_t* dv1 = &cv->verts[ tri->indexes[ 1 ] ];
		srfVert_t* dv2 = &cv->verts[ tri->indexes[ 2 ] ];

		vec3_t tangent, binormal;
		R_CalcTangents( tangent, binormal,
			dv0->xyz, dv1->xyz, dv2->xyz,
			dv0->st, dv1->st, dv2->st );
		R_TBNtoQtangents( tangent, binormal, dv0->normal,
			dv0->qtangent );
		R_TBNtoQtangents( tangent, binormal, dv1->normal,
			dv1->qtangent );
		R_TBNtoQtangents( tangent, binormal, dv2->normal,
			dv2->qtangent );
	}

	SphereFromBounds( cv->bounds[0], cv->bounds[1], cv->origin, &cv->radius );
}

static void ParseFace( dsurface_t* ds, drawVert_t* verts, bspSurface_t* surf, int* indexes ) {
	ParseTriangleSurface( ds, verts, surf, indexes );

	srfGeneric_t* surface = ( srfGeneric_t* ) surf->data;
	if ( surface->surfaceType == surfaceType_t::SF_SKIP ) {
		return;
	}

	surface->surfaceType = surfaceType_t::SF_FACE;

	// take the plane information from the lightmap vector
	for ( int i = 0; i < 3; i++ ) {
		surface->plane.normal[ i ] = LittleFloat( ds->lightmapVecs[ 2 ][ i ] );
	}

	surface->plane.dist = DotProduct( surface->verts[ 0 ].xyz, surface->plane.normal );
	SetPlaneSignbits( &surface->plane );
	surface->plane.type = PlaneTypeForNormal( surface->plane.normal );
}

static void ParseTriSurf( dsurface_t* ds, drawVert_t* verts, bspSurface_t* surf, int* indexes ) {
	ParseTriangleSurface( ds, verts, surf, indexes );

	srfGeneric_t* surface = ( srfGeneric_t* ) surf->data;
	if ( surface->surfaceType == surfaceType_t::SF_SKIP ) {
		return;
	}

	surface->surfaceType = surfaceType_t::SF_TRIANGLES;

	plane_t plane;
	srfVert_t* v1 = surface->verts + surface->triangles[0].indexes[0];
	srfVert_t* v2 = surface->verts + surface->triangles[0].indexes[1];
	srfVert_t* v3 = surface->verts + surface->triangles[0].indexes[2];

	PlaneFromPoints( plane, v1->xyz, v2->xyz, v3->xyz );
	VectorCopy( plane.normal, surface->plane.normal );
	surface->plane.dist = plane.dist;
}

static void ParseMesh( dsurface_t *ds, drawVert_t *verts, bspSurface_t *surf )
{
	srfGridMesh_t        *grid;
	int                  width, height, numPoints;
	static srfVert_t     points[ MAX_PATCH_SIZE * MAX_PATCH_SIZE ];
	vec3_t               bounds[ 2 ];
	vec2_t               stBounds[ 2 ], tcOffset;
	vec3_t               tmpVec;
	static surfaceType_t skipData = surfaceType_t::SF_SKIP;
	int                  realLightmapNum;

	// get lightmap
	realLightmapNum = LittleLong( ds->lightmapNum );

	if ( tr.worldLightMapping || tr.worldDeluxeMapping )
	{
		surf->lightmapNum = realLightmapNum;
	}
	else
	{
		surf->lightmapNum = -1;
	}

	if ( tr.worldDeluxeMapping && surf->lightmapNum >= 2 )
	{
		surf->lightmapNum /= 2;
	}

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum );

	if ( r_singleShader->integer && !surf->shader->isSky )
	{
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW )
	{
		surf->data = &skipData;
		return;
	}

	width = LittleLong( ds->patchWidth );
	height = LittleLong( ds->patchHeight );

	if ( width < 0 || width > MAX_PATCH_SIZE || height < 0 || height > MAX_PATCH_SIZE )
	{
		Sys::Drop( "ParseMesh: bad size: width: %i (range: [0, %i]), height: %i (range: [0, %i]), surface: firstVertex: %i,"
		" shader: %s", width, MAX_PATCH_SIZE, height, MAX_PATCH_SIZE, ds->firstVert, surf->shader->name );
	}

	verts += LittleLong( ds->firstVert );
	numPoints = width * height;

	// compute min/max texture coords on the fly
	stBounds[ 0 ][ 0 ] =  99999.0f;
	stBounds[ 0 ][ 1 ] =  99999.0f;
	stBounds[ 1 ][ 0 ] = -99999.0f;
	stBounds[ 1 ][ 1 ] = -99999.0f;

	for ( int i = 0; i < numPoints; i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			points[ i ].xyz[ j ] = LittleFloat( verts[ i ].xyz[ j ] );
			points[ i ].normal[ j ] = LittleFloat( verts[ i ].normal[ j ] );
		}

		for ( int j = 0; j < 2; j++ )
		{
			points[ i ].st[ j ] = LittleFloat( verts[ i ].st[ j ] );
			points[ i ].lightmap[ j ] = LittleFloat( verts[ i ].lightmap[ j ] );
		}

		points[ i ].lightmap[ 0 ] = LittleFloat( verts[ i ].lightmap[ 0 ] );
		points[ i ].lightmap[ 1 ] = LittleFloat( verts[ i ].lightmap[ 1 ] );

		ValidateVertex( &points[i], ds->firstVert + i, surf->shader );

		for( int j = 0; j < 2; j++ ) {
			stBounds[ 0 ][ j ] = std::min( stBounds[ 0 ][ j ], points[ i ].st[ j ] );
			stBounds[ 1 ][ j ] = std::max( stBounds[ 1 ][ j ], points[ i ].st[ j ] );
		}

		points[ i ].lightColor = Color::Adapt( verts[ i ].color );

		R_LinearizeLightingColorBytes( points[ i ].lightColor.ToArray() );

		R_ColorShiftLightingBytes( points[ i ].lightColor.ToArray() );
	}

	// center texture coords
	for( int j = 0; j < 2; j++ ) {
		tcOffset[ j ] = 0.5f * (stBounds[ 1 ][ j ] + stBounds[ 0 ][ j ]);
		tcOffset[ j ] = rintf( tcOffset[ j ] );
	}

	for ( int i = 0; i < numPoints; i++ )
	{
		for ( int j = 0; j < 2; j++ )
		{
			points[ i ].st[ j ] -= tcOffset[ j ];
		}
	}

	// pre-tesselate
	grid = R_SubdividePatchToGrid( width, height, points );
	surf->data = ( surfaceType_t * ) grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for ( int i = 0; i < 3; i++ )
	{
		bounds[ 0 ][ i ] = LittleFloat( ds->lightmapVecs[ 0 ][ i ] );
		bounds[ 1 ][ i ] = LittleFloat( ds->lightmapVecs[ 1 ][ i ] );
	}

	VectorAdd( bounds[ 0 ], bounds[ 1 ], bounds[ 1 ] );
	VectorScale( bounds[ 1 ], 0.5f, grid->lodOrigin );
	VectorSubtract( bounds[ 0 ], grid->lodOrigin, tmpVec );
	grid->lodRadius = VectorLength( tmpVec );

	SphereFromBounds( grid->bounds[0], grid->bounds[1], grid->origin, &grid->radius );
}

/*
=================
R_MergedWidthPoints

returns true if there are grid points merged on a width edge
=================
*/
int R_MergedWidthPoints( srfGridMesh_t *grid, int offset )
{
	int i, j;

	for ( i = 1; i < grid->width - 1; i++ )
	{
		for ( j = i + 1; j < grid->width - 1; j++ )
		{
			if ( fabsf( grid->verts[ i + offset ].xyz[ 0 ] - grid->verts[ j + offset ].xyz[ 0 ] ) > 0.1f )
			{
				continue;
			}

			if ( fabsf( grid->verts[ i + offset ].xyz[ 1 ] - grid->verts[ j + offset ].xyz[ 1 ] ) > 0.1f )
			{
				continue;
			}

			if ( fabsf( grid->verts[ i + offset ].xyz[ 2 ] - grid->verts[ j + offset ].xyz[ 2 ] ) > 0.1f )
			{
				continue;
			}

			return true;
		}
	}

	return false;
}

/*
=================
R_MergedHeightPoints

returns true if there are grid points merged on a height edge
=================
*/
int R_MergedHeightPoints( srfGridMesh_t *grid, int offset )
{
	int i, j;

	for ( i = 1; i < grid->height - 1; i++ )
	{
		for ( j = i + 1; j < grid->height - 1; j++ )
		{
			if ( fabsf( grid->verts[ grid->width * i + offset ].xyz[ 0 ] - grid->verts[ grid->width * j + offset ].xyz[ 0 ] ) > 0.1f )
			{
				continue;
			}

			if ( fabsf( grid->verts[ grid->width * i + offset ].xyz[ 1 ] - grid->verts[ grid->width * j + offset ].xyz[ 1 ] ) > 0.1f )
			{
				continue;
			}

			if ( fabsf( grid->verts[ grid->width * i + offset ].xyz[ 2 ] - grid->verts[ grid->width * j + offset ].xyz[ 2 ] ) > 0.1f )
			{
				continue;
			}

			return true;
		}
	}

	return false;
}

/*
=================
R_FixSharedVertexLodError_r

NOTE: never sync LoD through grid edges with merged points!

FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
=================
*/
void R_FixSharedVertexLodError_r( int start, srfGridMesh_t *grid1 )
{
	int           j, k, l, m, n, offset1, offset2, touch;
	srfGridMesh_t *grid2;

	for ( j = start; j < s_worldData.numSurfaces; j++ )
	{
		//
		grid2 = ( srfGridMesh_t * ) s_worldData.surfaces[ j ].data;

		// if this surface is not a grid
		if ( grid2->surfaceType != surfaceType_t::SF_GRID )
		{
			continue;
		}

		// if the LOD errors are already fixed for this patch
		if ( grid2->lodFixed == 2 )
		{
			continue;
		}

		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius )
		{
			continue;
		}

		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[ 0 ] != grid2->lodOrigin[ 0 ] )
		{
			continue;
		}

		if ( grid1->lodOrigin[ 1 ] != grid2->lodOrigin[ 1 ] )
		{
			continue;
		}

		if ( grid1->lodOrigin[ 2 ] != grid2->lodOrigin[ 2 ] )
		{
			continue;
		}

		//
		touch = false;

		for ( n = 0; n < 2; n++ )
		{
			//
			if ( n )
			{
				offset1 = ( grid1->height - 1 ) * grid1->width;
			}
			else
			{
				offset1 = 0;
			}

			if ( R_MergedWidthPoints( grid1, offset1 ) )
			{
				continue;
			}

			for ( k = 1; k < grid1->width - 1; k++ )
			{
				for ( m = 0; m < 2; m++ )
				{
					if ( m )
					{
						offset2 = ( grid2->height - 1 ) * grid2->width;
					}
					else
					{
						offset2 = 0;
					}

					if ( R_MergedWidthPoints( grid2, offset2 ) )
					{
						continue;
					}

					for ( l = 1; l < grid2->width - 1; l++ )
					{
						//
						if ( fabsf( grid1->verts[ k + offset1 ].xyz[ 0 ] - grid2->verts[ l + offset2 ].xyz[ 0 ] ) > 0.1f )
						{
							continue;
						}

						if ( fabsf( grid1->verts[ k + offset1 ].xyz[ 1 ] - grid2->verts[ l + offset2 ].xyz[ 1 ] ) > 0.1f )
						{
							continue;
						}

						if ( fabsf( grid1->verts[ k + offset1 ].xyz[ 2 ] - grid2->verts[ l + offset2 ].xyz[ 2 ] ) > 0.1f )
						{
							continue;
						}

						// ok the points are equal and should have the same lod error
						grid2->widthLodError[ l ] = grid1->widthLodError[ k ];
						touch = true;
					}
				}

				for ( m = 0; m < 2; m++ )
				{
					if ( m )
					{
						offset2 = grid2->width - 1;
					}
					else
					{
						offset2 = 0;
					}

					if ( R_MergedHeightPoints( grid2, offset2 ) )
					{
						continue;
					}

					for ( l = 1; l < grid2->height - 1; l++ )
					{
						//
						if ( fabsf( grid1->verts[ k + offset1 ].xyz[ 0 ] - grid2->verts[ grid2->width * l + offset2 ].xyz[ 0 ] ) > 0.1f )
						{
							continue;
						}

						if ( fabsf( grid1->verts[ k + offset1 ].xyz[ 1 ] - grid2->verts[ grid2->width * l + offset2 ].xyz[ 1 ] ) > 0.1f )
						{
							continue;
						}

						if ( fabsf( grid1->verts[ k + offset1 ].xyz[ 2 ] - grid2->verts[ grid2->width * l + offset2 ].xyz[ 2 ] ) > 0.1f )
						{
							continue;
						}

						// ok the points are equal and should have the same lod error
						grid2->heightLodError[ l ] = grid1->widthLodError[ k ];
						touch = true;
					}
				}
			}
		}

		for ( n = 0; n < 2; n++ )
		{
			//
			if ( n )
			{
				offset1 = grid1->width - 1;
			}
			else
			{
				offset1 = 0;
			}

			if ( R_MergedHeightPoints( grid1, offset1 ) )
			{
				continue;
			}

			for ( k = 1; k < grid1->height - 1; k++ )
			{
				for ( m = 0; m < 2; m++ )
				{
					if ( m )
					{
						offset2 = ( grid2->height - 1 ) * grid2->width;
					}
					else
					{
						offset2 = 0;
					}

					if ( R_MergedWidthPoints( grid2, offset2 ) )
					{
						continue;
					}

					for ( l = 1; l < grid2->width - 1; l++ )
					{
						//
						if ( fabsf( grid1->verts[ grid1->width * k + offset1 ].xyz[ 0 ] - grid2->verts[ l + offset2 ].xyz[ 0 ] ) > 0.1f )
						{
							continue;
						}

						if ( fabsf( grid1->verts[ grid1->width * k + offset1 ].xyz[ 1 ] - grid2->verts[ l + offset2 ].xyz[ 1 ] ) > 0.1f )
						{
							continue;
						}

						if ( fabsf( grid1->verts[ grid1->width * k + offset1 ].xyz[ 2 ] - grid2->verts[ l + offset2 ].xyz[ 2 ] ) > 0.1f )
						{
							continue;
						}

						// ok the points are equal and should have the same lod error
						grid2->widthLodError[ l ] = grid1->heightLodError[ k ];
						touch = true;
					}
				}

				for ( m = 0; m < 2; m++ )
				{
					if ( m )
					{
						offset2 = grid2->width - 1;
					}
					else
					{
						offset2 = 0;
					}

					if ( R_MergedHeightPoints( grid2, offset2 ) )
					{
						continue;
					}

					for ( l = 1; l < grid2->height - 1; l++ )
					{
						//
						if ( fabsf
						     ( grid1->verts[ grid1->width * k + offset1 ].xyz[ 0 ] -
						       grid2->verts[ grid2->width * l + offset2 ].xyz[ 0 ] ) > 0.1f )
						{
							continue;
						}

						if ( fabsf
						     ( grid1->verts[ grid1->width * k + offset1 ].xyz[ 1 ] -
						       grid2->verts[ grid2->width * l + offset2 ].xyz[ 1 ] ) > 0.1f )
						{
							continue;
						}

						if ( fabsf
						     ( grid1->verts[ grid1->width * k + offset1 ].xyz[ 2 ] -
						       grid2->verts[ grid2->width * l + offset2 ].xyz[ 2 ] ) > 0.1f )
						{
							continue;
						}

						// ok the points are equal and should have the same lod error
						grid2->heightLodError[ l ] = grid1->heightLodError[ k ];
						touch = true;
					}
				}
			}
		}

		if ( touch )
		{
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r( start, grid2 );
			//NOTE: this would be correct but makes things really slow
		}
	}
}

/*
=================
R_FixSharedVertexLodError

This function assumes that all patches in one group are nicely stitched together for the highest LoD.
If this is not the case this function will still do its job but won't fix the highest LoD cracks.
=================
*/
void R_FixSharedVertexLodError()
{
	int           i;
	srfGridMesh_t *grid1;

	for ( i = 0; i < s_worldData.numSurfaces; i++ )
	{
		//
		grid1 = ( srfGridMesh_t * ) s_worldData.surfaces[ i ].data;

		// if this surface is not a grid
		if ( grid1->surfaceType != surfaceType_t::SF_GRID )
		{
			continue;
		}

		//
		if ( grid1->lodFixed )
		{
			continue;
		}

		//
		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r( i + 1, grid1 );
	}
}

/*
===============
R_StitchPatches
===============
*/
int R_StitchPatches( int grid1num, int grid2num )
{
	float         *v1, *v2;
	srfGridMesh_t *grid1, *grid2;
	int           k, l, m, n, offset1, offset2, row, column;

	grid1 = ( srfGridMesh_t * ) s_worldData.surfaces[ grid1num ].data;
	grid2 = ( srfGridMesh_t * ) s_worldData.surfaces[ grid2num ].data;

	for ( n = 0; n < 2; n++ )
	{
		//
		if ( n )
		{
			offset1 = ( grid1->height - 1 ) * grid1->width;
		}
		else
		{
			offset1 = 0;
		}

		if ( R_MergedWidthPoints( grid1, offset1 ) )
		{
			continue;
		}

		for ( k = 0; k < grid1->width - 2; k += 2 )
		{
			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->width >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = ( grid2->height - 1 ) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->width - 1; l++ )
				{
					//
					v1 = grid1->verts[ k + offset1 ].xyz;
					v2 = grid2->verts[ l + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					v1 = grid1->verts[ k + 2 + offset1 ].xyz;
					v2 = grid2->verts[ l + 1 + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					//
					v1 = grid2->verts[ l + offset2 ].xyz;
					v2 = grid2->verts[ l + 1 + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) < 0.01f && fabsf( v1[ 1 ] - v2[ 1 ] ) < 0.01f && fabsf( v1[ 2 ] - v2[ 2 ] ) < 0.01f )
					{
						continue;
					}

					//
					// insert column into grid2 right after after column l
					if ( m )
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}

					grid2 = R_GridInsertColumn( grid2, l + 1, row, grid1->verts[ k + 1 + offset1 ].xyz, grid1->widthLodError[ k + 1 ] );
					grid2->lodStitched = false;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return true;
				}
			}

			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->height >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->height - 1; l++ )
				{
					//
					v1 = grid1->verts[ k + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * l + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					v1 = grid1->verts[ k + 2 + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					//
					v1 = grid2->verts[ grid2->width * l + offset2 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) < 0.01f && fabsf( v1[ 1 ] - v2[ 1 ] ) < 0.01f && fabsf( v1[ 2 ] - v2[ 2 ] ) < 0.01f )
					{
						continue;
					}

					//
					// insert row into grid2 right after after row l
					if ( m )
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}

					grid2 = R_GridInsertRow( grid2, l + 1, column, grid1->verts[ k + 1 + offset1 ].xyz, grid1->widthLodError[ k + 1 ] );
					grid2->lodStitched = false;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return true;
				}
			}
		}
	}

	for ( n = 0; n < 2; n++ )
	{
		//
		if ( n )
		{
			offset1 = grid1->width - 1;
		}
		else
		{
			offset1 = 0;
		}

		if ( R_MergedHeightPoints( grid1, offset1 ) )
		{
			continue;
		}

		for ( k = 0; k < grid1->height - 2; k += 2 )
		{
			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->width >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = ( grid2->height - 1 ) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->width - 1; l++ )
				{
					//
					v1 = grid1->verts[ grid1->width * k + offset1 ].xyz;
					v2 = grid2->verts[ l + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					v1 = grid1->verts[ grid1->width * ( k + 2 ) + offset1 ].xyz;
					v2 = grid2->verts[ l + 1 + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					//
					v1 = grid2->verts[ l + offset2 ].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) < 0.01f && fabsf( v1[ 1 ] - v2[ 1 ] ) < 0.01f && fabsf( v1[ 2 ] - v2[ 2 ] ) < 0.01f )
					{
						continue;
					}

					//
					// insert column into grid2 right after after column l
					if ( m )
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}

					grid2 = R_GridInsertColumn( grid2, l + 1, row,
					                            grid1->verts[ grid1->width * ( k + 1 ) + offset1 ].xyz, grid1->heightLodError[ k + 1 ] );
					grid2->lodStitched = false;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return true;
				}
			}

			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->height >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->height - 1; l++ )
				{
					//
					v1 = grid1->verts[ grid1->width * k + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * l + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					v1 = grid1->verts[ grid1->width * ( k + 2 ) + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					//
					v1 = grid2->verts[ grid2->width * l + offset2 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) < 0.01f && fabsf( v1[ 1 ] - v2[ 1 ] ) < 0.01f && fabsf( v1[ 2 ] - v2[ 2 ] ) < 0.01f )
					{
						continue;
					}

					//
					// insert row into grid2 right after after row l
					if ( m )
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}

					grid2 = R_GridInsertRow( grid2, l + 1, column,
					                         grid1->verts[ grid1->width * ( k + 1 ) + offset1 ].xyz, grid1->heightLodError[ k + 1 ] );
					grid2->lodStitched = false;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return true;
				}
			}
		}
	}

	for ( n = 0; n < 2; n++ )
	{
		//
		if ( n )
		{
			offset1 = ( grid1->height - 1 ) * grid1->width;
		}
		else
		{
			offset1 = 0;
		}

		if ( R_MergedWidthPoints( grid1, offset1 ) )
		{
			continue;
		}

		for ( k = grid1->width - 1; k > 1; k -= 2 )
		{
			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->width >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = ( grid2->height - 1 ) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->width - 1; l++ )
				{
					//
					v1 = grid1->verts[ k + offset1 ].xyz;
					v2 = grid2->verts[ l + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					v1 = grid1->verts[ k - 2 + offset1 ].xyz;
					v2 = grid2->verts[ l + 1 + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					//
					v1 = grid2->verts[ l + offset2 ].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) < 0.01f && fabsf( v1[ 1 ] - v2[ 1 ] ) < 0.01f && fabsf( v1[ 2 ] - v2[ 2 ] ) < 0.01f )
					{
						continue;
					}

					//
					// insert column into grid2 right after after column l
					if ( m )
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}

					grid2 = R_GridInsertColumn( grid2, l + 1, row, grid1->verts[ k - 1 + offset1 ].xyz, grid1->widthLodError[ k - 1 ] );
					grid2->lodStitched = false;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return true;
				}
			}

			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->height >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->height - 1; l++ )
				{
					//
					v1 = grid1->verts[ k + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * l + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					v1 = grid1->verts[ k - 2 + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					//
					v1 = grid2->verts[ grid2->width * l + offset2 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) < 0.01f && fabsf( v1[ 1 ] - v2[ 1 ] ) < 0.01f && fabsf( v1[ 2 ] - v2[ 2 ] ) < 0.01f )
					{
						continue;
					}

					//
					// insert row into grid2 right after after row l
					if ( m )
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}

					grid2 = R_GridInsertRow( grid2, l + 1, column, grid1->verts[ k - 1 + offset1 ].xyz, grid1->widthLodError[ k - 1 ] );

					if ( !grid2 )
					{
						break;
					}

					grid2->lodStitched = false;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return true;
				}
			}
		}
	}

	for ( n = 0; n < 2; n++ )
	{
		//
		if ( n )
		{
			offset1 = grid1->width - 1;
		}
		else
		{
			offset1 = 0;
		}

		if ( R_MergedHeightPoints( grid1, offset1 ) )
		{
			continue;
		}

		for ( k = grid1->height - 1; k > 1; k -= 2 )
		{
			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->width >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = ( grid2->height - 1 ) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->width - 1; l++ )
				{
					//
					v1 = grid1->verts[ grid1->width * k + offset1 ].xyz;
					v2 = grid2->verts[ l + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					v1 = grid1->verts[ grid1->width * ( k - 2 ) + offset1 ].xyz;
					v2 = grid2->verts[ l + 1 + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					//
					v1 = grid2->verts[ l + offset2 ].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) < 0.01f && fabsf( v1[ 1 ] - v2[ 1 ] ) < 0.01f && fabsf( v1[ 2 ] - v2[ 2 ] ) < 0.01f )
					{
						continue;
					}

					//
					// insert column into grid2 right after after column l
					if ( m )
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}

					grid2 = R_GridInsertColumn( grid2, l + 1, row,
					                            grid1->verts[ grid1->width * ( k - 1 ) + offset1 ].xyz, grid1->heightLodError[ k - 1 ] );
					grid2->lodStitched = false;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return true;
				}
			}

			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->height >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->height - 1; l++ )
				{
					//
					v1 = grid1->verts[ grid1->width * k + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * l + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					v1 = grid1->verts[ grid1->width * ( k - 2 ) + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 1 ] - v2[ 1 ] ) > 0.1f )
					{
						continue;
					}

					if ( fabsf( v1[ 2 ] - v2[ 2 ] ) > 0.1f )
					{
						continue;
					}

					//
					v1 = grid2->verts[ grid2->width * l + offset2 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabsf( v1[ 0 ] - v2[ 0 ] ) < 0.01f && fabsf( v1[ 1 ] - v2[ 1 ] ) < 0.01f && fabsf( v1[ 2 ] - v2[ 2 ] ) < 0.01f )
					{
						continue;
					}

					//
					// insert row into grid2 right after after row l
					if ( m )
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}

					grid2 = R_GridInsertRow( grid2, l + 1, column,
					                         grid1->verts[ grid1->width * ( k - 1 ) + offset1 ].xyz, grid1->heightLodError[ k - 1 ] );
					grid2->lodStitched = false;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return true;
				}
			}
		}
	}

	return false;
}

/*
===============
R_TryStitchingPatch

This function will try to stitch patches in the same LoD group together for the highest LoD.

Only single missing vertice cracks will be fixed.

Vertices will be joined at the patch side a crack is first found, at the other side
of the patch (on the same row or column) the vertices will not be joined and cracks
might still appear at that side.
===============
*/
int R_TryStitchingPatch( int grid1num )
{
	int           j, numstitches;

	numstitches = 0;

	for ( j = 0; j < s_worldData.numSurfaces; j++ )
	{
		srfGridMesh_t *grid2 = ( srfGridMesh_t * ) s_worldData.surfaces[ j ].data;

		// if this surface is not a grid
		if ( grid2->surfaceType != surfaceType_t::SF_GRID )
		{
			continue;
		}

		srfGridMesh_t *grid1 = ( srfGridMesh_t * ) s_worldData.surfaces[ grid1num ].data;

		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius )
		{
			continue;
		}

		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[ 0 ] != grid2->lodOrigin[ 0 ] )
		{
			continue;
		}

		if ( grid1->lodOrigin[ 1 ] != grid2->lodOrigin[ 1 ] )
		{
			continue;
		}

		if ( grid1->lodOrigin[ 2 ] != grid2->lodOrigin[ 2 ] )
		{
			continue;
		}

		//
		while ( R_StitchPatches( grid1num, j ) )
		{
			numstitches++;
		}
	}

	return numstitches;
}

/*
===============
R_StitchAllPatches
===============
*/
void R_StitchAllPatches()
{
	int           i, stitched, numstitches;
	srfGridMesh_t *grid1;

	Log::Debug("...stitching LoD cracks" );

	numstitches = 0;

	do
	{
		stitched = false;

		for ( i = 0; i < s_worldData.numSurfaces; i++ )
		{
			//
			grid1 = ( srfGridMesh_t * ) s_worldData.surfaces[ i ].data;

			// if this surface is not a grid
			if ( grid1->surfaceType != surfaceType_t::SF_GRID )
			{
				continue;
			}

			//
			if ( grid1->lodStitched )
			{
				continue;
			}

			//
			grid1->lodStitched = true;
			stitched = true;
			//
			numstitches += R_TryStitchingPatch( i );
		}
	}
	while ( stitched );

	Log::Debug("stitched %d LoD cracks", numstitches );
}

/*
===============
R_MovePatchSurfacesToHunk
===============
*/
void R_MovePatchSurfacesToHunk()
{
	int           i;
	srfGridMesh_t *grid, *hunkgrid;

	for ( i = 0; i < s_worldData.numSurfaces; i++ )
	{
		//
		grid = ( srfGridMesh_t * ) s_worldData.surfaces[ i ].data;

		// if this surface is not a grid
		if ( grid->surfaceType != surfaceType_t::SF_GRID )
		{
			continue;
		}

		//
		hunkgrid = (srfGridMesh_t*) ri.Hunk_Alloc( sizeof(srfGridMesh_t), ha_pref::h_low );
		*hunkgrid = *grid;

		hunkgrid->widthLodError = (float*) ri.Hunk_Alloc( grid->width * sizeof( float ), ha_pref::h_low );
		std::copy_n( grid->widthLodError, grid->width, hunkgrid->widthLodError );

		hunkgrid->heightLodError = (float*) ri.Hunk_Alloc( grid->height * sizeof( float ), ha_pref::h_low );
		std::copy_n( grid->heightLodError, grid->height, hunkgrid->heightLodError );

		hunkgrid->numTriangles = grid->numTriangles;
		hunkgrid->triangles = (srfTriangle_t*) ri.Hunk_Alloc( grid->numTriangles * sizeof( srfTriangle_t ), ha_pref::h_low );
		std::copy_n( grid->triangles, grid->numTriangles, hunkgrid->triangles );

		hunkgrid->numVerts = grid->numVerts;
		hunkgrid->verts = (srfVert_t*) ri.Hunk_Alloc( grid->numVerts * sizeof( srfVert_t ), ha_pref::h_low );
		std::copy_n( grid->verts, grid->numVerts, hunkgrid->verts );

		R_FreeSurfaceGridMesh( grid );

		s_worldData.surfaces[ i ].data = ( surfaceType_t * ) hunkgrid;
	}
}

/*
=================
R_CreateClusters
=================
*/
static void R_CreateClusters()
{
	int          i, j;
	bspSurface_t *surface;
	bspNode_t    *node;

	// reset surfaces' viewCount
	for ( i = 0, surface = s_worldData.surfaces; i < s_worldData.numSurfaces; i++, surface++ )
	{
		surface->viewCount = -1;
	}

	for ( j = 0, node = s_worldData.nodes; j < s_worldData.numnodes; j++, node++ )
	{
		node->visCounts[ 0 ] = -1;
	}
}

/*
===============
R_CreateWorldVBO
===============
*/
static void R_CreateWorldVBO() {
	int startTime = ri.Milliseconds();

	int numVertsInitial = 0;
	int numTriangles = 0;
	int numSurfaces = 0;
	int numPortals = 0;
	for ( int i = 0; i < s_worldData.numSurfaces; i++ ) {
		bspSurface_t* surface = &s_worldData.surfaces[i];

		// HACK: portals: don't use VBO because when adding a portal we have to read back the verts CPU-side
		// Autosprite: don't use VBO because verts are rewritten each time based on view origin
		if ( surface->shader->isPortal || surface->shader->autoSpriteMode != 0 ) {
			if( glConfig.usingMaterialSystem ) {
				materialSystem.autospriteSurfaces.push_back( surface );
			}

			if ( surface->shader->isPortal ) {
				numPortals++;
			}
			continue;
		}

		if ( glConfig.usingMaterialSystem && surface->shader->isSky ) {
			if ( std::find( materialSystem.skyShaders.begin(), materialSystem.skyShaders.end(), surface->shader )
				== materialSystem.skyShaders.end() ) {
				materialSystem.skyShaders.emplace_back( surface->shader );
			}

			/* Sky brushes are currently not used by the material system,
			but they still have to go into the VBO for the core renderer */
			surface->skyBrush = true;
		}

		if ( *surface->data == surfaceType_t::SF_FACE || *surface->data == surfaceType_t::SF_GRID
			|| *surface->data == surfaceType_t::SF_TRIANGLES ) {
			srfGeneric_t* srf = ( srfGeneric_t* ) surface->data;

			numVertsInitial += srf->numVerts;
			numTriangles += srf->numTriangles;
		} else {
			continue;
		}

		surface->renderable = true;

		if ( i >= ( int ) s_worldData.models[0].numSurfaces ) {
			surface->BSPModel = true;
		}

		numSurfaces++;
	}

	if ( !numVertsInitial || !numTriangles || !numSurfaces ) {
		return;
	}

	s_worldData.numPortals = numPortals;
	s_worldData.portals = ( AABB* ) ri.Hunk_Alloc( numPortals * sizeof( AABB ), ha_pref::h_low );
	int portal = 0;
	for ( int i = 0; i < s_worldData.numSurfaces; i++ ) {
		bspSurface_t* surface = &s_worldData.surfaces[i];

		if ( surface->shader->isPortal ) {
			surface->portalNum = portal;
			AABB* aabb = &s_worldData.portals[portal];
			switch ( *surface->data ) {
				case surfaceType_t::SF_GRID:
				{
					srfGeneric_t* srf = ( srfGeneric_t* ) surface->data;
					VectorCopy( srf->origin, aabb->origin );
					VectorCopy( srf->bounds[0], aabb->mins );
					VectorCopy( srf->bounds[1], aabb->maxs );
					Log::Warn( "Grid portals aren't properly supported" );
					break;
				}
				case surfaceType_t::SF_FACE:
				case surfaceType_t::SF_TRIANGLES:
				{
					srfGeneric_t* srf = ( srfGeneric_t* ) surface->data;
					VectorCopy( srf->origin, aabb->origin );
					VectorCopy( srf->bounds[0], aabb->mins );
					VectorCopy( srf->bounds[1], aabb->maxs );
					break;
				}
				default:
					Log::Warn( "Unsupported portal surface type" );
					break;
			}
			portal++;

			if( glConfig.usingMaterialSystem ) {
				MaterialSurface srf{};

				srf.shader = surface->shader;
				srf.surface = surface->data;
				srf.bspSurface = true;
				srf.lightMapNum = surface->lightmapNum;
				srf.portalNum = surface->portalNum;

				srf.firstIndex = ( ( srfGeneric_t* ) surface->data )->firstIndex;
				srf.count = ( ( srfGeneric_t* ) surface->data )->numTriangles * 3;
				srf.verts = ( ( srfGeneric_t* ) surface->data )->verts;
				srf.tris = ( ( srfGeneric_t* ) surface->data )->triangles;

				VectorCopy( ( ( srfGeneric_t* ) surface->data )->origin, srf.origin );
				srf.radius = ( ( srfGeneric_t* ) surface->data )->radius;

				materialSystem.portalSurfaces.emplace_back( srf );
			}
		} else {
			surface->portalNum = -1;
		}
	}

	bspSurface_t** rendererSurfaces = ( bspSurface_t** ) ri.Hunk_AllocateTempMemory( sizeof( bspSurface_t* ) * numSurfaces );
	numSurfaces = 0;
	for ( int i = 0; i < s_worldData.numSurfaces; i++ ) {
		bspSurface_t* surface = &s_worldData.surfaces[i];

		if ( surface->renderable ) {
			rendererSurfaces[numSurfaces++] = surface;
		}
	}

	OptimiseMapGeometryCore( &s_worldData, rendererSurfaces, numSurfaces );

	Log::Debug( "...calculating world VBO ( %i verts %i tris )", numVertsInitial, numTriangles );

	// Use srfVert_t for the temporary array used to feed R_CreateStaticVBO, despite containing
	// extraneous data, so that verts can be conveniently be bulk copied from the surface.
	srfVert_t* vboVerts = ( srfVert_t* ) ri.Hunk_AllocateTempMemory( numVertsInitial * sizeof( srfVert_t ) );
	glIndex_t* vboIdxs = ( glIndex_t* ) ri.Hunk_AllocateTempMemory( 3 * numTriangles * sizeof( glIndex_t ) );

	int numVerts;
	int numIndices;
	MergeDuplicateVertices( rendererSurfaces, numSurfaces, vboVerts, numVertsInitial, vboIdxs, 3 * numTriangles, numVerts, numIndices );

	if ( glConfig.usingMaterialSystem ) {
		OptimiseMapGeometryMaterial( &s_worldData, rendererSurfaces, numSurfaces, vboVerts, numVerts, vboIdxs, numIndices );
	}

	vertexAttributeSpec_t attrs[]{
		{ ATTR_INDEX_POSITION, GL_FLOAT, GL_FLOAT, &vboVerts[0].xyz, 3, sizeof( *vboVerts ), 0 },
		{ ATTR_INDEX_COLOR, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, &vboVerts[0].lightColor, 4, sizeof( *vboVerts ), ATTR_OPTION_NORMALIZE },
		{ ATTR_INDEX_QTANGENT, GL_SHORT, GL_SHORT, &vboVerts[0].qtangent, 4, sizeof( *vboVerts ), ATTR_OPTION_NORMALIZE },
		{ ATTR_INDEX_TEXCOORD, GL_FLOAT, GL_HALF_FLOAT, &vboVerts[0].st, 4, sizeof( *vboVerts ), 0 },
	};

	s_worldData.vbo = R_CreateStaticVBO(
		"staticWorld_VBO", std::begin( attrs ), std::end( attrs ), numVerts );
	s_worldData.ibo = R_CreateStaticIBO( "staticWorld_IBO", vboIdxs, numTriangles * 3 );

	SetupVAOBuffers( s_worldData.vbo, s_worldData.ibo,
		ATTR_POSITION | ATTR_COLOR
		| ATTR_QTANGENT | ATTR_TEXCOORD,
		&s_worldData.vbo->VAO );

	ri.Hunk_FreeTempMemory( vboIdxs );
	ri.Hunk_FreeTempMemory( vboVerts );

	MergeLeafSurfacesCore( &s_worldData, rendererSurfaces, numSurfaces );

	int endTime = ri.Milliseconds();
	Log::Debug( "world VBO calculation time = %5.2f seconds", ( endTime - startTime ) / 1000.0 );

	// point triangle surfaces to world VBO
	for ( int i = 0; i < numSurfaces; i++ ) {
		bspSurface_t* surface = rendererSurfaces[i];

		srfGeneric_t* srf = ( srfGeneric_t* ) surface->data;
		srf->vbo = s_worldData.vbo;
		srf->ibo = s_worldData.ibo;

		// clear data used for sorting
		surface->viewCount = -1;
	}

	ri.Hunk_FreeTempMemory( rendererSurfaces );
}

/*
===============
R_LoadSurfaces
===============
*/
static void R_LoadSurfaces( lump_t *surfs, lump_t *verts, lump_t *indexLump )
{
	dsurface_t   *in;
	bspSurface_t *out;
	drawVert_t   *dv;
	int          *indexes;
	int          count;
	int          numFaces, numMeshes, numTriSurfs, numFlares, numFoliages;
	int          i;

	Log::Debug("...loading surfaces" );

	numFaces = 0;
	numMeshes = 0;
	numTriSurfs = 0;
	numFlares = 0;
	numFoliages = 0;

	in = ( dsurface_t * )( fileBase + surfs->fileofs );

	if ( surfs->filelen % sizeof( *in ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = surfs->filelen / sizeof( *in );

	dv = ( drawVert_t * )( fileBase + verts->fileofs );

	if ( verts->filelen % sizeof( *dv ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	indexes = ( int * )( fileBase + indexLump->fileofs );

	if ( indexLump->filelen % sizeof( *indexes ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	out = (bspSurface_t*) ri.Hunk_Alloc( count * sizeof( *out ), ha_pref::h_low );

	s_worldData.surfaces = out;
	s_worldData.numSurfaces = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		switch ( LittleLong( in->surfaceType ) )
		{
			case mapSurfaceType_t::MST_PATCH:
				ParseMesh( in, dv, out );
				numMeshes++;
				break;

			case mapSurfaceType_t::MST_TRIANGLE_SOUP:
				ParseTriSurf( in, dv, out, indexes );
				numTriSurfs++;
				break;

			case mapSurfaceType_t::MST_PLANAR:
				ParseFace( in, dv, out, indexes );
				numFaces++;
				break;

			case mapSurfaceType_t::MST_FLARE:
				Log::Warn( "Surface type not supported: MST_FLARE; firstIndex: %i, numIndexes: %i, shaderNum: %i",
					in->firstIndex, in->numIndexes, in->shaderNum );

				// We still have to set these because other code will be checking them
				out->data = ( surfaceType_t* ) ri.Hunk_Alloc( sizeof( surfaceType_t ), ha_pref::h_low );
				*out->data = surfaceType_t::SF_BAD;
				out->shader = tr.defaultShader;

				numFlares++;
				break;

			case mapSurfaceType_t::MST_FOLIAGE:
				// Tr3B: TODO ParseFoliage
				Log::Warn( "Surface type not supported: MST_FOLIAGE, parsing as MST_TRIANGLE_SOUP instead;"
					" firstIndex: %i, numIndexes: %i, shaderNum: %i",
					in->firstIndex, in->numIndexes, in->shaderNum );
				ParseTriSurf( in, dv, out, indexes );
				numFoliages++;
				break;

			default:
				Sys::Drop( "Bad surfaceType" );
		}
	}

	Log::Debug( "...loaded %d faces, %i meshes, %i trisurfs, %i flares (skipped) %i foliages", numFaces, numMeshes, numTriSurfs,
	           numFlares, numFoliages );

	if ( r_stitchCurves->integer )
	{
		R_StitchAllPatches();
	}

	R_FixSharedVertexLodError();

	if ( r_stitchCurves->integer )
	{
		R_MovePatchSurfacesToHunk();
	}
}

/*
=================
R_LoadSubmodels
=================
*/
static void R_LoadSubmodels( lump_t *l )
{
	dmodel_t   *in;
	bspModel_t *out;
	int        i, j, count;

	Log::Debug("...loading submodels" );

	in = ( dmodel_t * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );

	s_worldData.numModels = count;
	s_worldData.models = out = (bspModel_t*) ri.Hunk_Alloc( count * sizeof( *out ), ha_pref::h_low );

	for ( i = 0; i < count; i++, in++, out++ )
	{
		model_t *model;

		model = R_AllocModel();

		ASSERT(model != nullptr);

		if ( model == nullptr )
		{
			Sys::Drop( "R_LoadSubmodels: R_AllocModel() failed" );
		}

		model->type = modtype_t::MOD_BSP;
		model->bsp = out;
		Com_sprintf( model->name, sizeof( model->name ), "*%d", i );

		for ( j = 0; j < 3; j++ )
		{
			out->bounds[ 0 ][ j ] = LittleFloat( in->mins[ j ] );
			out->bounds[ 1 ][ j ] = LittleFloat( in->maxs[ j ] );
		}

		out->firstSurface = s_worldData.surfaces + LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );
	}
}

//==================================================================

/*
=================
R_SetParent
=================
*/
static void R_SetParent( bspNode_t *node, bspNode_t *parent )
{
	node->parent = parent;

	if ( node->contents != CONTENTS_NODE )
	{
		return;
	}

	R_SetParent( node->children[ 0 ], node );
	R_SetParent( node->children[ 1 ], node );
}

/*
=================
R_LoadNodesAndLeafs
=================
*/
static void R_LoadNodesAndLeafs( lump_t *nodeLump, lump_t *leafLump )
{
	int           i, j, p;
	dnode_t       *in;
	dleaf_t       *inLeaf;
	bspNode_t     *out;
	int           numNodes, numLeafs;

	Log::Debug("...loading nodes and leaves" );

	in = ( dnode_t * ) ( void * )( fileBase + nodeLump->fileofs );

	if ( nodeLump->filelen % sizeof( dnode_t ) || leafLump->filelen % sizeof( dleaf_t ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	numNodes = nodeLump->filelen / sizeof( dnode_t );
	numLeafs = leafLump->filelen / sizeof( dleaf_t );

	out = (bspNode_t*) ri.Hunk_Alloc( ( numNodes + numLeafs ) * sizeof( *out ), ha_pref::h_low );

	s_worldData.nodes = out;
	s_worldData.numnodes = numNodes + numLeafs;
	s_worldData.numDecisionNodes = numNodes;

	// load nodes
	for ( i = 0; i < numNodes; i++, in++, out++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			out->mins[ j ] = LittleLong( in->mins[ j ] );
			out->maxs[ j ] = LittleLong( in->maxs[ j ] );
		}

		p = LittleLong( in->planeNum );
		out->plane = s_worldData.planes + p;

		out->contents = CONTENTS_NODE; // differentiate from leafs

		for ( j = 0; j < 2; j++ )
		{
			p = LittleLong( in->children[ j ] );

			if ( p >= 0 )
			{
				out->children[ j ] = s_worldData.nodes + p;
			}
			else
			{
				out->children[ j ] = s_worldData.nodes + numNodes + ( -1 - p );
			}
		}
	}

	// load leafs
	inLeaf = ( dleaf_t * )( fileBase + leafLump->fileofs );

	for ( i = 0; i < numLeafs; i++, inLeaf++, out++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			out->mins[ j ] = LittleLong( inLeaf->mins[ j ] );
			out->maxs[ j ] = LittleLong( inLeaf->maxs[ j ] );
		}

		out->cluster = LittleLong( inLeaf->cluster );
		out->area = LittleLong( inLeaf->area );

		if ( out->cluster >= s_worldData.numClusters )
		{
			s_worldData.numClusters = out->cluster + 1;
		}

		out->firstMarkSurface = LittleLong( inLeaf->firstLeafSurface );
		out->numMarkSurfaces = LittleLong( inLeaf->numLeafSurfaces );
	}

	// chain descendants and compute surface bounds
	R_SetParent( s_worldData.nodes, nullptr );

	backEndData[ 0 ]->traversalList = ( bspNode_t ** ) ri.Hunk_Alloc( sizeof( bspNode_t * ) * s_worldData.numnodes, ha_pref::h_low );
	backEndData[ 0 ]->traversalLength = 0;

	if ( r_smp->integer )
	{
		backEndData[ 1 ]->traversalList = ( bspNode_t ** ) ri.Hunk_Alloc( sizeof( bspNode_t * ) * s_worldData.numnodes, ha_pref::h_low );
		backEndData[ 1 ]->traversalLength = 0;
	}
}

//=============================================================================

/*
=================
R_LoadShaders
=================
*/
static void R_LoadShaders( lump_t *l )
{
	int       i, count;
	dshader_t *in, *out;

	Log::Debug("...loading shaders" );

	in = ( dshader_t * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );
	out = (dshader_t*) ri.Hunk_Alloc( count * sizeof( *out ), ha_pref::h_low );

	s_worldData.shaders = out;
	s_worldData.numShaders = count;

	memcpy( out, in, count * sizeof( *out ) );

	for ( i = 0; i < count; i++ )
	{
		Log::Debug("loading shader: '%s'", out[ i ].shader );

		out[ i ].surfaceFlags = LittleLong( out[ i ].surfaceFlags );
		out[ i ].contentFlags = LittleLong( out[ i ].contentFlags );
	}
}

/*
=================
R_LoadMarksurfaces
=================
*/
static void R_LoadMarksurfaces( lump_t *l )
{
	int          i, j, count;
	int          *in;
	bspSurface_t **out;

	Log::Debug("...loading mark surfaces" );

	in = ( int * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );
	out = (bspSurface_t**) ri.Hunk_Alloc( count * sizeof( *out ), ha_pref::h_low );

	s_worldData.markSurfaces = out;
	s_worldData.numMarkSurfaces = count;
	s_worldData.viewSurfaces = ( bspSurface_t ** ) ri.Hunk_Alloc( count * sizeof( *out ), ha_pref::h_low );

	for ( i = 0; i < count; i++ )
	{
		j = LittleLong( in[ i ] );
		if ( j < 0 || j >= s_worldData.numSurfaces )
		{
			Sys::Drop( "LoadMap: invalid surface number %d", j );
		}
		out[ i ] = s_worldData.surfaces + j;
		s_worldData.viewSurfaces[ i ] = out[ i ];
	}
}

/*
=================
R_LoadPlanes
=================
*/
static void R_LoadPlanes( lump_t *l )
{
	int      i, j;
	cplane_t *out;
	dplane_t *in;
	int      count;

	Log::Debug("...loading planes" );

	in = ( dplane_t * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );
	out = (cplane_t*) ri.Hunk_Alloc( count * 2 * sizeof( *out ), ha_pref::h_low );

	s_worldData.planes = out;
	s_worldData.numplanes = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			out->normal[ j ] = LittleFloat( in->normal[ j ] );
		}

		out->dist = LittleFloat( in->dist );
		out->type = PlaneTypeForNormal( out->normal );
		SetPlaneSignbits( out );
	}
}

/*
=================
R_LoadFogs
=================
*/
static void R_LoadFogs( lump_t *l, lump_t *brushesLump, lump_t *sidesLump )
{
	int          i;
	fog_t        *out;
	dfog_t       *fogs;
	dbrush_t     *brushes, *brush;
	dbrushside_t *sides;
	int          count, brushesCount, sidesCount;
	int          sideNum;
	int          planeNum;
	int          firstSide = 0;

	Log::Debug("...loading fogs" );

	fogs = ( dfog_t* )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *fogs ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *fogs );

	// create fog structures for them
	s_worldData.numFogs = count + 1;
	s_worldData.fogs = (fog_t*) ri.Hunk_Alloc( s_worldData.numFogs * sizeof( *out ), ha_pref::h_low );
	out = s_worldData.fogs + 1;

	if ( !count )
	{
		Log::Debug("no fog volumes loaded" );
		return;
	}

	brushes = ( dbrush_t * )( fileBase + brushesLump->fileofs );

	if ( brushesLump->filelen % sizeof( *brushes ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	brushesCount = brushesLump->filelen / sizeof( *brushes );

	sides = ( dbrushside_t * )( fileBase + sidesLump->fileofs );

	if ( sidesLump->filelen % sizeof( *sides ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	sidesCount = sidesLump->filelen / sizeof( *sides );

	struct FogVert
	{
		vec3_t position;
		vec4_t surface;
		vec4_t planes[ 5 ]; // faces other than the current triangle's
	};
	std::vector<FogVert> fogVerts(24 * count);
	std::vector<glIndex_t> fogIndexes(36 * count);

	for ( i = 0; i < count; i++, fogs++ )
	{
		out->originalBrushNumber = LittleLong( fogs->brushNum );

		// ydnar: global fog has a brush number of -1, and no visible side
		if ( out->originalBrushNumber == -1 )
		{
			VectorSet( out->bounds[ 0 ], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD );
			VectorSet( out->bounds[ 1 ], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD );
		}
		else
		{
			if ( out->originalBrushNumber >= brushesCount )
			{
				Sys::Drop( "fog brushNumber out of range" );
			}

			brush = brushes + out->originalBrushNumber;

			firstSide = LittleLong( brush->firstSide );

			if ( firstSide > sidesCount - 6 )
			{
				Sys::Drop( "fog brush sideNumber out of range" );
			}

			// brushes are always sorted with the axial sides first
			sideNum = firstSide + 0;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 0 ][ 0 ] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 1;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 1 ][ 0 ] = s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 2;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 0 ][ 1 ] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 3;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 1 ][ 1 ] = s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 4;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 0 ][ 2 ] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 5;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 1 ][ 2 ] = s_worldData.planes[ planeNum ].dist;
		}

		// it says RSF_3D but if there is no shader text found it should probably just error instead
		// of trying to create an implicit shader from an image...
		out->shader = R_FindShader( fogs->shader, RSF_3D );

		// set the gradient vector
		sideNum = LittleLong( fogs->visibleSide );

		// ydnar: made this check a little more strenuous (was sideNum == -1)
		if ( sideNum < 0 || sideNum >= sidesCount )
		{
			Vector4Set( out->surface, 0.0f, 0.0f, 0.0f, -1.0e10f );
		}
		else
		{
			planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
			VectorSubtract( vec3_origin, s_worldData.planes[ planeNum ].normal, out->surface );
			out->surface[ 3 ] = -s_worldData.planes[ planeNum ].dist;
		}

		// add faces of fog brush with information about the other planes
		// TODO: allow non-axis-aligned boxes. The GLSL can draw any brush with up to 6 faces.
		constexpr int faces[ 6 ][ 4 ]
		{
			{ 2, 0, 6, 4 },
			{ 1, 3, 5, 7 },
			{ 0, 1, 4, 5 },
			{ 3, 2, 7, 6 },
			{ 2, 3, 0, 1 },
			{ 7, 6, 5, 4 }
		};

		for ( int face = 0; face < 6; face++ )
		{
			for ( int v = 0; v < 4; v++ )
			{
				FogVert &vert = fogVerts[ i * 24 + face * 4 + v ];
				int p = faces[ face ][ v ];
				vert.position[ 0 ] = out->bounds[ p & 1 ][ 0 ];
				vert.position[ 1 ] = out->bounds[ ( p >> 1 ) & 1 ][ 1 ];
				vert.position[ 2 ] = out->bounds[ p >> 2 ][ 2 ];

				VectorCopy( out->surface, vert.surface );
				vert.surface[ 3 ] = -out->surface[ 3 ];

				vec4_t *plane = vert.planes;
				for ( int otherFace = 0; otherFace < 6; otherFace++ )
				{
					if ( face != otherFace )
					{
						planeNum = LittleLong( sides[ firstSide + otherFace ].planeNum );
						VectorCopy( s_worldData.planes[ planeNum ].normal, *plane );
						( *plane )[ 3 ] = s_worldData.planes[ planeNum ].dist;
						++plane;
					}
				}
			}

			fogIndexes[ 36 * i + 6 * face + 0 ] = i * 24 + face * 4 + 0;
			fogIndexes[ 36 * i + 6 * face + 1 ] = i * 24 + face * 4 + 1;
			fogIndexes[ 36 * i + 6 * face + 2 ] = i * 24 + face * 4 + 2;
			fogIndexes[ 36 * i + 6 * face + 3 ] = i * 24 + face * 4 + 2;
			fogIndexes[ 36 * i + 6 * face + 4 ] = i * 24 + face * 4 + 1;
			fogIndexes[ 36 * i + 6 * face + 5 ] = i * 24 + face * 4 + 3;
		}

		// add draw surf for fog brush faces
		out->surf.firstIndex = 36 * i;
		out->surf.numTriangles = 12;
		out->surf.surfaceType = surfaceType_t::SF_TRIANGLES;

		out++;
	}

	vertexAttributeSpec_t attributes[] {
		{ ATTR_INDEX_POSITION, GL_FLOAT, GL_FLOAT, &fogVerts[ 0 ].position, 3, sizeof( fogVerts[ 0 ] ), 0 },
		{ ATTR_INDEX_FOG_SURFACE, GL_FLOAT, GL_FLOAT, &fogVerts[ 0 ].surface, 4, sizeof( fogVerts[ 0 ] ), 0 },
		{ ATTR_INDEX_FOG_PLANES_0 + 0, GL_FLOAT, GL_FLOAT, &fogVerts[ 0 ].planes[ 0 ], 4, sizeof( FogVert ), 0 },
		{ ATTR_INDEX_FOG_PLANES_0 + 1, GL_FLOAT, GL_FLOAT, &fogVerts[ 0 ].planes[ 1 ], 4, sizeof( FogVert ), 0 },
		{ ATTR_INDEX_FOG_PLANES_0 + 2, GL_FLOAT, GL_FLOAT, &fogVerts[ 0 ].planes[ 2 ], 4, sizeof( FogVert ), 0 },
		{ ATTR_INDEX_FOG_PLANES_0 + 3, GL_FLOAT, GL_FLOAT, &fogVerts[ 0 ].planes[ 3 ], 4, sizeof( FogVert ), 0 },
		{ ATTR_INDEX_FOG_PLANES_0 + 4, GL_FLOAT, GL_FLOAT, &fogVerts[ 0 ].planes[ 4 ], 4, sizeof( FogVert ), 0 },
	};
	VBO_t *fogVBO = R_CreateStaticVBO(
		"fogs VBO", std::begin( attributes ), std::end( attributes ), fogVerts.size() );
	IBO_t *fogIBO = R_CreateStaticIBO( "fogs IBO", &fogIndexes[ 0 ], fogIndexes.size() );
	SetupVAOBuffers( fogVBO, fogIBO, ATTR_POSITION | ATTR_FOG_SURFACE | ATTR_FOG_PLANES, &fogVBO->VAO );

	for ( int j = 1; j < s_worldData.numFogs; j++ )
	{
		s_worldData.fogs[ j ].surf.vbo = fogVBO;
		s_worldData.fogs[ j ].surf.ibo = fogIBO;
	}

	Log::Debug("%i fog volumes loaded", s_worldData.numFogs );
}

static void R_SetConstantColorLightGrid( const byte color[3] )
{
	world_t *w = &s_worldData;

	// generate default 1x1x1 light grid
	w->lightGridSize[ 0 ] = 100000.0f;
	w->lightGridSize[ 1 ] = 100000.0f;
	w->lightGridSize[ 2 ] = 100000.0f;
	w->lightGridInverseSize[ 0 ] = 1.0f / w->lightGridSize[ 0 ];
	w->lightGridInverseSize[ 1 ] = 1.0f / w->lightGridSize[ 1 ];
	w->lightGridInverseSize[ 2 ] = 1.0f / w->lightGridSize[ 2 ];

	VectorSet( w->lightGridOrigin, 0.0f, 0.0f, 0.0f );

	VectorMA( w->lightGridOrigin, -0.5f, w->lightGridSize,
		  w->lightGridGLOrigin );

	VectorSet( w->lightGridBounds, 1, 1, 1 );
	w->numLightGridPoints = 1;

	w->lightGridGLScale[ 0 ] = w->lightGridInverseSize[ 0 ];
	w->lightGridGLScale[ 1 ] = w->lightGridInverseSize[ 1 ];
	w->lightGridGLScale[ 2 ] = w->lightGridInverseSize[ 2 ];

	bspGridPoint1_t *gridPoint1 = (bspGridPoint1_t *) ri.Hunk_Alloc( sizeof( bspGridPoint1_t ) + sizeof( bspGridPoint2_t ), ha_pref::h_low );
	bspGridPoint2_t *gridPoint2 = (bspGridPoint2_t *) (gridPoint1 + w->numLightGridPoints);

	// default some white light from above
	gridPoint1->color[ 0 ] = color[0];
	gridPoint1->color[ 1 ] = color[1];
	gridPoint1->color[ 2 ] = color[2];
	gridPoint1->ambientPart = 128;
	gridPoint2->direction[ 0 ] = 128 + floatToSnorm8( 0.0f );
	gridPoint2->direction[ 1 ] = 128 + floatToSnorm8( 0.0f );
	gridPoint2->direction[ 2 ] = 128 + floatToSnorm8( 1.0f );
	gridPoint2->isSet = 255;

	w->lightGridData1 = gridPoint1;
	w->lightGridData2 = gridPoint2;

	imageParams_t imageParams = {};
	imageParams.bits = IF_NOPICMIP;
	imageParams.filterType = filterType_t::FT_DEFAULT;
	imageParams.wrapType = wrapTypeEnum_t::WT_EDGE_CLAMP;

	tr.lightGrid1Image = R_Create3DImage("<lightGrid1>", (const byte *)w->lightGridData1, w->lightGridBounds[ 0 ], w->lightGridBounds[ 1 ], w->lightGridBounds[ 2 ], imageParams );

	if ( glConfig.deluxeMapping )
	{
		tr.lightGrid2Image = R_Create3DImage("<lightGrid2>", (const byte *)w->lightGridData2, w->lightGridBounds[ 0 ], w->lightGridBounds[ 1 ], w->lightGridBounds[ 2 ], imageParams );
	}
}

/*
================
R_LoadLightGrid
================
*/
void R_LoadLightGrid( lump_t *l )
{
	if ( glConfig.max3DTextureSize == 0 )
	{
		Log::Warn( "Grid lighting disabled because of missing 3D texture support." );

		if ( glConfig.deluxeMapping )
		{
			Log::Warn( "Grid deluxe mapping disabled because of missing 3D texture support." );
		}

		tr.lightGrid1Image = nullptr;
		tr.lightGrid2Image = nullptr;
		return;
	}

	int            i, j, k;
	world_t        *w;
	float          *wMins, *wMaxs;
	dgridPoint_t   *in;
	bspGridPoint1_t *gridPoint1;
	bspGridPoint2_t *gridPoint2;
	float          lat, lng;
	int            from[ 3 ], to[ 3 ];
	float          weights[ 3 ] = { 0.25f, 0.5f, 0.25f };
	float          *factors[ 3 ] = { weights, weights, weights };
	vec3_t         ambientColor, directedColor, direction;
	float          scale;

	if ( tr.ambientLightSet ) {
		const byte color[3]{ floatToUnorm8( tr.ambientLight[0] ), floatToUnorm8( tr.ambientLight[1] ),
			floatToUnorm8( tr.ambientLight[2] ) };
		R_SetConstantColorLightGrid( color );
	}

	if ( !r_precomputedLighting->integer )
	{
		const byte color[3] { 64, 64, 64 };
		R_SetConstantColorLightGrid( color );
		return;
	}

	Log::Debug("...loading light grid" );

	w = &s_worldData;

	w->lightGridInverseSize[ 0 ] = 1.0f / w->lightGridSize[ 0 ];
	w->lightGridInverseSize[ 1 ] = 1.0f / w->lightGridSize[ 1 ];
	w->lightGridInverseSize[ 2 ] = 1.0f / w->lightGridSize[ 2 ];

	wMins = w->models[ 0 ].bounds[ 0 ];
	wMaxs = w->models[ 0 ].bounds[ 1 ];

	for ( i = 0; i < 3; i++ )
	{
		float numNegativePoints = ceil( wMins[ i ] / w->lightGridSize[ i ] );
		float numPositivePoints = floor( wMaxs[ i ] / w->lightGridSize[ i ] );
		w->lightGridBounds[ i ] = static_cast<int>( numPositivePoints - numNegativePoints ) + 1;

		if ( w->lightGridBounds[ i ] <= 0 || w->lightGridBounds[ i ] > 5000 )
		{
			// sanity check to avoid integer overflows etc.
			Log::Warn( "invalid light grid parameters, default light grid used" );
			const byte color[ 3 ]{ 64, 64, 64 };
			R_SetConstantColorLightGrid( color );
			return;
		}

		w->lightGridOrigin[ i ] = w->lightGridSize[ i ] * numNegativePoints;
	}

	VectorMA( w->lightGridOrigin, -0.5f, w->lightGridSize,
		  w->lightGridGLOrigin );

	w->lightGridGLScale[ 0 ] = w->lightGridInverseSize[ 0 ] / w->lightGridBounds[ 0 ];
	w->lightGridGLScale[ 1 ] = w->lightGridInverseSize[ 1 ] / w->lightGridBounds[ 1 ];
	w->lightGridGLScale[ 2 ] = w->lightGridInverseSize[ 2 ] / w->lightGridBounds[ 2 ];

	w->numLightGridPoints = w->lightGridBounds[ 0 ] * w->lightGridBounds[ 1 ] * w->lightGridBounds[ 2 ];

	Log::Debug("grid size (%i %i %i)", ( int ) w->lightGridSize[ 0 ], ( int ) w->lightGridSize[ 1 ],
	           ( int ) w->lightGridSize[ 2 ] );
	Log::Debug("grid bounds (%i %i %i)", w->lightGridBounds[ 0 ], w->lightGridBounds[ 1 ],
			   w->lightGridBounds[ 2 ]);

	if ( static_cast<size_t>(l->filelen) != w->numLightGridPoints * sizeof( dgridPoint_t ) )
	{
		Log::Warn("light grid mismatch, default light grid used" );

		const byte color[3]{ 64, 64, 64 };
		R_SetConstantColorLightGrid( color );

		return;
	}

	in = ( dgridPoint_t * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	i = w->numLightGridPoints * ( sizeof( *gridPoint1 ) + sizeof( *gridPoint2 ) );
	gridPoint1 = (bspGridPoint1_t *) ri.Hunk_Alloc( i, ha_pref::h_low );
	gridPoint2 = (bspGridPoint2_t *) (gridPoint1 + w->numLightGridPoints);

	w->lightGridData1 = gridPoint1;
	w->lightGridData2 = gridPoint2;

	for ( i = 0; i < w->numLightGridPoints;
	      i++, in++, gridPoint1++, gridPoint2++ )
	{
		byte tmpAmbient[ 4 ];
		byte tmpDirected[ 4 ];

		tmpAmbient[ 0 ] = in->ambient[ 0 ];
		tmpAmbient[ 1 ] = in->ambient[ 1 ];
		tmpAmbient[ 2 ] = in->ambient[ 2 ];
		tmpAmbient[ 3 ] = 255;

		/* Make sure we don't change the (0, 0, 0) points because those are points in walls,
		which we'll fill up by interpolating nearby points later */
		if ( tmpAmbient[ 0 ] == 0 && tmpAmbient[ 1 ] == 0 && tmpAmbient[ 2 ] == 0 )
		{
			continue;
		}

		tmpDirected[ 0 ] = in->directed[ 0 ];
		tmpDirected[ 1 ] = in->directed[ 1 ];
		tmpDirected[ 2 ] = in->directed[ 2 ];
		tmpDirected[ 3 ] = 255;

		R_ColorShiftLightingBytes( tmpAmbient );
		R_ColorShiftLightingBytes( tmpDirected );

		for ( j = 0; j < 3; j++ )
		{
			ambientColor[ j ] = tmpAmbient[ j ] * ( 1.0f / 255.0f );
			directedColor[ j ] = tmpDirected[ j ] * ( 1.0f / 255.0f );
		}

		if ( tr.worldLinearizeTexture )
		{
			convertFromSRGB( ambientColor );
			convertFromSRGB( directedColor );
		}

		const float forceAmbient = r_forceAmbient.Get();
		if ( ambientColor[0] < forceAmbient &&
			ambientColor[1] < forceAmbient &&
			ambientColor[2] < forceAmbient )
		{
			VectorSet( ambientColor, forceAmbient, forceAmbient, forceAmbient );
		}

		// standard spherical coordinates to cartesian coordinates conversion
		// RB: having a look in NormalToLatLong used by q3map2 shows the order of latLong
		// Lng = 0 at (1,0,0), 90 at (0,1,0), etc., encoded in 8-bit sine table format
		// Lat = 0 at (0,0,1) to 180 (0,0,-1), encoded in 8-bit sine table format
		// (so the upper bit of lat is wasted)

		lat = DEG2RAD( in->latLong[ 0 ] * ( 360.0f / 255.0f ) );
		lng = DEG2RAD( in->latLong[ 1 ] * ( 360.0f / 255.0f ) );

		direction[ 0 ] = cosf( lng ) * sinf( lat );
		direction[ 1 ] = sinf( lng ) * sinf( lat );
		direction[ 2 ] = cosf( lat );

		// Pack data into an bspGridPoint
		gridPoint1->color[ 0 ] = floatToUnorm8( 0.5f * (ambientColor[ 0 ] + directedColor[ 0 ]) );
		gridPoint1->color[ 1 ] = floatToUnorm8( 0.5f * (ambientColor[ 1 ] + directedColor[ 1 ]) );
		gridPoint1->color[ 2 ] = floatToUnorm8( 0.5f * (ambientColor[ 2 ] + directedColor[ 2 ]) );

		float ambientLength = VectorLength(ambientColor);
		float directedLength = VectorLength(directedColor);
		float length = ambientLength + directedLength;
		gridPoint1->ambientPart = floatToUnorm8( ambientLength / length );

		gridPoint2->direction[0] = 128 + floatToSnorm8( direction[ 0 ] );
		gridPoint2->direction[1] = 128 + floatToSnorm8( direction[ 1 ] );
		gridPoint2->direction[2] = 128 + floatToSnorm8( direction[ 2 ] );
		gridPoint2->isSet = 255;
	}

	// fill in gridpoints with zero light (samples in walls) to avoid
	// darkening of objects near walls
	// FIXME: the interpolation includes other interpolated data points so the
	// result depends on iteration order
	gridPoint1 = w->lightGridData1;
	gridPoint2 = w->lightGridData2;

	for( k = 0; k < w->lightGridBounds[ 2 ]; k++ ) {
		from[ 2 ] = k - 1;
		to[ 2 ] = k + 1;

		for( j = 0; j < w->lightGridBounds[ 1 ]; j++ ) {
			from[ 1 ] = j - 1;
			to[ 1 ] = j + 1;

			for( i = 0; i < w->lightGridBounds[ 0 ];
			     i++, gridPoint1++, gridPoint2++ ) {
				from[ 0 ] = i - 1;
				to[ 0 ] = i + 1;

				if ( gridPoint2->isSet )
				{
					continue;
				}

				scale = R_InterpolateLightGrid( w, from, to, factors,
								ambientColor, directedColor,
								direction );
				if( scale > 0.0f ) {
					scale = 1.0f / scale;

					VectorScale( ambientColor, scale, ambientColor );
					VectorScale( directedColor, scale, directedColor );
					VectorScale( direction, scale, direction );


					gridPoint1->color[0] = floatToUnorm8(0.5f * (ambientColor[0] + directedColor[0]));
					gridPoint1->color[1] = floatToUnorm8(0.5f * (ambientColor[1] + directedColor[1]));
					gridPoint1->color[2] = floatToUnorm8(0.5f * (ambientColor[2] + directedColor[2]));
					gridPoint1->ambientPart = floatToUnorm8(VectorLength(ambientColor) / (VectorLength(ambientColor) + VectorLength(directedColor)));

					gridPoint2->direction[0] = 128 + floatToSnorm8(direction[0]);
					gridPoint2->direction[1] = 128 + floatToSnorm8(direction[1]);
					gridPoint2->direction[2] = 128 + floatToSnorm8(direction[2]);
					gridPoint2->isSet = 255;
				}
			}
		}
	}

	imageParams_t imageParams = {};
	imageParams.bits = IF_NOPICMIP;
	imageParams.filterType = filterType_t::FT_LINEAR;
	imageParams.wrapType = wrapTypeEnum_t::WT_EDGE_CLAMP;

	tr.lightGrid1Image = R_Create3DImage("<lightGrid1>", (const byte *)w->lightGridData1, w->lightGridBounds[ 0 ], w->lightGridBounds[ 1 ], w->lightGridBounds[ 2 ], imageParams );

	if ( glConfig.deluxeMapping )
	{
		tr.lightGrid2Image = R_Create3DImage("<lightGrid2>", (const byte *)w->lightGridData2, w->lightGridBounds[ 0 ], w->lightGridBounds[ 1 ], w->lightGridBounds[ 2 ], imageParams );
	}

	Log::Debug("%i light grid points created", w->numLightGridPoints );
}

// The NOP variants are in tr_init.cpp.
static float convertFloatFromSRGB_accurate( float f )
{
	if ( backEnd.projection2D )
	{
		return f;
	}

	return convertFromSRGB( f );
}

static float convertFloatFromSRGB_cheap( float f )
{
	if ( backEnd.projection2D )
	{
		return f;
	}

	return convertFromSRGB( f, false );
}

static Color::Color convertColorFromSRGB_accurate( Color::Color c )
{
	if ( backEnd.projection2D )
	{
		return c;
	}

	return c.ConvertFromSRGB();
}

static Color::Color convertColorFromSRGB_cheap( Color::Color c )
{
	if ( backEnd.projection2D )
	{
		return c;
	}

	return c.ConvertFromSRGB( false );
}

/*
================
R_LoadEntities
================
*/
void R_LoadEntities( lump_t *l, std::string &externalEntities )
{
	const char *p, *token;
	char *s;
	char         keyname[ MAX_TOKEN_CHARS ];
	char         value[ MAX_TOKEN_CHARS ];
	world_t      *w;

	Log::Debug("...loading entities" );

	w = &s_worldData;
	w->lightGridSize[ 0 ] = 64;
	w->lightGridSize[ 1 ] = 64;
	w->lightGridSize[ 2 ] = 128;

	// store for reference by the cgame
	if ( externalEntities.empty() )
	{
		w->entityString = (char*) ri.Hunk_Alloc( l->filelen + 1, ha_pref::h_low );
		//strcpy(w->entityString, (char *)(fileBase + l->fileofs));
		Q_strncpyz( w->entityString, ( char * )( fileBase + l->fileofs ), l->filelen + 1 );
	}
	else
	{
		w->entityString = (char*) ri.Hunk_Alloc( externalEntities.length() + 1, ha_pref::h_low );
		Q_strncpyz( w->entityString, externalEntities.c_str(), externalEntities.length() + 1 );
	}

	w->entityParsePoint = w->entityString;

	p = w->entityString;

	// only parse the world spawn
	while ( true )
	{
		// parse key
		token = COM_ParseExt2( &p, true );

		if ( !*token )
		{
			Log::Warn("unexpected end of entities string while parsing worldspawn" );
			break;
		}

		if ( *token == '{' )
		{
			continue;
		}

		if ( *token == '}' )
		{
			break;
		}

		Q_strncpyz( keyname, token, sizeof( keyname ) );

		// parse value
		token = COM_ParseExt2( &p, false );

		if ( !*token )
		{
			continue;
		}

		Q_strncpyz( value, token, sizeof( value ) );

		// check for remapping of shaders for vertex lighting
		s = (char *) "vertexremapshader";

		if ( !Q_strncmp( keyname, s, strlen( s ) ) )
		{
			s = strchr( value, ';' );

			if ( !s )
			{
				Log::Warn("no semi colon in vertexshaderremap '%s'", value );
				break;
			}

			*s++ = 0;
			continue;
		}

		// check for remapping of shaders
		s = (char *) "remapshader";

		if ( !Q_strncmp( keyname, s, strlen( s ) ) )
		{
			s = strchr( value, ';' );

			if ( !s )
			{
				Log::Warn("no semi colon in shaderremap '%s'", value );
				break;
			}

			*s++ = 0;
			R_RemapShader( value, s, "0" );
			continue;
		}

		// check for a different grid size
		if ( !Q_stricmp( keyname, "gridsize" ) )
		{
			sscanf( value, "%f %f %f", &w->lightGridSize[ 0 ], &w->lightGridSize[ 1 ], &w->lightGridSize[ 2 ] );
			continue;
		}

		// check for ambient color
		else if ( !Q_stricmp( keyname, "ambientColor" ) )
		{
			if ( r_forceAmbient.Get() == -1 ) {
				sscanf( value, "%f %f %f", &tr.ambientLight[0], &tr.ambientLight[1],
					&tr.ambientLight[2] );

				VectorScale( tr.ambientLight, r_ambientScale.Get(), tr.ambientLight );
				tr.ambientLightSet = true;
			}
		}

		// check for deluxe mapping support
		if ( !Q_stricmp( keyname, "deluxeMapping" ) && !Q_stricmp( value, "1" ) )
		{
			Log::Debug("map features directional light mapping" );
			// This will be disabled if the engine fails to load the lightmaps.
			tr.worldDeluxeMapping = glConfig.deluxeMapping;
			continue;
		}

		if ( !r_overbrightIgnoreMapSettings.Get() )
		{
			// check for mapOverBrightBits override
			if ( !Q_stricmp( keyname, "mapOverBrightBits" ) )
			{
				tr.mapOverBrightBits = Math::Clamp( atof( value ), 0.0, 3.0 );
				continue;
			}
		}

		// check for deluxe mapping provided by NetRadiant's q3map2
		if ( !Q_stricmp( keyname, "_q3map2_cmdline" ) )
		{
			s = strstr( value, "-deluxe" );

			if ( s )
			{
				Log::Debug("map features directional light mapping" );
				// This will be disabled if the engine fails to load the lightmaps.
				tr.worldDeluxeMapping = glConfig.deluxeMapping;
			}

			bool sRGBtex = false;
			bool sRGBcolor = false;
			bool sRGBlight = false;

			s = strstr( value, "-sRGB" );

			if ( s && ( s[5] == ' ' || s[5] == '\0' ) )
			{
				sRGBtex = true;
				sRGBcolor = true;
				sRGBlight = true;
			}

			s = strstr( value, "-nosRGB" );

			if ( s && ( s[5] == ' ' || s[5] == '\0' ) )
			{
				sRGBtex = false;
				sRGBcolor = false;
				sRGBlight = false;
			}

			if ( strstr( value, "-sRGBlight" ) )
			{
				sRGBlight = true;
			}

			if ( strstr( value, "-nosRGBlight" ) )
			{
				sRGBlight = false;
			}

			if ( strstr( value, "-sRGBcolor" ) )
			{
				sRGBcolor = true;
			}

			if ( strstr( value, "-nosRGBcolor" ) )
			{
				sRGBcolor = false;
			}

			if ( strstr( value, "-sRGBtex" ) )
			{
				sRGBtex = true;
			}

			if ( strstr( value, "-nosRGBtex" ) )
			{
				sRGBtex = false;
			}

			if ( sRGBlight )
			{
				Log::Debug( "Map features lights in sRGB colorspace." );
				tr.worldLinearizeLightMap = true;
			}

			if ( sRGBcolor && sRGBtex )
			{
				Log::Debug( "Map features lights computed with linear colors and textures." );
				tr.worldLinearizeTexture = true;
			}
			else if ( sRGBcolor != sRGBtex )
			{
				Log::Warn( "Map features lights computed with a mix of linear and non-linear colors or textures, acting like both colors and textures were linear when lights were computed." );
				tr.worldLinearizeTexture = true;
			}

			continue;
		}

		// check for HDR light mapping support
		if ( !Q_stricmp( keyname, "hdrRGBE" ) && !Q_stricmp( value, "1" ) )
		{
			Log::Debug("map features HDR light mapping" );
			tr.worldHDR_RGBE = true;
			continue;
		}

		if ( !Q_stricmp( keyname, "classname" ) && Q_stricmp( value, "worldspawn" ) )
		{
			Log::Warn("expected worldspawn found '%s'", value );
			continue;
		}
	}

	if ( tr.worldLinearizeTexture )
	{
		if ( r_accurateSRGB.Get() )
		{
			tr.convertFloatFromSRGB = convertFloatFromSRGB_accurate;
			tr.convertColorFromSRGB = convertColorFromSRGB_accurate;
		}
		else
		{
			tr.convertFloatFromSRGB = convertFloatFromSRGB_cheap;
			tr.convertColorFromSRGB = convertColorFromSRGB_cheap;
		}
	}
}

/*
=================
R_GetEntityToken
=================
*/
bool R_GetEntityToken( char *buffer, int size )
{
	const char *s;

	s = COM_Parse2( &s_worldData.entityParsePoint );
	Q_strncpyz( buffer, s, size );

	if ( !s_worldData.entityParsePoint || !s[ 0 ] )
	{
		s_worldData.entityParsePoint = s_worldData.entityString;
		return false;
	}
	else
	{
		return true;
	}
}

static std::string headerString;
static const std::string gridPath = "reflectionCubemaps";
static const std::string gridExtension = ".cubemapGrid";
static bool R_LoadCubeMaps() {
	std::error_code err;

	const std::string dirPath = Str::Format( "%s/%s/", gridPath, tr.world->baseName );
	std::string cubemapGridPath = Str::Format( "%s%s%s", dirPath, tr.world->baseName, gridExtension );
	FS::File cubemapGridFile = FS::HomePath::OpenRead( cubemapGridPath, err );
	if ( err ) {
		if ( err != std::error_code( Util::ordinal( FS::filesystem_error::no_such_file ), FS::filesystem_category() ) ) {
			Log::Notice( "No saved cubemap grid found for %s", cubemapGridPath );
			return false;
		}

		Log::Warn( "Failed to open cubemap probe grid file %s: %s", cubemapGridPath, err.message() );
		return false;
	}

	std::istringstream gridStream( cubemapGridFile.ReadAll() );
	std::string line;

	std::getline( gridStream, line, ' ' );
	const uint32_t cubemapVersion = std::stoi( line );
	std::getline( gridStream, line, '\n' );

	if ( cubemapVersion != REFLECTION_CUBEMAP_VERSION || line != headerString ) {
		Log::Notice( "Saved cube probe version or BSP header doesn't match the current one "
			"(current: version: %u, header: %s; saved: version: %u, header: %s)", REFLECTION_CUBEMAP_VERSION, headerString, cubemapVersion, line );
		return false;
	}

	std::getline( gridStream, line, ' ' );
	const int cubemapSize = std::stoi( line );
	std::getline( gridStream, line, '\n' );
	const uint32_t cubemapSpacing = std::stoi( line );

	if ( cubemapSize != r_cubeProbeSize.Get() || cubemapSpacing != tr.cubeProbeSpacing ) {
		Log::Notice( "Saved cube probe parameters don't match current ones (current: size: %u, spacing: %u; saved: size: %u, spacing: %u)",
			r_cubeProbeSize.Get(), tr.cubeProbeSpacing, cubemapSize, cubemapSpacing );
		return false;
	}

	uint32_t gridSize[3];
	for ( int i = 0; i < 3; i++ ) {
		std::getline( gridStream, line, ' ' );
		gridSize[i] = std::stoi( line );
	}
	tr.cubeProbeGrid.SetSize( gridSize[0], gridSize[1], gridSize[2] );

	std::getline( gridStream, line, '\n' );
	const uint32_t count = std::stoi( line );

	for ( uint32_t i = 0; i < count; i++ ) {
		cubemapProbe_t cubeProbe;
		for ( int j = 0; j < 3; j++ ) {
			std::getline( gridStream, line, ( j < 2 ? ' ' : '\n' ) );
			cubeProbe.origin[j] = std::stof( line );
		}

		imageParams_t imageParams = {};
		imageParams.bits = IF_HOMEPATH;
		imageParams.filterType = filterType_t::FT_DEFAULT;
		imageParams.wrapType = wrapTypeEnum_t::WT_EDGE_CLAMP;

		std::string imageName = Str::Format( "%s%u", dirPath, i );

		image_t* cubemap = R_FindCubeImage( imageName.c_str(), imageParams );
		if ( !cubemap ) {
			Log::Warn( "Failed to load cubemap %s", imageName );
			return false;
		}
		cubeProbe.cubemap = cubemap;

		tr.cubeProbes.push_back( cubeProbe );
	}

	for ( uint32_t i = 0; i < tr.cubeProbeGrid.size; i++ ) {
		std::getline( gridStream, line, '\n' );
		line.erase( std::remove( line.begin(), line.end(), '\r' ), line.end() );
		tr.cubeProbeGrid( i ) = std::stoi( line );
	}

	Log::Notice( "Loaded cubemap probe grid from %s", cubemapGridPath );

	return true;
}

static bool R_SaveCubeMaps() {
	const std::string dirPath = Str::Format( "%s/%s/", gridPath, tr.world->baseName );

	std::error_code err;
	std::string cubemapGridPath = Str::Format( "%s%s%s", dirPath, tr.world->baseName, gridExtension );
	FS::File cubemapGridFile = FS::HomePath::OpenWrite( cubemapGridPath, err );
	if ( err ) {
		Log::Warn( "Failed to open cubemap grid file %s: %s", cubemapGridPath, err.message() );
		return false;
	}

	cubemapGridFile.Printf( "%u %s\n", REFLECTION_CUBEMAP_VERSION, headerString );

	cubemapGridFile.Printf( "%u %u\n", r_cubeProbeSize.Get(), tr.cubeProbeSpacing );

	cubemapGridFile.Printf( "%u %u %u %u\n", tr.cubeProbeGrid.width, tr.cubeProbeGrid.height, tr.cubeProbeGrid.depth, tr.cubeProbes.size() - 1 );

	for ( uint32_t i = 1; i < tr.cubeProbes.size(); i++ ) {
		cubemapGridFile.Printf( "%f %f %f\n", tr.cubeProbes[i].origin[0], tr.cubeProbes[i].origin[1], tr.cubeProbes[i].origin[2] );

		std::string imagePath = Str::Format( "%s%u.ktx", dirPath, i - 1 );

		SaveImageKTX( imagePath.c_str(), tr.cubeProbes[i].cubemap );
	}

	for ( uint32_t i = 0; i < tr.cubeProbeGrid.size; i++ ) {
		cubemapGridFile.Printf( "%u\n", tr.cubeProbeGrid( i ) );
	}

	cubemapGridFile.Flush( err );
	if ( err ) {
		Sys::Drop( "Failed to write cubemap probe grid: %s %s", cubemapGridPath, err.message() );
	}

	Log::Notice( "Saved cubemap probe grid %s", cubemapGridPath );

	return true;
}

void R_GetNearestCubeMaps( const vec3_t position, cubemapProbe_t** cubeProbes, vec4_t trilerp, const uint8_t samples,
	vec3_t* gridPoints ) {
	ASSERT_GE( samples, 1 );
	ASSERT_LE( samples, 4 );

	vec3_t pos;
	VectorCopy( position, pos );
	VectorSubtract( pos, tr.world->nodes[0].mins, pos );
	VectorScale( pos, 1.0 / tr.cubeProbeSpacing, pos );

	struct ProbeTrilerp {
		vec3_t gridPoint;
		uint32_t probe;
		float distance;
	};

	ProbeTrilerp probes[8];

	uint32_t gridPosition[3]{ ( uint32_t ) pos[0], ( uint32_t ) pos[1], ( uint32_t ) pos[2] };
	static uint32_t offsets[8][3] = { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1, 1 }, { 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 0 }, { 1, 1, 1 } };
	for ( int i = 0; i < 8; i++ ) {
		probes[i].probe = tr.cubeProbeGrid( gridPosition[0] + offsets[i][0], gridPosition[1] + offsets[i][1], gridPosition[2] + offsets[i][2] );
		VectorSet( probes[i].gridPoint, gridPosition[0] + offsets[i][0], gridPosition[1] + offsets[i][1], gridPosition[2] + offsets[i][2] );
		probes[i].distance = Distance( pos, probes[i].gridPoint );
	}

	std::sort( std::begin( probes ), std::end( probes ),
		[]( const ProbeTrilerp& lhs, const ProbeTrilerp& rhs ) {
			return lhs.distance < rhs.distance;
		} );

	float distanceSum = 0;
	for ( uint8_t i = 0; i < samples; i++ ) {
		distanceSum += Distance( position, tr.cubeProbes[probes[i].probe].origin );
	}

	for ( uint8_t i = 0; i < samples; i++ ) {
		cubeProbes[i] = &tr.cubeProbes[probes[i].probe];
		trilerp[i] = Distance( position, cubeProbes[i]->origin ) / distanceSum;

		if ( gridPoints != nullptr ) {
			VectorCopy( probes[i].gridPoint, gridPoints[i] );
		}
	}
}

static bool R_NodeSuitableForCubeMap( const bspNode_t* node ) {
	if ( node->area == -1 ) {
		// location is in the void
		return false;
	}

	// This eliminates most of the void nodes, however some may still be left around patch meshes
	if ( !node->numMarkSurfaces ) {
		return false;
	}

	// There might be leafs with only invisible surfaces
	bool hasVisibleSurfaces = false;
	int surfaceCount = node->numMarkSurfaces;
	bspSurface_t** view = tr.world->viewSurfaces + node->firstMarkSurface;
	while ( surfaceCount-- ) {
		bspSurface_t* surface = *view;

		view++;

		if ( *( surface->data ) == surfaceType_t::SF_FACE || *( surface->data ) == surfaceType_t::SF_TRIANGLES
			|| *( surface->data ) == surfaceType_t::SF_VBO_MESH || *( surface->data ) == surfaceType_t::SF_GRID ) {
			hasVisibleSurfaces = true;
			break;
		}
	}

	if ( !hasVisibleSurfaces ) {
		return false;
	}

	return true;
}

static std::unordered_map<const bspNode_t*, uint32_t> cubeProbeMap;
int R_FindNearestCubeMapForGrid( const vec3_t position, bspNode_t* node ) {
	// check to see if this is a shit location
	if ( node->contents == CONTENTS_NODE ) {
		const float distance = DotProduct( position, node->plane->normal ) - node->plane->dist;

		uint32_t side = distance <= 0;

		int out = R_FindNearestCubeMapForGrid( position, node->children[side] );
		if ( out == -1 ) {
			return R_FindNearestCubeMapForGrid( position, node->children[side ^ 1] );
		}

		return out;
	}

	if ( !R_NodeSuitableForCubeMap( node ) ) {
		return -1;
	}

	std::unordered_map<const bspNode_t*, uint32_t>::iterator it = cubeProbeMap.find( node );
	if ( it == cubeProbeMap.end() ) {
		return -1;
	}

	vec3_t origin;
	VectorCopy( node->mins, origin );
	VectorAdd( origin, node->maxs, origin );
	VectorScale( origin, 0.5, origin );

	const byte* vis = R_ClusterPVS( node->cluster );
	const bspNode_t* node2 = R_PointInLeaf( origin );

	if ( !( vis[node->cluster >> 3] & ( 1 << ( node2->cluster & 7 ) ) ) ) {
		return -1;
	}

	return it->second;
}

void R_BuildCubeMaps()
{
	// Early abort if a BSP is not loaded yet since
	// the buildcubemaps command can be called from
	// everywhere including the main menu.
	if ( tr.world == nullptr )
	{
		return;
	}

	if ( !glConfig.reflectionMappingAvailable ) {
		Log::Notice( "Unable to build reflection cubemaps due to incorrect graphics settings" );
		return;
	}

	const int cubeMapSize = r_cubeProbeSize.Get();
	if ( cubeMapSize > glConfig.maxCubeMapTextureSize ) {
		Log::Warn( "Cube probe size exceeds max cubemap texture size (%i/%i)", cubeMapSize, glConfig.maxCubeMapTextureSize );
		return;
	}

	// calculate origins for our probes
	tr.cubeProbes.clear();
	cubeProbeMap.clear();

	cubemapProbe_t defaultCubeProbe{};
	VectorClear( defaultCubeProbe.origin );

	defaultCubeProbe.cubemap = tr.whiteCubeImage;

	tr.cubeProbes.push_back( defaultCubeProbe );

	if ( r_autoBuildCubeMaps.Get() == Util::ordinal( cubeProbesAutoBuildMode::CACHED ) && R_LoadCubeMaps() ) {
		glConfig.reflectionMapping = true;
		return;
	}

	const int startTime = ri.Milliseconds();

	// TODO: Use highest available settings here

	refdef_t rf{};

	for ( int i = 0; i < 6; i++ )
	{
		tr.cubeTemp[ i ] = (byte*) Z_Malloc( ( size_t ) cubeMapSize * cubeMapSize * 4 );
	}

	for ( int i = 0; i < tr.world->numnodes; i++ )
	{
		const bspNode_t* node = &tr.world->nodes[ i ];

		// check to see if this is a shit location
		if ( node->contents == CONTENTS_NODE )
		{
			continue;
		}

		if ( !R_NodeSuitableForCubeMap( node ) ) {
			continue;
		}

		vec3_t origin;
		VectorAdd( node->maxs, node->mins, origin );
		VectorScale( origin, 0.5, origin );

		// Don't spam probes where there's a lot of leafs
		// TODO: Find a better way to determine where to place probes
		if ( std::find_if( cubeProbeMap.begin(), cubeProbeMap.end(),
			[&origin]( const std::unordered_map<const bspNode_t*, uint32_t>::value_type& nodeCheck ) {
				vec3_t nodeOrigin;
				VectorAdd( nodeCheck.first->mins, nodeCheck.first->maxs, nodeOrigin );
				VectorScale( nodeOrigin, 0.5, nodeOrigin );

				return Distance( origin, nodeOrigin ) <= tr.cubeProbeSpacing;
			} ) != cubeProbeMap.end() ) {
			continue;
		}

		cubemapProbe_t cubeProbe {};
		VectorCopy( origin, cubeProbe.origin );
		tr.cubeProbes.push_back( cubeProbe );

		cubeProbeMap[node] = tr.cubeProbes.size() - 1;
	}

	Log::Notice( "Using cube probe grid size: %u %u %u", tr.cubeProbeGrid.width, tr.cubeProbeGrid.height, tr.cubeProbeGrid.depth );

	for( Grid<uint32_t>::Iterator it = tr.cubeProbeGrid.begin(); it != tr.cubeProbeGrid.end(); it++ ) {
		uint32_t x;
		uint32_t y;
		uint32_t z;
		tr.cubeProbeGrid.IteratorToCoords( it, &x, &y, &z );
		vec3_t position{ ( float ) x * tr.cubeProbeSpacing, ( float ) y * tr.cubeProbeSpacing, ( float ) z * tr.cubeProbeSpacing };

		// Match the map's start coords
		VectorAdd( position, tr.world->nodes[0].mins, position );

		int cubeProbe = R_FindNearestCubeMapForGrid( position, &tr.world->nodes[0] );

		if ( cubeProbe == -1 ) {
			cubeProbe = 0;
		}
		tr.cubeProbeGrid( x, y, z ) = cubeProbe;
	}

	Log::Notice( "...pre-rendering %d cubemaps", tr.cubeProbes.size() );

	const bool gpuOcclusionCulling = r_gpuOcclusionCulling.Get();
	r_gpuOcclusionCulling.Set( false );

	// We still need to run the cameraEffects shader for overbright, so set r_gamma to 1.0 here to avoid applying it twice to the reflection
	const float gamma = r_gamma->value;
	Cvar_SetValue( "r_gamma", 1.0 );

	for ( size_t i = 0; i < tr.cubeProbes.size(); i++ )
	{
		cubemapProbe_t* cubeProbe = &tr.cubeProbes[i];

		VectorCopy( cubeProbe->origin, rf.vieworg );

		AxisClear( rf.viewaxis );

		rf.fov_x = 90;
		rf.fov_y = 90;
		rf.x = 0;
		rf.y = 0;
		rf.width = cubeMapSize;
		rf.height = cubeMapSize;
		rf.time = 0;

		rf.gradingWeights[0] = 0.0;
		rf.gradingWeights[1] = 0.0;
		rf.gradingWeights[2] = 0.0;
		rf.gradingWeights[3] = 1.0;

		rf.rdflags = RDF_NOCUBEMAP | RDF_NOBLOOM;

		for ( int j = 0; j < 6; j++ )
		{

			switch ( j )
			{
				case 0:
					{
						//X+
						rf.viewaxis[ 0 ][ 0 ] = 1;
						rf.viewaxis[ 0 ][ 1 ] = 0;
						rf.viewaxis[ 0 ][ 2 ] = 0;

						rf.viewaxis[ 1 ][ 0 ] = 0;
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = 1;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						break;
					}

				case 1:
					{
						//X-
						rf.viewaxis[ 0 ][ 0 ] = -1;
						rf.viewaxis[ 0 ][ 1 ] = 0;
						rf.viewaxis[ 0 ][ 2 ] = 0;

						rf.viewaxis[ 1 ][ 0 ] = 0;
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = -1;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						break;
					}

				case 2:
					{
						//Y+
						rf.viewaxis[ 0 ][ 0 ] = 0;
						rf.viewaxis[ 0 ][ 1 ] = 1;
						rf.viewaxis[ 0 ][ 2 ] = 0;

						rf.viewaxis[ 1 ][ 0 ] = -1;
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = 0;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						break;
					}

				case 3:
					{
						//Y-
						rf.viewaxis[ 0 ][ 0 ] = 0;
						rf.viewaxis[ 0 ][ 1 ] = -1;
						rf.viewaxis[ 0 ][ 2 ] = 0;

						rf.viewaxis[ 1 ][ 0 ] = -1;
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = 0;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						break;
					}

				case 4:
					{
						//Z+
						rf.viewaxis[ 0 ][ 0 ] = 0;
						rf.viewaxis[ 0 ][ 1 ] = 0;
						rf.viewaxis[ 0 ][ 2 ] = 1;

						rf.viewaxis[ 1 ][ 0 ] = -1;
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = 0;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						break;
					}

				case 5:
					{
						//Z-
						rf.viewaxis[ 0 ][ 0 ] = 0;
						rf.viewaxis[ 0 ][ 1 ] = 0;
						rf.viewaxis[ 0 ][ 2 ] = -1;

						rf.viewaxis[ 1 ][ 0 ] = 1;
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = 0;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						break;
					}
			}

			tr.refdef.pixelTarget = tr.cubeTemp[ j ];
			memset( tr.cubeTemp[ j ], 255, ( size_t ) cubeMapSize * cubeMapSize * 4 );
			tr.refdef.pixelTargetWidth = cubeMapSize;
			tr.refdef.pixelTargetHeight = cubeMapSize;

			int msecUnused1;
			int msecUnused2;
			// Material system writes culled surfaces for the next frame, so we need to render twice with it to cull correctly
			if ( glConfig.usingMaterialSystem ) {
				tr.refdef.pixelTarget = nullptr;

				RE_BeginFrame();
				RE_RenderScene( &rf );
				RE_EndFrame( &msecUnused1, &msecUnused2 );

				tr.refdef.pixelTarget = tr.cubeTemp[j];
			}

			RE_BeginFrame();
			RE_RenderScene( &rf );
			RE_EndFrame( &msecUnused1, &msecUnused2 );

			// encode the pixel intensity into the alpha channel, saves work in the shader
			byte best;

			byte* dest = tr.cubeTemp[ j ];

			for ( int y = 0; y < cubeMapSize; y++ )
			{
				for ( int x = 0; x < cubeMapSize; x++ )
				{
					int xy = ( ( y * cubeMapSize ) + x ) * 4;

					const byte r = dest[ xy + 0 ];
					const byte g = dest[ xy + 1 ];
					const byte b = dest[ xy + 2 ];

					if ( ( r > g ) && ( r > b ) )
					{
						best = r;
					}
					else if ( ( g > r ) && ( g > b ) )
					{
						best = g;
					}
					else
					{
						best = b;
					}

					dest[ xy + 3 ] = best;
				}
			}
		}

		// build the cubemap
		std::string name = Str::Format( "_autoCube%d", i );
		cubeProbe->cubemap = R_AllocImage( name.c_str(), false );

		if ( !cubeProbe->cubemap )
		{
			return;
		}

		cubeProbe->cubemap->type = GL_TEXTURE_CUBE_MAP;

		cubeProbe->cubemap->width = cubeMapSize;
		cubeProbe->cubemap->height = cubeMapSize;

		cubeProbe->cubemap->bits = IF_NOPICMIP;
		cubeProbe->cubemap->filterType = filterType_t::FT_LINEAR;
		cubeProbe->cubemap->wrapType = wrapTypeEnum_t::WT_EDGE_CLAMP;

		imageParams_t imageParams = {};

		R_UploadImage( name.c_str(), ( const byte ** ) tr.cubeTemp, 6, 1, cubeProbe->cubemap, imageParams );
	}

	r_gpuOcclusionCulling.Set( gpuOcclusionCulling );

	Cvar_SetValue( "r_gamma", gamma );

	// turn pixel targets off
	tr.refdef.pixelTarget = nullptr;

	glConfig.reflectionMapping = true;

	const int endTime = ri.Milliseconds();
	Log::Notice( "Cubemap probes pre-rendering time of %d cubes = %5.2f seconds", tr.cubeProbes.size(),
	           ( endTime - startTime ) / 1000.0 );

	if ( r_autoBuildCubeMaps.Get() == Util::ordinal( cubeProbesAutoBuildMode::CACHED ) ) {
		R_SaveCubeMaps();
	}
}

static Cmd::LambdaCmd buildCubeMapsCmd(
	"buildcubemaps", Cmd::RENDERER, "generate cube probes for reflection mapping",
	[]( const Cmd::Args & ) { R_BuildCubeMaps(); });

static void SetWorldLight() {
	tr.worldLight = tr.lightMode;
	tr.modelLight = lightMode_t::FULLBRIGHT;
	tr.modelDeluxe = deluxeMode_t::NONE;

	// Use fullbright lighting for everything if the world is fullbright.
	if ( tr.worldLight != lightMode_t::FULLBRIGHT ) {
		if ( tr.worldLight == lightMode_t::MAP ) {
			// World surfaces use light mapping.

			if ( !tr.worldLightMapping ) {
				/* Use vertex light as a fallback on world surfaces missing a light map,
				q3map2 has an option to produce less lightmap files by skipping them when
				they are very similar to the vertex color. The vertex color is expected
				to match the color of the nearby lightmaps. We better not want to use
				the grid light as a fallback as it would be close but not close enough. */

				tr.worldLight = lightMode_t::VERTEX;
			}
		} else if ( tr.worldLight == lightMode_t::GRID ) {
			if ( !tr.lightGrid1Image ) {
				// Use vertex light on world surface if light color grid is missing.
				tr.worldLight = lightMode_t::VERTEX;
			}
		}

		if ( tr.worldDeluxeMapping ) {
			if ( tr.worldLight == lightMode_t::MAP ) {
				tr.worldDeluxe = deluxeMode_t::MAP;
			}

			/* The combination of grid light and deluxe map is
			technically doable, but rendering the world with a
			light grid while a light map is available is not
			the experience we want to provide, so we don't
			allow this combination to not compile the related
			shaders. */
		}

		/* We can technically use emulated deluxe map from light direction dir
		on surfaces with light map but no deluxe map, but this is ugly.
		Also, enabling it would require to make some macro not conflicting and
		then would increase the amount of GLSL shader variants to be compiled,
		this to render legacy maps in a way legacy renderers never rendered them.
		It could still be cool as an optional feature, if we use a better
		algorithm for emulating the deluxe map from light direction grid.
		See https://github.com/DaemonEngine/Daemon/issues/32 */

		if ( tr.lightGrid1Image ) {
			// Game model surfaces use grid lighting, they don't have vertex light colors.
			tr.modelLight = lightMode_t::GRID;
		}

		// Enable deluxe mapping emulation if light direction grid is there.
		if ( tr.lightGrid2Image ) {
			ASSERT( glConfig.deluxeMapping );
			// Game model surfaces use grid lighting, they don't have vertex light colors.
			tr.modelDeluxe = deluxeMode_t::GRID;

			// Only game models use emulated deluxe map from light direction grid.
		}
	}

	/* Set GLSL overbright parameters if the lighting mode is not fullbright. */
	if ( tr.lightMode != lightMode_t::FULLBRIGHT ) {
		if ( r_overbrightQ3.Get() ) {
			// light factor is applied to entire color buffer; identityLight can be used to cancel it
			tr.identityLight = 1.0f / float( 1 << tr.overbrightBits );
		} else {
			// light factor is applied wherever a precomputed light is sampled
			tr.mapLightFactor = float( 1 << tr.overbrightBits );
		}
	}
}

static void SetConstUniforms() {
	GLIMP_LOGCOMMENT( "--- SetConstUniforms ---" );

	globalUBOProxy->SetUniform_LightGridOrigin( tr.world->lightGridGLOrigin );
	globalUBOProxy->SetUniform_LightGridScale( tr.world->lightGridGLScale );

	globalUBOProxy->SetUniform_GlobalLightFactor( 1.0f / tr.identityLight );
	globalUBOProxy->SetUniform_SRGB( tr.worldLinearizeTexture );

	globalUBOProxy->SetUniform_ProfilerZero();

	if ( glConfig.usingBindlessTextures ) {
		if ( glConfig.colorGrading ) {
			globalUBOProxy->SetUniform_ColorMap3DBindless( GL_BindToTMU( 3, tr.colorGradeImage ) );
		}

		globalUBOProxy->SetUniform_DepthMapBindless( GL_BindToTMU( 1, tr.depthSamplerImage ) );

		if ( r_liquidMapping->integer ) {
			globalUBOProxy->SetUniform_PortalMapBindless( GL_BindToTMU( 1, tr.portalRenderImage ) );
		}

		if ( glConfig.realtimeLighting ) {
			globalUBOProxy->SetUniform_DepthTile1Bindless(
				GL_BindToTMU( 0, tr.depthtile1RenderImage )
			);

			globalUBOProxy->SetUniform_DepthTile2Bindless(
				GL_BindToTMU( 1, tr.depthtile2RenderImage )
			);

			globalUBOProxy->SetUniform_LightTilesBindless(
				GL_BindToTMU( BIND_LIGHTTILES, tr.lighttileRenderImage )
			);
		}

		globalUBOProxy->SetUniform_LightGrid1Bindless( GL_BindToTMU( BIND_LIGHTGRID1, tr.lightGrid1Image ) );

		if ( tr.lightGrid2Image )
		{
			globalUBOProxy->SetUniform_LightGrid2Bindless( GL_BindToTMU( BIND_LIGHTGRID2, tr.lightGrid2Image ) );
		}
	}

	if ( glConfig.usingMaterialSystem ) {
		materialSystem.SetConstUniforms();
	}

	if ( !globalUBOProxy->uniformsUpdated ) {
		return;
	}

	uint32_t* data = pushBuffer.MapGlobalUniformData( GLUniform::CONST );
	globalUBOProxy->WriteUniformsToBuffer( data, GLShader::PUSH, GLUniform::CONST );

	pushBuffer.PushGlobalUniforms();
}

/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
void RE_LoadWorldMap( const char *name )
{
	int       i;
	dheader_t *header;
	byte      *startMarker;

	if ( tr.worldMapLoaded )
	{
		Sys::Drop( "ERROR: attempted to redundantly load world map" );
	}

	Log::Debug("----- RE_LoadWorldMap( %s ) -----", name );

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[ 0 ] = 0.45f;
	tr.sunDirection[ 1 ] = 0.3f;
	tr.sunDirection[ 2 ] = 0.9f;

	VectorNormalize( tr.sunDirection );

	tr.worldMapLoaded = true;

	// load it
	std::error_code err;
	std::string buffer = FS::PakPath::ReadFile( name, err );

	if ( err )
	{
		Sys::Drop( "RE_LoadWorldMap: %s not found", name );
	}

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = nullptr;

	// It's probably a mistake if any of these lighting parameters are actually
	// used before a map is loaded.
	tr.worldLightMapping = false; // set by R_LoadLightmaps
	tr.worldDeluxeMapping = false; // set by R_LoadEntities
	tr.worldHDR_RGBE = false; // set by R_LoadEntities
	tr.mapOverBrightBits = r_overbrightDefaultExponent.Get(); // maybe set by R_LoadEntities
	tr.overbrightBits = std::min( tr.mapOverBrightBits, r_overbrightBits.Get() ); // set by RE_LoadWorldMap
	tr.mapLightFactor = 1.0f; // set by RE_LoadWorldMap
	tr.identityLight = 1.0f; // set by RE_LoadWorldMap
	tr.worldLinearizeTexture = false;
	tr.worldLinearizeLightMap = false;

	s_worldData = {};
	Q_strncpyz( s_worldData.name, name, sizeof( s_worldData.name ) );

	Q_strncpyz( s_worldData.baseName, COM_SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
	COM_StripExtension3( s_worldData.baseName, s_worldData.baseName, sizeof( s_worldData.baseName ) );
	tr.loadingMap = s_worldData.baseName;

	startMarker = (byte*) ri.Hunk_Alloc( 0, ha_pref::h_low );

	header = ( dheader_t * ) buffer.data();
	fileBase = ( byte * ) header;

	i = LittleLong( header->version );

	if ( i != BSP_VERSION && i != BSP_VERSION_Q3 )
	{
		Sys::Drop( "RE_LoadWorldMap: %s has wrong version number (%i should be %i for ET or %i for Q3)",
		           name, i, BSP_VERSION, BSP_VERSION_Q3 );
	}

	// swap all the lumps
	for ( unsigned j = 0; j < sizeof( dheader_t ) / 4; j++ )
	{
		( ( int * ) header ) [ j ] = LittleLong( ( ( int * ) header ) [ j ] );
	}

	if ( glConfig.reflectionMappingAvailable ) {
		// TODO: Take into account potential shader changes
		headerString = Str::Format( "%i %i %i %i %i", header->lumps[LUMP_PLANES].filelen, header->lumps[LUMP_NODES].filelen,
			header->lumps[LUMP_LEAFS].filelen, header->lumps[LUMP_BRUSHES].filelen, header->lumps[LUMP_SURFACES].filelen );
	}

	// load into heap

	std::string externalEntitiesFileName = FS::Path::StripExtension( name ) + ".ent";
	std::string externalEntities = FS::PakPath::ReadFile( externalEntitiesFileName, err );
	if ( err )
	{
		const std::error_code notFound( Util::ordinal( FS::filesystem_error::no_such_file ), FS::filesystem_category() );
		if ( err != notFound )
		{
			Sys::Drop( "Could not read file '%s': %s", externalEntitiesFileName.c_str(), err.message() );
		}
		externalEntities = "";
	}
	R_LoadEntities( &header->lumps[ LUMP_ENTITIES ], externalEntities );

	// Now we can set this after checking a possible worldspawn value for mapOverbrightBits
	if ( tr.worldLinearizeLightMap )
	{
		/* Never shift and clamp the lightmap when stored in sRGB space, that breaks it.
		Also maps using lightmaps in sRGB space are maintained maps and there is
		no need to provide such clamping meant for compatibility with legacy maps. */
		tr.overbrightBits = tr.mapOverBrightBits;
	}
	else
	{
		tr.overbrightBits = std::min( tr.mapOverBrightBits, r_overbrightBits.Get() );
	}

	R_LoadShaders( &header->lumps[ LUMP_SHADERS ] );

	R_LoadLightmaps( &header->lumps[ LUMP_LIGHTMAPS ], name );

	R_LoadPlanes( &header->lumps[ LUMP_PLANES ] );

	R_LoadSurfaces( &header->lumps[ LUMP_SURFACES ], &header->lumps[ LUMP_DRAWVERTS ], &header->lumps[ LUMP_DRAWINDEXES ] );

	R_LoadMarksurfaces( &header->lumps[ LUMP_LEAFSURFACES ] );

	R_LoadNodesAndLeafs( &header->lumps[ LUMP_NODES ], &header->lumps[ LUMP_LEAFS ] );

	R_LoadSubmodels( &header->lumps[ LUMP_MODELS ] );

	// moved fog lump loading here, so fogs can be tagged with a model num
	R_LoadFogs( &header->lumps[ LUMP_FOGS ], &header->lumps[ LUMP_BRUSHES ], &header->lumps[ LUMP_BRUSHSIDES ] );

	R_LoadVisibility( &header->lumps[ LUMP_VISIBILITY ] );

	R_LoadLightGrid( &header->lumps[ LUMP_LIGHTGRID ] );

	// create a static vbo for the world
	// Do SetWorldLight() before R_CreateWorldVBO(), because the latter will use the world light values to generate materials
	SetWorldLight();

	R_CreateWorldVBO();
	R_CreateClusters();

	if ( tr.hasSkybox ) {
		FinishSkybox();
	}

	s_worldData.dataSize = ( byte * ) ri.Hunk_Alloc( 0, ha_pref::h_low ) - startMarker;
	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

	tr.worldLoaded = true;
	tr.loadingMap = "";
	GLSL_InitWorldShaders();

	if ( glConfig.pushBufferAvailable ) {
		SetConstUniforms();
	}

	if ( glConfig.reflectionMappingAvailable ) {
		tr.cubeProbeSpacing = r_cubeProbeSpacing.Get();

		vec3_t worldSize;
		VectorCopy( tr.world->nodes[0].maxs, worldSize );
		VectorSubtract( worldSize, tr.world->nodes[0].mins, worldSize );

		uint32_t gridSize[3];
		gridSize[0] = ( worldSize[0] + tr.cubeProbeSpacing - 1 ) / tr.cubeProbeSpacing;
		gridSize[1] = ( worldSize[1] + tr.cubeProbeSpacing - 1 ) / tr.cubeProbeSpacing;
		gridSize[2] = ( worldSize[2] + tr.cubeProbeSpacing - 1 ) / tr.cubeProbeSpacing;

		// Make sure we don't create a way too large grid if the map is bigger than some arbitrary number (2mb grid max) (e. g. epic5)
		if ( gridSize[0] * gridSize[1] * gridSize[2] > ( 1 << 19 ) ) {
			tr.cubeProbeSpacing = std::cbrt( ( worldSize[0] + 1 ) * ( worldSize[1] + 1 ) * ( worldSize[2] + 1 ) / ( 1 << 19 ) );
			Log::Notice( "Map size too large (%f %f %f), using %u for cube probe spacing instead of %u",
				worldSize[0], worldSize[1], worldSize[2], tr.cubeProbeSpacing, r_cubeProbeSpacing.Get() );

			gridSize[0] = ( worldSize[0] + tr.cubeProbeSpacing - 1 ) / tr.cubeProbeSpacing;
			gridSize[1] = ( worldSize[1] + tr.cubeProbeSpacing - 1 ) / tr.cubeProbeSpacing;
			gridSize[2] = ( worldSize[2] + tr.cubeProbeSpacing - 1 ) / tr.cubeProbeSpacing;
		}

		tr.cubeProbeGrid.SetSize( gridSize[0], gridSize[1], gridSize[2] );
	}
}
