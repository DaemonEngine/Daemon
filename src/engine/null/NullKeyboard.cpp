/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2017 Daemon Developers
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

// Keyboard-based controls (and in-game console) are not needed for tty client or dedicated server.

#include "engine/client/keys.h"

using Keyboard::Key;

const Key Key::NONE;
const Key Key::CONSOLE;

Key Key::FromScancode(int) {
    return Key();
}

Key Key_StringToKeynum( const char * ) {
    return Key();
}

const char *Key_KeynumToString( Key ) {
    return "<unimplemented>";
}

void Key_KeynameCompletion(Cmd::CompletionResult& completions, Str::StringRef prefix) {
}

void Field_Draw(const Util::LineEditData&, int, int, bool, bool, float) {
}

void Field_KeyDownEvent(Util::LineEditData&, Keyboard::Key) {
}

void Field_CharEvent(Util::LineEditData&, int) {
}

bool Key_IsDown(Key) {
    return false;
}

int Key_GetTeam( const char *, const char * ) {
    return 0;
}

void Key_SetBinding( Key, int, const char * ) {
}

const char *Key_GetBinding( Key, int ) {
    return nullptr;
}

void Key_WriteBindings( fileHandle_t ) {
}

Key Key_GetKeyNumber() {
    return Key();
}

unsigned int Key_GetKeyTime() {
    return 0;
}

void CL_InitKeyCommands() {
}

void CL_ClearKeyBinding() {
}

void CL_KeyEvent( const Key&, bool, unsigned ) {
}

void CL_CharEvent( int ) {
}

void Key_ClearStates() {
}

void Key_SetTeam( int ) {
}

bool AnyKeyDown() {
    return false;
}
