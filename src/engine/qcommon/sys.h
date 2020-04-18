/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#ifndef ENGINE_QCOMMON_SYS_H_
#define ENGINE_QCOMMON_SYS_H_

#include "engine/qcommon/net_types.h"

void Sys_SendPacket(int length, const void *data, const netadr_t& to);
bool Sys_GetPacket(netadr_t *net_from, msg_t *net_message);

bool Sys_StringToAdr(const char *s, netadr_t *a, netadrtype_t family);

bool Sys_IsLANAddress(const netadr_t& adr);
void Sys_ShowIP();

#define Sys_Milliseconds Sys::Milliseconds

#endif // ENGINE_QCOMMON_SYS_H_
