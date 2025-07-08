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
// MiscCVarStore.cpp

#include "SrcDebug/LogExtend.h"
#include "SrcDebug/Tag.h"

Cvar::Cvar<bool> r_vkLogExtend( "r_vkLogExtend", "Print src location in all logs", Cvar::NONE, false );

Cvar::Cvar<bool> r_vkLogExtendWarn( "r_vkLogExtendWarn", "Print src location in warn logs", Cvar::NONE, false );
Cvar::Cvar<bool> r_vkLogExtendNotice( "r_vkLogExtendNotice", "Print src location in notice logs", Cvar::NONE, false );
Cvar::Cvar<bool> r_vkLogExtendVerbose( "r_vkLogExtendVerbose", "Print src location in verbose logs", Cvar::NONE, false );
Cvar::Cvar<bool> r_vkLogExtendDebug( "r_vkLogExtendDebug", "Print src location in debug logs", Cvar::NONE, false );

Cvar::Range<Cvar::Cvar<int>> r_vkLogExtendedFunctionNames( "r_vkLogExtendedFunctionNames",
	"Extended log function format: 0 - only name, 1 - name + template specialisation, 2 - all", Cvar::NONE,
	LogExtendedFunctionMode::NAME, LogExtendedFunctionMode::NAME, LogExtendedFunctionMode::FULL );