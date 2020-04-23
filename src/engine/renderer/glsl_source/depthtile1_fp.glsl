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

/* depthtile1_fp.glsl */

uniform sampler2D u_DepthMap;
IN(flat) vec3 unprojectionParams;

uniform vec3 u_zFar;

const vec2 pixelScale = 1 / r_FBufSize;

DECLARE_OUTPUT(vec4)

vec4 depthToZ(in vec4 depth) {
  return unprojectionParams.x / ( unprojectionParams.y * depth - unprojectionParams.z );
}

float max16(in vec4 data0, in vec4 data1, in vec4 data2, in vec4 data3) {
  vec4 max01 = max(data0, data1);
  vec4 max23 = max(data2, data3);
  vec4 max4 = max(max01, max23);
  vec2 max2 = max(max4.xy, max4.zw);
  return max(max2.x, max2.y);
}

float min16(in vec4 data0, in vec4 data1, in vec4 data2, in vec4 data3)
{
  vec4 min01 = min(data0, data1);
  vec4 min23 = min(data2, data3);
  vec4 min4 = min(min01, min23);
  vec2 min2 = min(min4.xy, min4.zw);
  return min(min2.x, min2.y);
}
void	main()
{
  vec2 st = gl_FragCoord.st * 4.0 * pixelScale;
  vec4 depth[4], mask[4];

#ifdef HAVE_ARB_texture_gather
  depth[0] = textureGather( u_DepthMap, st + vec2(-1.0, -1.0) * pixelScale );
  depth[1] = textureGather( u_DepthMap, st + vec2(-1.0,  1.0) * pixelScale );
  depth[2] = textureGather( u_DepthMap, st + vec2( 1.0,  1.0) * pixelScale );
  depth[3] = textureGather( u_DepthMap, st + vec2( 1.0, -1.0) * pixelScale );
#else
  depth[0] = vec4(texture2D( u_DepthMap, st + vec2(-1.5, -1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-1.5, -0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-0.5, -0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-0.5, -1.5) * pixelScale ).x);
  depth[1] = vec4(texture2D( u_DepthMap, st + vec2( 0.5, -1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 0.5, -0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 1.5, -0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 1.5, -1.5) * pixelScale ).x);
  depth[2] = vec4(texture2D( u_DepthMap, st + vec2( 0.5,  0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 0.5,  1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 1.5,  1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 1.5,  0.5) * pixelScale ).x);
  depth[3] = vec4(texture2D( u_DepthMap, st + vec2(-1.5,  0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-1.5,  1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-0.5,  1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-0.5,  0.5) * pixelScale ).x);
#endif

 // mask out sky pixels (z buffer depth == 1.0) to help with depth discontinuities
  mask[0] = 1.0 - step(1.0, depth[0]);
  mask[1] = 1.0 - step(1.0, depth[1]);
  mask[2] = 1.0 - step(1.0, depth[2]);
  mask[3] = 1.0 - step(1.0, depth[3]);

  float samples = dot( mask[0] + mask[1] + mask[2] + mask[3], vec4(1.0) );
  if ( samples > 0.0 ) {
    samples = 1.0 / samples;

    depth[0] = depthToZ(depth[0]);
    depth[1] = depthToZ(depth[1]);
    depth[2] = depthToZ(depth[2]);
    depth[3] = depthToZ(depth[3]);

    // don't need to mask out sky depth for min operation (because sky has a larger depth than everything else)
    float minDepth = min16( depth[0], depth[1], depth[2], depth[3] );

    depth[0] *= mask[0];
    depth[1] *= mask[1];
    depth[2] *= mask[2];
    depth[3] *= mask[3];

    float maxDepth = max16( depth[0], depth[1], depth[2], depth[3] );

    float avgDepth = dot( depth[0] + depth[1] + depth[2] + depth[3],
			  vec4( samples ) );

    depth[0] -= avgDepth * mask[0];
    depth[1] -= avgDepth * mask[1];
    depth[2] -= avgDepth * mask[2];
    depth[3] -= avgDepth * mask[3];

    float variance = dot( depth[0], depth[0] ) + dot( depth[1], depth[1] ) +
      dot( depth[2], depth[2] ) + dot( depth[3], depth[3] );
    variance *= samples;
    outputColor = vec4( maxDepth, minDepth, avgDepth, sqrt( variance ) );
  } else {
    // found just sky pixels
    outputColor = vec4( 99999.0, 99999.0, 99999.0, 0.0 );
  }
}
