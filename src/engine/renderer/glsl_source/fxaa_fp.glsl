/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2013 Daemon Developers
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

===========================================================================
*/

/* fxaa_fp.glsl */

// Control knobs.
#if __VERSION__ == 120
#define FXAA_GLSL_120 1
#else
#define FXAA_GLSL_130 1
#endif

#define FXAA_PC 1
#define FXAA_QUALITY_PRESET 12
#define FXAA_GREEN_AS_LUMA 0

#insert fxaa3_11_fp

#if defined(HAVE_ARB_bindless_texture)
uniform sampler2D u_ColorMap_linear;
#define u_ColorMap u_ColorMap_linear
#else
uniform sampler2D	u_ColorMap;
#endif

#if __VERSION__ > 120
out vec4 outputColor;
#else
#define outputColor gl_FragColor
#endif

void	main()
{
	vec4 color = FxaaPixelShader(
		gl_FragCoord.xy / r_FBufSize, //pos
		vec4(0.0), //not used
		u_ColorMap, //tex
		u_ColorMap, //not used
		u_ColorMap, //not used
		1 / r_FBufSize, //fxaaQualityRcpFrame
		vec4(0.0), //not used
		vec4(0.0), //not used
		vec4(0.0), //not used
		r_FXAASubPix, //fxaaQualitySubpix
		r_FXAAEdgeThreshold, //fxaaQualityEdgeThreshold
		r_FXAAEdgeThresholdMin, //fxaaQualityEdgeThresholdMin
		0.0, //not used
		0.0, //not used
		0.0, //not used
		vec4(0.0) //not used
	);

	#if defined(r_showFXAA)
	{
		vec4 originalColor = FxaaTexTop( u_ColorMap, gl_FragCoord.xy / r_FBufSize );

		if ( color.r != originalColor.r
			|| color.g != originalColor.g
			|| color.b != originalColor.b )
		{
			color.rgb = vec3(1.0, 0.0, 0.0);
		}
	}
	#endif

	outputColor = vec4( color.rgb, 1.0f );
}
