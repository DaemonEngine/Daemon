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
#include "GLUtils.h"

// Comments may include short quotes from other authors.

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

	// Consider the larger edge as the "image dimension"
	int scaledDimension = std::max( image->width, image->height );

	int scalingStep = 0;

	// Scale down the image size according to the screen size.
	if ( image->bits & IF_FITSCREEN )
	{
		int largerSide = std::max( glConfig.vidWidth, glConfig.vidHeight );

		if ( scaledDimension > largerSide )
		{
			while ( scaledDimension > largerSide )
			{
				scaledDimension >>= 1;
				scalingStep++;
			}

			/* With r_imageFitScreen == 1, we need the larger image size before
			it becomes smaller than screen.

			With r_imageFitScreen == 2 the image is never larger than screen, as
			we allow the larger size that is not larger than screen, it can be the
			larger size smaller than screen. */
			if ( scaledDimension != largerSide && r_imageFitScreen.Get() != 2 )
			{
				scaledDimension <<= 1;
				scalingStep--;
			}
		}

		return scalingStep;
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
