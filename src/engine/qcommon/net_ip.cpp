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

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include <common/FileSystem.h>
#include "engine/framework/Application.h"
#include "engine/framework/Network.h"
#include "server/server.h"

#ifdef _WIN32
#       include <winsock2.h>
#       include <ws2tcpip.h>
#       if WINVER < 0x501
#               ifdef __MINGW32__
// wspiapi.h isn't available on MinGW, so if it's
// present it's because the end user has added it
// and we should look for it in our tree
#                       include "wspiapi.h"
#               else
#                       include <wspiapi.h>
#               endif
#       else
#               include <ws2spi.h>
#       endif

using socklen_t = int;
#       ifdef ADDRESS_FAMILY
#               define sa_family_t ADDRESS_FAMILY
#       else
using sa_family_t = unsigned short;
#       endif

#       define socketError   WSAGetLastError()

namespace net {
	namespace errc {
		constexpr auto resource_unavailable_try_again = WSAEWOULDBLOCK;
		constexpr auto address_not_available = WSAEADDRNOTAVAIL;
		constexpr auto address_family_not_supported = WSAEAFNOSUPPORT;
		constexpr auto connection_reset = WSAECONNRESET;
	}  // namespace errc
}  // namespace net

static WSADATA  winsockdata;
static bool winsockInitialized = false;

#else

#       if MAC_OS_X_VERSION_MIN_REQUIRED == 1020
// needed for socklen_t on OSX 10.2
#               define _BSD_SOCKLEN_T_
#       endif

#       include <arpa/inet.h>
#       include <netdb.h>
#       include <netinet/in.h>
#       include <sys/socket.h>
#       include <net/if.h>
#       include <sys/ioctl.h>
#       include <sys/types.h>
#       include <sys/time.h>
#       include <unistd.h>
#       if !defined( __sun ) && !defined( __sgi )
#               include <ifaddrs.h>
#       endif

#       ifdef __sun
#               include <sys/filio.h>
#       endif

using SOCKET = int;
constexpr SOCKET INVALID_SOCKET{-1};
constexpr SOCKET SOCKET_ERROR{-1};

#       define closesocket    close
#       define ioctlsocket    ioctl
#       define socketError    errno

namespace net {
	namespace errc {
		constexpr auto resource_unavailable_try_again = EWOULDBLOCK;
		constexpr auto address_not_available = EADDRNOTAVAIL;
		constexpr auto address_family_not_supported = EAFNOSUPPORT;
		constexpr auto connection_reset = ECONNRESET;
	}  // namespace errc
}  // namespace net

#endif

static bool            usingSocks = false;
static bool            networkingEnabled = false;

cvar_t                     *net_enabled;

static cvar_t              *net_socksEnabled;
static cvar_t              *net_socksServer;
static cvar_t              *net_socksPort;
static cvar_t              *net_socksUsername;
static cvar_t              *net_socksPassword;

static cvar_t              *net_ip;
static cvar_t              *net_ip6;
static cvar_t              *net_port;
static cvar_t              *net_port6;
static cvar_t              *net_mcast6addr;
static cvar_t              *net_mcast6iface;

static struct sockaddr     socksRelayAddr;

static SOCKET              ip_socket = INVALID_SOCKET;
static SOCKET              ip6_socket = INVALID_SOCKET;
static SOCKET              socks_socket = INVALID_SOCKET;
static SOCKET              multicast6_socket = INVALID_SOCKET;

// Keep track of currently joined multicast group.
static struct ipv6_mreq    curgroup;

// And the currently bound address.
static struct sockaddr_in6 boundto;

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

// use an admin local address per default so that network admins can decide on how to handle quake3 traffic.
#define NET_MULTICAST_IP6 "ff04::696f:7175:616b:6533"

static const int MAX_IPS = 32;

struct nip_localaddr_t
{
	char                    ifname[ IF_NAMESIZE ];

	netadrtype_t            type;
	sa_family_t             family;
	struct sockaddr_storage addr;

	struct sockaddr_storage netmask;
};

// Used for Sys_IsLANAddress
static nip_localaddr_t localIP[ MAX_IPS ];
static int             numIP;

//=============================================================================

/*
====================
NET_ErrorString
====================
*/
static std::string NET_ErrorString()
{
	const auto errorCode = socketError;

	if (errorCode == 0)
	{
		return "NO ERROR";
	}

#ifdef _WIN32
	return Sys::Win32StrError(errorCode);
#else
	return strerror(errorCode);
#endif
}

void NetadrToSockadr( const netadr_t *a, struct sockaddr *s )
{
	memset( s, 0, sizeof( struct sockaddr ) );
	if ( a->type == netadrtype_t::NA_BROADCAST )
	{
		( ( struct sockaddr_in * ) s )->sin_family = AF_INET;
		( ( struct sockaddr_in * ) s )->sin_port = a->port;
		( ( struct sockaddr_in * ) s )->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if ( a->type == netadrtype_t::NA_IP )
	{
		( ( struct sockaddr_in * ) s )->sin_family = AF_INET;
		( ( struct sockaddr_in * ) s )->sin_addr.s_addr = * ( int * ) &a->ip;
		( ( struct sockaddr_in * ) s )->sin_port = ( a->type == netadrtype_t::NA_IP_DUAL ? a->port4 : a->port );
	}
	else if ( NET_IS_IPv6( a->type ) )
	{
		( ( struct sockaddr_in6 * ) s )->sin6_family = AF_INET6;
		( ( struct sockaddr_in6 * ) s )->sin6_addr = * ( ( struct in6_addr * ) &a->ip6 );
		( ( struct sockaddr_in6 * ) s )->sin6_port = ( a->type == netadrtype_t::NA_IP_DUAL ? a->port6 : a->port );
		( ( struct sockaddr_in6 * ) s )->sin6_scope_id = a->scope_id;
	}
	else if ( a->type == netadrtype_t::NA_MULTICAST6 )
	{
		( ( struct sockaddr_in6 * ) s )->sin6_family = AF_INET6;
		( ( struct sockaddr_in6 * ) s )->sin6_addr = curgroup.ipv6mr_multiaddr;
		( ( struct sockaddr_in6 * ) s )->sin6_port = a->port;
	}
}

static void SockadrToNetadr( struct sockaddr *s, netadr_t *a )
{
	if ( s->sa_family == AF_INET )
	{
		a->type = netadrtype_t::NA_IP;
		* ( int * ) &a->ip = ( ( struct sockaddr_in * ) s )->sin_addr.s_addr;
		a->port = ( ( struct sockaddr_in * ) s )->sin_port;
	}
	else if ( s->sa_family == AF_INET6 )
	{
		a->type = netadrtype_t::NA_IP6;
		memcpy( a->ip6, & ( ( struct sockaddr_in6 * ) s )->sin6_addr, sizeof( a->ip6 ) );
		a->port = ( ( struct sockaddr_in6 * ) s )->sin6_port;
		a->scope_id = ( ( struct sockaddr_in6 * ) s )->sin6_scope_id;
	}
}

static struct addrinfo *SearchAddrInfo( struct addrinfo *hints, sa_family_t family )
{
	while ( hints )
	{
		if ( hints->ai_family == family )
		{
			return hints;
		}

		hints = hints->ai_next;
	}

	return nullptr;
}

/*
=============
Sys_StringToSockaddr

Must be thread safe - DNS thread calls via NET_StringToAdr
=============
*/
static bool Sys_StringToSockaddr( const char *s, struct sockaddr *sadr, unsigned sadr_len, sa_family_t family )
{
	struct addrinfo hints;

	struct addrinfo *res = nullptr;
	struct addrinfo *search = nullptr;
	struct addrinfo *hintsp;

	int             retval;

	memset( sadr, '\0', sizeof( *sadr ) );
	memset( &hints, '\0', sizeof( hints ) );

	hintsp = &hints;
	hintsp->ai_family = family; // FIXME: all protocols are queried with AF_UNSPEC even if some are disabled by net_enabled
	hintsp->ai_socktype = SOCK_DGRAM;

	retval = getaddrinfo( s, nullptr, hintsp, &res );
	int netEnabled = net_enabled->integer;

	if ( !retval )
	{
		if ( family == AF_UNSPEC )
		{
			// Decide here and now which protocol family to use
			if ( netEnabled & NET_PRIOV6 )
			{
				if ( netEnabled & NET_ENABLEV6 )
				{
					search = SearchAddrInfo( res, AF_INET6 );
				}

				if ( !search && ( netEnabled & NET_ENABLEV4 ) )
				{
					search = SearchAddrInfo( res, AF_INET );
				}
			}
			else
			{
				if ( netEnabled & NET_ENABLEV4 )
				{
					search = SearchAddrInfo( res, AF_INET );
				}

				if ( !search && ( netEnabled & NET_ENABLEV6 ) )
				{
					search = SearchAddrInfo( res, AF_INET6 );
				}
			}
		}
		else
		{
			search = SearchAddrInfo( res, family );
		}

		if ( search )
		{
			if ( search->ai_addrlen > sadr_len )
			{
				search->ai_addrlen = sadr_len;
			}

			memcpy( sadr, search->ai_addr, search->ai_addrlen );
			freeaddrinfo( res );

			return true;
		}
		else
		{
			Log::Notice( "Sys_StringToSockaddr: Error resolving %s: No address of required type found.", s );
		}
	}
	else
	{
		// Don't warn if both protocols are enabled but we looked up only a specific one
		bool skipWarn = family != AF_UNSPEC && ( netEnabled & NET_ENABLEV4 ) && ( netEnabled & NET_ENABLEV6 );
		if ( !skipWarn )
		{
#ifdef _WIN32
			// "The gai_strerror function is provided for compliance with IETF recommendations, but it is not thread
			// safe. Therefore, use of traditional Windows Sockets functions such as WSAGetLastError is recommended."
			std::string error = Sys::Win32StrError( WSAGetLastError() );
#else
			std::string error = gai_strerror( retval );
#endif
			Log::Notice( "Sys_StringToSockaddr: Error resolving %s: %s", s, error );
		}
	}

	if ( res )
	{
		freeaddrinfo( res );
	}

	return false;
}

/*
=============
Sys_SockaddrToString
=============
*/
void Sys_SockaddrToString( char *dest, int destlen, struct sockaddr *input )
{
	socklen_t inputlen;

	if ( input->sa_family == AF_INET6 )
	{
		inputlen = sizeof( struct sockaddr_in6 );
	}
	else
	{
		inputlen = sizeof( struct sockaddr_in );
	}

	if ( getnameinfo( input, inputlen, dest, destlen, nullptr, 0, NI_NUMERICHOST ) && destlen > 0 )
	{
		*dest = '\0';
	}
}

/*
=============
Sys_StringToAdr
=============
*/
bool Sys_StringToAdr( const char *s, netadr_t *a, netadrtype_t family )
{
	struct sockaddr_storage sadr;

	sa_family_t             fam;

	switch ( family )
	{
		case netadrtype_t::NA_IP:
			fam = AF_INET;
			break;

		case netadrtype_t::NA_IP6:
			fam = AF_INET6;
			break;

		default:
			fam = AF_UNSPEC;
			break;
	}

	if ( !Sys_StringToSockaddr( s, ( struct sockaddr * ) &sadr, sizeof( sadr ), fam ) )
	{
		return false;
	}

	SockadrToNetadr( ( struct sockaddr * ) &sadr, a );
	return true;
}

/*
===================
NET_CompareBaseAdrMask

Compare without port, and up to the bit number given in netmask.
===================
*/
bool NET_CompareBaseAdrMask( const netadr_t& a, const netadr_t& b, int netmask )
{
	byte     cmpmask, *addra, *addrb;
	int      curbyte;

	netadrtype_t a_type = NET_TYPE( a.type );

	if ( a_type != NET_TYPE( b.type ) )
	{
		return false;
	}

	if ( a_type == netadrtype_t::NA_LOOPBACK )
	{
		return true;
	}

	if ( a_type == netadrtype_t::NA_IP )
	{
		addra = ( byte * ) &a.ip;
		addrb = ( byte * ) &b.ip;

		if ( netmask < 0 || netmask > 32 )
		{
			netmask = 32;
		}
	}
	else if ( a_type == netadrtype_t::NA_IP6 )
	{
		addra = ( byte * ) &a.ip6;
		addrb = ( byte * ) &b.ip6;

		if ( netmask < 0 || netmask > 128 )
		{
			netmask = 128;
		}
	}
	else
	{
		Log::Notice( "NET_CompareBaseAdr: bad address type" );
		return false;
	}

	curbyte = netmask >> 3;

	if ( curbyte && memcmp( addra, addrb, curbyte ) )
	{
		return false;
	}

	netmask &= 7;

	if ( netmask )
	{
		cmpmask = ( 1 << netmask ) - 1;
		cmpmask <<= 8 - netmask;

		if ( ( addra[ curbyte ] & cmpmask ) == ( addrb[ curbyte ] & cmpmask ) )
		{
			return true;
		}
	}
	else
	{
		return true;
	}

	return false;
}

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
bool NET_CompareBaseAdr( const netadr_t& a, const netadr_t& b )
{
	return NET_CompareBaseAdrMask( a, b, -1 );
}

const char      *NET_AdrToString( const netadr_t& a )
{
	static  char s[ NET_ADDR_STR_MAX_LEN ];

	if ( a.type == netadrtype_t::NA_LOOPBACK )
	{
		Com_sprintf( s, sizeof( s ), "loopback" );
	}
	else if ( a.type == netadrtype_t::NA_BOT )
	{
		Com_sprintf( s, sizeof( s ), "bot" );
	}
	else if ( a.type == netadrtype_t::NA_IP || a.type == netadrtype_t::NA_IP6 || a.type == netadrtype_t::NA_IP_DUAL )
	{
		struct sockaddr_storage sadr;

		memset( &sadr, 0, sizeof( sadr ) );
		NetadrToSockadr( &a, ( struct sockaddr * ) &sadr );
		Sys_SockaddrToString( s, sizeof( s ), ( struct sockaddr * ) &sadr );
	}

	return s;
}

bool        NET_CompareAdr( const netadr_t& a, const netadr_t& b )
{
	if ( !NET_CompareBaseAdr( a, b ) )
	{
		return false;
	}

	if ( a.type == netadrtype_t::NA_IP || a.type == netadrtype_t::NA_IP6 )
	{
		if ( a.port == b.port )
		{
			return true;
		}
	}
	else
	{
		return true;
	}

	return false;
}

bool        NET_IsLocalAddress( const netadr_t& adr )
{
	return adr.type == netadrtype_t::NA_LOOPBACK;
}

//=============================================================================

/*
==================
Sys_GetPacket

Never called by the game logic, just the system event queuing
==================
*/
bool Sys_GetPacket( netadr_t *net_from, msg_t *net_message )
{
	int                     ret;
	struct sockaddr_storage from;

	socklen_t               fromlen;
	int                     err;

	if ( ip_socket != INVALID_SOCKET )
	{
		fromlen = sizeof( from );
		ret = recvfrom( ip_socket, ( char * ) net_message->data, net_message->maxsize, 0, ( struct sockaddr * ) &from, &fromlen );

		if ( ret == SOCKET_ERROR )
		{
			err = socketError;

			if ( err != net::errc::resource_unavailable_try_again && err != net::errc::connection_reset )
			{
				Log::Notice( "NET_GetPacket: %s", NET_ErrorString() );
			}
		}
		else
		{
			memset( ( ( struct sockaddr_in * ) &from )->sin_zero, 0, 8 );

			if ( usingSocks && memcmp( &from, &socksRelayAddr, fromlen ) == 0 )
			{
				if ( ret < 10 || net_message->data[ 0 ] != 0 || net_message->data[ 1 ] != 0 || net_message->data[ 2 ] != 0 || net_message->data[ 3 ] != 1 )
				{
					return false;
				}

				net_from->type = netadrtype_t::NA_IP;
				net_from->ip[ 0 ] = net_message->data[ 4 ];
				net_from->ip[ 1 ] = net_message->data[ 5 ];
				net_from->ip[ 2 ] = net_message->data[ 6 ];
				net_from->ip[ 3 ] = net_message->data[ 7 ];
				net_from->port = * ( short * ) &net_message->data[ 8 ];
				net_message->readcount = 10;
			}
			else
			{
				SockadrToNetadr( ( struct sockaddr * ) &from, net_from );
				net_message->readcount = 0;
			}

			if ( ret == net_message->maxsize )
			{
				Log::Notice( "Oversize packet from %s", NET_AdrToString( *net_from ) );
				return false;
			}

			net_message->cursize = ret;
			return true;
		}
	}

	if ( ip6_socket != INVALID_SOCKET )
	{
		fromlen = sizeof( from );
		ret = recvfrom( ip6_socket, ( char * ) net_message->data, net_message->maxsize, 0, ( struct sockaddr * ) &from, &fromlen );

		if ( ret == SOCKET_ERROR )
		{
			err = socketError;

			if ( err != net::errc::resource_unavailable_try_again && err != net::errc::connection_reset )
			{
				Log::Notice( "NET_GetPacket: %s", NET_ErrorString() );
			}
		}
		else
		{
			SockadrToNetadr( ( struct sockaddr * ) &from, net_from );
			net_message->readcount = 0;

			if ( ret == net_message->maxsize )
			{
				Log::Notice( "Oversize packet from %s", NET_AdrToString( *net_from ) );
				return false;
			}

			net_message->cursize = ret;
			return true;
		}
	}

	if ( multicast6_socket != INVALID_SOCKET && multicast6_socket != ip6_socket )
	{
		fromlen = sizeof( from );
		ret = recvfrom( multicast6_socket, ( char * ) net_message->data, net_message->maxsize, 0, ( struct sockaddr * ) &from, &fromlen );

		if ( ret == SOCKET_ERROR )
		{
			err = socketError;

			if ( err != net::errc::resource_unavailable_try_again && err != net::errc::connection_reset )
			{
				Log::Notice( "NET_GetPacket: %s", NET_ErrorString() );
			}
		}
		else
		{
			SockadrToNetadr( ( struct sockaddr * ) &from, net_from );
			net_message->readcount = 0;

			if ( ret == net_message->maxsize )
			{
				Log::Notice( "Oversize packet from %s", NET_AdrToString( *net_from ) );
				return false;
			}

			net_message->cursize = ret;
			return true;
		}
	}

	return false;
}

//=============================================================================

static char socksBuf[ 4096 ];

/*
==================
Sys_SendPacket
==================
*/
void Sys_SendPacket( int length, const void *data, const netadr_t& to )
{
	int                     ret = SOCKET_ERROR;
	struct sockaddr_storage addr;

	if ( to.type != netadrtype_t::NA_BROADCAST && to.type != netadrtype_t::NA_IP && to.type != netadrtype_t::NA_IP_DUAL && to.type != netadrtype_t::NA_IP6 && to.type != netadrtype_t::NA_MULTICAST6 )
	{
		Sys::Error( "Sys_SendPacket: bad address type" );
	}

	if ( ( ip_socket == INVALID_SOCKET && NET_IS_IPv4( to.type ) ) ||
	     ( ip_socket == INVALID_SOCKET && to.type == netadrtype_t::NA_BROADCAST ) ||
	     ( ip6_socket == INVALID_SOCKET && NET_IS_IPv6( to.type ) ) ||
	     ( ip6_socket == INVALID_SOCKET && to.type == netadrtype_t::NA_MULTICAST6 ) )
	{
		return;
	}

	if ( to.type == netadrtype_t::NA_MULTICAST6 && ( net_enabled->integer & NET_DISABLEMCAST ) )
	{
		return;
	}

	memset( &addr, 0, sizeof( addr ) );
	NetadrToSockadr( &to, ( struct sockaddr * ) &addr );

	if ( usingSocks && addr.ss_family == AF_INET /*to.type == NA_IP*/ )
	{
		socksBuf[ 0 ] = 0; // reserved
		socksBuf[ 1 ] = 0;
		socksBuf[ 2 ] = 0; // fragment (not fragmented)
		socksBuf[ 3 ] = 1; // address type: IPV4
		* ( int * ) &socksBuf[ 4 ] = ( ( struct sockaddr_in * ) &addr )->sin_addr.s_addr;
		* ( short * ) &socksBuf[ 8 ] = ( ( struct sockaddr_in * ) &addr )->sin_port;
		memcpy( &socksBuf[ 10 ], data, length );
		ret = sendto( ip_socket, ( const char* )socksBuf, length + 10, 0, &socksRelayAddr, sizeof( socksRelayAddr ) );
	}
	else
	{
		if ( addr.ss_family == AF_INET )
		{
			ret = sendto( ip_socket, ( const char* )data, length, 0, ( struct sockaddr * ) &addr, sizeof( struct sockaddr_in ) );
		}
		else if ( addr.ss_family == AF_INET6 )
		{
			ret = sendto( ip6_socket, ( const char* )data, length, 0, ( struct sockaddr * ) &addr, sizeof( struct sockaddr_in6 ) );
		}
	}

	if ( ret == SOCKET_ERROR )
	{
		int err = socketError;

		// wouldblock is silent
		if ( err == net::errc::resource_unavailable_try_again )
		{
			return;
		}

		// some PPP links do not allow broadcasts and return an error
		if ( ( err == net::errc::address_not_available ) && ( ( to.type == netadrtype_t::NA_BROADCAST ) ) )
		{
			return;
		}

		if ( addr.ss_family == AF_INET )
		{
			Log::Notice( "Sys_SendPacket (ipv4): %s", NET_ErrorString() );
		}
		else if ( addr.ss_family == AF_INET6 )
		{
			Log::Notice( "Sys_SendPacket (ipv6): %s", NET_ErrorString() );
		}
		else
		{
			Log::Notice( "Sys_SendPacket (%i): %s", addr.ss_family , NET_ErrorString() );
		}
	}
}

//=============================================================================

/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
bool Sys_IsLANAddress( const netadr_t& adr )
{
	int            index, run, addrsize;
	bool           differed;
	const byte     *compareadr, *comparemask, *compareip;

	if ( adr.type == netadrtype_t::NA_LOOPBACK )
	{
		return true;
	}

	if ( adr.type == netadrtype_t::NA_IP )
	{
		// RFC1918:
		// 10.0.0.0        -   10.255.255.255  (10/8 prefix)
		// 172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
		// 192.168.0.0     -   192.168.255.255 (192.168/16 prefix)

		// 127.0.0.0       -   127.255.255.255 (127/8 prefix)

		if ( adr.ip[ 0 ] == 10 )
		{
			return true;
		}

		if ( adr.ip[ 0 ] == 172 && ( adr.ip[ 1 ] & 0xf0 ) == 16 )
		{
			return true;
		}

		if ( adr.ip[ 0 ] == 192 && adr.ip[ 1 ] == 168 )
		{
			return true;
		}

		if ( adr.ip[ 0 ] == 127 )
		{
			return true;
		}
	}
	else if ( adr.type == netadrtype_t::NA_IP6 )
	{
		if ( adr.ip6[ 0 ] == 0xfe && ( adr.ip6[ 1 ] & 0xc0 ) == 0x80 )
		{
			return true;
		}

		if ( ( adr.ip6[ 0 ] & 0xfe ) == 0xfc )
		{
			return true;
		}
	}

	// Now compare against the networks this computer is member of.
	for ( index = 0; index < numIP; index++ )
	{
		if ( localIP[ index ].type == adr.type )
		{
			if ( adr.type == netadrtype_t::NA_IP )
			{
				compareip = ( byte * ) & ( ( struct sockaddr_in * ) &localIP[ index ].addr )->sin_addr.s_addr;
				comparemask = ( byte * ) & ( ( struct sockaddr_in * ) &localIP[ index ].netmask )->sin_addr.s_addr;
				compareadr = adr.ip;

				addrsize = sizeof( adr.ip );
			}
			else
			{
				// TODO? should we check the scope_id here?

				compareip = ( byte * ) & ( ( struct sockaddr_in6 * ) &localIP[ index ].addr )->sin6_addr;
				comparemask = ( byte * ) & ( ( struct sockaddr_in6 * ) &localIP[ index ].netmask )->sin6_addr;
				compareadr = adr.ip6;

				addrsize = sizeof( adr.ip6 );
			}

			differed = false;

			for ( run = 0; run < addrsize; run++ )
			{
				if ( ( compareip[ run ] & comparemask[ run ] ) != ( compareadr[ run ] & comparemask[ run ] ) )
				{
					differed = true;
					break;
				}
			}

			if ( !differed )
			{
				return true;
			}
		}
	}

	return false;
}

class ShowIPCommand : public Cmd::StaticCmd
{
public:
	ShowIPCommand() : StaticCmd("showip", Cmd::SERVER, "show addresses of network interfaces") {}

	void Run( const Cmd::Args & ) const override
	{
		int  i;
		char addrbuf[ NET_ADDR_STR_MAX_LEN ];

		for ( i = 0; i < numIP; i++ )
		{
			Sys_SockaddrToString( addrbuf, sizeof( addrbuf ), ( struct sockaddr * ) &localIP[ i ].addr );

			if ( localIP[ i ].type == netadrtype_t::NA_IP )
			{
				Print( "IP: %s", addrbuf );
			}
			else if ( localIP[ i ].type == netadrtype_t::NA_IP6 )
			{
				Print( "IP6: %s", addrbuf );
			}
		}
	}
};
static ShowIPCommand showipRegistration;

//=============================================================================

/*
====================
NET_IPSocket
====================
*/
SOCKET NET_IPSocket( const char *net_interface, int port, struct sockaddr_in *bindto, int *err )
{
	SOCKET             newsocket;
	struct sockaddr_in address;

	u_long             _true = 1;
	int                i = 1;

	*err = 0;

	Log::Notice( "Opening IP socket: %s:%s", net_interface ? net_interface : "0.0.0.0", port ? va( "%i", port ) : "*" );

#ifdef _WIN32
	newsocket = WSASocketW( PF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_NO_HANDLE_INHERIT );
#else
	newsocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
#endif

	if ( newsocket == INVALID_SOCKET )
	{
		*err = socketError;
		Log::Warn( "NET_IPSocket: socket: %s", NET_ErrorString() );
		return newsocket;
	}

	// make it non-blocking
	if ( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR )
	{
		Log::Warn( "NET_IPSocket: ioctl FIONBIO: %s", NET_ErrorString() );
		*err = socketError;
		closesocket( newsocket );
		return INVALID_SOCKET;
	}

	// make it broadcast capable
	if ( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, ( char * ) &i, sizeof( i ) ) == SOCKET_ERROR )
	{
		Log::Warn( "NET_IPSocket: setsockopt SO_BROADCAST: %s", NET_ErrorString() );
	}

	if ( !net_interface || !net_interface[ 0 ] )
	{
		memset( &address, 0, sizeof( address ) );
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		if ( !Sys_StringToSockaddr( net_interface, ( struct sockaddr * ) &address, sizeof( address ), AF_INET ) )
		{
			closesocket( newsocket );
			return INVALID_SOCKET;
		}
	}

	address.sin_port = htons( ( short ) port );

	if ( bind( newsocket, ( struct sockaddr * ) &address, sizeof( address ) ) == SOCKET_ERROR )
	{
		Log::Warn( "NET_IPSocket: bind: %s", NET_ErrorString() );
		*err = socketError;
		closesocket( newsocket );
		return INVALID_SOCKET;
	}

	if ( bindto )
	{
		// random port? find what was chosen
		if ( address.sin_port == 0 )
		{
			struct sockaddr_in addr; // enough space
			socklen_t          addrlen = sizeof( addr );

			if ( !getsockname( newsocket, (struct sockaddr *) &addr, &addrlen ) && addrlen )
			{
				address.sin_port = addr.sin_port;
			}
		}

		*bindto = address;
	}

	return newsocket;
}

/*
====================
NET_IP6Socket
====================
*/
SOCKET NET_IP6Socket( const char *net_interface, int port, struct sockaddr_in6 *bindto, int *err )
{
	SOCKET              newsocket;
	struct sockaddr_in6 address;

	u_long              _true = 1;
	bool            brackets = net_interface && Q_CountChar( net_interface, ':' );

	*err = 0;

	// Print the name in brackets if there is a colon:
	Log::Notice( "Opening IP6 socket: %s%s%s:%s", brackets ? "[" : "", net_interface ? net_interface : "[::]", brackets ? "]" : "", port ? va( "%i", port ) : "*" );

#ifdef _WIN32
	newsocket = WSASocketW( PF_INET6, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_NO_HANDLE_INHERIT );
#else
	newsocket = socket( PF_INET6, SOCK_DGRAM, IPPROTO_UDP );
#endif

	if ( newsocket  == INVALID_SOCKET )
	{
		*err = socketError;
		Log::Warn( "NET_IP6Socket: socket: %s", NET_ErrorString() );
		return newsocket;
	}

	// make it non-blocking
	if ( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR )
	{
		Log::Warn( "NET_IP6Socket: ioctl FIONBIO: %s", NET_ErrorString() );
		*err = socketError;
		closesocket( newsocket );
		return INVALID_SOCKET;
	}

#ifdef IPV6_V6ONLY
	{
		int i = 1;

		// IPv4 addresses should not be allowed to connect via this socket.
		if ( setsockopt( newsocket, IPPROTO_IPV6, IPV6_V6ONLY, ( char * ) &i, sizeof( i ) ) == SOCKET_ERROR )
		{
			// win32 systems don't seem to support this anyways.
			Log::Warn( "NET_IP6Socket: setsockopt IPV6_V6ONLY: %s", NET_ErrorString() );
		}
	}
#endif

	if ( !net_interface || !net_interface[ 0 ] )
	{
		memset( &address, 0, sizeof( address ) );
		address.sin6_family = AF_INET6;
		address.sin6_addr = in6addr_any;
	}
	else
	{
		if ( !Sys_StringToSockaddr( net_interface, ( struct sockaddr * ) &address, sizeof( address ), AF_INET6 ) )
		{
			closesocket( newsocket );
			return INVALID_SOCKET;
		}
	}

	address.sin6_port = htons( ( short ) port );

	if ( bind( newsocket, ( struct sockaddr * ) &address, sizeof( address ) ) == SOCKET_ERROR )
	{
		Log::Warn( "NET_IP6Socket: bind: %s", NET_ErrorString() );
		*err = socketError;
		closesocket( newsocket );
		return INVALID_SOCKET;
	}

	if ( bindto )
	{
		// random port? find what was chosen
		if ( address.sin6_port == 0 )
		{
			struct sockaddr_in6 addr; // enough space
			socklen_t           addrlen = sizeof( addr );

			if ( !getsockname( newsocket, (struct sockaddr *) &addr, &addrlen ) && addrlen )
			{
				address.sin6_port = addr.sin6_port;
			}
		}
		*bindto = address;
	}

	return newsocket;
}

/*
====================
NET_SetMulticast6
Set the current multicast group
====================
*/
void NET_SetMulticast6()
{
	struct sockaddr_in6 addr;

	if ( !*net_mcast6addr->string || !Sys_StringToSockaddr( net_mcast6addr->string, ( struct sockaddr * ) &addr, sizeof( addr ), AF_INET6 ) )
	{
		Log::Warn( "NET_JoinMulticast6: Incorrect multicast address given, "
		            "please set cvar %s to a sane value.\n", net_mcast6addr->name );

		Cvar_SetValue( net_enabled->name, net_enabled->integer | NET_DISABLEMCAST );

		return;
	}

	memcpy( &curgroup.ipv6mr_multiaddr, &addr.sin6_addr, sizeof( curgroup.ipv6mr_multiaddr ) );

	if ( *net_mcast6iface->string )
	{
#ifdef _WIN32
		curgroup.ipv6mr_interface = net_mcast6iface->integer;
#else
		curgroup.ipv6mr_interface = if_nametoindex( net_mcast6iface->string );
#endif
	}
	else
	{
		curgroup.ipv6mr_interface = 0;
	}
}

/*
====================
NET_JoinMulticast6
Join an ipv6 multicast group
====================
*/
void NET_JoinMulticast6()
{
	int err;

	if ( ip6_socket == INVALID_SOCKET || multicast6_socket != INVALID_SOCKET || ( net_enabled->integer & NET_DISABLEMCAST ) )
	{
		return;
	}

	if ( IN6_IS_ADDR_MULTICAST( &boundto.sin6_addr ) || IN6_IS_ADDR_UNSPECIFIED( &boundto.sin6_addr ) )
	{
		// The way the socket was bound does not prohibit receiving multi-cast packets. So we don't need to open a new one.
		multicast6_socket = ip6_socket;
	}
	else
	{
		if ( ( multicast6_socket = NET_IP6Socket( net_mcast6addr->string, ntohs( boundto.sin6_port ), nullptr, &err ) ) == INVALID_SOCKET )
		{
			// If the OS does not support binding to multicast addresses, like Windows XP, at least try with a non-multicast socket.
			multicast6_socket = ip6_socket;
		}
	}

	if ( curgroup.ipv6mr_interface )
	{
		if ( setsockopt( multicast6_socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		                 ( char * ) &curgroup.ipv6mr_interface, sizeof( curgroup.ipv6mr_interface ) ) < 0 )
		{
			Log::Notice( "NET_JoinMulticast6: Couldn't set scope on multicast socket: %s", NET_ErrorString() );

			if ( multicast6_socket != ip6_socket )
			{
				closesocket( multicast6_socket );
				multicast6_socket = INVALID_SOCKET;
				return;
			}
		}
	}

	if ( setsockopt( multicast6_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, ( char * ) &curgroup, sizeof( curgroup ) ) )
	{
		Log::Notice( "NET_JoinMulticast6: Couldn't join multicast group: %s", NET_ErrorString() );

		if ( multicast6_socket != ip6_socket )
		{
			closesocket( multicast6_socket );
			multicast6_socket = INVALID_SOCKET;
			return;
		}
	}
}

void NET_LeaveMulticast6()
{
	if ( multicast6_socket != INVALID_SOCKET )
	{
		if ( multicast6_socket != ip6_socket )
		{
			closesocket( multicast6_socket );
		}
		else
		{
			setsockopt( multicast6_socket, IPPROTO_IPV6, IPV6_LEAVE_GROUP, ( char * ) &curgroup, sizeof( curgroup ) );
		}

		multicast6_socket = INVALID_SOCKET;
	}
}

/*
====================
NET_OpenSocks
====================
*/
void NET_OpenSocks( int port )
{
	struct sockaddr_in address;

	struct hostent     *h;

	int                len;
	bool           rfc1929;
	unsigned char      buf[ 64 ];

	usingSocks = false;

	Log::Notice( "Opening connection to SOCKS server." );

#ifdef _WIN32
	socks_socket = WSASocketW( AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_NO_HANDLE_INHERIT );
#else
	socks_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
#endif

	if ( socks_socket == INVALID_SOCKET )
	{
		Log::Warn( "NET_OpenSocks: socket: %s", NET_ErrorString() );
		return;
	}

	h = gethostbyname( net_socksServer->string );

	if ( h == nullptr )
	{
		Log::Warn( "NET_OpenSocks: gethostbyname: %s", NET_ErrorString() );
		return;
	}

	if ( h->h_addrtype != AF_INET )
	{
		Log::Warn( "NET_OpenSocks: gethostbyname: address type was not AF_INET" );
		return;
	}

	memset( &address, 0, sizeof( address ) );
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = * ( int * ) h->h_addr_list[ 0 ];
	address.sin_port = htons( ( short ) net_socksPort->integer );

	if ( connect( socks_socket, ( struct sockaddr * ) &address, sizeof( address ) ) == SOCKET_ERROR )
	{
		Log::Notice( "NET_OpenSocks: connect: %s", NET_ErrorString() );
		return;
	}

	// send socks authentication handshake
	if ( *net_socksUsername->string || *net_socksPassword->string )
	{
		rfc1929 = true;
	}
	else
	{
		rfc1929 = false;
	}

	buf[ 0 ] = 5; // SOCKS version

	// method count
	if ( rfc1929 )
	{
		buf[ 1 ] = 2;
		len = 4;
	}
	else
	{
		buf[ 1 ] = 1;
		len = 3;
	}

	buf[ 2 ] = 0; // method #1 - method id #00: no authentication

	if ( rfc1929 )
	{
		buf[ 2 ] = 2; // method #2 - method id #02: username/password
	}

	if ( send( socks_socket, ( char * ) buf, len, 0 ) == SOCKET_ERROR )
	{
		Log::Notice( "NET_OpenSocks: send: %s", NET_ErrorString() );
		return;
	}

	// get the response
	len = recv( socks_socket, ( char * ) buf, 64, 0 );

	if ( len == SOCKET_ERROR )
	{
		Log::Notice( "NET_OpenSocks: recv: %s", NET_ErrorString() );
		return;
	}

	if ( len != 2 || buf[ 0 ] != 5 )
	{
		Log::Notice( "NET_OpenSocks: bad response" );
		return;
	}

	switch ( buf[ 1 ] )
	{
		case 0: // no authentication
			break;

		case 2: // username/password authentication
			break;

		default:
			Log::Notice( "NET_OpenSocks: request denied" );
			return;
	}

	// do username/password authentication if needed
	if ( buf[ 1 ] == 2 )
	{
		int ulen;
		int plen;

		// build the request
		ulen = strlen( net_socksUsername->string );
		plen = strlen( net_socksPassword->string );

		buf[ 0 ] = 1; // username/password authentication version
		buf[ 1 ] = ulen;

		if ( ulen )
		{
			memcpy( &buf[ 2 ], net_socksUsername->string, ulen );
		}

		buf[ 2 + ulen ] = plen;

		if ( plen )
		{
			memcpy( &buf[ 3 + ulen ], net_socksPassword->string, plen );
		}

		// send it
		if ( send( socks_socket, ( char * ) buf, 3 + ulen + plen, 0 ) == SOCKET_ERROR )
		{
			Log::Notice( "NET_OpenSocks: send: %s", NET_ErrorString() );
			return;
		}

		// get the response
		len = recv( socks_socket, ( char * ) buf, 64, 0 );

		if ( len == SOCKET_ERROR )
		{
			Log::Notice( "NET_OpenSocks: recv: %s", NET_ErrorString() );
			return;
		}

		if ( len != 2 || buf[ 0 ] != 1 )
		{
			Log::Notice( "NET_OpenSocks: bad response" );
			return;
		}

		if ( buf[ 1 ] != 0 )
		{
			Log::Notice( "NET_OpenSocks: authentication failed" );
			return;
		}
	}

	// send the UDP associate request
	buf[ 0 ] = 5; // SOCKS version
	buf[ 1 ] = 3; // command: UDP associate
	buf[ 2 ] = 0; // reserved
	buf[ 3 ] = 1; // address type: IPV4
	* ( int * ) &buf[ 4 ] = INADDR_ANY;
	* ( short * ) &buf[ 8 ] = htons( ( short ) port );  // port

	if ( send( socks_socket, ( char * ) buf, 10, 0 ) == SOCKET_ERROR )
	{
		Log::Notice( "NET_OpenSocks: send: %s", NET_ErrorString() );
		return;
	}

	// get the response
	len = recv( socks_socket, ( char * ) buf, 64, 0 );

	if ( len == SOCKET_ERROR )
	{
		Log::Notice( "NET_OpenSocks: recv: %s", NET_ErrorString() );
		return;
	}

	if ( len < 2 || buf[ 0 ] != 5 )
	{
		Log::Notice( "NET_OpenSocks: bad response" );
		return;
	}

	// check completion code
	if ( buf[ 1 ] != 0 )
	{
		Log::Notice( "NET_OpenSocks: request denied: %i", buf[ 1 ] );
		return;
	}

	if ( buf[ 3 ] != 1 )
	{
		Log::Notice( "NET_OpenSocks: relay address is not IPV4: %i", buf[ 3 ] );
		return;
	}

	memset( &socksRelayAddr, 0, sizeof( socksRelayAddr ) );
	( ( struct sockaddr_in * ) &socksRelayAddr )->sin_family = AF_INET;
	( ( struct sockaddr_in * ) &socksRelayAddr )->sin_addr.s_addr = * ( int * ) &buf[ 4 ];
	( ( struct sockaddr_in * ) &socksRelayAddr )->sin_port = * ( short * ) &buf[ 8 ];
	memset( ( ( struct sockaddr_in * ) &socksRelayAddr )->sin_zero, 0, 8 );

	usingSocks = true;
}

/*
=====================
NET_AddLocalAddress
=====================
*/
static void NET_AddLocalAddress( const char *ifname, struct sockaddr *addr, struct sockaddr *netmask )
{
	int         addrlen;
	sa_family_t family;

	// only add addresses that have all required info.
	if ( !addr || !netmask || !ifname )
	{
		return;
	}

	family = addr->sa_family;

	if ( numIP < MAX_IPS )
	{
		if ( family == AF_INET && ( net_enabled->integer & NET_ENABLEV4 ) )
		{
			addrlen = sizeof( struct sockaddr_in );
			localIP[ numIP ].type = netadrtype_t::NA_IP;
		}
		else if ( family == AF_INET6 && ( net_enabled->integer & NET_ENABLEV6 ) )
		{
			addrlen = sizeof( struct sockaddr_in6 );
			localIP[ numIP ].type = netadrtype_t::NA_IP6;
		}
		else
		{
			return;
		}

		Q_strncpyz( localIP[ numIP ].ifname, ifname, sizeof( localIP[ numIP ].ifname ) );

		localIP[ numIP ].family = family;

		memcpy( &localIP[ numIP ].addr, addr, addrlen );
		memcpy( &localIP[ numIP ].netmask, netmask, addrlen );

		numIP++;
	}
}

#if defined( __linux__ ) || defined( MACOSX ) || defined( __BSD__ )
static void NET_GetLocalAddress()
{
	struct ifaddrs *ifap, *search;

	numIP = 0;

	if ( getifaddrs( &ifap ) )
	{
		Log::Notice( "NET_GetLocalAddress: Unable to get list of network interfaces: %s", NET_ErrorString() );
	}
	else
	{
		for ( search = ifap; search; search = search->ifa_next )
		{
			// Only add interfaces that are up.
			if ( ifap->ifa_flags & IFF_UP )
			{
				NET_AddLocalAddress( search->ifa_name, search->ifa_addr, search->ifa_netmask );
			}
		}

		freeifaddrs( ifap );
	}
}

#else
static void NET_GetLocalAddress()
{
	char            hostname[ 256 ];
	struct addrinfo hint;

	struct addrinfo *res = nullptr;

	numIP = 0;

	if ( gethostname( hostname, 256 ) == SOCKET_ERROR )
	{
		return;
	}

	Log::Notice( "Hostname: %s", hostname );

	memset( &hint, 0, sizeof( hint ) );

	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_DGRAM;

	if ( !getaddrinfo( hostname, nullptr, &hint, &res ) )
	{
		struct sockaddr_in  mask4;

		struct sockaddr_in6 mask6;

		struct addrinfo     *search;

		/* On operating systems where it's more difficult to find out the configured interfaces, we'll just assume a
		 * netmask with all bits set. */

		memset( &mask4, 0, sizeof( mask4 ) );
		memset( &mask6, 0, sizeof( mask6 ) );
		mask4.sin_family = AF_INET;
		memset( &mask4.sin_addr.s_addr, 0xFF, sizeof( mask4.sin_addr.s_addr ) );
		mask6.sin6_family = AF_INET6;
		memset( &mask6.sin6_addr, 0xFF, sizeof( mask6.sin6_addr ) );

		// add all IP addresses from the returned list.
		for ( search = res; search; search = search->ai_next )
		{
			if ( search->ai_family == AF_INET )
			{
				NET_AddLocalAddress( "", search->ai_addr, ( struct sockaddr * ) &mask4 );
			}
			else if ( search->ai_family == AF_INET6 )
			{
				NET_AddLocalAddress( "", search->ai_addr, ( struct sockaddr * ) &mask6 );
			}
		}
	}

	if ( res )
	{
		freeaddrinfo( res );
	}
}

#endif

static const int MAX_TRY_PORTS = 10;

static int NET_EnsureValidPortNo( int port )
{
	if ( port == PORT_ANY )
	{
		return PORT_ANY;
	}

	port &= 0xFFFF;
	return ( port < 1024 ) ? 1024 : port; // the usual 1024 reserved ports
}

/*
====================
NET_OpenIP
====================
*/
static void NET_OpenIP( bool serverMode )
{
	int i;
	int err = 0;
	int port = PORT_ANY;
	int port6 = PORT_ANY;

	struct sockaddr_in boundto4;

	if ( serverMode )
	{
		port = NET_EnsureValidPortNo( net_port->integer );
		port6 = NET_EnsureValidPortNo( net_port6->integer );
	}

	NET_GetLocalAddress();

	// automatically scan for a valid port, so multiple
	// dedicated servers can be started without requiring
	// a different net_port for each one

	if ( net_enabled->integer & NET_ENABLEV6 )
	{
		for ( i = ( port6 == PORT_ANY ? 1 : MAX_TRY_PORTS ); i; i-- )
		{
			ip6_socket = NET_IP6Socket( net_ip6->string, port6, &boundto, &err );

			if ( ip6_socket != INVALID_SOCKET )
			{
				port6 = ntohs( boundto.sin6_port );
				break;
			}
			else if ( err == net::errc::address_family_not_supported )
			{
				port6 = PORT_ANY;
				break;
			}

			port6 = ( port6 == PORT_ANY ) ? port6 : NET_EnsureValidPortNo( port6 + 1 );
		}

		if ( ip6_socket == INVALID_SOCKET || err == net::errc::address_family_not_supported )
		{
			Log::Warn( "Couldn't bind to an IPv6 address." );
			port6 = PORT_ANY;
		}
	}

	if ( net_enabled->integer & NET_ENABLEV4 )
	{
		for ( i = ( port == PORT_ANY ? 1 : MAX_TRY_PORTS ); i; i-- )
		{
			ip_socket = NET_IPSocket( net_ip->string, port, &boundto4, &err );

			if ( ip_socket != INVALID_SOCKET )
			{
				port = ntohs( boundto4.sin_port );
				break;
			}
			else if ( err == net::errc::address_family_not_supported )
			{
				port = PORT_ANY;
				break;
			}

			port = ( port == PORT_ANY ) ? port : NET_EnsureValidPortNo( port + 1 );
		}

		if ( ip_socket == INVALID_SOCKET )
		{
			Log::Warn( "Couldn't bind to an IPv4 address." );
			port = PORT_ANY;
		}
	}

	if ( port != PORT_ANY && net_socksEnabled->integer )
	{
		NET_OpenSocks( port );
	}

	Cvar_Set( "net_currentPort", va( "%i", port ) );
	Cvar_Set( "net_currentPort6", va( "%i", port6 ) );
}

//===================================================================

/*
====================
NET_GetCvars
====================
*/
static void NET_GetCvars()
{
#ifdef BUILD_SERVER
	// I want server owners to explicitly turn on IPv6 support.
	net_enabled = Cvar_Get( "net_enabled", "1", CVAR_LATCH  );
#else

	/* End users have it enabled so they can connect to IPv6-only hosts, but IPv4 will be
	 * used if available due to ping */
	net_enabled = Cvar_Get( "net_enabled", "3", CVAR_LATCH  );
#endif

	net_ip = Cvar_Get( "net_ip", "0.0.0.0", CVAR_LATCH );
	net_ip6 = Cvar_Get( "net_ip6", "::", CVAR_LATCH );
	net_port = Cvar_Get( "net_port", va( "%i", PORT_SERVER ), CVAR_LATCH );
	net_port6 = Cvar_Get( "net_port6", va( "%i", PORT_SERVER ), CVAR_LATCH );

	// Some cvars for configuring multicast options which facilitates scanning for servers on local subnets.
	net_mcast6addr = Cvar_Get( "net_mcast6addr", NET_MULTICAST_IP6, CVAR_LATCH  );
#ifdef _WIN32
	net_mcast6iface = Cvar_Get( "net_mcast6iface", "0", CVAR_LATCH  );
#else
	net_mcast6iface = Cvar_Get( "net_mcast6iface", "", CVAR_LATCH  );
#endif

	net_socksEnabled = Cvar_Get( "net_socksEnabled", "0", CVAR_LATCH  );
	net_socksServer = Cvar_Get( "net_socksServer", "", CVAR_LATCH  );
	net_socksPort = Cvar_Get( "net_socksPort", "1080", CVAR_LATCH  );
	net_socksUsername = Cvar_Get( "net_socksUsername", "", CVAR_LATCH  );
	net_socksPassword = Cvar_Get( "net_socksPassword", "", CVAR_LATCH  );
}

void NET_EnableNetworking( bool serverMode )
{
	// get any latched changes to cvars
	NET_GetCvars();

	// always cycle off and on because this function is only called on a state change or forced restart
	NET_DisableNetworking();

	if ( !( net_enabled->integer & ( NET_ENABLEV4 | NET_ENABLEV6 ) ) )
	{
		return;
	}

	networkingEnabled = true;

	NET_OpenIP( serverMode );
	NET_SetMulticast6();
	SV_NET_Config();
}

void NET_DisableNetworking()
{
	if ( !networkingEnabled )
	{
		return;
	}

	networkingEnabled = false;

	if ( ip_socket != INVALID_SOCKET )
	{
		closesocket( ip_socket );
		ip_socket = INVALID_SOCKET;
	}

	if ( multicast6_socket != INVALID_SOCKET )
	{
		if ( multicast6_socket != ip6_socket )
		{
			closesocket( multicast6_socket );
		}

		multicast6_socket = INVALID_SOCKET;
	}

	if ( ip6_socket != INVALID_SOCKET )
	{
		closesocket( ip6_socket );
		ip6_socket = INVALID_SOCKET;
	}

	if ( socks_socket != INVALID_SOCKET )
	{
		closesocket( socks_socket );
		socks_socket = INVALID_SOCKET;
	}
}

/*
====================
NET_Init
====================
*/
void NET_Init()
{
#ifdef _WIN32
	int r;

	r = WSAStartup( MAKEWORD( 1, 1 ), &winsockdata );

	if ( r )
	{
		Log::Warn( "Winsock initialization failed, returned %d", r );
		return;
	}

	winsockInitialized = true;
	Log::Notice( "Winsock Initialized" );
#endif

	NET_EnableNetworking( Application::GetTraits().isServer );

	Cmd_AddCommand( "net_restart", NET_Restart_f );
}

/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown()
{
	NET_DisableNetworking();

#ifdef _WIN32
	if ( winsockInitialized )
	{
		WSACleanup();
		winsockInitialized = false;
	}
#endif
}

/*
====================
NET_Sleep

Sleeps msec or until something happens on the network
====================
*/
void NET_Sleep( int msec )
{
	struct timeval timeout;

	fd_set         fdset;
	SOCKET         highestfd = INVALID_SOCKET;

	if ( ip_socket == INVALID_SOCKET && ip6_socket == INVALID_SOCKET )
	{
		return;
	}

	if ( msec < 0 )
	{
		return;
	}

	FD_ZERO( &fdset );

	if ( ip_socket != INVALID_SOCKET )
	{
		FD_SET( ip_socket, &fdset );

		highestfd = ip_socket;
	}

	if ( ip6_socket != INVALID_SOCKET )
	{
		FD_SET( ip6_socket, &fdset );

		if ( highestfd == INVALID_SOCKET || ip6_socket > highestfd )
		{
			highestfd = ip6_socket;
		}
	}

	timeout.tv_sec = msec / 1000;
	timeout.tv_usec = ( msec % 1000 ) * 1000;
	select( highestfd + 1, &fdset, nullptr, nullptr, &timeout );
}

/*
====================
NET_Restart_f
====================
*/
void NET_Restart_f()
{
	NET_DisableNetworking();
	SV_NET_Config();
	Net::ShutDownDNS();
#ifdef BUILD_SERVER
	NET_EnableNetworking( true );
#else
	NET_EnableNetworking( !!com_sv_running->integer );
#endif
}
