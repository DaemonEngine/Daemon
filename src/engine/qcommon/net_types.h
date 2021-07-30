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

#ifndef ENGINE_QCOMMON_NET_TYPES_H_
#define ENGINE_QCOMMON_NET_TYPES_H_

#include "./q_shared.h"

enum class netadrtype_t : int
{
    NA_BOT,
    NA_BAD, // an address lookup failed
    NA_LOOPBACK,
    NA_BROADCAST,
    NA_IP,
    NA_IP6,
    NA_IP_DUAL,
    NA_MULTICAST6,
    NA_UNSPEC
};

enum class netsrc_t
{
    NS_CLIENT,
    NS_SERVER
};

struct netadr_t
{
    netadrtype_t   type;

    byte           ip[ 4 ];
    byte           ip6[ 16 ];

    unsigned short port; // port which is in use
    unsigned short port4, port6; // ports to choose from
    uint32_t       scope_id; // Needed for IPv6 link-local addresses
};

struct msg_t
{
    bool overflowed; // set to true if the buffer size failed
    bool oob;
    byte     *data;
    int      maxsize;
    int      cursize;
    int      uncompsize; // NERVE - SMF - net debugging
    int      readcount;
    int      bit; // for bitwise reads and writes
};

#endif // ENGINE_QCOMMON_NET_TYPES_H_
