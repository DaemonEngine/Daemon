/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// tr_font.c

#include "tr_local.h"

#include "qcommon/qcommon.h"
#include "qcommon/q_unicode.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ERRORS_H
#include FT_SYSTEM_H
#include FT_IMAGE_H
#include FT_OUTLINE_H

#define _FLOOR( x ) ( ( x ) & - 64 )
#define _CEIL( x )  ( ( ( x ) + 63 ) & - 64 )
#define _TRUNC( x ) ( ( x ) >> 6 )

using glyphBlock_t = glyphInfo_t[256];

FT_Library ftLibrary = nullptr;

static const int FONT_SIZE = 512;

void RE_RenderChunk( fontInfo_t *font, const int chunk );


void R_GetGlyphInfo( FT_GlyphSlot glyph, int *left, int *right, int *width, int *top, int *bottom, int *height, int *pitch )
{
	// ±1 adjustments for border-related reasons - really want clamp to transparent border (FIXME)

	*left = _FLOOR( glyph->metrics.horiBearingX - 1);
	*right = _CEIL( glyph->metrics.horiBearingX + glyph->metrics.width + 1);
	*width = _TRUNC( *right - *left );

	*top = _CEIL( glyph->metrics.horiBearingY + 1);
	*bottom = _FLOOR( glyph->metrics.horiBearingY - glyph->metrics.height - 1);
	*height = _TRUNC( *top - *bottom );
	*pitch = ( *width + 3 ) & - 4;
}

FT_Bitmap      *R_RenderGlyph( FT_GlyphSlot glyph, glyphInfo_t *glyphOut )
{
	FT_Bitmap *bit2;
	int       left, right, width, top, bottom, height, pitch, size;

	R_GetGlyphInfo( glyph, &left, &right, &width, &top, &bottom, &height, &pitch );

	if ( glyph->format == ft_glyph_format_outline )
	{
		size = pitch * height;

		bit2 = (FT_Bitmap*) Z_Malloc( sizeof( FT_Bitmap ) );

		bit2->width = width;
		bit2->rows = height;
		bit2->pitch = pitch;
		bit2->pixel_mode = ft_pixel_mode_grays;
		bit2->buffer = (unsigned char*) Z_Calloc( size );
		bit2->num_grays = 256;

		FT_Outline_Translate( &glyph->outline, -left, -bottom );

		FT_Outline_Get_Bitmap( ftLibrary, &glyph->outline, bit2 );

		glyphOut->height = height;
		glyphOut->pitch = pitch;
		glyphOut->top = ( glyph->metrics.horiBearingY >> 6 ) + 1;
		glyphOut->bottom = bottom;

		return bit2;
	}
	else
	{
		Log::Warn("Non-outline fonts are not supported" );
	}

	return nullptr;
}

static glyphInfo_t *RE_ConstructGlyphInfo( unsigned char *imageOut, int *xOut, int *yOut,
    int *maxHeight, FT_Face face, const int c, bool calcHeight )
{
	int                i;
	static glyphInfo_t glyph;
	unsigned char      *src, *dst;
	float              scaledWidth, scaledHeight;
	FT_Bitmap          *bitmap = nullptr;

	glyph = {};

	// make sure everything is here
	if ( face != nullptr )
	{
		FT_UInt index = FT_Get_Char_Index( face, c );

		if ( index == 0 )
		{
			return nullptr; // nothing to render
		}

		FT_Load_Glyph( face, index, FT_LOAD_DEFAULT );
		bitmap = R_RenderGlyph( face->glyph, &glyph );

		if ( bitmap )
		{
			glyph.xSkip = ( face->glyph->metrics.horiAdvance >> 6 ) + 1;
		}
		else
		{
			return nullptr;
		}

		if ( glyph.height > *maxHeight )
		{
			*maxHeight = glyph.height;
		}

		if ( calcHeight )
		{
			Z_Free( bitmap->buffer );
			Z_Free( bitmap );
			return &glyph;
		}

		scaledWidth = glyph.pitch;
		scaledHeight = glyph.height;

		// we need to make sure we fit
		if ( *xOut + scaledWidth + 1 >= ( FONT_SIZE - 1 ) )
		{
			*xOut = 0;
			*yOut += *maxHeight + 1;
		}

		if ( *yOut + *maxHeight + 1 >= ( FONT_SIZE - 1 ) )
		{
			*xOut = -1;
			Z_Free( bitmap->buffer );
			Z_Free( bitmap );
			return nullptr;
		}

		src = bitmap->buffer;
		dst = imageOut + ( *yOut * FONT_SIZE ) + *xOut;

		if ( bitmap->pixel_mode == ft_pixel_mode_mono )
		{
			for ( i = 0; i < glyph.height; i++ )
			{
				int           j;
				unsigned char *_src = src;
				unsigned char *_dst = dst;
				unsigned char mask = 0x80;
				unsigned char val = *_src;

				for ( j = 0; j < glyph.pitch; j++ )
				{
					if ( mask == 0x80 )
					{
						val = *_src++;
					}

					if ( val & mask )
					{
						*_dst = 0xff;
					}

					mask >>= 1;

					if ( mask == 0 )
					{
						mask = 0x80;
					}

					_dst++;
				}

				src += glyph.pitch;
				dst += FONT_SIZE;
			}
		}
		else
		{
			for ( i = 0; i < glyph.height; i++ )
			{
				memcpy( dst, src, glyph.pitch );
				src += glyph.pitch;
				dst += FONT_SIZE;
			}
		}

		// we now have an 8 bit per pixel grey scale bitmap
		// that is width wide and pf->ftSize->metrics.y_ppem tall

		glyph.imageHeight = scaledHeight;
		glyph.imageWidth = scaledWidth;
		glyph.s = ( float ) * xOut / FONT_SIZE;
		glyph.t = ( float ) * yOut / FONT_SIZE;
		glyph.s2 = glyph.s + ( float ) scaledWidth / FONT_SIZE;
		glyph.t2 = glyph.t + ( float ) scaledHeight / FONT_SIZE;
		glyph.shaderName[0] = 1; // flag that we have a glyph here

		*xOut += scaledWidth + 1;

		Z_Free( bitmap->buffer );
		Z_Free( bitmap );

		return &glyph;
	}

	return nullptr;
}

void RE_GlyphChar( fontInfo_t *font, int ch, glyphInfo_t *glyph )
{
	// default if out of range
	if ( ch < 0 || ( ch >= 0xD800 && ch < 0xE000 ) || ch >= 0x110000 || ch == 0xFFFD )
	{
		ch = 0;
	}

	// render if needed
	if ( !font->glyphBlock[ ch / 256 ] )
	{
		RE_RenderChunk( font, ch / 256 );
	}

	// default if no glyph
	if ( !font->glyphBlock[ ch / 256 ][ ch % 256 ].glyph )
	{
		ch = 0;
	}

	// we have a glyph
	*glyph = font->glyphBlock[ ch / 256][ ch % 256 ];
}

static void RE_StoreImage( fontInfo_t *font, int chunk, int page, int from, int to, const unsigned char *bitmap, int yEnd )
{
	int           scaledSize = FONT_SIZE * FONT_SIZE;
	int           i, j, y;
	float         max;

	glyphInfo_t   *glyphs = font->glyphBlock[chunk];

	unsigned char *buffer;
	image_t       *image;
	qhandle_t     h;

	char          fileName[ MAX_QPATH ];

	// about to render an image
	R_SyncRenderThread();

	// maybe crop image while retaining power-of-2 height
	i = 1;
	y = FONT_SIZE;

	// How much to reduce it?
	while ( yEnd < y / 2 - 1 ) { i += i; y /= 2; }

	// Fix up the glyphs' Y co-ordinates
	for ( j = from; j < to; j++ ) { glyphs[j].t *= i; glyphs[j].t2 *= i; }

	scaledSize /= i;

	max = 0;

	for ( i = 0; i < scaledSize; i++ )
	{
		if ( max < bitmap[ i ] )
		{
			max = bitmap[ i ];
		}
	}

	if ( max > 0 )
	{
		max = 255 / max;
	}

	buffer = ( unsigned char * ) Z_AllocUninit( scaledSize * 4 );
	for ( i = j = 0; i < scaledSize; i++ )
	{
		buffer[ j++ ] = 255;
		buffer[ j++ ] = 255;
		buffer[ j++ ] = 255;
		buffer[ j++ ] = ( ( float ) bitmap[ i ] * max );
	}

	Com_sprintf( fileName, sizeof( fileName ), "*%s_%i_%i_%i", font->name, chunk, page, font->pointSize );

	image = R_CreateGlyph( fileName, buffer, FONT_SIZE, y );

	Z_Free( buffer );

	h = RE_RegisterShaderFromImage( fileName, image );

	for ( j = from; j < to; j++ )
	{
		if ( font->glyphBlock[ chunk ][ j ].shaderName[0] ) // non-0 if we have a glyph here
		{
			font->glyphBlock[ chunk ][ j ].glyph = h;
			Q_strncpyz( font->glyphBlock[ chunk ][ j ].shaderName, fileName, sizeof( font->glyphBlock[ chunk ][ j ].shaderName ) );
		}
	}
}

static glyphBlock_t nullGlyphs;

void RE_RenderChunk( fontInfo_t *font, const int chunk )
{
	int           xOut, yOut, maxHeight;
	int           i, lastStart, page;
	unsigned char *out;
	glyphInfo_t   *glyphs;
	bool      rendered;

	const int     startGlyph = chunk * 256;

	// sanity check
	if ( chunk < 0 || chunk >= 0x1100 || font->glyphBlock[ chunk ] )
	{
		return;
	}

	out = (unsigned char*) Z_Calloc( FONT_SIZE * FONT_SIZE );

	// calculate max height
	maxHeight = 0;
	rendered = false;

	for ( i = 0; i < 256; i++ )
	{
		rendered |= !!RE_ConstructGlyphInfo( out, &xOut, &yOut, &maxHeight, (FT_Face) font->face, ( i + startGlyph ) ? ( i + startGlyph ) : 0xFFFD, true );
	}

	// no glyphs? just return
	if ( !rendered )
	{
		Z_Free( out );
		font->glyphBlock[ chunk ] = nullGlyphs;
		return;
	}

	glyphs = font->glyphBlock[ chunk ] = (glyphInfo_t*) Z_Calloc( sizeof( glyphBlock_t ) );

	xOut = yOut = 0;
	rendered = false;
	i = lastStart = page = 0;

	while ( i < 256 )
	{
		glyphInfo_t *glyph = RE_ConstructGlyphInfo( out, &xOut, &yOut, &maxHeight, (FT_Face) font->face, ( i + startGlyph ) ? ( i + startGlyph ) : 0xFFFD, false );

		if ( glyph )
		{
			rendered = true;
			glyphs[ i ] = *glyph;
		}

		if ( xOut == -1 )
		{
			RE_StoreImage( font, chunk, page++, lastStart, i, out, yOut + maxHeight + 1 );
			memset( out, 0, FONT_SIZE * FONT_SIZE );
			xOut = yOut = 0;
			rendered = false;
			lastStart = i;
		}
		else
		{
			i++;
		}
	}

	if ( rendered )
	{
		RE_StoreImage( font, chunk, page, lastStart, 256, out, yOut + maxHeight + 1 );
	}
	Z_Free( out );
}

static int RE_LoadFontFile( const char *name, void **buffer )
{
	void *tmp;
	int  length = ri.FS_ReadFile( name, &tmp );

	if ( length <= 0 )
	{
		return 0;
	}

	void *data = Z_AllocUninit( length );
	*buffer = data;

	memcpy( data, tmp, length );
	ri.FS_FreeFile( tmp );

	return length;
}

static void RE_FreeFontFile( void *data )
{
	Z_Free( data );
}

fontInfo_t* RE_RegisterFont( const char *fontName, int pointSize )
{
	FT_Face       face;
	void          *faceData = nullptr;
	int           len;
	char          strippedName[ MAX_QPATH ];

	if ( pointSize <= 0 )
	{
		pointSize = 12;
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	COM_StripExtension2( fontName, strippedName, sizeof( strippedName ) );

	if ( ftLibrary == nullptr )
	{
		Log::Warn("RE_RegisterFont: FreeType not initialized." );
		return nullptr;
	}

	len = RE_LoadFontFile( fontName, &faceData );

	if ( len <= 0 )
	{
		Log::Warn("RE_RegisterFont: Unable to read font file %s", fontName );
		RE_FreeFontFile( faceData );
		return nullptr;
	}

	// allocate on the stack first in case we fail
	if ( FT_New_Memory_Face( ftLibrary, (FT_Byte*) faceData, len, 0, &face ) )
	{
		Log::Warn("RE_RegisterFont: FreeType2, unable to allocate new face." );
		RE_FreeFontFile( faceData );
		return nullptr;
	}

	if ( FT_Set_Char_Size( face, pointSize << 6, pointSize << 6, 72, 72 ) )
	{
		Log::Warn("RE_RegisterFont: FreeType2, Unable to set face char size." );
		FT_Done_Face( face );
		RE_FreeFontFile( faceData );
		return nullptr;
	}

	auto *font = new fontInfo_t{};
	Q_strncpyz( font->name, strippedName, sizeof( font->name ) );
	font->face = face;
	font->faceData = faceData;
	font->pointSize = pointSize;

	RE_RenderChunk( font, 0 );

	return font;
}

void R_InitFreeType()
{
	if ( FT_Init_FreeType( &ftLibrary ) )
	{
		Log::Warn("R_InitFreeType: Unable to initialize FreeType." );
	}
}

void RE_UnregisterFont( fontInfo_t *font )
{
	if ( !font )
	{
		return;
	}

	if ( font->face )
	{
		FT_Done_Face( (FT_Face) font->face );
		RE_FreeFontFile( font->faceData );
	}

	for ( int i = 0; i < 0x1100; ++i )
	{
		if ( font->glyphBlock[ i ] && font->glyphBlock[ i ] != nullGlyphs )
		{
			Z_Free( font->glyphBlock[ i ] );
			font->glyphBlock[ i ] = nullptr;
		}
	}

	delete font;
}

void R_DoneFreeType()
{
	if ( ftLibrary )
	{
		FT_Done_FreeType( ftLibrary );
		ftLibrary = nullptr;
	}
}
