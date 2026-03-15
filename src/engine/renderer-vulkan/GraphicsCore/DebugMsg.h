/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2026 Daemon Developers
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
// DebugMsg.h

#ifndef DEBUG_MSG_H
#define DEBUG_MSG_H

#include "../Math/NumberTypes.h"

enum DebugMsgSeverity : uint32 {
	DEBUG_MSG_VERBOSE      = 1,
	DEBUG_MSG_INFO         = 2,
	DEBUG_MSG_WARNING      = 4,
	DEBUG_MSG_ERROR        = 8,
	DEBUG_MSG_SEVERITY_ALL = DEBUG_MSG_VERBOSE | DEBUG_MSG_INFO | DEBUG_MSG_WARNING | DEBUG_MSG_ERROR
};

enum DebugMsgType : uint32 {
	DEBUG_MSG_GENERAL     = 1,
	DEBUG_MSG_VALIDATION  = 2,
	DEBUG_MSG_PERFORMANCE = 4,
	DEBUG_MSG_TYPE_ALL    = DEBUG_MSG_GENERAL | DEBUG_MSG_VALIDATION | DEBUG_MSG_PERFORMANCE
};

void InitDebugMsg();

#endif // DEBUG_MSG_H