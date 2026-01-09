/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2017, Daemon Developers
All rights reserved.

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
#ifndef ENGINE_CLIENT_KEY_IDENTIFICATION_H_
#define ENGINE_CLIENT_KEY_IDENTIFICATION_H_

#include "common/KeyIdentification.h"
#include "common/String.h"
#include "engine/framework/CommandSystem.h"


namespace Keyboard {

Key StringToKey(Str::StringRef name);
std::string KeyToString(Key key);
std::string KeyToStringUnprefixed(Key key);

// Returns the code point of a character typed by the given key, or 0 if none is found.
int GetCharForScancode(int scancode);

// Returns the character corresponding to scancode sc in the QWERTY layout, if any, otherwise 0.
char ScancodeToAscii(int sc);

void CompleteKeyName(Cmd::CompletionResult& completions, Str::StringRef prefix);

} // namespace Keyboard

#endif // ENGINE_CLIENT_KEY_IDENTIFICATION_H_
