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

/* shaderProfiler_fp.glsl */

#if defined(r_profilerRenderSubGroups) && defined(HAVE_KHR_shader_subgroup_basic) && defined(HAVE_KHR_shader_subgroup_arithmetic)
	uniform float u_ProfilerZero;
	uniform uint u_ProfilerRenderSubGroups;
	IN(flat) float var_SubGroupCount;

	#define SHADER_PROFILER_SET( color ) SetSubGroupColor( color );

	void SetSubGroupColor( inout vec4 color ) {
		if( u_ProfilerRenderSubGroups != 0 ) {
			color *= u_ProfilerZero; // Multiply with an 0.0 uniform to stop shader compiler from optmising away the code

			float count = u_ProfilerRenderSubGroups == 2 ? subgroupAdd( 1.0 ) / float( gl_SubgroupSize ) : var_SubGroupCount;

			// Changes the colour to indicate how many active lanes are there in the subgroup
			color += vec4( count < 0.5 ? count + 0.5 : 0.0,
				            count >= 0.5 ? count : 0.0,
							0.0, 1.0 );
		}
	}
#else
	#define SHADER_PROFILER_SET( color )
#endif
