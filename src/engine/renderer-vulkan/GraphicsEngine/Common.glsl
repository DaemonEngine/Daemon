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

/* Common.glsl */

#extension GL_EXT_shader_explicit_arithmetic_types : require

#extension GL_EXT_buffer_reference2   : require
#extension GL_EXT_scalar_block_layout : require

#extension GL_EXT_nonuniform_qualifier : require

#extension GL_KHR_shader_subgroup_basic            : require
#extension GL_KHR_shader_subgroup_vote             : require
#extension GL_KHR_shader_subgroup_arithmetic       : require
#extension GL_KHR_shader_subgroup_ballot           : require
#extension GL_KHR_shader_subgroup_shuffle          : require
#extension GL_KHR_shader_subgroup_shuffle_relative : require
#extension GL_KHR_shader_subgroup_clustered        : require
#extension GL_KHR_shader_subgroup_quad             : require
#extension GL_KHR_shader_subgroup_rotate           : require

// #extension GL_KHR_subgroupuniform_qualifier : require

// #extension GL_EXT_shader_quad_control : require

// #extension GL_KHR_control_flow_attributes  : require
// #extension GL_KHR_control_flow_attributes2 : require

/*
#extension GL_EXT_shader_subgroup_extended_types_int8    : require
#extension GL_EXT_shader_subgroup_extended_types_int16   : require
#extension GL_EXT_shader_subgroup_extended_types_int64   : require
#extension GL_EXT_shader_subgroup_extended_types_float16 : require
*/

/*
#extension GL_KHR_spirv_intrinsics : require
#extension GL_KHR_nontemporal_keyword : require
#extension GL_KHR_demote_to_helper_invocation : require
#extension GL_KHR_integer_dot_product : require
#extension GL_KHR_shader_atomic_float : require
#extension GL_KHR_shader_realtime_clock : require
#extension GL_KHR_memory_scope_semantics : require

#extension GL_KHR_samplerless_texture_functions : require
*/

// #define constexpr const

#include "NumberTypes.h"

/* Common defines */

/* Allows accessing each element of a uvec4 array with a singular ID
Useful to avoid wasting memory due to alignment requirements
array must be in the form of uvec4 array[] */

#define UINT_FROM_UVEC4_ARRAY( array, id ) ( array[id / 4][id % 4] )
#define UVEC2_FROM_UVEC4_ARRAY( array, id ) ( id % 2 == 0 ? array[id / 2].xy : array[id / 2].zw )

// Scalar global workgroup ID
#define GLOBAL_GROUP_ID ( gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y\
                        + gl_WorkGroupID.y * gl_NumWorkGroups.x\
                        + gl_WorkGroupID.x )
						
// Scalar global invocation ID
#define GLOBAL_INVOCATION_ID ( gl_GlobalInvocationID.z * gl_NumWorkGroups.x * gl_WorkGroupSize.x\
                             * gl_NumWorkGroups.y * gl_WorkGroupSize.y\
                             + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x\
                             + gl_GlobalInvocationID.x )

#define sizeof( Type ) ( uint64( Type( uint64( 0 ) ) + 1 ) )

#define Buffer( alignment ) \
layout ( scalar, buffer_reference, buffer_reference_align = alignment ) buffer

#define BufferR( alignment ) \
layout ( scalar, buffer_reference, buffer_reference_align = alignment ) readonly buffer

#define BufferW( alignment ) \
layout ( scalar, buffer_reference, buffer_reference_align = alignment ) writeonly buffer

#define BufferS \
layout ( scalar, buffer_reference, buffer_reference_align = 4 ) buffer

#define BufferRS \
layout ( scalar, buffer_reference, buffer_reference_align = 4 ) readonly buffer

#define BufferWS \
layout ( scalar, buffer_reference, buffer_reference_align = 4 ) writeonly buffer