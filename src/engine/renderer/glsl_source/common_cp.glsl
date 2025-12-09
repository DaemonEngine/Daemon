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

/* common_cp.glsl */

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

/* Macro combinations for subgroup ops */

#if defined(HAVE_KHR_shader_subgroup_basic) && defined(HAVE_KHR_shader_subgroup_arithmetic)\
	&& defined(HAVE_KHR_shader_subgroup_ballot) && defined(HAVE_ARB_shader_atomic_counter_ops)
	#define SUBGROUP_STREAM_COMPACTION
#endif
