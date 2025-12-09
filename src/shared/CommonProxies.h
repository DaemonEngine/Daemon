/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
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

#ifndef SHARED_COMMON_PROXIES_H_
#define SHARED_COMMON_PROXIES_H_

#include "common/IPC/Channel.h"

namespace Cmd {

    void PushArgs(Str::StringRef args);
    void PopArgs();

}

namespace VM {

    void CrashDump(const uint8_t* data, size_t size);
    void InitializeProxies(int milliseconds);
    void HandleCommonSyscall(int major, int minor, Util::Reader reader, IPC::Channel& channel);

}

void trap_AddCommand(const char* cmdName);
void trap_RemoveCommand(const char* cmdName);
void trap_CompleteCallback(const char* complete);
int trap_Argc();
void trap_Argv(int n, char* buffer, int bufferLength);
const Cmd::Args& trap_Args();
void trap_EscapedArgs(char* buffer, int bufferLength);
void trap_LiteralArgs(char* buffer, int bufferLength);
void trap_Cvar_Set(const char* varName, const char* value);
int trap_Cvar_VariableIntegerValue(const char* varName);
float trap_Cvar_VariableValue(const char* varName);
void trap_Cvar_VariableStringBuffer(const char* varName, char* buffer, int bufsize);
void trap_Cvar_AddFlags(const char* varName, int flags);
int trap_Milliseconds();
void trap_SendConsoleCommand(const char* text);
int trap_FS_FOpenFile(const char* qpath, fileHandle_t* f, fsMode_t mode);
int trap_FS_OpenPakFile(Str::StringRef path, fileHandle_t& f);
int trap_FS_Read(void* buffer, int len, fileHandle_t f);
int trap_FS_Write(const void* buffer, int len, fileHandle_t f);
int trap_FS_Seek(fileHandle_t f, int offset, fsOrigin_t origin);
int trap_FS_Tell(fileHandle_t f);
int trap_FS_FileLength(fileHandle_t f);
void trap_FS_FCloseFile(fileHandle_t f);
int trap_FS_GetFileList(const char* path, const char* extension, char* listbuf, int bufsize);
int trap_FS_GetFileListRecursive(const char* path, const char* extension, char* listbuf, int bufsize);
bool trap_FindPak(const char* name);
bool trap_FS_LoadPak(const char* pak, const char* prefix);

#endif // SHARED_COMMON_PROXIES_H_
