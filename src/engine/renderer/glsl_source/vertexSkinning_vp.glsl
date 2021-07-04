/*
===========================================================================
Copyright (C) 2009-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// vertexSkinning_vp.glsl - GPU vertex skinning for skeletal meshes

#if defined(USE_VERTEX_SKINNING)

IN vec3 attr_Position;
IN vec2 attr_TexCoord0;
IN vec4 attr_Color;
IN vec4 attr_QTangent;
IN vec4 attr_BoneFactors;

// even elements are rotation quat, odd elements are translation + scale (in .w)
uniform vec4 u_Bones[ 2 * MAX_GLSL_BONES ];

void VertexFetch(out vec4 position,
		 out localBasis LB,
		 out vec4 color,
		 out vec2 texCoord,
		 out vec2 lmCoord)
{
	const float scale = 1.0 / 256.0;
	const float weightScale = 1.0 / 255.0;
	localBasis inLB;

	// Unpack data from "bone factors". This used to have the index in the high byte and the weight
	// in the low byte, which may seem a bit more logical, but it triggered issues with some
	// Nvidia shader compilers (https://github.com/DaemonEngine/Daemon/issues/472).
	vec4 ipart = floor( attr_BoneFactors * scale );
	vec4 fpart = attr_BoneFactors * scale - ipart;
	// idx = 2 times the original bone index (the index input to boneFactor)
	ivec4 idx = ivec4( fpart * 512.0 );
	vec4 weights = ipart * weightScale;

	vec4 quat = u_Bones[ idx.x ];
	vec4 trans = u_Bones[ idx.x + 1 ];

	QTangentToLocalBasis( attr_QTangent, inLB );

	position.xyz = weights.x * (QuatTransVec( quat, attr_Position ) * trans.w + trans.xyz);
	LB.normal = weights.x * (QuatTransVec( quat, inLB.normal ));
	LB.tangent = weights.x * (QuatTransVec( quat, inLB.tangent ));
	LB.binormal = weights.x * (QuatTransVec( quat, inLB.binormal ));
	
	quat = u_Bones[ idx.y ];
	trans = u_Bones[ idx.y + 1 ];

	position.xyz += weights.y * (QuatTransVec( quat, attr_Position ) * trans.w + trans.xyz);
	LB.normal += weights.y * (QuatTransVec( quat, inLB.normal ));
	LB.tangent += weights.y * (QuatTransVec( quat, inLB.tangent ));
	LB.binormal += weights.y * (QuatTransVec( quat, inLB.binormal ));

	quat = u_Bones[ idx.z ];
	trans = u_Bones[ idx.z + 1 ];

	position.xyz += weights.z * (QuatTransVec( quat, attr_Position ) * trans.w + trans.xyz);
	LB.normal += weights.z * (QuatTransVec( quat, inLB.normal ));
	LB.tangent += weights.z * (QuatTransVec( quat, inLB.tangent ));
	LB.binormal += weights.z * (QuatTransVec( quat, inLB.binormal ));

	quat = u_Bones[ idx.w ];
	trans = u_Bones[ idx.w + 1 ];

	position.xyz += weights.w * (QuatTransVec( quat, attr_Position ) * trans.w + trans.xyz);
	LB.normal += weights.w * (QuatTransVec( quat, inLB.normal ));
	LB.tangent += weights.w * (QuatTransVec( quat, inLB.tangent ));
	LB.binormal += weights.w * (QuatTransVec( quat, inLB.binormal ));

	position.w = 1.0;
	LB.normal   = normalize(LB.normal);
	LB.tangent  = normalize(LB.tangent);
	LB.binormal = normalize(LB.binormal);

	color    = attr_Color;
	texCoord = attr_TexCoord0;
	lmCoord  = attr_TexCoord0;
}
#endif
