/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2013-2016 Daemon Developers
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
// InternalImage.cpp
#include "tr_local.h"

// Comments may include short quotes from other authors.

int R_GetBlockSize( const image_t *image )
{
	switch ( image->internalFormat )
	{
		// S3TC formats

		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
			return 8;

		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
			return 16;

		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
			return 16;

		// RGTC formats

		case GL_COMPRESSED_RED_RGTC1:
			return 8;
		case GL_COMPRESSED_RG_RGTC2:
			return 16;

		/* Others

		Assume 16 so it may be large enough. */

		default:
			Log::Debug( "Unknown block size for undocumented internal format %i for image %s", image->internalFormat, image->name );
			return 16;
	}
}

int R_GetMipSize( int imageWidth, int imageHeight, int blockSize )
{
	return ( ( imageWidth + 3 ) >> 2 ) * ( ( imageHeight + 3 ) >> 2 ) * blockSize;
}

static int R_GetMipSize( const image_t *image )
{
	int imageWidth = image->uploadWidth;
	int imageHeight = image->uploadHeight;
	int blockSize = R_GetBlockSize( image );

	return R_GetMipSize( imageWidth, imageHeight, blockSize );
}

int R_GetImageHardwareScalingStep( image_t *image, GLenum format )
{
	/* From https://www.khronos.org/opengl/wiki/GLAPI/glTexImage3D

	> If target is GL_PROXY_TEXTURE_3D, no data is read from data,
	> but all of the texture image state is recalculated, checked
	> for consistency, and checked against the implementation's
	> capabilities. If the implementation cannot handle a texture
	> of the requested texture size, it sets all of the image state
	> to 0, but does not generate an error (see glGetError).
	> To query for an entire mipmap array, use an image array level
	> greater than or equal to 1.

	From https://www.khronos.org/opengl/wiki/GLAPI/glTexImage3D

	> To then query this state, call glGetTexLevelParameter. */

	void ( *functionCompressed3D ) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
	void ( *function3D ) (GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
	void ( *functionCompressed2D ) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
	void ( *function2D ) (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);

	GLenum target;

	constexpr GLint border = 0;
	constexpr GLenum type = GL_UNSIGNED_BYTE;

	bool texture3D = false;
	bool textureCompressed = format == GL_NONE;

	int mipSize = R_GetMipSize( image );

	switch ( image->type )
	{
		case GL_TEXTURE_3D:
			texture3D = true;
			functionCompressed3D = glCompressedTexImage3D;
			function3D = glTexImage3D;
			target = GL_PROXY_TEXTURE_3D;
			break;

		case GL_TEXTURE_2D:
			functionCompressed2D = glCompressedTexImage2D;
			function2D = glTexImage2D;
			target = GL_PROXY_TEXTURE_2D;
			break;

		case GL_TEXTURE_CUBE_MAP:
			functionCompressed2D = glCompressedTexImage2D;
			function2D = glTexImage2D;
			target = GL_PROXY_TEXTURE_CUBE_MAP;
			break;

		default:
			ASSERT_UNREACHABLE();
			return 0;
	}

	if ( texture3D )
	{
		if ( textureCompressed )
		{
			functionCompressed3D( target, 0, image->internalFormat, image->uploadWidth, image->uploadHeight, 0, border, mipSize, nullptr );

			GL_CheckErrors();
		}
		else
		{
			function3D( target, 0, image->internalFormat, image->uploadWidth, image->uploadHeight, 0, border, format, type, nullptr );

			GL_CheckErrors();
		}
	}
	else
	{
		if ( textureCompressed )
		{
			functionCompressed2D( target, 0, image->internalFormat, image->uploadWidth, image->uploadHeight, border, mipSize, nullptr );

			GL_CheckErrors();
		}
		else
		{
			function2D( target, 0, image->internalFormat, image->uploadWidth, image->uploadHeight, border, format, type, nullptr );

			GL_CheckErrors();
		}
	}

	GLint finalWidth;
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_WIDTH, &finalWidth );

	if ( finalWidth == 0 )
	{
		image->uploadWidth >>= 1;
		image->uploadHeight >>= 1;

		if ( image->uploadWidth <= 1 || image->uploadHeight <= 1 )
		{
			return 1;
		}

		return 1 + R_GetImageHardwareScalingStep( image, format );
	}

	return 0;
}

/*
===============
R_GetImageCustomScalingStep

Returns amount of downscaling step to be done on the given image
to perform optional picmip or other downscaling operation.

- the r_picMip cvar tells the renderer to downscale textures as many time as this cvar value;
- the imageMaxDimension material keyword tells the renderer to downscale related textures
  to that dimension if natively larger than that dimension;
- the r_imageMaxDimension cvar tells the renderer to downscale all textures to that dimension
  if natively larger than that dimension;
- the r_ignoreMaterialMaxDimension cvar tells the renderer to ignore the imageMaxDimension material
  keyword, r_imageMaxDimension still applies;
- the imageMinDimension material keyword tells the renderer to not downscale related textures
  below that dimension, has priority over r_picMip and r_imageMaxDimension;
- the r_ignoreMaterialMinDemension cvar tells the renderer to ignore the imageMinDimension material
  keyword;
- the r_replaceMaterialMinDimensionIfPresentWithMaxDimension cvar tells the renderer to use the
  maximum texture size for textures having a material keyword setting a minimum dimension;

Examples of scenarii:

- r_picMip set 3 with some materials having imageMinDimension set to 128:
  attempt to reduce three times the images, but for materials having that keyword,
  stop before the image dimension would be smaller than 128;
- r_imageMaxDimension set to 64 with some materials having imageMinDimension set to 128:
  reduce images having dimension greater than 64 to fit 64 dimension, but for materials
  having that keyword, reduce image to fit 128 dimension;
- r_imageMaxDimension set to 64 with some materials having imageMaxDimension set to 32:
  reduce images having dimension greater than 64 to fit 64 dimension, but for materials
  having that keyword, reduce image to fit 32 dimension;
- r_imageMaxDimension set to 64 and r_replaceMaterialMinDimensionIfPresentWithMaxDimension
  enabled: reduce images having dimension greater than 64 to fit 64 dimension, but for
  materials having that keyword, keep the native dimension, or the one set by material
  imageMaxDimension keyword;

Examples of use cases:

- Movie producer wanting to record a movie from a demo using the highest image definition
  possible even if the game developper configured them to be downscaled for performance
  purpose:
    set r_ignoreMaterialMaxDimension on
- Low budget player wanting to reduce all textures to low definition but keep configured
  minimum definition on the textures the game developer configured to not be reduced to a
  given size to keep the game playable:
    set r_imageMaxDimension 64
- Very low budget player wanting to reduce all textures to low definition in all case,
  taking risk stuff may not look as the game developers expect anyway:
    set r_imageMaxDimension 64
    set r_ignoreMaterialMinDimension on
- Competitive player wanting to reduce all textures to one flat color but keep configured
  minimum definition on the textures the game developer configured to not be reduced to a
  given size to keep the game playable:
    set r_imageMaxDimension 1
- Competitive player wanting to reduce all textures to one flat color but keep high definition
  on the textures the game developer configured to not be reduced to keep the game playable:
    ser r_imageMaxDimension 1
    set r_replaceMaterialMinDimensionIfPresentWithMaxDimension on
 
===============
*/
int R_GetImageCustomScalingStep( const image_t *image, const imageParams_t &imageParams )
{
	if ( image->bits & IF_NOPICMIP )
	{
		return 0;
	}

	int materialMinDimension = r_ignoreMaterialMinDimension->integer ? 0 : imageParams.minDimension;
	int materialMaxDimension = r_ignoreMaterialMaxDimension->integer ? 0 : imageParams.maxDimension;

	int minDimension;

	if ( materialMinDimension <= 0 )
	{
		minDimension = 1;
	}
	else if ( r_replaceMaterialMinDimensionIfPresentWithMaxDimension->integer )
	{
		minDimension = materialMaxDimension > 0 ? materialMaxDimension : std::numeric_limits<int>::max();
	}
	else
	{
		minDimension = materialMinDimension;
	}

	int maxDimension = materialMaxDimension > 0 ? materialMaxDimension : std::numeric_limits<int>::max();

	if ( r_imageMaxDimension->integer > 0 )
	{
		maxDimension = std::min( maxDimension, r_imageMaxDimension->integer );
	}

	// Consider the larger edge as the "image dimension"
	int scaledDimension = std::max( image->width, image->height );
	int scalingStep = 0;

	// 1st priority: scaledDimension >= minDimension
	// 2nd priority: scaledDimension <= maxDimension
	// 3rd priority: scalingStep >= r_picMip->integer
	// 4th priority: scalingStep as low as possible
	while ( scaledDimension > maxDimension || scalingStep < r_picMip->integer )
	{
		scaledDimension >>= 1;

		if ( scaledDimension < minDimension )
		{
			break;
		}

		++scalingStep;
	}

	return scalingStep;
}

void R_DownscaleImageDimensions( int scalingStep, int *scaledWidth, int *scaledHeight, const byte ***dataArray, int numLayers, int *numMips )
{
	scalingStep = std::min(scalingStep, *numMips - 1);

	if ( scalingStep > 0 )
	{
		*scaledWidth >>= scalingStep;
		*scaledHeight >>= scalingStep;

		if( *dataArray && *numMips > scalingStep ) {
			*dataArray += numLayers * scalingStep;
			*numMips -= scalingStep;
		}
	}

	// Clamp to minimum size.
	if ( *scaledWidth < 1 )
	{
		*scaledWidth = 1;
	}

	if ( *scaledHeight < 1 )
	{
		*scaledHeight = 1;
	}
}
