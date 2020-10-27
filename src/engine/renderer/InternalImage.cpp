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

int R_GetImageCustomScalingStep( const image_t *image, imageParams_t *imageParams )
{
	int scalingStep = 0;

	// Perform optional picmip operation.
	if ( !( image->bits & IF_NOPICMIP ) )
	{
		int imageMinDimension = std::max( -1, r_imageMinDimension->integer );
		int imageMaxDimension = std::max( 0, r_imageMaxDimension->integer );
		int shaderMinDimension = 0;
		int shaderMaxDimension = 0;

		if ( imageParams != nullptr )
		{
			shaderMinDimension = std::max( 0, imageParams->minDimension );
			shaderMaxDimension = std::max( 0, imageParams->maxDimension );
		}

		// If shader has imageMaxDimension value set, enforce downscaling using this value.
		if ( shaderMaxDimension != 0 )
		{
			// Except if r_imageMaxDimension is set and smaller.
			if ( imageMaxDimension == 0 || imageMaxDimension >= shaderMaxDimension )
			{
				imageMaxDimension = shaderMaxDimension;
			}
		}

		// If r_imageMinDimension == -1 and shader has imageMinDimension value set, keep original size.
		if ( imageMinDimension == -1 && shaderMinDimension != 0 )
		{
			imageMaxDimension = 0;
		}

		if ( imageMaxDimension != 0 )
		{
			// If image has imageMinDimension value set and r_imageMinDimension > 0,
			// use greater value between imageMinDimension and imageMaxDimension.
			if ( shaderMinDimension > 0 && imageMinDimension > 0 )
			{
				imageMaxDimension = std::max( imageMinDimension, imageMaxDimension );
			}

			// Do not downscale this image below imageMinDimension size.
			if ( imageMaxDimension < shaderMinDimension )
			{
				imageMaxDimension = shaderMinDimension;
			}

			// Downscale image to imageMaxDimension size.
			int scaledWidth = image->width;
			int scaledHeight = image->height;

			while ( imageMaxDimension < scaledWidth || imageMaxDimension < scaledHeight )
			{
				scaledWidth >>= 1;
				scaledHeight >>= 1;
				scalingStep++;
			}
		}

		scalingStep = std::max( scalingStep, std::max( 0, r_picmip->integer ) );
	}

	return scalingStep;
}

void R_DownscaleImageDimensions( int scalingStep, int *scaledWidth, int *scaledHeight, const byte ***dataArray, int numLayers, int *numMips )
{
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
