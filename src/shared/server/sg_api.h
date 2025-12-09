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

#ifndef SHARED_SERVER_API_H_
#define SHARED_SERVER_API_H_

#include <common/IPC/Primitives.h>

extern IPC::SharedMemory shmRegion;

void             trap_LocateGameData( int numGEntities, int sizeofGEntity_t, int sizeofGClient );
void             trap_DropClient( int clientNum, const char *reason );
void             trap_SendServerCommand( int clientNum, const char *text );
void             trap_SetConfigstring( int num, const char *string );
void             trap_SetConfigstringRestrictions( int num, const clientList_t *clientList );
void             trap_GetConfigstring( int num, char *buffer, int bufferSize );
void             trap_SetUserinfo( int num, const char *buffer );
void             trap_GetUserinfo( int num, char *buffer, int bufferSize );
void             trap_GetServerinfo( char *buffer, int bufferSize );
int              trap_BotAllocateClient();
void             trap_BotFreeClient( int clientNum );
void             trap_GetUsercmd( int clientNum, usercmd_t *cmd );
bool         trap_GetEntityToken( char *buffer, int bufferSize );
int              trap_BotGetServerCommand( int clientNum, char *message, int size );
int              trap_RSA_GenerateMessage( const char *public_key, char *cleartext, char *encrypted );
void             trap_GenFingerprint( const char *pubkey, int size, char *buffer, int bufsize );
void             trap_GetPlayerPubkey( int clientNum, char *pubkey, int size );
void             trap_GetTimeString( char *buffer, int size, const char *format, const qtime_t *tm );

#endif
