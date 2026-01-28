/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2025 Daemon Developers
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
// ResultCheck.h

#ifndef RESULT_CHECK_H
#define RESULT_CHECK_H

#include "../Error.h"

#include "Vulkan.h"

constexpr int resultSuccess = VK_SUCCESS | VK_NOT_READY | VK_TIMEOUT | VK_EVENT_SET | VK_EVENT_RESET | VK_INCOMPLETE
	| VK_SUBOPTIMAL_KHR | VK_THREAD_IDLE_KHR | VK_THREAD_DONE_KHR | VK_OPERATION_DEFERRED_KHR | VK_OPERATION_NOT_DEFERRED_KHR
	| VK_PIPELINE_COMPILE_REQUIRED | VK_PIPELINE_BINARY_MISSING_KHR | VK_INCOMPATIBLE_SHADER_BINARY_EXT;

/* struct ResultCheck {
	ResultCheck( const VkResult skipRes, const VkResult res );
	ResultCheck& operator=( const VkResult res );
};

int ResCheck( const VkResult skipRes, const VkResult res );

#define ResCheckExt( skipRes, res ) \
if ( !( res & ( resultSuccess | skipRes ) ) ) { \
	Err( "Vulkan function failed: %s (%s:%u)", string_VkResult( res ), __FILE__, __LINE__ ); \
	return; \
} */

#define ResultCheckExt( res, skipRes ) \
resultCheck = res; \
if ( resultCheck && !( resultCheck & ( resultSuccess ^ skipRes ) ) ) { \
	Err( "Vulkan function failed: %s (%s:%u)", string_VkResult( res ), __FILE__, __LINE__ ); \
	return; \
}

#define ResultCheckExtRet( res, skipRes ) \
resultCheck = res; \
if ( resultCheck && !( resultCheck & ( resultSuccess ^ skipRes ) ) ) { \
	Err( "Vulkan function failed: %s (%s:%u)", string_VkResult( res ), __FILE__, __LINE__ ); \
	return 0; \
}

#define ResultCheck( res )    ResultCheckExt( res, 0 )

#define ResultCheckRet( res ) ResultCheckExtRet( res, 0 )

struct ResCheck {
	ResCheck();

	void operator=( const VkResult result );
};

// Here so it's not a "hidden" local variable
extern thread_local VkResult resultCheck;
extern thread_local ResCheck resCheck;

#endif // RESULT_CHECK_H